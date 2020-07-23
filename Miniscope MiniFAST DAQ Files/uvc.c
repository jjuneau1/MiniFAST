/*
 ## Cypress FX3 Camera Kit Source file (uvc.c)
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

/* This project implements a USB Video Class device that streams uncompressed video
   data from an image sensor to a USB host PC.

   This firmware application makes use of two threads:
     1. The video streaming thread is responsible for handling the USB video streaming.
        If the UVC host has enabled video streaming, this thread continuously waits for
        a filled buffer, adds the appropriate UVC headers and commits the data. This
        thread also ensures the DMA multi-channel is reset and restarted at the end of
        each video frame. This thread is only idle when the UVC host has not enable video
        streaming.
     2. The UVC control thread handles UVC class specific requests that arrive on the
        control endpoint. The USB setup callback sets up events to notify this thread that
        a request has been received, and the thread handles them as soon as possible.
 */

#include <cyu3system.h>
#include <cyu3os.h>
#include <cyu3dma.h>
#include <cyu3error.h>
#include <cyu3usb.h>
#include <cyu3uart.h>
#include <cyu3gpif.h>
#include <cyu3i2c.h>
#include <cyu3spi.h> //Added by Daniel 4_9_2015
#include <cyu3gpio.h>
#include <cyu3pib.h>
#include <cyu3utils.h>

#include "definitions.h"
#include "uvc.h"
#include "sensor.h"
#include "camera_ptzcontrol.h"
#include "cyfxgpif2config.h"

/*************************************************************************************************
                                         Global Variables
 *************************************************************************************************/
static CyU3PThread   uvcAppThread;                      /* UVC video streaming thread. */
static CyU3PThread   uvcAppEP0Thread;                   /* UVC control request handling thread. */
static CyU3PEvent    glFxUVCEvent;                      /* Event group used to signal threads. */
CyU3PDmaMultiChannel glChHandleUVCStream;               /* DMA multi-channel handle. */

/* Current UVC control request fields. See USB specification for definition. */
uint8_t  bmReqType, bRequest;                           /* bmReqType and bRequest fields. */
uint16_t wValue, wIndex, wLength;                       /* wValue, wIndex and wLength fields. */

CyBool_t        isUsbConnected = CyFalse;               /* Whether USB connection is active. */
CyU3PUSBSpeed_t usbSpeed = CY_U3P_NOT_CONNECTED;        /* Current USB connection speed. */
CyBool_t        clearFeatureRqtReceived = CyFalse;      /* Whether a CLEAR_FEATURE (stop streaming) request has been
                                                           received. */
CyBool_t        streamingStarted = CyFalse;             /* Whether USB host has started streaming data */
#ifdef BACKFLOW_DETECT
uint8_t back_flow_detected = 0;                         /* Whether buffer overflow error is detected. */
#endif

#ifdef USB_DEBUG_INTERFACE
CyU3PDmaChannel  glDebugCmdChannel;                     /* Channel to receive debug commands on. */
CyU3PDmaChannel  glDebugRspChannel;                     /* Channel to send debug responses on. */
uint8_t         *glDebugRspBuffer;                      /* Buffer used to send debug responses. */
#endif

//Added by Daniel 6_22_2015
uint8_t recording = 0;

//----------------------------------

/* UVC Probe Control Settings for a USB 3.0 connection. */
uint8_t glProbeCtrl[CY_FX_UVC_MAX_PROBE_SETTING] = {
    0x00, 0x00,                 /* bmHint : no hit */
    0x01,                       /* Use 1st Video format index */
    0x01,                       /* Use 1st Video frame index */
 //   0x15, 0x16, 0x05, 0x00,     /* Desired frame interval in the unit of 100ns: 30 fps */ //60fps: 0x0B, 0x8B, 0x02, 0x00,
    0x80, 0x84, 0x1E, 0x00,     /* Desired frame interval in the unit of 100ns: 5 fps */ //60fps: 0x0B, 0x8B, 0x02, 0x00,
    0x00, 0x00,                 /* Key frame rate in key frame/video frame units: only applicable
                                   to video streaming with adjustable compression parameters */
    0x00, 0x00,                 /* PFrame rate in PFrame / key frame units: only applicable to
                                   video streaming with adjustable compression parameters */
    0x00, 0x00,                 /* Compression quality control: only applicable to video streaming
                                   with adjustable compression parameters */
    0x00, 0x00,                 /* Window size for average bit rate: only applicable to video
                                   streaming with adjustable compression parameters */
    0x00, 0x00,                 /* Internal video streaming i/f latency in ms */
//    0x00, 0x04, 0x0B, 0x00,   /* Max video frame size in bytes Original Value*/
    0xD0, 0xFC, 0x41, 0x00,   /* Max video frame size in bytes Original Value*/
//   0xC0,0x67,0x03,0x00,        /* Max video frame size in bytes (352*317*2)*/

    0x00, 0x40, 0x00, 0x00      /* No. of bytes device can rx in single payload = 16 KB */
};

/* UVC Probe Control Setting for a USB 2.0 connection. */
uint8_t glProbeCtrl20[CY_FX_UVC_MAX_PROBE_SETTING] = {
    0x00, 0x00,                 /* bmHint : no hit */
    0x01,                       /* Use 1st Video format index */
    0x01,                       /* Use 1st Video frame index */
    0x0B, 0x8B, 0x02, 0x00,     /* Desired frame interval in the unit of 100ns: 15 fps */
    0x00, 0x00,                 /* Key frame rate in key frame/video frame units: only applicable
                                   to video streaming with adjustable compression parameters */
    0x00, 0x00,                 /* PFrame rate in PFrame / key frame units: only applicable to
                                   video streaming with adjustable compression parameters */
    0x00, 0x00,                 /* Compression quality control: only applicable to video streaming
                                   with adjustable compression parameters */
    0x00, 0x00,                 /* Window size for average bit rate: only applicable to video
                                   streaming with adjustable compression parameters */
    0x00, 0x00,                 /* Internal video streaming i/f latency in ms */


    0x00, 0x04, 0x0B, 0x00,     /* Max video frame size in bytes*/
    0x00, 0x40, 0x00, 0x00      /* No. of bytes device can rx in single payload = 16 KB */
};
/* Video Probe Commit Control. This array is filled out when the host sends down the SET_CUR request. */
static uint8_t glCommitCtrl[CY_FX_UVC_MAX_PROBE_SETTING_ALIGNED];

/* Scratch buffer used for handling UVC class requests with a data phase. */
static uint8_t glEp0Buffer[32];

/* UVC Header to be prefixed at the top of each 16 KB video data buffer. */
uint8_t volatile glUVCHeader[CY_FX_UVC_MAX_HEADER] =
{
    0x0C,                               /* Header Length */
    0x8C,                               /* Bit field header field */
    0x00, 0x00, 0x00, 0x00,             /* Presentation time stamp field */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00  /* Source clock reference field */
};

volatile static CyBool_t hitFV = CyFalse;               /* Whether end of frame (FV) signal has been hit. */
volatile static CyBool_t gpif_initialized = CyFalse;    /* Whether the GPIF init function has been called. */
volatile static uint16_t prodCount = 0, consCount = 0;  /* Count of buffers received and committed during
                                                           the current video frame. */
volatile static CyBool_t gotPartial = CyFalse;          /* Helps track the last partial buffer
                                                         * to make sure it is committed to USB.
                                                         */

/* Add the UVC packet header to the top of the specified DMA buffer. */
void
CyFxUVCAddHeader (
        uint8_t *buffer_p,              /* Buffer pointer */
        uint8_t frameInd                /* EOF or normal frame indication */
        )
{
    /* Copy header to buffer */
    CyU3PMemCopy (buffer_p, (uint8_t *)glUVCHeader, CY_FX_UVC_MAX_HEADER);

    /* The EOF flag needs to be set if this is the last packet for this video frame. */
    if (frameInd & CY_FX_UVC_HEADER_EOF)
    {
        buffer_p[1] |= CY_FX_UVC_HEADER_EOF;
    }
}


/* Application Error Handler */
void
CyFxAppErrorHandler (
        CyU3PReturnStatus_t apiRetStatus    /* API return status */
        )
{
    /* This function is hit when we have hit a critical application error. This is not
       expected to happen, and the current implementation of this function does nothing
       except stay in a loop printing error messages through the UART port.

       This function can be modified to take additional error handling actions such
       as cycling the USB connection or performing a warm reset.
     */
//    for (;;)
//    {
        CyU3PDebugPrint (4, "Error handler...\r\n");
//        CyU3PThreadSleep (1000);
//    }
}

/* This function performs the operations for a Video Streaming Abort.
   This is called every time there is a USB reset, suspend or disconnect event.
 */
static void
CyFxUVCApplnAbortHandler (
        void)
{
	uint32_t flag;
	if (CyU3PEventGet (&glFxUVCEvent, CY_FX_UVC_STREAM_EVENT, CYU3P_EVENT_AND, &flag,CYU3P_NO_WAIT) == CY_U3P_SUCCESS)
	{
        /* Clear the Video Stream Request Event */
        CyU3PEventSet (&glFxUVCEvent, ~(CY_FX_UVC_STREAM_EVENT), CYU3P_EVENT_AND);

        /* Set Video Stream Abort Event */
        CyU3PEventSet (&glFxUVCEvent, CY_FX_UVC_STREAM_ABORT_EVENT, CYU3P_EVENT_OR);
	}
}

/* This is the Callback function to handle the USB Events */
static void
CyFxUVCApplnUSBEventCB (
        CyU3PUsbEventType_t evtype,  /* Event type */
        uint16_t             evdata  /* Event data */
        )
{
    switch (evtype)
    {
        case CY_U3P_USB_EVENT_RESET:
            CyU3PDebugPrint (4, "RESET encountered...\r\n");
            CyU3PGpifDisable (CyTrue);
            gpif_initialized = 0;
            streamingStarted = CyFalse;
            CyFxUVCApplnAbortHandler ();
            break;

        case CY_U3P_USB_EVENT_SUSPEND:
            CyU3PDebugPrint (4, "SUSPEND encountered...\r\n");
            CyU3PGpifDisable (CyTrue);
            gpif_initialized = 0;
            streamingStarted = CyFalse;
            CyFxUVCApplnAbortHandler ();
            break;

        case CY_U3P_USB_EVENT_DISCONNECT:
            CyU3PDebugPrint (4, "USB disconnected...\r\n");
            CyU3PGpifDisable (CyTrue);
            gpif_initialized = 0;
            isUsbConnected = CyFalse;
            streamingStarted = CyFalse;
            CyFxUVCApplnAbortHandler ();
            break;

#ifdef BACKFLOW_DETECT
        case CY_U3P_USB_EVENT_EP_UNDERRUN:
            CyU3PDebugPrint (4, "CY_U3P_USB_EVENT_EP_UNDERRUN encountered...\r\n");
            break;
#endif

        default:
            break;
    }
}

