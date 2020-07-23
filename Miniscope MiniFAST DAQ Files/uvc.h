/*
 ## Cypress FX3 Camera Kit header file (uvc.h)
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

/* This header file defines the UVC application constants and the Video Frame configurations */

#ifndef _INCLUDED_CYFXUVCAPP_H_
#define _INCLUDED_CYFXUVCAPP_H_

#include <cyu3types.h>
#include <cyu3usbconst.h>
#include <cyu3externcstart.h>

/* Definitions to enable/disable special features in this UVC application. */
/* #define UVC_PTZ_SUPPORT */           /* Enable if Pan, Tilt and Zoom controls are to be implemented. */
/* #define BACKFLOW_DETECT */           /* Enable if buffer overflow conditions are to be detected. */
#define DEBUG_PRINT_FRAME_COUNT 	   	/* Enable UART debug prints to print the frame count every end of frame */
/* #define USB_DEBUG_INTERFACE */       /* Enable custom USB interface for sensor interface debugging. */

/* UVC application thread parameters. */
#define UVC_APP_THREAD_STACK           (0x1000) /* Stack size for the video streaming thread is 4 KB. */
#define UVC_APP_THREAD_PRIORITY        (8)      /* Priority for the video streaming thread is 8. */

#define UVC_APP_EP0_THREAD_STACK       (0x0800) /* Stack size for the UVC control request thread is 2 KB. */
#define UVC_APP_EP0_THREAD_PRIORITY    (8)      /* Priority for the UVC control request thread is 8. */

/* DMA socket selection for UVC data transfer. */
#define CY_FX_EP_VIDEO_CONS_SOCKET      0x03    /* USB Consumer socket 3 is used for video data. */
#define CY_FX_EP_CONTROL_STATUS_SOCKET  0x02    /* USB Consumer socket 2 is used for the status pipe. */

/* Endpoint definition for UVC application */
#define CY_FX_EP_IN_TYPE                0x80    /* USB IN end points have MSB set */
#define CY_FX_EP_BULK_VIDEO             (CY_FX_EP_VIDEO_CONS_SOCKET | CY_FX_EP_IN_TYPE)         /* EP 3 IN */
#define CY_FX_EP_CONTROL_STATUS         (CY_FX_EP_CONTROL_STATUS_SOCKET | CY_FX_EP_IN_TYPE)     /* EP 2 IN */

#ifdef USB_DEBUG_INTERFACE
/* Socket and endpoint definitions for the USB debug interface. */
#define CY_FX_EP_DEBUG_CMD_SOCKET       0x04    /* USB Producer socket 4 is used as the debug command pipe. */
#define CY_FX_EP_DEBUG_RSP_SOCKET       0x04    /* USB Consumer socket 4 is used as the debug response pipe. */
#define CY_FX_EP_DEBUG_CMD              (CY_FX_EP_DEBUG_CMD_SOCKET)                             /* EP 4 OUT */
#define CY_FX_EP_DEBUG_RSP              (CY_FX_EP_DEBUG_RSP_SOCKET | CY_FX_EP_IN_TYPE)          /* EP 4 IN */
#endif

/* UVC Video Streaming Endpoint Packet Size */
#define CY_FX_EP_BULK_VIDEO_PKT_SIZE    (0x400)         /* 1024 Bytes */

/* UVC Video Streaming Endpoint Packet Count */
#define CY_FX_EP_BULK_VIDEO_PKTS_COUNT  (0x10)          /* 16 packets (burst of 16) per DMA buffer. */

/* DMA buffer size used for video streaming. */


#define CY_FX_UVC_STREAM_BUF_SIZE      (CY_FX_EP_BULK_VIDEO_PKTS_COUNT * CY_FX_EP_BULK_VIDEO_PKT_SIZE)  /* 16 KB */

/* Maximum video data that can be accommodated in one DMA buffer. */
#define CY_FX_UVC_BUF_FULL_SIZE        (CY_FX_UVC_STREAM_BUF_SIZE - 16)

/* Number of DMA buffers per GPIF DMA thread. */
#define CY_FX_UVC_STREAM_BUF_COUNT     (4)

