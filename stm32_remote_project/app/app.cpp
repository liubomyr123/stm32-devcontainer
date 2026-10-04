#include <array>

#include "cmsis_os.h"
#include "fatfs.h"
#include "include/joystickKY023.hpp"
#include "include/logger.hpp"
#include "include/nrf24radio.hpp"
#include "include/sdcard.hpp"

extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim2;

extern "C" void app_main()
{
    LOG_INFO("APP", "Started!");

    // SdCard card = {};

    // if (!card.init())
    // {
    //     vTaskDelete(nullptr);
    // }

    // const char message[] = "Hello from STM32!\r\n";
    // card.updateCurrentLog(message);

    JoystickKY023 joystickControl{&hadc1,         //
                                  &htim2,         //
                                  ADC_CHANNEL_0,  //
                                  ADC_CHANNEL_1,  //
                                  GPIOC,          //
                                  GPIO_PIN_1};

    if (!joystickControl.init())
    {
        vTaskDelete(nullptr);
    }

    JoystickKY023 joystickCamera{&hadc1,          //
                                 &htim2,          //
                                 ADC_CHANNEL_4,   //
                                 ADC_CHANNEL_10,  //
                                 GPIOC,           //
                                 GPIO_PIN_2};

    if (!joystickCamera.init())
    {
        vTaskDelete(nullptr);
    }

    while (true)
    {
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_2);

        int16_t joyControlX = 0;
        joystickControl.readAdcChannelXPercentage(joyControlX);
        int16_t joyControlY = 0;
        joystickControl.readAdcChannelYPercentage(joyControlY);
        int16_t joyCameraX = 0;
        joystickCamera.readAdcChannelXPercentage(joyCameraX);
        int16_t joyCameraY = 0;
        joystickCamera.readAdcChannelYPercentage(joyCameraY);

        GPIO_PinState joyControlSw = joystickControl.readSwButton();
        GPIO_PinState joyCameraSw = joystickCamera.readSwButton();

        // LOG_INFO("JOY", "joyControl: X=[%d%%] Y=[%d%%] SW=%d | joyCamera: X=[%d%%] Y=[%d%%]
        // SW=%d",
        //          joyControlX, joyControlY, joyControlSw, joyCameraX, joyCameraY, joyCameraSw);

        osDelay(250);
    }
}

/*
 *  У пульта і машинки є спільна кодова фраза. Треба:
 *  - Щоб кожен переконався, що інший її знає, а фраза по радіо не летіла.
 *  - Щоб після цього в обох з'явилися нові ключі для цього сеансу.
 *
 * Чим ми оперуємо:
 *  - bind_phrase  - 16 байт, зашита в обидві прошивки
 *  - remote_id    - 4 байти, з заводського UID чипа пульта
 *  - car_id       - 4 байти, з заводського UID чипа машинки
 *  - R1           - 4 випадкові байти від пульта, нові при кожному Bind
 *  - R2           - 4 випадкові байти від машинки, нові при кожному Bind
 *
 * Ключів два, по одному на напрямок. Обидва виводяться з
 *  [remote_id 4][car_id 4][R1 4][R2 4], де блок[0] ^= маркер напрямку:
 *      - 0x01 для key_remote_to_car
 *      - 0x02 для key_car_to_remote
 *  І цей блок шифрується AES з ключем bind_phrase
 *
 *  Далі ми робимо наступне:
 *  1) Пульт надсилає BIND_REQ = [опкод][remote_id][R1].
 *  Машинка в цей час висить в очікуванні, постійно слухає.
 *  2) Машинка отримує пакет BIND_REQ, читає опкод, дістає remote_id та R1 та запам'ятовує їх.
 *  Пульт в цей час очікує на відповідь. Далі машинка
 *  надсилає BIND_RESP = [опкод][car_id][R2][ехо R1].
 *  Тепер машинка знає випадкове число пульта і своє, і може вивести обидва ключі.
 *  Але Bound вона ще не стає: вона не знає, чи пульт знає фразу.
 *  3) Пульт отримує пакет BIND_RESP, дістає car_id, R2 та ехо R1.
 *  Перевіряє, що його особистий R1 співпадає з тим, що вернула машинка.
 *  Потім також виводить обидва ключі з [remote_id][car_id][R1][R2] та напрямку.
 *  4) Далі пульт відправляє назад пакет BIND_CONFIRM = [опкод][ехо R2],
 *  зашифрований ключем key_remote_to_car.
 *  5) Машинка читає BIND_CONFIRM, розшифровує його ключем key_remote_to_car,
 *  перевіряє tag і те, що ехо R2 співпадає з її особистим R2.
 *  Якщо все ок, пульт точно знає фразу, і тільки тепер машинка стає Bound.
 *  6) Машинка відправляє пульту BIND_ACK = [опкод][ехо R1],
 *  зашифрований ключем key_car_to_remote.
 *  Пульт розшифровує, перевіряє tag і ехо R1. Якщо все ок, машинка точно знає фразу,
 *  і тільки тепер пульт стає Bound.
 *  7) В результаті кожна сторона переконалася, що бінд-фраза однакова, і з'єднання
 *  встановлене. Можна обмінюватися даними з ключами, унікальними на поточну сесію.
 *  Кожен наступний Bind дає нові R1 та R2, а отже нові ключі.
 *
 * ПАКЕТИ
 * ID у зашифрованих кадрах не дублюємо: вони вже входять у вивід ключа, тож
 * неправильний ID дає інший ключ, і кадр просто не розшифрується.
 *
 * 1) BIND_REQ      пульт -> машинка    відкритий
 *      [опкод 1][remote_id 4][R1 4]                                  = 9 байт
 *
 * 2) BIND_RESP     машинка -> пульт    відкритий
 *      [опкод 1][car_id 4][R2 4][ехо R1 4]                           = 13 байт
 *
 * 3) BIND_CONFIRM  пульт -> машинка    зашифрований ключем key_remote_to_car
 *      [nonce 7] [опкод 1][ехо R2 4] [tag 4]                         = 7 + 5 + 4 = 16 байт
 *
 * 4) BIND_ACK      машинка -> пульт    зашифрований ключем key_car_to_remote
 *      [nonce 7] [опкод 1][ехо R1 4] [tag 4]                         = 7 + 5 + 4 = 16 байт
 *
 * 5) PING_PONG     в обидва боки       зашифрований ключем свого напрямку
 *      [nonce 7] [опкод 1][counter 1] [tag 4]                        = 7 + 2 + 4 = 13 байт
 *
 * ПІДСУМОК ПО ДОВЖИНАХ
 *   Payload nRF24 статичний, тому всі кадри мають однакову довжину: 16 байт.
 *   BIND_CONFIRM і BIND_ACK заповнюють її повністю, а коротші кадри
 *   (BIND_REQ 9, BIND_RESP 13, PING_PONG 13) добиваємо нулями після кінця.
 *   Приймач для зашифрованих кадрів бере лише потрібну кількість байт і ігнорує добивання.
 */