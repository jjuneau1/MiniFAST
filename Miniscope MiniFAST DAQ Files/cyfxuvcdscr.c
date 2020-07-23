/*
 ## Cypress FX3 Camera Kit Source file (cyfxuvcdscr.c)
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

/*
 * This file contains the USB descriptors for the FX3
 */

#include "uvc.h"
#include "camera_ptzcontrol.h"
//WINWH 0780h for 1920
//WINWH 076Ch for 1900
//WINWH 0798h for 1948
//WINWH 0708h for 1800
//WINWH 02F0h for 752
#define WINWH_HIGH  	0x07
#define WINWH_LOW	  	0x08
//#define WINWH_HIGH  	0x07
//#define WINWH_LOW	  	0x98
//WINWV 1D8h for 472, 1E0 for 480

//WINWV 408h for 1032, 410h for 1040 for 30fps

//WINWV 218h for 536, 220h for 544 for MAX FPS 60

//WINWV 149h for 329, 151 for 337 MAX FPS 100

//WINWV 0C8h for 200, 0D0 for 208 MAX FPS 153

//WINWV 094h for 148, 09C for 156 Max FPS 200

//WINWV 069h for 105, 071 for 113 Max FPS 250

//WINWV 05Eh for 94, 066 for 102 Max FPS 293

//WINWV 02Fh for 47, 037 for 55 Max FPS 500

//WINWV 01Ch for 28, 024 for 36 Max FPS 700

//WINWV 040h for 64, 048 for 72 Max FPS 400 --- VMax 84d 0x54h
//Use the right number
#define WINWV_HIGH_8  	0x04
#define WINWV_LOW_8	  	0x10
//#define WINWV_HIGH_8  	0x00
//#define WINWV_LOW_8	  	0x9C

/* Standard Device Descriptor */
const uint8_t CyFxUSBDeviceDscr[] =
    {
        0x12,                           /* Descriptor Size */
        CY_U3P_USB_DEVICE_DESCR,        /* Device Descriptor Type */
        0x00,0x02,                      /* USB 2.0 */
        0xEF,                           /* Device Class */
        0x02,                           /* Device Sub-class */
        0x01,                           /* Device protocol */
        0x40,                           /* Maxpacket size for EP0 : 64 bytes */
        0xB4,0x04,                      /* Vendor ID */
        0xF8,0x00,                      /* Product ID */
        0x00,0x00,                      /* Device release number */
        0x01,                           /* Manufacture string index */
        0x02,                           /* Product string index */
        0x00,                           /* Serial number string index */
        0x01                            /* Number of configurations */
    };

/* Device Descriptor for SS */
const uint8_t CyFxUSBDeviceDscrSS[] =
    {
        0x12,                           /* Descriptor Size */
        CY_U3P_USB_DEVICE_DESCR,        /* Device Descriptor Type */
        0x00,0x03,                      /* USB 3.0 */
        0xEF,                           /* Device Class */
        0x02,                           /* Device Sub-class */
        0x01,                           /* Device protocol */
        0x09,                           /* Maxpacket size for EP0 : 2^9 Bytes */
        0xB4,0x04,                      /* Vendor ID */
        0xF9,0x00,                      /* Product ID */
        0x00,0x00,                      /* Device release number */
        0x01,                           /* Manufacture string index */
        0x02,                           /* Product string index */
        0x00,                           /* Serial number string index */
        0x01                            /* Number of configurations */
    };

/* Standard Device Qualifier Descriptor */
const uint8_t CyFxUSBDeviceQualDscr[] =
    {
        0x0A,                           /* Descriptor Size */
        CY_U3P_USB_DEVQUAL_DESCR,       /* Device Qualifier Descriptor Type */
        0x00,0x02,                      /* USB 2.0 */
        0xEF,                           /* Device Class */
        0x02,                           /* Device Sub-class */
        0x01,                           /* Device protocol */
        0x40,                           /* Maxpacket size for EP0 : 64 bytes */
        0x01,                           /* Number of configurations */
        0x00                            /* Reserved */
    };

/* Standard Full Speed Configuration Descriptor */
const uint8_t CyFxUSBFSConfigDscr[] =
    {

        /* Configuration Descriptor Type */
        0x09,                           /* Descriptor Size */
        CY_U3P_USB_CONFIG_DESCR,        /* Configuration Descriptor Type */
        0x09,0x00,                      /* Length of this descriptor and all sub descriptors */
        0x00,                           /* Number of interfaces */
        0x01,                           /* Configuration number */
        0x00,                           /* COnfiguration string index */
        0x80,                           /* Config characteristics - Bus powered */
        0x32,                           /* Max power consumption of device (in 2mA unit) : 100mA */
    };