/* Low Byte - UVC Video Streaming Endpoint Packet Size */
#define CY_FX_EP_BULK_VIDEO_PKT_SIZE_L  (uint8_t)(CY_FX_EP_BULK_VIDEO_PKT_SIZE & 0x00FF)

/* High Byte - UVC Video Streaming Endpoint Packet Size and No. of BULK packets */
#define CY_FX_EP_BULK_VIDEO_PKT_SIZE_H  (uint8_t)((CY_FX_EP_BULK_VIDEO_PKT_SIZE & 0xFF00) >> 8)

/* Event bits used for signaling the UVC application threads. */

/* Stream request event. Event flag that indicates that the streaming of video data has
   been enabled by the host. This flag is retained ON as long as video streaming is allowed,
   and is only turned off when the host indicates that data transfer should be stopped.
 */
#define CY_FX_UVC_STREAM_EVENT                  (1 << 0)

/* Abort streaming event. This event flag is set when the UVC host sends down a request
   (SET_INTERFACE or CLEAR_FEATURE) that indicates that video streaming should be stopped.
   The CY_FX_UVC_STREAM_EVENT event is cleared before setting this flag, and these two
   events can be considered as mutually exclusive.
 */
#define CY_FX_UVC_STREAM_ABORT_EVENT            (1 << 1)

/* UVC VIDEO_CONTROL_REQUEST event. This event flag indicates that a UVC class specific
   request addressed to the video control interface has been received. It should be cleared
   as soon as serviced by the firmware.
 */
#define CY_FX_UVC_VIDEO_CONTROL_REQUEST_EVENT   (1 << 2)

/* UVC VIDEO_STREAM_REQUEST event. This event flag indicates that a UVC class specific
   request addressed to the video streaming interface has been received. It should be cleared
   as soon as serviced by the firmware.
 */
#define CY_FX_UVC_VIDEO_STREAM_REQUEST_EVENT    (1 << 3)

#ifdef USB_DEBUG_INTERFACE
/* USB Debug Command event. This event flag indicates that a USB debug command has been
   received on the command endpoint.
 */
#define CY_FX_USB_DEBUG_CMD_EVENT               (1 << 4)
#endif

/*
   The following constants are taken from the USB and USB Video Class (UVC) specifications.
   They are defined here for convenient usage in the rest of the application source code.
 */
#define CY_FX_INTF_ASSN_DSCR_TYPE       (0x0B)          /* Type code for Interface Association Descriptor (IAD) */

#define CY_FX_USB_SETUP_REQ_TYPE_MASK   (uint32_t)(0x000000FF)  /* Mask for bmReqType field from a control request. */
#define CY_FX_USB_SETUP_REQ_MASK        (uint32_t)(0x0000FF00)  /* Mask for bRequest field from a control request. */
#define CY_FX_USB_SETUP_VALUE_MASK      (uint32_t)(0xFFFF0000)  /* Mask for wValue field from a control request. */
#define CY_FX_USB_SETUP_INDEX_MASK      (uint32_t)(0x0000FFFF)  /* Mask for wIndex field from a control request. */
#define CY_FX_USB_SETUP_LENGTH_MASK     (uint32_t)(0xFFFF0000)  /* Mask for wLength field from a control request. */

#define CY_FX_USB_SET_INTF_REQ_TYPE     (uint8_t)(0x01)         /* USB SET_INTERFACE Request Type. */
#define CY_FX_USB_SET_INTERFACE_REQ     (uint8_t)(0x0B)         /* USB SET_INTERFACE Request code. */

#define CY_FX_UVC_MAX_HEADER           (12)             /* Maximum UVC header size, in bytes. */
#define CY_FX_UVC_HEADER_DEFAULT_BFH   (0x8C)           /* Default BFH (Bit Field Header) for the UVC Header */
#define CY_FX_UVC_MAX_PROBE_SETTING    (26)             /* Maximum number of bytes in Probe Control */
#define CY_FX_UVC_MAX_PROBE_SETTING_ALIGNED (32)        /* Probe control data size aligned to 16 bytes. */

