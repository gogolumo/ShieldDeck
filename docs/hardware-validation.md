# Hardware validation — текущий журнал

## 2026-10-01: Buttons smoke test

Получен актуальный `origin/main`, исходный commit `e822768`, рабочая ветка `main`. Исходная заготовка успешно собиралась перед изменениями.

| Проверка | Фактический результат |
| --- | --- |
| USB discovery | `/dev/cu.usbmodem11101`, VID:PID `2341:0043`, соответствует Uno в Arduino AVR boards.txt |
| MCU | `avrdude` прочитал signature `0x1e950f` — ATmega328P |
| PlatformIO | atmelavr 5.1.0, framework-arduino-avr 5.2.0, target `uno` |
| Build | PASS; Flash 3238 / 32256 bytes, static RAM 228 / 2048 bytes |
| Upload | PASS; 3238 bytes записаны и проверены чтением Flash |
| Serial | 115200 baud, boot banner и повторяющийся DIAG ALIVE получены |
| Входы при старте | A1 = HIGH, A2 = HIGH, A3 = HIGH |
| Физическое S1 / A1 нажатие | Один контролируемый тест пройден: ровно один PRESS, без повторов после инструкции удерживать 3 s |
| Физическое S2 / A2 нажатие | Один контролируемый тест пройден: ровно один PRESS, изменился только второй счётчик |
| Физическое S3 / A3 нажатие | Один контролируемый тест пройден: ровно один PRESS, изменился только третий счётчик |
| Полярность LEDs на реальной плате | Подтверждена пользователем: D10–D13 active-low, поочерёдное свечение и все OFF в конце |
| Display 1234, порядок digits, flicker | Пользователь подтвердил `1234` слева направо без заметного мерцания/лишних сегментов |
| Buzzer короткий beep | Пользователь подтвердил звук; Serial подтверждает возврат D3 в HIGH через 40 ms |
| End-to-end Cmd+Tab | Пользователь подтвердил переключение приложения и feedback; journal содержит SENT_CMD_TAB и RESULT OK |

Первый реальный Serial фрагмент (14:35 UTC):

```text
DIAG|SHIELDDECK|BUTTONS|1
DIAG|INPUT|1|HIGH
DIAG|INPUT|2|HIGH
DIAG|INPUT|3|HIGH
DIAG|ALIVE|2000|0|0|0
DIAG|ALIVE|4000|0|0|0
```

DIAG ALIVE содержит uptime в ms и три накопительных счётчика PRESS. `BTN|1|PRESS` выдаётся на стабильном LOW после 25 ms, один раз до следующего стабильного отпускания. Кнопка, удерживаемая при boot, не создаёт PRESS до отпускания и нового нажатия. Short/long классификация пока отсутствует.

Это диагностическая firmware, разрешённая разделом diagnostic protocol. Она не объявляет production HELLO и не выполняет Mac actions. Для bridge далее используем существующие sid/rev/seq, ACK и RESULT из protocol v1; второй production protocol не создаём.