/* Standard High Speed Configuration Descriptor */
const uint8_t CyFxUSBHSConfigDscr[] =
    {

        /* Configuration Descriptor Type */
        0x09,                           /* Descriptor Size */
        CY_U3P_USB_CONFIG_DESCR,        /* Configuration Descriptor Type */
#ifdef USB_DEBUG_INTERFACE
        0xE4,0x00,                      /* Length of this descriptor and all sub descriptors */
        0x03,                           /* Number of interfaces */
#else
        0xCD,0x00,                      /* Length of this descriptor and all sub descriptors */
        0x02,                           /* Number of interfaces */
#endif
        0x01,                           /* Configuration number */
        0x00,                           /* COnfiguration string index */
        0x80,                           /* Config characteristics - Bus powered */
        0xFA,                           /* Max power consumption of device (in 2mA unit) : 500mA */

        /* Interface Association Descriptor */
        0x08,                           /* Descriptor Size */
        CY_FX_INTF_ASSN_DSCR_TYPE,      /* Interface Association Descr Type: 11 */
        0x00,                           /* I/f number of first VideoControl i/f */
        0x02,                           /* Number of Video i/f */
        0x0E,                           /* CC_VIDEO : Video i/f class code */
        0x03,                           /* SC_VIDEO_INTERFACE_COLLECTION : Subclass code */
        0x00,                           /* Protocol : Not used */
        0x00,                           /* String desc index for interface */

        /* Standard Video Control Interface Descriptor */
        0x09,                           /* Descriptor size */
        CY_U3P_USB_INTRFC_DESCR,        /* Interface Descriptor type */
        0x00,                           /* Interface number */
        0x00,                           /* Alternate setting number */
        0x01,                           /* Number of end points */
        0x0E,                           /* CC_VIDEO : Interface class */
        0x01,                           /* CC_VIDEOCONTROL : Interface sub class */
        0x00,                           /* Interface protocol code */
        0x00,                           /* Interface descriptor string index */

        /* Class specific VC Interface Header Descriptor */
        0x0D,                           /* Descriptor size */
        0x24,                           /* Class Specific I/f Header Descriptor type */
        0x01,                           /* Descriptor Sub type : VC_HEADER */
        0x00,0x01,                      /* Revision of class spec : 1.0 */
        0x50,0x00,                      /* Total Size of class specific descriptors (till Output terminal) */
        0x00,0x6C,0xDC,0x02,            /* Clock frequency : 48MHz(Deprecated) */
        0x01,                           /* Number of streaming interfaces */
        0x01,                           /* Video streaming I/f 1 belongs to VC i/f */

        /* Input (Camera) Terminal Descriptor */
        0x12,                           /* Descriptor size */
        0x24,                           /* Class specific interface desc type */
        0x02,                           /* Input Terminal Descriptor type */
        0x01,                           /* ID of this terminal */
        0x01,0x02,                      /* Camera terminal type */
        0x00,                           /* No association terminal */
        0x00,                           /* String desc index : Not used */
#ifdef UVC_PTZ_SUPPORT
        (uint8_t)(wObjectiveFocalLengthMin&0xFF),
        (uint8_t)((wObjectiveFocalLengthMin>>8)&0xFF),
        (uint8_t)(wObjectiveFocalLengthMax&0xFF),
        (uint8_t)((wObjectiveFocalLengthMax>>8)&0xFF),
        (uint8_t)(wOcularFocalLength&0xFF),
        (uint8_t)((wOcularFocalLength>>8)&0xFF),
#else
        0x00,0x00,                      /* No optical zoom supported */
        0x00,0x00,                      /* No optical zoom supported */
        0x00,0x00,                      /* No optical zoom supported */
#endif
        0x03,                           /* Size of controls field for this terminal : 3 bytes */
                                        /* A bit set to 1 indicates that the mentioned Control is
                                         * supported for the video stream in the bmControls field
                                         * D0: Scanning Mode
                                         * D1: Auto-Exposure Mode
                                         * D2: Auto-Exposure Priority
                                         * D3: Exposure Time (Absolute)
                                         * D4: Exposure Time (Relative)
                                         * D5: Focus (Absolute)
                                         * D6: Focus (Relative)
                                         * D7: Iris (Absolute)
                                         * D8: Iris (Relative)
                                         * D9: Zoom (Absolute)
                                         * D10: Zoom (Relative)
                                         * D11: PanTilt (Absolute)
                                         * D12: PanTilt (Relative)
                                         * D13: Roll (Absolute)
                                         * D14: Roll (Relative)
                                         * D15: Reserved
                                         * D16: Reserved
                                         * D17: Focus, Auto
                                         * D18: Privacy
                                         * D19: Focus, Simple
                                         * D20: Window
                                         * D21: Region of Interest
                                         * D22 – D23: Reserved, set to zero
                                         */
#ifdef UVC_PTZ_SUPPORT
        0x00,0x0A,0x00,                 /* bmControls field of camera terminal: PTZ supported */
#else
        0x00,0x00,0x00,                 /* bmControls field of camera terminal: No controls supported */
#endif

       /* Processing Unit Descriptor */
        0x0C,                           /* Descriptor size */
        0x24,                           /* Class specific interface desc type */
        0x05,                           /* Processing Unit Descriptor type */
        0x02,                           /* ID of this terminal */
        0x01,                           /* Source ID : 1 : Conencted to input terminal */
        0x00,0x40,                      /* Digital multiplier */
        0x03,                           /* Size of controls field for this terminal : 3 bytes */
                                        /* A bit set to 1 in the bmControls field indicates that
                                         * the mentioned Control is supported for the video stream.
                                         * D0: Brightness *
                                         * D1: Contrast Added 4_9_2015
                                         * D2: Hue Added 4_9_2015
                                         * D3: Saturation
                                         * D4: Sharpness
                                         * D5: Gamma
                                         * D6: White Balance Temperature
                                         * D7: White Balance Component
                                         * D8: Backlight Compensation
                                         * D9: Gain *
                                         * D10: Power Line Frequency
                                         * D11: Hue, Auto
                                         * D12: White Balance Temperature, Auto
                                         * D13: White Balance Component, Auto
                                         * D14: Digital Multiplier
                                         * D15: Digital Multiplier Limit
                                         * D16: Analog Video Standard
                                         * D17: Analog Video Lock Status
                                         * D18: Contrast, Auto
                                         * D19 – D23: Reserved. Set to zero.
                                         */
        0x0F,0x02,0x00,                 /* bmControls field of processing unit: Brightness and Gain control supported (0x01,0x02,0x00,)*/ //Changed by Daniel to add LED control 4_9_2015
        0x00,                           /* String desc index : Not used */

        /* Extension Unit Descriptor */
        0x1C,                           /* Descriptor size */
        0x24,                           /* Class specific interface desc type */
        0x06,                           /* Extension Unit Descriptor type */
        0x03,                           /* ID of this terminal */
        0xFF,0xFF,0xFF,0xFF,            /* 16 byte GUID */
        0xFF,0xFF,0xFF,0xFF,
        0xFF,0xFF,0xFF,0xFF,
        0xFF,0xFF,0xFF,0xFF,
        0x00,                           /* Number of controls in this terminal */
        0x01,                           /* Number of input pins in this terminal */
        0x02,                           /* Source ID : 2 : Connected to Proc Unit */
        0x03,                           /* Size of controls field for this terminal : 3 bytes */
        0x00,0x00,0x00,                 /* No controls supported */
        0x00,                           /* String desc index : Not used */

        /* Output Terminal Descriptor */
        0x09,                           /* Descriptor size */
        0x24,                           /* Class specific interface desc type */
        0x03,                           /* Output Terminal Descriptor type */
        0x04,                           /* ID of this terminal */
        0x01,0x01,                      /* USB Streaming terminal type */
        0x00,                           /* No association terminal */
        0x03,                           /* Source ID : 3 : Connected to Extn Unit */
        0x00,                           /* String desc index : Not used */

        /* Video Control Status Interrupt Endpoint Descriptor */
        0x07,                           /* Descriptor size */
        CY_U3P_USB_ENDPNT_DESCR,        /* Endpoint Descriptor Type */
        CY_FX_EP_CONTROL_STATUS,        /* Endpoint address and description */
        CY_U3P_USB_EP_INTR,             /* Interrupt End point Type */
        0x40,0x00,                      /* Max packet size = 64 bytes */
        0x08,                           /* Servicing interval : 8ms */

        /* Class Specific Interrupt Endpoint Descriptor */
        0x05,                           /* Descriptor size */
        0x25,                           /* Class Specific Endpoint Descriptor Type */
        CY_U3P_USB_EP_INTR,             /* End point Sub Type */
        0x40,0x00,                      /* Max packet size = 64 bytes */

        /* Standard Video Streaming Interface Descriptor (Alternate Setting 0) */
        0x09,                           /* Descriptor size */
        CY_U3P_USB_INTRFC_DESCR,        /* Interface Descriptor type */
        0x01,                           /* Interface number */
        0x00,                           /* Alternate setting number */
        0x01,                           /* Number of end points : Zero Bandwidth */
        0x0E,                           /* Interface class : CC_VIDEO */
        0x02,                           /* Interface sub class : CC_VIDEOSTREAMING */
        0x00,                           /* Interface protocol code : Undefined */
        0x00,                           /* Interface descriptor string index */

       /* Class-specific Video Streaming Input Header Descriptor */
        0x0E,                           /* Descriptor size */
        0x24,                           /* Class-specific VS I/f Type */
        0x01,                           /* Descriptotor Subtype : Input Header */
        0x01,                           /* 1 format desciptor follows */
        0x29,0x00,                      /* Total size of Class specific VS descr: 41 Bytes */
        CY_FX_EP_BULK_VIDEO,            /* EP address for BULK video data */
        0x00,                           /* No dynamic format change supported */
        0x04,                           /* Output terminal ID : 4 */
        0x01,                           /* Still image capture method 1 supported */
        0x00,                           /* Hardware trigger NOT supported */
        0x00,                           /* Hardware to initiate still image capture NOT supported */
        0x01,                           /* Size of controls field : 1 byte */
        0x00,                           /* D2 : Compression quality supported */


       /* Class specific Uncompressed VS format descriptor */
        0x1B,                           /* Descriptor size */
        0x24,                           /* Class-specific VS I/f Type */
        0x04,                           /* Subtype : uncompressed format I/F */
        0x01,                           /* Format desciptor index (only one format is supported) */
        0x01,                           /* number of frame descriptor followed */
        0x59,0x55,0x59,0x32,            /* GUID used to identify streaming-encoding format: YUY2  */
        0x00,0x00,0x10,0x00,
        0x80,0x00,0x00,0xAA,
        0x00,0x38,0x9B,0x71,
        0x10,                           /* Number of bits per pixel used to specify color in the decoded video frame.
                                           0 if not applicable: 10 bit per pixel */
        0x01,                           /* Optimum Frame Index for this stream: 1 */
        0x08,                           /* X dimension of the picture aspect ratio: Non-interlaced in
			        	   progressive scan */
        0x06,                           /* Y dimension of the picture aspect ratio: Non-interlaced in
					   progressive scan*/
        0x00,                           /* Interlace Flags: Progressive scanning, no interlace */
        0x00,                           /* duplication of the video stream restriction: 0 - no restriction */



       /* Class specific Uncompressed VS Frame descriptor */
        0x1E,                           /* Descriptor size */
        0x24,                           /* Descriptor type*/
        0x05,                           /* Subtype: uncompressed frame I/F */
        0x01,                           /* Frame Descriptor Index */
        0x03,                           /* Still image capture method 1 supported, fixed frame rate */
        0x00,0x02,                      /* Width in pixel: 320-QVGA */
        0xE0,0x01,                      /* Height in pixel 240-QVGA */
        0x00,0x50,0x97,0x31,            /* Min bit rate bits/s. Not specified, taken from MJPEG */
        0x00,0x50,0x97,0x31,            /* Max bit rate bits/s. Not specified, taken from MJPEG */
        0x00,0x60,0x09,0x00,            /* Maximum video or still frame size in bytes(Deprecated) */
        0x2A,0x2C,0x0A,0x00,            /* Default Frame Interval */
        0x01,                           /* Frame interval(Frame Rate) types: Only one frame interval supported */
        0x2A,0x2C,0x0A,0x00,            /* Shortest Frame Interval */

        /* Endpoint Descriptor for BULK Streaming Video Data */
        0x07,                           /* Descriptor size */
        CY_U3P_USB_ENDPNT_DESCR,        /* Endpoint Descriptor Type */
        CY_FX_EP_BULK_VIDEO,            /* Endpoint address and description */
        0x02,                           /* BULK End point */
        (uint8_t)(512 & 0x00FF),        /* High speed max packet size is always 512 bytes. */
        (uint8_t)((512 & 0xFF00)>>8),
        0x01                            /* Servicing interval for data transfers */

#ifdef USB_DEBUG_INTERFACE
        ,
        0x09,                           /* Descriptor size */
        CY_U3P_USB_INTRFC_DESCR,        /* Interface Descriptor type */
        0x02,                           /* Interface number */
        0x00,                           /* Alternate setting number */
        0x02,                           /* Number of end points */
        0xFF,                           /* Interface class */
        0x00,                           /* Interface sub class */
        0x00,                           /* Interface protocol code */
        0x00,                           /* Interface descriptor string index */

        /* Endpoint descriptor for producer EP */
        0x07,                           /* Descriptor size */
        CY_U3P_USB_ENDPNT_DESCR,        /* Endpoint descriptor type */
        CY_FX_EP_DEBUG_CMD,             /* Endpoint address and description */
        CY_U3P_USB_EP_BULK,             /* Bulk endpoint type */
        0x00,0x02,                      /* Max packet size = 512 bytes */
        0x00,                           /* Servicing interval for data transfers : 0 for bulk */
    
        /* Endpoint descriptor for consumer EP */
        0x07,                           /* Descriptor size */
        CY_U3P_USB_ENDPNT_DESCR,        /* Endpoint descriptor type */
        CY_FX_EP_DEBUG_RSP,             /* Endpoint address and description */
        CY_U3P_USB_EP_BULK,             /* Bulk endpoint type */
        0x00,0x02,                      /* Max packet size = 512 bytes */
        0x00                            /* Servicing interval for data transfers : 0 for Bulk */
#endif
    };