#define CY_FX_UVC_HEADER_FRAME          (0)                     /* UVC header value for normal frame indication */
#define CY_FX_UVC_HEADER_EOF            (uint8_t)(1 << 1)       /* UVC header value for end of frame indication */
#define CY_FX_UVC_HEADER_FRAME_ID       (uint8_t)(1 << 0)       /* Frame ID toggle bit in UVC header. */

#define CY_FX_USB_UVC_SET_REQ_TYPE      (uint8_t)(0x21)         /* UVC Interface SET Request Type */
#define CY_FX_USB_UVC_GET_REQ_TYPE      (uint8_t)(0xA1)         /* UVC Interface GET Request Type */
#define CY_FX_USB_UVC_GET_CUR_REQ       (uint8_t)(0x81)         /* UVC GET_CUR Request */
#define CY_FX_USB_UVC_SET_CUR_REQ       (uint8_t)(0x01)         /* UVC SET_CUR Request */
#define CY_FX_USB_UVC_GET_MIN_REQ       (uint8_t)(0x82)         /* UVC GET_MIN Request */
#define CY_FX_USB_UVC_GET_MAX_REQ       (uint8_t)(0x83)         /* UVC GET_MAX Request */
#define CY_FX_USB_UVC_GET_RES_REQ       (uint8_t)(0x84)         /* UVC GET_RES Request */
#define CY_FX_USB_UVC_GET_LEN_REQ       (uint8_t)(0x85)         /* UVC GET_LEN Request */
#define CY_FX_USB_UVC_GET_INFO_REQ      (uint8_t)(0x86)         /* UVC GET_INFO Request */
#define CY_FX_USB_UVC_GET_DEF_REQ       (uint8_t)(0x87)         /* UVC GET_DEF Request */

#define CY_FX_UVC_STREAM_INTERFACE      (uint8_t)(1)            /* Streaming Interface : Alternate Setting 1 */
#define CY_FX_UVC_CONTROL_INTERFACE     (uint8_t)(0)            /* Control Interface */
#define CY_FX_UVC_PROBE_CTRL            (uint16_t)(0x0100)      /* wValue setting used to access PROBE control. */
#define CY_FX_UVC_COMMIT_CTRL           (uint16_t)(0x0200)      /* wValue setting used to access COMMIT control. */

#define CY_FX_UVC_INTERFACE_CTRL        (uint8_t)(0)            /* wIndex value used to select UVC interface control. */
#define CY_FX_UVC_CAMERA_TERMINAL_ID    (uint8_t)(1)            /* wIndex value used to select Camera terminal. */
#define CY_FX_UVC_PROCESSING_UNIT_ID    (uint8_t)(2)            /* wIndex value used to select Processing Unit. */
#define CY_FX_UVC_EXTENSION_UNIT_ID     (uint8_t)(3)            /* wIndex value used to select Extension Unit. */

/* Processing Unit specific UVC control selector codes defined in the USB Video Class specification. */
#define CY_FX_UVC_PU_BACKLIGHT_COMPENSATION_CONTROL         (uint16_t)(0x0100)
#define CY_FX_UVC_PU_BRIGHTNESS_CONTROL                     (uint16_t)(0x0200)
#define CY_FX_UVC_PU_CONTRAST_CONTROL                       (uint16_t)(0x0300)
#define CY_FX_UVC_PU_GAIN_CONTROL                           (uint16_t)(0x0400)
#define CY_FX_UVC_PU_POWER_LINE_FREQUENCY_CONTROL           (uint16_t)(0x0500)
#define CY_FX_UVC_PU_HUE_CONTROL                            (uint16_t)(0x0600)
#define CY_FX_UVC_PU_SATURATION_CONTROL                     (uint16_t)(0x0700)
#define CY_FX_UVC_PU_SHARPNESS_CONTROL                      (uint16_t)(0x0800)
#define CY_FX_UVC_PU_GAMMA_CONTROL                          (uint16_t)(0x0900)
#define CY_FX_UVC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL      (uint16_t)(0x0A00)
#define CY_FX_UVC_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL (uint16_t)(0x0B00)
#define CY_FX_UVC_PU_WHITE_BALANCE_COMPONENT_CONTROL        (uint16_t)(0x0C00)
#define CY_FX_UVC_PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL   (uint16_t)(0x0D00)
#define CY_FX_UVC_PU_DIGITAL_MULTIPLIER_CONTROL             (uint16_t)(0x0E00)
#define CY_FX_UVC_PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL       (uint16_t)(0x0F00)
#define CY_FX_UVC_PU_HUE_AUTO_CONTROL                       (uint16_t)(0x1000)
#define CY_FX_UVC_PU_ANALOG_VIDEO_STANDARD_CONTROL          (uint16_t)(0x1100)
#define CY_FX_UVC_PU_ANALOG_LOCK_STATUS_CONTROL             (uint16_t)(0x1200)

