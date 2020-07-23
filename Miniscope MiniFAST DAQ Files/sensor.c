/*
 ## Cypress FX3 Camera Kit source file (sensor.c)
 ## ===========================
 ##
 ##  Copyright Cypress Semiconductor Corporation, 2010-2012,
 ##  All Rights Reserved
 ##  UNPUBLISHED, LICENSED SOFTWARE.
 ##
 ##  CONFIDENTIAL AND PROPRIETARY INFORMATION
 ##  WHICH IS THE PROPERTY OF CYPRESS.
 ##
 ##  Use of this file is governed
 ##  by the license agreement included in the file
 ##
 ##     <install>/license/license.txt
 ##
 ##  where <install> is the Cypress software
 ##  installation root directory path.
 ##
 ## ===========================
*/

/* This file implements the I2C based driver for the IMX290 image sensor used
   in the Enhanced Miniscope Image Sensor Module R1

 */

#include <cyu3system.h>
#include <cyu3os.h>
#include <cyu3dma.h>
#include <cyu3error.h>
#include <cyu3uart.h>
#include <cyu3i2c.h>
#include <cyu3spi.h> //Added by Daniel 4_9_2015
#include <cyu3types.h>
#include <cyu3gpio.h>
#include <cyu3utils.h>
#include "sensor.h"

int scaleFactorFPS;

/* This function inserts a delay between successful I2C transfers to prevent
   false errors due to the slave being busy.
 */
static void
SensorI2CAccessDelay (
        CyU3PReturnStatus_t status)
{
    /* Add a 10us delay if the I2C operation that preceded this call was successful. */
    if (status == CY_U3P_SUCCESS)
        CyU3PBusyWait (200); //in us
}
//Added Jill 6/4/18 - IMX290 uses 2 byte address and one byte data
/* Write to an I2C slave with one byte of data. */
CyU3PReturnStatus_t
SensorWrite2 (
        uint8_t slaveAddr,
        uint8_t highAddr,
        uint8_t lowAddr,
        uint8_t Data)
{
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
    CyU3PI2cPreamble_t  preamble;
    uint8_t buf[1];

    /* Validate the I2C slave address. */
    if ((slaveAddr != SENSOR_ADDR_WR) && (slaveAddr != I2C_MEMORY_ADDR_WR))
    {
        CyU3PDebugPrint (4, "I2C Slave address is not valid!\n");
        return 1;
    }

    /* Set the parameters for the I2C API access and then call the write API. */
    preamble.buffer[0] = slaveAddr;
    preamble.buffer[1] = highAddr;
    preamble.buffer[2] = lowAddr;
    preamble.length    = 3;             /*  Three byte preamble. */
    preamble.ctrlMask  = 0x0000;        /*  No additional start and stop bits. */

    buf[0] = Data;
  

    apiRetStatus = CyU3PI2cTransmitBytes (&preamble, buf, 1, 2);
    SensorI2CAccessDelay (apiRetStatus);

    return apiRetStatus;
}

/* Write to an I2C slave with two bytes of data. */
CyU3PReturnStatus_t
SensorWrite2B (
        uint8_t slaveAddr,
        uint8_t highAddr,
        uint8_t lowAddr,
        uint8_t highData,
        uint8_t lowData)
{
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
    CyU3PI2cPreamble_t  preamble;
    uint8_t buf[2];

    /* Validate the I2C slave address. */
    if ((slaveAddr != SENSOR_ADDR_WR) && (slaveAddr != I2C_MEMORY_ADDR_WR))
    {
        CyU3PDebugPrint (4, "I2C Slave address is not valid!\n");
        return 1;
    }

    /* Set the parameters for the I2C API access and then call the write API. */
    preamble.buffer[0] = slaveAddr;
    preamble.buffer[1] = highAddr;
    preamble.buffer[2] = lowAddr;
    preamble.length    = 3;             /*  Three byte preamble. */
    preamble.ctrlMask  = 0x0000;        /*  No additional start and stop bits. */

    buf[0] = highData;
    buf[1] = lowData;

    apiRetStatus = CyU3PI2cTransmitBytes (&preamble, buf, 2, 0);
    SensorI2CAccessDelay (apiRetStatus);

    return apiRetStatus;
}

