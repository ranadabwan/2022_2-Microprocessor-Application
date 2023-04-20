#include "msp.h"
#include "Clock.h"
#include "stdio.h"

#define LED_RED 1
#define LED_GREEN (LED_RED << 1)
#define LED_BLUE (LED_RED << 2)

int cur = 0;

void led_init() {
    // Set p2 as GPIO
    P2->SEL0 &= ~0x07;
    P2->SEL1 &= ~0x07;

    // Input or Output. cur is output
    P2->DIR |= 0x07;

    // Turn off LED
    P2->OUT &= ~0x07;
}

void switch_init() {
    P1->SEL0 &= ~0x12;
    P1->SEL1 &= ~0x12;

    P1->DIR &= ~0x12;

    P1->REN |= 0x12;

    P1->OUT |= 0x12;
}

void turn_on_led(int color) {
    P2->OUT &= ~0x07;
    P2->OUT |= color;
}

void turn_off_led() {
    P2->OUT &= ~0x07;
}
void systick_init(void) {
    SysTick->LOAD = 0x00FFFFFF;
    SysTick->CTRL = 0x00000005;
}

void systick_wait1ms() {
    SysTick->LOAD = 48000;
    SysTick->VAL = 0;
    while((SysTick->CTRL & 0x00010000) == 0) {};
}

void systick_wait1s() {
    int i;
    int count = 1000;
    int sw1 = P1->IN & 0x02;

    for (i = 0; i < count; i++) {
        systick_wait1ms();
        sw1 = P1->IN & 0x02;
        if (!sw1) {
            printf("Current Time : %d m %d s %d ms\n", cur/60000, cur/1000, cur%1000);
        };
        cur += 1;
    }
}

void motor_init(void){
    P3->SEL0 &= ~0xC0;
    P3->SEL1 &= ~0xC0;// configure nSLPR & nSLPL as GPIO
    P3->DIR |= 0xC0;// make nSLPR & nSLPL as output
    P3->OUT &= ~0xC0;// output LOW

    P5->SEL0 &= ~0x30;
    P5->SEL1 &= ~0x30;// configure DIRR & DIRL as GPIO
    P5->DIR |= 0x30;// make DIRR & DIRL as output
    P5->OUT &= ~0x30;// output LOW

    P2->SEL0 &= ~0xC0;
    P2->SEL1 &= ~0xC0;// PWMR & PWML
    P2->DIR |= 0xC0;// PWMR & PWML
    P2->OUT &= ~0xC0;// output LOW
}

void move_forward(void){
    P5->OUT &= ~0x30;
    P2->OUT |= 0xC0;
    P3->OUT |= 0xC0;
    Clock_Delay1us(1000);
}
void move_stop(void){
    P2->OUT &= ~0xC0;
    Clock_Delay1us(1000);
}

void ir_sensor_init(void){
    // even IR Emitter init
    P5->SEL0 &= ~0x08;
    P5->SEL1 &= ~0x08;
    P5->DIR |= 0x08;
    P5->OUT &= ~0x08;

    // odd IR Emitter init
    P9->SEL0 &= ~0x04;
    P9->SEL1 &= ~0x04;
    P9->DIR |= 0x04;
    P9->OUT &= ~0x04;

    // IR Sensor init
    P7->SEL0 &= ~0xFF;
    P7->SEL1 &= ~0xFF;
    P7->DIR &= ~0xFF;
}

void pwm_init34(uint16_t period, uint16_t duty3, uint16_t duty4) {
    // CCR0 period
    TIMER_A0->CCR[0] = period;

    // divide by 1
    TIMER_A0->EX0 = 0x0000;

    // toggle/reset
    TIMER_A0->CCTL[3] = 0x0040;
    TIMER_A0->CCR[3] = duty3;
    TIMER_A0->CCTL[4] = 0x0040;
    TIMER_A0->CCR[4] = duty4;

    // 0x200 -> SMCLK
    // 0b1100 0000 -> input divider /8
    // 0b0011 0000 -> up/down mode
    TIMER_A0->CTL = 0x02F0;

    // set alternative
    P2->DIR |= 0xC0;
    P2->SEL0 |= 0xC0;
    P2->SEL1 &= ~0xC0;
}

void pwm_motor_init(void){
    motor_init();

    pwm_init34(7500, 0, 0);
}

void pwm_move(uint16_t leftDuty, uint16_t rightDuty) {
    P3->OUT |= 0xC0;
    TIMER_A0->CCR[3] = leftDuty;
    TIMER_A0->CCR[4] = rightDuty;
}

void left_forward() {
    P5->OUT &= ~0x10;
}

void left_backward() {
    P5->OUT |= 0x10;
}

void right_forward() {
    P5->OUT &= ~0x20;
}