/* Callback to handle the USB Setup Requests and UVC Class events */
static CyBool_t
CyFxUVCApplnUSBSetupCB (
        uint32_t setupdat0, /* SETUP Data 0 */
        uint32_t setupdat1  /* SETUP Data 1 */
        )
{
    CyBool_t uvcHandleReq = CyFalse;
    uint32_t status;

    /* Obtain Request Type and Request */
    bmReqType = (uint8_t)(setupdat0 & CY_FX_USB_SETUP_REQ_TYPE_MASK);
    bRequest  = (uint8_t)((setupdat0 & CY_FX_USB_SETUP_REQ_MASK) >> 8);
    wValue    = (uint16_t)((setupdat0 & CY_FX_USB_SETUP_VALUE_MASK) >> 16);
    wIndex    = (uint16_t)(setupdat1 & CY_FX_USB_SETUP_INDEX_MASK);
    wLength   = (uint16_t)((setupdat1 & CY_FX_USB_SETUP_LENGTH_MASK) >> 16);

    /* Check for UVC Class Requests */
    switch (bmReqType)
    {
        case CY_FX_USB_UVC_GET_REQ_TYPE:
        case CY_FX_USB_UVC_SET_REQ_TYPE:
            /* UVC Specific requests are handled in the EP0 thread. */
            switch (wIndex & 0xFF)
            {
                case CY_FX_UVC_CONTROL_INTERFACE:
                    {
                        uvcHandleReq = CyTrue;
                        status = CyU3PEventSet (&glFxUVCEvent, CY_FX_UVC_VIDEO_CONTROL_REQUEST_EVENT,
                                CYU3P_EVENT_OR);
                        if (status != CY_U3P_SUCCESS)
                        {
                            CyU3PDebugPrint (4, "Set CY_FX_UVC_VIDEO_CONTROL_REQUEST_EVENT Failed %x\n", status);
                            CyU3PUsbStall (0, CyTrue, CyFalse);
                        }
                    }
                    break;

                case CY_FX_UVC_STREAM_INTERFACE:
                    {
                        uvcHandleReq = CyTrue;
                        status = CyU3PEventSet (&glFxUVCEvent, CY_FX_UVC_VIDEO_STREAM_REQUEST_EVENT,
                                CYU3P_EVENT_OR);
                        if (status != CY_U3P_SUCCESS)
                        {
                            /* Error handling */
                            CyU3PDebugPrint (4, "Set CY_FX_UVC_VIDEO_STREAM_REQUEST_EVENT Failed %x\n", status);
                            CyU3PUsbStall (0, CyTrue, CyFalse);
                        }
                    }
                    break;

                default:
                    break;
            }
            break;

        case CY_FX_USB_SET_INTF_REQ_TYPE:
            if (bRequest == CY_FX_USB_SET_INTERFACE_REQ)
            {
            	/* MAC OS sends Set Interface Alternate Setting 0 command after
            	 * stopping to stream. This application needs to stop streaming. */
                if ((wIndex == CY_FX_UVC_STREAM_INTERFACE) && (wValue == 0))
                {
                	/* Stop GPIF state machine to stop data transfers through FX3 */
                	CyU3PDebugPrint (4, "Alternate setting 0..\r\n");
                    CyU3PGpifDisable (CyTrue);
                    gpif_initialized = 0;
                    streamingStarted = CyFalse;
                    /* Place the EP in NAK mode before cleaning up the pipe. */
                    CyU3PUsbSetEpNak (CY_FX_EP_BULK_VIDEO, CyTrue);
                    CyU3PBusyWait (100);

                    /* Reset and flush the endpoint pipe. */
                    CyU3PDmaMultiChannelReset (&glChHandleUVCStream);
                    CyU3PUsbFlushEp (CY_FX_EP_BULK_VIDEO);
                    CyU3PUsbSetEpNak (CY_FX_EP_BULK_VIDEO, CyFalse);
                    CyU3PBusyWait (100);

                    /* Clear the stall condition and sequence numbers. */
                    CyU3PUsbStall (CY_FX_EP_BULK_VIDEO, CyFalse, CyTrue);
                    uvcHandleReq = CyTrue;
                    /* Complete Control request handshake */
                    CyU3PUsbAckSetup ();
                    /* Indicate stop streaming to main thread */
                    clearFeatureRqtReceived = CyTrue;
                    CyFxUVCApplnAbortHandler ();

                }
            }
            break;

        case CY_U3P_USB_TARGET_ENDPT:
            if (bRequest == CY_U3P_USB_SC_CLEAR_FEATURE)
            {
                if (wIndex == CY_FX_EP_BULK_VIDEO)
                {
                	/* Windows OS sends Clear Feature Request after it stops streaming,
                	 * however MAC OS sends clear feature request right after it sends a
                	 * Commit -> SET_CUR request. Hence, stop streaming only of streaming
                	 * has started. */
                    if (streamingStarted == CyTrue)
                    {
                        CyU3PDebugPrint (4, "Clear feature request detected..\r\n");

                        /* Disable the GPIF state machine. */
                        CyU3PGpifDisable (CyTrue);
                        gpif_initialized = 0;
                        streamingStarted = CyFalse;

                        /* Place the EP in NAK mode before cleaning up the pipe. */
                        CyU3PUsbSetEpNak (CY_FX_EP_BULK_VIDEO, CyTrue);
                        CyU3PBusyWait (100);

                        /* Reset and flush the endpoint pipe. */
                        CyU3PDmaMultiChannelReset (&glChHandleUVCStream);
                        CyU3PUsbFlushEp (CY_FX_EP_BULK_VIDEO);
                        CyU3PUsbSetEpNak (CY_FX_EP_BULK_VIDEO, CyFalse);
                        CyU3PBusyWait (100);

                        /* Clear the stall condition and sequence numbers. */
                        CyU3PUsbStall (CY_FX_EP_BULK_VIDEO, CyFalse, CyTrue);

                        uvcHandleReq = CyTrue;
                        /* Complete Control request handshake */
                        CyU3PUsbAckSetup ();
                        /* Indicate stop streaming to main thread */
                        clearFeatureRqtReceived = CyTrue;
                        CyFxUVCApplnAbortHandler ();
                    }
                    else
                    {
                        uvcHandleReq = CyTrue;
                        CyU3PUsbAckSetup ();
                    }
                }
            }
            break;

        default:
            break;
    }

    /* Return status of request handling to the USB driver */
    return uvcHandleReq;
}

/* DMA callback providing notification when each buffer has been sent out to the USB host.
 * This is used to track whether all of the data has been sent out.
 */
void
CyFxUvcApplnDmaCallback (
        CyU3PDmaMultiChannel *multiChHandle,
        CyU3PDmaCbType_t      type,
        CyU3PDmaCBInput_t    *input
        )
{
    if (type == CY_U3P_DMA_CB_CONS_EVENT)
    {
        consCount++;
        streamingStarted = CyTrue;
    }
}

/*
 * This function is called from the GPIF callback when we have reached the end of a video frame.
 * The DMA buffer containing the last part of the frame may not have been committed, and need to
 * be manually wrapped up. This function uses the current GPIF state ID to identify the socket on
 * which this last buffer is pending, and then uses the CyU3PDmaMultiChannelSetWrapUp function
 * to commit the buffer.
 */
static uint8_t
CyFxUvcAppCommitEOF (
        CyU3PDmaMultiChannel *handle,           /* Handle to DMA channel. */
        uint8_t stateId                         /* Current GPIF state ID. */
        )
{
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
    uint8_t socket = 0xFF;      /*  Invalid value. */




    // Added by Daniel 6_22_2015 to output Frame Capture Trigger -------
//    if (recording == 1) {
//		CyU3PReturnStatus_t status;
//		CyBool_t pinState;
//		CyU3PGpioGetValue(FRAME_OUT,&pinState);
//		if (pinState == CyTrue) {
//			status = CyU3PGpioSetValue(FRAME_OUT, CyFalse);
//			if (status != CY_U3P_SUCCESS) {
//				CyU3PDebugPrint(4, "GPIO Set Value Error, Error Code = %d\n",
//						status);
//			}
//		}
//		else {
//			status = CyU3PGpioSetValue(FRAME_OUT, CyTrue);
//			if (status != CY_U3P_SUCCESS) {
//				CyU3PDebugPrint(4, "GPIO Set Value Error, Error Code = %d\n",
//						status);
//			}
//		}
 //   }

    //------------------------------------------------------------------

    /* Verify that the current state is a terminal state for the GPIF state machine. */
    switch (stateId)
    {
    case FULL_BUF_IN_SCK0:
    case FULL_BUF_IN_SCK1:
         /* Buffer is already full and would have been committed. Do nothing. */
         break;

     case PARTIAL_BUF_IN_SCK0:
         socket = 0;
         break;

     case PARTIAL_BUF_IN_SCK1:
         socket = 1;
         break;

     default:
         /* Unexpected current state. Return error. */
         return 1;
 }


    if (socket != 0xFF)
    {
        /* We have a partial buffer. Commit the buffer manually. The Wrap Up API, here, helps produce a
           partially filled buffer on the producer side. This action will cause CyU3PDmaMultiChannelGetBuffer API
           in the UVCAppThread_Entry function to succeed one more time with less than full producer buffer count */



    	apiRetStatus = CyU3PDmaMultiChannelSetWrapUp (handle, socket);
        if (apiRetStatus != CY_U3P_SUCCESS)
        {
            CyU3PDebugPrint (4, "Channel Set WrapUp failed, Error code = %d\r\n", apiRetStatus);
            CyFxAppErrorHandler (apiRetStatus);
        }
        else
        {
			gotPartial = CyTrue; /* Flag is set to indicate the partial buffer is acquired */

        }
    }

    return 0;
}

/* GpifCB callback function is invoked when FV triggers GPIF interrupt */
void
CyFxGpifCB (
        CyU3PGpifEventType event,
        uint8_t currentState

        )
{

	if (event == CYU3P_GPIF_EVT_SM_INTERRUPT)
    {
        hitFV = CyTrue;
        if (CyFxUvcAppCommitEOF (&glChHandleUVCStream, currentState) != CY_U3P_SUCCESS)
	            CyU3PDebugPrint (4, "Commit EOF failed!\n");
    }



}

/* This function initializes the Debug Module for the UVC Application */
static void
CyFxUVCApplnDebugInit (
        void)
{
    CyU3PUartConfig_t uartConfig;
    CyU3PReturnStatus_t apiRetStatus;

    /* Initialize the UART for printing debug messages */
    apiRetStatus = CyU3PUartInit ();
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "UART initialization failed!\n");
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Set UART Configuration */
    uartConfig.baudRate = CY_U3P_UART_BAUDRATE_115200;
    uartConfig.stopBit  = CY_U3P_UART_ONE_STOP_BIT;
    uartConfig.parity   = CY_U3P_UART_NO_PARITY;
    uartConfig.txEnable = CyTrue;
    uartConfig.rxEnable = CyFalse;
    uartConfig.flowCtrl = CyFalse;
    uartConfig.isDma    = CyTrue;

    /* Set the UART configuration */
    apiRetStatus = CyU3PUartSetConfig (&uartConfig, NULL);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Set the UART transfer */
    apiRetStatus = CyU3PUartTxSetBlockXfer (0xFFFFFFFF);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Initialize the Debug logger module. */
    apiRetStatus = CyU3PDebugInit (CY_U3P_LPP_SOCKET_UART_CONS, 4);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Disable log message headers. */
    CyU3PDebugPreamble (CyFalse);
}

/* I2C initialization. */
static void
CyFxUVCApplnI2CInit (void)
{
    CyU3PI2cConfig_t i2cConfig;
    CyU3PReturnStatus_t status;

    status = CyU3PI2cInit ();
    if (status != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "I2C initialization failed!\n");
        CyFxAppErrorHandler (status);
    }

    /*  Set I2C Configuration */
    i2cConfig.bitRate    = 100000;      /*  100 KHz */
    i2cConfig.isDma      = CyFalse;
    i2cConfig.busTimeout = 0x00ffffffU; //0xffffffffU; //Changed by Daniel 8/7/2015
    i2cConfig.dmaTimeout = 0xffff;


    status = CyU3PI2cSetConfig (&i2cConfig, 0);
    if (CY_U3P_SUCCESS != status)
    {
        CyU3PDebugPrint (4, "I2C configuration failed!\n");
        CyFxAppErrorHandler (status);
    }
}

