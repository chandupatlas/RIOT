/******************************************************************************
Copyright 2008-2009, Freie Universitaet Berlin (FUB). All rights reserved.

These sources were developed at the Freie Universitaet Berlin, Computer Systems
and Telematics group (http://cst.mi.fu-berlin.de).
-------------------------------------------------------------------------------
This file is part of FeuerWare.

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

FeuerWare is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program.  If not, see http://www.gnu.org/licenses/ .
--------------------------------------------------------------------------------
For further information and questions please use the web site
    http://scatterweb.mi.fu-berlin.de
and the mailinglist (subscription via web site)
    scatterweb@lists.spline.inf.fu-berlin.de
*******************************************************************************/

/**
 * @ingroup msba2
 * @{
 */

/**
 * @file
 * @brief       MSB-A2 board initialization
 *
 * @author      Freie Universität Berlin, Computer Systems & Telematics, FeuerWhere project
 * @author      Heiko Will
 * @author      Kaspar Schleiser
 * @author      Michael Baar <baar@inf.fu-berlin.de>
 *
 * @note        $Id$
 */
#include <board.h>
#include <lpc23xx.h>
#include <VIC.h>
#include <cpu.h>
#include <config.h>
#include <string.h>
#include <flashrom.h>

#define PCRTC         BIT9
#define CL_CPU_DIV    4

config_t sysconfig  = {
    0,      ///< default ID
    0,      ///< default radio address
    0,      ///< default radio channel
};

/*---------------------------------------------------------------------------*/
/**
 * @brief   Enabling MAM and setting number of clocks used for Flash memory fetch
 * @internal
 */
static void
init_mam(void)
{
    MAMCR  = 0x0000;
    MAMTIM = 0x0003;
    MAMCR  = 0x0002;
}
/*---------------------------------------------------------------------------*/
static inline void
pllfeed(void)
{
    PLLFEED = 0xAA;
    PLLFEED = 0x55;
}
/*---------------------------------------------------------------------------*/
void init_clks1(void)
{
    // Disconnect PLL
    PLLCON &= ~0x0002;
    pllfeed();

    // Disable PLL
    PLLCON &= ~0x0001;
    pllfeed();

    SCS |= 0x20;                        // Enable main OSC
    while( !(SCS & 0x40) );             // Wait until main OSC is usable

    /* select main OSC, 16MHz, as the PLL clock source */
    CLKSRCSEL = 0x0001;

    // Setting Multiplier and Divider values
    PLLCFG = 0x0008;                    // M=9 N=1 Fcco = 288 MHz
    pllfeed();

    // Enabling the PLL */
    PLLCON = 0x0001;
    pllfeed();

    /* Set clock divider to 4 (value+1) */
    CCLKCFG = CL_CPU_DIV - 1;           // Fcpu = 72 MHz

#if USE_USB
    USBCLKCFG = USBCLKDivValue;     /* usbclk = 288 MHz/6 = 48 MHz */
#endif
}

void init_clks2(void){
    // Wait for the PLL to lock to set frequency
    while(!(PLLSTAT & BIT26));

    // Connect the PLL as the clock source
    PLLCON = 0x0003;
    pllfeed();

    /* Check connect bit status */
    while (!(PLLSTAT & BIT25));
}

void bl_init_clks(void)
{
    PCONP = PCRTC;          // switch off everything except RTC
    init_clks1();
    init_clks2();
    init_mam();
}

void bl_init_ports(void)
{
    SCS |= BIT0;                                            // Set IO Ports to fast switching mode

    /* UART0 */
    PINSEL0 |= BIT4 + BIT6;                                 // RxD0 and TxD0
    PINSEL0 &= ~(BIT5 + BIT7);

    /* LEDS */
    FIO3DIR |= LED_RED_PIN;
    FIO3DIR |= LED_GREEN_PIN;
    LED_RED_OFF;
    LED_GREEN_OFF;
}

void loop_delay(void) {
    volatile uint16_t i, j;
    for (i = 1; i < 30; i++) {
        for (j = 1; j != 0; j++) {
            asm volatile (" nop ");
        }
    }
}

void bl_blink(void) {
    LED_RED_ON;
    LED_GREEN_ON;
    
    loop_delay();

    LED_RED_OFF;
    LED_GREEN_OFF;
}

void bl_config_init(void) {
    extern char configmem[];
   if (*((uint16_t*) configmem) ==  CONFIG_KEY) {
       memcpy(&sysconfig, (configmem + sizeof(CONFIG_KEY)), sizeof(sysconfig));
       LED_GREEN_TOGGLE;
   }
   else {
       config_save();
   }
}
