/*******************************************************************************
 *
 *  RF1A.c - Hardware Abstraction Layer APIs for the RF1A radio module 
 *
 *  Copyright (C) 2010 Texas Instruments Incorporated - http://www.ti.com/
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
 ******************************************************************************/
#include "RF1A.h"
#include "cc430x513x.h"

// *****************************************************************************
// @fn          Strobe
// @brief       Send a command strobe to the radio. Includes workaround for RF1A7
// @param       unsigned char strobe        The strobe command to be sent
// @return      unsigned char statusByte    The status byte that follows the strobe
// *****************************************************************************
unsigned char Strobe(unsigned char strobe)
{
  unsigned char statusByte = 0;
  unsigned int  gdo_state;
  
  // Check for valid strobe command 
  if((strobe == 0xBD) || ((strobe >= RF_SRES) && (strobe <= RF_SNOP)))
  {
    // Clear the Status read flag 
    RF1AIFCTL1 &= ~(RFSTATIFG);    
    
    // Wait for radio to be ready for next instruction
    while( !(RF1AIFCTL1 & RFINSTRIFG));
    
    // Write the strobe instruction
    if ((strobe > RF_SRES) && (strobe < RF_SNOP))
    {
      gdo_state = ReadSingleReg(IOCFG2);    // buffer IOCFG2 state
      WriteSingleReg(IOCFG2, 0x29);         // chip-ready to GDO2
      
      RF1AINSTRB = strobe;
      if ( (RF1AIN&0x04)== 0x04 )           // chip at sleep mode    //p1.0À­µÍ
      {
        if ( (strobe == RF_SXOFF) || (strobe == RF_SPWD) || (strobe == RF_SWOR) ) { }
        else  	
        {
          while ((RF1AIN&0x04)== 0x04);     // chip-ready ?
          // Delay for ~810usec at 8MHz CPU clock, see CC430 UG RF1A radio chapter 
          __delay_cycles(710*12);	            
        }
      }
      WriteSingleReg(IOCFG2, gdo_state);    // restore IOCFG2 setting
    
      while( !(RF1AIFCTL1 & RFSTATIFG) );
    }
    else		                    // chip active mode (SRES)
    {	
      RF1AINSTRB = strobe; 	   
    }
    statusByte = RF1ASTATB;
  }
  return statusByte;
}

unsigned char Strobe_NoWait(unsigned char strobe)
{
  unsigned char statusByte = 0;
  unsigned int  gdo_state;
  
  // Check for valid strobe command 
  if((strobe == 0xBD) || ((strobe >= RF_SRES) && (strobe <= RF_SNOP)))
  {
    // Clear the Status read flag 
    RF1AIFCTL1 &= ~(RFSTATIFG);    
    
    // Wait for radio to be ready for next instruction
    while( !(RF1AIFCTL1 & RFINSTRIFG));
    
    // Write the strobe instruction
    if ((strobe > RF_SRES) && (strobe < RF_SNOP))
    {
      gdo_state = ReadSingleReg(IOCFG2);    // buffer IOCFG2 state
      WriteSingleReg(IOCFG2, 0x29);         // chip-ready to GDO2
      
      RF1AINSTRB = strobe; 
      if ( (RF1AIN&0x04)== 0x04 )           // chip at sleep mode
      {
        if ( (strobe == RF_SXOFF) || (strobe == RF_SPWD) || (strobe == RF_SWOR) ) { }
        else  	
        {
          while ((RF1AIN&0x04)== 0x04);     // chip-ready ?
          // Delay for ~810usec at 1.05MHz CPU clock, see erratum RF1A7
         __delay_cycles(850);
        }
      }
      WriteSingleReg(IOCFG2, gdo_state);    // restore IOCFG2 setting
    
      while( !(RF1AIFCTL1 & RFSTATIFG) );
    }
    else		                    // chip active mode (SRES)
    {	
      RF1AINSTRB = strobe; 	   
    }
    statusByte = RF1ASTATB;
  }
  return statusByte;
}

