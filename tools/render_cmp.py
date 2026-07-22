#!/usr/bin/env python3
"""Native-vs-PSX RENDER comparison harness.

TWO MODES, both explicit — the tool used to take a bare argv[1] that was a REPL PRELUDE while every
caller passed it an IMAGE PATH, so a two-image call silently re-ran the default scene and printed
"differing pixels: 0/76800" for images that differ by tens of thousands of pixels. Subcommands now.

  tools/render_cmp.py run  [repl-prelude]   render the SAME deterministic state twice — pc_render and
                                            PSXPORT_RENDER_PSX=1 — at 1x/4:3/30fps and diff the frames.
                                            The prelude must NOT contain `shot`/`quit`; those are added.
                                            Default prelude drives to the seaside walkable field.
  tools/render_cmp.py diff <a.png> <b.png>  diff two ALREADY-CAPTURED frames (same size). Use this when
                                            the two legs were captured by some other driver, e.g.
                                            tools/warpsweep.sh.

Both modes print an exact-differing count and a >8/255 count, and write a triptych
(a | b | diff-magenta) under scratch/screenshots/.
"""
import os, subprocess, sys
from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BIN  = os.path.join(ROOT, "scratch/bin/tomba2_port")
SHOTS= os.path.join(ROOT, "scratch/screenshots")
CMP_INI = os.path.join(ROOT, "scratch/cmp.ini")

DEFAULT_PRELUDE = "newgame\nrun 60\nskip 600\nrun 30"
NOISE = 8   # max-channel delta above which a pixel counts as a real difference, not dither/rounding


def write_cmp_ini():
    open(CMP_INI, "w").write("aspect=0\nires=1\nires_auto=0\nssao=0\nlight=0\nshadows=0\nfps60=0\n")


def run_leg(prelude, out_ppm, render_psx):
    env = dict(os.environ, PSXPORT_SETTINGS=CMP_INI, PSXPORT_REPL="1", PSXPORT_VK_HEADLESS="1",
               PSXPORT_NOAUDIO="1", PSXPORT_NO_FMV="1")
    if render_psx:
        env["PSXPORT_RENDER_PSX"] = "1"
    script = prelude + f"\nshot {out_ppm}\nquit\n"
    subprocess.run([BIN], input=script.encode(), env=env,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, cwd=ROOT)


def compare(path_a, path_b, out_name, label_a="A", label_b="B"):
    """Diff two frames. Returns 0 on success, 1 if unusable (missing / size mismatch)."""
    for p in (path_a, path_b):
        if not os.path.exists(p):
            print(f"MISSING {p}")
            return 1
    a = Image.open(path_a).convert("RGB")
    b = Image.open(path_b).convert("RGB")
    if a.size != b.size:
        print(f"SIZE MISMATCH {label_a}={a.size} {label_b}={b.size}")
        return 1
    W, H = a.size
    pa, pb = a.load(), b.load()
    diff = Image.new("RGB", (W, H)); pd = diff.load()
    n_exact = n_noisy = 0
    for y in range(H):
        for x in range(W):
            ca, cb = pa[x, y], pb[x, y]
            if ca != cb:
                n_exact += 1
                if max(abs(ca[0]-cb[0]), abs(ca[1]-cb[1]), abs(ca[2]-cb[2])) > NOISE:
                    n_noisy += 1
                    pd[x, y] = (255, 0, 255)
                    continue
            pd[x, y] = ca
    comp = Image.new("RGB", (W*3+8, H), (20, 20, 20))
    comp.paste(a, (0, 0)); comp.paste(b, (W+4, 0)); comp.paste(diff, (2*W+8, 0))
    out = os.path.join(SHOTS, out_name)
    os.makedirs(os.path.dirname(out), exist_ok=True)
    comp.save(out)
    total = W*H
    print(f"{label_a} | {label_b} | diff  -> {out}")
    print(f"differing pixels: {n_exact}/{total} = {100.0*n_exact/total:.2f}%   "
          f"(>{NOISE}/255: {n_noisy} = {100.0*n_noisy/total:.2f}%)")
    return 0


def main(argv):
    if len(argv) < 2 or argv[1] in ("-h", "--help"):
        print(__doc__)
        return 2
    mode = argv[1]
    if mode == "diff":
        if len(argv) != 4:
            print("usage: render_cmp.py diff <a.png> <b.png>")
            return 2
        return compare(argv[2], argv[3], "cmp_triptych.png",
                       os.path.basename(argv[2]), os.path.basename(argv[3]))
    if mode == "run":
        prelude = argv[2] if len(argv) > 2 else DEFAULT_PRELUDE
        if os.path.exists(prelude):
            print(f"REFUSING: '{prelude}' is a file — `run` takes a REPL PRELUDE, not an image. "
                  f"Did you mean `render_cmp.py diff a.png b.png`?")
            return 2
        write_cmp_ini()
        nat = os.path.join(SHOTS, "cmp_native.ppm"); psx = os.path.join(SHOTS, "cmp_psx.ppm")
        run_leg(prelude, nat, False)
        run_leg(prelude, psx, True)
        for src, dst in ((nat, "cmp_native.png"), (psx, "cmp_psx.png")):
            if os.path.exists(src):
                Image.open(src).convert("RGB").save(os.path.join(SHOTS, dst))
        return compare(nat, psx, "cmp_triptych.png", "native", "psx")
    print(f"unknown mode '{mode}' — expected `run` or `diff`. See --help.")
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv))
