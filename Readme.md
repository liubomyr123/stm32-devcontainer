# STM32 Dev Container — Документація

## Відкрити проект у Dev Container

1. Відкрий WSL термінал
2. Перейди в папку проекту:
```bash
cd ~/projects/stm32-template
```
3. Відкрий VS Code з цієї папки:
```bash
code .
```
4. У VS Code натисни `><` внизу зліва → **Reopen in Container**

> При першому запуску Docker збудує образ — це займе кілька хвилин

---

## CubeMX — генерація коду

1. Отримай шлях до проекту — відкрий WSL термінал і виконай:
```bash
wslpath -w ~/projects/stm32-template
```

Приклад відповіді:

```bash
\\wsl.localhost\Ubuntu-22.04\home\liubomyr\projects\stm32-template
```

2. Відкрий File Explorer на Windows і перейди за цією адресою в папку `stm32/`
3. Відкрий `stm32.ioc` у CubeMX
4. В секції **Project Manager**:
    - не міняй шляху до проекту
    - Toolchain/IDE має бути `CMake`
5. Натисни **Generate Code**

---

## Збілдити прошивку

```bash
chmod +x ./build.sh   # тільки перший раз
./build.sh
```

---

## Підключити ST-Link до WSL

ST-Link треба підключати до WSL кожного разу після перезавантаження Windows.

Виконай в **PowerShell як адміністратор**:

```powershell
usbipd attach --wsl --hardware-id 0483:374b
```

> `0483:374b` — унікальний VID:PID для ST-Link, однаковий на будь-якому комп'ютері.
> Якщо хочеш дізнатись busid свого пристрою — виконай `usbipd list`

---

## Залити прошивку

```bash
chmod +x ./flash.sh   # тільки перший раз
./flash.sh
```

---

## Дебаг

1. Підключи ST-Link до WSL (див. вище)
2. Натисни **F5** у VS Code — openocd запуститься автоматично
3. Дебагер зупиниться на початку `main()`
4. Використовуй стандартні кнопки VS Code: Step Over, Step Into, Continue

---

## Структура проекту

```
.
├── .devcontainer/       # Docker конфігурація
├── .vscode/             # VS Code налаштування
├── app/                 # Твій кастомний код
├── stm32/               # Код згенерований CubeMX
├── build.sh             # Скрипт білду
├── flash.sh             # Скрипт прошивки
└── CMakeLists.txt       # Рутовий CMake
```


## Troubleshooting

### VS Code не підключається до контейнера (`product.json` пошкоджений)

Симптом: при відкритті контейнера з'являється помилка `SyntaxError: Unexpected end of JSON input`

Виправлення — виконай в WSL терміналі:

```bash
docker ps -a  # знайди ID контейнера
docker rm -f <ID контейнера>
docker volume rm vscode
```

Потім **Reopen in Container** у VS Code.

---

### ST-Link не видно в контейнері

Симптом: `lsusb` не показує ST-Link

1. Перевір чи ST-Link підключений фізично
2. Виконай в **PowerShell як адміністратор**:
```powershell
usbipd list  # перевір чи є ST-Link в списку
usbipd attach --wsl --hardware-id 0483:374b
```
3. Перевір в контейнері:
```bash
lsusb  # має з'явитись ST-Link
```

> ST-Link треба підключати заново після кожного перезавантаження Windows
