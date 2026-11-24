// Código sendo feito pelo Yuri
// Há um semáforo de carros funcionando abaixo com pedestres
// Há o modo noturno configurado com ifdef

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <stdint.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>

#define MODO_NORMAL 0
#define MODO_NOTURNO 1

#define MODO_ATUAL MODO_NORMAL
        // Basta alterar para MODO_NORMAL ou MODO_NOTURNO a segunda variável
        // Para escolher o modo de funcionamento

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
atomic_t pedido_pedestre = ATOMIC_INIT(0);
int ciclos = 0;
#define STACK_SIZE 1024
#define LIMITE_CICLOS 4

K_EVENT_DEFINE(evento_controle);
#define FIM_CICLO BIT(0)

K_SEM_DEFINE(verde, 0, 1);
K_SEM_DEFINE(amarelo, 0, 1);
K_SEM_DEFINE(vermelho, 0, 1);
K_SEM_DEFINE(vermelhoped, 0, 1);
K_SEM_DEFINE(verdeped, 0, 1);
K_MUTEX_DEFINE(mutex_estado);
K_SEM_DEFINE(retoma_carros, 0, 1);

static struct gpio_callback botao_callback;

void botao_pressionado(const struct device *dev,
                       struct gpio_callback *cb,
                       uint32_t pins)
{
    atomic_set(&pedido_pedestre, 1);
}

void ledVerde(void *p1, void *p2, void *p3) {

    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

#if MODO_ATUAL == MODO_NOTURNO

    while(1){
        k_sleep(K_FOREVER);
    }

#else

    while(1){
        k_sem_take(&verde, K_FOREVER);
        atomic_set(&pedestre, 0); // Carros liberados
        k_sem_give(&vermelhoped);
        gpio_pin_set_dt(&led, 1);
        k_msleep(3000);
        gpio_pin_set_dt(&led, 0);
        k_sem_give(&amarelo);
    }

#endif

}

void ledAmarelo(void *p1, void *p2, void *p3) {

    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

#if MODO_ATUAL == MODO_NOTURNO

    while(1){
        gpio_pin_set_dt(&led, 1);
        gpio_pin_set_dt(&led2, 1);
        k_msleep(1000);
        gpio_pin_set_dt(&led, 0);
        gpio_pin_set_dt(&led2, 0);
        k_msleep(1000);
    }
    
#else

    while(1){
        k_sem_take(&amarelo, K_FOREVER);
        gpio_pin_set_dt(&led, 1);
        gpio_pin_set_dt(&led2, 1);
        k_msleep(1000);
        gpio_pin_set_dt(&led, 0);
        gpio_pin_set_dt(&led2, 0);
        k_sem_give(&vermelho);
    }

#endif

}

void ledVermelho(void *p1, void *p2, void *p3) {

    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

#if MODO_ATUAL == MODO_NOTURNO

    while(1){
        k_sleep(K_FOREVER);
    }

#else

    while(1){
        k_sem_take(&vermelho, K_FOREVER);
        gpio_pin_set_dt(&led2, 1);
        k_sem_give(&vermelhoped);
        k_msleep(4000);
        k_event_post(&evento_controle, FIM_CICLO);

        k_sem_take(&retoma_carros, K_FOREVER);

        if (atomic_get(&pedestre) == 0) {
            gpio_pin_set_dt(&led2, 0);
            k_sem_give(&verde);
        }
    }

#endif

}

void ledVermelhoPed(void *p1, void *p2, void *p3) {

    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

#if MODO_ATUAL == MODO_NOTURNO

    while(1){
        gpio_pin_set(gpio_b, 0, 1);
        k_msleep(1000);
        gpio_pin_set(gpio_b, 0, 0);
        k_msleep(1000);
    }

#else

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

#endif

}

void ledVerdePed(void *p1, void *p2, void *p3) {

    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

#if MODO_ATUAL == MODO_NOTURNO

    while(1){
        k_sleep(K_FOREVER);
    }

#else

    while(1){
        k_sem_take(&verdeped, K_FOREVER);
        if (atomic_get(&pedestre) == 1){
            gpio_pin_set(gpio_b, 1, 1);
        }
        else{
            gpio_pin_set(gpio_b, 1, 0);
        }
    }

#endif

}

void controle(void *p1, void *p2, void *p3) {

    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

#if MODO_ATUAL == MODO_NOTURNO

    while(1){
        k_sleep(K_FOREVER);
    }

#else

    while(1){

        // Aguarda o fim de um ciclo dos carros
        k_event_wait(&evento_controle, FIM_CICLO, true, K_FOREVER);

        k_mutex_lock(&mutex_estado, K_FOREVER);

        ciclos++;

        // Verifica se chegou o momento da fase de pedestres
        if(ciclos >= LIMITE_CICLOS|| atomic_get(&pedido_pedestre) == 1){

            ciclos = 0;

            k_mutex_unlock(&mutex_estado);

            // Carros parados, pedestres liberados
            atomic_set(&pedestre, 1);
            k_sem_give(&vermelhoped);

            // Duração da fase de pedestres
            k_msleep(4000);

            // Carros liberados, pedestres parados
            atomic_set(&pedestre, 0);
            atomic_set(&pedido_pedestre, 0);
            k_sem_give(&vermelhoped);
        }
        else{
            k_mutex_unlock(&mutex_estado);
        }

        // Libera os carros para o próximo ciclo
        k_sem_give(&retoma_carros);
    }

#endif

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
    gpio_pin_configure(gpio_b, 2, GPIO_INPUT | GPIO_PULL_UP); // Botão de Pedestre (CHECAR PINO)
    gpio_pin_set(gpio_b, 0, 1); // Vermelho de pedestres ligado
    gpio_pin_set(gpio_b, 1, 0); // Verde de pedestres desligado

    // Configura o callback do botão
    gpio_init_callback(&botao_callback, botao_pressionado, BIT(2));

    gpio_add_callback(gpio_b, &botao_callback);

    // Interrupção ao pressionar o botão
    gpio_pin_interrupt_configure(gpio_b, 2, GPIO_INT_EDGE_FALLING);

    k_sem_give(&verde);
    
    while (1) {
        k_sleep(K_FOREVER);
    }
}