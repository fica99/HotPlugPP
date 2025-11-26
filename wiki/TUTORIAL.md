# Туториал: Создание первого плагина

Это руководство проведёт вас через создание собственного плагина для HotPlugPP с нуля.

## Требования

- HotPlugPP собран и работает (см. [[Сборка|BUILD]])
- Базовые знания C++
- Текстовый редактор или IDE

## Обзор туториала

Мы создадим "Greeter Plugin", который:
1. Приветствует пользователя при загрузке
2. Подсчитывает обновления
3. Прощается при выгрузке

## Шаг 1: Создание исходного файла плагина

Создайте новый файл `examples/greeter_plugin/GreeterPlugin.cpp`:

```cpp
#include "hotplugpp/i_plugin.hpp"
#include <iostream>
#include <string>

class GreeterPlugin : public hotplugpp::IPlugin {
public:
    GreeterPlugin() : m_updateCount(0) {
        std::cout << "[GreeterPlugin] Экземпляр создан" << std::endl;
    }

    ~GreeterPlugin() override {
        std::cout << "[GreeterPlugin] Экземпляр уничтожен" << std::endl;
    }

    bool onLoad() override {
        std::cout << "[GreeterPlugin] Привет! Я загружаюсь." << std::endl;
        m_updateCount = 0;
        return true;
    }

    void onUnload() override {
        std::cout << "[GreeterPlugin] Пока! Я обновился " 
                  << m_updateCount << " раз." << std::endl;
    }

    void onUpdate(float deltaTime) override {
        m_updateCount++;
        
        // Вывод каждые 120 кадров (каждые 2 секунды при 60 FPS)
        if (m_updateCount % 120 == 0) {
            std::cout << "[GreeterPlugin] Всё ещё здесь! Обновление #" 
                      << m_updateCount << std::endl;
        }
    }

    const char* getName() const override {
        return "GreeterPlugin";
    }

    hotplugpp::Version getVersion() const override {
        return hotplugpp::Version(1, 0, 0);
    }

    const char* getDescription() const override {
        return "Дружелюбный плагин, который приветствует вас";
    }

private:
    int m_updateCount;
};

// Экспорт плагина - это обязательно!
HOTPLUGPP_CREATE_PLUGIN(GreeterPlugin)
```

> **Важно:** Обратите внимание на правильное имя заголовочного файла: `hotplugpp/i_plugin.hpp` (в snake_case).

## Шаг 2: Добавление в систему сборки

Отредактируйте `examples/CMakeLists.txt` и добавьте:

```cmake
# Greeter Plugin
add_library(greeter_plugin SHARED
    greeter_plugin/GreeterPlugin.cpp
)

target_include_directories(greeter_plugin PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

set_target_properties(greeter_plugin PROPERTIES
    PREFIX ""
    OUTPUT_NAME "greeter_plugin"
)

if(WIN32)
    set_target_properties(greeter_plugin PROPERTIES SUFFIX ".dll")
else()
    set_target_properties(greeter_plugin PROPERTIES PREFIX "lib" SUFFIX ".so")
endif()

add_custom_command(TARGET greeter_plugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:greeter_plugin>
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<TARGET_FILE_NAME:greeter_plugin>
    COMMENT "Копирование greeter_plugin в директорию bin"
)
```

## Шаг 3: Сборка плагина

```bash
cd build
cmake --build . --target greeter_plugin
```

Вы должны увидеть вывод:
```
[100%] Built target greeter_plugin
Copying greeter_plugin to bin directory
```

## Шаг 4: Запуск плагина

```bash
cd bin
./host_app ./libgreeter_plugin.so
```

Ожидаемый вывод:
```
=== HotPlugPP Example Host Application ===

Loading plugin from: ./libgreeter_plugin.so
[GreeterPlugin] Экземпляр создан
[GreeterPlugin] Привет! Я загружаюсь.
Plugin loaded successfully: GreeterPlugin v1.0.0

Plugin loaded successfully!
  Name: GreeterPlugin
  Version: 1.0.0
  Description: Дружелюбный плагин, который приветствует вас

Starting update loop (hot-reload monitoring enabled)...
You can modify and recompile the plugin to see hot-reload in action!

[GreeterPlugin] Всё ещё здесь! Обновление #120
[GreeterPlugin] Всё ещё здесь! Обновление #240
```

## Шаг 5: Тестирование горячей перезагрузки

Оставьте host_app работающим, затем в другом терминале:

1. **Отредактируйте плагин** — измените сообщение приветствия:
```cpp
bool onLoad() override {
    std::cout << "[GreeterPlugin] Привет снова! Я был перезагружен!" << std::endl;
    m_updateCount = 0;
    return true;
}
```

