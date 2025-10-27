"""Utility helpers for validating PI18/InfiniSolar CRC fields.

This script implements the CRC-16/XMODEM checksum that the PI18
handshake uses.  It can be used from the command line to quickly
validate frames captured from the serial logs to understand whether the
CRC mismatch is caused by the inverter or by the host implementation.
"""
from __future__ import annotations

import argparse
import binascii
from dataclasses import dataclass


@dataclass
class CRCResult:
    """Represents a decoded CRC value."""

    value: int

    @property
    def high(self) -> int:
        """High byte of the CRC (first byte transmitted by the inverter)."""

        return (self.value >> 8) & 0xFF

    @property
    def low(self) -> int:
        """Low byte of the CRC (second byte transmitted by the inverter)."""

        return self.value & 0xFF

    def as_hex(self) -> str:
        """Return the CRC in the ``0xHHLL`` string form."""

        return f"0x{self.value:04X}"

    def __str__(self) -> str:  # pragma: no cover - convenience string repr
        return self.as_hex()


def compute_crc(frame: bytes) -> CRCResult:
    """Compute the CRC-16/XMODEM checksum for ``frame``.

    The InfiniSolar PI18 protocol uses the CRC-16/XMODEM polynomial with
    an initial value of ``0x0000``.  Python's ``binascii.crc_hqx`` helper
    implements exactly this algorithm, so we reuse it here.
    """

    crc_value = binascii.crc_hqx(frame, 0)
    return CRCResult(crc_value)


def _format_bytes(raw: bytes) -> str:
    return " ".join(f"0x{byte:02X}" for byte in raw)


def run_cli() -> None:
    parser = argparse.ArgumentParser(description="Compute PI18 CRC-16/XMODEM values")
    parser.add_argument(
        "frame",
        help=(
            "ASCII frame to evaluate. Use Python-style escape sequences, "
            "e.g. '^D00512\\r'."
        ),
    )
    args = parser.parse_args()

    frame = args.frame.encode("utf-8").decode("unicode_escape").encode("latin1")
    result = compute_crc(frame)

    print("Frame bytes:", _format_bytes(frame))
    print("CRC value :", result.as_hex())
    print("High byte : 0x%02X" % result.high)
    print("Low byte  : 0x%02X" % result.low)


if __name__ == "__main__":  # pragma: no cover - script entry point
    run_cli()
