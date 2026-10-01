import Foundation
import CoreGraphics
import AppKit
import ApplicationServices
import Darwin

// Stable command-line helper. No key events are emitted by permission checks.
let mode = CommandLine.arguments.dropFirst().first ?? "--check"
if mode == "--request-permission" {
    _ = CGRequestPostEventAccess()
}
guard ["--check", "--request-permission", "--discord-mic", "--discord-deafen", "--discord-camera", "--discord-camera-check"].contains(mode) else {
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

guard let discord = NSRunningApplication.runningApplications(withBundleIdentifier: "com.hnc.Discord").first else {
    fputs("DISCORD_NOT_RUNNING: open the Discord desktop app first\n", stderr)
    exit(3)
}

func axAttribute(_ element: AXUIElement, _ name: String) -> Any? {
    var value: CFTypeRef?
    return AXUIElementCopyAttributeValue(element, name as CFString, &value) == .success ? value : nil
}

func cameraControl(in element: AXUIElement, depth: Int, remaining: inout Int) -> AXUIElement? {
    guard depth < 32 && remaining > 0 else { return nil }
    remaining -= 1
    if axAttribute(element, kAXRoleAttribute) as? String == kAXButtonRole as String {
        let names = [kAXTitleAttribute, kAXDescriptionAttribute, kAXHelpAttribute]
            .compactMap { axAttribute(element, $0) as? String }
            .joined(separator: " ")
            .lowercased()
        let labels = ["turn on camera", "turn off camera", "turn on video", "turn off video",
                      "включить камеру", "выключить камеру", "включить видео", "выключить видео"]
        if labels.contains(where: { names.contains($0) }) { return element }
    }
    if let children = axAttribute(element, kAXChildrenAttribute) as? [AXUIElement] {
        for child in children {
            if let found = cameraControl(in: child, depth: depth + 1, remaining: &remaining) { return found }
        }
    }
    return nil
}

if mode == "--discord-camera" || mode == "--discord-camera-check" {
    guard AXIsProcessTrusted() else {
        fputs("ACCESSIBILITY_REQUIRED: camera control needs Accessibility permission\n", stderr)
        exit(2)
    }
    let app = AXUIElementCreateApplication(discord.processIdentifier)
    AXUIElementSetMessagingTimeout(app, 2)
    // Electron does not always expose its web controls until requested.
    guard AXUIElementSetAttributeValue(app, "AXManualAccessibility" as CFString, true as CFTypeRef) == .success else {
        fputs("DISCORD_ACCESSIBILITY_UNAVAILABLE\n", stderr)
        exit(3)
    }
    var remaining = 10_000
    if let windows = axAttribute(app, kAXWindowsAttribute) as? [AXUIElement] {
        for window in windows {
            if let button = cameraControl(in: window, depth: 0, remaining: &remaining) {
                if mode == "--discord-camera-check" {
                    print("DISCORD_CAMERA_CONTROL_AVAILABLE")
                    exit(0)
                }
                guard AXUIElementPerformAction(button, kAXPressAction as CFString) == .success else {
                    fputs("DISCORD_CAMERA_PRESS_FAILED\n", stderr)
                    exit(5)
                }
                print("DISCORD_CAMERA_CONTROL_PRESSED")
                exit(0)
            }
        }
    }
    fputs("DISCORD_CAMERA_UNAVAILABLE: join a voice/video call and keep its camera control visible\n", stderr)
    exit(3)
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
let key: CGKeyCode = mode == "--discord-mic" ? 46 : 2 // ANSI M / D
let strokes: [(CGKeyCode, Bool, CGEventFlags)] = [
    (55, true, .maskCommand), (56, true, both),
    (key, true, both), (key, false, both),
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
print(mode == "--discord-mic" ? "SENT_DISCORD_MIC_SHORTCUT" : "SENT_DISCORD_DEAFEN_SHORTCUT")
