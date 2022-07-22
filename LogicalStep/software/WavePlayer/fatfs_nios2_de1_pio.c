/****************************************************************************
*  Copyright (C) 2012 by Michael Fischer.
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without 
*  modification, are permitted provided that the following conditions 
*  are met:
*  
*  1. Redistributions of source code must retain the above copyright 
*     notice, this list of conditions and the following disclaimer.
*  2. Redistributions in binary form must reproduce the above copyright
*     notice, this list of conditions and the following disclaimer in the 
*     documentation and/or other materials provided with the distribution.
*  3. Neither the name of the author nor the names of its contributors may 
*     be used to endorse or promote products derived from this software 
*     without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL 
*  THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS 
*  OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED 
*  AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
*  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF 
*  THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
*  SUCH DAMAGE.
*
****************************************************************************
*  History:
*
*  23.08.2012  mifi  First version, tested with an Altera DE1 board.
****************************************************************************/
#define __FATFS_NIOS2_DE1_PIO_C__

#if (FFS_SUPPORT_HW_NIOS2_DE1_PIO >= 1)

/*=========================================================================*/
/*  Includes                                                               */
/*=========================================================================*/
#include <system.h>
#include <io.h>

/*=========================================================================*/
/*  DEFINE: All Structures and Common Constants                            */
/*=========================================================================*/

/*
 * Simple SPI (pio) defines
 */
#define ADDR_DATA       0
#define ADDR_SET        1
#define ADDR_CLR        2

#define SPI_CS_PIN      1
#define SPI_SCK_PIN     2
#define SPI_MOSI_PIN    4
#define SPI_MISO_PIN    1

#define SELECT()        IOWR(PIO_SPI_0_BASE, ADDR_CLR, SPI_CS_PIN)  /* MMC CS = L */
#define DESELECT()      IOWR(PIO_SPI_0_BASE, ADDR_SET, SPI_CS_PIN)  /* MMC CS = H */

#define SCK_HIGH()      IOWR(PIO_SPI_0_BASE, ADDR_SET, SPI_SCK_PIN)
#define SCK_LOW()       IOWR(PIO_SPI_0_BASE, ADDR_CLR, SPI_SCK_PIN)

#define MOSI_HIGH()     IOWR(PIO_SPI_0_BASE, ADDR_SET, SPI_MOSI_PIN)
#define MOSI_LOW()      IOWR(PIO_SPI_0_BASE, ADDR_CLR, SPI_MOSI_PIN)

#define GET_MISO()      IORD(PIO_SPI_0_BASE, ADDR_DATA)

#define NOP()           asm("nop")

#define POWER_ON()
#define POWER_OFF()

/*=========================================================================*/
/*  DEFINE: Prototypes                                                     */
/*=========================================================================*/

/*=========================================================================*/
/*  DEFINE: Definition of all local Data                                   */
/*=========================================================================*/

/*=========================================================================*/
/*  DEFINE: Definition of all local Procedures                             */
/*=========================================================================*/
/***************************************************************************/
/*  SetLowSpeed                                                            */
/*                                                                         */
/*  In    : none                                                           */
/*  Out   : none                                                           */
/*  Return: none                                                           */
/***************************************************************************/
static void SetLowSpeed(void)
{
   /* Do nothing here, spi with pio is slow */
} /* SetLowSpeed */

/***************************************************************************/
/*  SetHighSpeed                                                           */
/*                                                                         */
/*  In    : none                                                           */
/*  Out   : none                                                           */
/*  Return: none                                                           */
/***************************************************************************/
static void SetHighSpeed(void)
{
   /* Do nothing here, spi with pio can not speed up */
} /* SetHighSpeed */

