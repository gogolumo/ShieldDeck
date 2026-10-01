# ShieldDeck v1: архитектура и UX

Дата проектирования: 2026-10-01. Это спецификация будущего продукта, а не описание уже работающих функций.

## 1. Product concept

ShieldDeck — три физические команды для текущего рабочего контекста. Arduino обслуживает hardware и передаёт события; Mac хранит назначения, выполняет действия и сообщает результат. Один USB кабель обеспечивает питание и Serial. Дополнительные компоненты для v1 не нужны.

Главный принцип: устройство не должно показывать неподтверждённое состояние microphone/camera. Нажатие, отправка команды и реальный результат — разные события.

## 2. Hardware и проверенные предпосылки

В существующем `platformio.ini` выбрана `board = uno`, платформа `atmelavr`: проект настроен на классическую ATmega328P Uno. Это ещё не идентификация физической платы. До прошивки проверяем её маркировку и USB bridge. На Mac обнаружен `/dev/cu.usbmodem11101`; путь может изменяться и не должен быть зашит в приложение.

| Элемент | Pins | Примечание |
| --- | --- | --- |
| Buttons 1–3 | A1, A2, A3 | Active-low; чтение как digital input |
| LEDs для buttons 1–3 | D10, D11, D12 | Логическая нумерация проекта |
| Status LED4 | D13 | Надпись D1 на shield может означать другой порядок |
| Display | latch D4, clock D7, data D8 | Два 74HC595, четыре multiplexed digits |
| Buzzer | D3 | Полярность и тип проверяем отдельно |
| Trimmer | A0 | В v1 не используется |
| Свободные | D5, D6, D9, A5 | Расширение не требуется; A5 у Uno не PWM |