/* SPI initialization. */ //Added by Daniel 4_9_2015
static void
CyFxUVCApplnSPIInit (void)
{

	CyU3PSpiConfig_t spiConfig;
	CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

	/* Start the SPI module and configure the master. */
	status = CyU3PSpiInit();
	if (status != CY_U3P_SUCCESS)
	{
		CyU3PDebugPrint (4, "SPI initialization failed!\n");
		CyFxAppErrorHandler (status);
	}

	/* Start the SPI master block. Run the SPI clock at 8MHz
	 * and configure the word length to 8 bits. Also configure
	 * the slave select using FW. */
	CyU3PMemSet ((uint8_t *)&spiConfig, 0, sizeof(spiConfig));
	spiConfig.isLsbFirst = CyFalse; //False is MSB first
	spiConfig.cpol       = CyTrue;  //True clk idles high
	spiConfig.ssnPol     = CyFalse; //False is active low
	spiConfig.cpha       = CyTrue;  //True: Transmits data going from idle (de-assert) to assert. Latches data on assert to de-assert
	spiConfig.leadTime   = CY_U3P_SPI_SSN_LAG_LEAD_HALF_CLK;
	spiConfig.lagTime    = CY_U3P_SPI_SSN_LAG_LEAD_HALF_CLK;
	spiConfig.ssnCtrl    = CY_U3P_SPI_SSN_CTRL_FW;
	spiConfig.clock      = 8000000;
	spiConfig.wordLen    = 8; //DAC uses 16 bit words

	status = CyU3PSpiSetConfig (&spiConfig, NULL);
	 if (CY_U3P_SUCCESS != status)
	{
		CyU3PDebugPrint (4, "SPI configuration failed!\n");
		CyFxAppErrorHandler (status);
	}
	 CyU3PSpiSetSsnLine (CyTrue);
}

/* Handles general communication between DAQ software and PCB. */ //Added by Daniel 6_24_2015
CyU3PReturnStatus_t handleCommunication (
        uint8_t  value)
{
	switch(value) {
		case RECORD_START:
			recording = 1;
			break;
		case RECORD_END:
			recording = 0;
			break;
		case SET_CMOS_SETTINGS:
			SensorInit();
			break;
//Added new sections and commented out old one Jill 8/30/18
		case INIT_F:
			SensorSetFPS(1);
		break;
		case FPS_8:
			SensorSetFPS(2);
		break;
		case FPS_30:
			SensorSetFPS(3);
		break;
		case FPS_60:
			SensorSetFPS(4);
		break;
		case FPS_100:
			SensorSetFPS(5);
		break;
		case FPS_153:
			SensorSetFPS(6);
		break;
		case FPS_200:
			SensorSetFPS(7);
		break;
		case FPS_250:
			SensorSetFPS(8);
		break;
		case FPS_293:
			SensorSetFPS(9);
		break;
		case INIT_WIN:
			SensorSetWindow(1);
		break;
		case WIN0:
			SensorSetWindow(2);
		break;
		case WIN50:
			SensorSetWindow(3);
		break;
		case WIN100:
			SensorSetWindow(4);
		break;
		case WIN150:
			SensorSetWindow(5);
		break;
		case WIN200:
			SensorSetWindow(6);
		break;
		case WIN250:
			SensorSetWindow(7);
		break;
		case WIN300:
			SensorSetWindow(8);
		break;
		case WIN350:
			SensorSetWindow(9);
		break;
		case WIN400:
			SensorSetWindow(10);
		break;
		case WIN450:
			SensorSetWindow(11);
		break;
		case WIN500:
			SensorSetWindow(12);
		break;
		case WIN550:
			SensorSetWindow(13);
		break;
		case WIN600:
			SensorSetWindow(14);
		break;
		case WIN650:
			SensorSetWindow(15);
		break;
		case WIN700:
			SensorSetWindow(16);
		break;
		case WIN750:
			SensorSetWindow(17);
		break;
		case WIN800:
			SensorSetWindow(18);
		break;
		case WIN850:
			SensorSetWindow(19);
		break;
		case WIN900:
			SensorSetWindow(20);
		break;
		case WIN950:
			SensorSetWindow(21);
		break;
		case HGC_ON:
			SensorSetFPS(10);
		break;
		case HGC_OFF:
			SensorSetFPS(11);
		break;
		case FPS_500:
			SensorSetFPS(12);
		break;
		case FPS_700:
			SensorSetFPS(13);
		break;
		case FPS_400:
			SensorSetFPS(14);
		break;
		case FPS_1:
			SensorSetFPS(15);
		break;
		case FPS_2:
			SensorSetFPS(16);
		break;
		case FPS_3:
			SensorSetFPS(17);
		break;
		case FPS_4:
			SensorSetFPS(18);
		break;
		default:
		break;
	}
	return CY_U3P_SUCCESS;
}

/* SPI read / write for DAC MCP4922. */ //Added by Daniel 4_9_2015
CyU3PReturnStatus_t CyFxDACSpiWrite (
        uint8_t  *buffer)
{
	uint8_t bufferTemp[2];
//	bufferTemp[0] = (buffer[0]>>4)|0x30;//buffer[1]; Commented next 2 lines Jill
//	bufferTemp[1] = buffer[0]<<4;//buffer[0];
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

	CyU3PSpiSetSsnLine (CyFalse);
	status = CyU3PSpiTransmitWords (buffer, 2);
	if (status != CY_U3P_SUCCESS)
	{
		CyU3PDebugPrint (2, "SPI WRITE command failed\r\n");
		CyU3PSpiSetSsnLine (CyTrue);
		return status;
	}

	CyU3PSpiSetSsnLine (CyTrue);

	//Toggle LDAC pin low then high
/*	status = CyU3PGpioSetValue(DAC_LDAC, CyFalse);
	if (status != CY_U3P_SUCCESS) {
		CyU3PDebugPrint(4, "GPIO Set Value Error, Error Code = %d\n",
				status);
	}
	CyU3PThreadSleep (10);
	status = CyU3PGpioSetValue(DAC_LDAC, CyTrue);
		if (status != CY_U3P_SUCCESS) {
			CyU3PDebugPrint(4, "GPIO Set Value Error, Error Code = %d\n",
					status);
		}
*/
    CyU3PThreadSleep (10);

    return CY_U3P_SUCCESS;
}

CyU3PReturnStatus_t CyFxDACI2CWrite (
        uint8_t  *buffer)
{
	CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
	CyU3PI2cPreamble_t preamble;

	uint8_t bufferTemp[1];
	bufferTemp[0] = buffer[0]<<4;

	preamble.buffer[0] = DAC_ADDR_WR;
	preamble.buffer[1] = buffer[0]>>4;
	preamble.length    = 2;
	preamble.ctrlMask  = 0x0000;

	apiRetStatus = CyU3PI2cTransmitBytes (&preamble, bufferTemp, 1, 0);

	if (apiRetStatus == CY_U3P_SUCCESS)
		CyU3PBusyWait (100);
	else
		CyU3PDebugPrint (2, "I2C DAC WRITE command failed\r\n");

    return apiRetStatus;
}

#ifdef BACKFLOW_DETECT
static void CyFxUvcAppPibCallback (
        CyU3PPibIntrType cbType,
        uint16_t cbArg)
{
    if ((cbType == CYU3P_PIB_INTR_ERROR) && ((cbArg == 0x1005) || (cbArg == 0x1006)))
    {
        if (!back_flow_detected)
        {
            CyU3PDebugPrint (4, "Backflow detected...\r\n");
            back_flow_detected = 1;
        }
    }
}
#endif

#ifdef USB_DEBUG_INTERFACE
static void
CyFxUvcAppDebugCallback (
        CyU3PDmaChannel   *handle,
        CyU3PDmaCbType_t   type,
        CyU3PDmaCBInput_t *input)
{
    if (type == CY_U3P_DMA_CB_PROD_EVENT)
    {
        /* Data has been received. Notify the EP0 thread which handles the debug commands as well. */
        CyU3PEventSet (&glFxUVCEvent, CY_FX_USB_DEBUG_CMD_EVENT, CYU3P_EVENT_OR);
    }
}
#endif

/* This function initializes the USB Module, creates event group,
   sets the enumeration descriptors, configures the Endpoints and
   configures the DMA module for the UVC Application */
static void
CyFxUVCApplnInit (void)
{
    CyU3PDmaMultiChannelConfig_t dmaMultiConfig;
    CyU3PEpConfig_t              endPointConfig;
    CyU3PReturnStatus_t          apiRetStatus;
    CyU3PGpioClock_t             gpioClock;
    CyU3PGpioSimpleConfig_t      gpioConfig;
    CyU3PPibClock_t              pibclock;

#ifdef USB_DEBUG_INTERFACE
    CyU3PDmaChannelConfig_t channelConfig;
#endif

    /* Create UVC event group */
    apiRetStatus = CyU3PEventCreate (&glFxUVCEvent);
    if (apiRetStatus != 0)
    {
        CyU3PDebugPrint (4, "UVC Create Event failed, Error Code = %d\n", apiRetStatus);
        CyFxAppErrorHandler (apiRetStatus);
    }

#ifdef UVC_PTZ_SUPPORT
    CyFxUvcAppPTZInit ();
#endif

    isUsbConnected = CyFalse;
    clearFeatureRqtReceived = CyFalse;

    /* Init the GPIO module */
    gpioClock.fastClkDiv = 2;
    gpioClock.slowClkDiv = 2;
    gpioClock.simpleDiv  = CY_U3P_GPIO_SIMPLE_DIV_BY_2;
    gpioClock.clkSrc     = CY_U3P_SYS_CLK;
    gpioClock.halfDiv    = 0;

    /* Initialize Gpio interface */
    apiRetStatus = CyU3PGpioInit (&gpioClock, NULL);
    if (apiRetStatus != 0)
    {
        CyU3PDebugPrint (4, "GPIO Init failed, Error Code = %d\n", apiRetStatus);
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* CTL pins are restricted and cannot be configured using I/O matrix configuration function,
     * must use GpioOverride to configure it */

    //Commented by Daniel 3_27_2015
    //Uncommented by Jill 6/18/18 For Testing

    apiRetStatus = CyU3PDeviceGpioOverride (GPIO_TEST, CyTrue);
    if (apiRetStatus != 0)
    {
        CyU3PDebugPrint (4, "GPIO Override failed, Error Code = %d\n", apiRetStatus);
        CyFxAppErrorHandler (apiRetStatus);
    }

//    /* SENSOR_RESET_GPIO is the Sensor reset pin */
    gpioConfig.outValue    = CyTrue;
    gpioConfig.driveLowEn  = CyTrue;
    gpioConfig.driveHighEn = CyTrue;
    gpioConfig.inputEn     = CyFalse;
    gpioConfig.intrMode    = CY_U3P_GPIO_NO_INTR;
    apiRetStatus           = CyU3PGpioSetSimpleConfig (GPIO_TEST, &gpioConfig);
   if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "GPIO Set Config Error, Error Code = %d\n", apiRetStatus);
        CyFxAppErrorHandler (apiRetStatus);
   }


    //-----------Added by Daniel 4_9_2015
