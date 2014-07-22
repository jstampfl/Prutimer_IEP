// pi.p
//      To get the echo pulse on pin P8 15.
//      assemble:  pasm -v3 -b pt.p    (could be pasm_2 if you follow directions
.setcallreg r2.w0            //   
.origin 0
.entrypoint T811
#define CTRL 0x22000         //Start of control registers.
#define IEP  0x2E000         //Start of IEP registers.
T811:
   ldi r11,0
   ldi r12,0                 //pointer to data memory
   mov r7, 0x111             // value to enable the IEP
                             // GLOBAL_CONFIG
                             //    0x1 to enable
                             //    0x10 to set default increment
                             //    0x100 to set compensation increment
                             //     compensation is not used here 
   mov r5,IEP                // pointer to IEP registers
   ldi r6,0                  // index for IEP registers
   ldi r3, 0                 // zero to disable IEP
   sbbo r3,r5,r6,4           // Disable the IEP count
   ldi r6,0xC                // Offset for IEP counter
   ldi r4,1
   sbbo r4,r5,r6,4           // zero the counter
TB1:
  qbbs TB1,r31,15            // wait here for echo line to go low

// WAIT FOR THE ECHO PULSE

TB2:

  qbbc TB2,r31,15            // wait here for echo line to go high
                             //  - start of pulse
                             
// START THE IEP COUNTER

   sbbo r7,r5,0,4             // enable IEP 

// WAIT FOR THE PULSE TO END

TB3:

   qbbs TB3,r31,15            // wait here until echo line goes low.

// GET THE CYCLE COUNT AND PUT IN PRU0 LOCAL DATA MEMORY

  lbbo r11,r5,0xC,4           // get the count
  sbbo r10,r12,0,8           // put r10 & r11 (the unused signal
                             // & cycle count) in pru0 data memory
// STOP THE IEP COUNTER

  sbbo r3,r5,0,4             // put back
  sbbo r3,r5,0xC,4            // Zero the cycle count

//  SEND INTERRUPT SIGNAL TO LINUX SIDE

  mov r31.b0,35              //Send interrupt to Linux side

  lbbo r11,r12,8,4           // Check for end flag from Linux side
  qbne TB1,r11,2             // go back for more

TB4:
  halt
