"""Read-only diagnostic capture. Requires pyserial (included with PlatformIO)."""

import argparse
from datetime import datetime, timezone
from pathlib import Path

import serial


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("port")
    parser.add_argument("--log", required=True, type=Path)
    args = parser.parse_args()
    with args.log.open("a", encoding="utf-8", buffering=1) as log:
        def record(text):
            line = f"{datetime.now(timezone.utc).isoformat()} {text}"
            print(line, flush=True)
            log.write(line + "\n")

        try:
            with serial.Serial(args.port, 115200, timeout=0.2, exclusive=True) as port:
                record(f"CAPTURE|OPEN|{args.port}|115200")
                pending = bytearray()
                discard = False
                while True:
                    for byte in port.read(min(port.in_waiting or 1, 256)):
                        if byte == 10:
                            if not discard:
                                record(pending.rstrip(b"\r").decode("ascii", errors="replace"))
                            pending.clear()
                            discard = False
                        elif not discard:
                            pending.append(byte)
                            if len(pending) > 128:
                                record("CAPTURE|OVERSIZE_LINE")
                                pending.clear()
                                discard = True
        except KeyboardInterrupt:
            record("CAPTURE|CLOSED")
        except serial.SerialException as error:
            record(f"CAPTURE|ERROR|{error}")
            raise SystemExit(1)


if __name__ == "__main__":
    main()