/*	apiRetStatus = CyU3PDeviceGpioOverride (DAC_LDAC, CyTrue);
	if (apiRetStatus != 0)
	{
		CyU3PDebugPrint (4, "GPIO Override failed, Error Code = %d\n", apiRetStatus);
		CyFxAppErrorHandler (apiRetStatus);
	}

	/* DAC_LDAC is the DAC Latch pin */
	gpioConfig.outValue    = CyFalse;
	gpioConfig.driveLowEn  = CyTrue;
	gpioConfig.driveHighEn = CyTrue;
	gpioConfig.inputEn     = CyTrue;
	gpioConfig.intrMode    = CY_U3P_GPIO_NO_INTR;
	apiRetStatus           = CyU3PGpioSetSimpleConfig (DAC_LDAC, &gpioConfig);
/*	if (apiRetStatus != CY_U3P_SUCCESS)
	{
		CyU3PDebugPrint (4, "GPIO Set Config Error, Error Code = %d\n", apiRetStatus);
		CyFxAppErrorHandler (apiRetStatus);
	}

	// For DAC shdn pin
	apiRetStatus = CyU3PDeviceGpioOverride (DAC_SHDN, CyTrue);
	if (apiRetStatus != 0)
	{
		CyU3PDebugPrint (4, "GPIO Override failed, Error Code = %d\n", apiRetStatus);
		CyFxAppErrorHandler (apiRetStatus);
	}

	/* DAC_SHDN is the DAC nShutdown pin */
	gpioConfig.outValue    = CyFalse;
	gpioConfig.driveLowEn  = CyTrue;
	gpioConfig.driveHighEn = CyTrue;
	gpioConfig.inputEn     = CyTrue;
	gpioConfig.intrMode    = CY_U3P_GPIO_NO_INTR;
	apiRetStatus           = CyU3PGpioSetSimpleConfig (DAC_SHDN, &gpioConfig);
/*	if (apiRetStatus != CY_U3P_SUCCESS)
	{
		CyU3PDebugPrint (4, "GPIO Set Config Error, Error Code = %d\n", apiRetStatus);
		CyFxAppErrorHandler (apiRetStatus);
	}
*/
	// SMA output of frame acquisition
	apiRetStatus = CyU3PDeviceGpioOverride (FRAME_OUT, CyTrue);
		if (apiRetStatus != 0)
		{
			CyU3PDebugPrint (4, "GPIO Override failed, Error Code = %d\n", apiRetStatus);
			CyFxAppErrorHandler (apiRetStatus);
		}

		/* FRAME Capture Output */
		gpioConfig.outValue    = CyFalse;
		gpioConfig.driveLowEn  = CyTrue;
		gpioConfig.driveHighEn = CyTrue;
		gpioConfig.inputEn     = CyFalse;
		gpioConfig.intrMode    = CY_U3P_GPIO_NO_INTR;
		apiRetStatus           = CyU3PGpioSetSimpleConfig (FRAME_OUT, &gpioConfig);
		if (apiRetStatus != CY_U3P_SUCCESS)
		{
			CyU3PDebugPrint (4, "GPIO Set Config Error, Error Code = %d\n", apiRetStatus);
			CyFxAppErrorHandler (apiRetStatus);
		}

/*
		// SMA Input of record trigger (Added by Daniel 10_30_2015)
		apiRetStatus = CyU3PDeviceGpioOverride (TRIG_RECORD_EXT, CyTrue);
			if (apiRetStatus != 0)
			{
				CyU3PDebugPrint (4, "GPIO Override failed, Error Code = %d\n", apiRetStatus);
				CyFxAppErrorHandler (apiRetStatus);
			}

			/* Record Trigger Settings */
			gpioConfig.outValue    = CyFalse;
			gpioConfig.driveLowEn  = CyFalse;
			gpioConfig.driveHighEn = CyFalse;
			gpioConfig.inputEn     = CyTrue;
			gpioConfig.intrMode    = CY_U3P_GPIO_NO_INTR;
			apiRetStatus           = CyU3PGpioSetSimpleConfig (TRIG_RECORD_EXT, &gpioConfig);

			if (apiRetStatus != CY_U3P_SUCCESS)
			{
				CyU3PDebugPrint (4, "GPIO Set Config Error, Error Code = %d\n", apiRetStatus);
				CyFxAppErrorHandler (apiRetStatus);
			}
			apiRetStatus = CyU3PGpioSetIoMode (TRIG_RECORD_EXT,CY_U3P_GPIO_IO_MODE_WPD);  //Added by Daniel 11_2_2015
			if (apiRetStatus != CY_U3P_SUCCESS)
			{
				CyU3PDebugPrint (4, "GPIO Set IO Mode Error, Error Code = %d\n", apiRetStatus);
				CyFxAppErrorHandler (apiRetStatus);
			}

	//-----------------------------------------------------------------------------------------

    /* Initialize the P-port. */
    pibclock.clkDiv      = 2;
    pibclock.clkSrc      = CY_U3P_SYS_CLK;
    pibclock.isDllEnable = CyFalse;
    pibclock.isHalfDiv   = CyFalse;

    apiRetStatus = CyU3PPibInit (CyTrue, &pibclock);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "PIB Function Failed to Start, Error Code = %d\n", apiRetStatus);
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Setup the Callback to Handle the GPIF INTR event */
    CyU3PGpifRegisterCallback (CyFxGpifCB);

#ifdef BACKFLOW_DETECT
    back_flow_detected = 0;
    CyU3PPibRegisterCallback (CyFxUvcAppPibCallback, CYU3P_PIB_INTR_ERROR);
#endif

    /* Image sensor initialization. Reset and then initialize with appropriate configuration. */

    SensorInit ();

    /* USB initialization. */
    apiRetStatus = CyU3PUsbStart ();
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB Function Failed to Start, Error Code = %d\n", apiRetStatus);
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Setup the Callback to Handle the USB Setup Requests */
    CyU3PUsbRegisterSetupCallback (CyFxUVCApplnUSBSetupCB, CyFalse);

    /* Setup the Callback to Handle the USB Events */
    CyU3PUsbRegisterEventCallback (CyFxUVCApplnUSBEventCB);

    /* Register the USB device descriptors with the driver. */
    CyU3PUsbSetDesc (CY_U3P_USB_SET_HS_DEVICE_DESCR, NULL, (uint8_t *)CyFxUSBDeviceDscr);
    CyU3PUsbSetDesc (CY_U3P_USB_SET_SS_DEVICE_DESCR, NULL, (uint8_t *)CyFxUSBDeviceDscrSS);

    /* BOS and Device qualifier descriptors. */
    CyU3PUsbSetDesc (CY_U3P_USB_SET_DEVQUAL_DESCR, NULL, (uint8_t *)CyFxUSBDeviceQualDscr);
    CyU3PUsbSetDesc (CY_U3P_USB_SET_SS_BOS_DESCR, NULL, (uint8_t *)CyFxUSBBOSDscr);

    /* Configuration descriptors. */
    CyU3PUsbSetDesc (CY_U3P_USB_SET_HS_CONFIG_DESCR, NULL, (uint8_t *)CyFxUSBHSConfigDscr);
    CyU3PUsbSetDesc (CY_U3P_USB_SET_FS_CONFIG_DESCR, NULL, (uint8_t *)CyFxUSBFSConfigDscr);
    CyU3PUsbSetDesc (CY_U3P_USB_SET_SS_CONFIG_DESCR, NULL, (uint8_t *)CyFxUSBSSConfigDscr);

    /* String Descriptors */
    CyU3PUsbSetDesc (CY_U3P_USB_SET_STRING_DESCR, 0, (uint8_t *)CyFxUSBStringLangIDDscr);
    CyU3PUsbSetDesc (CY_U3P_USB_SET_STRING_DESCR, 1, (uint8_t *)CyFxUSBManufactureDscr);
    CyU3PUsbSetDesc (CY_U3P_USB_SET_STRING_DESCR, 2, (uint8_t *)CyFxUSBProductDscr);

    /* Configure the video streaming endpoint. */
    endPointConfig.enable   = 1;
    endPointConfig.epType   = CY_U3P_USB_EP_BULK;
    endPointConfig.pcktSize = CY_FX_EP_BULK_VIDEO_PKT_SIZE;
    endPointConfig.isoPkts  = 1;
    endPointConfig.burstLen = 16;
    endPointConfig.streams  = 0;
    apiRetStatus = CyU3PSetEpConfig (CY_FX_EP_BULK_VIDEO, &endPointConfig);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        /* Error Handling */
        CyU3PDebugPrint (4, "USB Set Endpoint config failed, Error Code = %d\n", apiRetStatus);
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Configure the status interrupt endpoint.
       Note: This endpoint is not being used by the application as of now. This can be used in case
       UVC device needs to notify the host about any error conditions. A MANUAL_OUT DMA channel
       can be associated with this endpoint and used to send these data packets.
     */
    endPointConfig.enable   = 1;
    endPointConfig.epType   = CY_U3P_USB_EP_INTR;
    endPointConfig.pcktSize = 64;
    endPointConfig.isoPkts  = 0;
    endPointConfig.streams  = 0;
    endPointConfig.burstLen = 1;
    apiRetStatus = CyU3PSetEpConfig (CY_FX_EP_CONTROL_STATUS, &endPointConfig);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        /* Error Handling */
        CyU3PDebugPrint (4, "USB Set Endpoint config failed, Error Code = %d\n", apiRetStatus);
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Create a DMA Manual channel for sending the video data to the USB host. */
    dmaMultiConfig.size           = CY_FX_UVC_STREAM_BUF_SIZE;
    dmaMultiConfig.count          = CY_FX_UVC_STREAM_BUF_COUNT;
    dmaMultiConfig.validSckCount  = 2;
    dmaMultiConfig.prodSckId [0]  = (CyU3PDmaSocketId_t)CY_U3P_PIB_SOCKET_0;
    dmaMultiConfig.prodSckId [1]  = (CyU3PDmaSocketId_t)CY_U3P_PIB_SOCKET_1;
    dmaMultiConfig.consSckId [0]  = (CyU3PDmaSocketId_t)(CY_U3P_UIB_SOCKET_CONS_0 | CY_FX_EP_VIDEO_CONS_SOCKET);
    dmaMultiConfig.prodAvailCount = 0;
    dmaMultiConfig.prodHeader     = 12;                 /* 12 byte UVC header to be added. */
    dmaMultiConfig.prodFooter     = 4;                  /* 4 byte footer to compensate for the 12 byte header. */
    dmaMultiConfig.consHeader     = 0;
    dmaMultiConfig.dmaMode        = CY_U3P_DMA_MODE_BYTE;
    dmaMultiConfig.notification   = CY_U3P_DMA_CB_CONS_EVENT;
    dmaMultiConfig.cb             = CyFxUvcApplnDmaCallback;
    apiRetStatus = CyU3PDmaMultiChannelCreate (&glChHandleUVCStream, CY_U3P_DMA_TYPE_MANUAL_MANY_TO_ONE,
            &dmaMultiConfig);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        /* Error handling */
        CyU3PDebugPrint (4, "DMA Channel Creation Failed, Error Code = %d\n", apiRetStatus);
        CyFxAppErrorHandler (apiRetStatus);
    }

