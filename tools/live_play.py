#!/usr/bin/env python3
"""live_play.py — PLAY the Tomba! 2 port over its live debug server, and report what it did.

    uv run --frozen python tools/live_play.py
    uv run --frozen python tools/live_play.py --hold right --seconds 10 --port 5983
    uv run --frozen python tools/live_play.py --binary build/bin/tomba2_port

WHY THIS IS NOT `tools/gate.py`. The gate blocks the product between REPL commands, which is the
right tool for stepping to a state and reading it. This one leaves the product running at full speed
and talks to it the way a player would be watched: a pad edge delivered across real PRESENTED
frames, guest state sampled while the picture keeps updating, and a screenshot of what is actually
on screen. That is the only way to see a defect that needs the game to be RUNNING — a producer that
refuses after twenty seconds of play, a picture that goes black while walking, a frame-time cliff —
rather than a state that happens to be parked at a barrier.

THE MENU SEQUENCE IS NOT WRITTEN HERE. `tools/title_prompts.py` owns which button a screen is asking
for, and `tools/oracle_tomba2.py` asks the same module, so the live route and the two-core
comparison cannot answer different prompts. This file is the transport: how a duty cycle becomes
`tap`/`press`/`release` over a socket, and what the run reports.

THE LAUNCH ENVIRONMENT IS NOT WRITTEN HERE EITHER. `tools/gate.py` owns it (psxport's
`agent_environment`, this repo's disc resolution, and the tracked settings file), so this tool cannot
drift from the gate's idea of a headless agent run. It adds exactly two things: PSXPORT_REPL popped
(nothing here speaks that protocol, and a product waiting for a prompt would never present) and
PSXPORT_DEBUG_SERVER naming this run's own port. The endpoint also LIFTS the product's frame cap,
which is what a driven run needs and an unattended one does not.

ONE MEASUREMENT SHAPED THE LOOP, so it is stated here rather than discovered again: a command over
this endpoint is serviced once per PRESENTED frame, and a round trip costs about 50 ms on an idle
title screen and considerably more while the attract demo is streaming overlays. The route therefore
reads the WHOLE task-0 slot in ONE command (the stage entry, the six halfword state machine and the
menu cursor all live inside it) and spends its remaining round trips on the frame counter and the
tap. A driver that spends seven round trips per decision cannot hit a title window that is a dozen
frames wide, and its duty cycle silently stretches by the latency.

REFUSALS, never a degraded run: an endpoint that never answers, a command the binary does not
implement, a reply that came back as the server's own timeout notice, a route leg that never
arrives, a held input that did not move the player, and a run with zero executed guest blocks.
"""

from __future__ import annotations

import argparse
import hashlib
import signal
import subprocess
import sys
import time
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
TOOLS = REPO / "tools"
sys.path.insert(0, str(REPO / "external" / "psxport" / "tools"))
sys.path.insert(0, str(TOOLS))

# tools/gate.py owns the product launch environment; tools/title_prompts.py owns which button a
# screen is asking for; dbgclient is the framework's one protocol implementation.
import gate
import title_prompts
from dbgclient import LiveClient

OUT_DIR = REPO / "scratch" / "live"
LOG = OUT_DIR / "live_play.log"
SERVER_TIMEOUT = "(debug server:"
# The attract-demo flag is sampled this often (in polls) as well as at every leg arrival, so the run
# reports what it read in BOTH the attract phase and gameplay. One value in both is a discriminator
# that separates nothing, and saying so needs both classes.
ATTRACT_EVERY_POLLS = 32
# How often the route re-reads the presented-frame counter to police its own budget. Every read is a
# round trip, and the budget only needs to be approximate between reads; the denominator the run
# REPORTS is always read exactly, at the leg arrivals and at the end.
FRAME_EVERY_POLLS = 8


class Refusal(SystemExit):
    """A run that cannot honestly report the thing it was asked to report."""


