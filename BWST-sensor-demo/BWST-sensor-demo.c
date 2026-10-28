
/******************************************************************************

 @file  BWST-sensor-demo.c

 @brief Battery-free wireless torque sensor -- sensor node firmware.
        Samples a strain-gauge bridge and transmits the result over the
        CC430F5137 sub-1 GHz radio. Based on example code from Texas
        Instruments.

 @author Pengyu Li (STVL, City University of Hong Kong)

 Copyright (c) 2023 Pengyu Li
 SPDX-License-Identifier: MIT

 Target Device: CC430F5137

 ******************************************************************************/

#include "BWST-sensor-demo.h"

#include "CC430F5137.h"




unsigned char TXByteCtr;
unsigned char RXByteCtr;
unsigned char Read=0;

unsigned int ADC_Result=0;

unsigned char Strain[2]={0,0};


unsigned char TXData[20];

unsigned char awake=1;
unsigned char rx_index=0;
unsigned char tx_index=0;


unsigned char TxBuffer[4]={0x0,0x0,0x01,0X02};

void Trans_Data(void);


void maincode(void);

void timer(unsigned int t);


void main( void )
{
    WDTCTL = WDTPW + WDTHOLD;




    InitRadio();            //Initialize the radio core


       P2SEL |= BIT0;                            // Enable A/D channel A0

       /* Initialize REF module */
       // Enable 2.5V shared reference, disable temperature sensor to save power
       REFCTL0 |= REFMSTR+REFVSEL_0+REFON+REFTCOFF;

       /* Initialize ADC12 */
       ADC12CTL0 = ADC12ON+ADC12SHT02;           // Turn on ADC12, set sampling time
       ADC12CTL1 = ADC12SHP;                     // Use sampling timer
       ADC12MCTL0 = ADC12SREF_1;                 // Vr+=Vref+ and Vr-=AVss
       ADC12IE = 0x01;                          // ADC_IFG upon conv result-ADCMEMO
       __delay_cycles(75);                       // 75 us delay @ ~1MHz

       ADC12CTL0 |= ADC12ENC;                    // Enable conversions


       __bis_SR_register(GIE); //Enable interrup




while(1)
{
    if(awake==1)        //If awake from low power mode
    {
         maincode();    //Execute measurement and transmission

         awake=0;

    }
         timer(20000); //100 3ms

}
}




void InitRadio(void)
{
  // Set the High-Power Mode Request Enable bit so LPM0 can be entered
  // with active radio enabled 
  PMMCTL0_H = 0xA5;
  PMMCTL0_L |= PMMHPMRE_L; 
  PMMCTL0_H = 0x00; 
  
  WriteRfSettings();
  
  WriteSinglePATable(0xc3);  //50-10dbm  26--12dbm c0 236mv/r 26:148mv  152mv(dvk) c3
}


void Transmit(unsigned char *buffer, unsigned char length)
{

  RF1AIES |= BIT9;                          
  RF1AIFG &= ~BIT9;                         // Clear pending interrupts
  RF1AIE |= BIT9;                           // Enable TX end-of-packet interrupt
  
  WriteBurstReg(RF_TXFIFOWR, buffer, length);     
  
  Strobe( RF_STX );                         // Strobe STX
 // Strobe( RF_STX );
}

void ReceiveOn(void)
{  
  RF1AIES |= BIT9;                          // Falling edge of RFIFG9
  RF1AIFG &= ~BIT9;                         // Clear a pending interrupt
  RF1AIE  |= BIT9;                          // Enable the interrupt 
  
  // Radio is in IDLE following a TX, so strobe SRX to enter Receive Mode
  Strobe( RF_SRX );                      
}

void ReceiveOff(void)
{
  RF1AIE &= ~BIT9;                          // Disable RX interrupts
  RF1AIFG &= ~BIT9;                         // Clear pending IFG

  // It is possible that ReceiveOff is called while radio is receiving a packet.
  // Therefore, it is necessary to flush the RX FIFO after issuing IDLE strobe 
  // such that the RXFIFO is empty prior to receiving a packet.
  Strobe( RF_SIDLE );
  Strobe( RF_SFRX  );                       
}

#pragma vector=CC1101_VECTOR
__interrupt void CC1101_ISR(void)
{
  switch(__even_in_range(RF1AIV,32))        // Prioritizing Radio Core Interrupt 
  {
    case  0: break;                         // No RF core interrupt pending
    case  2: break;                         // RFIFG0
    case  4: break;                         // RFIFG1
    case  6: break;                         // RFIFG2
    case  8: break;                         // RFIFG3
    case 10: break;                         // RFIFG4
    case 12: break;                         // RFIFG5
    case 14: break;                         // RFIFG6
    case 16: break;                         // RFIFG7
    case 18: break;                         // RFIFG8
    case 20:                                // RFIFG9
               RF1AIE &= ~BIT9;                    // Disable TX end-of-packet interrupt

             break;
    case 22: break;                         // RFIFG10
    case 24: break;                         // RFIFG11
    case 26: break;                         // RFIFG12
    case 28: break;                         // RFIFG13
    case 30: break;                         // RFIFG14
    case 32: break;                         // RFIFG15
  }  
  __bic_SR_register_on_exit(LPM3_bits);
}






#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer_A (void)

{
    awake=1;
    TA0CTL=MC_0;
   __bic_SR_register_on_exit(LPM3_bits);

}


void timer(unsigned int t)    //For delay
{
    TA0CTL=MC_0;
    TA0CCTL0=CCIE;
    TA0CCR0=t;
    TA0CTL=TASSEL_1 +MC_1 + TACLR;
    __bis_SR_register(LPM3_bits + GIE);
}




void maincode()
{
    ReceiveOff();




P2OUT |= BIT2;  //BRIDGE
 timer(35);  //100:3ms


ADC12CTL0 |= ADC12SC;                   // Sampling and conversion start



   __bis_SR_register(LPM3_bits + GIE);

P2OUT &= ~BIT2;


Trans_Data();


Transmit(TxBuffer, sizeof TxBuffer);  //Transmit data
__bis_SR_register(LPM3_bits + GIE);





}





void Trans_Data(void)
{


    TxBuffer[0] = (unsigned char) (ADC_Result >> 8) & 0xFF;
    TxBuffer[1] = (unsigned char) ADC_Result & 0xFF;

}


#pragma vector=ADC12_VECTOR
__interrupt void ADC12ISR (void)
{
  switch(__even_in_range(ADC12IV,34))
  {
  case  6:                                  // Vector  6:  ADC12IFG0
      ADC_Result = ADC12MEM0;                       // Move results, IFG is cleared





    __bic_SR_register_on_exit(LPM3_bits);   // Exit active CPU
   // timer_start();
    break;
  default: break;
  }
}