#ifdef USB_DEBUG_INTERFACE
    /* Configure the endpoints and create DMA channels used by the USB debug interface.
       The command (OUT) endpoint is configured in packet mode and enabled to receive data.
       Once the CY_U3P_DMA_CB_PROD_EVENT callback is received, the received data packet is
       processed and the data is returned through the CyU3PDmaChannelSetupSendBuffer API call.
     */

    endPointConfig.enable   = 1;
    endPointConfig.epType   = CY_U3P_USB_EP_BULK;
    endPointConfig.pcktSize = 1024;                     /* Use SuperSpeed settings here. */
    endPointConfig.isoPkts  = 0;
    endPointConfig.streams  = 0;
    endPointConfig.burstLen = 1;

    apiRetStatus = CyU3PSetEpConfig (CY_FX_EP_DEBUG_CMD, &endPointConfig);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "Debug Command endpoint config failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler (apiRetStatus);
    }

    CyU3PUsbSetEpPktMode (CY_FX_EP_DEBUG_CMD, CyTrue);

    apiRetStatus = CyU3PSetEpConfig (CY_FX_EP_DEBUG_RSP, &endPointConfig);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "Debug Response endpoint config failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler (apiRetStatus);
    }

    channelConfig.size           = 1024;
    channelConfig.count          = 1;
    channelConfig.prodSckId      = CY_U3P_UIB_SOCKET_PROD_0 | CY_FX_EP_DEBUG_CMD_SOCKET;
    channelConfig.consSckId      = CY_U3P_CPU_SOCKET_CONS;
    channelConfig.prodAvailCount = 0;
    channelConfig.prodHeader     = 0;
    channelConfig.prodFooter     = 0;
    channelConfig.consHeader     = 0;
    channelConfig.dmaMode        = CY_U3P_DMA_MODE_BYTE;
    channelConfig.notification   = CY_U3P_DMA_CB_PROD_EVENT;
    channelConfig.cb             = CyFxUvcAppDebugCallback;

    apiRetStatus = CyU3PDmaChannelCreate (&glDebugCmdChannel, CY_U3P_DMA_TYPE_MANUAL_IN, &channelConfig);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "Debug Command channel create failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler (apiRetStatus);
    }

    apiRetStatus = CyU3PDmaChannelSetXfer (&glDebugCmdChannel, 0);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "Debug channel SetXfer failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler (apiRetStatus);
    }

    channelConfig.size           = 1024;
    channelConfig.count          = 0;           /* No buffers allocated. We will only use the SetupSend API. */
    channelConfig.prodSckId      = CY_U3P_CPU_SOCKET_PROD;
    channelConfig.consSckId      = CY_U3P_UIB_SOCKET_CONS_0 | CY_FX_EP_DEBUG_RSP_SOCKET;
    channelConfig.prodAvailCount = 0;
    channelConfig.prodHeader     = 0;
    channelConfig.prodFooter     = 0;
    channelConfig.consHeader     = 0;
    channelConfig.dmaMode        = CY_U3P_DMA_MODE_BYTE;
    channelConfig.notification   = 0;
    channelConfig.cb             = 0;

    apiRetStatus = CyU3PDmaChannelCreate (&glDebugRspChannel, CY_U3P_DMA_TYPE_MANUAL_OUT, &channelConfig);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "Debug Response channel create failed, Error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler (apiRetStatus);
    }

    glDebugRspBuffer = (uint8_t *)CyU3PDmaBufferAlloc (1024);
    if (glDebugRspBuffer == 0)
    {
        CyU3PDebugPrint (4, "Failed to allocate memory for debug buffer\r\n");
        CyFxAppErrorHandler (CY_U3P_ERROR_MEMORY_ERROR);
    }
#endif

    /* Enable USB connection from the FX3 device, preferably at USB 3.0 speed. */
    apiRetStatus = CyU3PConnectState (CyTrue, CyTrue);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "USB Connect failed, Error Code = %d\n", apiRetStatus);
        CyFxAppErrorHandler (apiRetStatus);
    }
}

/*
 * Load the GPIF configuration on the GPIF-II engine. This operation is performed whenever a new video
 * streaming session is started.
 */
static void
CyFxUvcAppGpifInit (
        void)
{
    CyU3PReturnStatus_t apiRetStatus;

    apiRetStatus =  CyU3PGpifLoad ((CyU3PGpifConfig_t *) &CyFxGpifConfig);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        /* Error Handling */
        CyU3PDebugPrint (4, "Loading GPIF Configuration failed, Error Code = %d\r\n", apiRetStatus);
        CyFxAppErrorHandler (apiRetStatus);
    }

    /* Start the state machine from the designated start state. */
    apiRetStatus = CyU3PGpifSMStart (START, ALPHA_START);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        /* Error Handling */
        CyU3PDebugPrint (4, "Starting GPIF state machine failed, Error Code = %d\r\n", apiRetStatus);
        CyFxAppErrorHandler (apiRetStatus);
    }
}

/*
 * Entry function for the UVC Application Thread
 */
void
UVCAppThread_Entry (
        uint32_t input)
{
    CyU3PDmaBuffer_t    produced_buffer;
    CyU3PReturnStatus_t apiRetStatus;
    uint32_t flag;
    uint32_t tmp_data[4] ={0};
    uint8_t state;
#ifdef DEBUG_PRINT_FRAME_COUNT
    uint32_t frameCnt = 0;
#endif

    /* Initialize the Uart Debug Module */
    //CyFxUVCApplnDebugInit ();

    /* Initialize the I2C interface */
    CyFxUVCApplnI2CInit ();

    /* Initialize the SPI interface */ //Added by Daniel 4_9_2015
    CyFxUVCApplnSPIInit ();

    /* Initialize the UVC Application */
    CyFxUVCApplnInit ();

    /*
       This thread continually checks whether video streaming is enabled, and commits video data if so.

       The CY_FX_UVC_STREAM_EVENT and CY_FX_UVC_STREAM_ABORT_EVENT event flags are monitored by this
       thread. The CY_FX_UVC_STREAM_EVENT event flag is enabled when the USB host sends a COMMIT control
       request to the video streaming interface, and stays ON as long as video streaming is enabled.

       The CY_FX_UVC_STREAM_ABORT_EVENT event indicates that we need to abort the video streaming. This
       only happens when we receive a CLEAR_FEATURE request indicating that streaming is to be stopped,
       or when we have a critical error in the data path. In both of these cases, the CY_FX_UVC_STREAM_EVENT
       event flag will be cleared before the CY_FX_UVC_STREAM_ABORT_EVENT event flag is enabled.

       This sequence ensures that we do not get stuck in a loop where we are trying to send data instead
       of handling the abort request.
     */
    for (;;)
    {
        /* Waiting for the Video Stream Event */
        if (CyU3PEventGet (&glFxUVCEvent, CY_FX_UVC_STREAM_EVENT, CYU3P_EVENT_AND, &flag,
                    CYU3P_NO_WAIT) == CY_U3P_SUCCESS)
        {
            /* Check if we have a buffer ready to go. */
            apiRetStatus = CyU3PDmaMultiChannelGetBuffer (&glChHandleUVCStream, &produced_buffer, CYU3P_NO_WAIT);
            if (apiRetStatus == CY_U3P_SUCCESS)
            {
                if (produced_buffer.count == CY_FX_UVC_BUF_FULL_SIZE)
                {
                    CyFxUVCAddHeader (produced_buffer.buffer - CY_FX_UVC_MAX_HEADER, CY_FX_UVC_HEADER_FRAME);
                }
                else
                {
                    /* If we have a partial buffer, this is guaranteed to be the end of the video frame for uncompressed images. */
                    CyFxUVCAddHeader (produced_buffer.buffer - CY_FX_UVC_MAX_HEADER, CY_FX_UVC_HEADER_EOF);
                    gotPartial = CyFalse; /* Flag is reset to indicate that the partial buffer was committed to USB */
                }

                /* Commit the updated DMA buffer to the USB endpoint. */
                prodCount++;
                apiRetStatus = CyU3PDmaMultiChannelCommitBuffer (&glChHandleUVCStream,
                        produced_buffer.count + CY_FX_UVC_MAX_HEADER, 0);
                if (apiRetStatus != CY_U3P_SUCCESS)
                {
                    prodCount--;
                    CyU3PDebugPrint (4, "Error in multichannelcommitbuffer: Code = %d, size = %x, dmaDone %x\r\n",
                            apiRetStatus, produced_buffer.count, prodCount - consCount);
                }
            }

            /* If we have the end of frame signal and all of the committed data (including partial buffer)
             * has been read by the USB host; we can reset the DMA channel and prepare for the next video frame.
             */
            if ((hitFV) && (prodCount == consCount) && (!gotPartial))
            {
                prodCount = 0;
                consCount = 0;
                hitFV     = CyFalse;

#ifdef BACKFLOW_DETECT
                back_flow_detected = 0;
#endif

#ifdef DEBUG_PRINT_FRAME_COUNT
                CyU3PDebugPrint (4, "frame %d\r\n", frameCnt++);
#endif

                /* Toggle UVC header FRAME ID bit */
                glUVCHeader[1] ^= CY_FX_UVC_HEADER_FRAME_ID;

                /* Reset the DMA channel. */
                apiRetStatus = CyU3PDmaMultiChannelReset (&glChHandleUVCStream);
                if (apiRetStatus != CY_U3P_SUCCESS)
                {
                    CyU3PDebugPrint (4, "DMA Channel Reset Failed, Error Code = %d\n", apiRetStatus);
                    CyFxAppErrorHandler (apiRetStatus);
                }

                /* Start Channel Immediately */
                apiRetStatus = CyU3PDmaMultiChannelSetXfer (&glChHandleUVCStream, 0, 0);
                if (apiRetStatus != CY_U3P_SUCCESS)
                {
                    CyU3PDebugPrint (4, "DMA Channel Set Transfer Failed, Error Code = %d\n", apiRetStatus);
                    CyFxAppErrorHandler (apiRetStatus);
                }

                /* Jump to the start state of the GPIF state machine. 257 is used as an
                   arbitrary invalid state (> 255) number. */
                CyU3PGpifSMSwitch (257, 0, 257, 0, 2);





           }
        }
        else
        {
            /* If we have a stream abort request pending. */
            if (CyU3PEventGet (&glFxUVCEvent, CY_FX_UVC_STREAM_ABORT_EVENT, CYU3P_EVENT_AND_CLEAR,
                        &flag, CYU3P_NO_WAIT) == CY_U3P_SUCCESS)
            {
                hitFV     = CyFalse;
                prodCount = 0;
                consCount = 0;

                if (!clearFeatureRqtReceived)
                {
                    apiRetStatus = CyU3PDmaMultiChannelReset (&glChHandleUVCStream);
                    if (apiRetStatus != CY_U3P_SUCCESS)
                    {
                        CyFxAppErrorHandler (apiRetStatus);
                    }

                    /* Flush the Endpoint memory */
                    CyU3PUsbFlushEp (CY_FX_EP_BULK_VIDEO);
                }

                clearFeatureRqtReceived = CyFalse;
            }
            else
            {
                /* We are essentially idle at this point. Wait for the reception of a start streaming request. */
                CyU3PEventGet (&glFxUVCEvent, CY_FX_UVC_STREAM_EVENT, CYU3P_EVENT_AND, &flag, CYU3P_WAIT_FOREVER);

                /* Set DMA Channel transfer size, first producer socket */
                apiRetStatus = CyU3PDmaMultiChannelSetXfer (&glChHandleUVCStream, 0, 0);
                if (apiRetStatus != CY_U3P_SUCCESS)
                {
                    /* Error handling */
                    CyU3PDebugPrint (4, "DMA Channel Set Transfer Failed, Error Code = %d\r\n", apiRetStatus);
                    CyFxAppErrorHandler (apiRetStatus);
                }

                /* Initialize gpif configuration and waveform descriptors */
                if (gpif_initialized == CyFalse)
                {
                    CyFxUvcAppGpifInit ();
                    gpif_initialized = CyTrue;
                }
                else
                {
                    /* Jump to the start state of the GPIF state machine. 257 is used as an
                       arbitrary invalid state (> 255) number. */
                    CyU3PGpifSMSwitch (257, 0, 257, 0, 2);

                }
            }
        }

        /* Allow other ready threads to run before proceeding. */
        CyU3PThreadRelinquish ();
    }
}

