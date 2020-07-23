/*
 ## Cypress FX3 Camera Kit header file (sensor.h)
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

/* This file defines the parameters and the interface for the IMX290 image
   sensor driver.
 */

#ifndef _INCLUDED_SENSOR_H_
#define _INCLUDED_SENSOR_H_

#include <cyu3types.h>



#define SADDR_HIGH
//#define IMAGE_SENSOR_LVDS //Added by Daniel 8_3_2015
#define IMAGE_SENSOR_IMX290

//WINWV 1D8h for 472, 1E0 for 480

//WINWV 218h for 536, 220h for 544 for 60fps

//WINWV 408h for 1032, 410h for 1040 for 30fps

//WINWV 149h for 329, 151 for 337 MAX FPS 100

//WINWV 0C8h for 200, 0D0 for 208 MAX FPS 153

//WINWV 094h for 148, 09C for 156 Max FPS 200

//WINWV 069h for 105, 071 for 113 Max FPS 250

//WINWV 05Eh for 94, 066 for 102 Max FPS 293

//WINWV 040h for 64, 048 for 72 Max FPS 400 --- VMax 84d 0x54h

//WINWV 02Fh for 47, 037 for 55 Max FPS 500

//WINWV 01Ch for 28, 024 for 36 Max FPS 700
//Use the left number
#define WINWV_HIGH  	0x04
#define WINWV_LOW	  	0x08
//#define WINWV_HIGH  	0x00
//#define WINWV_LOW	  	0x94

//WINWH 0780h for 1920
//WINWH 076Ch for 1900
//WINWH 0798h for 1948
//WINWH 0708h for 1800
//WINWH 02F0h for 752
#define WINWH_HIGH  	0x07
#define WINWH_LOW	  	0x08
//#define WINWH_HIGH  	0x07
//#define WINWH_LOW	  	0x98

//Max Size for Recording + behavior Cam - Otherwise miniscope program will crash
//WINWH 0780h for 1920, //WINWV 408h for 1032



/* I2C Slave address for the Ser/Deser sensor. */
//Added by Daniel 8_3_2015
#define DESER_ADDR_WR	0xC0
#define DESER_ADDR_RD   0xC1
#define SER_ADDR_WR		0xB0
#define SER_ADDR_RD		0xB1

/* I2C address for the DAC. */
//Added by Daniel 8_10_2015
#define DAC_ADDR_WR		0b10011000 //For DAC5571

//Added Jill 6/4/18 - I2C address for IMX290

#define SENSOR_ADDR_WR 0x34             /* Slave address used to write sensor registers. */
#define SENSOR_ADDR_RD 0x35             /* Slave address used to read from sensor registers. */

//Below commented out section is from Daniel's code
/* I2C Slave address for the image sensor. */
//#ifdef SADDR_HIGH
//#define SENSOR_ADDR_WR 0xBA             /* Slave address used to write sensor registers. */
//#define SENSOR_ADDR_RD 0xBB             /* Slave address used to read from sensor registers. */
//#define SENSOR_ADDR_WR 0xB8             /* Slave address used to write sensor registers. */
//#define SENSOR_ADDR_RD 0xB9             /* Slave address used to read from sensor registers. */
//#else
//#define SENSOR_ADDR_WR 0x90             /* Slave address used to write sensor registers. */
//#define SENSOR_ADDR_RD 0x91             /* Slave address used to read from sensor registers. */
//#endif

#define I2C_SLAVEADDR_MASK 0xFE         /* Mask to get actual I2C slave address value without direction bit. */

#define I2C_MEMORY_ADDR_WR 0xA0         /* I2C slave address used to write to an EEPROM. */
#define I2C_MEMORY_ADDR_RD 0xA1         /* I2C slave address used to read from an EEPROM. */

/* GPIO 22 on FX3 is used to reset the Image sensor. */
//#define SENSOR_RESET_GPIO 22 //Not used anymore. Daniel 4_9_2015
#define DAC_LDAC			17
#define DAC_SHDN			18
#define FRAME_OUT			20
#define GPIO_TEST			22	//Added Jill 8/28/18

/* Function    : SensorWrite2B
   Description : Write two bytes of data to image sensor over I2C interface.
   Parameters  :
                 slaveAddr - I2C slave address for the sensor.
                 highAddr  - High byte of memory address being written to.
                 lowAddr   - Low byte of memory address being written to.
                 highData  - High byte of data to be written.
                 lowData   - Low byte of data to be written.
 */

extern CyU3PReturnStatus_t
SensorWrite2 (
        uint8_t slaveAddr,
        uint8_t highAddr,
        uint8_t lowAddr,
        uint8_t Data);

