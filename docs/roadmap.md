# Roadmap и учебный порядок

Текущий результат — документация и исходная заготовка PlatformIO. Ни один hardware test ещё не выполнен. Firmware не менялась, устройство не прошивалось.

## Правило обучения

Один маленький embedded шаг за раз: объяснение → задание → самостоятельная попытка пользователя → проверка конкретного кода → исправление с объяснением. Полное решение показывается только после попытки. Разработка Mac companion может идти быстрее, когда маленький firmware transport уже проверен. Следующая фаза начинается после проверки exit criteria предыдущей.

## Phase 0 — Hardware validation

Подтвердить модель платы, USB bridge и ориентацию shield; записать в hardware checklist. По отдельности проверить A1, A2, A3; затем D10–D13 с фактическим порядком и полярностью; затем buzzer с коротким импульсом и гарантированным выключением; затем display по последовательности 74HC595 → shiftOut → segment map → digit selection → multiplexing. Не подключать и не снимать shield под питанием.

Exit: для каждого компонента записаны ожидаемое/фактическое поведение и pins; все четыре digits и сегменты читаемы. Нельзя помечать компонент проверенным по одной успешной компиляции. Buzzer до отдельного урока не включаем.

## Phase 1 — Button events

Одна кнопка и raw state → debounce → все три кнопки → short/long и подавление short после hold. Сначала читаем события через Serial Monitor. Это диагностическая firmware без запуска Mac actions.

Exit: 100 нажатий каждой кнопки — 100 short events; удержание button 3 — один long и ноль short; отпускание и bounce не создают повторов. После boot/reconnect удерживаемая кнопка не запускает действие. Сначала цифры подсчёта фиксируются вручную.

## Phase 2 — Serial session и feedback

Постепенно реализовать ограниченный parser, HELLO/config, event IDs, ACK/RESULT и heartbeat; затем LED/display overlays и optional sound. Использовать Mac test harness без выполнения команд. Не добавлять сразу весь protocol parser в первое упражнение.

Exit: основные случаи из protocol checklist проходят; offline заметен не позднее 3 s; повреждённый ввод не зависает и не останавливает display. Нажатие даёт local feedback не позднее 50 ms после стабильного входа. Сканирование не пропускает слоты при обычном Serial traffic; отсутствие заметного flicker проверяется глазами, а интервалы — измерением loop timing. Logic analyzer не обязателен.

## Phase 3 — Mac prototype

Минимальный Serial client с тремя безопасными hardcoded mappings: Cmd+Tab, Open Application, Cmd+Shift+4. Проверить Accessibility и отказ в разрешении; корректный release modifiers. Play/Pause вводим позже как app-specific mapping после выбора приложения, а не притворяемся готовым global media API.

Exit: команды запускаются только от валидных events текущей session; 20 reconnect cycles не выполняют старых нажатий. Timeout и busy дают понятную ошибку, UI/heartbeat не блокируются. Успех «shortcut sent» не объявляется подтверждением конечного результата.

## Phase 4 — Native companion и configurable actions

Menu bar status, Configure, шесть action types, field validation, Test, permissions, sound setting, activity log; локальное atomic сохранение. Сначала интегрировать transport prototype, затем handlers по одному.

Exit: переназначение любой кнопки без перепрошивки; настройки переживают restart; corrupted config не запускает scripts и восстанавливается из backup либо предлагает reset. Пробелы и специальные символы в paths/Shortcut names не превращаются в shell code. Пустой/несуществующий target и отказ permissions корректно отображаются.

## Phase 5 — Profiles

CRUD profiles, selection в menu bar, long button 3, согласование config revision и очистка старого state. Safe defaults: General активен, неподтверждённые commands в других templates выключены.

Exit: rename/duplicate/delete работают, display slot уникален; последний profile нельзя удалить. Переход не запускает primary action button 3. Event или state старой revision не воздействует на новые bindings. Hold при единственном profile ничего не ломает.

## Phase 6 — State synchronization

Сначала fake StateProvider только в test harness для проверки TTL/UNKNOWN. Затем один реальный adapter с доказуемым source, например mute конкретного input device при наличии writable Core Audio property. Это дополнительный этап, который не блокирует выпуск честного stateless v1.

Exit для любого объявленного supported adapter: внешнее изменение отражено за ≤2 s; при потере наблюдения UNKNOWN за ≤5 s. Смена input device, закрытие target app, отказ permissions и sleep не оставляют старое уверенное ON/OFF. Camera adapter не выпускается только на основании keyboard shortcut или device busy flag.

## Phase 7 — Polish и enclosure

Проверить legibility, latency, ночной режим без buzzer, reconnect после sleep/wake и endurance. После измерений платы — removable labels и 3D-printed enclosure, сохраняющий USB доступ. Будущие OLED/encoder/layers — отдельный scope.

## Definition of Done: ShieldDeck v1

- [ ] Плата и VMA209 проверены на hardware; pin/polarity checklist заполнен.
- [ ] Firmware собирается в PlatformIO для подтверждённой платы; flash/RAM budget записан, остаётся запас для stack, нет heap роста на runtime.
- [ ] 3 buttons: debounce, один short на отпускание, один long button 3; нет accidental double execution.
- [ ] Display, LEDs и отключаемый buzzer работают одновременно без blocking delays и заметного flicker.
- [ ] Шесть action types реализованы и проверены хотя бы на одном действии каждого типа на Mac пользователя.
- [ ] Configure меняет назначения без прошивки; есть рабочие profiles и сохранение после restart.
- [ ] Permissions отказаны/отозваны — ошибка объясняется, другие actions продолжают работать.
- [ ] Shell/AppleScript исполняются только из пользовательской конфигурации, без auto-download/root; перед Test виден точный текст.
- [ ] Не выполняются offline, duplicate или stale events; malformed Serial input не приводит к action.
- [ ] 20 unplug/reconnect cycles и sleep/wake не требуют перезапуска приложения; port корректно освобождается для прошивки.
- [ ] Action timeout, busy и crash/disconnect отображаются честно; никаких автоматических retries с возможными side effects.
- [ ] Stateful LEDs либо подкреплены наблюдением с TTL, либо отсутствуют; global camera/mic switch не рекламируется.
- [ ] 2 часа обычной работы без зависаний и мерцания; результаты записаны, а не предположены.
- [ ] README описывает install/build/upload, permissions, назначение LEDs/error codes и known limitations фактической реализации.
- [ ] Source и документация закоммичены; GitHub соответствует локальному commit; личные configs/секреты/build artifacts не включены.

## Первое маленькое задание — пока без кода

Посмотри на свою Arduino и запиши её модель/маркировку, а также название USB chip, если надпись доступна без разборки. Затем ответь:

1. К какому pin подключена первая кнопка VMA209?
2. При active-low какой уровень ожидается при нажатии: HIGH или LOW?
3. Почему нельзя считать `board = uno` доказательством модели подключённой платы?

После этого начнём только чтение одной кнопки. Ты напишешь небольшой код, я проверю его; готовая firmware целиком на этом шаге не нужна.
