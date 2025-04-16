
/*
* This file is part of the Arducam Qwiic Camera project.
*
* Copyright 2025 Arducam Technology co., Ltd. All Rights Reserved.
*
* This work is licensed under the MIT license, see the file LICENSE for
* details.
*
*/
#ifndef __ARDUCAM_QWIIC_CAM_H
#define __ARDUCAM_QWIIC_CAM_H

#include <stdint.h>
#include <Arduino.h>
#include <Wire.h>
#include <WiFiS3.h>

/**
* @file Arducam_Qwiic_CAM.h
* @author Arducam
* @date 2025/4/14
* @version V1.0.0
* @copyright Arducam
*/

#define ARDUCHIP_FRAMES     0x01
#define ARDUCHIP_TEST1      0x00 // TEST register
#define ARDUCHIP_FIFO       0x04 // FIFO and I2C control
#define ARDUCHIP_FIFO_2     0x07 // FIFO and I2C control
#define FIFO_CLEAR_ID_MASK  0x01
#define FIFO_START_MASK     0x02

#define FIFO_RDPTR_RST_MASK 0x10
#define FIFO_WRPTR_RST_MASK 0x20
#define FIFO_CLEAR_MASK     0x80

#define ARDUCHIP_TRIG       0x44 // Trigger source
#define VSYNC_MASK          0x01
#define SHUTTER_MASK        0x02
#define CAP_DONE_MASK       0x04

#define FIFO_SIZE1          0x45 // Camera write FIFO size[7:0] for burst to read
#define FIFO_SIZE2          0x46 // Camera write FIFO size[15:8]
#define FIFO_SIZE3          0x47 // Camera write FIFO size[18:16]

#define SENSOR_DATA         0x48 // Camera write FIFO size[18:16]

#define BURST_FIFO_READ     0x3C // Burst FIFO read operation
#define SINGLE_FIFO_READ    0x3D // Single FIFO read operation

#define READ_IMAGE_LENGTH           				255
#define CAPRURE_MAX_NUM                            0xff

#define CAM_REG_POWER_CONTROL                      0X02
#define CAM_REG_SENSOR_RESET                       0X07
#define CAM_REG_FORMAT                             0X20
#define CAM_REG_CAPTURE_RESOLUTION                 0X21
#define CAM_REG_BRIGHTNESS_CONTROL                 0X22
#define CAM_REG_CONTRAST_CONTROL                   0X23
#define CAM_REG_SATURATION_CONTROL                 0X24
#define CAM_REG_EV_CONTROL                         0X25
#define CAM_REG_WHILEBALANCE_MODE_CONTROL          0X26
#define CAM_REG_COLOR_EFFECT_CONTROL               0X27
#define CAM_REG_SHARPNESS_CONTROL                  0X28
#define CAM_REG_IMAGE_QUALITY                      0x2A
#define CAM_REG_EXPOSURE_GAIN_WHILEBALANCE_CONTROL 0X30
#define CAM_REG_BURST_FIFO_READ_OPERATION          0X3C
#define CAM_REG_SINGLE_FIFO_READ_OPERATION         0X3D
#define CAM_REG_SENSOR_ID                          0x40
#define CAM_REG_YEAR_ID                            0x41
#define CAM_REG_MONTH_ID                           0x42
#define CAM_REG_DAY_ID                             0x43
#define CAM_REG_SENSOR_STATE                       0x44

#define CAM_I2C_READ_MODE                          (1 << 0)
#define CAM_REG_SENSOR_STATE_IDLE                  (1 << 1)
#define CAM_SENSOR_RESET_ENABLE                    (1 << 6)
#define CAM_FORMAT_BASICS                          (0 << 0)
#define CAM_SET_CAPTURE_MODE                       (0 << 7)
#define CAM_SET_VIDEO_MODE                         (1 << 7)

/**
 * @enum CamStatus
 * @brief Camera status
 */
 typedef enum {
    CAM_ERR_SUCCESS     = 0,  /**<Operation succeeded*/
    CAM_ERR_NO_CALLBACK = -1, /**< No callback function is registered*/
} CamStatus;

/**
 * @enum CAM_IMAGE_MODE
 * @brief Configure camera resolution
 */
