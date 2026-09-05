#!/usr/bin/env python3
"""Compare Tomba! 1's Lightrec CRT0 boundary with an independent CPU oracle."""

from __future__ import annotations

import argparse
import re
import subprocess
from dataclasses import dataclass

REGISTER_NAMES = (
    "at", "v0", "v1", "a0", "a1", "a2", "a3", "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra",
    "lo", "hi",
)


class Refusal(RuntimeError):
    """The evidence cannot support an agreement claim."""


@dataclass(frozen=True)
class Boundary:
    target: int
    pc: int
    registers: dict[str, int]


def run(command: list[str], label: str) -> str:
    result = subprocess.run(command, capture_output=True, text=True, check=False, timeout=60)
    if result.returncode != 0:
        detail = (result.stderr or result.stdout).strip()
        raise Refusal(f"{label} refused with exit {result.returncode}:\n{detail}")
    return result.stdout


def parse_boundary(text: str, prefix: str) -> Boundary:
    capture = re.search(
        rf"^# {re.escape(prefix)}CAPTURED-CALL target=0x([0-9A-Fa-f]+) ra=0x[0-9A-Fa-f]+(?: step=\d+)?$",
        text,
        re.MULTILINE,
    )
    header = re.search(
        rf"^# {re.escape(prefix)}CALL-BOUNDARY-REGS(?: step=\d+)? pc=0x([0-9A-Fa-f]+)$",
        text,
        re.MULTILINE,
    )
    values = dict(
        (name, int(value, 16))
        for name, value in re.findall(
            rf"^# {re.escape(prefix)}CALL-BOUNDARY-REG (\w+)=0x([0-9A-Fa-f]+)$",
            text,
            re.MULTILINE,
        )
    )
    if capture is None or header is None:
        raise Refusal(f"{prefix or 'oracle '}trace has no complete call boundary")
    missing = sorted(set(REGISTER_NAMES) - values.keys())
    extra = sorted(values.keys() - set(REGISTER_NAMES))
    if missing or extra:
        raise Refusal(f"register denominator changed (missing={missing}, extra={extra})")
    return Boundary(int(capture.group(1), 16), int(header.group(1), 16), values)


def differences(oracle: Boundary, port: Boundary) -> list[str]:
    result: list[str] = []
    if oracle.target != port.target:
        result.append(f"target oracle=0x{oracle.target:08X} port=0x{port.target:08X}")
    if oracle.pc != port.pc:
        result.append(f"pc oracle=0x{oracle.pc:08X} port=0x{port.pc:08X}")
    for name in REGISTER_NAMES:
        if oracle.registers[name] != port.registers[name]:
            result.append(
                f"{name} oracle=0x{oracle.registers[name]:08X} port=0x{port.registers[name]:08X}"
            )
    return result


def selftest() -> int:
    registers = {name: index for index, name in enumerate(REGISTER_NAMES)}
    boundary = Boundary(0x80001000, 0x80001000, registers)
    altered = Boundary(boundary.target, boundary.pc, {**registers, "gp": 0xDEADBEEF})
    checks = (
        ("equal boundaries agree", differences(boundary, boundary) == []),
        ("forced field produces the opposite answer", len(differences(boundary, altered)) == 1),
    )
    failed = [label for label, passed in checks if not passed]
    for label, passed in checks:
        print(f"{'PASS' if passed else 'FAIL'}: {label}")
    print(f"SELFTEST: {len(checks) - len(failed)}/{len(checks)} checks passed")
    return 1 if failed else 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("executable", nargs="?")
    parser.add_argument("--oracle-trace")
    parser.add_argument("--port-trace")
    parser.add_argument("--steps", type=int, default=1_000_000)
    parser.add_argument("--force-port-field", choices=REGISTER_NAMES)
    parser.add_argument("--expect-difference", action="store_true")
    parser.add_argument("--selftest", action="store_true")
    args = parser.parse_args()
    if args.selftest:
        return selftest()
    if not args.executable or not args.oracle_trace or not args.port_trace:
        raise Refusal("executable, --oracle-trace, and --port-trace are required")

    oracle_command = [
        args.oracle_trace,
        args.executable,
        "--steps",
        str(args.steps),
        "--capture-first-call",
        "--summary-only",
    ]
    oracle_a_text = run(oracle_command, "oracle run A")
    oracle_b_text = run(oracle_command, "oracle run B")
    oracle_a = parse_boundary(oracle_a_text, "")
    oracle_b = parse_boundary(oracle_b_text, "")
    if oracle_a != oracle_b:
        raise Refusal("two independent-CPU runs disagree; the boundary is not deterministic")

    port_text = run(
        [args.port_trace, args.executable, "--target", f"0x{oracle_a.target:08X}"],
        "Lightrec product",
    )
    port = parse_boundary(port_text, "PORT-")
    if args.force_port_field:
        port = Boundary(
            port.target,
            port.pc,
            {**port.registers, args.force_port_field: port.registers[args.force_port_field] ^ 1},
        )
    found = differences(oracle_a, port)
    if args.expect_difference:
        if not found:
            raise Refusal("expected a forced disagreement but all compared fields agree")
        print(f"EXPECTED DIFFERENCE: {len(found)} field(s)")
        return 0
    if found:
        print(f"DISAGREE: {len(found)} of {len(REGISTER_NAMES) + 2} compared field(s)")
        for difference in found:
            print(f"  - {difference}")
        return 1
    print(
        f"AGREE: {len(REGISTER_NAMES) + 2}/{len(REGISTER_NAMES) + 2} target/pc/register fields; "
        "independent oracle deterministic across 2/2 runs"
    )
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Refusal as error:
        print(f"REFUSED: {error}")
        raise SystemExit(2)
