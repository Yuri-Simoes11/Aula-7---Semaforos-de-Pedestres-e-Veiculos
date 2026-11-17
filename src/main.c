// Código sendo feito pelo Yuri

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <stdint.h>
#include <zephyr/logging/log.h>

// Define o LED usando Device Tree
#define LED0_NODE DT_ALIAS(led0) // Verde
#define LED1_NODE DT_ALIAS(led1) // Azul
#define LED2_NODE DT_ALIAS(led2) // Vermelho

// Verifica se o LED está definido no Device Tree
#if DT_NODE_HAS_STATUS(LED0_NODE, okay)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
#else
#error "Unsupported board: led0 devicetree alias is not defined"
#endif

#if DT_NODE_HAS_STATUS(LED1_NODE, okay)
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
#else
#error "Unsupported board: led1 devicetree alias is not defined"
#endif

#if DT_NODE_HAS_STATUS(LED2_NODE, okay)
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(LED2_NODE, gpios);
#else
#error "Unsupported board: led2 devicetree alias is not defined"
#endif

#define STACK_SIZE 1024

void ledverde(void *p1, void *p2, void *p3) {

    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    while(1){
        gpio_pin_toggle_dt(&led);
        k_msleep(500);
    }
}

void ledvermelho(void *p1, void *p2, void *p3) {

    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    while(1){
        gpio_pin_toggle_dt(&led2);
        k_msleep(2000);
    }
}

// Definição das Threads
K_THREAD_DEFINE(a_tid, STACK_SIZE, ledverde,
                NULL, NULL, NULL,
                5, 0, 0);

K_THREAD_DEFINE(b_tid, STACK_SIZE, ledvermelho,
                NULL, NULL, NULL,
                5, 0, 0);

void main(void)
{
    int ret;

    // Verifica se o device está pronto
    if (!gpio_is_ready_dt(&led) || !gpio_is_ready_dt(&led1) || !gpio_is_ready_dt(&led2)) {
        printk("Error: LED device %s is not ready\n", led.port->name);
        return;
    }

    // Configura o pino como saída
    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        printk("Error %d: failed to configure LED pin\n", ret);
        return;
    }

    ret = gpio_pin_configure_dt(&led1, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        printk("Error %d: failed to configure LED pin\n", ret);
        return;
    }

    ret = gpio_pin_configure_dt(&led2, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        printk("Error %d: failed to configure LED pin\n", ret);
        return;
    }
    
    while (1) {

        k_sleep(K_FOREVER);
    }
}