// *****************************************************************************
// @fn          ReadSingleReg
// @brief       Read a single byte from the radio register
// @param       unsigned char addr      Target radio register address
// @return      unsigned char data_out  Value of byte that was read
// *****************************************************************************
unsigned char ReadSingleReg(unsigned char addr)
{
  unsigned char data_out;
  
  // Check for valid configuration register address, 0x3E refers to PATABLE 
  if ((addr <= 0x2E) || (addr == 0x3E))
    // Send address + Instruction + 1 dummy byte (auto-read)
    RF1AINSTR1B = (addr | RF_SNGLREGRD);    
  else
    // Send address + Instruction + 1 dummy byte (auto-read)
    RF1AINSTR1B = (addr | RF_STATREGRD);    
  
  //while (!(RF1AIFCTL1 & RFDOUTIFG) );
  __delay_cycles(100);
  data_out = RF1ADOUT1B;                    // Read data and clears the RFDOUTIFG

  return data_out;
}

// *****************************************************************************
// @fn          WriteSingleReg
// @brief       Write a single byte to a radio register
// @param       unsigned char addr      Target radio register address
// @param       unsigned char value     Value to be written
// @return      none
// *****************************************************************************
void WriteSingleReg(unsigned char addr, unsigned char value)
{   
  while (!(RF1AIFCTL1 & RFINSTRIFG));       // Wait for the Radio to be ready for next instruction
  RF1AINSTRB = (addr | RF_REGWR);	    // Send address + Instruction

  RF1ADINB = value; 			    // Write data in 

  __no_operation(); 
}
        
// *****************************************************************************
// @fn          ReadBurstReg
// @brief       Read multiple bytes to the radio registers
// @param       unsigned char addr      Beginning address of burst read
// @param       unsigned char *buffer   Pointer to data table
// @param       unsigned char count     Number of bytes to be read
// @return      none
// *****************************************************************************
void ReadBurstReg(unsigned char addr, unsigned char *buffer, unsigned char count)
{
  unsigned int i;
  
  while (!(RF1AIFCTL1 & RFINSTRIFG));       // Wait for INSTRIFG
  RF1AINSTR1B = (addr | RF_REGRD);          // Send addr of first conf. reg. to be read 
                                            // ... and the burst-register read instruction
  for (i = 0; i < (count-1); i++)
  {
    while (!(RFDOUTIFG&RF1AIFCTL1));        // Wait for the Radio Core to update the RF1ADOUTB reg
    buffer[i] = RF1ADOUT1B;                 // Read DOUT from Radio Core + clears RFDOUTIFG
                                            // Also initiates auo-read for next DOUT byte
  }
  buffer[count-1] = RF1ADOUT0B;             // Store the last DOUT from Radio Core
}  

// *****************************************************************************
// @fn          WriteBurstReg
// @brief       Write multiple bytes to the radio registers
// @param       unsigned char addr      Beginning address of burst write
// @param       unsigned char *buffer   Pointer to data table
// @param       unsigned char count     Number of bytes to be written
// @return      none
// *****************************************************************************
void WriteBurstReg(unsigned char addr, unsigned char *buffer, unsigned char count)
{  
  unsigned char i;

  while (!(RF1AIFCTL1 & RFINSTRIFG));       // Wait for the Radio to be ready for next instruction
  RF1AINSTRW = ((addr | RF_REGWR)<<8 ) + buffer[0]; // Send address + Instruction

  for (i = 1; i < count; i++)
  {
    RF1ADINB = buffer[i];                   // Send data
    while (!(RFDINIFG & RF1AIFCTL1));       // Wait for TX to finish
  } 
  i = RF1ADOUTB;                            // Reset RFDOUTIFG flag which contains status byte
}

// *****************************************************************************
// @fn          ResetRadioCore
// @brief       Reset the radio core using RF_SRES command
// @param       none
// @return      none
// *****************************************************************************
void ResetRadioCore (void)
{
  Strobe(RF_SRES);                          // Reset the Radio Core
  Strobe(RF_SNOP);                          // Reset Radio Pointer
}

