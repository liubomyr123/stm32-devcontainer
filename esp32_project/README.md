# ESP32 Project

ESP32-CAM проект на ESP-IDF — WiFi AP, веб сервер, камера, UART до STM32.

## Початок роботи

### 1. Відкрити в Dev Container

Відкрий папку `esp32_project/` в VS Code і натисни **"Reopen in Container"**.

### 2. Підключення ESP32-CAM

Підключи CH340 адаптер через USB. В PowerShell (адміністратор):

```powershell
usbipd list
usbipd bind --busid <BUSID>
usbipd attach --wsl --busid <BUSID>
```

CH340 має VID:PID `0403:6001`.

Перевір що пристрій з'явився в контейнері:

```bash
ls /dev/ttyUSB0
```

### 3. Збірка

```bash
./build.sh
```

### 4. Прошивка

```bash
./flash.sh
```

### 5. Моніторинг

```bash
./monitor.sh
```

Вийти з монітора: **Ctrl+]**

Логи зберігаються в `logs/session_TIMESTAMP.log`.

### 6. Форматування коду

```bash
./format.sh
```

### 7. Статичний аналіз

```bash
./lint.sh
```