/* Function    : SensorWrite2
   Description : Write one byte of data to image sensor over I2C interface.
   Parameters  :
                 slaveAddr - I2C slave address for the sensor.
                 highAddr  - High byte of memory address being written to.
                 lowAddr   - Low byte of memory address being written to.
                 Data      - Data byte
                 
 */
 
 
 
 
 extern CyU3PReturnStatus_t
SensorWrite2B (
        uint8_t slaveAddr,
        uint8_t highAddr,
        uint8_t lowAddr,
        uint8_t highData,
        uint8_t lowData);

/* Function    : SensorWrite
   Description : Write arbitrary amount of data to image sensor over I2C interface.
   Parameters  :
                 slaveAddr - I2C slave address for the sensor.
                 highAddr  - High byte of memory address being written to.
                 lowAddr   - Low byte of memory address being written to.
                 count     - Size of write data in bytes. Limited to a maximum of 64 bytes.
                 buf       - Pointer to buffer containing data.
 */
extern CyU3PReturnStatus_t
SensorWrite (
        uint8_t slaveAddr,
        uint8_t Addr,
        uint8_t count,
        uint8_t *buf);

/* Function    : SensorRead2B
   Description : Read 2 bytes of data from image sensor over I2C interface.
   Parameters  :
                 slaveAddr - I2C slave address for the sensor.
                 highAddr  - High byte of memory address being written to.
                 lowAddr   - Low byte of memory address being written to.
                 buf       - Buffer to be filled with data. MSB goes in byte 0.
 */
//Added by Jill 6/4/18
 extern CyU3PReturnStatus_t
SensorRead2 (
        uint8_t slaveAddr,
        uint8_t highAddr,
        uint8_t lowAddr,
        uint8_t *buf);

/* Function    : SensorRead
   Description : Read arbitrary amount of data from image sensor over I2C interface.
   Parameters  :
                 slaveAddr - I2C slave address for the sensor.
                 highAddr  - High byte of memory address being written to.
                 lowAddr   - Low byte of memory address being written to.
                 buf       - Buffer to be filled with one byte of data.
 */
 
 
 
 extern CyU3PReturnStatus_t
SensorRead2B (
        uint8_t slaveAddr,
        uint8_t highAddr,
        uint8_t lowAddr,
        uint8_t *buf);

/* Function    : SensorRead
   Description : Read arbitrary amount of data from image sensor over I2C interface.
   Parameters  :
                 slaveAddr - I2C slave address for the sensor.
                 highAddr  - High byte of memory address being written to.
                 lowAddr   - Low byte of memory address being written to.
                 count     = Size of data to be read in bytes. Limited to a max of 64.
                 buf       - Buffer to be filled with data.
 */
extern CyU3PReturnStatus_t
SensorRead (
        uint8_t slaveAddr,
        uint8_t Addr,
        uint8_t count,
        uint8_t *buf);

/* Function    : SensorInit
   Description : Initialize the MT9M114 sensor.
   Parameters  : None
 */
extern void
SensorInit (
        void);

/* Function    : SensorReset
   Description : Reset the MT9M114 image sensor using FX3 GPIO.
   Parameters  : None
 */
extern void
SensorReset (
        void);

/* Function    : SensorChangeConfig
   Description : Update sensor configuration based on selected video parameters.
   Parameters  : None
 */
extern void
SensorChangeConfig (
        void);

/* Function     : SensorScaling_HD720p_30fps
   Description  : Configure the MT9M114 sensor for 720p 30 fps video stream.
   Parameters   : None
 */
extern void
SensorScaling_HD720p_30fps (
        void);

/* Function     : SensorScaling_VGA
   Description  : Configure the MT9M114 sensor for VGA video stream.
   Parameters   : None
 */
extern void
SensorScaling_VGA (
        void);

/* Function    : SensorI2cBusTest
   Description : Test whether the MT9M114 sensor is connected on the I2C bus.
   Parameters  : None
 */
extern uint8_t
SensorI2cBusTest (
        void);

/* Function    : SensorGetBrightness
   Description : Get the current brightness setting from the MT9M114 sensor.
   Parameters  : None
 */
extern uint8_t
SensorGetBrightness (
        void);

/* Function    : SensorSetBrightness
   Description : Set the desired brightness setting on the MT9M114 sensor.
   Parameters  :
                 brightness - Desired brightness level.
 */
extern void
SensorSetBrightness (
        uint8_t brightness);


extern uint8_t
SensorGetGain (
        void);

extern void
SensorSetGain (
        uint8_t gain);

extern void
SensorSetFPS (
        uint8_t FPS);
extern void
SensorSetWindow (
		uint8_t WIN);
#endif /* _INCLUDED_SENSOR_H_ */

/*[]*/