void right_backward() {
    P5->OUT |= 0x20;
}

void (*TimerA2Task) (void);

void TimerA2_Init(void(*task) (void), uint16_t period) {
    TimerA2Task = task;
    TIMER_A2->CTL = 0x0280;
    TIMER_A2->CCTL[0] = 0x0010;
    TIMER_A2->CCR[0] = (period - 1);
    TIMER_A2->EX0 = 0x0005;
    NVIC->IP[3] = (NVIC->IP[3]&0xFFFFFF00) | 0X00000040;
    NVIC->ISER[0] = 0x00001000;
    TIMER_A2->CTL |= 0x0014;
}

void TA2_0_IRQHandler(void) {
    TIMER_A2->CCTL[0] &= ~0x0001;
    (*TimerA2Task) ();
}

void timer_A3_capture_init() {
    P10->SEL0 |= 0x30;
    P10->SEL1 &= ~0x30;
    P10->DIR &= ~0x30;

    TIMER_A3->CTL &= ~0x0030;;
    TIMER_A3->CTL = 0x0200;

    TIMER_A3->CCTL[0] = 0x4910;
    TIMER_A3->CCTL[1] = 0x4910;
    TIMER_A3->EX0 &= ~0x0007;

    NVIC->IP[3] = (NVIC->IP[3]&0x0000FFFF) | 0x40400000;
    NVIC->ISER[0] = 0x0000C000;
    TIMER_A3->CTL |= 0x0024;
}

uint16_t first_left;
uint16_t first_right;

uint16_t period_left;
uint16_t period_right;

void TA3_0_IRQHandler(void) {
    TIMER_A3->CCTL[0] &= ~0x0001;
    period_right = TIMER_A3->CCR[0] - first_right;
    first_right = TIMER_A3->CCR[0];
}

/*
void TA3_N_IRQHandler(void) {
    TIMER_A3->CCTL[1] &= ~0x0001;
    period_left = TIMER_A3->CCR[1] - first_left;
    first_left = TIMER_A3->CCR[1];
}
*/

uint32_t get_left_rpm() {
    return 2000000 / period_left;
}

uint32_t get_right_rpm() {
    return 2000000 / period_right;
}

uint32_t left_count;

void TA3_N_IRQHandler(void){
    TIMER_A3->CCTL[1] &= ~0x0001;
    left_count++;
}

void task() {
    printf("interrupt occur!\n");
}

void main(void) {
    Clock_Init48MHz();
    systick_init();
    pwm_motor_init();

    int sensor;
    int chk = 0;
    ir_sensor_init();

    timer_A3_capture_init();
    left_forward();
    right_forward();
    pwm_move(0,0);

    while(1) {
        // Turn on IR LEDs
        P5->OUT |= 0x08;
        P9->OUT |= 0x04;

        //Make P7.0-P7.7 as output
        P7->DIR = 0xFF;
        // Charges a capacitor
        P7->OUT = 0xFF;
        // Wait for fully charged
        Clock_Delay1us(20);

        //Make P7.0-P7.7 as input
        P7->DIR = 0x00;

        Clock_Delay1us(950);

        sensor = P7->IN & 0xFF;
        //white : 0, black : 1
        printf("sensor : %x \n", sensor);
        if ( (sensor == 0xFC) || (sensor == 0x3F) || (sensor == 0x7E) || (sensor == 0xFE) || (sensor == 0x7F) ){
            if (chk <= 1){
                left_forward();
                right_forward();
                pwm_move(1000,1000);
                systick_wait1s();
                chk = 1;
            } else {
                pwm_move(0,0);
                break;
            }
        } else if ((sensor == 0x18) || (sensor == 0x30) || (sensor == 0x0C) || (sensor == 0x10) || (sensor == 0x08)) {
            left_forward();
            right_forward();
            pwm_move(1000,1000);
            if (chk == 1){
                chk = 2;
            }
        } else if ((((sensor & 0x10) == 0x00) || ((sensor & 0xF0) == 0x10)) && (sensor & 0x0F)) {
            pwm_move(0,0);
            left_forward();
            right_backward();
            pwm_move(1000, 1000);
            if (chk == 1){
                chk = 2;
            }
        } else if (((sensor & 0x08) == 0x00) && (sensor & 0xF0)) {
            pwm_move(0,0);
            left_backward();
            right_forward();
            pwm_move(1000, 1000);
            if (chk == 1){
                chk = 2;
            }
        } else {
            pwm_move(0,0);
            left_forward();
            right_forward();
            pwm_move(1000,1000);
            if (chk == 1){
                chk = 2;
            }
        }

        // Trun off The IR LEDs
        P5->OUT &= ~0x08;
        P9->OUT &= ~0x04;

        Clock_Delay1ms(10);
    }
}
