import Foundation
import CoreGraphics
import Darwin

// Stable command-line helper. No key events are emitted by permission checks.
let mode = CommandLine.arguments.dropFirst().first ?? "--check"
if mode == "--request-permission" {
    _ = CGRequestPostEventAccess()
}
guard ["--check", "--request-permission", "--cmd-tab"].contains(mode) else {
    fputs("Unknown operation\n", stderr)
    exit(3)
}
guard CGPreflightPostEventAccess() else {
    fputs("ACCESSIBILITY_REQUIRED: System Settings > Privacy & Security > Accessibility. Enable the responsible launching app or ShieldDeckKeys, then retry.\n", stderr)
    exit(2)
}
if mode != "--cmd-tab" {
    print("ACCESSIBILITY_OK")
    exit(0)
}

// Avoid colliding with keys that the user is currently holding.
let physicalFlags = CGEventSource.flagsState(.combinedSessionState)
let modifiers: CGEventFlags = [.maskCommand, .maskShift, .maskControl, .maskAlternate]
guard physicalFlags.intersection(modifiers).isEmpty else {
    fputs("BUSY: release keyboard modifiers before using S1\n", stderr)
    exit(4)
}
guard let source = CGEventSource(stateID: .privateState),
      let commandDown = CGEvent(keyboardEventSource: source, virtualKey: 55, keyDown: true),
      let tabDown = CGEvent(keyboardEventSource: source, virtualKey: 48, keyDown: true),
      let tabUp = CGEvent(keyboardEventSource: source, virtualKey: 48, keyDown: false),
      let commandUp = CGEvent(keyboardEventSource: source, virtualKey: 55, keyDown: false) else {
    fputs("EVENT_CREATION_FAILED\n", stderr)
    exit(5)
}
commandDown.flags = .maskCommand
tabDown.flags = .maskCommand
tabUp.flags = .maskCommand
commandUp.flags = []
// All events are allocated before keyDown; always release both keys.
commandDown.post(tap: .cghidEventTap)
usleep(20_000)
tabDown.post(tap: .cghidEventTap)
usleep(20_000)
tabUp.post(tap: .cghidEventTap)
commandUp.post(tap: .cghidEventTap)
print("SENT_CMD_TAB")  // Delivery to macOS, not proof of a visible app switch.