Распиновка сверена с [руководством Velleman](https://cdn.velleman.eu/downloads/29/vma209_a4v01.pdf). В примерах есть спорные комментарии о LED ON/OFF, поэтому электрическую полярность и физический порядок проверяем в Phase 0; logical ON прячем за driver. Существующие примеры с `delay()` — учебные демонстрации, не основа runtime.

## 3. UX трёх кнопок

- Debounce: исходно 25 ms стабильного уровня на обоих фронтах, затем проверка на реальной плате.
- Short press: один запуск при отпускании после debounce. Так hold не запускает случайно short action.
- Long press: от 900 ms. Только button 3 переключает на следующий profile; short action после hold подавляется. Buttons 1/2 при hold не выполняют дополнительных действий.
- Удержание не повторяет команду. Double press и комбинации в v1 отсутствуют.
- На принятом нажатии сразу показываем `b001`/`b002`/`b003`; для stateless LED — короткая вспышка. Это подтверждает кнопку, а не успех действия.
- Одновременно исполняется одна команда. Новые команды во время исполнения отклоняются с `E004`; они не копятся для неожиданного выполнения позже.

Короткое нажатие естественно добавляет длительность удержания к задержке запуска. Это сознательная цена простой и предсказуемой механики short/long. При смене profile или reconnect удерживаемые кнопки сначала должны быть отпущены: новое действие возможно только с нового нажатия.

## 4. Четыре LEDs и настоящий state

LED1–3 соответствуют button 1–3. В начальном MVP все команды stateless: LED обычно погашен, вспыхивает после нажатия и отдельно после результата. Постоянные ON/OFF включаются только для action с проверенным StateProvider.

| Режим | Индикация |
| --- | --- |
| Stateless | 60 ms при нажатии, 150 ms при успешной отправке/завершении |
| Observed ON | Постоянно горит |
| Observed OFF | Погашен |
| Observed UNKNOWN / устарел | Две короткие вспышки каждые 2 s |
| Ошибка команды | `E00x` на display, детали в Mac; observed LED не подменяется |
| Offline | LED1–3 погашены, display `----` |

Для будущего microphone action выбираем **ON = подтверждённый MUTE**, красный LED напоминает, что звук выключен. Для camera action ON = подтверждённая передача видео в выбранном приложении. Различие подписывается в Configure и на сменных labels (`MUTE`, `CAM`); пользователь не должен угадывать смысл. UNKNOWN никогда не выдаётся за OFF. Observed LED не мигает при нажатии: немедленный feedback даёт display, чтобы не искажать state.

LED4 сообщает только о соединении: медленно мигает, пока companion отсутствует; постоянно горит после handshake/config; быстро мигает при несовместимой версии. При полностью выключенной плате он, естественно, не работает. Profile виден на display, поэтому отдельный profile LED не нужен.

## 5. Display и sound

Используем маленький проверяемый набор glyphs: цифры, `P`, `b`, `E`, `-`, пробел. Не пытаемся написать MIC, CAM, ON или длинные названия на seven-segment.

| Ситуация | Display | Длительность |
| --- | --- | --- |
| Ожидание / offline | `----` | До соединения |
| Ready | `P001` … `P099` | Текущий profile |
| Кнопка | `b001` … `b003` | Минимум 150 ms; затем до результата, максимум 30 s |
| Успех | Возврат к `Pnnn` | Краткий LED feedback для stateless |
| Ошибка | `E001` … `E006` | 1.5 s, затем profile/offline |
| Смена profile | Новый `Pnnn` | Два коротких гашения display |

Приоритет: offline/version fault → ошибка → pending action → profile. Реальная индикация должна быть проверена глазами на shield. Несколько overlays не останавливают сканирование display.

Buzzer полезен как необязательный feedback, по умолчанию выключен. При включении: success — 20 ms, profile — два импульса по 25 ms с паузой 60 ms, error — 100 ms. Отличаем длительностью, не обещаем разные высоты звука до проверки типа buzzer. Никаких сигналов на каждом heartbeat или постоянного писка. В offline звук выключается. Trimmer оставляем свободным; brightness через него — возможный следующий учебный эксперимент, не обязательная функция.

## 6. HID или Serial

| Подход | Преимущества | Ограничения |
| --- | --- | --- |
| USB HID | На совместимой плате обычные клавиши работают без companion | Сам по себе не обеспечивает профили, app integration и обратный state |
| USB Serial + companion | Mapping меняется на Mac; доступны scripts, Shortcuts, feedback, profiles | Нужен запущенный companion и разрешения для конкретных actions |

Выбираем **Serial**. У классической Uno главный ATmega328P не имеет native USB: на оригинальной Uno R3 USB обслуживает отдельный ATmega16U2. Leonardo/Micro с USB-capable MCU подходят для стандартного Keyboard workflow. Переделка USB bridge Uno — отдельный проект, а совместимые платы могут иметь другой bridge; в v1 этого не делаем. Источники: [Uno R3 datasheet](https://docs.arduino.cc/resources/datasheets/A000066-datasheet.pdf), [Arduino Keyboard](https://docs.arduino.cc/language-reference/en/functions/usb/Keyboard/).

## 7. macOS permissions и пределы управления

| Действие | Разрешение / проверка |
| --- | --- |
| Чтение Serial выбранной платы | Обычный локальный Serial доступ; порт может быть занят Serial Monitor |
| Генерация keyboard shortcuts | Accessibility; проверка перед запуском |
| AppleScript, управляющий другим app | Automation для конкретного target; UI scripting дополнительно Accessibility |
| Open Application / URL | Обычно без Accessibility; выбранный ресурс может иметь ограничения доступа |
| Shell / macOS Shortcut | Разрешения зависят от действий внутри; отсутствие разрешения возвращает ошибку |
| Работа с защищёнными файлами | Возможны отдельные Files and Folders prompts; Full Disk Access по умолчанию не запрашиваем |

Accessibility и Automation — разные разрешения. Запрашиваем их при настройке нужного action с объяснением. Отказ не блокирует остальные функции. В v1 приложение не записывает audio/video и не перехватывает глобальный ввод, поэтому blanket Camera, Microphone и Input Monitoring permissions не нужны. Отправить Cmd+Shift+4 — ещё не значит получить screenshot: пользователь может отменить выделение.

Источники: [Apple: Accessibility](https://support.apple.com/en-gu/guide/mac-help/mh43185/mac), [Apple: Automation](https://support.apple.com/en-nz/guide/mac-help/mchl108e1718/mac), [Apple: UI scripting](https://developer.apple.com/library/archive/documentation/LanguagesUtilities/Conceptual/MacAutomationScriptingGuide/AutomatetheUserInterface.html).

### Microphone

Core Audio предоставляет [kAudioDevicePropertyMute](https://developer.apple.com/documentation/coreaudio/kaudiodevicepropertymute). Будущий adapter должен проверить наличие и возможность записи свойства у конкретного input device, прочитать значение после изменения и отслеживать его обновления. Если capability отсутствует — action недоступен, без скрытой имитации через input gain = 0.

**Инженерный вывод:** mute выбранного устройства не доказывает mute в Zoom/Teams и не покрывает остальные microphones. Приложение может использовать другое устройство, а software mute внутри meeting app — независимое состояние. Поэтому универсальный «Mute всего Mac» в v1 не обещаем.

### Camera

Документированное [isInUseByAnotherApplication](https://developer.apple.com/documentation/avfoundation/avcapturedevice/isinusebyanotherapplication?language=objc) сообщает об использовании capture device. Оно не даёт состояния «моё видео отправляется в текущий meeting» и не является переключателем чужой camera session.

**Решение проекта:** глобальный Camera Toggle не входит в v1. Текущий CLI prototype имеет отдельный Discord action: через Accessibility нажимает подписанный элемент камеры в открытом звонке и возвращает ошибку, если элемент недоступен. Это действие не является StateProvider: остаётся momentary feedback и необходима визуальная проверка в Discord. Позже возможен adapter для конкретного app с проверенным API/state. Не выключаем системные службы, не меняем TCC через shell и не выдаём последнее нажатие за privacy guarantee.

## 8. MVP actions

Реализуем шесть типов с общей границей validate / execute / result:

| Type | Параметры | Что означает успех |
| --- | --- | --- |
| Keyboard Shortcut | Key code, modifiers, foreground или конкретный bundle ID | События отправлены; результат внутри app неизвестен |
| Open Application | Bundle ID | Система приняла запуск/активацию |
| Open Resource | File/folder path либо http/https URL | Система приняла открытие |
| Run macOS Shortcut | Имя пользовательского Shortcut | CLI завершился с exit code 0 |
| Run Shell Command | Введённая пользователем команда, cwd, timeout | Процесс завершился с exit code 0 |
| Run AppleScript | Пользовательский текст, target при необходимости | Script завершился без ошибки |

Для Shortcuts используем системный CLI с аргументами, без склеивания имени в shell. Apple описывает [запуск Shortcuts из command line](https://support.apple.com/en-nz/guide/shortcuts-mac/-apd455c82f02/mac); интерактивный Shortcut может ждать пользователя, поэтому timeout обязателен.

Cmd+Tab, Cmd+C/V и Cmd+Shift+4 — presets Keyboard Shortcut. При завершении и ошибке отпускаем все синтезированные modifiers. Foreground preset работает с активным приложением; target-specific preset проверяет bundle ID и активацию, иначе ошибка. Раскладка и Secure Input могут ограничивать результат; это проверяется на Mac пользователя.

Play/Pause, Previous/Next сначала предлагаются только как проверенные app-specific AppleScript/Shortcut presets. Универсальные media keys, volume adapter, наблюдаемый microphone, hide/show и graceful Quit — расширения. Focus можно назначить через пользовательский Shortcut, без обещания наблюдаемого Focus state. Нельзя маскировать force kill под Close Application.

Shell editor показывает точный текст и пометку «выполнится от вашего пользователя». Сохранение привязывает команду к кнопке; Test — явный запуск. Не скачиваем и не исполняем code автоматически, не используем sudo/root, импортированные scripts до просмотра выключены. Shell по природе способен менять файлы: это не sandbox. Для других action types параметры передаются отдельными arguments. stdout/stderr ограничены по размеру, секреты и полные scripts не попадают в обычный журнал.

## 9. Profiles

Один profile содержит UUID, имя, display slot 1–99, три bindings и настройки feedback. UUID стабилен при переименовании; slot нужен только для четырёхзначного display. Create, rename, duplicate, delete и выбор active profile доступны в Configure. Последний profile удалить нельзя; дубликат получает новый UUID и свободный slot.

Первый рабочий General: 1 → Cmd+Tab, 2 → Open Application, 3 → Cmd+Shift+4. Meeting/Coding/Study/Media создаются как редактируемые шаблоны, неподтверждённые actions выключены. Meeting не содержит работающие «по умолчанию» глобальные mic/camera switches.

В menu bar выбираем profile напрямую, hold button 3 циклически переключает enabled profiles. При единственном profile hold ничего не меняет. Mac — единственный источник active profile. Arduino показывает новый номер только после принятия CONFIG. Смена во время действия или нажатой кнопки откладывается до idle/release; быстрые повторные запросы переключения не накапливаются.

## 10. Serial protocol

Текстовые строки, version 1, 115200 baud, 8N1. Полный контракт находится в [protocol.md](protocol.md). Firmware отправляет только идентификаторы buttons/events; имена приложений и shell commands на плату не передаются.

Есть handshake, session ID, config revision, event ID, ACK, RESULT и heartbeat. Это нужно для отличения старого результата от нового нажатия, а не для создания сложного RPC. В MVP нет автоматического повторения actions после timeout или reconnect.

## 11. Firmware architecture

| Модуль | Ответственность |
| --- | --- |
| ButtonManager | Active-low inputs, debounce, short/long, release gate |
| DisplayManager | Glyph map, digit scan, framebuffer, временные overlays |
| LedManager | Logical ON/OFF, stateless pulse, unknown pattern, connection LED |
| BuzzerManager | Неблокирующая последовательность импульсов и sound off |
| SerialManager | Ограниченный parser, handshake, очередь TX, heartbeat |
| AppController | State machine, pending event, profile revision, приоритет feedback |

Это логические границы; на ранних уроках не нужно создавать шесть файлов ради одного button. Pins и полярность — единая hardware configuration. Строки Serial — fixed-size buffers; избегаем heap `String`, бесконечных очередей, `readStringUntil()` с ожиданием и длинных `delay()`.

Главный loop регулярно обслуживает все managers. Display: исходная цель — один digit каждые 2 ms, около 125 полных кадров/s. Тайминги button/beep/overlay — через `millis()`, scan при необходимости через `micros()`, с корректным unsigned elapsed при wraparound. Serial за одну итерацию получает ограниченный byte budget; TX отправляется только при свободном месте. Иначе Serial flood способен остановить display. Timer interrupts добавляем только если измерения покажут необходимость.

### Как будем изучать display

Сначала объясняем 74HC595: serial bits накапливаются внутри, latch одновременно переносит их на outputs. Два последовательно соединённых регистра дают 16 выходных бит. Затем разбираем `shiftOut()` — передачу одного byte с выбранным порядком битов. После этого экспериментально сопоставляем bits с сегментами a–g/decimal point, затем определяем digit selection и порядок digits. Только потом пишем multiplexing driver: быстро выбираем по одному digit и подаём нужные segments. При смене frame предусматриваем blanking для проверки ghosting. До этого этапа готовый driver не выдаём.

## 12. Companion architecture

Native menu bar приложение: AppKit status item, SwiftUI Configure. Меню показывает Connected/Offline, имя profile, три action labels, Configure и Quit. Configure: выбор порта, profiles, action type/parameters, Test, описание feedback, sound on/off, разрешения и последние ошибки. LED mode Auto определяется capabilities; display использует фиксированные patterns v1. Произвольные LED/display patterns оставляем расширению, fake persistent state не предлагаем.

| Компонент | Ответственность |
| --- | --- |
| SerialTransport | Discovery, выбранный port, exclusive open, read/write, reconnect |
| ProtocolSession | Версия, session/config IDs, heartbeat, event validation/dedup |
| ProfileStore | Локальная versioned JSON config, atomic save, migration и backup |
| ActionRegistry / ActionRunner | Валидация, handlers шести типов, execution queue, timeout |
| PermissionService | Проверки и объяснение Accessibility/Automation отказов |
| StateStore / StateProvider | Только наблюдаемые значения, источник и freshness |
| FeedbackCoordinator | Преобразование результата/state в протокол, без mic/Zoom логики в firmware |
| AppModel / Views | UI на main thread, status menu и Configure |

Serial работает независимо от UI и script execution. Блокирующие процессы не выполняются на main thread. Приложение выбирает порт один раз; не отправляет команды всем найденным Serial devices. Reconnect учитывает reset Uno при открытии порта, sleep/wake и занятость порта. Disconnect при прошивке освобождает port.

Начальный вариант — локальный app вне App Store sandbox, без daemon/root helper; стабильный bundle ID. Signing/notarization рассматриваем перед распространением. Никаких cloud accounts или backend для MVP.

Timeout action — 30 s, без интерактивного terminal. При timeout companion пытается остановить управляемый процесс и сообщает, что уже выполненные side effects не откатываются; запуск Shortcut может пережить завершение CLI. Повторный запуск не автоматический. До подтверждённого завершения собственного процесса очередь остаётся занятой.

## 13. State synchronization и state machine

Action state имеет три модели: Stateless, Observed(value + source + timestamp), Unknown. Последняя команда хранится в activity log отдельно и не используется как наблюдаемый state. После смены profile, adapter error, sleep, device change или истечения freshness observed state становится Unknown.

Будущий StateProvider перечитывает состояние после команды, подписывается на изменения и делает сверку не реже раза в 2 s. Он обновляет STATE с TTL 5 s только после валидного чтения. Отсутствие новых events само по себе не обновляет свежесть. Heartbeat подтверждает соединение, но не свежесть microphone state. Для app-specific adapter событие «пользователь нажал мышью mute» должно изменить LED без нажатия ShieldDeck. Если это невозможно проверить, adapter остаётся stateless.

Основная connection machine:

```text
BOOT → WAIT_FOR_MAC → HANDSHAKE → SYNCING → READY
          ↑              |          |        |
          └──── timeout/disconnect/reset ─────┘
Несовместимая версия → VERSION_ERROR → новый handshake
```

В READY отдельно живёт action machine `IDLE → WAIT_ACK → RUNNING → IDLE`. Ошибка результата даёт короткий overlay, а не останавливает устройство. Смена profile идёт через SYNCING и блокирует новые действия до принятия config. Lost heartbeat немедленно сбрасывает pending и persistent LEDs; offline events не сохраняются.

## 14. Roadmap

Порядок: hardware validation → button events → Serial handshake + feedback → маленький Mac prototype → native configurable app → profiles → проверяемые state adapters → enclosure/polish. Подробные задания и exit criteria: [roadmap.md](roadmap.md).

## 15. Definition of Done и слабые места

v1 — повседневный configurable macro pad с шестью action types, profiles, устойчивым reconnect и честным feedback. Наблюдаемые mic/camera adapters не являются условием выпуска, но никакая UI надпись не должна обещать их поддержку. Проверяемые критерии: [Definition of Done](roadmap.md#definition-of-done-shielddeck-v1).

Слабые места: неудобные маленькие shield buttons; отсутствие подписей на display; dependency от работающего Mac app; TCC permissions; активное окно и раскладка; несовместимые app shortcuts; Serial reset/занятый port; ограниченные SRAM и runtime budgets Uno. Точное «нажал один раз — выполнилось ровно один раз» при crash между действием и ответом не гарантируется: показываем неопределённый результат, не повторяем.

Позже: 3D-printed enclosure с USB доступом и сменными бумажными labels, blank key caps/наклейками или простыми icons. Сначала измерить реальную сборку и высоту buttons. OLED/LCD, rotary encoder и layers — отдельная v2, не скрытая покупка для v1.
