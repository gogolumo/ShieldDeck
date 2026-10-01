# ShieldDeck

Программируемый macro pad для Mac на Arduino Uno + Velleman VMA209.
Три кнопки запускают назначенные в macOS действия, четыре LEDs и display дают обратную связь.
Переназначение команд и profiles не требует перепрошивки Arduino.

**Статус: три действия S1/S2/S3 и физический feedback подтверждены пользователем.**
Hardware smoke пройден: три кнопки, active-low LEDs, display `1234` и короткий buzzer. S1 — Cmd+Tab, S2 — Калькулятор, S3 — выбор области screenshot (Cmd+Shift+4). Клавиши отправляются только при наличии Accessibility. [Сборка и запуск bridge](companion/README.md), [фактические результаты](docs/hardware-validation.md).

При соединении display показывает `P001`, LED4 горит. S1 запускает команду при отпускании; LED1 даёт краткий feedback и гаснет. Без Mac — `----` и мигание LED4. `E001` = timeout, `E002` = permission denied, `E003` = действие не назначено/недоступно, `E004` = busy, `E005` = execution failed, `E006` = config mismatch. OK означает отправку клавиш; видимое переключение подтверждает пользователь.

## Документация

- [Архитектура и UX](docs/architecture.md): решения по всем 15 пунктам задания, ограничения macOS, hardware и state synchronization.
- [Serial protocol v1 — draft](docs/protocol.md): сообщения, reconnect, ошибки и защита от повторного выполнения.
- [Roadmap и Definition of Done](docs/roadmap.md): этапы, критерии проверки и первое учебное задание.

## Структура

| Путь | Назначение |
| --- | --- |
| `platformio.ini` | Существующая конфигурация `atmelavr / uno / arduino` |
| `src/`, `include/`, `test/` | Firmware, drivers, host tests; `env:uno` — protocol v1, `env:smoke` — hardware diagnostics |
| `docs/` | Проектирование и учебный план |
| `companion/` | Python Serial bridge, native Swift keyboard helper и protocol tests |
| `tools/` | Serial capture и автоматическая проверка protocol на реальной плате |

Текущий порядок по обновлённому заданию: небольшой firmware шаг → build → upload → Serial → физический тест → следующий шаг. Код и прошивку выполняет агент; пользователь помогает там, где нужно нажать кнопку, увидеть LED/display или услышать beep. Объяснения на русском, названия API и programming terms — на английском.

Следующий этап — конфигурируемые назначения и profiles после обсуждения scope. Полноценный menu bar app пока не реализован. Для остановки текущего CLI bridge — Ctrl+C в его терминале; Arduino затем покажет `----`.
Сгенерированные файлы, локальные настройки с командами и секреты не входят в Git.
