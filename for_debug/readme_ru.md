# Объекты simstr в отладчиках
[On English|По-английски](readme.md)

Для более удобного отображения объектов simstr в отладчиках msvc и gdb подготовлено два файла:
simstr.natvis - для использования в отладчике MSVC, и simstr_pretty_print.py для работы с gdb.

# Объекты simstr в отладчиках
Если вы работаете в MS Visual Studio simstr.natvis автоматически добавляется в pdb файл,
и обеспечивает удобный просмотр строковых объектов simstr везде, где используется эта библиотека.

# Объекты simstr в Visual Studio Code
## При работе в gdb
В конфигурации отладчика необходимо добавить следующие строки:

```
    "setupCommands": [
        {
            "description": "Enable pretty-printing for gdb",
            "text": "-enable-pretty-printing",
            "ignoreFailures": true
        },
        {
            "description": "Enable pretty-printing for simstr",
            "text": "source ${workspaceFolder}/for_debug/simstr_pretty_print.py",
            "ignoreFailures": true
        }
    ]
```

После этого в отладчике будут удобно отображаться объекты simstr.
Скрипт инспектирует переменные этих типов и выдает для них текстовое описание, отображающее их
содержимое в удобном виде. В первой строке отображается основная информация, видимая в окне
инспектирования переменных. При наведении указателя мыши на переменную в окне исходного кода
или на значении в окне инспектирования переменных во всплывающем тултипе будет показана
остальная информация.

Конфигурации отладчика располагаются в файле `.vscode/launch.json`.
Возможно, у вас также установлено расширение `CMake Tools`, которое позволяет выбирать целевой проект для запуска.
В этом случае настройки для подключения скрипта прописываются в `.vscode/settings.json`:

```
    "cmake.debugConfig": {
        "MIMode": "gdb",
        "environment": [],
        "setupCommands": [
            {
                "description": "Enable pretty-printing for gdb",
                "text": "-enable-pretty-printing",
                "ignoreFailures": true
            },
            {
                "text": "source ${workspaceFolder}/for_debug/simstr_pretty_print.py",
                "description": "pretty print simstr",
                "ignoreFailures": true
            }
        ]
    }
```

Если же в Visual Studio Code вы работаете с отладчиком MSVC, то есть в launch.json `"type": "cppvsdbg"` то настройка другая:
```
    "configurations": [
        {
            .....
            "type": "cppvsdbg",
            ....
            "visualizerFile": "${workspaceFolder}/for_debug/simstr.natvis"
        },
        ...
```
