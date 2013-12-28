plate_recognition
=================
Plate recognition library

Поддерживает компиляторы
=================
Visual Studio 2012
mingw
g++
Android NDK - пока не тестил

Поддерживает платформы
=================
Windows
Android
iOS - пока не тестил
Mac - пока не тестил
Linux - пока не тестил

Обязательные зависимости
=================
OpenCV

Не обязательные зависимости
=================
Qt

СБОРКА CMake
=================
Скопировать файл FindOpenCV.cmake в папку где лежат все конфигурационные файлы *.cmake, у меня это: C:\Program Files (x86)\CMake 2.8\share\cmake-2.8\Modules
Задать переменной окружения CMAKE_PREFIX_PATH путь %QT%/msvc2012/lib/cmake что бы CMake нашел нужные файлы

СБОРКА qmake
=================
1. В файле opencv.pri задать путь к библиотеке OpenCV.
2. В файле opencv.pri задать версию библиотеки OpenCV.