// *****************************************************************************
// @fn          WriteRfSettings
// @brief       Write the minimum set of RF configuration register settings
// @param       RF_SETTINGS *pRfSettings  Pointer to the structure that holds the rf settings
// @return      none
// *****************************************************************************
void WriteRfSettings() {

    //when rate>250kb current optimization bit (MDMCFG2) should be 0;
       //4 bit sync and preameable for rate<500kbps   for 500kbps 8 bits preameable

//

    //500KB
//    WriteSingleReg(FSCTRL1,  0x08);//pRfSettings->fsctrl1);
//       //WriteSingleReg(FSCTRL0,  pRfSettings->fsctrl0);
//       //WriteSingleReg(SYNC1,    pRfSettings->sync1);\
//       //WriteSingleReg(SYNC0,    pRfSettings->sync0);
//       WriteSingleReg(FREQ2, 0x10);//pRfSettings->freq2);
//       WriteSingleReg(FREQ1, 0XA7);//   pRfSettings->freq1);
//       WriteSingleReg(FREQ0, 0X62);//   pRfSettings->freq0);
//       WriteSingleReg(MDMCFG4, 0X0E);// pRfSettings->mdmcfg4);
//       WriteSingleReg(MDMCFG3, 0X3B);// pRfSettings->mdmcfg3);
//       WriteSingleReg(MDMCFG2, 0X73);// pRfSettings->mdmcfg2);
//       WriteSingleReg(MDMCFG1, 0X42);// pRfSettings->mdmcfg1);
//       WriteSingleReg(MDMCFG0, 0XF8); //pRfSettings->mdmcfg0);
//      // WriteSingleReg(CHANNR,   pRfSettings->channr);
//       WriteSingleReg(DEVIATN, 0x0);// pRfSettings->deviatn);
//       WriteSingleReg(FREND1,  0XB6);// pRfSettings->frend1);
//       WriteSingleReg(FREND0,  0X10);// pRfSettings->frend0);
//       WriteSingleReg(MCSM0 ,  0X18);// pRfSettings->mcsm0);
//       WriteSingleReg(FOCCFG,  0X1D);// pRfSettings->foccfg);
//       WriteSingleReg(BSCFG,   0X1C);//pRfSettings->bscfg);
//       WriteSingleReg(AGCCTRL2, 0XC7);//pRfSettings->agcctrl2);
//       WriteSingleReg(AGCCTRL1, 0X00);//pRfSettings->agcctrl1);
//       WriteSingleReg(AGCCTRL0, 0XB0);//pRfSettings->agcctrl0);
//       WriteSingleReg(FSCAL3,   0X10);//pRfSettings->fscal3);
//       WriteSingleReg(FSCAL2,   0X2A);//pRfSettings->fscal2);
//       WriteSingleReg(FSCAL1,  0X00);// pRfSettings->fscal1);
//       WriteSingleReg(FSCAL0,   0X1F);//pRfSettings->fscal0);
//      // WriteSingleReg(FSTEST,   pRfSettings->fstest);
//      // WriteSingleReg(TEST2,    pRfSettings->test2);
//      // WriteSingleReg(TEST1,    pRfSettings->test1);
//       WriteSingleReg(TEST0,  0X09);//  pRfSettings->test0);
//      // WriteSingleReg(FIFOTHR,  pRfSettings->fifothr);
//      // WriteSingleReg(IOCFG2,   pRfSettings->iocfg2);
//       WriteSingleReg(IOCFG0,  0X06); //pRfSettings->iocfg0);
//     //  WriteSingleReg(PKTCTRL1, pRfSettings->pktctrl1);
//       WriteSingleReg(PKTCTRL0, 0X04);//pRfSettings->pktctrl0);
//      // WriteSingleReg(ADDR,     pRfSettings->addr);
//       WriteSingleReg(PKTLEN,  0X08);



//        //500KB kit
//        WriteSingleReg(FSCTRL1,  0xE);//pRfSettings->fsctrl1);
//           //WriteSingleReg(FSCTRL0,  pRfSettings->fsctrl0);
//           //WriteSingleReg(SYNC1,    pRfSettings->sync1);
//           //WriteSingleReg(SYNC0,    pRfSettings->sync0);
//           WriteSingleReg(FREQ2, 0x10);//pRfSettings->freq2);
//           WriteSingleReg(FREQ1, 0XA7);//   pRfSettings->freq1);
//           WriteSingleReg(FREQ0, 0X62);//   pRfSettings->freq0);
//           WriteSingleReg(MDMCFG4, 0X0E);// pRfSettings->mdmcfg4);
//           WriteSingleReg(MDMCFG3, 0X3B);// pRfSettings->mdmcfg3);
//           WriteSingleReg(MDMCFG2, 0X73);// pRfSettings->mdmcfg2);
//           WriteSingleReg(MDMCFG1, 0X42);// pRfSettings->mdmcfg1);
//           WriteSingleReg(MDMCFG0, 0XF8); //pRfSettings->mdmcfg0);
//          // WriteSingleReg(CHANNR,   pRfSettings->channr);
//           WriteSingleReg(DEVIATN, 0x0);// pRfSettings->deviatn);
//           WriteSingleReg(FREND1,  0XB6);// pRfSettings->frend1);
//           WriteSingleReg(FREND0,  0X10);// pRfSettings->frend0);
//           WriteSingleReg(MCSM0 ,  0X18);// pRfSettings->mcsm0);
//           WriteSingleReg(FOCCFG,  0X1D);// pRfSettings->foccfg);
//           WriteSingleReg(BSCFG,   0X1C);//pRfSettings->bscfg);
//           WriteSingleReg(AGCCTRL2, 0XC7);//pRfSettings->agcctrl2);
//           WriteSingleReg(AGCCTRL1, 0X00);//pRfSettings->agcctrl1);
//           WriteSingleReg(AGCCTRL0, 0XB0);//pRfSettings->agcctrl0);
//           WriteSingleReg(FSCAL3,   0X10);//pRfSettings->fscal3);
//           WriteSingleReg(FSCAL2,   0X2A);//pRfSettings->fscal2);
//           WriteSingleReg(FSCAL1,  0X00);// pRfSettings->fscal1);
//           WriteSingleReg(FSCAL0,   0X1F);//pRfSettings->fscal0);
//          // WriteSingleReg(FSTEST,   pRfSettings->fstest);
//          // WriteSingleReg(TEST2,    pRfSettings->test2);
//          // WriteSingleReg(TEST1,    pRfSettings->test1);
//           WriteSingleReg(TEST0,  0X09);//  pRfSettings->test0);
//          // WriteSingleReg(FIFOTHR,  pRfSettings->fifothr);
//          // WriteSingleReg(IOCFG2,   pRfSettings->iocfg2);
//           WriteSingleReg(IOCFG0,  0X06); //pRfSettings->iocfg0);
//         //  WriteSingleReg(PKTCTRL1, pRfSettings->pktctrl1);
//           WriteSingleReg(PKTCTRL0, 0X04);//pRfSettings->pktctrl0);
//          // WriteSingleReg(ADDR,     pRfSettings->addr);
//           WriteSingleReg(PKTLEN,  0X08);





    //250KB
//    WriteSingleReg(FSCTRL1,  0x0C);//pRfSettings->fsctrl1);
//    //WriteSingleReg(FSCTRL0,  pRfSettings->fsctrl0);
//    //WriteSingleReg(SYNC1,    pRfSettings->sync1);
//    //WriteSingleReg(SYNC0,    pRfSettings->sync0);
//    WriteSingleReg(FREQ2, 0x10);//pRfSettings->freq2);
//    WriteSingleReg(FREQ1, 0XA7);//   pRfSettings->freq1);
//    WriteSingleReg(FREQ0, 0X62);//   pRfSettings->freq0);
//    WriteSingleReg(MDMCFG4, 0X2D);// pRfSettings->mdmcfg4);
//    WriteSingleReg(MDMCFG3, 0X3B);// pRfSettings->mdmcfg3);
//    WriteSingleReg(MDMCFG2, 0X13);// pRfSettings->mdmcfg2);
//    WriteSingleReg(MDMCFG1, 0X22);// pRfSettings->mdmcfg1);
//    WriteSingleReg(MDMCFG0, 0XF8); //pRfSettings->mdmcfg0);
//   // WriteSingleReg(CHANNR,   pRfSettings->channr);
//    WriteSingleReg(DEVIATN, 0x62);// pRfSettings->deviatn);
//    WriteSingleReg(FREND1,  0XB6);// pRfSettings->frend1);
//    WriteSingleReg(FREND0,  0X10);// pRfSettings->frend0);
//    WriteSingleReg(MCSM0 ,  0X10);// pRfSettings->mcsm0);
//    WriteSingleReg(FOCCFG,  0X1D);// pRfSettings->foccfg);
//    WriteSingleReg(BSCFG,   0X1C);//pRfSettings->bscfg);
//    WriteSingleReg(AGCCTRL2, 0XC7);//pRfSettings->agcctrl2);
//    WriteSingleReg(AGCCTRL1, 0X00);//pRfSettings->agcctrl1);
//    WriteSingleReg(AGCCTRL0, 0XB0);//pRfSettings->agcctrl0);
//    WriteSingleReg(FSCAL3,   0XEA);//pRfSettings->fscal3);
//    WriteSingleReg(FSCAL2,   0X2A);//pRfSettings->fscal2);
//    WriteSingleReg(FSCAL1,  0X00);// pRfSettings->fscal1);
//    WriteSingleReg(FSCAL0,   0X1F);//pRfSettings->fscal0);
//   // WriteSingleReg(FSTEST,   pRfSettings->fstest);
//   // WriteSingleReg(TEST2,    pRfSettings->test2);
//   // WriteSingleReg(TEST1,    pRfSettings->test1);
//    WriteSingleReg(TEST0,  0X09);//  pRfSettings->test0);
//   // WriteSingleReg(FIFOTHR,  pRfSettings->fifothr);
//   // WriteSingleReg(IOCFG2,   pRfSettings->iocfg2);
//    WriteSingleReg(IOCFG0,  0X06); //pRfSettings->iocfg0);
//  //  WriteSingleReg(PKTCTRL1, pRfSettings->pktctrl1);
//    WriteSingleReg(PKTCTRL0, 0X04);//pRfSettings->pktctrl0);
//   // WriteSingleReg(ADDR,     pRfSettings->addr);
//    WriteSingleReg(PKTLEN,  0X08);


//
//    //38.4 kb
//         //
//            // Rf settings for CC430
//            //
//            WriteSingleReg(IOCFG2,0x29);  //GDO2 Output Configuration
//            //WriteSingleReg(IOCFG1,0x2E);  //GDO1 Output Configuration
//            WriteSingleReg(IOCFG0,0x06);  //GDO0 Output Configuration
//            WriteSingleReg(FIFOTHR,0x47); //RX FIFO and TX FIFO Thresholds
//            WriteSingleReg(PKTLEN,0x08);  //Packet Length
//            WriteSingleReg(PKTCTRL1,0x04);//Packet Automation Control
//            WriteSingleReg(PKTCTRL0,0x04);//Packet Automation Control
//            WriteSingleReg(ADDR,0x00);    //Device Address
//            WriteSingleReg(CHANNR,0x00);  //Channel Number
//            WriteSingleReg(FSCTRL1,0x08); //Frequency Synthesizer Control
//            WriteSingleReg(FSCTRL0,0x00); //Frequency Synthesizer Control
//            WriteSingleReg(FREQ2,0x10);   //Frequency Control Word, High Byte
//            WriteSingleReg(FREQ1,0xA7);   //Frequency Control Word, Middle Byte
//            WriteSingleReg(FREQ0,0x62);   //Frequency Control Word, Low Byte
//            WriteSingleReg(MDMCFG4,0xFA); //Modem Configuration   FA. 38.4kb   D5 1.2k
//            WriteSingleReg(MDMCFG3,0x83); //Modem Configuration
//            WriteSingleReg(MDMCFG2,0x91); //Modem Configuration  //
//            WriteSingleReg(MDMCFG1,0x02); //Modem Configuration  // 02
//            WriteSingleReg(MDMCFG0,0xF8); //Modem Configuration  //F8
//            WriteSingleReg(DEVIATN,0x34); //Modem Deviation Setting
//            WriteSingleReg(MCSM0,0x18);   //Main Radio Control State Machine Configuration  // 18
//            WriteSingleReg(FOCCFG,0x16);  //Frequency Offset Compensation Configuration
//            WriteSingleReg(BSCFG,0x6C);   //Bit Synchronization Configuration
//            WriteSingleReg(AGCCTRL2,0x43);//AGC Control
//            WriteSingleReg(AGCCTRL1,0x40);//AGC Control
//            WriteSingleReg(AGCCTRL0,0x91);//AGC Control
//            WriteSingleReg(WORCTRL,0xFB); //Wake On Radio Control
//            WriteSingleReg(FREND1,0x56);  //Front End RX Configuration
//            WriteSingleReg(FREND0,0x10);  //Front End TX Configuration
//            WriteSingleReg(FSCAL3,0xE9);  //Frequency Synthesizer Calibration
//            WriteSingleReg(FSCAL2,0x2A);  //Frequency Synthesizer Calibration
//            WriteSingleReg(FSCAL1,0x00);  //Frequency Synthesizer Calibration
//            WriteSingleReg(FSCAL0,0x1F);  //Frequency Synthesizer Calibration
//            WriteSingleReg(TEST2,0x81);   //Various Test Settings
//            WriteSingleReg(TEST1,0x35);   //Various Test Settings
//            WriteSingleReg(TEST0,0x09);   //Various Test Settings
//            WriteSingleReg(FSTEST,0x59);
//     //ftest






        //38.4 kb for kit
             //
                // Rf settings for CC430
                //
                WriteSingleReg(IOCFG2,0x29);  //GDO2 Output Configuration
                //WriteSingleReg(IOCFG1,0x2E);  //GDO1 Output Configuration
                WriteSingleReg(IOCFG0,0x06);  //GDO0 Output Configuration
                WriteSingleReg(FIFOTHR,0x47); //RX FIFO and TX FIFO Thresholds
                WriteSingleReg(PKTLEN,0x04);  //Packet Length
                WriteSingleReg(PKTCTRL1,0x04);//Packet Automation Control
                WriteSingleReg(PKTCTRL0,0x04);//Packet Automation Control
                WriteSingleReg(ADDR,0x00);    //Device Address
                WriteSingleReg(CHANNR,0x00);  //Channel Number
                WriteSingleReg(FSCTRL1,0x08); //Frequency Synthesizer Control 08 0E
                //WriteSingleReg(FSCTRL0,0x00); //Frequency Synthesizer Control
                WriteSingleReg(FREQ2,0x10);   //Frequency Control Word, High Byte
                WriteSingleReg(FREQ1,0xA7);   //Frequency Control Word, Middle Byte
                WriteSingleReg(FREQ0,0x62);   //Frequency Control Word, Low Byte
                WriteSingleReg(MDMCFG4,0xFA); //Modem Configuration   FA. 38.4kb   D5 1.2k
                WriteSingleReg(MDMCFG3,0x83); //Modem Configuration
                WriteSingleReg(MDMCFG2,0x93); //Modem Configuration  //
                WriteSingleReg(MDMCFG1,0x22); //Modem Configuration  // 02  channel spacing
                WriteSingleReg(MDMCFG0,0xF8); //Modem Configuration  //F8
                WriteSingleReg(DEVIATN,0x34); //Modem Deviation Setting
                WriteSingleReg(MCSM0,0x18);   //Main Radio Control State Machine Configuration  // 18  10
                WriteSingleReg(FOCCFG,0x1D);  //Frequency Offset Compensation Configuration  1D:OFFSET=BW/8   16:BW/4
                WriteSingleReg(BSCFG,0x1C);   //Bit Synchronization Configuration
                WriteSingleReg(AGCCTRL2,0xC7);//AGC Control
                WriteSingleReg(AGCCTRL1,0x00);//AGC Control
                WriteSingleReg(AGCCTRL0,0xB0);//AGC Control
                //WriteSingleReg(WORCTRL,0xFB); //Wake On Radio Control
                WriteSingleReg(FREND1,0xB6);  //Front End RX Configuration
                WriteSingleReg(FREND0,0x10);  //Front End TX Configuration
                WriteSingleReg(FSCAL3,0x10);  //Frequency Synthesizer Calibration
                WriteSingleReg(FSCAL2,0x2A);  //Frequency Synthesizer Calibration
                WriteSingleReg(FSCAL1,0x00);  //Frequency Synthesizer Calibration
                WriteSingleReg(FSCAL0,0x1F);  //Frequency Synthesizer Calibration
                //WriteSingleReg(TEST2,0x81);   //Various Test Settings
                //WriteSingleReg(TEST1,0x35);   //Various Test Settings
                WriteSingleReg(TEST0,0x09);   //Various Test Settings
                //WriteSingleReg(FSTEST,0x59);






//
//            // 38kb kit -2
////            // Rf settings for CC430
////            //
//       WriteSingleReg(FSCTRL1,  0x08);//pRfSettings->fsctrl1);
//             //WriteSingleReg(FSCTRL0,  pRfSettings->fsctrl0);
//             //WriteSingleReg(SYNC1,    pRfSettings->sync1);\
//             //WriteSingleReg(SYNC0,    pRfSettings->sync0);
//             WriteSingleReg(FREQ2, 0x10);//pRfSettings->freq2);
//             WriteSingleReg(FREQ1, 0XA7);//   pRfSettings->freq1);
//             WriteSingleReg(FREQ0, 0X62);//   pRfSettings->freq0);
//             WriteSingleReg(MDMCFG4, 0XCA);// pRfSettings->mdmcfg4);
//             WriteSingleReg(MDMCFG3, 0X83);// pRfSettings->mdmcfg3);
//             WriteSingleReg(MDMCFG2, 0X73);// pRfSettings->mdmcfg2);
//             WriteSingleReg(MDMCFG1, 0X22);// pRfSettings->mdmcfg1);
//             WriteSingleReg(MDMCFG0, 0XF8); //pRfSettings->mdmcfg0);
//            // WriteSingleReg(CHANNR,   pRfSettings->channr);
//             WriteSingleReg(DEVIATN, 0x0);// pRfSettings->deviatn);
//             WriteSingleReg(FREND1,  0XB6);// pRfSettings->frend1);
//             WriteSingleReg(FREND0,  0X10);// pRfSettings->frend0);
//             WriteSingleReg(MCSM0 ,  0X18);// pRfSettings->mcsm0);
//             WriteSingleReg(FOCCFG,  0X1D);// pRfSettings->foccfg);
//             WriteSingleReg(BSCFG,   0X1C);//pRfSettings->bscfg);
//             WriteSingleReg(AGCCTRL2, 0XC7);//pRfSettings->agcctrl2);
//             WriteSingleReg(AGCCTRL1, 0X00);//pRfSettings->agcctrl1);
//             WriteSingleReg(AGCCTRL0, 0XB0);//pRfSettings->agcctrl0);
//             WriteSingleReg(FSCAL3,   0X10);//pRfSettings->fscal3);
//             WriteSingleReg(FSCAL2,   0X2A);//pRfSettings->fscal2);
//             WriteSingleReg(FSCAL1,  0X00);// pRfSettings->fscal1);
//             WriteSingleReg(FSCAL0,   0X1F);//pRfSettings->fscal0);
//            // WriteSingleReg(FSTEST,   pRfSettings->fstest);
//            // WriteSingleReg(TEST2,    pRfSettings->test2);
//            // WriteSingleReg(TEST1,    pRfSettings->test1);
//             WriteSingleReg(TEST0,  0X09);//  pRfSettings->test0);
//            // WriteSingleReg(FIFOTHR,  pRfSettings->fifothr);
//            // WriteSingleReg(IOCFG2,   pRfSettings->iocfg2);
//             WriteSingleReg(IOCFG0,  0X06); //pRfSettings->iocfg0);
//           //  WriteSingleReg(PKTCTRL1, pRfSettings->pktctrl1);
//             WriteSingleReg(PKTCTRL0, 0X04);//pRfSettings->pktctrl0);
//            // WriteSingleReg(ADDR,     pRfSettings->addr);
//             WriteSingleReg(PKTLEN,  0X08);
//


//       //250k
//       //
//          // Rf settings for CC430
//          //
//          WriteSingleReg(IOCFG2,0x29);  //GDO2 Output Configuration
//          //WriteSingleReg(IOCFG1,0x2E);  //GDO1 Output Configuration
//          WriteSingleReg(IOCFG0,0x06);  //GDO0 Output Configuration
//          WriteSingleReg(FIFOTHR,0x47); //RX FIFO and TX FIFO Thresholds
//          WriteSingleReg(PKTLEN,0x08);  //Packet Length
//          WriteSingleReg(PKTCTRL1,0x04);//Packet Automation Control
//          WriteSingleReg(PKTCTRL0,0x04);//Packet Automation Control
//          WriteSingleReg(ADDR,0x00);    //Device Address
//          WriteSingleReg(CHANNR,0x00);  //Channel Number
//          WriteSingleReg(FSCTRL1,0x08); //Frequency Synthesizer Control
//          WriteSingleReg(FSCTRL0,0x00); //Frequency Synthesizer Control
//          WriteSingleReg(FREQ2,0x10);   //Frequency Control Word, High Byte
//          WriteSingleReg(FREQ1,0xA7);   //Frequency Control Word, Middle Byte
//          WriteSingleReg(FREQ0,0x62);   //Frequency Control Word, Low Byte
//          WriteSingleReg(MDMCFG4,0x0E); //Modem Configuration   FA. 38.4kb   D5 1.2k   0E
//          WriteSingleReg(MDMCFG3,0x3B); //Modem Configuration
//          WriteSingleReg(MDMCFG2,0x13); //Modem Configuration  //
//          WriteSingleReg(MDMCFG1,0x02); //Modem Configuration  // 02
//          WriteSingleReg(MDMCFG0,0xF8); //Modem Configuration  //F8
//          WriteSingleReg(DEVIATN,0x34); //Modem Deviation Setting
//          WriteSingleReg(MCSM0,0x18);   //Main Radio Control State Machine Configuration  // 18
//          WriteSingleReg(FOCCFG,0x16);  //Frequency Offset Compensation Configuration
//          WriteSingleReg(BSCFG,0x6C);   //Bit Synchronization Configuration
//          WriteSingleReg(AGCCTRL2,0x43);//AGC Control
//          WriteSingleReg(AGCCTRL1,0x40);//AGC Control
//          WriteSingleReg(AGCCTRL0,0x91);//AGC Control
//          WriteSingleReg(WORCTRL,0xFB); //Wake On Radio Control
//          WriteSingleReg(FREND1,0x56);  //Front End RX Configuration
//          WriteSingleReg(FREND0,0x10);  //Front End TX Configuration
//          WriteSingleReg(FSCAL3,0xE9);  //Frequency Synthesizer Calibration
//          WriteSingleReg(FSCAL2,0x2A);  //Frequency Synthesizer Calibration
//          WriteSingleReg(FSCAL1,0x00);  //Frequency Synthesizer Calibration
//          WriteSingleReg(FSCAL0,0x1F);  //Frequency Synthesizer Calibration
//          WriteSingleReg(TEST2,0x81);   //Various Test Settings
//          WriteSingleReg(TEST1,0x35);   //Various Test Settings
//          WriteSingleReg(TEST0,0x09);   //Various Test Settings
//          WriteSingleReg(FSTEST,0x59);
//   //ftest

}

