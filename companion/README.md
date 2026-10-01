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

Перед запуском закрыть Serial Monitor/capture; портом владеет только bridge. Firmware должна быть собрана и загружена через `pio run -e uno -t upload`. Для возврата к hardware tests используется `-e smoke`.

Назначения выполняются на отпускании кнопки:

| Кнопка | Действие | Что подтверждает OK |
| --- | --- | --- |
| S1 | Cmd+Tab | Keyboard events отправлены |
| S2 | Открыть/активировать Калькулятор | Launch Services принял запрос для `com.apple.calculator` |
| S3 | Cmd+Shift+4 | Отправлен shortcut выбора области screenshot; создание файла не подтверждается |

Калькулятор реализует выбранный в архитектуре Open Application preset; глобальный Play/Pause пока не добавлен. S3 можно отменить клавишей Escape, не создавая изображение. Калькулятор не требует Accessibility; S1/S3 требуют.

LED4 постоянно горит при handshake/config/heartbeat; без bridge мигает, display `----`. Ready display `P001`, нажатие `b001`/`b002`/`b003`, успех — кратко `0001`/`0002`/`0003` и соответствующий LED на 150 ms. Затем LED гаснет, display возвращается к profile. Sound по умолчанию выключен. Это stateless feedback; видимый результат подтверждается пользователем.

Bridge запоминает USB serial number выбранного устройства и reconnect делает только к нему, даже при смене port path. Если плата отсутствует при запуске, передать `--serial-number` явно. Ctrl+C закрывает порт; device станет offline через 3 s. Log печатается в stdout; при необходимости перенаправить в файл вне Git. `--no-actions` позволяет проверять transport без клавиш: такой режим всегда возвращает ERR 3, не фиктивный OK.

## Accessibility

Helper перед каждым action вызывает `CGPreflightPostEventAccess()`. Если доступа нет, exit code 2 превращается в RESULT ERR 2 и display `E002`. Получить системный запрос можно командой:

```sh
companion/.build/ShieldDeckKeys --request-permission
```

В System Settings → Privacy & Security → Accessibility разрешение включается пользователем для указанного macOS приложения. Для CLI это может быть запускающее приложение (Codex/Terminal), а не сам helper. После включения повторить `--check`; без `ACCESSIBILITY_OK` не считать настройку успешной. Программа не меняет TCC database и не обходит согласие. Источники: [Apple Accessibility](https://support.apple.com/guide/mac-help/allow-accessibility-apps-to-access-your-mac-mh43185/mac), [CoreGraphics preflight](https://developer.apple.com/documentation/coregraphics/cgpreflightposteventaccess()).

Если пользователь держит Command/Shift/Control/Option на клавиатуре, helper возвращает BUSY, чтобы не смешивать нажатия. При action timeout bridge сообщает неизвестный результат и оставляет runner busy до фактического завершения, без повторного запуска. Старый результат после reset/disconnect не переносится в новую session. Не закрывать/убивать helper принудительно в момент отправки клавиш.

## Проверки

```sh
~/.platformio/penv/bin/python -m unittest discover -s companion -p 'test_*.py' -v
```

Unit tests не отправляют клавиши и не открывают приложения. Покрыты duplicate/in-flight events, denied permissions, wrong session/revision, недопустимые номера кнопок, busy, heartbeat, timeout, reset, framing, маршрутизация трёх actions, Launch Services failure и no-actions mode. Физические тесты и визуальный результат Mac выполняются отдельно.

Будущий native app: Swift + SwiftUI Configure, AppKit menu bar. Первоначальная проверка CLI выполняется на текущем Mac; совместимость других macOS ещё не проверена.

Описание компонентов, action types и permissions находится в [архитектуре](../docs/architecture.md). Контракт с Arduino — в [Serial protocol](../docs/protocol.md).

Личная конфигурация будет храниться в Application Support, вне репозитория. Примеры profiles допускаются только с безопасными демонстрационными значениями.
