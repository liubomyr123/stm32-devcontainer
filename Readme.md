# RC Car

Проект радіокерованої машинки з камерою.

## Структура

```
rc-car/
├── stm32_project/   # STM32F429I-DISC1 — керування моторами
└── esp32_project/   # ESP32-CAM — WiFi, веб інтерфейс, камера
```

## Проекти

### STM32
Керування 4 DC моторами через TB6612FNG драйвери, FreeRTOS State Machine, UART прийом команд.

Детальніше: [stm32_project/README.md](stm32_project/README.md)

### ESP32
WiFi точка доступу, HTTP веб сервер з кнопками керування, відео стрімінг з камери, UART до STM32.

Детальніше: [esp32_project/README.md](esp32_project/README.md)

## Як відкрити проект

Кожен проект має власний Dev Container. Відкрий потрібну папку в VS Code:

```
File → Open Folder → stm32_project/ або esp32_project/
```

Потім натисни **"Reopen in Container"**.
