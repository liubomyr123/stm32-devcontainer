#include "include/button.h"
#include "include/led.h"

extern "C" void app_main() {
    LED led(GPIOG, GPIO_PIN_13);
    Button btn(GPIOA, GPIO_PIN_0);

    while (1) {
        if (btn.isPressed()) {
            led.toggle();
            led.delay(500);
        } else {
            led.off();
        }
    }
}