2. **Пересоберите только плагин**:
```bash
cd build
cmake --build . --target greeter_plugin
```

3. **Наблюдайте за терминалом host_app** — вы должны увидеть:
```
[GreeterPlugin] Пока! Я обновился XXX раз.
[GreeterPlugin] Экземпляр уничтожен
Plugin file modified, reloading...
[GreeterPlugin] Экземпляр создан
[GreeterPlugin] Привет снова! Я был перезагружен!
Plugin loaded successfully: GreeterPlugin v1.0.0

*** Plugin has been reloaded! ***
```

Поздравляем! Вы создали свой первый плагин с горячей перезагрузкой!

## Продвинутый пример: Управление состоянием

Давайте улучшим плагин, добавив отслеживание состояния:

```cpp
class GreeterPlugin : public hotplugpp::IPlugin {
public:
    GreeterPlugin() : m_updateCount(0), m_totalTime(0.0f) {}

    bool onLoad() override {
        std::cout << "[GreeterPlugin] Привет! Начинаем с чистого листа." << std::endl;
        // Состояние сбрасывается при перезагрузке
        m_updateCount = 0;
        m_totalTime = 0.0f;
        return true;
    }

    void onUpdate(float deltaTime) override {
        m_updateCount++;
        m_totalTime += deltaTime;
        
        if (m_updateCount % 120 == 0) {
            std::cout << "[GreeterPlugin] Обновление #" << m_updateCount 
                      << " (работает " << m_totalTime << " сек)" << std::endl;
        }
    }

    void onUnload() override {
        std::cout << "[GreeterPlugin] Пока! Статистика:" << std::endl;
        std::cout << "  Обновлений: " << m_updateCount << std::endl;
        std::cout << "  Время работы: " << m_totalTime << " сек" << std::endl;
    }

    // ... остальная реализация

private:
    int m_updateCount;
    float m_totalTime;
};
```

## Лучшие практики

### 1. Инициализация в onLoad(), очистка в onUnload()

```cpp
bool onLoad() override {
    m_texture = loadTexture("texture.png");
    return m_texture != nullptr;  // Вернуть false при ошибке
}

void onUnload() override {
    delete m_texture;
    m_texture = nullptr;
}
```

### 2. Держите onUpdate() быстрым

```cpp
void onUpdate(float deltaTime) override {
    // Хорошо: быстрая операция
    m_position += m_velocity * deltaTime;
    
    // Плохо: не выполняйте тяжёлые операции каждый кадр
}
```

### 3. Обрабатывайте горячую перезагрузку корректно

Состояние **НЕ** сохраняется между перезагрузками.

**Почему?** Во время горячей перезагрузки HotPlugPP:
- Вызывает `onUnload()` на старом экземпляре плагина
- Уничтожает старый экземпляр
- Создаёт новый экземпляр и вызывает `onLoad()`

Это означает, что любое состояние в памяти теряется. Если вам нужно сохранить состояние:
- Сохраняйте в файл в `onUnload()`
- Загружайте из файла в `onLoad()`
- Используйте отдельный менеджер состояния в хост-приложении

### 4. Версионирование плагинов

```cpp
hotplugpp::Version getVersion() const override {
    return hotplugpp::Version(1, 2, 3);  // major.minor.patch
}
```

Увеличивайте:
- **Major**: несовместимые изменения
- **Minor**: новые функции, обратная совместимость
- **Patch**: исправления ошибок

### 5. Обработка ошибок

```cpp
bool onLoad() override {
    if (!initializeSubsystemA()) {
        std::cerr << "[MyPlugin] Не удалось инициализировать подсистему A" << std::endl;
        return false;  // Прервать загрузку
    }
    
    if (!initializeSubsystemB()) {
        std::cerr << "[MyPlugin] Не удалось инициализировать подсистему B" << std::endl;
        cleanupSubsystemA();  // Очистить то, что начали
        return false;
    }
    
    return true;
}
```

## Устранение неполадок

### Плагин не загружается
- Проверьте правильность пути к файлу
- Убедитесь, что файл имеет права на выполнение
- Проверьте наличие всех зависимостей

### "Failed to find plugin factory functions"
Убедитесь, что вы добавили:
```cpp
HOTPLUGPP_CREATE_PLUGIN(YourPluginClassName)
```

### Горячая перезагрузка не работает
- Убедитесь, что файл действительно был пересобран
- Проверьте, что `checkAndReload()` вызывается
- Убедитесь, что файл плагина не заблокирован другим процессом

## Следующие шаги

- Изучите примеры: `sample_plugin` и `math_plugin`
- Прочитайте [[API Reference|API]] для полной документации интерфейсов
- Создайте что-нибудь реальное!

Удачной разработки плагинов! 🔌