CyU3PReturnStatus_t
SensorWrite (
        uint8_t slaveAddr,
        uint8_t Addr,
        uint8_t count,
        uint8_t *buf)
{
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
    CyU3PI2cPreamble_t preamble;

    /* Validate the I2C slave address. */
    if ((slaveAddr != SENSOR_ADDR_WR) && (slaveAddr != I2C_MEMORY_ADDR_WR))
    {
        CyU3PDebugPrint (4, "I2C Slave address is not valid!\n");
     //   return 1;
    }

    if (count > 64)
    {
        CyU3PDebugPrint (4, "ERROR: SensorWrite count > 64\n");
        return 1;
    }

    /* Set up the I2C control parameters and invoke the write API. */
    preamble.buffer[0] = slaveAddr;
    preamble.buffer[1] = Addr;
    preamble.length    = 2;
    preamble.ctrlMask  = 0x0000;

    apiRetStatus = CyU3PI2cTransmitBytes (&preamble, buf, count, 0);
    SensorI2CAccessDelay (apiRetStatus);

    return apiRetStatus;
}
//Added by Jill 6/4/18 - IMX290 Uses a 2 byte address and one byte of data
CyU3PReturnStatus_t
SensorRead2 (
        uint8_t slaveAddr,
        uint8_t highAddr,
        uint8_t lowAddr,
        uint8_t *buf)
{
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
    CyU3PI2cPreamble_t preamble;

    if ((slaveAddr != SENSOR_ADDR_RD) && (slaveAddr != I2C_MEMORY_ADDR_RD))
    {
        CyU3PDebugPrint (4, "I2C Slave address is not valid!\n");
    //    return 1;
    }

    preamble.buffer[0] = slaveAddr  & I2C_SLAVEADDR_MASK;;
    preamble.buffer[1] = highAddr;
    preamble.buffer[2] = lowAddr;
    preamble.buffer[3] = slaveAddr;
    preamble.length    = 4;
    preamble.ctrlMask  = 0x0004;                                /*  Send start bit after third byte of preamble. */

    apiRetStatus = CyU3PI2cReceiveBytes (&preamble, buf, 1, 0);
    SensorI2CAccessDelay (apiRetStatus);

    return apiRetStatus;
}



CyU3PReturnStatus_t
SensorRead2B (
        uint8_t slaveAddr,
        uint8_t highAddr,
        uint8_t lowAddr,
        uint8_t *buf)
{
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
    CyU3PI2cPreamble_t preamble;

    if ((slaveAddr != SENSOR_ADDR_RD) && (slaveAddr != I2C_MEMORY_ADDR_RD))
    {
        CyU3PDebugPrint (4, "I2C Slave address is not valid!\n");
    //    return 1;
    }

    preamble.buffer[0] = slaveAddr & I2C_SLAVEADDR_MASK;        /*  Mask out the transfer type bit. */
    preamble.buffer[1] = highAddr;
    preamble.buffer[2] = lowAddr;
    preamble.buffer[3] = slaveAddr;
    preamble.length    = 4;
    preamble.ctrlMask  = 0x0004;                                /*  Send start bit after third byte of preamble. */

    apiRetStatus = CyU3PI2cReceiveBytes (&preamble, buf, 2, 0);
    SensorI2CAccessDelay (apiRetStatus);

    return apiRetStatus;
}

CyU3PReturnStatus_t
SensorRead (
        uint8_t slaveAddr,
        uint8_t Addr,
        uint8_t count,
        uint8_t *buf)
{
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
    CyU3PI2cPreamble_t preamble;

    /* Validate the parameters. */
    if ((slaveAddr != SENSOR_ADDR_RD) && (slaveAddr != I2C_MEMORY_ADDR_RD))
    {
        CyU3PDebugPrint (4, "I2C Slave address is not valid!\n");
    //    return 1;
    }
    if ( count > 64 )
    {
        CyU3PDebugPrint (4, "ERROR: SensorWrite count > 64\n");
        return 1;
    }

    preamble.buffer[0] = slaveAddr & I2C_SLAVEADDR_MASK;        /*  Mask out the transfer type bit. */
    preamble.buffer[1] = Addr;
    preamble.buffer[2] = slaveAddr;
    preamble.length    = 3;
    preamble.ctrlMask  = 0x0002;                                /*  Send start bit after third byte of preamble. */

    apiRetStatus = CyU3PI2cReceiveBytes (&preamble, buf, count, 0);
    SensorI2CAccessDelay (apiRetStatus);

    return apiRetStatus;
}



