# ShieldDeck

Программируемый macro pad для Mac на Arduino Uno + Velleman VMA209.
Три кнопки запускают назначенные в macOS действия, четыре LEDs и display дают обратную связь.
Переназначение команд и profiles не требует перепрошивки Arduino.

**Статус: первый hardware smoke test загружен на Arduino Uno.**
Firmware читает A1–A3 с debounce; все три кнопки прошли одиночные физические тесты. Пользователь подтвердил последовательность и active-low полярность LEDs D10–D13. Загружен display test `1234`; визуальное подтверждение ожидается. S1 повторяет LED sequence. Bridge и Cmd+Tab пока не реализованы. [Фактические результаты и команды проверки](docs/hardware-validation.md).

## Документация

- [Архитектура и UX](docs/architecture.md): решения по всем 15 пунктам задания, ограничения macOS, hardware и state synchronization.
- [Serial protocol v1 — draft](docs/protocol.md): сообщения, reconnect, ошибки и защита от повторного выполнения.
- [Roadmap и Definition of Done](docs/roadmap.md): этапы, критерии проверки и первое учебное задание.

## Структура

| Путь | Назначение |
| --- | --- |
| `platformio.ini` | Существующая конфигурация `atmelavr / uno / arduino` |
| `src/`, `include/`, `lib/`, `test/` | Исходная заготовка PlatformIO для будущей firmware |
| `docs/` | Проектирование и учебный план |
| `companion/` | Место для будущего native macOS приложения |

Текущий порядок по обновлённому заданию: небольшой firmware шаг → build → upload → Serial → физический тест → следующий шаг. Код и прошивку выполняет агент; пользователь помогает там, где нужно нажать кнопку, увидеть LED/display или услышать beep. Объяснения на русском, названия API и programming terms — на английском.

Следующий шаг — визуально проверить `1234` на display. Остальные этапы выполняются последовательно после проверки hardware.
Сгенерированные файлы, локальные настройки с командами и секреты не входят в Git.
