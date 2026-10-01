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
| Полярность LEDs на реальной плате | Ещё не подтверждена физически |
| Display 1234, порядок digits, flicker | Ещё не проверены |
| Buzzer короткий beep | Ещё не проверен |
| End-to-end Cmd+Tab | Ещё не реализован и не проверен |

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