/*
 * Handler for control requests addressed to the Processing Unit.
 */
static void
UVCHandleProcessingUnitRqts (
		void)
{
	CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
	uint16_t readCount;
	uint32_t gpioVal0, gpioVal1;
	CyBool_t GPIOState;
	switch (wValue)
	{
	case CY_FX_UVC_PU_BRIGHTNESS_CONTROL:
		switch (bRequest)
		{
		case CY_FX_USB_UVC_GET_LEN_REQ: /* Length of brightness data = 1 byte. */
			glEp0Buffer[0] = 1;
			CyU3PUsbSendEP0Data (1, (uint8_t *)glEp0Buffer);
			break;
		case CY_FX_USB_UVC_GET_CUR_REQ: /* Current brightness value. */
			glEp0Buffer[0] = 0;//SensorGetBrightness ();
			CyU3PUsbSendEP0Data (1, (uint8_t *)glEp0Buffer);
			break;
		case CY_FX_USB_UVC_GET_MIN_REQ: /* Minimum brightness = 0. */
			glEp0Buffer[0] = 0;
			CyU3PUsbSendEP0Data (1, (uint8_t *)glEp0Buffer);
			break;
		case CY_FX_USB_UVC_GET_MAX_REQ: /* Maximum brightness = 255. */
			glEp0Buffer[0] = 255;
			CyU3PUsbSendEP0Data (1, (uint8_t *)glEp0Buffer);
			break;
		case CY_FX_USB_UVC_GET_RES_REQ: /* Resolution = 1. */
			glEp0Buffer[0] = 1;
			CyU3PUsbSendEP0Data (1, (uint8_t *)glEp0Buffer);
			break;
		case CY_FX_USB_UVC_GET_INFO_REQ: /* Both GET and SET requests are supported, auto modes not supported */
			glEp0Buffer[0] = 3;
			CyU3PUsbSendEP0Data (1, (uint8_t *)glEp0Buffer);
			break;
		case CY_FX_USB_UVC_GET_DEF_REQ: /* Default brightness value = 55. */
			glEp0Buffer[0] = 255;
			CyU3PUsbSendEP0Data (1, (uint8_t *)glEp0Buffer);
			break;
		case CY_FX_USB_UVC_SET_CUR_REQ: /* Update brightness value. */
			apiRetStatus = CyU3PUsbGetEP0Data (CY_FX_UVC_MAX_PROBE_SETTING_ALIGNED,
					glEp0Buffer, &readCount);
			if (apiRetStatus == CY_U3P_SUCCESS)
			{
				SensorSetBrightness (glEp0Buffer[0]);
			}
			break;
		default:
			CyU3PUsbStall (0, CyTrue, CyFalse);
			break;
		}
		break;
	case CY_FX_UVC_PU_GAIN_CONTROL:
		switch (bRequest)
		{
		case CY_FX_USB_UVC_GET_LEN_REQ: /* Length of gain data = 1 byte. */
				glEp0Buffer[0] = 1;
				CyU3PUsbSendEP0Data (1, (uint8_t *)glEp0Buffer);
				break;
		case CY_FX_USB_UVC_GET_CUR_REQ: /* Current gain value. */
			//glEp0Buffer[0] = 0;//SensorGetGain ();		//Commented out by Jill

			glEp0Buffer[0] = SensorGetGain();
			CyU3PUsbSendEP0Data (1, (uint8_t *)glEp0Buffer);
			break;
		case CY_FX_USB_UVC_GET_MIN_REQ: /* Minimum gain = 0. */
			//glEp0Buffer[0] = 16;
			glEp0Buffer[0] = 1;				//Changed by Jill 8/29/18
			CyU3PUsbSendEP0Data (1, (uint8_t *)glEp0Buffer);
			break;
		case CY_FX_USB_UVC_GET_MAX_REQ: /* Maximum gain = 255. */
			//glEp0Buffer[0] = 64;			//Changed by Jill 8/29/18
			//glEp0Buffer[0] = 100;
			glEp0Buffer[0] = 240;
			CyU3PUsbSendEP0Data (1, (uint8_t *)glEp0Buffer);
			break;
		case CY_FX_USB_UVC_GET_RES_REQ: /* Resolution = 1. */
			glEp0Buffer[0] = 1;
			CyU3PUsbSendEP0Data (1, (uint8_t *)glEp0Buffer);
			break;
		case CY_FX_USB_UVC_GET_INFO_REQ: /* Both GET and SET requests are supported, auto modes not supported */
			glEp0Buffer[0] = 3;
			CyU3PUsbSendEP0Data (1, (uint8_t *)glEp0Buffer);
			break;
		case CY_FX_USB_UVC_GET_DEF_REQ: /* Default gain value = 55. */
			//glEp0Buffer[0] = 16;
			glEp0Buffer[0] = 1;     //Changed by Jill 8/29/18
			CyU3PUsbSendEP0Data (1, (uint8_t *)glEp0Buffer);
			break;
		case CY_FX_USB_UVC_SET_CUR_REQ: /* Update gain value. */
			apiRetStatus = CyU3PUsbGetEP0Data (CY_FX_UVC_MAX_PROBE_SETTING_ALIGNED,
					glEp0Buffer, &readCount);
			if (apiRetStatus == CY_U3P_SUCCESS)
			{
				SensorSetGain (glEp0Buffer[0]);
			}
			break;
		default:
			CyU3PUsbStall (0, CyTrue, CyFalse);
			break;
		}
		break;
		case CY_FX_UVC_PU_HUE_CONTROL : //This is how LED brightness is transmitted over USB. Added by Daniel 4_9_2015
			switch (bRequest)
			{
			case CY_FX_USB_UVC_GET_LEN_REQ: /* Length of brightness data = 1 byte. */
				glEp0Buffer[0] = 1;
				CyU3PUsbSendEP0Data (1, (uint8_t *)glEp0Buffer);
				break;
			case CY_FX_USB_UVC_GET_CUR_REQ: /* Current brightness value. */
				glEp0Buffer[0] = 0;
				//glEp0Buffer[1] = 0;
				CyU3PUsbSendEP0Data (1, (uint8_t *)glEp0Buffer);
				break;
			case CY_FX_USB_UVC_GET_MIN_REQ: /* Minimum brightness = 0. */
				glEp0Buffer[0] = 0;
				CyU3PUsbSendEP0Data (1, (uint8_t *)glEp0Buffer);
				break;
			case CY_FX_USB_UVC_GET_MAX_REQ: /* Maximum brightness = 255. */
				glEp0Buffer[0] = 255;
				//glEp0Buffer[1] = 255;
				CyU3PUsbSendEP0Data (1, (uint8_t *)glEp0Buffer);
				break;
			case CY_FX_USB_UVC_GET_RES_REQ: /* Resolution = 1. */
				glEp0Buffer[0] = 1;
				CyU3PUsbSendEP0Data (1, (uint8_t *)glEp0Buffer);
				break;
			case CY_FX_USB_UVC_GET_INFO_REQ: /* Both GET and SET requests are supported, auto modes not supported */
				glEp0Buffer[0] = 3;
				CyU3PUsbSendEP0Data (1, (uint8_t *)glEp0Buffer);
				break;
			case CY_FX_USB_UVC_GET_DEF_REQ: /* Default brightness value = 55. */
				glEp0Buffer[0] = 0;
				//glEp0Buffer[1] = 0;
				CyU3PUsbSendEP0Data (1, (uint8_t *)glEp0Buffer);
				break;
			case CY_FX_USB_UVC_SET_CUR_REQ: /* Update brightness value. */
				apiRetStatus = CyU3PUsbGetEP0Data (CY_FX_UVC_MAX_PROBE_SETTING_ALIGNED,
						glEp0Buffer, &readCount);
				if (apiRetStatus == CY_U3P_SUCCESS)
				{
					//CyFxDACSpiWrite(glEp0Buffer);
					CyFxDACI2CWrite(glEp0Buffer);
				}
				break;
			default:
				CyU3PUsbStall (0, CyTrue, CyFalse);
				break;
			}
			break;
			case CY_FX_UVC_PU_SATURATION_CONTROL: //Added by Daniel 6_24_2015. Used for general communication between DAQ software and PCB
				switch (bRequest)
				{
				case CY_FX_USB_UVC_GET_LEN_REQ: /* Length of gain data = 1 byte. */
						glEp0Buffer[0] = 1;
						CyU3PUsbSendEP0Data (1, (uint8_t *)glEp0Buffer);
						break;
				case CY_FX_USB_UVC_GET_CUR_REQ: /* Get GPIO values. Added by Daniel 10_30_2015*/
					apiRetStatus = CyU3PGpioGetIOValues (&gpioVal0, &gpioVal1);
					//apiRetStatus = CyU3PGpioSimpleGetValue(TRIG_RECORD_EXT,&GPIOState);
					glEp0Buffer[0] = (gpioVal0>>GPIO_SHIFT)&GPIO_MASK;
					//glEp0Buffer[0] = GPIOState;
					CyU3PUsbSendEP0Data (1, (uint8_t *)glEp0Buffer);
					break;
				case CY_FX_USB_UVC_GET_MIN_REQ: /* Minimum gain = 0. */
					glEp0Buffer[0] = 0;
					CyU3PUsbSendEP0Data (1, (uint8_t *)glEp0Buffer);
					break;
				case CY_FX_USB_UVC_GET_MAX_REQ: /* Maximum gain = 255. */
					glEp0Buffer[0] = 255;
					CyU3PUsbSendEP0Data (1, (uint8_t *)glEp0Buffer);
					break;
				case CY_FX_USB_UVC_GET_RES_REQ: /* Resolution = 1. */
					glEp0Buffer[0] = 1;
					CyU3PUsbSendEP0Data (1, (uint8_t *)glEp0Buffer);
					break;
				case CY_FX_USB_UVC_GET_INFO_REQ: /* Both GET and SET requests are supported, auto modes not supported */
					glEp0Buffer[0] = 3;
					CyU3PUsbSendEP0Data (1, (uint8_t *)glEp0Buffer);
					break;
				case CY_FX_USB_UVC_GET_DEF_REQ: /* Default gain value = 55. */
					glEp0Buffer[0] = 0;
					CyU3PUsbSendEP0Data (1, (uint8_t *)glEp0Buffer);
					break;
				case CY_FX_USB_UVC_SET_CUR_REQ: /* Update communication value. */
					apiRetStatus = CyU3PUsbGetEP0Data (CY_FX_UVC_MAX_PROBE_SETTING_ALIGNED,
							glEp0Buffer, &readCount);
					if (apiRetStatus == CY_U3P_SUCCESS)
					{
						handleCommunication (glEp0Buffer[0]);
					}
					break;
				default:
					CyU3PUsbStall (0, CyTrue, CyFalse);
					break;
				}
				break;
	default:
				/*
				 * Only the brightness control is supported as of now. Add additional code here to support
				 * other controls.
				 */
	CyU3PUsbStall (0, CyTrue, CyFalse);
	break;
	}
}

/*
 * Handler for control requests addressed to the UVC Camera Terminal unit.
 */
