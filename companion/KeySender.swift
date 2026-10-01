import Foundation
import CoreGraphics
import Darwin

// Stable command-line helper. No key events are emitted by permission checks.
let mode = CommandLine.arguments.dropFirst().first ?? "--check"
if mode == "--request-permission" {
    _ = CGRequestPostEventAccess()
}
guard ["--check", "--request-permission", "--cmd-tab", "--screenshot"].contains(mode) else {
    fputs("Unknown operation\n", stderr)
    exit(3)
}
guard CGPreflightPostEventAccess() else {
    fputs("ACCESSIBILITY_REQUIRED: System Settings > Privacy & Security > Accessibility. Enable the responsible launching app or ShieldDeckKeys, then retry.\n", stderr)
    exit(2)
}
if mode == "--check" || mode == "--request-permission" {
    print("ACCESSIBILITY_OK")
    exit(0)
}

// Avoid colliding with keys that the user is currently holding.
let physicalFlags = CGEventSource.flagsState(.combinedSessionState)
let modifiers: CGEventFlags = [.maskCommand, .maskShift, .maskControl, .maskAlternate]
guard physicalFlags.intersection(modifiers).isEmpty else {
    fputs("BUSY: release keyboard modifiers before using the macro\n", stderr)
    exit(4)
}
guard let source = CGEventSource(stateID: .privateState) else {
    fputs("EVENT_CREATION_FAILED\n", stderr)
    exit(5)
}
let both: CGEventFlags = [.maskCommand, .maskShift]
let strokes: [(CGKeyCode, Bool, CGEventFlags)] = mode == "--cmd-tab" ? [
    (55, true, .maskCommand), (48, true, .maskCommand),
    (48, false, .maskCommand), (55, false, [])
] : [
    (55, true, .maskCommand), (56, true, both),
    (21, true, both), (21, false, both), // ANSI 4, not numeric keypad 4.
    (56, false, .maskCommand), (55, false, [])
]
// Allocate the complete sequence before posting any keyDown.
var events: [CGEvent] = []
for (key, down, flags) in strokes {
    guard let event = CGEvent(keyboardEventSource: source, virtualKey: key, keyDown: down) else {
        fputs("EVENT_CREATION_FAILED\n", stderr)
        exit(5)
    }
    event.flags = flags
    events.append(event)
}
for (index, event) in events.enumerated() {
    event.post(tap: .cghidEventTap)
    if index + 1 < events.count { usleep(20_000) }
}
// Screenshot opens selection mode; the user may cancel without saving an image.
print(mode == "--cmd-tab" ? "SENT_CMD_TAB" : "SENT_SCREENSHOT_SHORTCUT")