/* BOS for SS */

#define CY_FX_BOS_DSCR_TYPE             15
#define CY_FX_DEVICE_CAPB_DSCR_TYPE     16
#define CY_FX_SS_EP_COMPN_DSCR_TYPE     48

/* Device Capability Type Codes */
#define CY_FX_WIRELESS_USB_CAPB_TYPE    1
#define CY_FX_USB2_EXTN_CAPB_TYPE       2
#define CY_FX_SS_USB_CAPB_TYPE          3
#define CY_FX_CONTAINER_ID_CAPBD_TYPE   4

const uint8_t CyFxUSBBOSDscr[] =
{
        0x05,                           /* Descriptor Size */
        CY_FX_BOS_DSCR_TYPE,            /* Device Descriptor Type */
        0x16,0x00,                      /* Length of this descriptor and all sub descriptors */
        0x02,                           /* Number of device capability descriptors */

        /* USB 2.0 Extension */
        0x07,                           /* Descriptor Size */
        CY_FX_DEVICE_CAPB_DSCR_TYPE,    /* Device Capability Type descriptor */
        CY_FX_USB2_EXTN_CAPB_TYPE,      /* USB 2.0 Extension Capability Type */
        0x00,0x00,0x00,0x00,            /* Supported device level features  */

        /* SuperSpeed Device Capability */
        0x0A,                           /* Descriptor Size */
        CY_FX_DEVICE_CAPB_DSCR_TYPE,    /* Device Capability Type descriptor */
        CY_FX_SS_USB_CAPB_TYPE,         /* SuperSpeed Device Capability Type */
        0x00,                           /* Supported device level features  */
        0x0E,0x00,                      /* Speeds Supported by the device : SS, HS and FS */
        0x03,                           /* Functionality support */
        0x00,                           /* U1 Device Exit Latency */
        0x00,0x00                       /* U2 Device Exit Latency */
};