void
SensorInit (
        void)
{
	uint8_t buf[2];
	CyU3PReturnStatus_t status;
	CyBool_t pinState;

//Added by Jill 6/4/18

buf[0] = SENSOR_ADDR_WR; //sets allowable i2c addresses to send through serializer
SensorWrite (DESER_ADDR_WR, 0x08, 1, buf);
SensorWrite (DESER_ADDR_WR, 0x10, 1, buf);

buf[0] = DAC_ADDR_WR; //sets allowable i2c addresses to send through serializer
SensorWrite (DESER_ADDR_WR, 0x09, 1, buf);
SensorWrite (DESER_ADDR_WR, 0x11, 1, buf);

//For IMX290 Init - Jill
/*
//For 1080p  - Uses all the default values for everything VMAX 0x4EE for fastest 27fps. Make larger for slower values
SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x07,0x00);				//VReverse, HReverse = Normal, Window Cropping
SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x09,0x02);				//FRSEL 02h
SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x18,0xEE);				//VMAX = 04EEh  or for 30fps Registers 18,19 and 1A
SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x19,0x08);				//
SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x1A,0x00);				//
*/

//For Window cropping
SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x09,0x02);				//FRSEL = 02h
SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x07,0x40);				//VReverse, HReverse = Normal, Window Cropping

SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x18,0xDC);				//VMAX = 148h Registers 18,19 and 1A
SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x19,0x05);				//
SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x1A,0x00);				//

SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x1C,0x30);				//HMAX = 1130h Registers 1C,1D
SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x1D,0x11);				//

SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x3A,0x07);				//WINWV_OB = 0x07

SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x3E,WINWV_LOW);		//WINWV
SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x3F,WINWV_HIGH);		//
SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x42,WINWH_LOW);		//WINWH
SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x43,WINWH_HIGH);		//

SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x40,0x00);				//WINPH
SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x41,0x00);				//
SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x3C,0x00);				//WINPV
SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x3D,0x00);				//

SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x14, 0x00);			//Gain

SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x5C,0x0C);             //INCK Sel = 74.25MHz
SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x5D,0x00);
SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x5E,0x10);				//
SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x5F,0x01);				//
SensorWrite2 (SENSOR_ADDR_WR, 0x31, 0x5E,0x1B);				//
SensorWrite2 (SENSOR_ADDR_WR, 0x31, 0x64,0x1B);				//
SensorWrite2 (SENSOR_ADDR_WR, 0x34, 0x80,0x92);				//

SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x0A,0x00);				//Black Level Offset set to recommended value
SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x0B,0x00);				//Black Level Offset set to recommended value

/*
SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x0A,0x00);				//Black Level Offset set to PG Mode

SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x8C,0x51);				//PG Mode 1
SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x0E,0x01);				//Black Level Offset set to PG Mode
SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x0F,0x01);				//Black Level Offset set to PG Mode
SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x94,0xFE);				//PG Mode 1
SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x95,0x0F);				//PG Mode 1

SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x92,0xAA);				//PG Mode 1
SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x93,0x0A);				//PG Mode 1
*/

SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x4B,0x00);				//Sets XVS and XHS to high
SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x4A,0xC2);				//Sets output to DCK Sync

// Below is required by data sheet without explanation
SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x0F,0x00);				//Required
SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x10,0x21);				//Required
SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x12,0x64);				//Required
SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x13,0x00);				//Required
SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x16,0x09);				//Required

SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x70,0x02);				//Required
SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x71,0x11);				//Required
SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x9B,0x10);				//Required
SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x9C,0x22);				//Required
SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0xA2,0x02);				//Required
SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0xA6,0x20);				//Required
SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0xA8,0x20);				//Required
SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0xAA,0x20);				//Required
SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0xAC,0x20);				//Required
SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0xB0,0x43);				//Required
SensorWrite2 (SENSOR_ADDR_WR, 0x31, 0x19,0x9E);				//Required
SensorWrite2 (SENSOR_ADDR_WR, 0x31, 0x1C,0x1E);				//Required
SensorWrite2 (SENSOR_ADDR_WR, 0x31, 0x1E,0x08);				//Required
SensorWrite2 (SENSOR_ADDR_WR, 0x31, 0x28,0x05);				//Required
SensorWrite2 (SENSOR_ADDR_WR, 0x31, 0x3D,0x83);				//Required
SensorWrite2 (SENSOR_ADDR_WR, 0x31, 0x50,0x03);				//Required
SensorWrite2 (SENSOR_ADDR_WR, 0x31, 0x7E,0x00);				//Required
SensorWrite2 (SENSOR_ADDR_WR, 0x32, 0xB8,0x50);				//Required
SensorWrite2 (SENSOR_ADDR_WR, 0x32, 0xB9,0x10);				//Required
SensorWrite2 (SENSOR_ADDR_WR, 0x32, 0xBA,0x00);				//Required
SensorWrite2 (SENSOR_ADDR_WR, 0x32, 0xBB,0x04);				//Required
SensorWrite2 (SENSOR_ADDR_WR, 0x32, 0xC8,0x50);				//Required
SensorWrite2 (SENSOR_ADDR_WR, 0x32, 0xC9,0x10);				//Required
SensorWrite2 (SENSOR_ADDR_WR, 0x32, 0xCA,0x00);				//Required
SensorWrite2 (SENSOR_ADDR_WR, 0x32, 0xCB,0x04);				//Required
SensorWrite2 (SENSOR_ADDR_WR, 0x33, 0x2C,0xD3);				//Required
SensorWrite2 (SENSOR_ADDR_WR, 0x33, 0x2D,0x10);				//Required
SensorWrite2 (SENSOR_ADDR_WR, 0x33, 0x2E,0x0D);				//Required
SensorWrite2 (SENSOR_ADDR_WR, 0x33, 0x58,0x06);				//Required
SensorWrite2 (SENSOR_ADDR_WR, 0x33, 0x59,0xE1);				//Required
SensorWrite2 (SENSOR_ADDR_WR, 0x33, 0x5A,0x11);				//Required
SensorWrite2 (SENSOR_ADDR_WR, 0x33, 0x60,0x1E);				//Required
SensorWrite2 (SENSOR_ADDR_WR, 0x33, 0x61,0x61);				//Required
SensorWrite2 (SENSOR_ADDR_WR, 0x33, 0x62,0x10);				//Required
SensorWrite2 (SENSOR_ADDR_WR, 0x33, 0xB0,0x50);				//Required
SensorWrite2 (SENSOR_ADDR_WR, 0x33, 0xB2,0x1A);				//Required
SensorWrite2 (SENSOR_ADDR_WR, 0x33, 0xB3,0x04);				//Required

SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x00,0x00);				//Take sensor out of standby mode