static void
UVCHandleCameraTerminalRqts (
        void)
{
#ifdef UVC_PTZ_SUPPORT
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
    uint16_t readCount;
    uint16_t zoomVal;
    int32_t  panVal, tiltVal;
    CyBool_t sendData = CyFalse;
#endif

    switch (wValue)
    {
#ifdef UVC_PTZ_SUPPORT
        case CY_FX_UVC_CT_ZOOM_ABSOLUTE_CONTROL:
            switch (bRequest)
            {
                case CY_FX_USB_UVC_GET_INFO_REQ:
                    glEp0Buffer[0] = 3;                /* Support GET/SET queries. */
                    CyU3PUsbSendEP0Data (1, (uint8_t *)glEp0Buffer);
                    break;
                case CY_FX_USB_UVC_GET_CUR_REQ: /* Current zoom control value. */
                    zoomVal  = CyFxUvcAppGetCurrentZoom ();
                    sendData = CyTrue;
                    break;
                case CY_FX_USB_UVC_GET_MIN_REQ: /* Minimum zoom control value. */
                    zoomVal  = CyFxUvcAppGetMinimumZoom ();
                    sendData = CyTrue;
                    break;
                case CY_FX_USB_UVC_GET_MAX_REQ: /* Maximum zoom control value. */
                    zoomVal  = CyFxUvcAppGetMaximumZoom ();
                    sendData = CyTrue;
                    break;
                case CY_FX_USB_UVC_GET_RES_REQ: /* Resolution is one unit. */
                    zoomVal  = CyFxUvcAppGetZoomResolution ();
                    sendData = CyTrue;
                    break;
                case CY_FX_USB_UVC_GET_DEF_REQ: /* Default zoom setting. */
                    zoomVal  = CyFxUvcAppGetDefaultZoom ();
                    sendData = CyTrue;
                    break;
                case CY_FX_USB_UVC_SET_CUR_REQ:
                    apiRetStatus = CyU3PUsbGetEP0Data (CY_FX_UVC_MAX_PROBE_SETTING_ALIGNED,
                            glEp0Buffer, &readCount);
                    if (apiRetStatus == CY_U3P_SUCCESS)
                    {
                        zoomVal = (glEp0Buffer[0]) | (glEp0Buffer[1] << 8);
                        CyFxUvcAppModifyZoom (zoomVal);
                    }
                    break;
                default:
                    CyU3PUsbStall (0, CyTrue, CyFalse);
                    break;
            }

            if (sendData)
            {
                /* Send the 2-byte data in zoomVal back to the USB host. */
                glEp0Buffer[0] = CY_U3P_GET_LSB (zoomVal);
                glEp0Buffer[1] = CY_U3P_GET_MSB (zoomVal);
                CyU3PUsbSendEP0Data (wLength, (uint8_t *)glEp0Buffer);
            }
            break;

        case CY_FX_UVC_CT_PANTILT_ABSOLUTE_CONTROL:
            switch (bRequest)
            {
                case CY_FX_USB_UVC_GET_INFO_REQ:
                    glEp0Buffer[0] = 3;                /* GET/SET requests supported for this control */
                    CyU3PUsbSendEP0Data (1, (uint8_t *)glEp0Buffer);
                    break;
                case CY_FX_USB_UVC_GET_CUR_REQ:
                    panVal   = CyFxUvcAppGetCurrentPan ();
                    tiltVal  = CyFxUvcAppGetCurrentTilt ();
                    sendData = CyTrue;
                    break;
                case CY_FX_USB_UVC_GET_MIN_REQ:
                    panVal   = CyFxUvcAppGetMinimumPan ();
                    tiltVal  = CyFxUvcAppGetMinimumTilt ();
                    sendData = CyTrue;
                    break;
                case CY_FX_USB_UVC_GET_MAX_REQ:
                    panVal   = CyFxUvcAppGetMaximumPan ();
                    tiltVal  = CyFxUvcAppGetMaximumTilt ();
                    sendData = CyTrue;
                    break;
                case CY_FX_USB_UVC_GET_RES_REQ:
                    panVal   = CyFxUvcAppGetPanResolution ();
                    tiltVal  = CyFxUvcAppGetTiltResolution ();
                    sendData = CyTrue;
                    break;
                case CY_FX_USB_UVC_GET_DEF_REQ:
                    panVal   = CyFxUvcAppGetDefaultPan ();
                    tiltVal  = CyFxUvcAppGetDefaultTilt ();
                    sendData = CyTrue;
                    break;
                case CY_FX_USB_UVC_SET_CUR_REQ:
                    apiRetStatus = CyU3PUsbGetEP0Data (CY_FX_UVC_MAX_PROBE_SETTING_ALIGNED,
                            glEp0Buffer, &readCount);
                    if (apiRetStatus == CY_U3P_SUCCESS)
                    {
                        panVal = (glEp0Buffer[0]) | (glEp0Buffer[1]<<8) |
                            (glEp0Buffer[2]<<16) | (glEp0Buffer[2]<<24);
                        tiltVal = (glEp0Buffer[4]) | (glEp0Buffer[5]<<8) |
                            (glEp0Buffer[6]<<16) | (glEp0Buffer[7]<<24);

                        CyFxUvcAppModifyPan (panVal);
                        CyFxUvcAppModifyTilt (tiltVal);
                    }
                    break;
                default:
                    CyU3PUsbStall (0, CyTrue, CyFalse);
                    break;
            }

            if (sendData)
            {
                /* Send the 8-byte PAN and TILT values back to the USB host. */
                glEp0Buffer[0] = CY_U3P_DWORD_GET_BYTE0 (panVal);
                glEp0Buffer[1] = CY_U3P_DWORD_GET_BYTE1 (panVal);
                glEp0Buffer[2] = CY_U3P_DWORD_GET_BYTE2 (panVal);
                glEp0Buffer[3] = CY_U3P_DWORD_GET_BYTE3 (panVal);
                glEp0Buffer[4] = CY_U3P_DWORD_GET_BYTE0 (tiltVal);
                glEp0Buffer[5] = CY_U3P_DWORD_GET_BYTE1 (tiltVal);
                glEp0Buffer[6] = CY_U3P_DWORD_GET_BYTE2 (tiltVal);
                glEp0Buffer[7] = CY_U3P_DWORD_GET_BYTE3 (tiltVal);
                CyU3PUsbSendEP0Data (wLength, (uint8_t *)glEp0Buffer);
            }
            break;
#endif

        default:
            CyU3PUsbStall (0, CyTrue, CyFalse);
            break;
    }
}

/*
 * Handler for UVC Interface control requests.
 */
static void
UVCHandleInterfaceCtrlRqts (
        void)
{
    /* No requests supported as of now. Just stall EP0 to fail the request. */
    CyU3PUsbStall (0, CyTrue, CyFalse);
}

/*
 * Handler for control requests addressed to the Extension Unit.
 */
static void
UVCHandleExtensionUnitRqts (
        void)
{
    /* No requests supported as of now. Just stall EP0 to fail the request. */
    CyU3PUsbStall (0, CyTrue, CyFalse);
}

/*
 * Handler for the video streaming control requests.
 */
static void
UVCHandleVideoStreamingRqts (
        void)
{
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
    uint16_t readCount;

    switch (wValue)
    {
        case CY_FX_UVC_PROBE_CTRL:
            switch (bRequest)
            {
                case CY_FX_USB_UVC_GET_INFO_REQ:
                    glEp0Buffer[0] = 3;                /* GET/SET requests are supported. */
                    CyU3PUsbSendEP0Data (1, (uint8_t *)glEp0Buffer);
                    break;
                case CY_FX_USB_UVC_GET_LEN_REQ:
                    glEp0Buffer[0] = CY_FX_UVC_MAX_PROBE_SETTING;
                    CyU3PUsbSendEP0Data (1, (uint8_t *)glEp0Buffer);
                    break;
                case CY_FX_USB_UVC_GET_CUR_REQ:
                case CY_FX_USB_UVC_GET_MIN_REQ:
                case CY_FX_USB_UVC_GET_MAX_REQ:
                case CY_FX_USB_UVC_GET_DEF_REQ: /* There is only one setting per USB speed. */
                    if (usbSpeed == CY_U3P_SUPER_SPEED)
                    {
                        CyU3PUsbSendEP0Data (CY_FX_UVC_MAX_PROBE_SETTING, (uint8_t *)glProbeCtrl);
                    }
                    else
                    {
                        CyU3PUsbSendEP0Data (CY_FX_UVC_MAX_PROBE_SETTING, (uint8_t *)glProbeCtrl20);
                    }
                    break;
                case CY_FX_USB_UVC_SET_CUR_REQ:
                    apiRetStatus = CyU3PUsbGetEP0Data (CY_FX_UVC_MAX_PROBE_SETTING_ALIGNED,
                            glCommitCtrl, &readCount);
                    if (apiRetStatus == CY_U3P_SUCCESS)
                    {
                        if (usbSpeed == CY_U3P_SUPER_SPEED)
                        {
                            /* Copy the relevant settings from the host provided data into the
                               active data structure. */
                            glProbeCtrl[2] = glCommitCtrl[2];
                            glProbeCtrl[3] = glCommitCtrl[3];
                            glProbeCtrl[4] = glCommitCtrl[4];
                            glProbeCtrl[5] = glCommitCtrl[5];
                            glProbeCtrl[6] = glCommitCtrl[6];
                            glProbeCtrl[7] = glCommitCtrl[7];
                        }
                    }
                    break;
                default:
                    CyU3PUsbStall (0, CyTrue, CyFalse);
                    break;
            }
            break;

        case CY_FX_UVC_COMMIT_CTRL:
            switch (bRequest)
            {
                case CY_FX_USB_UVC_GET_INFO_REQ:
                    glEp0Buffer[0] = 3;                        /* GET/SET requests are supported. */
                    CyU3PUsbSendEP0Data (1, (uint8_t *)glEp0Buffer);
                    break;
                case CY_FX_USB_UVC_GET_LEN_REQ:
                    glEp0Buffer[0] = CY_FX_UVC_MAX_PROBE_SETTING;
                    CyU3PUsbSendEP0Data (1, (uint8_t *)glEp0Buffer);
                    break;
                case CY_FX_USB_UVC_GET_CUR_REQ:
                    if (usbSpeed == CY_U3P_SUPER_SPEED)
                    {
                        CyU3PUsbSendEP0Data (CY_FX_UVC_MAX_PROBE_SETTING, (uint8_t *)glProbeCtrl);
                    }
                    else
                    {
                        CyU3PUsbSendEP0Data (CY_FX_UVC_MAX_PROBE_SETTING, (uint8_t *)glProbeCtrl20);
                    }
                    break;
                case CY_FX_USB_UVC_SET_CUR_REQ:
                    /* The host has selected the parameters for the video stream. Check the desired
                       resolution settings, configure the sensor and start the video stream.
                       */
                    apiRetStatus = CyU3PUsbGetEP0Data (CY_FX_UVC_MAX_PROBE_SETTING_ALIGNED,
                            glCommitCtrl, &readCount);
                    if (apiRetStatus == CY_U3P_SUCCESS)
                    {
                        if (usbSpeed == CY_U3P_SUPER_SPEED)
                        {

                        }
                        else
                        {

                        }

                        /* We can start streaming video now. */
                        apiRetStatus = CyU3PEventSet (&glFxUVCEvent, CY_FX_UVC_STREAM_EVENT, CYU3P_EVENT_OR);
                        if (apiRetStatus != CY_U3P_SUCCESS)
                        {
                            CyU3PDebugPrint (4, "Set CY_FX_UVC_STREAM_EVENT failed %x\n", apiRetStatus);
                        }
                    }
                    break;

                default:
                    CyU3PUsbStall (0, CyTrue, CyFalse);
                    break;
            }
            break;

        default:
            CyU3PUsbStall (0, CyTrue, CyFalse);
            break;
    }
}

