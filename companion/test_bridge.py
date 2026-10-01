import unittest
from bridge_protocol import ConnectionLost, LineReader, Session


class Runner:
    def __init__(self):
        self.busy = False
        self.started = 0
        self.result = None

    def start(self):
        self.started += 1
        self.busy = True

    def poll(self):
        result = self.result
        self.result = None
        if result is not None:
            self.busy = False
        return result


class BridgeTests(unittest.TestCase):
    def setUp(self):
        self.sent = []
        self.runner = Runner()
        self.session = Session(self.sent.append, self.runner, lambda s: None, 0)
        self.session.receive("HELLO|SHIELDDECK|1", 0)
        self.sid = self.session.sid
        self.session.receive(f"READY|{self.sid}", 0.1)
        self.session.receive(f"CONFIGURED|{self.sid}|1", 0.2)

    def button(self, seq=1, button=1, sid=None, rev=1):
        self.session.receive(f"BTN|{sid or self.sid}|{rev}|{seq}|{button}|SHORT", 0.3)

    def test_dedup_inflight_and_completed(self):
        self.button(); self.button()
        self.assertEqual(self.runner.started, 1)
        self.runner.result = 0
        self.session.tick(0.4)
        self.button()
        self.assertEqual(self.runner.started, 1)
        self.assertTrue(self.sent[-1].endswith("|1|1|OK|0"))

    def test_permission_denial_is_error(self):
        self.button()
        self.runner.result = 2
        self.session.tick(0.4)
        self.assertTrue(self.sent[-1].endswith("|ERR|2"))

    def test_wrong_session_revision_and_unassigned(self):
        self.button(sid="00000000")
        self.button(rev=2)
        self.assertTrue(self.sent[-1].endswith("|ERR|6"))
        self.button(button=2)
        self.assertEqual(self.runner.started, 0)
        self.assertTrue(self.sent[-1].endswith("|ERR|3"))

    def test_busy_and_heartbeat_during_action(self):
        self.button(); self.button(seq=2)
        self.assertTrue(self.sent[-1].endswith("|ERR|4"))
        self.session.tick(1.2)
        self.assertIn(f"PING|{self.sid}", self.sent)
        self.assertEqual(self.runner.started, 1)

    def test_timeout_not_retried_and_not_later_success(self):
        self.button()
        self.session.tick(2.4)
        self.assertTrue(self.sent[-1].endswith("|ERR|1"))
        self.assertTrue(self.runner.busy)
        self.runner.result = 0
        self.session.tick(2.5)
        self.assertFalse(any("|OK|0" in s for s in self.sent))
        self.button()
        self.assertEqual(self.runner.started, 1)

    def test_reset_discards_old_result(self):
        self.button()
        self.session.receive("HELLO|SHIELDDECK|1", 0.4)
        self.runner.result = 0
        self.session.tick(0.5)
        self.assertFalse(any("|OK|0" in s for s in self.sent))

    def test_no_heartbeat(self):
        with self.assertRaises(ConnectionLost):
            self.session.tick(3.2)

    def test_frames(self):
        reader = LineReader()
        self.assertEqual(reader.feed(b"HEL", 0), [])
        self.assertEqual(reader.feed(b"LO|SHIELDDECK|1\r\nX\n", 0.1), ["HELLO|SHIELDDECK|1", "X"])
        self.assertEqual(reader.feed(b"X" * 96 + b"PING\n", 0.2), [])
        self.assertEqual(reader.feed(b"BAD\0INPUT\n", 0.3), [])
        reader.feed(b"BT", 0.4)
        self.assertEqual(reader.feed(b"N|old\nNEW\n", 1), ["NEW"])


if __name__ == "__main__":
    unittest.main()
