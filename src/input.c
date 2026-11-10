#include "input.h"
#include "board.h"
#include "clock.h"

/* Кнопки активны высоким уровнем: так устроен WKUP (PA0) и проверка
   SW2 в старом main. Дребезг 30 мс, длинное нажатие 800 мс.
   Очередь односторонняя: пишет только SysTick, читает только main. */

#define DEBOUNCE_TICKS 3u
#define LONG_TICKS 80u
#define QMASK 7u

static volatile uint32_t g_ms;
static volatile uint8_t q_buf[8];
static volatile uint8_t q_w;
static volatile uint8_t q_r;
static volatile uint8_t suppress;

typedef struct {
    uint8_t last_raw;
    uint8_t cnt;
    uint8_t down;
    uint8_t long_sent;
    uint16_t hold;
} BtnState;

static BtnState btn[2];

static int read_level(int index) {
    if (index == 0) {
        return gpio_read(BOARD_BTN1_PORT, BOARD_BTN1_PIN);
    }
    return gpio_read(BOARD_BTN2_PORT, BOARD_BTN2_PIN);
}

static void q_push(uint8_t event) {
    uint8_t next = (uint8_t)((q_w + 1u) & QMASK);
    if (next == q_r) {
        return;
    }
    q_buf[q_w] = event;
    q_w = next;
}

static int port_index(GPIO_TypeDef *port) {
    if (port == GPIOB) {
        return 1;
    }
    if (port == GPIOC) {
        return 2;
    }
    return 0;
}

static int exti_irq(unsigned line) {
    if (line <= 4u) {
        return 6 + (int)line;
    }
    if (line <= 9u) {
        return 23;
    }
    return 40;
}

static void exti_arm(GPIO_TypeDef *port, unsigned pin) {
    unsigned idx = pin >> 2;
    unsigned shift = (pin & 3u) * 4u;
    AFIO->EXTICR[idx] = (AFIO->EXTICR[idx] & ~(0xFu << shift))
        | ((uint32_t)port_index(port) << shift);
    EXTI->IMR |= 1u << pin;
    EXTI->RTSR |= 1u << pin;
    EXTI->FTSR &= ~(1u << pin);
    nvic_enable(exti_irq(pin));
}

static void clear_button_exti(void) {
    uint32_t mask = (1u << BOARD_BTN1_PIN) | (1u << BOARD_BTN2_PIN);
    if (EXTI->PR & mask) {
        EXTI->PR = mask;
    }
}

void EXTI0_IRQHandler(void) { clear_button_exti(); }
void EXTI1_IRQHandler(void) { clear_button_exti(); }
void EXTI2_IRQHandler(void) { clear_button_exti(); }
void EXTI3_IRQHandler(void) { clear_button_exti(); }
void EXTI4_IRQHandler(void) { clear_button_exti(); }
void EXTI9_5_IRQHandler(void) { clear_button_exti(); }
void EXTI15_10_IRQHandler(void) { clear_button_exti(); }

void buttons_init(void) {
    gpio_mode(BOARD_BTN1_PORT, BOARD_BTN1_PIN, GPIO_IN_PULL);
    gpio_reset(BOARD_BTN1_PORT, BOARD_BTN1_PIN);   /* подтяг к земле */
    gpio_mode(BOARD_BTN2_PORT, BOARD_BTN2_PIN, GPIO_IN_PULL);
    gpio_reset(BOARD_BTN2_PORT, BOARD_BTN2_PIN);
    exti_arm(BOARD_BTN1_PORT, BOARD_BTN1_PIN);
    exti_arm(BOARD_BTN2_PORT, BOARD_BTN2_PIN);
    if (read_level(0) || read_level(1)) {
        suppress = 1u;
    }
}

void buttons_suppress_until_release(void) {
    irq_disable();
    q_r = q_w;
    suppress = 1u;
    irq_enable();
}

uint8_t buttons_get(void) {
    if (q_r == q_w) {
        return BTN_NONE;
    }
    uint8_t event = q_buf[q_r];
    q_r = (uint8_t)((q_r + 1u) & QMASK);
    return event;
}

uint32_t millis(void) {
    return g_ms;
}

static void service_button(int index, uint8_t short_ev, uint8_t long_ev) {
    int level = read_level(index);
    BtnState *b = &btn[index];
    if ((uint8_t)level != b->last_raw) {
        b->last_raw = (uint8_t)level;
        b->cnt = 0u;
    }
    if (b->cnt < 250u) {
        b->cnt++;
    }
    if (b->cnt == DEBOUNCE_TICKS && (uint8_t)level != b->down) {
        b->down = (uint8_t)level;
        if (level) {
            b->hold = 0u;
            b->long_sent = 0u;
        } else if (!b->long_sent && !suppress) {
            q_push(short_ev);
        }
    }
    if (b->down) {
        if (b->hold < 60000u) {
            b->hold++;
        }
        if (!b->long_sent && b->hold >= LONG_TICKS) {
            b->long_sent = 1u;
            if (!suppress) {
                q_push(long_ev);
            }
        }
    }
}

void SysTick_Handler(void) {
    g_ms += 10u;
    service_button(0, BTN1_SHORT, BTN1_LONG);
    service_button(1, BTN2_SHORT, BTN2_LONG);
    if (suppress && !btn[0].down && !btn[1].down && !read_level(0) && !read_level(1)) {
        suppress = 0u;
    }
}

void systick_start(void) {
    SYST_CSR = 0u;
    SYST_RVR = (SystemCoreClock / 100u) - 1u;
    SYST_CVR = 0u;
    SYST_CSR = SYST_ENABLE | SYST_TICKINT | SYST_CLKSRC;
}