CyU3PBusyWait (999);										//Delay for sensor to stabilize
status = SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x02,0x00);				//Master Mode start
if (status == CY_U3P_SUCCESS)
{
		CyU3PGpioGetValue(GPIO_TEST,&pinState);
       	 		if (pinState == CyTrue)
	 		  		 	       	 			CyU3PGpioSetValue(GPIO_TEST, CyFalse);

	 		  		 	       	 		else
	 		  		 	       	 			CyU3PGpioSetValue(GPIO_TEST, CyTrue);
}
/*
#ifdef IMAGE_SENSOR_LVDS
    //Reset
    buf[0] = 0;
    buf[1] = 1;
    SensorWrite (SENSOR_ADDR_WR, 0x0C, 2, buf);
    buf[0] = 0;
    buf[1] = 0;
    SensorWrite (SENSOR_ADDR_WR, 0x0C, 2, buf);

    SensorRead (SENSOR_ADDR_RD, 0x07, 2, buf);
    CyU3PDebugPrint (4, "Control Reg = %d %d\n", buf[0],buf[1]);

    //Enable LVDS driver bit4 = 0
    buf[0] = 0;
    buf[1] = 0;
    SensorWrite (SENSOR_ADDR_WR, 0xB3, 2, buf);

    //De-assert LVDS power down bit1 = 0
    buf[0] = 0;
    buf[1] = 0;
    SensorWrite (SENSOR_ADDR_WR, 0xB1, 2, buf);

    //Issue a soft reset bit0 = 1 followed bu bit0 = 0
    buf[0] = 0;
    buf[1] = 1;
    SensorWrite (SENSOR_ADDR_WR, 0x0C, 2, buf);
    buf[0] = 0;
    buf[1] = 0;
    SensorWrite (SENSOR_ADDR_WR, 0x0C, 2, buf);

    //Force sync pattern for deserializer to lock (added 8/6/2014)
    buf[0] = 0;
    buf[1] = 1;
    SensorWrite (SENSOR_ADDR_WR, 0xB5, 2, buf);
    CyU3PBusyWait(1000);
    buf[0] = 0;
    buf[1] = 0;
    SensorWrite (SENSOR_ADDR_WR, 0xB5, 2, buf);
    CyU3PBusyWait(1000);

    buf[0] = 0;
    buf[1] = 0; //bit0 is for Exposure | bit1 is for Gain
    SensorWrite (SENSOR_ADDR_WR, 0xAF, 2, buf); //Disables auto gain and exposure

//    buf[0] = 0x03; // max int time at 30Hz
//    buf[1] = 0xC0;
    buf[0] = 0x00;//0x05;
    buf[1] = 0xFF;//0x14;
    SensorWrite (SENSOR_ADDR_WR, 0x0B, 2, buf); //Sets total shutter width

    buf[0] = 0x02;//0x3A;
    buf[1] = 0x21;//0x34;
    SensorWrite (SENSOR_ADDR_WR, 0x06, 2, buf); //Sets vertical blanking to extend past shutter width

    buf[0] = 0;//0b00111000;
    buf[1] = 0x00;
    SensorWrite (SENSOR_ADDR_WR, 0x7F, 2, buf); //Test pattern off if buf[0] = 0

    buf[0] = 0x00;
    buf[1] = 0b00110100; //bit 5 enables noise correction
    SensorWrite (SENSOR_ADDR_WR, 0x70, 2, buf); //Turns off noise reduction

    //REPEAT!!!---------------
    CyU3PBusyWait(1000);
    //Reset
        buf[0] = 0;
        buf[1] = 1;
        SensorWrite (SENSOR_ADDR_WR, 0x0C, 2, buf);
        buf[0] = 0;
        buf[1] = 0;
        SensorWrite (SENSOR_ADDR_WR, 0x0C, 2, buf);

        SensorRead (SENSOR_ADDR_RD, 0x07, 2, buf);
        CyU3PDebugPrint (4, "Control Reg = %d %d\n", buf[0],buf[1]);

        //Enable LVDS driver bit4 = 0
        buf[0] = 0;
        buf[1] = 0;
        SensorWrite (SENSOR_ADDR_WR, 0xB3, 2, buf);

        //De-assert LVDS power down bit1 = 0
        buf[0] = 0;
        buf[1] = 0;
        SensorWrite (SENSOR_ADDR_WR, 0xB1, 2, buf);

        //Issue a soft reset bit0 = 1 followed bu bit0 = 0
        buf[0] = 0;
        buf[1] = 1;
        SensorWrite (SENSOR_ADDR_WR, 0x0C, 2, buf);
        buf[0] = 0;
        buf[1] = 0;
        SensorWrite (SENSOR_ADDR_WR, 0x0C, 2, buf);

        //Force sync pattern for deserializer to lock (added 8/6/2014)
        buf[0] = 0;
        buf[1] = 1;
        SensorWrite (SENSOR_ADDR_WR, 0xB5, 2, buf);
        CyU3PBusyWait(1000);
        buf[0] = 0;
        buf[1] = 0;
        SensorWrite (SENSOR_ADDR_WR, 0xB5, 2, buf);
        CyU3PBusyWait(1000);

        buf[0] = 0;
        buf[1] = 0; //bit0 is for Exposure | bit1 is for Gain
        SensorWrite (SENSOR_ADDR_WR, 0xAF, 2, buf); //Disables auto gain and exposure

    //    buf[0] = 0x03; // max int time at 30Hz
    //    buf[1] = 0xC0;
        buf[0] = 0x00;
        buf[1] = 0xFF;
        SensorWrite (SENSOR_ADDR_WR, 0x0B, 2, buf); //Sets total shutter width

        buf[0] = 0x02;
        buf[1] = 0x21;
        SensorWrite (SENSOR_ADDR_WR, 0x06, 2, buf); //Sets vertical blanking to extend past shutter width

        buf[0] = 0;//0b00111000;
        buf[1] = 0x00;
        SensorWrite (SENSOR_ADDR_WR, 0x7F, 2, buf); //Test pattern off if buf[0] = 0

        buf[0] = 0x00;
        buf[1] = 0b00110100; //bit 5 enables noise correction
        SensorWrite (SENSOR_ADDR_WR, 0x70, 2, buf); //Turns off noise reduction
        //----------------------------------------------------------------
	#else
    	buf[0] = SENSOR_ADDR_WR; //sets allowable i2c addresses to send through serializer
    	SensorWrite (DESER_ADDR_WR, 0x08, 1, buf);
    	SensorWrite (DESER_ADDR_WR, 0x10, 1, buf);

    	buf[0] = DAC_ADDR_WR; //sets allowable i2c addresses to send through serializer
    	SensorWrite (DESER_ADDR_WR, 0x09, 1, buf);
    	SensorWrite (DESER_ADDR_WR, 0x11, 1, buf);

        buf[0] = 0;
        buf[1] = 0; //bit0 is for Exposure | bit1 is for Gain
        SensorWrite (SENSOR_ADDR_WR, 0xAF, 2, buf); //Disables auto gain and exposure

            //    buf[0] = 0x03; // max int time at 30Hz
            //    buf[1] = 0xC0;
        buf[0] = 0x00;
        buf[1] = 0xFF;
        SensorWrite (SENSOR_ADDR_WR, 0x0B, 2, buf); //Sets total shutter width

        buf[0] = 0x02;
        buf[1] = 0x21;
        SensorWrite (SENSOR_ADDR_WR, 0x06, 2, buf); //Sets vertical blanking to extend past shutter width
        scaleFactorFPS = 1000;



//                buf[0] = 0;//0b00111000;
//                buf[1] = 0x00;
//                SensorWrite (SENSOR_ADDR_WR, 0x7F, 2, buf); //Test pattern off if buf[0] = 0
//
//                buf[0] = 0x00;
//                buf[1] = 0b00110100; //bit 5 enables noise correction
//                SensorWrite (SENSOR_ADDR_WR, 0x70, 2, buf); //Turns off noise reduction
	#endif
*/
}

