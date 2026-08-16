#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define SLEEP_TIME_MS 100

/* The devicetree node identifier for the "led0" alias. */
#define LED_NODE DT_ALIAS(led0)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);
static const struct pwm_dt_spec pwm = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led0));

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

int main(void)
{
    uint8_t duty = 0;

    if (!gpio_is_ready_dt(&led)) return 0;
    if (!pwm_is_ready_dt(&pwm)) return 0;
    if (gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE) < 0) return 0;

    int ret = 0;
    while (ret == 0) {
        if (duty % 10 == 0) {
            int gpio_ret = gpio_pin_toggle_dt(&led);
            if (gpio_ret < 0) {
                ret = gpio_ret;
            }
        }

        uint32_t period = (uint32_t)(((uint64_t)pwm.period * duty) / 255);
        ret = pwm_set_pulse_dt(&pwm, period);

        LOG_INF("PWM: %d", duty);
        k_msleep(SLEEP_TIME_MS);
        duty++;
    }

    return ret;
}
