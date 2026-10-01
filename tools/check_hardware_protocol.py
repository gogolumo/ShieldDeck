"""Read/write transport check on a flashed Uno. No Mac keyboard actions."""
import argparse
import time
import serial


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("port")
    args = parser.parse_args()
    with serial.Serial(args.port, 115200, timeout=0.05, write_timeout=0.5, exclusive=True) as port:
        # Uno can reset on open; don't treat pre-reset buffered HELLO as boot-ready.
        time.sleep(2)
        port.reset_input_buffer()
        def send(line):
            port.write((line + "\n").encode("ascii"))

        def wait_for(expected, seconds=3):
            end = time.monotonic() + seconds
            pending = bytearray()
            while time.monotonic() < end:
                pending.extend(port.read(port.in_waiting or 1))
                while b"\n" in pending:
                    raw, _, rest = pending.partition(b"\n")
                    pending = bytearray(rest)
                    line = raw.rstrip(b"\r").decode("ascii", errors="replace")
                    print("RX", line, flush=True)
                    if line == expected or (isinstance(expected, tuple) and line in expected):
                        return line
            raise AssertionError(f"Timed out waiting for {expected}")

        def configure(sid):
            config = f"CONFIG|{sid}|1|1|0|PULSE|PULSE|PULSE"
            for _ in range(8):
                send(config)
                response = wait_for((f"CONFIGURED|{sid}|1", f"REJECT|{sid}|1|BUSY"), 0.3)
                if response.startswith("CONFIGURED"):
                    return config
                # Boot requires a stable released input before accepting mapping.
                time.sleep(0.1)
            raise AssertionError("CONFIG remains BUSY: release physical buttons")

        wait_for("HELLO|SHIELDDECK|1", 5)
        send("WELCOME|1|A0B0C0D0")
        wait_for("READY|A0B0C0D0")
        config = configure("A0B0C0D0")
        send(config)
        wait_for("CONFIGURED|A0B0C0D0|1")
        port.write(b"PI")
        time.sleep(0.05)
        port.write(b"NG|A0B0C0D0\r\n")
        wait_for("PONG|A0B0C0D0")
        for bad in ("PING|DEADBEEF", "PING||A0B0C0D0", "X" * 100 + "PING|A0B0C0D0"):
            send(bad)
            assert b"PONG" not in port.read(128), bad
        send("PING|A0B0C0D0")
        wait_for("PONG|A0B0C0D0")
        # Stop heartbeat, observe actual firmware returning to discovery.
        wait_for("HELLO|SHIELDDECK|1", 4)
        send("PING|A0B0C0D0")
        assert b"PONG" not in port.read(128)
        send("WELCOME|1|1234ABCD")
        wait_for("READY|1234ABCD")
        configure("1234ABCD")
        send("PING|1234ABCD")
        wait_for("PONG|1234ABCD")
        print("PASS: real-board handshake, duplicate CONFIG, fragmented CRLF, malformed frames, heartbeat timeout and new session")


if __name__ == "__main__":
    main()