/***************************************************************************/
/*  InitDiskIOHardware                                                     */
/*                                                                         */
/*  Here the diskio interface is initialise, in this case the PIO-SPI.     */
/*                                                                         */
/*  In    : none                                                           */
/*  Out   : none                                                           */
/*  Return: none                                                           */
/***************************************************************************/
static void InitDiskIOHardware(void)
{
   /* Deselct before to prevent glitch */
   DESELECT();
   
   SCK_LOW();
   MOSI_LOW();

   SetLowSpeed();
   
} /* InitDiskIOHardware */

/***************************************************************************/
/*  Here comes some macros to speed up the transfer performance.           */
/*                                                                         */
/*            Be careful if you port this part to an other CPU.            */
/*             !!! This part is high platform dependent. !!!               */
/***************************************************************************/

/*
 * Transmit data only, without to store the receive data.
 * This function will be used normally to send an U8.
 */
#define TRANSMIT_U8(_dat)   if (_dat & 0x80) MOSI_HIGH(); else MOSI_LOW(); \
                            SCK_HIGH(); SCK_LOW();                         \
                            if (_dat & 0x40) MOSI_HIGH(); else MOSI_LOW(); \
                            SCK_HIGH(); SCK_LOW();                         \
                            if (_dat & 0x20) MOSI_HIGH(); else MOSI_LOW(); \
                            SCK_HIGH(); SCK_LOW();                         \
                            if (_dat & 0x10) MOSI_HIGH(); else MOSI_LOW(); \
                            SCK_HIGH(); SCK_LOW();                         \
                            if (_dat & 0x08) MOSI_HIGH(); else MOSI_LOW(); \
                            SCK_HIGH(); SCK_LOW();                         \
                            if (_dat & 0x04) MOSI_HIGH(); else MOSI_LOW(); \
                            SCK_HIGH(); SCK_LOW();                         \
                            if (_dat & 0x02) MOSI_HIGH(); else MOSI_LOW(); \
                            SCK_HIGH(); SCK_LOW();                         \
                            if (_dat & 0x01) MOSI_HIGH(); else MOSI_LOW(); \
                            SCK_HIGH(); SCK_LOW();
   
/*
 * The next function transmit the data "very fast", becasue
 * we do not need to take care of receive data. This function
 * will be used to transmit data in 16 bit mode.
 */
#define TRANSMIT_FAST(_dat) if (_dat & 0x8000) MOSI_HIGH(); else MOSI_LOW(); \
                            SCK_HIGH(); SCK_LOW();                           \
                            if (_dat & 0x4000) MOSI_HIGH(); else MOSI_LOW(); \
                            SCK_HIGH(); SCK_LOW();                           \
                            if (_dat & 0x2000) MOSI_HIGH(); else MOSI_LOW(); \
                            SCK_HIGH(); SCK_LOW();                           \
                            if (_dat & 0x1000) MOSI_HIGH(); else MOSI_LOW(); \
                            SCK_HIGH(); SCK_LOW();                           \
                            if (_dat & 0x0800) MOSI_HIGH(); else MOSI_LOW(); \
                            SCK_HIGH(); SCK_LOW();                           \
                            if (_dat & 0x0400) MOSI_HIGH(); else MOSI_LOW(); \
                            SCK_HIGH(); SCK_LOW();                           \
                            if (_dat & 0x0200) MOSI_HIGH(); else MOSI_LOW(); \
                            SCK_HIGH(); SCK_LOW();                           \
                            if (_dat & 0x0100) MOSI_HIGH(); else MOSI_LOW(); \
                            SCK_HIGH(); SCK_LOW();                           \
                            if (_dat & 0x0080) MOSI_HIGH(); else MOSI_LOW(); \
                            SCK_HIGH(); SCK_LOW();                           \
                            if (_dat & 0x0040) MOSI_HIGH(); else MOSI_LOW(); \
                            SCK_HIGH(); SCK_LOW();                           \
                            if (_dat & 0x0020) MOSI_HIGH(); else MOSI_LOW(); \
                            SCK_HIGH(); SCK_LOW();                           \
                            if (_dat & 0x0010) MOSI_HIGH(); else MOSI_LOW(); \
                            SCK_HIGH(); SCK_LOW();                           \
                            if (_dat & 0x0008) MOSI_HIGH(); else MOSI_LOW(); \
                            SCK_HIGH(); SCK_LOW();                           \
                            if (_dat & 0x0004) MOSI_HIGH(); else MOSI_LOW(); \
                            SCK_HIGH(); SCK_LOW();                           \
                            if (_dat & 0x0002) MOSI_HIGH(); else MOSI_LOW(); \
                            SCK_HIGH(); SCK_LOW();                           \
                            if (_dat & 0x0001) MOSI_HIGH(); else MOSI_LOW(); \
                            SCK_HIGH(); SCK_LOW();

