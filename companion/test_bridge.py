import unittest
from unittest.mock import patch, Mock
from bridge_protocol import ConnectionLost, LineReader, Session
from bridge import KeyRunner


class Runner:
    def __init__(self):
        self.busy = False
        self.started = 0
        self.result = None
        self.buttons = []

    def start(self, button):
        self.started += 1
        self.busy = True
        self.buttons.append(button)

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

    def test_wrong_session_revision(self):
        self.button(sid="00000000")
        self.button(rev=2)
        self.assertTrue(self.sent[-1].endswith("|ERR|6"))
        self.assertEqual(self.runner.started, 0)

    def test_three_button_mappings(self):
        for button in (1, 2, 3):
            self.button(seq=button, button=button)
            self.runner.result = 0
            self.session.tick(0.4)
        self.assertEqual(self.runner.buttons, [1, 2, 3])

    def test_out_of_range_button_is_ignored(self):
        self.button(button=4)
        self.assertEqual(self.runner.started, 0)

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


class RunnerTests(unittest.TestCase):
    @patch("bridge.subprocess.Popen")
    def test_commands_and_success_semantics(self, popen):
        runner = KeyRunner("/example/ShieldDeckKeys")
        for button, command, output in (
            (1, ["/example/ShieldDeckKeys", "--cmd-tab"], "SENT_CMD_TAB\n"),
            (2, ["/usr/bin/open", "-b", "com.apple.calculator"], ""),
            (3, ["/example/ShieldDeckKeys", "--screenshot"], "SENT_SCREENSHOT_SHORTCUT\n"),
        ):
            process = Mock()
            process.poll.return_value = 0
            process.returncode = 0
            process.communicate.return_value = (output, "")
            popen.return_value = process
            runner.start(button)
            self.assertEqual(popen.call_args.args[0], command)
            self.assertEqual(runner.poll(), 0)
            self.assertFalse(runner.busy)

    @patch("bridge.subprocess.Popen")
    def test_open_failure_is_not_permission_success(self, popen):
        process = Mock()
        process.poll.return_value = process.returncode = 2
        process.communicate.return_value = ("", "launch failed")
        popen.return_value = process
        runner = KeyRunner("/example/keys")
        runner.start(2)
        self.assertEqual(runner.poll(), 5)

    def test_disabled_does_not_report_success(self):
        runner = KeyRunner("unused", disabled=True)
        runner.start(3)
        self.assertEqual(runner.poll(), 3)


if __name__ == "__main__":
    unittest.main()
