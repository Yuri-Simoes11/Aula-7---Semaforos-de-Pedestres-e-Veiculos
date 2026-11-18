// Código sendo feito pelo Yuri
// Há um semáforo de carros funcionando abaixo

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <stdint.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>

// Define o LED usando Device Tree
#define LED0_NODE DT_ALIAS(led0) // Verde
#define LED1_NODE DT_ALIAS(led1) // Azul
#define LED2_NODE DT_ALIAS(led2) // Vermelho

#define GPIO_PORT DT_NODELABEL(gpiob)
static const struct device *gpio_b = DEVICE_DT_GET(GPIO_PORT);

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

atomic_t pedestre = ATOMIC_INIT(0);
volatile int ciclos = 0;
#define STACK_SIZE 1024

K_SEM_DEFINE(verde, 0, 1);
K_SEM_DEFINE(amarelo, 0, 1);
K_SEM_DEFINE(vermelho, 0, 1);
K_SEM_DEFINE(vermelhoped, 0, 1);
K_SEM_DEFINE(verdeped, 0, 1);

void ledVerde(void *p1, void *p2, void *p3) {

    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    while(1){
        k_sem_take(&verde, K_FOREVER);
        atomic_set(&pedestre, 0); // Carros liberados
        k_sem_give(&vermelhoped);
        gpio_pin_set_dt(&led, 1);
        k_msleep(3000);
        gpio_pin_set_dt(&led, 0);
        k_sem_give(&amarelo);
    }
}

void ledAmarelo(void *p1, void *p2, void *p3) {

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

void ledVermelho(void *p1, void *p2, void *p3) {

    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    while(1){
        k_sem_take(&vermelho, K_FOREVER);
        atomic_set(&pedestre, 1); // Carros parados
        k_sem_give(&vermelhoped);
        gpio_pin_set_dt(&led2, 1);
        k_msleep(4000);
        gpio_pin_set_dt(&led2, 0);
        ciclos++;
        k_sem_give(&verde);
    }
}

void ledVermelhoPed(void *p1, void *p2, void *p3) {

    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    while(1){
        k_sem_take(&vermelhoped, K_FOREVER);
        if (atomic_get(&pedestre) == 0){
            gpio_pin_set(gpio_b, 0, 1);
        }
        else{
            gpio_pin_set(gpio_b, 0, 0);
        }
        k_sem_give(&verdeped);
    }

}

void ledVerdePed(void *p1, void *p2, void *p3) {

    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    while(1){
        k_sem_take(&verdeped, K_FOREVER);
        if (atomic_get(&pedestre) == 1){
            gpio_pin_set(gpio_b, 1, 1);
        }
        else{
            gpio_pin_set(gpio_b, 1, 0);
        }
    }
}

void controle(void *p1, void *p2, void *p3) {

    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    if(ciclos == 4){
        ciclos = 0;
    }

    int estado;
    estado = gpio_pin_get(gpio_b, 0);

}

// Definição das Threads
K_THREAD_DEFINE(a_tid, STACK_SIZE, ledVerde,
                NULL, NULL, NULL,
                5, 0, 0);

K_THREAD_DEFINE(b_tid, STACK_SIZE, ledVermelho,
                NULL, NULL, NULL,
                5, 0, 0);

K_THREAD_DEFINE(c_tid, STACK_SIZE, ledAmarelo,
                NULL, NULL, NULL,
                5, 0, 0);

K_THREAD_DEFINE(d_tid, STACK_SIZE, ledVermelhoPed,
                NULL, NULL, NULL,
                4, 0, 0);

K_THREAD_DEFINE(e_tid, STACK_SIZE, ledVerdePed,
                NULL, NULL, NULL,
                4, 0, 0);

K_THREAD_DEFINE(f_tid, STACK_SIZE, controle,
                NULL, NULL, NULL,
                2, 0, 0);

void main(void) {
    int ret;

    // Verifica se o device está pronto
    if (!gpio_is_ready_dt(&led) || !gpio_is_ready_dt(&led1) || !gpio_is_ready_dt(&led2)) {
        printk("Error: LED device %s is not ready\n", led.port->name);
        return;
    }

    if (!device_is_ready(gpio_b)) {
        printk("GPIO B nao esta pronto!\n");
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

    gpio_pin_configure(gpio_b, 0, GPIO_OUTPUT); // PTB0 é o vermelho
    gpio_pin_configure(gpio_b, 1, GPIO_OUTPUT); // PTB1 é o verde
    gpio_pin_configure(gpio_b, 2, GPIO_INPUT); // Botão de Semáforos (CHECAR PINO)
    gpio_pin_set(gpio_b, 0, 0);
    gpio_pin_set(gpio_b, 1, 0);

    k_sem_give(&verde);
    
    while (1) {
        k_sleep(K_FOREVER);
    }
}