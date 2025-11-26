# HotPlugPP API Reference

Полная документация API для системы плагинов HotPlugPP.

## Содержание

- [Основные интерфейсы](#основные-интерфейсы)
  - [IPlugin](#iplugin)
  - [Version](#version)
- [Загрузчик плагинов](#загрузчик-плагинов)
  - [PluginLoader](#pluginloader)
  - [PluginInfo](#plugininfo)
- [Макросы](#макросы)
- [Поддержка платформ](#поддержка-платформ)

---

## Основные интерфейсы

### IPlugin

Базовый интерфейс, который должны реализовать все плагины.

**Заголовочный файл:** `hotplugpp/i_plugin.hpp`

```cpp
namespace hotplugpp {
    class IPlugin {
    public:
        virtual ~IPlugin() = default;
        virtual bool onLoad() = 0;
        virtual void onUnload() = 0;
        virtual void onUpdate(float deltaTime) = 0;
        virtual const char* getName() const = 0;
        virtual Version getVersion() const = 0;
        virtual const char* getDescription() const = 0;
    };
}
```

#### Методы

##### `bool onLoad()`

Вызывается при загрузке плагина. Используйте для инициализации ресурсов, выделения памяти или настройки состояния.

**Возвращает:** `true` при успешной инициализации, `false` для отмены загрузки.

**Пример:**
```cpp
bool onLoad() override {
    m_texture = loadTexture("plugin_texture.png");
    return m_texture != nullptr;
}
```

---

##### `void onUnload()`

Вызывается перед выгрузкой плагина. Освободите ресурсы здесь.

**Примечание:** Вызывается всегда, даже если `onLoad()` вернул false.

**Пример:**
```cpp
void onUnload() override {
    delete m_texture;
    m_texture = nullptr;
}
```

---

##### `void onUpdate(float deltaTime)`

Вызывается регулярно (обычно каждый кадр). Здесь выполняется основная логика плагина.

**Параметры:**
- `deltaTime`: Время, прошедшее с последнего обновления, в секундах

**Пример:**
```cpp
void onUpdate(float deltaTime) override {
    m_position += m_velocity * deltaTime;
    if (m_position > 100.0f) {
        m_velocity *= -1.0f;
    }
}
```

---

##### `const char* getName() const`

**Возвращает:** Имя плагина как C-строку.

**Пример:**
```cpp
const char* getName() const override {
    return "MyAwesomePlugin";
}
```

---

##### `Version getVersion() const`

**Возвращает:** Версию плагина.

**Пример:**
```cpp
Version getVersion() const override {
    return Version(1, 2, 3); // v1.2.3
}
```

---

##### `const char* getDescription() const`

**Возвращает:** Краткое описание плагина.

**Пример:**
```cpp
const char* getDescription() const override {
    return "Предоставляет продвинутые эффекты частиц";
}
```

---

### Version

Структура версии для семантического версионирования и проверки совместимости.

**Заголовочный файл:** `hotplugpp/i_plugin.hpp`

```cpp
struct Version {
    uint32_t major;
    uint32_t minor;
    uint32_t patch;
    
    Version(uint32_t maj = 1, uint32_t min = 0, uint32_t pat = 0);
    bool isCompatible(const Version& other) const;
    std::string toString() const;
    
    // Операторы сравнения
    bool operator==(const Version& other) const;
    bool operator!=(const Version& other) const;
    bool operator<(const Version& other) const;
    bool operator>(const Version& other) const;
    bool operator<=(const Version& other) const;
    bool operator>=(const Version& other) const;
};
```

#### Конструкторы

##### `Version(uint32_t major = 1, uint32_t minor = 0, uint32_t patch = 0)`

Создаёт объект версии.

**Параметры:**
- `major`: Мажорная версия (несовместимые изменения API)
- `minor`: Минорная версия (обратно совместимые новые функции)
- `patch`: Патч-версия (обратно совместимые исправления ошибок)

---

#### Методы

##### `bool isCompatible(const Version& other) const`

Проверяет совместимость этой версии с другой версией.

**Правила совместимости:**
- Мажорные версии должны совпадать точно
- Минорная версия должна быть >= минорной версии другой
- Патч-версия игнорируется

**Возвращает:** `true` если совместимо, `false` иначе.

**Пример:**
```cpp
Version plugin(1, 5, 0);
Version required(1, 3, 0);

if (plugin.isCompatible(required)) {
    // Плагин версии 1.5.0 совместим с требованием 1.3.0
}
```

---

##### `std::string toString() const`

**Возвращает:** Версию как строку в формате "major.minor.patch".

---

## Загрузчик плагинов

### PluginLoader

Управляет динамической загрузкой, выгрузкой и горячей перезагрузкой плагинов.

**Заголовочный файл:** `hotplugpp/plugin_loader.hpp`

```cpp
class PluginLoader {
public:
    PluginLoader();
    ~PluginLoader();
    
    // Копирование запрещено
    PluginLoader(const PluginLoader&) = delete;
    PluginLoader& operator=(const PluginLoader&) = delete;
    
    bool loadPlugin(const std::string& path);
    void unloadPlugin();
    bool checkAndReload();
    IPlugin* getPlugin() const;
    bool isLoaded() const;
    std::string getPluginPath() const;
    void setReloadCallback(std::function<void()> callback);
};
```

#### Конструкторы

##### `PluginLoader()`

Создаёт новый загрузчик плагинов. Плагины не загружены изначально.

---

#### Методы

##### `bool loadPlugin(const std::string& path)`

Загружает плагин из файла разделяемой библиотеки.

**Параметры:**
- `path`: Путь к библиотеке плагина (.so/.dll/.dylib)

**Возвращает:** `true` при успешной загрузке, `false` иначе.

**Поведение:**
1. Выгружает текущий загруженный плагин
2. Открывает разделяемую библиотеку
3. Находит функции `createPlugin` и `destroyPlugin`
4. Создаёт экземпляр плагина
5. Вызывает `onLoad()`

**Пример:**
```cpp
PluginLoader loader;
if (!loader.loadPlugin("./plugins/my_plugin.so")) {
    std::cerr << "Не удалось загрузить плагин!" << std::endl;
}
```

---

##### `void unloadPlugin()`

Выгружает текущий загруженный плагин.

**Поведение:**
1. Вызывает `onUnload()` на плагине
2. Уничтожает экземпляр плагина
3. Закрывает разделяемую библиотеку

**Пример:**
```cpp
loader.unloadPlugin();
```

---

##### `bool checkAndReload()`

Проверяет, был ли файл плагина изменён, и перезагружает при необходимости.

**Возвращает:** `true` если плагин был перезагружен, `false` иначе.

**Поведение:**
1. Проверяет время модификации файла
2. Если изменено, выгружает старый плагин
3. Загружает новый плагин из того же пути
4. Вызывает callback перезагрузки, если установлен

**Пример:**
```cpp
// В вашем цикле обновления
if (loader.checkAndReload()) {
    std::cout << "Плагин был горячо перезагружен!" << std::endl;
}
```

---

##### `IPlugin* getPlugin() const`

**Возвращает:** Указатель на загруженный экземпляр плагина или `nullptr`, если плагин не загружен.

**Пример:**
```cpp
if (auto* plugin = loader.getPlugin()) {
    plugin->onUpdate(deltaTime);
}
```

---

##### `bool isLoaded() const`

**Возвращает:** `true` если плагин загружен, `false` иначе.

**Пример:**
```cpp
if (loader.isLoaded()) {
    loader.getPlugin()->onUpdate(deltaTime);
}
```

---

##### `std::string getPluginPath() const`

**Возвращает:** Путь текущего загруженного плагина или пустую строку, если не загружен.

---

##### `void setReloadCallback(std::function<void()> callback)`

Устанавливает функцию обратного вызова, которая будет вызвана при горячей перезагрузке плагина.

**Параметры:**
- `callback`: Функция для вызова при перезагрузке

**Пример:**
```cpp
loader.setReloadCallback([]() {
    std::cout << "Плагин перезагружен, переинициализация состояния..." << std::endl;
    // Переинициализируйте состояние, зависящее от плагина
});
```

---

### PluginInfo

Структура, содержащая метаданные и дескриптор плагина.

**Заголовочный файл:** `hotplugpp/plugin_loader.hpp`

```cpp
struct PluginInfo {
    std::string path;
    LibraryHandle handle = nullptr;
    IPlugin* instance = nullptr;
    CreatePluginFunc createFunc = nullptr;
    DestroyPluginFunc destroyFunc = nullptr;
    std::chrono::system_clock::time_point lastModified;
    bool isLoaded = false;
};
```

---

## Макросы

### HOTPLUGPP_CREATE_PLUGIN(PluginClass)

Удобный макрос для создания необходимых фабричных функций плагина.

**Параметры:**
- `PluginClass`: Имя вашего класса плагина

**Генерирует:**
- `createPlugin()`: Фабричная функция, создающая экземпляр
- `destroyPlugin()`: Фабричная функция, уничтожающая экземпляр

**Пример:**
```cpp
class MyPlugin : public hotplugpp::IPlugin {
    // ... реализация ...
};

HOTPLUGPP_CREATE_PLUGIN(MyPlugin)
```

**Эквивалентно:**
```cpp
extern "C" HOTPLUGPP_API hotplugpp::IPlugin* createPlugin() {
    return new MyPlugin();
}

extern "C" HOTPLUGPP_API void destroyPlugin(hotplugpp::IPlugin* plugin) {
    delete plugin;
}
```

---

### HOTPLUGPP_API

Платформенно-зависимый макрос экспорта для символов разделяемой библиотеки.

**Раскрывается в:**
- Windows: `__declspec(dllexport)`
- Unix/Linux: `__attribute__((visibility("default")))`

---

### HOTPLUGPP_PLUGIN_EXPORT

Макрос для объявления функции с `extern "C"` линковкой.

**Раскрывается в:** `extern "C"`

---

## Поддержка платформ

### Платформенно-зависимое поведение

#### Windows (.dll)
- Использует `LoadLibraryA()` и `GetProcAddress()`
- Требует `__declspec(dllexport)` для экспортируемых символов
- Расширение файла: `.dll`

#### Linux (.so)
- Использует `dlopen()` и `dlsym()`
- Требует флаг линковщика `-ldl`
- Расширение файла: `.so`
- Требует позиционно-независимый код (`-fPIC`)

#### macOS (.dylib)
- Использует `dlopen()` и `dlsym()`
- Расширение файла: `.dylib`
- Поведение аналогично Linux

---

## Определения типов

```cpp
// Типы указателей на функции для фабрик плагинов
typedef hotplugpp::IPlugin* (*CreatePluginFunc)();
typedef void (*DestroyPluginFunc)(hotplugpp::IPlugin*);

// Платформенно-зависимый дескриптор библиотеки
#ifdef _WIN32
    typedef HMODULE LibraryHandle;
#else
    typedef void* LibraryHandle;
#endif
```

---

## Обработка ошибок

### Частые ошибки

**Плагин не загружается:**
- Файл не существует или неверный путь
- Отсутствуют зависимости
- Несовместимая архитектура (32-bit vs 64-bit)
- Отсутствуют функции экспорта

**Горячая перезагрузка не работает:**
- Файл заблокирован другим процессом
- Ошибки компиляции в новой версии
- Проблемы с правами доступа к файлу

**Ошибки времени выполнения:**
- Плагин падает в `onUpdate()`
- Утечки памяти в коде плагина
- Исключения, выброшенные из плагина

### Лучшие практики

1. **Всегда проверяйте возвращаемые значения:**
```cpp
if (!loader.loadPlugin(path)) {
    // Обработка ошибки
}
```

2. **Используйте RAII в плагинах:**
```cpp
class MyPlugin : public hotplugpp::IPlugin {
    std::unique_ptr<Resource> m_resource;
    
    bool onLoad() override {
        m_resource = std::make_unique<Resource>();
        return m_resource != nullptr;
    }
};
```

3. **Обрабатывайте горячую перезагрузку корректно:**
```cpp
loader.setReloadCallback([&]() {
    // Сохраните состояние перед перезагрузкой
    // Восстановите состояние после перезагрузки
});
```

---

## Примеры

### Базовый плагин

```cpp
#include "hotplugpp/i_plugin.hpp"
#include <iostream>

class SimplePlugin : public hotplugpp::IPlugin {
public:
    bool onLoad() override {
        std::cout << "SimplePlugin загружен!" << std::endl;
        return true;
    }

    void onUnload() override {
        std::cout << "SimplePlugin выгружается..." << std::endl;
    }

    void onUpdate(float deltaTime) override {
        // Выполняйте работу здесь
    }

    const char* getName() const override { return "SimplePlugin"; }
    hotplugpp::Version getVersion() const override { return {1, 0, 0}; }
    const char* getDescription() const override { return "Простой плагин"; }
};

HOTPLUGPP_CREATE_PLUGIN(SimplePlugin)
```

### Хост-приложение

```cpp
#include "hotplugpp/plugin_loader.hpp"
#include <iostream>
#include <chrono>
#include <thread>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Использование: " << argv[0] << " <plugin.so>" << std::endl;
        return 1;
    }

    hotplugpp::PluginLoader loader;
    
    if (!loader.loadPlugin(argv[1])) {
        return 1;
    }

    // Главный цикл
    const float deltaTime = 1.0f / 60.0f;
    while (true) {
        auto start = std::chrono::steady_clock::now();
        
        // Обновление плагина
        if (auto* plugin = loader.getPlugin()) {
            plugin->onUpdate(deltaTime);
        }
        
        // Проверка горячей перезагрузки
        loader.checkAndReload();
        
        // Сон для поддержания частоты кадров
        auto end = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::this_thread::sleep_for(std::chrono::milliseconds(16) - elapsed);
    }

    return 0;
}
```

---

## Потокобезопасность

⚠️ **Внимание:** Текущая реализация **не является потокобезопасной**.

- Не вызывайте методы загрузчика из нескольких потоков
- Сериализуйте доступ с помощью мьютексов для многопоточных приложений

---

## Советы по производительности

1. **Проверка горячей перезагрузки**: Обычно достаточно раз в секунду
2. **Инициализация плагина**: Держите `onLoad()` быстрым
3. **Производительность обновления**: `onUpdate()` вызывается часто — оптимизируйте его
4. **Память**: Плагины разделяют кучу с хостом — очищайте в `onUnload()`

---

Для дополнительной информации:
- [[Главная|Home]] — Обзор
- [[Сборка|BUILD]] — Инструкции по сборке
- [[Туториал|TUTORIAL]] — Пошаговое руководство
- [Примеры](https://github.com/fica99/HotPlugPP/tree/main/examples) — Рабочие примеры