/*
 * RECEIVE_FAST will be used in ReceiveDatablock only.
 */
#define RECEIVE_FAST(_buff,_dest)   _buff = 0;                       \
                                    MOSI_HIGH();                     \
                                    if (GET_MISO()) _buff |= 0x8000; \
                                    SCK_HIGH();                      \
                                    SCK_LOW();                       \
                                    if (GET_MISO()) _buff |= 0x4000; \
                                    SCK_HIGH();                      \
                                    SCK_LOW();                       \
                                    if (GET_MISO()) _buff |= 0x2000; \
                                    SCK_HIGH();                      \
                                    SCK_LOW();                       \
                                    if (GET_MISO()) _buff |= 0x1000; \
                                    SCK_HIGH();                      \
                                    SCK_LOW();                       \
                                    if (GET_MISO()) _buff |= 0x0800; \
                                    SCK_HIGH();                      \
                                    SCK_LOW();                       \
                                    if (GET_MISO()) _buff |= 0x0400; \
                                    SCK_HIGH();                      \
                                    SCK_LOW();                       \
                                    if (GET_MISO()) _buff |= 0x0200; \
                                    SCK_HIGH();                      \
                                    SCK_LOW();                       \
                                    if (GET_MISO()) _buff |= 0x0100; \
                                    SCK_HIGH();                      \
                                    SCK_LOW();                       \
                                    if (GET_MISO()) _buff |= 0x0080; \
                                    SCK_HIGH();                      \
                                    SCK_LOW();                       \
                                    if (GET_MISO()) _buff |= 0x0040; \
                                    SCK_HIGH();                      \
                                    SCK_LOW();                       \
                                    if (GET_MISO()) _buff |= 0x0020; \
                                    SCK_HIGH();                      \
                                    SCK_LOW();                       \
                                    if (GET_MISO()) _buff |= 0x0010; \
                                    SCK_HIGH();                      \
                                    SCK_LOW();                       \
                                    if (GET_MISO()) _buff |= 0x0008; \
                                    SCK_HIGH();                      \
                                    SCK_LOW();                       \
                                    if (GET_MISO()) _buff |= 0x0004; \
                                    SCK_HIGH();                      \
                                    SCK_LOW();                       \
                                    if (GET_MISO()) _buff |= 0x0002; \
                                    SCK_HIGH();                      \
                                    SCK_LOW();                       \
                                    if (GET_MISO()) _buff |= 0x0001; \
                                    SCK_HIGH();                      \
                                    SCK_LOW();                       \
                                    *_dest++ = (_buff >> 8) & 0xFF;  \
                                    *_dest++ = _buff & 0xFF;
                            
