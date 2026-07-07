# STM32 Project

STM32F429I-DISC1 проект на HAL + FreeRTOS — керування моторами, UART команди, State Machine.

## Початок роботи

### 1. Відкрити в Dev Container

Відкрий папку `stm32_project/` в VS Code і натисни **"Reopen in Container"**.

### 2. Підключення STM32

Підключи STM32F429I-DISC1 через USB. В PowerShell (адміністратор):

```powershell
usbipd list
usbipd bind --busid <BUSID>
usbipd attach --wsl --busid <BUSID>
```

ST-Link має VID:PID `0483:374b`.

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

Вийти з монітора: **Ctrl+A** потім **Ctrl+X**

Логи зберігаються в `logs/session_TIMESTAMP.log`.

### 6. Форматування коду

```bash
./format.sh
```

### 7. Статичний аналіз

```bash
./lint.sh
```

## CubeMX — генерація коду

1. Отримай шлях до проекту — відкрий WSL термінал і виконай:

```bash
wslpath -w ~/projects/stm32-template
```

Приклад відповіді:
```
\\wsl.localhost\Ubuntu-22.04\home\liubomyr\projects\stm32-template
```

2. Відкрий File Explorer на Windows і перейди за цією адресою в папку `stm32_project/stm32/`
3. Відкрий `stm32.ioc` у CubeMX
4. В секції **Project Manager**:
   - не міняй шляху до проекту
   - Toolchain/IDE має бути `CMake`
5. Натисни **Generate Code**
6. Після генерації — збілдуй проект:

```bash
./build.sh
```
