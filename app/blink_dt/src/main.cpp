#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(app_led), gpios);
static const struct gpio_dt_spec onboard = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

int main(void)
{
    bool led_state = true;
    int32_t sleep_duration_ms = 1000;

    if (!gpio_is_ready_dt(&led)) return 0;
    if (!gpio_is_ready_dt(&onboard)) return 0;

    if (gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE) < 0) return 0;
    if (gpio_pin_configure_dt(&onboard, GPIO_OUTPUT_ACTIVE) < 0) return 0;

    #ifdef CONFIG_DT_BLINK_LED
        sleep_duration_ms = CONFIG_APP_HEARTBEAT_PERIOD_MS;
    #endif // CONFIG_DT_BLINK_LED

    while (1) {
        if (gpio_pin_toggle_dt(&led) < 0) return 0;
        if (gpio_pin_toggle_dt(&onboard) < 0) return 0;

        led_state = !led_state;
        k_msleep(sleep_duration_ms);
    }
    return 0;
}
