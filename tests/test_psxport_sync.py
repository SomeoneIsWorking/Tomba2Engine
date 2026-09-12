"""The pin check must examine the selected verifier build, including stale receipts."""

from __future__ import annotations

import contextlib
import io
import os
import sys
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import mock_open, patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import psxport_sync  # noqa: E402
import verify_ci  # noqa: E402


class PinCheckTests(unittest.TestCase):
    def setUp(self) -> None:
        self.build = os.path.join(psxport_sync.REPO, "build", "ninja-check")
        self.args = SimpleNamespace(build=self.build)

    def run_check(self, receipt: str | None) -> tuple[int, str]:
        output = io.StringIO()
        with patch.object(psxport_sync, "read_pin", return_value=("url", "0123456789abcdef")):
            with patch.object(psxport_sync.os.path, "isfile", return_value=receipt is not None):
                with patch("builtins.open", mock_open(read_data=receipt or "")) as opened:
                    with contextlib.redirect_stdout(output):
                        result = psxport_sync.do_check(self.args)
        if receipt is not None:
            opened.assert_called_once_with(os.path.join(self.build, "psxport_resolved.txt"))
        return result, output.getvalue()

    def test_matching_selected_build_passes(self) -> None:
        status, output = self.run_check("dir = framework\ncommit = 0123456789abcdef\n")
        self.assertEqual(status, 0)
        self.assertIn("check OK", output)

    def test_stale_selected_build_fails(self) -> None:
        status, output = self.run_check("dir = framework\ncommit = fedcba9876543210\n")
        self.assertEqual(status, 1)
        self.assertIn("check FAILED", output)

    def test_missing_selected_build_refuses(self) -> None:
        status, output = self.run_check(None)
        self.assertEqual(status, 2)
        self.assertIn("REFUSED", output)

    def run_verifier(self, receipt: str) -> tuple[int, str]:
        output = io.StringIO()
        with patch.object(verify_ci, "run_consumer_verification", return_value=0) as verifier:
            with patch.object(psxport_sync, "read_pin", return_value=("url", "0123456789abcdef")):
                with patch.object(psxport_sync.os.path, "isfile", return_value=True):
                    with patch("builtins.open", mock_open(read_data=receipt)) as opened:
                        with contextlib.redirect_stdout(output):
                            result = verify_ci.main(["--build", self.build])
        self.assertEqual(verifier.call_args.args[0].build, Path(self.build).resolve())
        opened.assert_called_once_with(os.path.join(self.build, "psxport_resolved.txt"))
        return result, output.getvalue()

    def test_verifier_accepts_matching_selected_build(self) -> None:
        status, output = self.run_verifier("dir = framework\ncommit = 0123456789abcdef\n")
        self.assertEqual(status, 0)
        self.assertIn("check OK", output)

    def test_verifier_rejects_stale_selected_build(self) -> None:
        status, output = self.run_verifier("dir = framework\ncommit = fedcba9876543210\n")
        self.assertEqual(status, 1)
        self.assertIn("check FAILED", output)


if __name__ == "__main__":
    unittest.main()