def ask(client: LiveClient, line: str) -> str:
    """One command, refusing the two replies that are NOT answers.

    The server's own timeout notice is a sentence about the debug server, and an empty reply is a
    socket that closed mid-run. Either one, read as data, is how a tool ends up reporting zeros it
    never measured. A `? <line>` reply is the third: the product does not implement that command,
    which is a fact about the BINARY (see --binary), not about the run, and it must not be reported
    as a zero either.
    """
    reply = client.send(line)
    if reply.startswith(SERVER_TIMEOUT):
        raise Refusal(f"REFUSED: the endpoint timed out servicing `{line}` — the product was not "
                      f"presenting. Reply: {reply.strip()}")
    if not reply.strip():
        raise Refusal(f"REFUSED: `{line}` came back EMPTY — the endpoint closed or stopped answering "
                      f"mid-run. Nothing below this line was measured.")
    if reply.lstrip().startswith("?"):
        raise Refusal(f"REFUSED: this product does not implement `{line.split()[0]}` "
                      f"({reply.strip()}). The endpoint's command set is the BINARY's, so this is a "
                      f"stale build, not a zero: rebuild the tree this run used, or point --binary "
                      f"at a build made against a framework that has the command.")
    return reply


def byte_at(client: LiveClient, address: int) -> int:
    """One guest RAM byte. The reply is `ADDR: HH HH ..`; only the first byte is asked for."""
    reply = ask(client, f"r {address:08X} 1")
    try:
        return int(reply.rsplit(":", 1)[1].split()[0], 16)
    except (IndexError, ValueError) as error:
        raise Refusal(f"REFUSED: r {address:08X} returned {reply.strip()!r}") from error


def fixed_16_16(words: list[int]) -> str:
    return "[" + ", ".join(f"{word / 65536.0:.3f}" for word in words) + "]"


def display_path(path: Path) -> str:
    """Repo-relative when it is inside the repository, absolute otherwise. A run's report names what it
    touched, and an absolute machine path in a report is a path nobody else can open."""
    try:
        return str(path.relative_to(REPO))
    except ValueError:
        return str(path)


def resolved_framework(build: Path) -> str:
    """Which framework commit the build tree resolved when CMake configured it.

    Printed because it decides what the numbers below DESCRIBE. Two trees in this repository are
    configured against two different framework commits (the recorded pin and the dev clone), and a
    play-through's evidence belongs to the framework that produced it — a run whose numbers are
    later read as describing framework HEAD when it described the pin is exactly the mistake
    `psxport.pin` exists to prevent.
    """
    resolved = build / "psxport_resolved.txt"
    if not resolved.is_file():
        return "(this build tree records no psxport_resolved.txt)"
    fields = {key.strip(): value.strip() for key, value in
              (line.split("=", 1) for line in
               resolved.read_text(encoding="utf-8", errors="replace").splitlines() if "=" in line)}
    return f"{fields.get('commit', '?')[:12]} ({fields.get('dir', '?')})"


def binary_identity(path: Path) -> dict[str, str]:
    """md5 + mtime, from the framework's one binary-identity owner, so this tool cannot disagree
    with the gate about which build produced a run."""
    sys.path.insert(0, str(Path(gate.PSXPORT) / "tools" / "oracle"))
    from compare import binary_identity as identity

    return identity(path)


