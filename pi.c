/* pi.c -
 * HC-SR04  Ultrasonic Distance sensor using the IEP timer of the Pruss on
 * one of the PRUSS on the Beaglebone Black.
 * Files involved  pi.c, pi.p, prujts.dts
 * compile:  gcc pi.c -o pi -lprussdrv 
 * Uses the GPIO pin P9.12 (GPIO1_28 = 60) for the trigger.
 *
 * The C-code is derived from:
 * PRU_memAccessPRUDataRam.c
 *
 * Copyright (C) 2012 Texas Instruments Incorporated - http://www.ti.com/
 *
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
*/

/*
 * ============================================================================
 * Copyright (c) Texas Instruments Inc 2010-12
 *
 * Use of this software is controlled by the terms and conditions found in the
 * license agreement under which this software has been supplied or provided.
 * ============================================================================
 */

/*****************************************************************************
 * Copyright (c) John Stampfl 2014
 * This code is provided for your reading pleasure.
 * anyother use is at your own risk.  
 *
*  pi.c - sends the trigger pulse to the SR04 and waits for an interrupt from
*  the PRUSS, then reads the data from the Data memory of pru0. 
*
*  pi.p - times the echo and puts the value in pru local data memory. Then
*  interrupts the Linux program
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <prussdrv.h>
#include <pruss_intc_mapping.h>

#define PRU_NUM 	0
#define AM33XX

static int LOCAL_exampleInit ( unsigned short pruNum );
static int LOCAL_pr ( unsigned short pruNum );
static void *pruDataMem;
static unsigned int *pruDataMem_int;
/*  gpiosval toggles the gpio pin, used as the trigger.
 *
 */
int gpiosval(unsigned int gpio, unsigned int value) {
	int fd,len;
	char buf[64];
    len = snprintf(buf,sizeof(buf),"/sys/class/gpio/gpio%d/value",gpio);
    fd = open(buf,O_WRONLY);
    if (fd < 0) {
	    perror("gpio/setvalue");
	    return fd;
    }
//    printf("value %d\n",value);
    if(value) write(fd,"1",2);
    else write(fd,"0",2);
    close(fd);
    return 0;
}

int main (void)
{
    int ret1,i;
    int fd,len;
    char buf[64];

    unsigned int ret,gpio,value;
    tpruss_intc_initdata pruss_intc_initdata = PRUSS_INTC_INITDATA;

    printf("\nINFO: Starting %s example.\r\n", "pt - SR04");

    gpio = 60;  //Use P9.12 (GPIO1_28) for trigger.

    fd = open("/sys/class/gpio/export",O_WRONLY);
    if (fd < 0) {
	    perror("gpio/export");
	    return fd;
    }
    len = snprintf(buf,sizeof(buf),"%d",gpio);
    write(fd,buf,len);
    close(fd);
    len = snprintf(buf,sizeof(buf),"/sys/class/gpio/gpio%d/direction",gpio);
    fd = open(buf,O_WRONLY);
    if (fd < 0) {
	    perror("gpio/dirction");
	    return fd;
    }
    write(fd,"out",4);
    close(fd);
    len = snprintf(buf,sizeof(buf),"/sys/class/gpio/gpio%d/value",gpio);
    fd = open(buf,O_WRONLY);
    if (fd < 0) {
	    perror("gpio/value");
	    return fd;
    }

    ret = gpiosval(60,0);  // set the trigger low

    /* Initialize the PRU */
    prussdrv_init ();

    /* Open PRU Interrupt */
    ret = prussdrv_open(PRU_EVTOUT_0);
    if (ret)
    {
        printf("prussdrv_open open failed\n");
        return (ret);
    }

    /* Get the interrupt initialized */
    prussdrv_pruintc_init(&pruss_intc_initdata);

    /* Initialize example */
    printf("\tINFO: Initializing example.\r\n");
    LOCAL_exampleInit(PRU_NUM);

    /* Execute example on PRU */
    printf("\tINFO: Executing example.\r\n");
    prussdrv_exec_program (PRU_NUM, "./pi.bin");
    sleep(5);
    for (i = 0;i < 50;i++) {
    //          send the trigger pulse
    ret = gpiosval(60,1);  //Put the trigger high
    usleep(11);
    ret = gpiosval(60,0);   // now low.


    printf("\tINFO: Waiting for \r\n");
    prussdrv_pru_wait_event (PRU_EVTOUT_0);
    prussdrv_pru_clear_event (PRU_EVTOUT_0, PRU0_ARM_INTERRUPT);

    /* Check if example passed */
    ret1 = LOCAL_pr(PRU_NUM); // go get & print result
    usleep (500000); //Just pause so don't go too fast
    }
    sleep (2);
    /* Disable PRU and close memory mapping*/
    pruDataMem_int[2] = 2;  // use this as signal to pru to stop
    ret1 = LOCAL_pr(PRU_NUM); // get the last result
    prussdrv_pru_disable (PRU_NUM);
    prussdrv_exit ();

    return(0);

}

static int LOCAL_exampleInit ( unsigned short pruNum )
{
    //Initialize pointer to PRU data memory
    if (pruNum == 0)
    {
      prussdrv_map_prumem (PRUSS0_PRU0_DATARAM, &pruDataMem);
    }
    else if (pruNum == 1)
    {
      prussdrv_map_prumem (PRUSS0_PRU1_DATARAM, &pruDataMem);
    }
    pruDataMem_int = (unsigned int*) pruDataMem;

    // Flush the values in the PRU data memory locations
    pruDataMem_int[0] = 0x00;
    pruDataMem_int[1] = 0x00;
    pruDataMem_int[2] = 0x00;
    pruDataMem_int[3] = 0x00;

    return(0);
}

static int LOCAL_pr ( unsigned short pruNum )
{
    double tt;
    double xt = 200000000.0; //clock
    double xs = 34375.0; //speed of sound in centimeters
	int i;
	//Here read data from pru memory
	for (i=0;i<8;i++){
		printf("data %d %x\n",i,pruDataMem_int[i]);
	}
	tt = 1.0/xt;
	printf("period:  %.9f\n",tt);
	tt =  tt *(double) pruDataMem_int[1];
	printf("elasped:  %f\n",tt);
        tt=((tt * xs) / 2.0); 
	printf("distance: %f\n",tt);

        return 0;
}