/* Super Speed Configuration Descriptor */
const uint8_t CyFxUSBSSConfigDscr[] =
    {

        /* Configuration Descriptor Type */
        0x09,                           /* Descriptor Size */
        CY_U3P_USB_CONFIG_DESCR,        /* Configuration Descriptor Type */
#ifdef USB_DEBUG_INTERFACE
        0xFC,0x00,                      /* Total length of this and all sub-descriptors. */
        0x03,                           /* Number of interfaces */
#else
        0xD9,0x00,                      /* Length of this descriptor and all sub descriptors */
        0x02,                           /* Number of interfaces */
#endif
        0x01,                           /* Configuration number */
        0x00,                           /* Configuration string index */
        0x80,                           /* Config characteristics - Bus powered */
 //       0x32,                           /* Max power consumption of device (in 8mA unit) : 400mA */ Changed Jill
        0x3E,                           /* Max power consumption of device (in 8mA unit) : 400mA */

        /* Interface Association Descriptor */
        0x08,                           /* Descriptor Size */
        CY_FX_INTF_ASSN_DSCR_TYPE,      /* Interface Association Descr Type: 11 */
        0x00,                           /* I/f number of first VideoControl i/f */
        0x02,                           /* Number of Video i/f */
        0x0E,                           /* CC_VIDEO : Video i/f class code */
        0x03,                           /* SC_VIDEO_INTERFACE_COLLECTION : Subclass code */
        0x00,                           /* Protocol : Not used */
        0x00,                           /* String desc index for interface */

        /* Standard Video Control Interface Descriptor */
        0x09,                           /* Descriptor size */
        CY_U3P_USB_INTRFC_DESCR,        /* Interface Descriptor type */
        0x00,                           /* Interface number */
        0x00,                           /* Alternate setting number */
        0x01,                           /* Number of end points */
        0x0E,                           /* CC_VIDEO : Interface class */
        0x01,                           /* CC_VIDEOCONTROL : Interface sub class */
        0x00,                           /* Interface protocol code */
        0x00,                           /* Interface descriptor string index */

        /* Class specific VC Interface Header Descriptor */
        0x0D,                           /* Descriptor size */
        0x24,                           /* Class Specific I/f Header Descriptor type */
        0x01,                           /* Descriptor Sub type : VC_HEADER */
        0x00,0x01,                      /* Revision of class spec : 1.0 */
        0x50,0x00,                      /* Total Size of class specific descriptors (till Output terminal) */
        0x00,0x6C,0xDC,0x02,            /* Clock frequency : 48MHz(Deprecated) */
        0x01,                           /* Number of streaming interfaces */
        0x01,                           /* Video streaming I/f 1 belongs to VC i/f */

        /* Input (Camera) Terminal Descriptor */
        0x12,                           /* Descriptor size */
        0x24,                           /* Class specific interface desc type */
        0x02,                           /* Input Terminal Descriptor type */
        0x01,                           /* ID of this terminal */
        0x01,0x02,                      /* Camera terminal type */
        0x00,                           /* No association terminal */
        0x00,                           /* String desc index : Not used */
#ifdef UVC_PTZ_SUPPORT
        (uint8_t)(wObjectiveFocalLengthMin&0xFF),
        (uint8_t)((wObjectiveFocalLengthMin>>8)&0xFF),
        (uint8_t)(wObjectiveFocalLengthMax&0xFF),
        (uint8_t)((wObjectiveFocalLengthMax>>8)&0xFF),
        (uint8_t)(wOcularFocalLength&0xFF),
        (uint8_t)((wOcularFocalLength>>8)&0xFF),
#else
        0x00,0x00,                      /* No optical zoom supported */
        0x00,0x00,                      /* No optical zoom supported */
        0x00,0x00,                      /* No optical zoom supported */
#endif
        0x03,                           /* Size of controls field for this terminal : 3 bytes */
                                        /* A bit set to 1 in the bmControls field indicates that
                                         * the mentioned Control is supported for the video stream.
                                         * D0: Scanning Mode
                                         * D1: Auto-Exposure Mode
                                         * D2: Auto-Exposure Priority
                                         * D3: Exposure Time (Absolute)
                                         * D4: Exposure Time (Relative)
                                         * D5: Focus (Absolute)
                                         * D6: Focus (Relative)
                                         * D7: Iris (Absolute)
                                         * D8: Iris (Relative)
                                         * D9: Zoom (Absolute)
                                         * D10: Zoom (Relative)
                                         * D11: PanTilt (Absolute)
                                         * D12: PanTilt (Relative)
                                         * D13: Roll (Absolute)
                                         * D14: Roll (Relative)
                                         * D15: Reserved
                                         * D16: Reserved
                                         * D17: Focus, Auto
                                         * D18: Privacy
                                         * D19: Focus, Simple
                                         * D20: Window
                                         * D21: Region of Interest
                                         * D22 – D23: Reserved, set to zero
                                         */
#ifdef UVC_PTZ_SUPPORT
        0x00,0x0A,0x00,                 /* bmControls field of camera terminal: PTZ supported */
#else
        0x00,0x00,0x00,                 /* bmControls field of camera terminal: No controls supported */
#endif

        /* Processing Unit Descriptor */
        0x0C,                           /* Descriptor size */
        0x24,                           /* Class specific interface desc type */
        0x05,                           /* Processing Unit Descriptor type */
        0x02,                           /* ID of this terminal */
        0x01,                           /* Source ID : 1 : Conencted to input terminal */
        0x00,0x40,                      /* Digital multiplier */
        0x03,                           /* Size of controls field for this terminal : 3 bytes */
        0x0F,0x02,0x00,                 /* bmControls field of processing unit: Brightness control supported Old:0x01,0x00,0x00, */ //Daniel: This must be changed to handle other UVC camera controls
        0x00,                           /* String desc index : Not used */

        /* Extension Unit Descriptor */
        0x1C,                           /* Descriptor size */
        0x24,                           /* Class specific interface desc type */
        0x06,                           /* Extension Unit Descriptor type */
        0x03,                           /* ID of this terminal */
        0xFF,0xFF,0xFF,0xFF,            /* 16 byte GUID */
        0xFF,0xFF,0xFF,0xFF,
        0xFF,0xFF,0xFF,0xFF,
        0xFF,0xFF,0xFF,0xFF,
        0x00,                           /* Number of controls in this terminal */
        0x01,                           /* Number of input pins in this terminal */
        0x02,                           /* Source ID : 2 : Connected to Proc Unit */
        0x03,                           /* Size of controls field for this terminal : 3 bytes */
                                        /* A bit set to 1 in the bmControls field indicates that
                                         * the mentioned Control is supported for the video stream.
                                         * D0: Brightness *
                                         * D1: Contrast *
                                         * D2: Hue
                                         * D3: Saturation
                                         * D4: Sharpness
                                         * D5: Gamma
                                         * D6: White Balance Temperature
                                         * D7: White Balance Component
                                         * D8: Backlight Compensation
                                         * D9: Gain *
                                         * D10: Power Line Frequency
                                         * D11: Hue, Auto
                                         * D12: White Balance Temperature, Auto
                                         * D13: White Balance Component, Auto
                                         * D14: Digital Multiplier
                                         * D15: Digital Multiplier Limit
                                         * D16: Analog Video Standard
                                         * D17: Analog Video Lock Status
                                         * D18: Contrast, Auto
                                         * D19 – D23: Reserved. Set to zero.
                                         */
        0x0F,0x02,0x00,                 /* bmControls field of processing unit: No control supported (0x00,0x00,0x00,)*/ //Changed by Daniel to add LED control 4_9_2015
        0x00,                           /* String desc index : Not used */

        /* Output Terminal Descriptor */
        0x09,                           /* Descriptor size */
        0x24,                           /* Class specific interface desc type */
        0x03,                           /* Output Terminal Descriptor type */
        0x04,                           /* ID of this terminal */
        0x01,0x01,                      /* USB Streaming terminal type */
        0x00,                           /* No association terminal */
        0x03,                           /* Source ID : 3 : Connected to Extn Unit */
        0x00,                           /* String desc index : Not used */

        /* Video Control Status Interrupt Endpoint Descriptor */
        0x07,                           /* Descriptor size */
        CY_U3P_USB_ENDPNT_DESCR,        /* Endpoint Descriptor Type */
        CY_FX_EP_CONTROL_STATUS,        /* Endpoint address and description */
        CY_U3P_USB_EP_INTR,             /* Interrupt End point Type */
        0x00,0x04,                      /* Max packet size = 1024 bytes */
        0x01,                           /* Servicing interval */

        /* Super Speed Endpoint Companion Descriptor */
        0x06,                           /* Descriptor size */
        CY_U3P_SS_EP_COMPN_DESCR,       /* SS Endpoint Companion Descriptor Type */
        0x00,                           /* Max no. of packets in a Burst : 1 */
        0x00,                           /* Attribute: N.A. */
        0x00,                           /* Bytes per interval:1024 */
        0x04,

        /* Class Specific Interrupt Endpoint Descriptor */
        0x05,                           /* Descriptor size */
        0x25,                           /* Class Specific Endpoint Descriptor Type */
        CY_U3P_USB_EP_INTR,             /* End point Sub Type */
        0x40,0x00,                      /* Max packet size = 64 bytes */


        /* Standard Video Streaming Interface Descriptor (Alternate Setting 0) */
        0x09,                           /* Descriptor size */
        CY_U3P_USB_INTRFC_DESCR,        /* Interface Descriptor type */
        0x01,                           /* Interface number */
        0x00,                           /* Alternate setting number */
        0x01,                           /* Number of end points */
        0x0E,                           /* Interface class : CC_VIDEO */
        0x02,                           /* Interface sub class : CC_VIDEOSTREAMING */
        0x00,                           /* Interface protocol code : Undefined */
        0x00,                           /* Interface descriptor string index */

       /* Class-specific Video Streaming Input Header Descriptor */
        0x0E,                           /* Descriptor size */
        0x24,                           /* Class-specific VS I/f Type */
        0x01,                           /* Descriptotor Subtype : Input Header */
        0x01,                           /* 1 format desciptor follows */
        0x47,0x00,                      /* Total size of Class specific VS descr */
        CY_FX_EP_BULK_VIDEO,            /* EP address for BULK video data */
        0x00,                           /* No dynamic format change supported */
        0x04,                           /* Output terminal ID : 4 */
        0x01,                           /* Still image capture method 1 supported */
        0x00,                           /* Hardware trigger NOT supported */
        0x00,                           /* Hardware to initiate still image capture NOT supported */
        0x01,                           /* Size of controls field : 1 byte */
        0x00,                           /* D2 : Compression quality supported */


        /* Class specific Uncompressed VS format descriptor */
	   0x1B,                           /* Descriptor size */
	   0x24,                           /* Class-specific VS I/f Type */
	   0x04,                           /* Subtype : uncompressed format I/F */
	   0x01,                           /* Format desciptor index */
	   0x01,                           /* Number of frame descriptor followed */
	   0x59,0x55,0x59,0x32,            /* GUID used to identify streaming-encoding format: YUY2  */
	   0x00,0x00,0x10,0x00,
	   0x80,0x00,0x00,0xAA,
	   0x00,0x38,0x9B,0x71,
	   0x10,                           /* Number of bits per pixel */
	   0x01,                           /* Optimum Frame Index for this stream: 1 */

	   //For MT9V032
	   0x00,                           /* X dimension of the picture aspect ratio; Non-interlaced */
	   0x1E,                           /* Y dimension of the pictuer aspect ratio: Non-interlaced */

	   0x00,                           /* Interlace Flags: Progressive scanning, no interlace */
	   0x00,                           /* duplication of the video stream restriction: 0 - no restriction */

	   /* Class specific Uncompressed VS frame descriptor */
	   0x1E,                           /* Descriptor size */
	   0x24,                           /* Descriptor type*/
	   0x05,                           /* Subtype: uncompressed frame I/F */
	   0x01,                           /* Frame Descriptor Index */
	   0x03,                           /* Still image capture method 1 supported, fixed frame rate */
//Below is used to change size - Jill
	   //Used for MT9V032
	   WINWH_LOW, WINWH_HIGH,                     /* Width in pixel 0x98, 0x07 --- For 1944 */
	   WINWV_LOW_8, WINWV_HIGH_8,                     /* Height in pixel 0xE0, 0x01 --- For 480*/
//	   0x9C, 0x07,                     /* Width in pixel 0x9C, 0x07 For 1948 1080*/
//	   0x56, 0x04,                     /* Height in pixel 0x56, 0x04 For 1110 for 1080p*/

//	   0x00,0x00,0x00,0x00,
//	   0x00,0x00,0x00,0x00,
	   0x50,0x00,0x00,0x00,            /* Min bit rate bits/s. 0x00,0x50,0x97,0x31*/
	   0x50,0x00,0x00,0x00,            /* Min bit rate bits/s. 0x00,0x50,0x97,0x31*/
	   //	   0x00,0x80,0xA7,0x24,            /* Max bit rate bits/s. 0x00,0x50,0x97,0x31*/
//	   0x00,0xA4,0x2F,0x00,            /* Maximum video or still frame size in bytes(Deprecated)*/
	   0x00,0x00,0x00,0x00,
	   0x80, 0x84, 0x1E, 0x00,
//	   0x15, 0x16, 0x05, 0x00,         /* 30fps 0x15, 0x16, 0x05, 0x00 | 60fps: 0x0B, 0x8B, 0x02, 0x00*/
	   0x01,
//	   0x15, 0x16, 0x05, 0x00,
	   0x80, 0x84, 0x1E, 0x00,
        /* Endpoint Descriptor for BULK Streaming Video Data */
        0x07,                           /* Descriptor size */
        CY_U3P_USB_ENDPNT_DESCR,        /* Endpoint Descriptor Type */
        CY_FX_EP_BULK_VIDEO,            /* Endpoint address and description */
        CY_U3P_USB_EP_BULK,             /* BULK End point */
        CY_FX_EP_BULK_VIDEO_PKT_SIZE_L, /* EP MaxPcktSize: 1024B */
        CY_FX_EP_BULK_VIDEO_PKT_SIZE_H, /* EP MaxPcktSize: 1024B */
        0x01,                           /* Servicing interval for data transfers */

        /* Super Speed Endpoint Companion Descriptor */
        0x06,                           /* Descriptor size */
        CY_U3P_SS_EP_COMPN_DESCR,       /* SS Endpoint Companion Descriptor Type */
        0x0F,                           /* Max number of packets per burst: 16 */
        0x00,                           /* Attribute: Streams not defined */
        0x00,                           /* No meaning for bulk */
        0x00

#ifdef USB_DEBUG_INTERFACE
        ,
        0x09,                           /* Descriptor size */
        CY_U3P_USB_INTRFC_DESCR,        /* Interface Descriptor type */
        0x02,                           /* Interface number */
        0x00,                           /* Alternate setting number */
        0x02,                           /* Number of end points */
        0xFF,                           /* Interface class */
        0x00,                           /* Interface sub class */
        0x00,                           /* Interface protocol code */
        0x00,                           /* Interface descriptor string index */

        /* Endpoint descriptor for producer EP */
        0x07,                           /* Descriptor size */
        CY_U3P_USB_ENDPNT_DESCR,        /* Endpoint descriptor type */
        CY_FX_EP_DEBUG_CMD,             /* Endpoint address and description */
        CY_U3P_USB_EP_BULK,             /* Bulk endpoint type */
        0x00,0x04,                      /* Max packet size = 1024 bytes */
        0x00,                           /* Servicing interval for data transfers : 0 for bulk */
    
        /* Super speed endpoint companion descriptor for producer EP */
        0x06,                           /* Descriptor size */
        CY_U3P_SS_EP_COMPN_DESCR,       /* SS endpoint companion descriptor type */
        0,                              /* No burst support. */
        0x00,                           /* Max streams for bulk EP = 0 (No streams) */
        0x00,0x00,                      /* Service interval for the EP : 0 for bulk */
    
        /* Endpoint descriptor for consumer EP */
        0x07,                           /* Descriptor size */
        CY_U3P_USB_ENDPNT_DESCR,        /* Endpoint descriptor type */
        CY_FX_EP_DEBUG_RSP,             /* Endpoint address and description */
        CY_U3P_USB_EP_BULK,             /* Bulk endpoint type */
        0x00,0x04,                      /* Max packet size = 1024 bytes */
        0x00,                           /* Servicing interval for data transfers : 0 for Bulk */
    
        /* Super speed endpoint companion descriptor for consumer EP */
        0x06,                           /* Descriptor size */
        CY_U3P_SS_EP_COMPN_DESCR,       /* SS endpoint companion descriptor type */
        0,                              /* No burst support. */
        0x00,                           /* Max streams for bulk EP = 0 (No streams) */
        0x00,0x00                       /* Service interval for the EP : 0 for bulk */
#endif
    };


