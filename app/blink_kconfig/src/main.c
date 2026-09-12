#include <time.h>
#include <math.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define THREAD_PERIOD_MS 1

#define LED_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);
static const struct pwm_dt_spec pwm = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led0));

// Heartbeat parameters
const double BPM = 60.0; // Beats per minute
const double PI = 3.14159265358979323846;

LOG_MODULE_REGISTER(blink_kconfig, LOG_LEVEL_INF);

void get_blink_sleep(unsigned int * sleep_time_ms)
{
    if (IS_ENABLED(CONFIG_BLINK_SLEEP_500MS))
    {
        *sleep_time_ms = 500;
    }
    else if (IS_ENABLED(CONFIG_BLINK_SLEEP_2000MS))
    {
        *sleep_time_ms = 2000;   
    }
    else
    {
        *sleep_time_ms = 1000;
    }
}

void get_brightness(uint8_t * brightness)
{
    #ifdef CONFIG_LED_BRIGHTNESS
    *brightness = CONFIG_LED_BRIGHTNESS;
    #else
    *brightness = 100;
    #endif
}

void get_fade_duration(uint16_t * fade_durantion_ms)
{
    #ifdef CONFIG_LED_FADE_DURATION
    *fade_durantion_ms = CONFIG_LED_FADE_DURATION;
    #else
    get_blink_sleep(fade_durantion_ms);
    #endif
}

// Function to simulate heartbeat brightness (0.0 to 1.0)
double heartbeatBrightness(double t) {
    // Heartbeat period in seconds
    double period = 60.0 / BPM;

    // Time within one heartbeat cycle
    double phase = fmod(t, period) / period;

    // Double pulse: two Gaussian peaks per cycle
    double pulse1 = exp(-pow((phase - 0.1) / 0.03, 2));
    double pulse2 = exp(-pow((phase - 0.3) / 0.03, 2));

    // Combine pulses and normalize
    return min(1.0, pulse1 + 0.6 * pulse2);
}

int main(void)
{
    int ret = 0;
    bool raising = true;
    unsigned int sleep_time = 1000;
    uint8_t brightness = 100;
    uint16_t fade_duration = 1000;
    uint8_t duty = 0;
    uint8_t step = 10;

    // Check if config is set
    if (!IS_ENABLED(CONFIG_LED_SUBSYSTEM))
    {
        LOG_WRN("Config LED Subsystem not set. Doing nothing!");
        return -ENODATA;
    }

    bool verbose = IS_ENABLED(CONFIG_LED_DEBUGGING);

    // Configure blink GPIO
    if (!gpio_is_ready_dt(&led))
    {
        if (verbose)
            LOG_ERR("LED is not ready");
        return ENODEV;
    }
    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
    if (ret != 0)
    {
        if (verbose)
            LOG_ERR("Failed to configure LED!");
        return ret;
    }

    // Configure PWM GPIO
    if (!pwm_is_ready_dt(&pwm))
    {
        if (verbose)
            LOG_ERR("PWM is not ready");
        return ENODEV;
    }

    get_blink_sleep(&sleep_time);
    get_brightness(&brightness);
    get_fade_duration(&fade_duration);
    uint16_t fade_sleep_time = fade_duration * step / UINT8_MAX;

    if (verbose)
    {
        LOG_INF("Start main loop with:");
        LOG_INF("\tSleep: %d", sleep_time);
        LOG_INF("\tBrightness: %d", brightness);
        LOG_INF("\tFade Dur.: %d (%d)", fade_duration, fade_sleep_time);
    }

    struct timespec start, now;
    sys_clock_gettime(SYS_CLOCK_MONOTONIC, &start);

    // Application Loop
    while (1)
    {
        // Custom Mode
        if (IS_ENABLED(CONFIG_LED_CUSTOM_BLINK))
        {
            sys_clock_gettime(SYS_CLOCK_MONOTONIC, &now);
            double t = (now.tv_sec - start.tv_sec) +
                (now.tv_nsec - start.tv_nsec) / 1e9;

            double brightness = heartbeatBrightness(t);

            // Converte para porcentagem (0–100%)
            uint32_t level = pwm.period*brightness;

            if (pwm_set_pulse_dt(&pwm, level) != 0 && verbose)
            {
                LOG_ERR("Failed to set HEARTBEAT");
            }
            
            if (brightness > 0.5)
            {
                gpio_pin_set_dt(&led, 1);
            }
            else
            {
                gpio_pin_set_dt(&led, 0);
            }

            // (~60 FPS)
            k_msleep(16);
        }
        // Fade Mode
        else if (IS_ENABLED(CONFIG_LED_ADVANCED))
        {
            uint32_t period = (uint32_t)(((uint64_t)pwm.period * duty) / 255);
            if (pwm_set_pulse_dt(&pwm, period) != 0 && verbose)
            {
                LOG_ERR("Failed to set PWM");
            }
            
            if (raising && ((uint16_t)duty + 10) > 255)
            {
                if (verbose)
                    LOG_INF("Set FALLING");
                raising = false;
            }
            else if (!raising && (duty - 10U) > duty)
            {
                if (verbose)
                    LOG_INF("Set RAISING");
                raising = true;
            }

            int sign = raising ? 1 : -1;
            duty += (10*sign);

            k_msleep(fade_sleep_time);
        }
        // Blink Mode
        else
        {
            if (gpio_pin_toggle_dt(&led) != 0 && verbose)
            {
                LOG_ERR("Failed to toggle GPIO");
            }

            if (verbose)
                LOG_INF("LED Toggled");

            k_msleep(sleep_time);
        }
    }
    


    return ret;
}