/*
 * Verify that the sensor can be accessed over the I2C bus from FX3.
 */
 
 uint8_t
SensorI2cBusTest (
        void)
{
    /* The sensor ID register can be read here to verify sensor connectivity. */
    uint8_t buf[2];
    buf[0] = buf[1] = 1;
    /* Reading sensor ID */
 /*   if (SensorRead (SENSOR_ADDR_RD, 0x00, 2, buf) == CY_U3P_SUCCESS)
    {
    	CyU3PDebugPrint (4, "ID Code = %d %d\n", buf[0],buf[1]);
    	//if ((buf[0] == 0x13) && (buf[1] == 0x24)) //for MT9V034
        if ((buf[0] == 0x13) && (buf[1] == 0x13)) // if ((buf[0] == 0x24) && (buf[1] == 0x81))
        {
            return CY_U3P_SUCCESS;
        }
    }
    CyU3PDebugPrint (4, "ID Code = %d %d\n", buf[0],buf[1]);
*/
    return 1;

}

 /*
    Get the current brightness setting Changed Jill 2-17-19
  */
 uint8_t
 SensorGetBrightness (
         void)
 {
	 uint8_t buf[2];
     SensorRead2 (SENSOR_ADDR_RD, 0x30, 0x0A, buf[0]);
     SensorRead2 (SENSOR_ADDR_RD, 0x30, 0x0B, buf[1]);
     if(buf[1] > 0)
     {
    	 buf[0] = buf[0] << 1;
     	 buf[0] = buf[0] & 0x80;
     }
     else
     {
    	 buf[0] = buf[0]<<1;
     }
 return buf[0];
 }

 /*
    Update the brightness setting Changed Jill 2-17-19
  */
 void
 SensorSetBrightness (
         uint8_t brightness)
 {
	 uint8_t set_value;
	 if(brightness <= 0x7F)
	 {
		 set_value = brightness << 1;
		 SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x0A,set_value);				//Black Level Offset set to recommended value
		 SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x0B,0x00);				//Black Level Offset set to recommended value
	 }
	 else
	 {
		 set_value = brightness  << 1 ;
		 if(brightness == 0xFF)
			 set_value = 0xFF;
		 SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x0A,set_value);				//Black Level Offset set to recommended value
		 SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x0B,0x01);				//Black Level Offset set to recommended value
	 }

 }
 uint8_t
 SensorGetGain (
         void)
 {


	 uint8_t buf[1];
//     SensorRead (SENSOR_ADDR_RD, 0xBA, 2, buf);  Commented out by Jill 8/28/18
     SensorRead2 (SENSOR_ADDR_RD, 0x30, 0x14, buf);

//     CyU3PSpiTransmitWords (buf, 1);

     //uint16_t temp = (buf[0]<<8) | buf[1];
     //	return (uint8_t)buf[1]&0b01111111; 		   Commented out by Jill 8/28/18
     return buf[0];
 }

 /*
    Update the brightness setting
  */
 void
 SensorSetGain (
         uint8_t gain)
 {

	 if (gain > 0xF0)
 		gain=0xF0;

	 SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x14, gain);				//Set Gain - Added by Jill 8/28/18

 }