typedef enum {
    CAM_IMAGE_MODE_QQVGA  = 0x00,  /**<160x120 */
    CAM_IMAGE_MODE_QVGA   = 0x01,  /**<320x240*/
    CAM_IMAGE_MODE_VGA    = 0x02,  /**<640x480*/
    CAM_IMAGE_MODE_SVGA   = 0x03,  /**<800x600*/
    CAM_IMAGE_MODE_HD     = 0x04,  /**<1280x720*/
    CAM_IMAGE_MODE_SXGAM  = 0x05,  /**<1280x960*/
    CAM_IMAGE_MODE_UXGA   = 0x06,  /**<1600x1200*/
    CAM_IMAGE_MODE_FHD    = 0x07,  /**<1920x1080*/
    CAM_IMAGE_MODE_QXGA   = 0x08,  /**<2048x1536*/
    CAM_IMAGE_MODE_WQXGA2 = 0x09,  /**<2592x1944*/
    CAM_IMAGE_MODE_96X96  = 0x0a,  /**<96x96*/
    CAM_IMAGE_MODE_128X128 = 0x0b, /**<128x128*/
    CAM_IMAGE_MODE_320X320 = 0x0c, /**<320x320*/
    /// @cond
    CAM_IMAGE_MODE_12      = 0x0d, /**<Reserve*/
    CAM_IMAGE_MODE_13      = 0x0e, /**<Reserve*/
    CAM_IMAGE_MODE_14      = 0x0f, /**<Reserve*/
    CAM_IMAGE_MODE_15      = 0x10, /**<Reserve*/
    CAM_IMAGE_MODE_NONE,
    /// @endcond
} CAM_IMAGE_MODE;

/**
 * @enum CAM_CONTRAST_LEVEL
 * @brief Configure camera contrast level
 */
typedef enum {
    CAM_CONTRAST_LEVEL_MINUS_3 = 6, /**<Level -3 */
    CAM_CONTRAST_LEVEL_MINUS_2 = 4, /**<Level -2 */
    CAM_CONTRAST_LEVEL_MINUS_1 = 2, /**<Level -1 */
    CAM_CONTRAST_LEVEL_DEFAULT = 0, /**<Level Default*/
    CAM_CONTRAST_LEVEL_1       = 1, /**<Level +1 */
    CAM_CONTRAST_LEVEL_2       = 3, /**<Level +2 */
    CAM_CONTRAST_LEVEL_3       = 5, /**<Level +3 */
} CAM_CONTRAST_LEVEL;

/**
 * @enum CAM_EV_LEVEL
 * @brief Configure camera EV level
 */
typedef enum {
    CAM_EV_LEVEL_MINUS_3 = 6, /**<Level -3 */
    CAM_EV_LEVEL_MINUS_2 = 4, /**<Level -2 */
    CAM_EV_LEVEL_MINUS_1 = 2, /**<Level -1 */
    CAM_EV_LEVEL_DEFAULT = 0, /**<Level Default*/
    CAM_EV_LEVEL_1       = 1, /**<Level +1 */
    CAM_EV_LEVEL_2       = 3, /**<Level +2 */
    CAM_EV_LEVEL_3       = 5, /**<Level +3 */
} CAM_EV_LEVEL;

/**
 * @enum CAM_STAURATION_LEVEL
 * @brief Configure camera stauration  level
 */
typedef enum {
    CAM_STAURATION_LEVEL_MINUS_3 = 6, /**<Level -3 */
    CAM_STAURATION_LEVEL_MINUS_2 = 4, /**<Level -2 */
    CAM_STAURATION_LEVEL_MINUS_1 = 2, /**<Level -1 */
    CAM_STAURATION_LEVEL_DEFAULT = 0, /**<Level Default*/
    CAM_STAURATION_LEVEL_1       = 1, /**<Level +1 */
    CAM_STAURATION_LEVEL_2       = 3, /**<Level +2 */
    CAM_STAURATION_LEVEL_3       = 5, /**<Level +3 */
} CAM_STAURATION_LEVEL;

/**
 * @enum CAM_BRIGHTNESS_LEVEL
 * @brief Configure camera brightness level
 */
typedef enum {
    CAM_BRIGHTNESS_LEVEL_MINUS_4 = 8, /**<Level -4 */
    CAM_BRIGHTNESS_LEVEL_MINUS_3 = 6, /**<Level -3 */
    CAM_BRIGHTNESS_LEVEL_MINUS_2 = 4, /**<Level -2 */
    CAM_BRIGHTNESS_LEVEL_MINUS_1 = 2, /**<Level -1 */
    CAM_BRIGHTNESS_LEVEL_DEFAULT = 0, /**<Level Default*/
    CAM_BRIGHTNESS_LEVEL_1       = 1, /**<Level +1 */
    CAM_BRIGHTNESS_LEVEL_2       = 3, /**<Level +2 */
    CAM_BRIGHTNESS_LEVEL_3       = 5, /**<Level +3 */
    CAM_BRIGHTNESS_LEVEL_4       = 7, /**<Level +4 */
} CAM_BRIGHTNESS_LEVEL;

