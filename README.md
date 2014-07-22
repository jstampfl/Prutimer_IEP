Prutimer_IEP
========
Example of using the PRUSS IEP timer on the BEAGLEBONE to time a pulse.

notes - comments about this example

pi.c - Initialize the Pruss, send trigger pulse to the HC-SR04, waits for response from the PRUSS, reads the cycle count
       from the data memory of PRU0.  Computes the distance and displays on terminal.
       
pi.p - The PRUSS waits for echo to go high, starts IEP counter, waits for echo to go low, stops IEP counter and moves
       IEP count to local PRU0 data memory, zeros the counter and send signal to Linux side.
       
prujts-00A0.dts - The device tree overlay to enable the PRUSS