// *****************************************************************************
// @fn          WritePATable
// @brief       Write data to power table
// @param       unsigned char value		Value to write
// @return      none
// *****************************************************************************
void WriteSinglePATable(unsigned char value)
{
  while( !(RF1AIFCTL1 & RFINSTRIFG));
  RF1AINSTRW = 0x3E00 + value;              // PA Table single write
  
  while( !(RF1AIFCTL1 & RFINSTRIFG));
  RF1AINSTRB = RF_SNOP;                     // reset PA_Table pointer
}

// *****************************************************************************
// @fn          WritePATable
// @brief       Write to multiple locations in power table 
// @param       unsigned char *buffer	Pointer to the table of values to be written 
// @param       unsigned char count	Number of values to be written
// @return      none
// *****************************************************************************
void WriteBurstPATable(unsigned char *buffer, unsigned char count)
{
  volatile char i = 0; 
  
  while( !(RF1AIFCTL1 & RFINSTRIFG));
  RF1AINSTRW = 0x7E00 + buffer[i];          // PA Table burst write   

  for (i = 1; i < count; i++)
  {
    RF1ADINB = buffer[i];                   // Send data
    while (!(RFDINIFG & RF1AIFCTL1));       // Wait for TX to finish
  } 
  i = RF1ADOUTB;                            // Reset RFDOUTIFG flag which contains status byte

  while( !(RF1AIFCTL1 & RFINSTRIFG));
  RF1AINSTRB = RF_SNOP;                     // reset PA Table pointer
}
