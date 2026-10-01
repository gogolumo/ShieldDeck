# ShieldDeck

Macro pad для Mac на Arduino Uno и Velleman VMA209. Плата передаёт нажатия по USB Serial, а запущенный на Mac bridge выполняет команды и возвращает результат для LED и дисплея. Текущие назначения жёстко заданы в bridge; интерфейс переназначения и profiles ещё не реализованы.

| Кнопка | Текущее действие |
| --- | --- |
| S1 | Переключить микрофон Discord (mute/unmute) |
| S2 | Переключить Deafen в Discord (звук Discord и микрофон) |
| S3 | Нажать кнопку включения/выключения камеры в открытом звонке Discord |

Нужен установленный и запущенный **Discord для macOS**. S1 и S2 отправляют сочетания ⌘⇧M и ⌘⇧D; S3 ищет подписанную кнопку камеры через Accessibility. Если Discord не запущен или кнопка камеры недоступна, устройство показывает `E003`. S3 управляет только видео Discord в звонке, а не доступом к камере во всей macOS. Если Discord показывает предпросмотр видео, включение камеры может потребовать подтверждения в приложении.

**Статус проверки:** hardware, Serial и прежние демонстрационные действия подтверждены пользователем. Для текущей Discord-схемы журнал bridge за 19:53–19:55 UTC содержит 64 события от кнопок, 63 успешных результата отправки команды/нажатия элемента и один `E003` для недоступной камеры. Подтверждения пользователем фактического mute, Deafen и передачи видео пока нет; `OK` не означает, что состояние Discord проверено. Подробнее — в [журнале проверок](docs/hardware-validation.md).

## Запуск на текущем Mac

1. Подключить Arduino с загруженной `env:uno` firmware, закрыть Serial Monitor и запустить Discord.
2. Из корня репозитория собрать helper и проверить Accessibility:

   ```sh
   mkdir -p companion/.build
   xcrun swiftc -O companion/KeySender.swift -o companion/.build/ShieldDeckKeys
   companion/.build/ShieldDeckKeys --check
   ```

3. Запустить bridge (путь USB-порта может отличаться):

   ```sh
   ~/.platformio/penv/bin/python companion/bridge.py --port /dev/cu.usbmodem11101
   ```

Если `--check` сообщает об отказе, включить Accessibility для запускающего приложения в macOS System Settings → Privacy & Security → Accessibility. Подробности сборки, проверки и ограничений — в [инструкции companion](companion/README.md). Для первой прошивки или повторной загрузки firmware: `~/.platformio/penv/bin/pio run -e uno -t upload`; на время upload bridge нужно остановить клавишами Ctrl+C.

При соединении дисплей показывает `P001`, LED4 горит. Команда запускается при отпускании кнопки; LED1–3 дают короткий отклик и гаснут. Они **не показывают текущее состояние** микрофона, звука или камеры. Без bridge дисплей показывает `----`, LED4 мигает. Ошибки: `E001` timeout, `E002` нет разрешения, `E003` действие недоступно, `E004` занято, `E005` ошибка исполнения, `E006` несовпадение конфигурации.

## Проект и документация

| Путь | Назначение |
| --- | --- |
| `platformio.ini`, `src/`, `include/`, `test/` | Firmware: `env:uno` — рабочий Serial protocol; `env:smoke` — hardware diagnostics |
| `companion/` | Python bridge, Swift helper и host tests |
| `tools/` | Serial capture и проверка protocol на плате |
| `docs/` | [Архитектура](docs/architecture.md), [протокол](docs/protocol.md), [план](docs/roadmap.md), [журнал проверок](docs/hardware-validation.md) |

Следующий шаг — подтвердить глазами переключение всех трёх функций в Discord, затем добавить настраиваемые назначения и profiles. Личные настройки, секреты, кэш и файлы сборки не входят в Git.