/* Standard Language ID String Descriptor */
const uint8_t CyFxUSBStringLangIDDscr[] =
    {
        0x04,                           /* Descriptor Size */
        CY_U3P_USB_STRING_DESCR,        /* Device Descriptor Type */
        0x09,0x04                       /* Language ID supported */
    };

/* Standard Manufacturer String Descriptor */
const uint8_t CyFxUSBManufactureDscr[] =
    {
        0x10,                           /* Descriptor Size */
        CY_U3P_USB_STRING_DESCR,        /* Device Descriptor Type */
        'C',0x00,
        'y',0x00,
        'p',0x00,
        'r',0x00,
        'e',0x00,
        's',0x00,
        's',0x00
    };


/* Standard Product String Descriptor */
const uint8_t CyFxUSBProductDscr[] =
    {
//        0x08,                           /* Descriptor Size */
//        CY_U3P_USB_STRING_DESCR,        /* Device Descriptor Type */
//        'F',0x00,
//        'X',0x00,
//        '3',0x00

    	//Added by Daniel 11_2_2015
//		0x14,                           /* Descriptor Size */
//		CY_U3P_USB_STRING_DESCR,        /* Device Descriptor Type */
//		'M',0x00,
//		'I',0x00,
//		'N',0x00,
//		'I',0x00,
//		'S',0x00,
//		'C',0x00,
//		'O',0x00,
//		'P',0x00,
//		'E',0x00
    	//Added by Jill 6/2/18
		0x18,                           /* Descriptor Size */
		CY_U3P_USB_STRING_DESCR,        /* Device Descriptor Type */
		'M',0x00,
		'I',0x00,
		'N',0x00,
		'I',0x00,
		'S',0x00,
		'C',0x00,
		'O',0x00,
		'P',0x00,
		'E',0x00,
		'R',0x00,
		'1',0x00
    };

