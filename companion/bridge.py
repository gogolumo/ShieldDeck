"""macOS bridge: S1 Cmd+Tab, S2 Calculator, S3 screenshot selection; protocol v1."""
import argparse
from datetime import datetime, timezone
import fcntl
from pathlib import Path
import subprocess
import termios
import time

import serial
from serial.tools import list_ports
from bridge_protocol import ConnectionLost, LineReader, Session


def log(message):
    print(f"{datetime.now(timezone.utc).isoformat()} {message}", flush=True)


class KeyRunner:
    def __init__(self, executable, disabled=False):
        self.executable = executable
        self.disabled = disabled
        self.process = None
        self.simulated = False
        self.button = None

    @property
    def busy(self):
        return self.process is not None or self.simulated

    def start(self, button):
        self.button = button
        if self.disabled:
            self.simulated = True
            return
        commands = {
            1: [str(self.executable), "--cmd-tab"],
            2: ["/usr/bin/open", "-b", "com.apple.calculator"],
            3: [str(self.executable), "--screenshot"],
        }
        self.process = subprocess.Popen(commands[button],
                                        stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

    def poll(self):
        if self.simulated:
            self.simulated = False
            log("ACTION_DISABLED: test mode, no keys sent")
            return 3  # Deliberately never report OK for a simulation.
        if self.process is None or self.process.poll() is None:
            return None
        stdout, stderr = self.process.communicate()
        code = self.process.returncode
        self.process = None
        expected = {1: "SENT_CMD_TAB", 2: "", 3: "SENT_SCREENSHOT_SHORTCUT"}[self.button]
        if code == 0 and stdout.strip() == expected:
            messages = {
                1: "SENT_CMD_TAB: macOS events posted; visible switch requires user confirmation",
                2: "OPEN_CALCULATOR_REQUESTED: Launch Services accepted; visible app requires user confirmation",
                3: "SENT_SCREENSHOT_SHORTCUT: selection requested; no claim that an image was saved",
            }
            log(messages[self.button])
            return 0
        log(f"ACTION_ERROR: {stderr.strip()[:1000] or 'native helper failed'}")
        return code if self.button != 2 and code in (2, 4) else 5


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True)
    parser.add_argument("--serial-number", help="Reconnect only to this USB identity")
    parser.add_argument("--keys", type=Path, default=Path(__file__).parent / ".build/ShieldDeckKeys")
    parser.add_argument("--no-actions", action="store_true", help="Return ERR|3; never send keys")
    args = parser.parse_args()
    identity = args.serial_number
    for item in list_ports.comports():
        if item.device == args.port and not identity:
            identity = item.serial_number
    if not identity:
        parser.error("USB identity is unavailable; specify --serial-number for the selected board")
    if not args.no_actions:
        try:
            check = subprocess.run([str(args.keys), "--check"], capture_output=True, text=True, timeout=5)
        except (OSError, subprocess.TimeoutExpired) as error:
            parser.error(f"Build/check the native helper first: {error}")
        log((check.stdout + check.stderr).strip())
    runner = KeyRunner(args.keys, args.no_actions)
    backoff = 1
    try:
        while True:
            # Reap old helpers even while USB is unplugged; never replay them.
            runner.poll()
            matches = [p.device for p in list_ports.comports() if p.serial_number == identity]
            if len(matches) != 1:
                log("OFFLINE: waiting for selected Arduino USB identity")
                time.sleep(backoff)
                backoff = min(backoff * 2, 5)
                continue
            try:
                with serial.Serial(matches[0], 115200, timeout=0.02, write_timeout=0.2, exclusive=True) as port:
                    fcntl.ioctl(port.fileno(), termios.TIOCEXCL)
                    log(f"OPEN|{matches[0]}|115200")

                    def send(line):
                        data = (line + "\n").encode("ascii")
                        if port.write(data) != len(data):
                            raise ConnectionLost("Partial serial write")
                        log(f"TX {line}")

                    reader = LineReader()
                    session = Session(send, runner, log, time.monotonic())
                    while True:
                        now = time.monotonic()
                        session.tick(now)  # Reject expired sessions before consuming new input.
                        data = port.read(min(port.in_waiting or 1, 256))
                        now = time.monotonic()
                        for line in reader.feed(data, now):
                            log(f"RX {line}")
                            session.receive(line, now)
                        if session.state == "active":
                            backoff = 1
            except (serial.SerialException, OSError, ConnectionLost) as error:
                log(f"DISCONNECTED: {error}; old button events will not be replayed")
                time.sleep(backoff)
                backoff = min(backoff * 2, 5)
    except KeyboardInterrupt:
        log("STOPPED: serial released; device will show offline within 3 seconds")


if __name__ == "__main__":
    main()
