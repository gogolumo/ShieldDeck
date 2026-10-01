# ShieldDeck Serial protocol v1 — draft

Статус: проект контракта; обе реализации ещё отсутствуют. Wire format — ASCII, separator `|`, terminator LF; CR перед LF допустим и удаляется. 115200 baud, 8N1. Длина строки до 95 bytes без terminator, buffer 96 bytes с завершающим NUL. Переполнение: отбросить всю строку до LF, не исполнять её префикс. Неоконченная строка удаляется после 500 ms без новых bytes.

Точные число полей, направления, enum values и диапазоны проверяются до изменения состояния. Пустые поля, NUL, прочие control characters и неизвестные команды не исполняются. Ошибки parser логируются с rate limit на Mac; на плате не вызывают звуковой/Serial storm. CRC в v1 не добавляем; это локальный USB transport, не аутентифицированная сеть.

## Идентификаторы и диапазоны

| Поле | Значение |
| --- | --- |
| `sid` | 8 uppercase hexadecimal characters; новый случайный session ID от Mac на каждое соединение |
| `rev` | Config revision 1–65535; не переиспользуется в session |
| `seq` | Event ID 1–65535; монотонный в session, один счётчик для всех buttons |
| `button` | 1, 2 или 3 |
| `profile` | Display slot 1–99; на display дополняется нулями |
| `sound` | 0 или 1 |
| `mode1..3` | `PULSE` или `OBSERVED`; в начальном MVP только PULSE |
| `value` | `ON`, `OFF`, `UNKNOWN` |

Перед переполнением rev/seq требуется новая session; старые IDs не оборачиваются в той же session. Sid не секрет и не security boundary. Единовременно один Mac process владеет выбранным port.

## Handshake и config

```text
Arduino → HELLO|SHIELDDECK|1
Mac     → WELCOME|1|A1B2C3D4
Arduino → READY|A1B2C3D4
Mac     → CONFIG|A1B2C3D4|1|1|0|PULSE|PULSE|PULSE
Arduino → CONFIGURED|A1B2C3D4|1
```

CONFIG поля: sid, rev, profile, sound, mode1, mode2, mode3. Применяется атомарно: profile/modes/sound меняются вместе, старые observed states очищаются в UNKNOWN, stateless LEDs выключаются. Новый CONFIG принимается только при idle и отпущенных кнопках; иначе Arduino отвечает `REJECT|sid|rev|BUSY`. Отказ не меняет active config. До первого CONFIG действие кнопок не отправляется.

Arduino выдаёт HELLO раз в секунду в WAIT_FOR_MAC; после WELCOME ждёт CONFIG до 3 s. Mac при открытии порта ждёт HELLO до 5 s с учётом возможного reset платы. Без HELLO показывает ошибку подключения; reconnect с задержкой 1, 2, 4, максимум 5 s. В состоянии READY Arduino игнорирует непрошеный WELCOME; новый HELLO во время session означает reset и требует от Mac новой session.

В ответ на другой protocol version Mac показывает «несовместимая версия» и ничего не выполняет. Если Arduino получает WELCOME с неподдерживаемой версией, выдаёт `VERSION|1` и переходит в VERSION_ERROR с периодическим HELLO. Произвольные строки, похожие на log, не считаются handshake.

Mac сохраняет старый mapping до CONFIGURED и блокирует запуск новых actions во время SYNCING. После отправки CONFIG с rev R события старой revision отклоняются, даже если уже были в пути. Отправленный CONFIG можно повторить с тем же rev и идентичным содержимым: плата подтверждает без повторного profile feedback. Совпавший rev с другим содержимым — protocol error, reconnect. При BUSY Mac ждёт release/idle и повторяет тот же CONFIG, максимум 3 s. При отсутствии согласования начинает новую session вместо угадывания profile.

## Button events и результаты

```text
Arduino → BTN|A1B2C3D4|1|42|1|SHORT
Mac     → ACK|A1B2C3D4|1|42
Mac     → RESULT|A1B2C3D4|1|42|OK|0
```

BTN поля: sid, rev, seq, button, gesture (`SHORT` или `LONG`). SHORT — primary mapping. LONG только button 3 — системный запрос следующего profile; LONG от buttons 1/2 в v1 не отправляется.

На принятом нажатии Arduino показывает локальный feedback; SHORT отправляет при отпускании, LONG — один раз после порога. Firmware хранит один pending event. При дополнительном short во время pending сообщает локальный E004, нового BTN не отправляет и не откладывает его. Макрос не стартует без active session/config и совпадения sid/rev.

ACK означает только «принято исполнителем», RESULT — результат. Mac отправляет ACK перед началом action; duplicate seq никогда не запускает повторный action. Mac хранит high-water seq, текущую запись и последний завершённый результат: текущему duplicate возвращает ACK, последнему завершённому — сохранённый RESULT, более старые игнорирует. Новому seq при занятом runner отвечает RESULT ERR 4 без ACK; его seq тоже считается обработанным. Новому seq с неправильной revision возвращает RESULT ERR 6 и не исполняет.

