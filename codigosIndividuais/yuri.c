// Código sendo feito pelo Yuri
// Há um semáforo de carros funcionando abaixo

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

K_SEM_DEFINE(verde, 1, 1)
K_SEM_DEFINE(amarelo, 0, 1)
K_SEM_DEFINE(vermelho, 0, 1)

void ledverde(void *p1, void *p2, void *p3) {

    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    while(1){
        k_sem_take(&verde, K_FOREVER);
        gpio_pin_set_dt(&led, 1);
        k_msleep(3000);
        gpio_pin_set_dt(&led, 0);
        k_sem_give(&amarelo);
    }
}

void ledamarelo(void *p1, void *p2, void *p3) {

    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    while(1){
        k_sem_take(&amarelo, K_FOREVER);
        gpio_pin_set_dt(&led, 1);
        gpio_pin_set_dt(&led2, 1);
        k_msleep(1000);
        gpio_pin_set_dt(&led, 0);
        gpio_pin_set_dt(&led2, 0);
        k_sem_give(&vermelho);
    }
}

void ledvermelho(void *p1, void *p2, void *p3) {

    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    while(1){
        k_sem_take(&vermelho, K_FOREVER);
        gpio_pin_set_dt(&led2, 1);
        k_msleep(4000);
        gpio_pin_set_dt(&led2, 0);
        k_sem_give(&verde);
    }
}

// Definição das Threads
K_THREAD_DEFINE(a_tid, STACK_SIZE, ledverde,
                NULL, NULL, NULL,
                5, 0, 0);

K_THREAD_DEFINE(b_tid, STACK_SIZE, ledvermelho,
                NULL, NULL, NULL,
                5, 0, 0);

K_THREAD_DEFINE(c_tid, STACK_SIZE, ledamarelo,
                NULL, NULL, NULL,
                5, 0, 0);

void main(void) {
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