/**
 * @enum CAM_SHARPNESS_LEVEL
 * @brief Configure camera Sharpness level
 */
typedef enum {
    CAM_SHARPNESS_LEVEL_AUTO = 0, /**<Sharpness Auto */
    CAM_SHARPNESS_LEVEL_1,        /**<Sharpness Level 1 */
    CAM_SHARPNESS_LEVEL_2,        /**<Sharpness Level 2 */
    CAM_SHARPNESS_LEVEL_3,        /**<Sharpness Level 3 */
    CAM_SHARPNESS_LEVEL_4,        /**<Sharpness Level 4 */
    CAM_SHARPNESS_LEVEL_5,        /**<Sharpness Level 5 */
    CAM_SHARPNESS_LEVEL_6,        /**<Sharpness Level 6 */
    CAM_SHARPNESS_LEVEL_7,        /**<Sharpness Level 7 */
    CAM_SHARPNESS_LEVEL_8,        /**<Sharpness Level 8 */
} CAM_SHARPNESS_LEVEL;

/**
 * @enum CAM_VIDEO_MODE
 * @brief Configure resolution in video streaming mode
 */
typedef enum {
    CAM_VIDEO_MODE_0 = 1, /**< 320x240 */
    CAM_VIDEO_MODE_1 = 2, /**< 640x480 */
} CAM_VIDEO_MODE;

/**
 * @enum CAM_IMAGE_PIX_FMT
 * @brief Configure image pixel format
 */
typedef enum {
    CAM_IMAGE_PIX_FMT_JPG    = 0x01, /**< JPEG format */
    CAM_IMAGE_PIX_FMT_RGB565 = 0x02, /**< RGB565 format */
    CAM_IMAGE_PIX_FMT_YUV    = 0x03, /**< YUV format */
    CAM_IMAGE_PIX_FMT_NONE,          /**< No defined format */
} CAM_IMAGE_PIX_FMT;

/**
 * @enum CAM_WHITE_BALANCE
 * @brief Configure white balance mode
 */
typedef enum {
    CAM_WHITE_BALANCE_MODE_DEFAULT = 0, /**< Auto */
    CAM_WHITE_BALANCE_MODE_SUNNY,       /**< Sunny */
    CAM_WHITE_BALANCE_MODE_OFFICE,      /**< Office */
    CAM_WHITE_BALANCE_MODE_CLOUDY,      /**< Cloudy*/
    CAM_WHITE_BALANCE_MODE_HOME,        /**< Home */
} CAM_WHITE_BALANCE;

/**
 * @enum CAM_COLOR_FX
 * @brief Configure special effects
 */
typedef enum {
    CAM_COLOR_FX_NONE = 0,      /**< no effect   */
    CAM_COLOR_FX_BLUEISH,       /**< cool light   */
    CAM_COLOR_FX_REDISH,        /**< warm   */
    CAM_COLOR_FX_BW,            /**< Black/white   */
    CAM_COLOR_FX_SEPIA,         /**< Sepia   */
    CAM_COLOR_FX_NEGATIVE,      /**< positive/negative inversion  */
    CAM_COLOR_FX_GRASS_GREEN,   /**< Grass green */
    CAM_COLOR_FX_OVER_EXPOSURE, /**< Over exposure*/
    CAM_COLOR_FX_SOLARIZE,      /**< Solarize   */
} CAM_COLOR_FX;

typedef enum {
    HIGH_QUALITY    = 0,
    DEFAULT_QUALITY = 1,
    LOW_QUALITY     = 2,
} IMAGE_QUALITY;

/**
* @brief Arducam Qwiic CAM Class
*/
class Arducam_Qwiic_CAM
{
private:


public:

	uint32_t totalLength;                           /**< The total length of the picture */
	uint8_t cameraId;                               /**< Model of camera module */
	uint8_t burstFirstFlag;                         /**< Flag bit for reading data for the first time in
													burst mode */
	uint8_t previewMode;                            /**< Stream mode flag */
	uint8_t currentPixelFormat;                     /**< The currently set image pixel format */
	uint8_t currentPictureMode;                     /**< Currently set resolution */
	uint8_t deviceAddress;                          /**< Device address */

	//**********************************************
	//!
	//! @brief Constructor of camera class
	//!
	//!
	//**********************************************
	Arducam_Qwiic_CAM(void); 