//Added Jill 8/30/18
 void SensorSetFPS (uint8_t FPS) {
 //Update the FPS of the sensor
	  	uint8_t buf[2];
	  	CyBool_t pinState;
	  	switch (FPS) {
	  	case (1): //This case does nothing because the Miniscope software quits when something is here

	  	break;
	  	case (2): //8FPS
			SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x18,0xEE);				//VMAX = 0FEEh  or for 8fps Registers 18,19 and 1A
			SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x19,0x0F);				//
			SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x1A,0x00);				//
	  	break;
	  	case (3): //30FPS
			SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x18,0x66);				//VMAX = 04EEh  or for 30fps Registers 18,19 and 1A
			SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x19,0x04);				//
			SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x1A,0x00);				//
	  	break;
	  	case (4): //60FPS
			SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x18,0x33);				//VMAX = 0233h  or for 60fps Registers 18,19 and 1A
			SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x19,0x02);				//
			SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x1A,0x00);				//
	  	break;
	  	case (5): //100FPS
			SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x18,0x77);				//VMAX = 0155h  or for 99fps Registers 18,19 and 1A
			SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x19,0x01);				//
			SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x1A,0x00);				//
		break;
	  	case (6): //153FPS
			SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x18,0xDC);				//VMAX = 00DCh  or for 153fps Registers 18,19 and 1A
			SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x19,0x00);				//
			SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x1A,0x00);				//
		break;
	  	case (7): //200FPS
	  		SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x18,0xA8);				//VMAX = 00A8h  or for 201fps Registers 18,19 and 1A
	  		SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x19,0x00);				//
	  		SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x1A,0x00);				//
	  	break;
	  	case (8): //250FPS
	  		SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x18,0x87);				//VMAX = 0087h  or for 250fps Registers 18,19 and 1A
	  		SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x19,0x00);				//
	  		SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x1A,0x00);				//
	  	break;
	  	case (9): //293FPS
	  		SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x18,0x73);				//VMAX = 0073h  or for 293fps Registers 18,19 and 1A
	  		SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x19,0x00);				//
	  		SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x1A,0x00);				//
	  		  	break;
	  	case (10): //High Gain Conversion ON
			SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x09,0x12);			//FRSEL = 0x12h 1 sets HGC on and 2 for FRSEL
		break;
	  	case (11): //High Gain Conversion OFF
			SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x09,0x02);			//FRSEL = 0x02h 0 sets HGC off and 2 for FRSEL
		break;
	  	case (12): //500FPS
	  		 SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x18,0x43);				//VMAX = 0043h  or for 500fps Registers 18,19 and 1A
	  		 SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x19,0x00);				//
	  		 SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x1A,0x00);				//
	  	break;
	  	case (13): //700FPS
	  		  SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x18,0x30);				//VMAX = 0030h  or for 700fps Registers 18,19 and 1A
	  		  SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x19,0x00);				//
	  		  SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x1A,0x00);				//
	   break;
	  	case (14): //400FPS
	  		  SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x18,0x54);				//VMAX = 0054h  or for 400fps Registers 18,19 and 1A
	  		  SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x19,0x00);				//
	  		  SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x1A,0x00);				//
	  	break;
	  	case (15): //1FPS
			SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x18,0x00);				//VMAX = 8000h Registers 18,19 and 1A
			SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x19,0x80);				//
			SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x1A,0x00);				//
	    break;
	  	case (16): //2FPS
				SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x18,0x00);				//VMAX = 7FFFh Registers 18,19 and 1A
				SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x19,0x60);				//
				SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x1A,0x00);				//
		break;
	  	case (17): //3FPS
				SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x18,0x00);				//VMAX = 7FFFh Registers 18,19 and 1A
				SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x19,0x40);				//
				SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x1A,0x00);				//
		break;
	  	case (18): //4FPS
				SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x18,0x00);				//VMAX = 7FFFh Registers 18,19 and 1A
				SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x19,0x20);				//
				SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x1A,0x00);				//
		break;
	  	}

 }
 void SensorSetWindow (uint8_t WIN) {
 //Update the window position of the sensor ADded Jill 2-17-19

	  	switch (WIN) {
	  	case (1): //This case does nothing because the Miniscope software quits when something is here

	  	break;
	  	case (2): //Window is at 0 pixels
	  		  	SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x3C,0x00);				//WINPV
	  		  	SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x3D,0x00);				//
	  	break;
	  	case (3): //Window is at 51 pixels
	  		    SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x3C,0x33);				//WINPV
	  		  	SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x3D,0x00);				//
	  	break;
	  	case (4): //Window is at 101  pixels
	  		  	SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x3C,0x65);				//WINPV
	  		  	SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x3D,0x00);				//
	  	break;
	  	case (5): //Window is at 151  pixels
	  		   	SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x3C,0x97);				//WINPV
	  		   	SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x3D,0x00);				//
	  	break;
	  	case (6): //201
 		  		SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x3C,0xC9);				//WINPV
	  		  	SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x3D,0x00);				//
	  	break;
	  	case (7): //251
	  	 		SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x3C,0xFB);				//WINPV
	  		  	SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x3D,0x00);				//
	  	break;
	  	case (8): //Window is at 301 pixels
	  		  	SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x3C,0x2D);				//WINPV
	  		  	SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x3D,0x01);				//
	  	break;
	  	case (9): //Window is at 351 pixels
	  		  	SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x3C,0x5F);				//WINPV
	  		  	SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x3D,0x01);				//
	  	break;
	  	case (10): //401
	  		  	SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x3C,0x91);				//WINPV
	  		  	SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x3D,0x01);				//
	  	break;
	  	case (11): //451
	  		  	SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x3C,0xC3);				//WINPV
	  		  	SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x3D,0x01);				//
	  	break;
	  	case (12): //Window is at 501 pixels
	  		  	SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x3C,0xF5);				//WINPV
	  		  	SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x3D,0x01);				//
	  	break;
	  	case (13): //Window is at 551 pixels
	  		  	SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x3C,0x27);				//WINPV
	  		    SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x3D,0x02);				//
	  	break;
	  	case (14): //601
 	  		  	SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x3C,0x59);				//WINPV
 	  		  	SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x3D,0x02);				//
		break;
	  	case (15): //651
	  	 	  	SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x3C,0x8B);				//WINPV
	  	 	  	SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x3D,0x02);				//
	  	break;
	  	case (16): //Window is at 701 pixels
	  		  	SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x3C,0xBD);				//WINPV
	  		  	SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x3D,0x02);				//
	  	break;
	  	case (17): //Window is at 751 pixels
	  		  	SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x3C,0xEF);				//WINPV
	  		  	SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x3D,0x02);				//
	  	break;
	  	case (18): //801
	  		  	SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x3C,0x21);				//WINPV
	  		  	SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x3D,0x03);				//
		break;
	  	case (19): //851
	  			SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x3C,0x53);				//WINPV
	  		  	SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x3D,0x03);				//
	  	break;
	  	case (20): //901
	  			SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x3C,0x85);				//WINPV
	  		  	SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x3D,0x03);				//
	  	break;
	  	case (21): //951
			SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x3C,0xB7);					//WINPV
		  	SensorWrite2 (SENSOR_ADDR_WR, 0x30, 0x3D,0x03);					//
		break;
	  	}

 }


 /* Commented out Jill 8/30/18
 //Update the FPS of the sensor
 void SensorSetFPS (uint8_t FPS) {
 	uint8_t buf[2];
 	uint16_t hBlank=0;
 	uint16_t vBlank=0;
 	SensorSetBrightness(1);

 	switch (FPS) {
 		case (5):
 			hBlank = 993;
 			vBlank = 2500;
 			scaleFactorFPS = 2970;
 			break;
 		case (10):
 			hBlank = 750;
 			vBlank = 1250;
 			scaleFactorFPS = 1720;
 			break;
 		case (15):
 			hBlank = 657;
 			vBlank = 750;
 			scaleFactorFPS = 1220;
 			break;
 		case (20):
 			hBlank = 870;
 			vBlank = 400;
 			scaleFactorFPS = 2970;
 			break;
 		case (30):
 			hBlank = 94;
 			vBlank = 545;
 			scaleFactorFPS = 1000;
 			break;
 		case (60):
 			hBlank = 93;
 			vBlank = 33;
 			scaleFactorFPS = 500;
 			break;
 	}

 	buf[0] = (uint8_t)((hBlank>>8)&0x0003);
 	buf[1] = (uint8_t)(hBlank&0x00FF);
 	SensorWrite (SENSOR_ADDR_WR, 0x05, 2, buf); //Sets horizontal blanking
 	buf[0] = (uint8_t)((vBlank>>8)&0x007F);
 	buf[1] = (uint8_t)(vBlank&0x00FF);
 	SensorWrite (SENSOR_ADDR_WR, 0x06, 2, buf); //Sets vertical blanking


 	SensorSetBrightness(255);
 }
 */