def launch(port: int, binary: Path, log: Path) -> subprocess.Popen:
    """The product, headless and silent and unpaced, with the live endpoint on this run's own port.

    `gate.native_environment` is the one launch policy in this repository; it also names the tracked
    settings file, which is what keeps an agent run from being configured by whichever untracked
    psxport_settings.ini happens to sit beside the binary.
    """
    environment = gate.native_environment(watchdog=3600)
    environment.pop("PSXPORT_REPL", None)
    environment["PSXPORT_DEBUG_SERVER"] = str(port)
    environment["PSXPORT_LOG_FILE"] = str(log)
    log.parent.mkdir(parents=True, exist_ok=True)
    log.write_text("")  # the logger appends, and "last run" must be one run
    image = Path(gate.EXE)
    for required in (binary, image):
        if not required.is_file():
            raise Refusal(f"REFUSED: {required} is missing — NOTHING WAS RUN. Build with "
                          f"`cmake --build {Path(binary).parent.parent} --target tomba2_port "
                          f"-j$(nproc)` and provision the title first; this tool does not build and "
                          f"does not extract.")
    return subprocess.Popen([str(binary), str(image)], cwd=gate.REPO, env=environment,
                            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def connect(port: int, seconds: float, log: Path) -> LiveClient:
    """Wait for the endpoint, and REFUSE with the product's own log if it never answers. A client
    that cannot connect must not fall back to anything else: "no live session" is the finding, and a
    run that quietly used another path would report play evidence it does not have."""
    deadline = time.monotonic() + seconds
    last: OSError | None = None
    while time.monotonic() < deadline:
        try:
            return LiveClient(port)
        except OSError as error:
            last = error
            # Tight on purpose: the endpoint answers from the FIRST presented frame, and this title's
            # two-page title menu is answered by presses that have to land in the first few hundred
            # frames of it. A slow retry loop connects late, into a screen that is already waiting.
            time.sleep(0.02)
    raise Refusal(f"REFUSED: the live endpoint never answered on 127.0.0.1:{port} within {seconds:.0f}s "
                  f"({last}). The product's own log is {log}.")


class Session:
    """The play-through: reach gameplay the way a player does, then hold what it is told to and report
    what it saw. Every step counts what it did, so a run that never got past the title says so with a
    number rather than a zero."""

    def __init__(self, client: LiveClient) -> None:
        self.client = client
        self.answers = 0     # menu answers delivered (one `tap`)
        self.presses = 0     # pad commands issued, taps and holds/releases both
        self.polls = 0       # observations, i.e. round trips spent deciding
        self.attract_seen: dict[str, set[int]] = {}
        self.first_frame = self.frame()
        self.frames_seen = self.first_frame

    # ---- observation -------------------------------------------------------------------------
    def frame(self) -> int:
        return int(ask(self.client, "frame").split("frame=")[1].split()[0])

    def observe(self) -> tuple[title_prompts.Screen, int]:
        """The whole task-0 slot in ONE command, decoded by title_prompts: the stage entry, the six
        halfwords of the state machine, and the title menu's cursor byte. One round trip per decision
        is not an optimisation, it is what keeps the duty cycle inside the title's input window."""
        raw = bytes(int(value, 16) for value in
                    ask(self.client, f"r {title_prompts.TASK0:08X} "
                                     f"{title_prompts.TASK0_PROBE_BYTES}").rsplit(":", 1)[1].split())
        self.polls += 1
        return title_prompts.screen_from_task0(raw)

    def pad_frames(self) -> int:
        """The product's own count of pad service frames, from the live capture ring.

        This is the PORT's input clock, and it is NOT a game tick: measured over a hold window it
        advances exactly once per presented frame, because the pad is serviced once per presented
        frame (game/core/frame_driver.cpp). It is reported because that measurement is what says so,
        and because "the guest's input clock and the presented clock are the same clock" is a fact a
        reader would otherwise have to take on trust. The two scratchpad words that look like guest
        tick counters are not: 0x1F800160 is a world coordinate the native pool writes
        (game/world/pool.cpp) and 0x1F80017C is read only as a blink phase
        (game/render/field_hud.cpp) and advances thousands of times per presented frame.
        """
        reply = ask(self.client, "padrec")
        return int(reply.split()[1])

    def guest_counters(self) -> dict[str, int] | None:
        """The dynarec's counters, or None when this binary's endpoint has no `guest` command. Used
        to measure the GUEST's own work across the hold window, which is the only per-game-work
        measure this endpoint exposes and therefore the honest stand-in for "game ticks"."""
        try:
            return guest_execution(self.client)[0].get("guest")
        except Refusal:
            return None

    def player_position(self) -> list[int]:
        reply = ask(self.client, f"rw {title_prompts.PLAYER_POSITION:08X} "
                                 f"{title_prompts.PLAYER_POSITION_WORDS}")
        return [int(value, 16) for value in reply.rsplit(":", 1)[1].split()]

    def sample_attract(self, phase: str, *, force: bool = False) -> None:
        if not force and self.polls % ATTRACT_EVERY_POLLS:
            return
        self.attract_seen.setdefault(phase, set()).add(byte_at(self.client, title_prompts.ATTRACT_FLAG))

    # ---- input --------------------------------------------------------------------------------
    def tap(self, button: str, frames: int) -> None:
        """A pad EDGE that spans `frames` presented frames, and the ONLY shape that reached this title's
        front end. Measured on the same build: 12 `tap cross 6` presses walked boot → GAME (main menu
        entered six times), while 12 `press cross` + 0.3 s + `release cross` pairs produced no main-menu
        entry and no GAME in 12000 presented frames. So a menu answer is a tap, and a held input — which
        is what movement needs — is `press`/`release` in play_window. (`tap` and a press+release inside
        one frame are not the same thing: the latter can be serviced inside a single frame and be
        invisible to the guest, which is the hazard this endpoint's own client documents.)"""
        ask(self.client, f"tap {button} {frames}")
        self.answers += 1
        self.presses += 1

    # ---- the route ----------------------------------------------------------------------------
    def drive_to_gameplay(self, budget: int, timeout: float, settle: int) -> int:
        """Cold boot -> the GAME stage -> the field -> the player holding the pad, in title_prompts'
        legs. Each leg is asked the same question the oracle comparison asks; only the transport
        differs, so a leg this route cannot answer is one the comparison cannot answer either.

        WHY A LIVE PRESS WAITS, instead of pressing on the step's period alone. Two measured facts
        about driving this title over this endpoint, both of which cost a run to learn:

        * A press in the FIRST frames of a screen is dropped. The title's own verified sequence
          (docs/tomba2-newgame.md §1) is "run 40 to let the menu settle, tap Cross, run 60, tap
          Cross" — and measured: 97 presses spread over 40000 presented frames, every one of them
          inside the first few frames of the screen it was answering, left the front end alternating
          between the title and the main menu for the whole run and never once entering the game.
        * Pressing with no regard for the screen at all is worse, because the front end has a cancel
          arm: game/scene/demo.cpp's own s3 handler routes a third outcome back to the title. So a
          blind cadence trades the two pages for each other.

        So the live transport presses no sooner than one POLL after the screen it is answering last
        changed, and at most once per poll. A poll is one round trip, measured at 55-124 presented
        frames per poll across the runs that reached the game — always more than the 40 frames the
        title's own sequence waits, which is why the settle is counted in polls here and in frames
        there. The oracle comparison keeps its blind per-frame cadence untouched: it steps one game
        frame at a time from boot, resolves the whole two-page title inside 25 frames, and its
        recorded checkpoints are built on exactly that input.

        WHY ONE ROUND TRIP PER DECISION. A poll used to cost two (the observation plus the frame
        counter), and the frame counter buys precision the transport does not have: the title's accept
        window is narrower than the round-trip cost of asking, so a second round trip per decision
        halves the number of presses that fit inside one screen (measured 1-2 of them) and doubles the
        wall clock. The frame counter is still read every FRAME_EVERY_POLLS polls for the budget and
        the denominators, and the measured frames-per-poll is printed with the route.
        """
        started = time.monotonic()
        for index, step in enumerate(title_prompts.GAMEPLAY_ROUTE):
            leg_frame = self.frame()
            leg_poll = self.polls
            changed_at_poll = self.polls
            last_press_poll: int | None = None
            last_screen: title_prompts.Screen | None = None
            phase = "attract/title" if step is title_prompts.GAMEPLAY_ROUTE[0] else "gameplay"
            while True:
                screen, cursor = self.observe()
                if last_screen is not None and screen != last_screen:
                    changed_at_poll = self.polls
                last_screen = screen
                if step.reached(screen):
                    self.sample_attract(phase, force=True)
                    arrived = self.frame()
                    polls = self.polls - leg_poll
                    print(f"[live] leg {index + 1} of {len(title_prompts.GAMEPLAY_ROUTE)} "
                          f"'{step.name}' reached after {arrived - leg_frame} presented frames in "
                          f"{polls} observation(s) ({(arrived - leg_frame) / max(1, polls):.0f} frames "
                          f"per observation, {self.answers} menu answers so far): "
                          f"{screen.describe()} cursor={cursor}")
                    break
                settled = (self.polls - changed_at_poll) >= settle
                due = last_press_poll is None or self.polls > last_press_poll
                if step.button and settled and due:
                    self.tap(step.button, step.width)
                    last_press_poll = self.polls
                self.sample_attract(phase)
                if self.polls % FRAME_EVERY_POLLS == 0:
                    self.frames_seen = self.frame()
                spent = self.frames_seen - self.first_frame
                if spent > budget:
                    raise Refusal(f"REFUSED: leg {index + 1} of "
                                  f"{len(title_prompts.GAMEPLAY_ROUTE)} ('{step.name}') never arrived "
                                  f"within {budget} presented frames ({self.answers} menu answers, "
                                  f"{self.polls} observations); the product was last at "
                                  f"{screen.describe()} cursor={cursor}.")
                if time.monotonic() - started > timeout:
                    raise Refusal(f"REFUSED: leg {index + 1} of "
                                  f"{len(title_prompts.GAMEPLAY_ROUTE)} ('{step.name}') never arrived "
                                  f"within {timeout:.0f}s of wall clock ({self.answers} menu answers, "
                                  f"{self.polls} observations); the product was last at "
                                  f"{screen.describe()} cursor={cursor}.")
        return self.frame() - self.first_frame

    def play_window(self, buttons: list[str], seconds: float) -> dict:
        """Hold buttons for a wall-clock window while the game runs, and report what moved. Held input
        is what a player does; a tap is a menu answer. Every clock this endpoint exposes is sampled
        across the window, including the dynarec's own counters, so "the game kept running while the
        picture was being sampled" is a number rather than an assumption."""
        for button in buttons:
            ask(self.client, f"press {button}")
            self.presses += 1
        before_frame = self.frame()
        before_pad = self.pad_frames()
        before_guest = self.guest_counters() or {}
        before_position = self.player_position()
        samples = 0
        started = time.monotonic()
        while time.monotonic() - started < seconds:
            self.frame()
            time.sleep(0.05)
            samples += 1
        for button in buttons:
            ask(self.client, f"release {button}")
            self.presses += 1
        after_guest = self.guest_counters() or {}
        return {
            "buttons": buttons,
            "seconds": round(time.monotonic() - started, 2),
            "presented_frames": self.frame() - before_frame,
            "pad_frames": self.pad_frames() - before_pad,
            "samples": samples,
            "position_before": before_position,
            "position_after": self.player_position(),
            "guest_before": before_guest,
            "guest_after": after_guest,
        }


def capture(client: LiveClient, path: Path) -> dict:
    """Photograph what is PRESENTED, and read the picture's own size back out of the file.

    `.ppm` on purpose: the P6 header states the width in the file, so the capture is its own evidence
    of how wide the picture is, independent of what the log announced about it.
    """
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.exists():
        path.unlink()
    reply = ask(client, f"shot {path}").strip()
    if not path.is_file():
        raise Refusal(f"REFUSED: the endpoint reported `{reply}` but wrote no file at {path}. A "
                      f"capture that does not exist is not evidence of a picture.")
    size = path.stat().st_size
    payload = path.read_bytes()
    header = payload[:32]
    width = height = 0
    if header.startswith(b"P6"):
        fields = header.split()
        if len(fields) >= 4:
            width, height = int(fields[1]), int(fields[2])
    return {"path": path, "reply": reply, "bytes": size, "width": width, "height": height,
            "digest": hashlib.md5(payload).hexdigest(),
            "format": "P6 ppm" if header.startswith(b"P6") else repr(header[:4])}


def effective_configuration(client: LiveClient) -> dict:
    """The configuration the product is actually running, asked of the configuration owner over the
    endpoint. This is a QUERY, not a log read: the boot log prints each knob once, at boot, and the
    picture's mode can change afterwards, so a log line is a statement about the past and this is the
    answer as it stands. `cvars` also carries the layer each value came from, which is what says
    whether a run tested what it meant to test."""
    reply = ask(client, "cvars")
    knobs: dict[str, str] = {}
    layers: dict[str, str] = {}
    for line in reply.splitlines():
        fields = line.split()
        # "  NAME  kind = value [layer]" — the name is the first token, the layer the bracketed one.
        if len(fields) < 4 or fields[2] != "=":
            continue
        layer = next((index for index in range(3, len(fields)) if fields[index].startswith("[")), None)
        if layer is None:
            continue
        knobs[fields[0]] = " ".join(fields[3:layer])
        layers[fields[0]] = fields[layer].strip("[]")
    return {
        "knobs": knobs,
        "layers": layers,
        "env_audit": next((line.strip() for line in reply.splitlines()
                           if line.startswith("env audit:")), "(no audit line)"),
        "unmatched": sorted({line.split("UNKNOWN ", 1)[1].split()[0]
                             for line in reply.splitlines() if line.strip().startswith("UNKNOWN ")}),
    }


def guest_execution(client: LiveClient) -> tuple[dict[str, dict[str, int]], str]:
    """The dynarec's own denominators for this run, live. A gameplay claim needs nonzero translated
    and executed blocks with the fallback accounted for; these are the numbers that say so, asked of
    the process rather than recovered from text it happened to print.

    The reply carries TWO lines, `guest:` and `fallback:`, and BOTH name `calls` and `instructions`.
    They are kept apart by their prefix rather than merged into one dict, because a merged dict
    answers both questions with whichever line was parsed last: on a run with zero fallback, the
    merged reading of `instructions` is the count the DYNAREC executed. The raw reply is returned as
    well, because a parsed number nobody can check is a number nobody can refute.
    """
    reply = ask(client, "guest")
    counters: dict[str, dict[str, int]] = {}
    for line in reply.splitlines():
        prefix, separator, rest = line.partition(":")
        prefix = prefix.strip()
        if not separator or prefix not in ("guest", "fallback"):
            continue
        counters.setdefault(prefix, {})
        for token in rest.split():
            key, equals, value = token.partition("=")
            if equals:
                counters[prefix][key] = int(value) if value.isdigit() else -1
    if "guest" not in counters:
        raise Refusal(f"REFUSED: the `guest` reply carried no counters: {reply.strip()!r}")
    return counters, reply.strip()


def log_facts(log: Path) -> dict:
    """What the PRODUCT said about its own picture, read from the log this run owns. The `[wide]`
    announcement is a log line by construction — it is the framework's on-change statement of the
    geometry a run resolved — so the log is where it lives; everything else in this report is asked
    over the endpoint instead."""
    facts: dict[str, list[str]] = {"wide": [], "wide_warn": [], "fps60": []}
    if not log.is_file():
        return facts
    for line in log.read_text(encoding="utf-8", errors="replace").splitlines():
        if "[wide:" in line:
            facts["wide_warn"].append(line.strip())
        elif "[wide]" in line:
            facts["wide"].append(line.strip())
        elif "[fps60]" in line:
            facts["fps60"].append(line.strip())
    return facts


def report(client: LiveClient, log: Path) -> None:
    """The configuration, the dynarec denominators, and the widescreen proof — and a refusal if the
    run executed no guest blocks, because a play-through with no dynarec execution is not gameplay
    evidence however good the screenshot looks."""
    configuration = effective_configuration(client)
    counters, raw_guest = guest_execution(client)
    dynarec = counters["guest"]
    fallback = counters.get("fallback", {})
    executed = dynarec.get("executed_blocks", 0)
    facts = log_facts(log)

    print("[live] effective configuration, asked of the product over `cvars`:")
    for name in ("PSXPORT_SETTINGS", "PSXPORT_DEBUG_SERVER", "PSXPORT_FPS60", "PSXPORT_RENDER_PATH",
                 "PSXPORT_VK_HEADLESS", "PSXPORT_NOPACE", "PSXPORT_NOAUDIO", "PSXPORT_REPL"):
        if name in configuration["knobs"]:
            print(f"[live]   {name} = {configuration['knobs'][name]} [{configuration['layers'][name]}]")
    print(f"[live]   {configuration['env_audit']}")
    if configuration["unmatched"]:
        print(f"[live]   env variables that matched no knob (they did NOTHING this run): "
              f"{', '.join(configuration['unmatched'])}")

    print(f"[live] `guest`, verbatim: {raw_guest}")
    print(f"[live] dynarec denominators: {executed} executed blocks of "
          f"{dynarec.get('translated_blocks', 0)} translated, "
          f"{dynarec.get('executed_instructions', 0)} guest instructions in "
          f"{dynarec.get('calls', 0)} executor calls, {dynarec.get('host_dispatches', 0)} host "
          f"dispatches, cache {dynarec.get('cache_hits', 0)} hits / {dynarec.get('cache_misses', 0)} "
          f"misses, {dynarec.get('invalidations', 0)} invalidations, {dynarec.get('faults', 0)} faults")
    print(f"[live] interpreter fallback: {fallback.get('calls', -1)} calls, "
          f"{fallback.get('instructions', -1)} instructions, "
          f"{fallback.get('refused_calls', -1)} refused "
          f"(compilation_failed={fallback.get('compilation_failed', -1)} "
          f"self_modifying_code={fallback.get('self_modifying_code', -1)} "
          f"unsupported_block={fallback.get('unsupported_block', -1)} "
          f"load_delay_hazard={fallback.get('load_delay_hazard', -1)} "
          f"unsafe_instruction_fetch={fallback.get('unsafe_instruction_fetch', -1)})")

    for line in facts["wide"] + facts["wide_warn"] + facts["fps60"]:
        print(f"[live] {line}")
    widened = None
    if facts["wide"]:
        fields = dict(part.split("=", 1) for part in facts["wide"][-1].split() if "=" in part)
        if "render_width" in fields and "native_width" in fields:
            widened = int(fields["render_width"]) > int(fields["native_width"])
            print(f"[live] widescreen: render_width {fields['render_width']} > native_width "
                  f"{fields['native_width']} is {widened}")
    if facts["wide"] and widened is not True:
        raise Refusal("REFUSED: the run announced a wide picture request that did not widen "
                      f"(render_width <= native_width); see {log} for the announcement and its reason.")
    if executed <= 0:
        raise Refusal("REFUSED: the product presented frames but executed no guest blocks; a "
                      "play-through with no dynarec execution is not gameplay evidence.")


def kill(process: subprocess.Popen) -> int:
    """Kill by the PID this tool launched, never by name: another agent's product run must not die
    with this one."""
    if process.poll() is None:
        process.send_signal(signal.SIGTERM)
        try:
            process.wait(timeout=30)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait(timeout=30)
    return process.returncode


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--port", type=int, default=5983, help="live endpoint port (default 5983)")
    parser.add_argument("--binary", type=Path, default=Path(gate.BIN),
                        help="the product to play (default: tools/gate.py's canonical build)")
    parser.add_argument("--hold", nargs="*", default=["right"], metavar="BUTTON",
                        help="buttons to hold once gameplay is reached (default: right)")
    parser.add_argument("--seconds", type=float, default=8.0, help="how long to hold them")
    parser.add_argument("--settle", type=int, default=1, metavar="POLLS",
                        help="observations to let a screen settle before answering it (default 1; a "
                             "poll is 55-124 presented frames measured, and the title's own verified "
                             "sequence waits 40 — see drive_to_gameplay)")
    parser.add_argument("--budget", type=int, default=40000,
                        help="presented frames allowed for the whole route")
    parser.add_argument("--timeout", type=float, default=420.0,
                        help="wall-clock seconds allowed for the whole route")
    parser.add_argument("--connect-timeout", type=float, default=120.0)
    parser.add_argument("--shot", type=Path, default=OUT_DIR / "gameplay.ppm")
    arguments = parser.parse_args()

    matched, scanned, disagreements = title_prompts.verify_address_owners(REPO)
    print(f"[live] guest-address owners: {matched} of {scanned} source(s) still declare the address "
          f"this tool reads")
    for disagreement in disagreements:
        print(f"[live]   {disagreement}")
    if disagreements:
        raise Refusal("REFUSED: a guest address this tool reads is no longer declared by the source "
                      "that owns it. Read the disagreement above and the cited file before trusting "
                      "any number below.")

    binary = arguments.binary if arguments.binary.is_absolute() else REPO / arguments.binary
    identity = binary_identity(binary)
    build_tree = binary.parent.parent
    print(f"[live] product: {display_path(binary)} md5 {identity['md5'][:12]} "
          f"mtime {identity['mtime']}, framework {resolved_framework(build_tree)}")
    print(f"[live] boot image: {display_path(Path(gate.EXE))}")

    process = launch(arguments.port, binary, LOG)
    client: LiveClient | None = None
    try:
        client = connect(arguments.port, arguments.connect_timeout, LOG)
        session = Session(client)
        print(f"[live] endpoint on 127.0.0.1:{arguments.port}, product pid {process.pid}, presenting "
              f"from frame {session.first_frame}")
        route_frames = session.drive_to_gameplay(arguments.budget, arguments.timeout,
                                                arguments.settle)
        print(f"[live] gameplay reached: {route_frames} presented frames of route driving, "
              f"{session.answers} menu answers, {session.polls} observations, {session.presses} "
              f"pad commands")

        gameplay = capture(client, arguments.shot)
        print(f"[live] gameplay screenshot: {gameplay['reply']} — {gameplay['bytes']} bytes, "
              f"{gameplay['width']}x{gameplay['height']} {gameplay['format']}")
        for phase, values in session.attract_seen.items():
            print(f"[live] attract-demo flag 0x{title_prompts.ATTRACT_FLAG:08X} in {phase}: "
                  f"{sorted(values)} ({len(session.attract_seen[phase])} sample(s) taken across "
                  f"{session.polls} observations)")

        held = session.play_window(list(arguments.hold), arguments.seconds) if arguments.hold else {}
        if held:
            moved = held["position_before"] != held["position_after"]
            print(f"[live] held {held['buttons']} for {held['seconds']}s: "
                  f"{held['presented_frames']} presented frames, {held['pad_frames']} pad service "
                  f"frames (the port's input clock, measured here at "
                  f"{held['pad_frames'] / max(1, held['presented_frames']):.2f} per presented frame — "
                  f"so it is NOT a game tick), {held['samples']} frame samples read while it ran")
            if held["guest_after"]:
                print(f"[live]   guest work during the window: "
                      f"{held['guest_after'].get('executed_instructions', 0) - held['guest_before'].get('executed_instructions', 0)}"
                      f" instructions and "
                      f"{held['guest_after'].get('executed_blocks', 0) - held['guest_before'].get('executed_blocks', 0)}"
                      f" blocks executed, "
                      f"{held['guest_after'].get('cache_hits', 0) - held['guest_before'].get('cache_hits', 0)}"
                      f" cache hits — the game kept executing while the picture was sampled")
            print(f"[live]   player position {fixed_16_16(held['position_before'])} -> "
                  f"{fixed_16_16(held['position_after'])} (16.16 at "
                  f"0x{title_prompts.PLAYER_POSITION:08X}) — "
                  f"{'MOVED' if moved else 'DID NOT MOVE'}")
            after = capture(client, arguments.shot.with_name("gameplay_after_hold.ppm"))
            verdict = ("the picture CHANGED" if after["digest"] != gameplay["digest"]
                       else "the picture is byte-identical to the first capture")
            print(f"[live] screenshot after the hold: {after['bytes']} bytes, "
                  f"{after['width']}x{after['height']}, {verdict} (md5 {after['digest'][:12]} vs "
                  f"{gameplay['digest'][:12]}; equal byte SIZE is not equal picture, which is why "
                  f"this compares content)")
            if not moved:
                raise Refusal("REFUSED: gameplay was reached and the picture is live, but the player "
                              f"did not move while {held['buttons']} was held for {held['seconds']}s. "
                              f"That is a finding, not a pass.")
        report(client, LOG)
        client.quit()
        client = None
        return 0
    finally:
        if client is not None:
            client.close()
        print(f"[live] product pid {process.pid} exited {kill(process)}; log {LOG}")


if __name__ == "__main__":
    raise SystemExit(main())
