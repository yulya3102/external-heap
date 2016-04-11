Простая реализация при добавлении и удалении узла каждый раз загружает
структуру кучи с диска и обновляет ее, используя `std::push_heap` и
`std::pop_heap`, что дает $O(n log n)$ обращений к диску.

Даже при малом размере кэша (до 3 узлов) и малом размере узла B-дерева (от
2 до 5 значений) простая реализация на тесте с 1000 элементами работает
ощутимо медленнее из-за большoго количества обращений к диску.


    [==========] Running 1 test from 1 test case.
    [----------] Global test environment set-up.
    [----------] 1 test from comparsion
    [ RUN      ] comparsion.all
    Size: 1000 elements
    Filling buffer tree heap: 503 ms
    Filling simple heap: 721 ms
    Fetch elements from buffer tree heap: 486 ms
    Fetch elements from simple heap: 999 ms
    [       OK ] comparsion.all (2741 ms)
    [----------] 1 test from comparsion (2741 ms total)

    [----------] Global test environment tear-down
    [==========] 1 test from 1 test case ran. (2741 ms total)
    [  PASSED  ] 1 test.
