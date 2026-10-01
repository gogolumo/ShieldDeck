# ShieldDeck for macOS

Здесь будет native menu bar приложение. На текущем этапе согласована только архитектура.

Планируемый стек: Swift + SwiftUI для Configure, AppKit для menu bar, POSIX Serial transport. Минимальная целевая версия — macOS 13; первоначальная проверка на текущем Mac, затем отдельно совместимость с минимальной версией.

Описание компонентов, action types и permissions находится в [архитектуре](../docs/architecture.md). Контракт с Arduino — в [Serial protocol](../docs/protocol.md).

Личная конфигурация будет храниться в Application Support, вне репозитория. Примеры profiles допускаются только с безопасными демонстрационными значениями.