По [схеме VMA209](https://cdn.shopify.com/s/files/1/0174/1800/files/vma209_scheme.pdf?v=1623322939) аноды LEDs подключены к +5 V, cathodes через резисторы — к D10–D13; LOW включает LED. Buzzer управляется PNP 3906: HIGH выключает его. Это вывод из схемы, а не физическое подтверждение экземпляра. В текущем тесте D3/D10–D13 удерживаются HIGH, display загашен через registers. Startup LED sequence, beep и multiplexing добавим отдельным hardware шагом после кнопок.

## Воспроизведение

Если `pio` отсутствует в PATH, на текущем Mac executable находится в `~/.platformio/penv/bin/pio`. Сначала закрыть Serial capture/Monitor перед upload.

```sh
~/.platformio/penv/bin/pio run -e uno
~/.platformio/penv/bin/pio run -e uno -t upload --upload-port /dev/cu.usbmodem11101
~/.platformio/penv/bin/python tools/serial_capture.py /dev/cu.usbmodem11101 --log /tmp/shielddeck-buttons-2026-10-01.log
```

Порт уточняется через `pio device list`, путь не зашит в firmware. Capture только читает Serial и сохраняет timestamped log, останавливается Ctrl+C; открытие порта может reset Uno. Не открывать параллельно второй Serial Monitor. Лог временный, вне Git; значимые подтверждённые результаты переносим сюда.

Host tests debounce (не заменяют физические тесты):

```sh
clang++ -std=c++11 -Wall -Wextra -Werror -fsanitize=address,undefined -Iinclude test/buttons_host.cpp -o /tmp/shielddeck-buttons-test
/tmp/shielddeck-buttons-test
```

PASS: bounce на нажатии и отпускании, одно событие при удержании, rearm после отпускания, boot-held, переполнение millis. Проверка CLI capture `--help` также прошла.

## Контролируемая проверка S1, 14:50:55 UTC

После инструкции удержать S1 3 секунды и отпустить пользователь сообщил «нажал». В Serial получено ровно одно новое событие:

```text
14:50:55.300 DIAG|ALIVE|38000|21|4|4
14:50:55.898 BTN|1|PRESS
14:50:57.303 DIAG|ALIVE|40000|22|4|4
14:51:11.314 DIAG|ALIVE|54000|22|4|4
```

Только первый счётчик увеличился на один; за следующие 15 s повторов не было. Это подтверждает один контролируемый тест S1/A1 и его active-low обработку. Точная длительность удержания в текущем diagnostic format не измеряется (RELEASE не передаётся).

Ранее в журнале была серия событий всех трёх кнопок и reset. Количество физических нажатий той серии неизвестно; её не используем как доказательство double-trigger или надёжности debounce.

## Контролируемая проверка S2, 14:52:08 UTC

Пользователь подтвердил одно нажатие S2. Получено ровно одно событие:

```text
14:52:07.354 DIAG|ALIVE|110000|22|4|4
14:52:08.975 BTN|2|PRESS
14:52:09.356 DIAG|ALIVE|112000|22|5|4
14:52:19.362 DIAG|ALIVE|122000|22|5|4
```

Второй счётчик увеличился с 4 до 5, остальные не изменились; за следующие 10 s повторов не было. Один контролируемый тест S2/A2 пройден. Это не заменяет длительную проверку debounce.

## Контролируемая проверка S3, 16:29:45 UTC

После USB disconnect в 14:56 UTC прежний capture завершился с `Device not configured`. Устройство снова обнаружено с тем же USB serial number; capture восстановлен в 16:29 UTC. Открытие порта перезапустило Uno, поэтому счётчики начали с нуля. Нажатия до восстановления чтения не считаем проверенными.

После повторного задания S3 получено:

```text
16:29:44.965 DIAG|ALIVE|28000|0|0|0
16:29:45.657 BTN|3|PRESS
16:29:46.964 DIAG|ALIVE|30000|0|0|1
16:29:56.974 DIAG|ALIVE|40000|0|0|1
```

Ровно один PRESS, изменился только третий счётчик, за следующие 11 s повторов нет. Все три кнопки прошли по одному контролируемому тесту; длительная проверка надёжности ещё не выполнена.

## Загружен smoke test LEDs, 16:31 UTC

Diagnostic firmware `BUTTONS_LEDS|2` добавляет отдельный LedManager: через 1.5 s после boot последовательно выставляет D10, D11, D12, D13 в LOW на 700 ms, затем HIGH с паузой 300 ms. После последовательности все четыре outputs остаются HIGH. S1 повторяет тест, если он уже завершён; нажатие во время последовательности её не перезапускает. Debounce и Serial продолжают обслуживаться, blocking delays нет. Buzzer/display остаются выключены.

Ожидаемая физическая картина по схеме: по одному загораются четыре разных LEDs, между ними короткие тёмные паузы, в конце все погашены. Serial `DIAG|LED|pin|LOW/HIGH` подтверждает только выданный уровень, а не свечение. Полярность на реальной плате будет подтверждена только ответом пользователя.

Build/upload PASS: Flash 3580 / 32256 bytes, static RAM 234 / 2048 bytes; avrdude подтвердил запись 3580 bytes. Host tests кнопок PASS. После открытия Serial получены banner `BUTTONS_LEDS|2`, по одному LOW/HIGH для pins 10, 11, 12, 13 и `DIAG|LED|DONE`. DIAG ALIVE продолжался во время последовательности, счётчики оставались нулевыми. Capture оставлен открытым в прежний log.

Следующее действие пользователя: нажать S1 один раз и сообщить, загорались ли четыре разных LEDs по одному и погасли ли все в конце. Display и buzzer на этом этапе не тестируются.

## Подтверждение LEDs и переход к display

На вопрос о четырёх поочерёдно загорающихся LEDs с полным выключением в конце пользователь ответил «работает». Тест LEDs принят: D10, D11, D12, D13 включаются LOW и выключаются HIGH. Воспроизведение через S1 также присутствует в Serial log. Это подтверждает электрическую полярность и работу четырёх выходов; физические надписи D1–D4 на shield не переименовываем и не смешиваем с Arduino pin numbers.

## Загружен display smoke test, 16:34 UTC

Diagnostic firmware `DISPLAY|3` постоянно сканирует `1234`. Отдельный DisplayManager использует latch D4, clock D7, data D8. Buzzer остаётся HIGH/off, кнопки и LED sequence работают как раньше.

Два 74HC595 соединены последовательно: первый отправленный byte проходит в U3 и задаёт сегменты a–g/decimal point (bits 0–7, active-low), второй остаётся в U2 и выбирает один из четырёх разрядов (bits 0–3, active-high). `shiftOut()` передаёт byte MSB-first; импульс latch применяет оба byte одновременно.

Для цифр 1/2/3/4 segment bytes: `F9/A4/B0/99`; digit masks: `01/02/04/08`. Перед каждым переключением driver фиксирует blank frame `FF/00`, затем нужные segments и digit mask. Один разряд обслуживается каждые 2000 us без `delay()`, полный кадр — примерно 8 ms. Наличие всех четырёх читаемых цифр и правильный порядок ещё требуют визуального подтверждения; `1234` не проверяет абсолютно каждый сегмент.

Build/upload PASS: Flash 4074 / 32256 bytes, static RAM 253 / 2048 bytes; avrdude подтвердил запись 4074 bytes. Serial содержит `DISPLAY|3`, heartbeat, LED sequence и S1 PRESS. Накопительный `DIAG|SCAN_MAX_US` в первых 8 s составил 2048 us, включая LED sequence и S1. Это измерение интервала программного scan, а не измерение яркости или оптического flicker.

Следующее действие пользователя: посмотреть на display и подтвердить `1234` слева направо без заметного мерцания и лишних сегментов. После этого — отдельный короткий buzzer test.

## Подтверждение display и buzzer test

Пользователь ответил «да» на вопрос о `1234` слева направо без заметного мерцания и лишних сегментов. Визуальная проверка этого pattern пройдена; проверка абсолютно всех сегментов и длительный endurance остаются отдельными задачами. До смены firmware накопительный max scan gap достигал 2576 us при последующих кнопочных тестах; это не меняет пользовательского наблюдения, но сохраняется как измеренная граница этого запуска.

Следующий diagnostic build `BUZZER|4`: BuzzerManager переводит D3 из HIGH в LOW примерно на 40 ms только по S2 и возвращает HIGH через `millis()`, без delay и timer interrupts. На boot звук не запускается. S1 по-прежнему повторяет LEDs, display показывает `1234`, S3 только отправляет PRESS.

Схема подтверждает PNP driver, но не наличие внутреннего генератора buzzer. Короткий DC impulse проверяет реальный звук: ожидаем brief beep у active buzzer, у passive может быть только click — тогда потребуется отдельный тест с waveform. Не утверждаем, что тип установлен до ответа пользователя. В установленном Arduino AVR core `noTone()` оставляет pin LOW, поэтому нельзя использовать его как выключение VMA209 без возврата D3 в HIGH; текущий DC test вообще не использует tone/noTone.

Build/upload в 16:37 UTC — PASS: Flash 4288 / 32256 bytes, static RAM 258 / 2048 bytes; avrdude подтвердил запись 4288 bytes. После открытия capture пришёл `DIAG|SHIELDDECK|BUZZER|4`, heartbeat и полный LED sequence. В первые 6 s max scan gap составил 2332 us; buzzer автоматически не запускался. Проверка его физического звука пока ожидается.

Следующий тест: одно нажатие S2. В Serial ожидаются `BTN|2|PRESS`, `DIAG|BUZZER|LOW` и `DIAG|BUZZER|HIGH|40` (допустима небольшая погрешность loop). Пользователь должен услышать короткий сигнал с последующей тишиной либо сообщить «щелчок»/«тишина»/другое поведение. Не помечаем buzzer исправным по одной компиляции или software log.

## Buzzer подтверждён; первая вертикаль в разработке

Пользователь ответил «есть» на вопрос о коротком писке с последующей тишиной. Serial в 17:35 UTC содержит несколько S2 событий; каждый сопутствующий `DIAG|BUZZER|LOW` завершён `DIAG|BUZZER|HIGH|40`. Число физических нажатий этой серии не устанавливалось, поэтому по ней не делаем вывод о debounce. Короткий DC pulse достаточен для слышимого feedback на данном экземпляре.

Рабочая firmware protocol v1 теперь находится в `src/main.cpp`/AppController. Hardware smoke сохранён как `src/smoke_main.cpp`, environment `smoke`; для его повторной загрузки старые команды `pio run -e uno` из разделов выше нужно заменить на `-e smoke`.

Host integration test `test/firmware_host.cpp` компилирует настоящие managers/controller с fake Arduino IO: PASS для handshake, release-only events, CONFIG BUSY, session/sequence matching, duplicate/late RESULT, timeout, malformed/oversize frames, reconnect и momentary LEDs. Это software проверка, не замена физического end-to-end теста.

## Protocol v1 загружен, Mac bridge подключён — 17:58 UTC

- Firmware `env:uno`: build/upload PASS, Flash 9112 / 32256 bytes, static RAM 746 / 2048 bytes; 9112 bytes проверены avrdude.
- `env:smoke` также собирается; предыдущие hardware diagnostics доступны отдельно.
- `tools/check_hardware_protocol.py`: PASS на физической плате — handshake, повтор CONFIG, fragmented CRLF, неправильные/слишком длинные строки, прекращение heartbeat и новая session без reboot.
- Test harness учитывает bootloader reset: после открытия ждёт 2 s и очищает старый USB input перед проверкой HELLO. Первоначальные прогоны показали startup BUSY и stale pre-reset HELLO; это исправлено в harness. Реальный bridge обрабатывает каждый новый HELLO и повторяет CONFIG при BUSY.
- 8 Python protocol tests PASS; native helper скомпилирован. Начальный preflight возвратил ACCESSIBILITY_REQUIRED; после системного запроса текущий preflight возвращает ACCESSIBILITY_OK. Key events при проверке разрешения не отправлялись.
- Live bridge log `/tmp/shielddeck-bridge-2026-10-01.log`: HELLO → WELCOME → READY → PING/PONG → CONFIG → startup BUSY → CONFIG retry → CONFIGURED → CONNECTED. Последующий heartbeat продолжается.
- Active session на момент проверки `082E642A`, slot 1, sound off, все LED modes PULSE. S1 назначена на Cmd+Tab, S2/S3 возвращают ERR 3.

Bridge оставлен работающим, Serial Monitor/capture остановлен. Логи остаются вне Git. S1 выполняет action на отпускании; long/profile механика на этой стадии не добавлена. Смена foreground app, отсутствие двойного запуска и реальный LED/display feedback ещё требуют одного контролируемого физического теста пользователем. Milestone пока **не помечен выполненным**.

Команды software проверки:

```sh
clang++ -std=c++11 -Wall -Wextra -Werror -fsanitize=address,undefined -Itest/fakes -Iinclude test/firmware_host.cpp src/AppController.cpp src/ButtonManager.cpp src/BuzzerManager.cpp src/DisplayManager.cpp src/LedManager.cpp src/SerialManager.cpp -o /tmp/shielddeck-firmware-test
/tmp/shielddeck-firmware-test
~/.platformio/penv/bin/python -m unittest discover -s companion -p 'test_*.py' -v
```

Реальный transport test выполняется только при остановленном bridge (он открывает/reset-ит порт, выполняет handshake и проверяет отключение heartbeat):

```sh
~/.platformio/penv/bin/python tools/check_hardware_protocol.py /dev/cu.usbmodem11101
```

## Первый end-to-end milestone подтверждён — 18:00 UTC

На просьбу нажать S1 и проверить переключение приложения с кратким LED1 feedback пользователь ответил «получилось». В live session `082E642A` подтверждена вся цепочка физическая кнопка → Serial → native helper → результат Mac → feedback.

Пример завершённого события:

```text
18:00:35.093 RX BTN|082E642A|1|31|1|SHORT
18:00:35.094 TX ACK|082E642A|1|31
18:00:35.269 SENT_CMD_TAB
18:00:35.273 TX RESULT|082E642A|1|31|OK|0
```

Во время пользовательских проб до подтверждения зарегистрирован 31 event: 17 S1, 10 S2, 4 S3. Повторных sid/rev/seq нет. Для S1: 16 отправок Cmd+Tab с OK и один ERR 4 (busy); S2/S3 ещё не назначены и получили 14 ERR 3. Пользователь пробовал несколько кнопок, поэтому этот журнал не является контролируемым тестом «31 физических нажатие». Одиночные button smoke tests и отсутствие повторного запуска одного event ID проверены отдельно.

Первый рабочий vertical slice принят по пользовательскому подтверждению. Полный endurance, 100 нажатий и все критерии ShieldDeck v1 ещё не выполнены. Следующий разрешённый этап — назначить S2 безопасное Open Application и S3 screenshot shortcut, с отдельной физической проверкой каждой кнопки.

## Добавлены назначения S2/S3

S2 запускает `/usr/bin/open -b com.apple.calculator`; bundle ID проверен в установленном `/System/Applications/Calculator.app`. S3 отправляет Cmd+Shift+4 через native helper. OK для S3 означает отправку shortcut, а не создание screenshot; пользователь может отменить selection через Escape.

Swift helper пересобран, preflight ACCESSIBILITY_OK. 13 software tests PASS, включая маршрутизацию трёх кнопок и ошибки запуска. В tests процессы замоканы — реальные клавиши/приложения не запускаются. Firmware не изменена: используются прежние generic button events и feedback. Поэтому повторная прошивка не требуется.

Bridge перезапущен с новыми назначениями; log: `/tmp/shielddeck-bridge-three-buttons-2026-10-01.log`. Физические проверки S2/S3 пока ожидаются. Следующее действие пользователя — одно нажатие S2 с подтверждением появления Калькулятора и краткого feedback LED2.

## S2 → Калькулятор подтверждён — 19:12 UTC

На просьбу нажать S2 и проверить появление Калькулятора и краткий LED2 feedback пользователь ответил «работает». В journal текущей session `795987E2` найдено ровно одно соответствующее событие и успешный ответ:

```text
19:12:48.503 RX BTN|795987E2|1|1|2|SHORT
19:12:48.503 TX ACK|795987E2|1|1
19:12:48.608 OPEN_CALCULATOR_REQUESTED
19:12:48.609 TX RESULT|795987E2|1|1|OK|0
```

За последующие 14 s повторного BTN с тем же ID нет. Launch Services вернул успех; пользователь подтвердил физический результат. Следующее действие — один раз нажать S3 и проверить появление macOS screenshot selection (Cmd+Shift+4) и краткий LED3 feedback. Необязательно сохранять screenshot: selection можно отменить Escape.