	//**********************************************
	//!
	//! @brief reset camera
	//!
	//**********************************************
	CamStatus reset(void); 

	//**********************************************
	//!
	//! @brief Initialize the configuration of the camera module
	//! @return Return operation status
	//**********************************************
	CamStatus begin(void);

	//**********************************************
	//!
	//! @brief Start a snapshot with specified resolution and pixel format
	//!
	//! @param mode Resolution of the camera module
	//! @param pixel_format Output image pixel format,which supports JPEG, RGB,
	//! YUV
	//!
	//! @return Return operation status
	//!
	//! @note The mode parameter must be the resolution which the current camera
	//! supported
	//**********************************************
	CamStatus takePicture(CAM_IMAGE_MODE mode, CAM_IMAGE_PIX_FMT pixel_format);

	//**********************************************
	//!
	//! @brief  Set the white balance mode Manually
	//!
	//! @param   mode White balance mode
	//!
	//! @return Return operation status
	//!
	//**********************************************
	CamStatus setAutoWhiteBalanceMode(CAM_WHITE_BALANCE mode);

	//**********************************************
	//!
	//! @brief Set special effects
	//!
	//! @param  effect Special effects mode
	//!
	//! @return Return operation status
	//!
	//**********************************************
	CamStatus setColorEffect(CAM_COLOR_FX effect);

	//**********************************************
	//!
	//! @brief Set saturation level
	//!
	//! @param   level Saturation level
	//!
	//! @return Return operation status
	//!
	//**********************************************
	CamStatus setSaturation(CAM_STAURATION_LEVEL level);

	//**********************************************
	//!
	//! @brief Set EV level
	//!
	//! @param  level EV level
	//!
	//! @return Return operation status
	//!
	//**********************************************
	CamStatus setEV(CAM_EV_LEVEL level);

	//**********************************************
	//!
	//! @brief Set Contrast level
	//!
	//! @param  level Contrast level
	//!
	//! @return Return operation status
	//!
	//**********************************************
	CamStatus setContrast(CAM_CONTRAST_LEVEL level);

	//**********************************************
	//!
	//! @brief Set Brightness level
	//!
	//! @param  level Brightness level
	//!
	//! @return Return operation status
	//!
	//**********************************************
	CamStatus setBrightness(CAM_BRIGHTNESS_LEVEL level);

	//**********************************************
	//!
	//! @brief Set Sharpness level
	//!
	//! @param  level Sharpness level
	//!
	//! @return Return operation status
	//!
	//! @note Only `3MP` cameras support sharpness control
	//**********************************************
	CamStatus setSharpness(CAM_SHARPNESS_LEVEL level);

	//**********************************************
	//!
	//! @brief Set jpeg image quality
	//!
	//! @param  qualtiy Image Quality
	//!
	//! @return Return operation status
	//**********************************************
	CamStatus setImageQuality(IMAGE_QUALITY qualtiy);

	//**********************************************
	//!
	//! @brief Write register
	//!
	//! @param  reg Register address
	//! @param  data Register value
	//!
	//! @return Return operation status
	//**********************************************
	CamStatus writeReg(uint8_t, uint8_t);

	//**********************************************
	//!
	//! @brief Read register
	//!
	//! @param  reg Register address
	//!
	//! @return Returns the value of the register
	//**********************************************
	uint8_t readReg(uint8_t);

	//**********************************************
	//!
	//! @brief Read image data with specified length to buffer
	//!
	//! @param  buff Buffer for storing camera data
	//! @param  length The length of the image data to be read
	//!
	//! @return Returns the length actually read
	//!
	//**********************************************
	uint8_t readBuff(WiFiClient *client, uint8_t*, uint32_t);

	//**********************************************
	//!
	//! @brief Read a byte from FIFO
	//!
	//! @return Returns Camera data
	//!
	//! @note Before calling this function, make sure that the data is available
	//! in the buffer
	//**********************************************
	uint8_t readByte(void);

	//**********************************************
	//!
	//! @brief Compare register bit
	//!
	//! @param  addr Register address
	//! @param  bit Bit number
	//!
	//! @return Returns comparison resuilt
	//**********************************************
	uint8_t getBit(uint8_t addr, uint8_t bit);

	//**********************************************
	//!
	//! @brief Get the length of the picture
	//!
	//! @return Return the length of the picture
	//**********************************************
	uint32_t getTotalLength(void);

};

#endif /*__ARDUCAM_QWIIC_CAM_H*/
