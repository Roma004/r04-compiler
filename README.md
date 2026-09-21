# Проектирование компилятора

Чтобы собрать проект:
```sh
mkdir build && cd build
cmake ..
make
```

Чтобы запустить тесты:
```sh
make all_tests
# или
make
./all_tests
```

Посмотреть покрытие кода:
```sh
make coverage          # формирует gcovr отчёт
make show-coverage     # формирует отчёт и открывает в браузере чрез xdg-open
```
