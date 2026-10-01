# ShieldDeck

Программируемый macro pad для Mac на Arduino Uno + Velleman VMA209.
Три кнопки запускают назначенные в macOS действия, четыре LEDs и display дают обратную связь.
Переназначение команд и profiles не требует перепрошивки Arduino.

**Статус: проектирование, до Phase 0. Рабочей firmware и companion app пока нет.**
`src/main.cpp` — исходный шаблон PlatformIO; он не реализует ShieldDeck.

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

Разрабатываем embedded часть маленькими шагами: задание → попытка пользователя → проверка → объяснение исправления. Готовую firmware заранее не выдаём. Объяснения на русском, названия API и programming terms — на английском.

Сейчас ничего прошивать не нужно. Следующий шаг — первое задание в roadmap.
Сгенерированные файлы, локальные настройки с командами и секреты не входят в Git.
