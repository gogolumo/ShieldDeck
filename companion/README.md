# ShieldDeck for macOS

Здесь находится CLI bridge: Serial → действие Mac → ACK/RESULT. Native helper на Swift отправляет keyboard events через CoreGraphics; Python bridge обслуживает Serial, heartbeat и reconnect независимо от действия. Menu bar UI пока отсутствует.

## Сборка и запуск

Из корня репозитория (Python из PlatformIO уже содержит pyserial):

```sh
mkdir -p companion/.build
xcrun swiftc -O companion/KeySender.swift -o companion/.build/ShieldDeckKeys
companion/.build/ShieldDeckKeys --check
~/.platformio/penv/bin/python companion/bridge.py --port /dev/cu.usbmodem11101
```

Перед запуском закрыть Serial Monitor/capture и уже работающий bridge; портом владеет только один процесс. Firmware должна быть собрана и загружена через `~/.platformio/penv/bin/pio run -e uno -t upload` (для upload сначала остановить bridge). Для возврата к hardware tests используется `-e smoke`.

Назначения выполняются на отпускании кнопки:

| Кнопка | Действие | Что подтверждает OK |
| --- | --- | --- |
| S1 | Discord: mute/unmute микрофона | Отправлен ⌘⇧M; фактический mute виден в Discord |
| S2 | Discord: Deafen/Undeafen | Отправлен ⌘⇧D; фактический Deafen виден в Discord |
| S3 | Discord: камера в текущем звонке | Нажат доступный элемент Turn on/off Camera; передачу видео проверить в Discord |

Все три действия требуют установленный и запущенный desktop Discord. S1/S2 используют [официальные сочетания Discord](https://support.discord.com/hc/en-us/articles/31232432266647-Discord-Commands-Shortcuts-and-Navigation-Guide). Deafen выключает звук Discord и также отключает микрофон. S3 ищет элемент камеры в доступном через macOS Accessibility интерфейсе Discord, а не управляет камерой всей системы. Для него нужен открытый voice/video call и видимый элемент управления камерой. Если Discord не запущен или элемент не найден, возвращается `E003`; никаких клавиш или кликов наугад не отправляется. Если в Discord включён предварительный просмотр видео, после S3 может потребоваться подтверждение в интерфейсе. [Описание управления камерой у Discord](https://support.discord.com/hc/en-us/articles/360041721052-Video-Calls).

Для физической проверки последовательно нажать S1 и сверить значок mute, нажать S2 и сверить значок Deafen, затем войти в звонок и нажать S3, сверив состояние видео. При включении камеры проверить и возможный экран предпросмотра. Журнал показывает отправку shortcut или успешный Accessibility press, но не подтверждает, что Discord изменил состояние; такое подтверждение даёт проверка в интерфейсе.

LED4 постоянно горит при handshake/config/heartbeat; без bridge мигает, display `----`. Ready display `P001`, нажатие `b001`/`b002`/`b003`, успех — кратко `0001`/`0002`/`0003` и соответствующий LED на 150 ms. Затем LED гаснет, display возвращается к profile. Sound по умолчанию выключен. Это stateless feedback: LED не показывает фактическое состояние mute/deafen/camera, а только принятый результат действия.

Bridge запоминает USB serial number выбранного устройства и reconnect делает только к нему, даже при смене port path. Если плата отсутствует при запуске, передать `--serial-number` явно. Ctrl+C закрывает порт; device станет offline через 3 s. Log печатается в stdout; при необходимости перенаправить в файл вне Git. `--no-actions` позволяет проверять transport без клавиш: такой режим всегда возвращает ERR 3, не фиктивный OK.

## Accessibility

Helper перед каждым action вызывает `CGPreflightPostEventAccess()`; для S3 дополнительно проверяется `AXIsProcessTrusted()`. Если доступа нет, exit code 2 превращается в RESULT ERR 2 и display `E002`. Получить системный запрос можно командой:

```sh
companion/.build/ShieldDeckKeys --request-permission
```

В System Settings → Privacy & Security → Accessibility разрешение включается пользователем для указанного macOS приложения. Для CLI это может быть запускающее приложение (Codex/Terminal), а не сам helper. После включения повторить `--check`; без `ACCESSIBILITY_OK` не считать настройку успешной. Программа не меняет TCC database и не обходит согласие. Источники: [Apple Accessibility](https://support.apple.com/guide/mac-help/allow-accessibility-apps-to-access-your-mac-mh43185/mac), [CoreGraphics preflight](https://developer.apple.com/documentation/coregraphics/cgpreflightposteventaccess()).

Для S1/S2, если пользователь держит Command/Shift/Control/Option на клавиатуре, helper возвращает BUSY, чтобы не смешивать нажатия. При action timeout bridge сообщает неизвестный результат и оставляет runner busy до фактического завершения, без повторного запуска. Старый результат после reset/disconnect не переносится в новую session. Не закрывать/убивать helper принудительно в момент отправки клавиш. Для безопасной проверки наличия элемента камеры без нажатия: `companion/.build/ShieldDeckKeys --discord-camera-check`.

## Проверки

```sh
~/.platformio/penv/bin/python -m unittest discover -s companion -p 'test_*.py' -v
```

Unit tests не отправляют клавиши и не открывают приложения. Покрыты duplicate/in-flight events, denied permissions, wrong session/revision, недопустимые номера кнопок, busy, heartbeat, timeout, reset, framing, маршрутизация трёх actions, недоступная камера и no-actions mode. Физические тесты и визуальный результат Mac выполняются отдельно.

Будущий native app: Swift + SwiftUI Configure, AppKit menu bar. Первоначальная проверка CLI выполняется на текущем Mac; совместимость других macOS ещё не проверена.

Описание компонентов, action types и permissions находится в [архитектуре](../docs/architecture.md). Контракт с Arduino — в [Serial protocol](../docs/protocol.md).

Личная конфигурация будет храниться в Application Support, вне репозитория. Примеры profiles допускаются только с безопасными демонстрационными значениями.
