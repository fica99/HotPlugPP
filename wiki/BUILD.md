# Сборка HotPlugPP

Подробные инструкции по сборке для всех платформ.

## Требования

### Обязательные
- CMake 3.15 или выше
- C++17-совместимый компилятор:
  - GCC 7+ (Linux)
  - Clang 5+ (macOS/Linux)
  - MSVC 2017+ (Windows)
  - MinGW-w64 (Windows)

### Опциональные
- Git (для клонирования репозитория)
- IDE с поддержкой CMake (CLion, Visual Studio, VS Code)

## Сборка на Linux/macOS

```bash
# Клонирование репозитория
git clone https://github.com/fica99/HotPlugPP.git
cd HotPlugPP

# Создание директории сборки
mkdir build
cd build

# Конфигурация с CMake
cmake ..

# Сборка
cmake --build .

# Или используйте make напрямую
make -j$(nproc)

# Запуск примера
./bin/host_app ./bin/libsample_plugin.so
```

## Сборка на Windows

### Использование Visual Studio

```cmd
# Откройте Developer Command Prompt for VS

# Клонирование репозитория
git clone https://github.com/fica99/HotPlugPP.git
cd HotPlugPP

# Создание директории сборки
mkdir build
cd build

# Конфигурация для Visual Studio (пример для VS 2022)
cmake .. -G "Visual Studio 17 2022"

# Сборка
cmake --build . --config Release

# Запуск примера
.\bin\Release\host_app.exe .\bin\Release\sample_plugin.dll
```

### Использование MinGW

```cmd
# Клонирование репозитория
git clone https://github.com/fica99/HotPlugPP.git
cd HotPlugPP

# Создание директории сборки
mkdir build
cd build

# Конфигурация для MinGW
cmake .. -G "MinGW Makefiles"

# Сборка
cmake --build .

# Запуск примера
.\bin\host_app.exe .\bin\libsample_plugin.dll
```

## Опции сборки

### Debug сборка

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .
```

### Release сборка

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

### Указание компилятора

```bash
# Использовать Clang
cmake .. -DCMAKE_CXX_COMPILER=clang++

# Использовать конкретную версию GCC
cmake .. -DCMAKE_CXX_COMPILER=g++-11
```

## Структура выходных файлов

После сборки структура директорий будет следующей:

```
build/
├── bin/
│   ├── host_app (или host_app.exe на Windows)
│   └── libsample_plugin.so (или .dll/.dylib)
└── lib/
    ├── libhotplugpp.a
    └── libsample_plugin.so (или .dll/.dylib)
```

## Интеграция в ваш проект

### Вариант 1: Добавление как поддиректории

```cmake
# В вашем CMakeLists.txt
add_subdirectory(external/HotPlugPP)
target_link_libraries(your_target PRIVATE hotplugpp)
```

### Вариант 2: Установка и использование

```bash
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build .
sudo cmake --install .
```

После установки HotPlugPP добавьте следующее в `CMakeLists.txt` вашего проекта:

```cmake
find_package(HotPlugPP REQUIRED)
target_link_libraries(your_target PRIVATE hotplugpp)
```

### Вариант 3: Header-only интеграция

Скопируйте директорию `include/` в ваш проект и скомпилируйте `src/plugin_loader.cpp`.

## Устранение неполадок

### Linux: "cannot open shared object file"

Установите `LD_LIBRARY_PATH`:
```bash
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./lib
./bin/host_app ./lib/libsample_plugin.so
```

### Windows: "The specified module could not be found"

Убедитесь, что все DLL находятся в той же директории, что и исполняемый файл, или в PATH.

### macOS: "dyld: Library not loaded"

Установите путь к библиотекам времени выполнения:
```bash
export DYLD_LIBRARY_PATH=$DYLD_LIBRARY_PATH:./lib
./bin/host_app ./lib/libsample_plugin.dylib
```

### Ошибка CMake: "Could not find a configuration file"

Проверьте версию CMake:
```bash
cmake --version  # Должна быть 3.15+
```

## Следующие шаги

- См. [[Туториал|TUTORIAL]] для создания вашего первого плагина
- См. [[API Reference|API]] для полной документации API
- Ознакомьтесь с примерами в директории `examples/`