/***************************************************************************/
/*  ReceiveU8                                                              */
/*                                                                         */
/*  Send a dummy value to the SPI bus and receive the data.                */
/*                                                                         */
/*  In    : none                                                           */
/*  Out   : none                                                           */
/*  Return: Data                                                           */
/***************************************************************************/
static FFS_U8 ReceiveU8 (void)
{
   FFS_U8 value = 0;
   
   MOSI_HIGH();   /* Dummy value */
   
   if (GET_MISO()) value |= 0x80;
   SCK_HIGH();
   SCK_LOW();

   if (GET_MISO()) value |= 0x40;
   SCK_HIGH();
   SCK_LOW();

   if (GET_MISO()) value |= 0x20;
   SCK_HIGH();
   SCK_LOW();

   if (GET_MISO()) value |= 0x10;
   SCK_HIGH();
   SCK_LOW();

   if (GET_MISO()) value |= 0x08;
   SCK_HIGH();
   SCK_LOW();
   
   if (GET_MISO()) value |= 0x04;
   SCK_HIGH();
   SCK_LOW();

   if (GET_MISO()) value |= 0x02;
   SCK_HIGH();
   SCK_LOW();

   if (GET_MISO()) value |= 0x01;
   SCK_HIGH();
   SCK_LOW();
   
   return(value);
} /* ReceiveU8 */

/***************************************************************************/
/*  ReceiveDatablock                                                       */
/*                                                                         */
/*  Receive a data packet from MMC/SD card. Number of "btr" bytes will be  */
/*  store in the given buffer "buff". The byte count "btr" must be         */
/*  a multiple of 4.                                                       */
/*                                                                         */
/*  In    : buff, btr                                                      */
/*  Out   : none                                                           */
/*  Return: In case of an error return FALSE                               */
/***************************************************************************/
static int ReceiveDatablock(FFS_U8 * buff, uint32_t btr)
{
   FFS_U8 token;
   volatile FFS_U16 value;

   Timer1 = 10;
   do /* Wait for data packet in timeout of 100ms */
   {
      token = ReceiveU8();
   }
   while ((token == 0xFF) && Timer1);

   if (token != 0xFE)
      return(FFS_FALSE);  /* If not valid data token, return with error */
      
   do /* Receive the data block into buffer */
   {
      RECEIVE_FAST(value, buff);
      RECEIVE_FAST(value, buff);
   }
   while (btr -= 4);
   
   ReceiveU8();   /* Discard CRC */
   ReceiveU8();

   return(FFS_TRUE);  /* Return with success */
} /* ReceiveDatablock */

#if (_READONLY == 0)
/***************************************************************************/
/*  TransmitDatablock                                                      */
/*                                                                         */
/*  Send a block of 512 bytes to the MMC/SD card.                          */
/*                                                                         */
/*  In    : buff, token (Data/Stop token)                                  */
/*  Out   : none                                                           */
/*  Return: In case of an error return FALSE                               */
/***************************************************************************/
static int TransmitDatablock(const FFS_U8 * buff, FFS_U8 token)
{
   FFS_U8 resp, wc = 0;
   
   if (WaitReady() != 0xFF)
      return(FFS_FALSE);

   TRANSMIT_U8(token);  /* Xmit data token */
   if (token != 0xFD)   /* Is data token */
   {
      do /* Send the 512 byte data block */
      {
         TRANSMIT_FAST(((*buff << 8) | *(buff + 1)));
         buff += 2;
      }
      while (--wc);

      TRANSMIT_FAST(0xFFFF);  /* CRC (Dummy) */
      
      resp = ReceiveU8();  /* Reveive data response */
      if ((resp & 0x1F) != 0x05) /* If not accepted, return with error */
      {
         return(FFS_FALSE);
      }
   }

   return(FFS_TRUE);  /* Return with success */
} /* TransmitDatablock */
#endif /* _READONLY */

/***************************************************************************/
/*  GetCDWP                                                                */
/*                                                                         */
/*  Return the status of the CD and WP socket pin.                         */
/*                                                                         */
/*  In    : none                                                           */
/*  Out   : none                                                           */
/*  Return: Data                                                           */
/***************************************************************************/
static FFS_U32 GetCDWP(void)
{
   FFS_U32 value = 0;
   
   /*
    * CD and Wp is not supported by the Altera DE1 board.
    */

   return(value);
} /* GetCDWP */

#endif /* (FFS_SUPPORT_HW_NIOS2_DE1_PIO >= 1) */

/*** EOF ***/
