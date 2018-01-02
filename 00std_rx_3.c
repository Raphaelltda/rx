/* Receiver - Master Device*/
/* This code will periodically send out a sync signal */

#include "msp430f1611.h"
#include "stdint.h"

volatile int sync = 0;
volatile int trig = 0;
volatile int echo = 0;


int main(void) {
   WDTCTL     =  WDTPW + WDTHOLD;                // disable watchdog timer
   
   DCOCTL     =  DCO0 | DCO1 | DCO2;             // MCLK at 4.8MHz
   BCSCTL1    =  RSEL0 | RSEL1 | RSEL2;          // MCLK at 4.8MHz
   
   P5DIR     |=  0x70;                         // set pins as otuputs (4, 5, 6) - P5DIR = 0bx111xxxx for LEDs
   P5OUT     |=  0x70;                         // set the pins HIGH - P2OUT = 0bx111xxxx, all LEDs off
   
    P2DIR     |=  0x48;                          // Set P2.6/3 as output (1 = out, 0 = in)
    P2OUT     &= ~0x08;                          // Set P2.3 initially low - For trig
    P2OUT     |=  0x40;                          // Set P2.6 initially high - For sync

   
   // Echo detection on falling edge
   P2DIR     &= ~0x80;                           // Set P2.7 as an input
   P2OUT     |=  0x80;                           // put P2.7 initially high
   P2IE      |=  0x80;                           // enable P2.7 interrupt
   P2IFG     &= ~0x80;                           // clear pin flag in advance
   P2IES     |=  0x80;                           // interrupt on falling edge


    TACCTL0    =  CCIE;                           // capture/compare interrupt enable
    TACTL      =  TASSEL_1 | MC_1 | ID_2;         // Select ACLK, 'counts up' mode, input divider (4)
    TACCR0     =  18000;                          // 18000k@32kHz ~ 550 ms
    // 550 ms x 4 = 2.2 s
    
    __enable_interrupt();    // System is NOT going into any Low Power Mode
    
    while(1){
        if (trig != 0){            // for timer
            if (sync > 2){        // send sync signal every 4 cycles
                P2OUT ^= 0x40;     // toggle P2.6 (Pull low)
                P5OUT ^= 0x20;     // toggle green led -> for testing purposes
                __delay_cycles(24); // delay ~ 5 us
                P2OUT ^= 0x40;
                sync=0;
            }
            P5OUT ^= 0x10;          // toggle red led
            P2OUT ^= 0x08;          // toggle P2.3 - trig
            __delay_cycles(100);    // delay ~ 20 us
            P2OUT ^= 0x08;
            __delay_cycles(144000); // delay 50 ms
            P5OUT ^= 0x10;
            trig = 0;              // clear flag
        }
        
        if (echo != 0){            // for echo
            if (TAR < 1000){        // if detected before 30 ms 
                int j;
                for (j=0; j<8; j++){ // flash blue led repeatedly
                    P5OUT ^= 0x40;
                    __delay_cycles(96000); // delay 20 ms
                }
            }
            echo = 0;              // clear flag
        }
    }
}

//brief This function is called when the TimerA interrupt fires.
#pragma vector = TIMERA0_VECTOR
__interrupt void TIMERA0_ISR (void) {
    trig = 1;                  // raise trig flag
    sync++;
}

//brief This function is called when the P2.7 interrupt fires.
#pragma vector = PORT2_VECTOR
__interrupt void PORT2_ISR (void) {
    echo = 1;              // raise echo flag
    P2IFG &= ~0x80;        // clear pin flag
}