Для profile hold Mac отвечает ACK и OK после проверки возможности перехода, затем отправляет CONFIG нового profile; именно CONFIGURED завершает переключение. Если кнопка ещё удерживается, Mac повторяет CONFIG по правилам BUSY. При единственном profile возвращает OK без CONFIG. Error/timeout при CONFIG не должен показывать ложный новый profile.

Firmware ждёт ACK максимум 1 s, окончательный RESULT максимум 30 s от отправки BTN. RESULT допустим и без полученного ACK. После timeout показывает E001 и не повторяет BTN; поздний RESULT игнорирует. Mac продолжает учитывать реально работающий процесс как busy. Не переиспользуем seq после timeout. OK для keyboard значит «отправлено», а не «приложение действительно переключилось»; это явно указывается в activity log.

```text
Mac → RESULT|A1B2C3D4|1|42|ERR|2
```

| Code | Display | Смысл |
| --- | --- | --- |
| 0 | Profile | Успех, допустим только с OK |
| 1 | E001 | Timeout / результат неизвестен |
| 2 | E002 | Permission denied |
| 3 | E003 | Target/resource/action недоступен |
| 4 | E004 | Busy |
| 5 | E005 | Execution failed |
| 6 | E006 | Config/protocol mismatch |

ERR допустим только с codes 1–6. Текст подробной ошибки остаётся в Mac; длинные stderr и scripts не передаются по Serial. Firmware сопоставляет RESULT с pending sid/rev/seq; результат для другого события ничего не меняет.

## Heartbeat и disconnect

```text
Mac     → PING|A1B2C3D4
Arduino → PONG|A1B2C3D4
```

Mac отправляет PING каждую секунду независимо от UI и action runner. Arduino считает связь потерянной через 3 s без валидного PING своей session; Mac — через 3 s без PONG. Heartbeat запускается сразу после READY, включая SYNCING. При disconnect/reset: pending очищается, LEDs1–3 выключаются, sound off, display `----`, новый handshake. Старые события, результаты и states не переносятся в новую session. Кнопки должны быть отпущены перед новой работой.

Mac при disconnect блокирует запуск новых actions, пытается отменить свои процессы и не повторяет их после reconnect. Уже выполненные внешние действия нельзя гарантированно отменить. Crash Mac между выполнением и RESULT означает неизвестный результат; пользователь проверяет его, а не получает автоматический retry.

## Observed state — зарезервированная возможность v1

```text
Mac → STATE|A1B2C3D4|1|1|ON|5000
Mac → STATE|A1B2C3D4|1|2|UNKNOWN|0
```

Поля: sid, rev, button, value, ttl_ms. ON/OFF требуют TTL 1000–5000 ms; UNKNOWN требует 0. STATE действует только для OBSERVED binding текущей config, не является результатом команды и может приходить при внешнем изменении state. После TTL автоматически UNKNOWN, даже при исправном heartbeat. TTL измеряется от приёма по monotonic clock платы, не требует общей синхронизации часов.

StateProvider отправляет ON/OFF только после свежего наблюдения, максимум раз в 2 s или при изменении. При изменении назначения увеличивается rev; запоздалый state старой revision игнорируется. В MVP без наблюдаемых adapters все mode = PULSE и STATE не используется. Этот резерв не означает готовность microphone/camera integration.

## Feedback и диагностический режим

Отдельные LED/DISPLAY/BEEP команды в рабочем v1 не нужны: firmware выводит согласованный feedback по CONFIG, BTN и RESULT. Это сокращает протокол и исключает случайное постоянное «MIC OFF» из макроса. Настройки v1: sound on/off; LED feedback — Auto по capabilities, display patterns фиксированы. Configure объясняет их смысл. Произвольные patterns потребуют согласованного расширения протокола и не показываются как доступные настройки.

Для Serial Monitor companion отключается и освобождает port. HELLO, WELCOME, CONFIG и остальные строки можно вводить вручную; для сохранения session требуется PING чаще одного раза в 3 s. Упрощённые учебные сообщения Phase 1 явно считаются diagnostic firmware и не смешиваются с production protocol.

## Проверки контракта перед реализацией

- Разбить сообщение по любым границам чтения; склеить несколько сообщений в одном чтении.
- CRLF, слишком длинная строка, неверное поле, неверный enum, незавершённая строка.
- Неизвестный sid/rev, duplicate BTN, поздний RESULT, reset между ACK и RESULT.
- CONFIG потерян/повторён, hold во время CONFIG, reply BUSY, events во время SYNCING.
- Потеря PING/PONG, sleep/wake, занятый port и unplug во время script.
- TTL истёк при продолжающемся heartbeat: observed LED переходит UNKNOWN.
- seq/rev перед wraparound: новая session без повторного action.