/*
 * Entry function for the UVC control request processing thread.
 */
void
UVCAppEP0Thread_Entry (
        uint32_t input)
{
    uint32_t eventMask = (CY_FX_UVC_VIDEO_CONTROL_REQUEST_EVENT | CY_FX_UVC_VIDEO_STREAM_REQUEST_EVENT);
    uint32_t eventFlag;

#ifdef USB_DEBUG_INTERFACE
    CyU3PReturnStatus_t apiRetStatus;
    CyU3PDmaBuffer_t    dmaInfo;

    eventMask |= CY_FX_USB_DEBUG_CMD_EVENT;
#endif

    for (;;)
    {
        /* Wait for a Video control or streaming related request on the control endpoint. */
        if (CyU3PEventGet (&glFxUVCEvent, eventMask, CYU3P_EVENT_OR_CLEAR, &eventFlag,
                    CYU3P_WAIT_FOREVER) == CY_U3P_SUCCESS)
        {
            /* If this is the first request received during this connection, query the connection speed. */
            if (!isUsbConnected)
            {
                usbSpeed = CyU3PUsbGetSpeed ();
                if (usbSpeed != CY_U3P_NOT_CONNECTED)
                {
                    isUsbConnected = CyTrue;
                }
            }

            if (eventFlag & CY_FX_UVC_VIDEO_CONTROL_REQUEST_EVENT)
            {
                switch ((wIndex >> 8))
                {
                    case CY_FX_UVC_PROCESSING_UNIT_ID:
                        UVCHandleProcessingUnitRqts ();
                        break;

                    case CY_FX_UVC_CAMERA_TERMINAL_ID:
                        UVCHandleCameraTerminalRqts ();
                        break;

                    case CY_FX_UVC_INTERFACE_CTRL:
                        UVCHandleInterfaceCtrlRqts ();
                        break;

                    case CY_FX_UVC_EXTENSION_UNIT_ID:
                        UVCHandleExtensionUnitRqts ();
                        break;

                    default:
                        /* Unsupported request. Fail by stalling the control endpoint. */
                        CyU3PUsbStall (0, CyTrue, CyFalse);
                        break;
                }
            }

            if (eventFlag & CY_FX_UVC_VIDEO_STREAM_REQUEST_EVENT)
            {
                if (wIndex != CY_FX_UVC_STREAM_INTERFACE)
                {
                    CyU3PUsbStall (0, CyTrue, CyFalse);
                }
                else
                {
                    UVCHandleVideoStreamingRqts ();
                }
            }

#ifdef USB_DEBUG_INTERFACE
            if (eventFlag & CY_FX_USB_DEBUG_CMD_EVENT)
            {
                /* Get the command buffer */
                apiRetStatus = CyU3PDmaChannelGetBuffer (&glDebugCmdChannel, &dmaInfo, CYU3P_WAIT_FOREVER);
                if (apiRetStatus != CY_U3P_SUCCESS)
                {
                    CyU3PDebugPrint (4, "Failed to receive debug command, Error code = %d\r\n", apiRetStatus);
                    CyFxAppErrorHandler (apiRetStatus);
                }

                /* Decode the command from the command buffer, error checking is not implemented,
                 * so the command is expected to be correctly sent from the host application. First byte indicates
                 * read (0x00) or write (0x01) command. Second and third bytes are register address high byte and
                 * register address low byte. For read commands the fourth byte (optional) can be N>0, to read N
                 * registers in sequence. Response first byte is status (0=Pass, !0=Fail) followed by N pairs of
                 * register value high byte and register value low byte.
                 */
                if (dmaInfo.buffer[0] == 0)
                {
                    if (dmaInfo.count == 3)
                    {
                        glDebugRspBuffer[0] = SensorRead2B (SENSOR_ADDR_RD, dmaInfo.buffer[1], dmaInfo.buffer[2],
                        		(glDebugRspBuffer+1));
                        dmaInfo.count = 3;
                    }
                    else if (dmaInfo.count == 4)
                    {
                        if (dmaInfo.buffer[3] > 0)
                        {
                                glDebugRspBuffer[0] = SensorRead (SENSOR_ADDR_RD, dmaInfo.buffer[1], dmaInfo.buffer[2],
                                		(dmaInfo.buffer[3]*2), (glDebugRspBuffer+1));
                        }
                        dmaInfo.count = dmaInfo.buffer[3]*2+1;
                    }
                }
                /*  For write commands, the register address is followed by N pairs (N>0) of register value high byte
                 *  and register value low byte to write in sequence. Response first byte is status (0=Pass, !0=Fail)
                 *  followed by N pairs of register value high byte and register value low byte after modification.
                 */
                else if (dmaInfo.buffer[0] == 1)
                {
                        glDebugRspBuffer[0] = SensorWrite (SENSOR_ADDR_WR, dmaInfo.buffer[1], dmaInfo.buffer[2],
                        		(dmaInfo.count-3), (dmaInfo.buffer+3));
                        if (glDebugRspBuffer[0] != CY_U3P_SUCCESS)
                        	break;
                        glDebugRspBuffer[0] = SensorRead (SENSOR_ADDR_RD, dmaInfo.buffer[1], dmaInfo.buffer[2],
                        		(dmaInfo.count-3), (glDebugRspBuffer+1));
                        if (glDebugRspBuffer[0] != CY_U3P_SUCCESS)
                        	break;
                    dmaInfo.count -= 2;
                }
                /* Default case, prepare buffer for loop back command in response */
                else
                {
                   /* For now, we just copy the command into the response buffer; and send it back to the
                      USB host. This can be expanded to include I2C transfers. */
                    CyU3PMemCopy (glDebugRspBuffer, dmaInfo.buffer, dmaInfo.count);
                }

                dmaInfo.buffer = glDebugRspBuffer;
                dmaInfo.size   = 1024;
                dmaInfo.status = 0;

                /* Free the command buffer to receive the next command. */
                apiRetStatus = CyU3PDmaChannelDiscardBuffer (&glDebugCmdChannel);
                if (apiRetStatus != CY_U3P_SUCCESS)
                {
                    CyU3PDebugPrint (4, "Failed to free up command OUT EP buffer, Error code = %d\r\n", apiRetStatus);
                    CyFxAppErrorHandler (apiRetStatus);
                }

                /* Wait until the response has gone out. */
                CyU3PDmaChannelWaitForCompletion (&glDebugRspChannel, CYU3P_WAIT_FOREVER);

                apiRetStatus = CyU3PDmaChannelSetupSendBuffer (&glDebugRspChannel, &dmaInfo);
                if (apiRetStatus != CY_U3P_SUCCESS)
                {
                    CyU3PDebugPrint (4, "Failed to send debug response, Error code = %d\r\n", apiRetStatus);
                    CyFxAppErrorHandler (apiRetStatus);
                }
            }
#endif
        }

        /* Allow other ready threads to run. */
        CyU3PThreadRelinquish ();
    }
}

/*
 * This function is called by the FX3 framework once the ThreadX RTOS has started up.
 * The application specific threads and other OS resources are created and initialized here.
 */
void
CyFxApplicationDefine (
        void)
{
    void *ptr1, *ptr2;
    uint32_t retThrdCreate;

    /* Allocate the memory for the thread stacks. */
    ptr1 = CyU3PMemAlloc (UVC_APP_THREAD_STACK);
    ptr2 = CyU3PMemAlloc (UVC_APP_THREAD_STACK);
    if ((ptr1 == 0) || (ptr2 == 0))
        goto fatalErrorHandler;

    /* Create the UVC application thread. */
    retThrdCreate = CyU3PThreadCreate (&uvcAppThread,   /* UVC Thread structure */
            "30:UVC App Thread",                        /* Thread Id and name */
            UVCAppThread_Entry,                         /* UVC Application Thread Entry function */
            0,                                          /* No input parameter to thread */
            ptr1,                                       /* Pointer to the allocated thread stack */
            UVC_APP_THREAD_STACK,                       /* UVC Application Thread stack size */
            UVC_APP_THREAD_PRIORITY,                    /* UVC Application Thread priority */
            UVC_APP_THREAD_PRIORITY,                    /* Threshold value for thread pre-emption. */
            CYU3P_NO_TIME_SLICE,                        /* No time slice for the application thread */
            CYU3P_AUTO_START                            /* Start the Thread immediately */
            );
    if (retThrdCreate != 0)
    {
        goto fatalErrorHandler;
    }

    /* Create the control request handling thread. */
    retThrdCreate = CyU3PThreadCreate (&uvcAppEP0Thread,        /* UVC Thread structure */
            "31:UVC App EP0 Thread",                            /* Thread Id and name */
            UVCAppEP0Thread_Entry,                              /* UVC Application EP0 Thread Entry function */
            0,                                                  /* No input parameter to thread */
            ptr2,                                               /* Pointer to the allocated thread stack */
            UVC_APP_EP0_THREAD_STACK,                           /* UVC Application Thread stack size */
            UVC_APP_EP0_THREAD_PRIORITY,                        /* UVC Application Thread priority */
            UVC_APP_EP0_THREAD_PRIORITY,                        /* Threshold value for thread pre-emption. */
            CYU3P_NO_TIME_SLICE,                                /* No time slice for the application thread */
            CYU3P_AUTO_START                                    /* Start the Thread immediately */
            );
    if (retThrdCreate != 0)
    {
        goto fatalErrorHandler;
    }

    return;

fatalErrorHandler:
    /* Add custom recovery or debug actions here */
    /* Loop indefinitely */
    while (1);
}

/* Main entry point for the C code. We perform device initialization and start
 * the ThreadX RTOS here.
 */
int
main (
        void)
{
    CyU3PReturnStatus_t apiRetStatus;
    CyU3PIoMatrixConfig_t io_cfg;

    /* Initialize the device */
    apiRetStatus = CyU3PDeviceInit (0);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        goto handle_fatal_error;
    }

    /* Turn on instruction cache to improve firmware performance. Use Release build to improve it further */
    apiRetStatus = CyU3PDeviceCacheControl (CyTrue, CyFalse, CyFalse);

    /* Configure the IO matrix for the device. */
//Changed per data sheet - Jill 6/18
    io_cfg.isDQ32Bit        = CyFalse;
    io_cfg.lppMode          = CY_U3P_IO_MATRIX_LPP_DEFAULT;
    io_cfg.gpioSimpleEn[0]  = 0;
    io_cfg.gpioSimpleEn[1]  = 0;
    io_cfg.gpioComplexEn[0] = 0;
    io_cfg.gpioComplexEn[1] = 0;
    io_cfg.useUart          = CyFalse;   /* Uart is enabled for logging. */
    io_cfg.useI2C           = CyTrue;   /* I2C is used for the sensor interface. */
    io_cfg.useI2S           = CyFalse;
    io_cfg.useSpi           = CyTrue; //CyFalse; //Changed by Daniel 4_9_2015

    apiRetStatus = CyU3PDeviceConfigureIOMatrix (&io_cfg);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        goto handle_fatal_error;
    }

    /* This is a non returnable call for initializing the RTOS kernel */
    CyU3PKernelEntry ();

    /* Dummy return to make the compiler happy */
    return 0;

handle_fatal_error:
    /* Cannot recover from this error. */
    while (1);
}

