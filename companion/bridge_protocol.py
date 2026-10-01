"""Protocol v1 subset for three stateless actions; no macOS imports."""
import secrets


class ConnectionLost(Exception):
    pass


class LineReader:
    def __init__(self):
        self.buffer = bytearray()
        self.discard = False
        self.last_byte = 0.0

    def feed(self, data, now):
        if self.buffer and now - self.last_byte >= 0.5:
            self.buffer.clear()
            self.discard = True
        lines = []
        for byte in data:
            self.last_byte = now
            if byte == 10:
                if not self.discard and self.buffer:
                    lines.append(self.buffer.rstrip(b"\r").decode("ascii"))
                self.buffer.clear()
                self.discard = False
            elif not self.discard:
                if (self.buffer.endswith(b"\r") or (byte != 13 and not 32 <= byte <= 126)
                        or len(self.buffer) >= 95):
                    self.buffer.clear()
                    self.discard = True
                else:
                    self.buffer.append(byte)
        return lines


def number(text, low, high):
    if not text or len(text) > 5 or not text.isascii() or not text.isdecimal():
        return None
    value = int(text)
    return value if low <= value <= high else None


class Session:
    def __init__(self, send, runner, log, now):
        self.send, self.runner, self.log = send, runner, log
        self.opened = now
        self.sid = None
        self.state = "hello"
        self.rev = 1
        self.high_seq = 0
        self.cache = {}
        self.pending = None
        self.last_pong = self.last_ping = now
        self.config_at = self.config_sent = now
        self.ready_at = now

    def config(self, now):
        self.send(f"CONFIG|{self.sid}|1|1|0|PULSE|PULSE|PULSE")
        self.config_sent = now

    def result(self, rev, seq, code):
        line = f"RESULT|{self.sid}|{rev}|{seq}|{'OK' if code == 0 else 'ERR'}|{code}"
        self.send(line)
        self.cache[(rev, seq)] = line
        if len(self.cache) > 16:
            self.cache.pop(next(iter(self.cache)))

    def receive(self, line, now):
        f = line.split("|")
        if any(not field for field in f):
            return
        if f[:2] == ["HELLO", "SHIELDDECK"] and len(f) == 3:
            if f[2] != "1":
                raise ConnectionLost("Unsupported protocol version")
            self.sid = secrets.token_hex(4).upper()
            self.state = "ready"
            self.ready_at = now
            self.high_seq = 0
            self.cache.clear()
            self.pending = None  # Old action may finish, but is never replayed.
            self.send(f"WELCOME|1|{self.sid}")
            return
        if len(f) < 2 or not self.sid or f[1] != self.sid:
            return
        if f[0] == "READY" and len(f) == 2 and self.state == "ready":
            self.state = "config"
            self.last_pong = self.last_ping = self.config_at = now
            self.send(f"PING|{self.sid}")
            self.config(now)
        elif f[0] == "PONG" and len(f) == 2 and self.state in ("config", "active"):
            self.last_pong = now
        elif f == ["CONFIGURED", self.sid, "1"] and self.state == "config":
            self.state = "active"
            self.log("CONNECTED: P001; S1 = Discord mic; S2 = Discord deafen; S3 = Discord camera")
        elif f[0] == "BTN" and len(f) == 6 and self.state == "active":
            rev, seq, button = number(f[2], 1, 65535), number(f[3], 1, 65535), number(f[4], 1, 3)
            if None in (rev, seq, button) or f[5] != "SHORT":
                return
            if rev != self.rev:
                self.result(rev, seq, 6)
                return
            if seq <= self.high_seq:
                if self.pending and self.pending[:2] == (rev, seq):
                    self.send(f"ACK|{self.sid}|{rev}|{seq}")
                elif (rev, seq) in self.cache:
                    self.send(self.cache[(rev, seq)])
                return
            self.high_seq = seq
            if self.runner.busy:
                self.result(rev, seq, 4)
                return
            self.send(f"ACK|{self.sid}|{rev}|{seq}")
            self.pending = (rev, seq, now, False)
            try:
                self.runner.start(button)
            except OSError as error:
                self.log(f"ACTION_ERROR: {error}")
                self.result(rev, seq, 3)
                self.pending = None

    def tick(self, now):
        if self.state == "hello" and now - self.opened >= 5:
            raise ConnectionLost("No ShieldDeck HELLO")
        if self.state == "ready" and now - self.ready_at >= 3:
            raise ConnectionLost("No READY")
        if self.state in ("config", "active"):
            if now - self.last_pong >= 3:
                raise ConnectionLost("Heartbeat lost")
            if now - self.last_ping >= 1:
                self.send(f"PING|{self.sid}")
                self.last_ping = now
        if self.state == "config":
            if now - self.config_at >= 3:
                raise ConnectionLost("CONFIG not accepted; release buttons")
            if now - self.config_sent >= 0.25:
                self.config(now)
        result = self.runner.poll()
        if result is not None and self.pending:
            rev, seq, _, timed_out = self.pending
            if not timed_out:
                self.result(rev, seq, result)
            self.pending = None
        elif self.pending and now - self.pending[2] >= 5 and not self.pending[3]:
            rev, seq, start, _ = self.pending
            self.result(rev, seq, 1)
            self.pending = (rev, seq, start, True)
            self.log("ACTION_TIMEOUT: outcome unknown; no retry; runner stays busy until exit")
