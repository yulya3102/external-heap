Heap (data structure) in external memory.

## Краткая теория

Элементы делятся на две категории: "большие" значения и "малые". Малые
значения хранятся в оперативной памяти, большие хранятся во внешней памяти.
Для хранения больших значений используется буферное дерево, для малых —
список. Каждый добавляемый ключ классифицируется как большой или малый, в
зависимости от этого добавляется в нужную структуру. При удалении
минимального элемента берется первое значение из множества "малых".

При переполнении множества "малых" значений наибольшие из них перемещаются в
"большие". При опустошении множества "малых" значений из буферного дерева
извлекается самый левый лист и становится новым множеством "малых"
значений.

Буферное дерево — B-дерево, "кэширующее" операции. Так как в данной
реализации удаление элементов должно сразу возвращать результат, кэшируются
только добавления элементов в дерево. Кэширование реализовано
дополнительным буфером со списком еще не сделанных операций в каждом узле.
Если буфер переполняется, все операции из него выполняются по очереди,
возможно, будущи закэшированными на более глубоких узлах.

## Зависимости

*   `boost` (работа с ФС)
*   `gtest` (тестирование)
*   `protobuf` (сериализация)

## Структура репозитория

*   `storage/`:

    *   реализация LRU cache (`cache`) для автоматической загрузки и
        сохранения узлов буферного дерева;
    *   общий интерфейс (`basic_storage`) независимого хранилища
        сериализованных узлов буферного дерева и две его реализации (`memory` и
        `directory`);

*   `btree/`:

    *   реализация буферного дерева и сериализации узлов;

*   `heap/`:

    *   реализация кучи;

*   `simple/`:

    *   реализация простого неоптимального варианта кучи во внешней памяти
        и тесты для сравнения с основной реализацией;
    *   результаты тестов.