/* Camera Terminal specific UVC control selector codes defined in the USB Video Class specification. */
#define CY_FX_UVC_CT_SCANNING_MODE_CONTROL                  (uint16_t)(0x0100)
#define CY_FX_UVC_CT_AE_MODE_CONTROL                        (uint16_t)(0x0200)
#define CY_FX_UVC_CT_AE_PRIORITY_CONTROL                    (uint16_t)(0x0300)
#define CY_FX_UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL         (uint16_t)(0x0400)
#define CY_FX_UVC_CT_EXPOSURE_TIME_RELATIVE_CONTROL         (uint16_t)(0x0500)
#define CY_FX_UVC_CT_FOCUS_ABSOLUTE_CONTROL                 (uint16_t)(0x0600)
#define CY_FX_UVC_CT_FOCUS_RELATIVE_CONTROL                 (uint16_t)(0x0700)
#define CY_FX_UVC_CT_FOCUS_AUTO_CONTROL                     (uint16_t)(0x0800)
#define CY_FX_UVC_CT_IRIS_ABSOLUTE_CONTROL                  (uint16_t)(0x0900)
#define CY_FX_UVC_CT_IRIS_RELATIVE_CONTROL                  (uint16_t)(0x0A00)
#define CY_FX_UVC_CT_ZOOM_ABSOLUTE_CONTROL                  (uint16_t)(0x0B00)
#define CY_FX_UVC_CT_ZOOM_RELATIVE_CONTROL                  (uint16_t)(0x0C00)
#define CY_FX_UVC_CT_PANTILT_ABSOLUTE_CONTROL               (uint16_t)(0x0D00)
#define CY_FX_UVC_CT_PANTILT_RELATIVE_CONTROL               (uint16_t)(0x0E00)
#define CY_FX_UVC_CT_ROLL_ABSOLUTE_CONTROL                  (uint16_t)(0x0F00)
#define CY_FX_UVC_CT_ROLL_RELATIVE_CONTROL                  (uint16_t)(0x1000)
#define CY_FX_UVC_CT_PRIVACY_CONTROL                        (uint16_t)(0x1100)

/* Extern definitions of the USB Enumeration constant arrays used for the UVC application.
   These arrays are defined in the cyfxuvcdscr.c file.
 */
extern const uint8_t CyFxUSBDeviceDscr[];               /* USB 2.0 Device descriptor. */
extern const uint8_t CyFxUSBDeviceDscrSS[];             /* USB 3.0 device descriptor. */

extern const uint8_t CyFxUSBDeviceQualDscr[];           /* USB 2.0 Device Qual descriptor. */
extern const uint8_t CyFxUSBBOSDscr[];                  /* USB 3.0 BOS descriptor. */

extern const uint8_t CyFxUSBFSConfigDscr[];             /* Full Speed Config descriptor. */
extern const uint8_t CyFxUSBHSConfigDscr[];             /* High Speed Config descriptor. */
extern const uint8_t CyFxUSBSSConfigDscr[];             /* USB 3.0 config descriptor. */

extern const uint8_t CyFxUSBStringLangIDDscr[];         /* String 0 descriptor. */
extern const uint8_t CyFxUSBManufactureDscr[];          /* Manufacturer string descriptor. */
extern const uint8_t CyFxUSBProductDscr[];              /* Product string descriptor. */

#include <cyu3externcend.h>

#endif /* _INCLUDED_CYFXUVCAPP_H_ */

/*[]*/

