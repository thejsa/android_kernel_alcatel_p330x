/* Copyright (c) 2013, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *         Modify History For This Module
 * When           Who             What,Where,Why
 * --------------------------------------------------------------------------------------
 * 13/10/18      Wang Penglei       Add driver for MIATA's 1st sub CAM
 * 13/10/25      Wang Penglei       Improve driver for MIATA's 1st sub CAM
 * 14/01/02      Wang Penglei       Add driver for RIO6's 2nd sub CAM
 * --------------------------------------------------------------------------------------
*/
#include "msm_sensor.h"
#include "msm_cci.h"
#include "msm_camera_io_util.h"
#define GC2035_RIO6_SENSOR_NAME "gc2035_rio6"

#undef CDBG
#ifdef CONFIG_MSMB_CAMERA_DEBUG
#define CDBG(fmt, args...) printk(fmt, ##args)
#else
#define CDBG(fmt, args...) do { } while (0)
#endif


DEFINE_MSM_MUTEX(gc2035_rio6_mut);
static struct msm_sensor_ctrl_t gc2035_rio6_s_ctrl;

static struct msm_sensor_power_setting gc2035_rio6_power_setting[] = {
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_LOW,
		.delay = 0,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_STANDBY,
		.config_val = GPIO_OUT_HIGH,
		.delay = 0,
	},
	{
		.seq_type = SENSOR_VREG,
		.seq_val = CAM_VIO,
		.config_val = 0,
		.delay = 0,
	},
	{
		.seq_type = SENSOR_VREG,
		.seq_val = CAM_VDIG,
		.config_val = 0,
		.delay = 0,
	},
	{
		.seq_type = SENSOR_CLK,
		.seq_val = SENSOR_CAM_MCLK,
		.config_val = 24000000,
		.delay = 5,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_STANDBY,
		.config_val = GPIO_OUT_LOW,
		.delay = 0,
	},
    {
        .seq_type = SENSOR_GPIO,
        .seq_val = SENSOR_GPIO_VANA,
        .config_val = GPIO_OUT_LOW,
        .delay = 5,
    },
    {
        .seq_type = SENSOR_GPIO,
        .seq_val = SENSOR_GPIO_VANA,
        .config_val = GPIO_OUT_HIGH,
        .delay = 5,
    },  
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_HIGH,
		.delay = 1,
	},
};



static struct msm_camera_i2c_reg_conf gc2035_rio6_start_settings[] = {
	{0xfe, 0x03},
	{0x10, 0x94},
	{0xfe, 0x00},
};

static struct msm_camera_i2c_reg_conf gc2035_rio6_stop_settings[] = {
	{0xfe, 0x03},
	{0x10, 0x00},
	{0xfe, 0x00},
};

// preview setting
static struct msm_camera_i2c_reg_conf gc2035_rio6_svga_settings[] = {
 // vga = 640*480
 // modify to 800*600
	{0xfe, 0x00},
	{0xfa, 0x00},
	{0xb6, 0x03},
	{0xc8, 0x14},

	{0x90, 0x01},
	{0x95, 0x02},
	{0x96, 0x58},
	{0x97, 0x03},
	{0x98, 0x20},

	{0xfe, 0x03},
	{0x40, 0x40},
	{0x41, 0x02},
	{0x42, 0x40},
	{0x43, 0x06},
	{0x17, 0x00},

	{0x12, 0x40},
	{0x13, 0x06},
	{0x04, 0x90},
	{0x05, 0x00},
	{0xfe, 0x00},
};

// snapshot
static struct msm_camera_i2c_reg_conf gc2035_rio6_uxga_settings_0[] = {
//uxga = 1600*1200, 2M
	{0xfe, 0x00},
	{0xb6, 0x00},
};

static struct msm_camera_i2c_reg_conf gc2035_rio6_uxga_settings_1[] = {
	{0x99, 0x11},
	{0x9a, 0x06},
	{0x9b, 0x00},
	{0x9c, 0x00},
	{0x9d, 0x00},
	{0x9e, 0x00},
	{0x9f, 0x00},
	{0xa0, 0x00},
	{0xa1, 0x00},
	{0xa2, 0x00},

	{0xfa, 0x11},
	{0xc8, 0x00},
	{0x90, 0x01},
	{0x95, 0x04},
	{0x96, 0xb0},
	{0x97, 0x06},
	{0x98, 0x40},

	{0xfe, 0x03},
	{0x12, 0x80},
	{0x13, 0x0c},
	{0x04, 0x20},
	{0x05, 0x00},
	//{0x10, 0x94},
	{0xfe, 0x00},

};

/*[BUGFIX]-Mod-BEGIN by TCTNB.WPL, bug553835, 2013/11/20 */
//set sensor init setting
static struct msm_camera_i2c_reg_conf gc2035_rio6_recommend_settings_0[] = {
	{0xfe, 0x80},  
	{0xfe, 0x80},  
	{0xfe, 0x80},  
	{0xfc, 0x06},  
	{0xf9, 0xfe},  
	{0xfa, 0x00},  
	{0xf6, 0x00},  

	{0xf7, 0x05},
	{0xf8, 0x85},

	{0xfe, 0x00},
	{0x82, 0x00},  
	{0xb3, 0x60},  
	{0xb4, 0x40},  
	{0xb5, 0x60},  
	{0x03, 0x02},  
	{0x04, 0xee},  

	{0xfe, 0x00},
	{0xec, 0x06},
	{0xed, 0x06},
	{0xee, 0x62},
	{0xef, 0x92},

	{0x0a, 0x00},  
	{0x0c, 0x00},  
	{0x0d, 0x04},  
	{0x0e, 0xc0},  
	{0x0f, 0x06},  
	{0x10, 0x58},  
	{0x17, 0x14},
	{0x18, 0x0e}, //blk mode 0e
	{0x19, 0x0c},  
	{0x1a, 0x01},  
	{0x1b, 0x8b},
	{0x1c, 0x05},
	{0x1e, 0x88},  
	{0x1f, 0x08},
	{0x20, 0x05},  
	{0x21, 0x0f},  
	{0x22, 0xf0},
	{0x23, 0xc3},  
	{0x24, 0x17},

	//AWB gray
	{0xfe, 0x01},
	{0x4f, 0x00},
	{0x4d, 0x32}, // 30
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4d, 0x42}, // 40
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4d, 0x52}, // 50
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4d, 0x62}, // 60
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4d, 0x72}, // 70
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4d, 0x82}, // 80
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4d, 0x92}, // 90
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4e, 0x04},
	{0x4f, 0x01},
	{0x50, 0x88},
	{0xfe, 0x00},
	{0x82, 0xfe},

	//AEC
	{0xfe, 0x01},  
	{0x01, 0x08},//AEC measure window
	{0x02, 0x60},
	{0x03, 0x08},
	{0x04, 0x90},
	{0x05, 0x1c},
	{0x06, 0x4a},
	{0x07, 0x2c},
	{0x08, 0x70},
	{0x0b, 0x90},//fix target enable
	{0x0c, 0x11},//center weight X4
	{0x13, 0x75},
	{0x1e, 0x41},
	{0x1b, 0x02},
	{0x11, 0x40},
	{0x1f, 0xc0},
	{0x20, 0x60},
	{0x47, 0x30},
	{0x48, 0x70},

	{0xfe, 0x00},
	{0x05, 0x01},
	{0x06, 0x0d},
	{0x07, 0x00},
	{0x08, 0x40},
	{0xfe, 0x01},
	{0x27, 0x00},
	{0x28, 0xa0},
	{0x29, 0x05},
	{0x2a, 0x00},
	{0x2b, 0x06},
	{0x2c, 0x40},
	{0x2d, 0x07},
	{0x2e, 0x80},
	{0x2f, 0x0a},
	{0x30, 0x00},
	{0x3e, 0x40},
	{0xfe, 0x00},

	{0xb6, 0x03},
	{0xfe, 0x00},  
	{0x3f, 0x00},  
	////////////BLK//////////////
	{0x40, 0x77},  // 77 ndark enable
	//{0x41, 0x0a},
	{0x42, 0x7f},  
	{0x43, 0x30},  
	{0x5c, 0x08},  
	{0x5e, 0x20},  //offset ratio
	{0x5f, 0x20},
	{0x60, 0x20},
	{0x61, 0x20},
	{0x62, 0x20},
	{0x63, 0x20},
	{0x64, 0x20},
	{0x65, 0x20},
	{0x66, 0x00},  //current ratio
	{0x67, 0x00},
	{0x68, 0x00},
	{0x69, 0x00},

	///block////////////
	{0x80, 0xff},
	{0x81, 0x26},  //skin mode?
	{0x87, 0x90},
	{0x84, 0x02},
	{0x86, 0x02},
	{0x8b, 0xbc},
	{0xb0, 0x80},
	{0xc0, 0x40},

	//cc
	{0xfe, 0x02},
	{0xc0, 0x01},
	{0xc1, 0x40},
	{0xc2, 0xfc},
	{0xc3, 0x05},
	{0xc4, 0xfc},
	{0xc5, 0x42},
	{0xc6, 0xf4},
	{0xc7, 0x44},
	{0xc8, 0xf8},
	{0xc9, 0x08},
	{0xca, 0x00},
	{0xcb, 0x40},
	{0xcc, 0xe8},
	{0xcd, 0x36},
	{0xce, 0xf6},
	{0xcf, 0x04},
	{0xe3, 0x0c},
	{0xe4, 0x44},
	{0xe5, 0xe5},
	{0xfe, 0x00},
	//AWB
	{0xfe, 0x01},
	{0x50, 0x88},
	{0x52, 0x08},
	{0x56, 0x04},
	{0x57, 0x20},
	{0x58, 0x01},
	{0x5b, 0x02},
	{0x61, 0xaa},
	{0x62, 0xaa},
	{0x71, 0x00},
	{0x72, 0x20},
	{0x74, 0x10},
	{0x77, 0x08},
	{0x78, 0xfd},
	{0x80, 0x15},
	{0x84, 0x0a},
	{0x86, 0x10},
	{0x87, 0x00},
	{0x88, 0x06},
	{0x8a, 0xc0},
	{0x89, 0x75},
	{0x84, 0x08},
	{0x8b, 0x00},
	{0x8d, 0x70},
	{0x8e, 0x70},
	{0x8f, 0xf4},
	{0xfe, 0x00},
	//lsc
	{0xfe, 0x01},
	{0xc2, 0x21},
	{0xc3, 0x18},
	{0xc4, 0x15},
	{0xc8, 0x20},
	{0xc9, 0x20},
	{0xca, 0x11},
	{0xbc, 0x58},
	{0xbd, 0x46},
	{0xbe, 0x35},
	{0xb6, 0x4b},
	{0xb7, 0x45},
	{0xb8, 0x26},
	{0xc5, 0x18},
	{0xc6, 0x15},
	{0xc7, 0x02},
	{0xcb, 0x08},
	{0xcc, 0x04},
	{0xcd, 0x09},
	{0xbf, 0x16},
	{0xc0, 0x00},
	{0xc1, 0x09},
	{0xb9, 0x1f},
	{0xba, 0x05},
	{0xbb, 0x15},
	{0xaa, 0x2c},
	{0xab, 0x2e},
	{0xac, 0x21},
	{0xad, 0x20},
	{0xae, 0x0f},
	{0xaf, 0x17},
	{0xb0, 0x21},
	{0xb1, 0x1c},
	{0xb2, 0x1c},
	{0xb3, 0x12},
	{0xb4, 0x13},
	{0xb5, 0x17},
	{0xd0, 0x0e},
	{0xd2, 0x27},
	{0xd3, 0x00},
	{0xd8, 0x20},
	{0xda, 0x1a},
	{0xdb, 0x11},
	{0xdc, 0x3a},
	{0xde, 0x05},
	{0xdf, 0x09},
	{0xd4, 0x2c},
	{0xd6, 0x08},
	{0xd7, 0x0c},
	{0xa4, 0x00},
	{0xa5, 0x00},
	{0xa6, 0x00},
	{0xa7, 0x00},
	{0xa8, 0x20},
	{0xa9, 0x00},
	{0xa1, 0x80},
	{0xa2, 0x80},

	//asde
	{0xfe, 0x01},
	{0x21, 0xdf}, //luma level
	{0xfe, 0x02},
	{0xa4, 0x00},
	{0xa5, 0x70}, //lsc th	//40
	{0xa2, 0x50}, //lsc slope  //a0
	{0xa6, 0x80},
	{0xa7, 0x80},
	{0xab, 0x31},
	{0xa9, 0x6f},
	{0xb0, 0x99},
	{0xb1, 0x34},
	{0xb3, 0x80},
	{0xde, 0xb8},
	{0x38, 0x0f}, //auto gray slope
	{0x39, 0x40}, //auto gray th
	{0x83, 0x00},
	{0x84, 0x45},
	//YCP
	{0xd1, 0x30},
	{0xd2, 0x30},
	{0xd3, 0x40},
	{0xd4, 0x80},
	{0xd5, 0x00},
	{0xdc, 0x30},
	{0xdd, 0xb8},
	{0xfe, 0x00},
	///////dndd///////////
	{0xfe, 0x02},
	{0x88, 0x15},
	{0x8c, 0xf6},
	{0x89, 0x03},
	////////EE},/////////
	{0xfe, 0x02},
	{0x90, 0x6c},
	{0x97, 0x43},
	////==============RGB Gamma
	{0xfe, 0x02},
	{0x15, 0x0a},
	{0x16, 0x12},
	{0x17, 0x19},
	{0x18, 0x1f},
	{0x19, 0x2c},
	{0x1a, 0x38},
	{0x1b, 0x42},
	{0x1c, 0x4e},
	{0x1d, 0x63},
	{0x1e, 0x76},
	{0x1f, 0x87},
	{0x20, 0x96},
	{0x21, 0xa2},
	{0x22, 0xb8},
	{0x23, 0xca},
	{0x24, 0xd8},
	{0x25, 0xe3},
	{0x26, 0xf0},
	{0x27, 0xf8},
	{0x28, 0xfd},
	{0x29, 0xff},
	//Ygamma
	{0xfe, 0x02},
	{0x2b, 0x00},
	{0x2c, 0x04},
	{0x2d, 0x09},
	{0x2e, 0x18},
	{0x2f, 0x27},
	{0x30, 0x37},
	{0x31, 0x49},
	{0x32, 0x5c},
	{0x33, 0x7e},
	{0x34, 0xa0},
	{0x35, 0xc0},
	{0x36, 0xe0},
	{0x37, 0xff},
	{0xfe, 0x00},
	{0x82, 0xfe},
	//output setting
	{0x90, 0x01},
	{0x95, 0x02},
	{0x96, 0x58},
	{0x97, 0x03},
	{0x98, 0x20},


	/////////mipi setting////////
	{0xf2, 0x00},
	{0xf3, 0x00},
	{0xf4, 0x00},
	{0xf5, 0x00},

	{0xfe, 0x03},
	{0x01, 0x03},
	{0x02, 0x37},
	{0x03, 0x10},
	{0x04, 0x90},
	{0x05, 0x00},
	{0x06, 0x90},
	{0x11, 0x1E},
	{0x12, 0x40},
	{0x13, 0x06},
	{0x17, 0x00},

	{0x40, 0x40},
	{0x41, 0x02},
	{0x42, 0x40},
	{0x43, 0x06},

	{0x21, 0x01},
	{0x22, 0x03},
	{0x23, 0x01},
	{0x29, 0x03},
	{0x2a, 0x01},
	{0xfe, 0x00},
};

static struct msm_camera_i2c_reg_conf gc2035_rio6_recommend_settings_1[] = {
	////for awb clear/// 
	{0xfe, 0x01},
	{0x4f, 0x00}, 
	{0x4d, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4d, 0x10}, 
	{0x4e, 0x00}, 
	{0x4e, 0x00}, 
	{0x4e, 0x00}, 
	{0x4e, 0x00}, 
	{0x4e, 0x00}, 
	{0x4e, 0x00}, 
	{0x4e, 0x00}, 
	{0x4e, 0x00}, 
	{0x4e, 0x00}, 
	{0x4e, 0x00}, 
	{0x4e, 0x00}, 
	{0x4e, 0x00}, 
	{0x4e, 0x00}, 
	{0x4e, 0x00}, 
	{0x4e, 0x00}, 
	{0x4e, 0x00}, 
	{0x4d, 0x20},  ///////////////20
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4d, 0x30}, //////////////////30
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4d, 0x40},  //////////////////////40
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00}, 
	{0x4e, 0x00}, 
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4d, 0x50}, //////////////////50
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00}, 
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00}, 
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4d, 0x60}, /////////////////60
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00}, 
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4d, 0x70}, ///////////////////70
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00}, 
	{0x4e, 0x00}, 
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4d, 0x80}, /////////////////////80
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00}, 
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00}, 		  
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00}, 
	{0x4d, 0x90}, //////////////////////90
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00}, 
	{0x4e, 0x00}, 
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00}, 
	{0x4d, 0xa0}, /////////////////a0
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00}, 
	{0x4d, 0xb0}, //////////////////////////////////b0
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00}, 
	{0x4e, 0x00}, 
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4d, 0xc0}, //////////////////////////////////c0
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00}, 
	{0x4e, 0x00}, 
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4d, 0xd0}, ////////////////////////////d0
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00}, 
	{0x4e, 0x00}, 
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00}, 
	{0x4d, 0xe0}, /////////////////////////////////e0
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00}, 
	{0x4e, 0x00}, 
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4d, 0xf0}, /////////////////////////////////f0
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00}, 
	{0x4e, 0x00}, 
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4f, 0x01}, 
	////for awb vlaue//
	{0xfe, 0x01},
	{0x4f, 0x00},
	{0x4d, 0x34},
	{0x4e, 0x04},
	{0x4e, 0x02},
	{0x4d, 0x43},
	{0x4e, 0x00}, //08
	{0x4e, 0x04}, // 04
	{0x4e, 0x00},
	{0x4e, 0x00},
	{0x4d, 0x53},
	{0x4e, 0x00}, //08
	{0x4d, 0x63},
	{0x4e, 0x00}, // 10
	{0x4d, 0x72},
	{0x4e, 0x20}, // 20
	{0x4f, 0x01},
	{0xfe, 0x00},
};


/*[BUGFIX]-Mod-END by TCTNB.WPL, bug553835, 2013/11/20 */

static struct msm_camera_i2c_reg_conf GC2035_RIO6_reg_saturation[11][5] = {
	{
{0xfe, 0x02},{0xd1, 0x10},{0xd2, 0x10},{0xfe, 0x00}
	},
	{
{0xfe, 0x02},{0xd1, 0x18},{0xd2, 0x18},{0xfe, 0x00}
	},
	{
{0xfe, 0x02},{0xd1, 0x20},{0xd2, 0x20},{0xfe, 0x00}
	},
	{
{0xfe, 0x02},{0xd1, 0x28},{0xd2, 0x28},{0xfe, 0x00}
	},
	{
{0xfe, 0x02},{0xd1, 0x2c},{0xd2, 0x2c},{0xfe, 0x00}
	},
	{
{0xfe, 0x02},{0xd1, 0x30},{0xd2, 0x30},{0xfe, 0x00}
	},
	{
{0xfe, 0x02},{0xd1, 0x40},{0xd2, 0x40},{0xfe, 0x00}
	},
	{
{0xfe, 0x02},{0xd1, 0x48},{0xd2, 0x48},{0xfe, 0x00}
	},
	{
{0xfe, 0x02},{0xd1, 0x50},{0xd2, 0x50},{0xfe, 0x00}
	},
	{
{0xfe, 0x02},{0xd1, 0x58},{0xd2, 0x58},{0xfe, 0x00}
	},
	{
{0xfe, 0x02},{0xd1, 0x60},{0xd2, 0x60},{0xfe, 0x00}
	},
};

static struct msm_camera_i2c_reg_conf GC2035_RIO6_reg_contrast[11][3] = {
	{
{0xfe, 0x02},{0xd3, 0x18},{0xfe, 0x00}
	},
	{
{0xfe, 0x02},{0xd3, 0x20},{0xfe, 0x00}
	},
	{
{0xfe, 0x02},{0xd3, 0x28},{0xfe, 0x00}
	},
	{
{0xfe, 0x02},{0xd3, 0x30},{0xfe, 0x00}
	},
	{
{0xfe, 0x02},{0xd3, 0x36},{0xfe, 0x00}
	},
	{
{0xfe, 0x02},{0xd3, 0x38},{0xfe, 0x00}
	},
	{
{0xfe, 0x02},{0xd3, 0x48},{0xfe, 0x00}
	},
	{
{0xfe, 0x02},{0xd3, 0x50},{0xfe, 0x00}
	},
	{
{0xfe, 0x02},{0xd3, 0x58},{0xfe, 0x00}
	},
	{
{0xfe, 0x02},{0xd3, 0x60},{0xfe, 0x00}
	},
	{
{0xfe, 0x02},{0xd3, 0x68},{0xfe, 0x00}
	},
};

static struct msm_camera_i2c_reg_conf GC2035_RIO6_reg_sharpness[6][3] = {
	{{0xfe, 0x02},{0x97, 0x20},{0xfe, 0x00}},//Sharpness -2
	{{0xfe, 0x02},{0x97, 0x32},{0xfe, 0x00}},//Sharpness -1
	{{0xfe, 0x02},{0x97, 0x34},{0xfe, 0x00}},//Sharpness
	{{0xfe, 0x02},{0x97, 0x45},{0xfe, 0x00}},//Sharpness +1
	{{0xfe, 0x02},{0x97, 0x56},{0xfe, 0x00}},//Sharpness +2
	{{0xfe, 0x02},{0x97, 0x78},{0xfe, 0x00}},//Sharpness +3
};
static struct msm_camera_i2c_reg_conf GC2035_RIO6_reg_exposure_compensation[5][3] = {
	{{0xfe, 0x01},{0x13, 0x60},{0xfe, 0x00}},//Exposure -2 30
	{{0xfe, 0x01},{0x13, 0x70},{0xfe, 0x00}},//Exposure -1 34
	{{0xfe, 0x01},{0x13, 0x75},{0xfe, 0x00}},//Exposure      36
	{{0xfe, 0x01},{0x13, 0x90},{0xfe, 0x00}},//Exposure +1 50
	{{0xfe, 0x01},{0x13, 0xa0},{0xfe, 0x00}},//Exposure +2 60
};
static struct msm_camera_i2c_reg_conf GC2035_RIO6_reg_antibanding[][17] = {
	/* OFF */
	{
	{0xfe, 0x00},{0x05, 0x01},{0x06, 0x58},{0x07, 0x00},{0x08, 0x22},{0xfe, 0x01},{0x27, 0x00},{0x28, 0x7d}, 
	{0x29, 0x04},{0x2a, 0xe2}, {0x2b, 0x05},{0x2c, 0xdc},{0x2d, 0x05},{0x2e, 0xdc},{0x2f, 0x07},{0x30, 0x53}
	}, /*ANTIBANDING 60HZ*/


	/* 50Hz */
	{
	{0xfe, 0x00},{0x05, 0x01},{0x06, 0x58},{0x07, 0x00},{0x08, 0x86},{0xfe, 0x01},{0x27, 0x00},{0x28, 0x96}, 
	{0x29, 0x05},{0x2a, 0x46}, {0x2b, 0x05},{0x2c, 0xdc},{0x2d, 0x05},{0x2e, 0xdc},{0x2f, 0x07},{0x30, 0x9e}
	}, /*ANTIBANDING 50HZ*/


	/* 60Hz */
	{
	{0xfe, 0x00},{0x05, 0x01},{0x06, 0x58},{0x07, 0x00},{0x08, 0x22},{0xfe, 0x01},{0x27, 0x00},{0x28, 0x7d}, 
	{0x29, 0x04},{0x2a, 0xe2}, {0x2b, 0x05},{0x2c, 0xdc},{0x2d, 0x05},{0x2e, 0xdc},{0x2f, 0x07},{0x30, 0x53}
	}, /*ANTIBANDING 60HZ*/


	/* AUTO */
	{
	{0xfe, 0x00},{0x05, 0x01},{0x06, 0x58},{0x07, 0x00},{0x08, 0x86},{0xfe, 0x01},{0x27, 0x00},{0x28, 0x96}, 
	{0x29, 0x05},{0x2a, 0x46}, {0x2b, 0x05},{0x2c, 0xdc},{0x2d, 0x05},{0x2e, 0xdc},{0x2f, 0x07},{0x30, 0x9e}
	},/*ANTIBANDING 50HZ*/
};

//begin effect
static struct msm_camera_i2c_reg_conf GC2035_RIO6_reg_effect_normal[] = {
	/* normal: */
	{0x83, 0xe0},
};

static struct msm_camera_i2c_reg_conf GC2035_RIO6_reg_effect_black_white[] = {
	/* B&W: */
{0x83, 0x12}
};

static struct msm_camera_i2c_reg_conf GC2035_RIO6_reg_effect_negative[] = {
	/* Negative: */
{0x83, 0x01}
};

static struct msm_camera_i2c_reg_conf GC2035_RIO6_reg_effect_old_movie[] = {
	/* Sepia(antique): */
{0x83, 0x82}
};

static struct msm_camera_i2c_reg_conf GC2035_RIO6_reg_effect_solarize[] = {
	{0xfe, 0x00},
};
// end effect


//begin scene, not realised
static struct msm_camera_i2c_reg_conf GC2035_RIO6_reg_scene_auto[] = {
	/* <SCENE_auto> */
		{0xfe, 0x00},

};

static struct msm_camera_i2c_reg_conf GC2035_RIO6_reg_scene_portrait[] = {
	/* <CAMTUNING_SCENE_PORTRAIT> */
		{0xfe, 0x00},

};

static struct msm_camera_i2c_reg_conf GC2035_RIO6_reg_scene_landscape[] = {
	/* <CAMTUNING_SCENE_LANDSCAPE> */
		{0xfe, 0x00},

};

static struct msm_camera_i2c_reg_conf GC2035_RIO6_reg_scene_night[] = {
	/* <SCENE_NIGHT> */
		{0xfe, 0x00},

};
//end scene


//begin white balance
static struct msm_camera_i2c_reg_conf GC2035_RIO6_reg_wb_auto[] = {
	/* Auto: */
{0xb3, 0x61},{0xb4, 0x40},{0xb5, 0x61},{0x82, 0xfe}
};

static struct msm_camera_i2c_reg_conf GC2035_RIO6_reg_wb_sunny[] = {
	/* Sunny: */
{0x82, 0xfe},{0xb3, 0x58},{0xb4, 0x40},{0xb5, 0x50}
};

static struct msm_camera_i2c_reg_conf GC2035_RIO6_reg_wb_cloudy[] = {
	/* Cloudy: */
{0x82, 0xfe},{0xb3, 0x8c},{0xb4, 0x50},{0xb5, 0x40}
};

static struct msm_camera_i2c_reg_conf GC2035_RIO6_reg_wb_office[] = {
	/* Office: */
{0x82, 0xfe},{0xb3, 0x72},{0xb4, 0x40},{0xb5, 0x5b}
};

static struct msm_camera_i2c_reg_conf GC2035_RIO6_reg_wb_home[] = {
	/* Home: */
{0x82, 0xfe},{0xb3, 0x50},{0xb4, 0x40},{0xb5, 0xa8}
};
//end white balance



static void gc2035_rio6_set_stauration(struct msm_sensor_ctrl_t *s_ctrl, int value)
{
	pr_debug("%s %d", __func__, value);

	s_ctrl->sensor_i2c_client->i2c_func_tbl->
	i2c_write_conf_tbl(
	s_ctrl->sensor_i2c_client, GC2035_RIO6_reg_saturation[value],
	ARRAY_SIZE(GC2035_RIO6_reg_saturation[value]),
	MSM_CAMERA_I2C_BYTE_DATA);
}

static void gc2035_rio6_set_contrast(struct msm_sensor_ctrl_t *s_ctrl, int value)
{
	pr_debug("%s %d", __func__, value);

	s_ctrl->sensor_i2c_client->i2c_func_tbl->
	i2c_write_conf_tbl(
	s_ctrl->sensor_i2c_client, GC2035_RIO6_reg_contrast[value],
	ARRAY_SIZE(GC2035_RIO6_reg_contrast[value]),
	MSM_CAMERA_I2C_BYTE_DATA);
}

static void gc2035_rio6_set_sharpness(struct msm_sensor_ctrl_t *s_ctrl, int value)
{
	int val = value / 6;
	pr_debug("%s %d", __func__, value);

	s_ctrl->sensor_i2c_client->i2c_func_tbl->
	i2c_write_conf_tbl(
	s_ctrl->sensor_i2c_client, GC2035_RIO6_reg_sharpness[val],
	ARRAY_SIZE(GC2035_RIO6_reg_sharpness[val]),
	MSM_CAMERA_I2C_BYTE_DATA);
}
static void gc2035_rio6_set_exposure_compensation(struct msm_sensor_ctrl_t *s_ctrl,
	int value)
{
	int val = (value + 12) / 6;
	pr_debug("%s %d", __func__, val);

	s_ctrl->sensor_i2c_client->i2c_func_tbl->
	i2c_write_conf_tbl(
	s_ctrl->sensor_i2c_client, GC2035_RIO6_reg_exposure_compensation[val],
	ARRAY_SIZE(GC2035_RIO6_reg_exposure_compensation[val]),
	MSM_CAMERA_I2C_BYTE_DATA);
}
static void gc2035_rio6_set_effect(struct msm_sensor_ctrl_t *s_ctrl, int value)
{
	pr_debug("%s %d", __func__, value);
	switch (value) {
	case MSM_CAMERA_EFFECT_MODE_OFF: {
		s_ctrl->sensor_i2c_client->i2c_func_tbl->
		i2c_write_conf_tbl(
		s_ctrl->sensor_i2c_client, GC2035_RIO6_reg_effect_normal,
		ARRAY_SIZE(GC2035_RIO6_reg_effect_normal),
		MSM_CAMERA_I2C_BYTE_DATA);
		break;
	}
	case MSM_CAMERA_EFFECT_MODE_MONO: {
		s_ctrl->sensor_i2c_client->i2c_func_tbl->
		i2c_write_conf_tbl(
		s_ctrl->sensor_i2c_client, GC2035_RIO6_reg_effect_black_white,
		ARRAY_SIZE(GC2035_RIO6_reg_effect_black_white),
		MSM_CAMERA_I2C_BYTE_DATA);
		break;
	}
	case MSM_CAMERA_EFFECT_MODE_NEGATIVE: {
		s_ctrl->sensor_i2c_client->i2c_func_tbl->
		i2c_write_conf_tbl(
		s_ctrl->sensor_i2c_client, GC2035_RIO6_reg_effect_negative,
		ARRAY_SIZE(GC2035_RIO6_reg_effect_negative),
		MSM_CAMERA_I2C_BYTE_DATA);
		break;
	}
	case MSM_CAMERA_EFFECT_MODE_SEPIA: {
		s_ctrl->sensor_i2c_client->i2c_func_tbl->
		i2c_write_conf_tbl(
		s_ctrl->sensor_i2c_client, GC2035_RIO6_reg_effect_old_movie,
		ARRAY_SIZE(GC2035_RIO6_reg_effect_old_movie),
		MSM_CAMERA_I2C_BYTE_DATA);
		break;
	}
	case MSM_CAMERA_EFFECT_MODE_SOLARIZE: {
		s_ctrl->sensor_i2c_client->i2c_func_tbl->
		i2c_write_conf_tbl(
		s_ctrl->sensor_i2c_client, GC2035_RIO6_reg_effect_solarize,
		ARRAY_SIZE(GC2035_RIO6_reg_effect_solarize),
		MSM_CAMERA_I2C_BYTE_DATA);
		break;
	}
	default:
		s_ctrl->sensor_i2c_client->i2c_func_tbl->
		i2c_write_conf_tbl(
		s_ctrl->sensor_i2c_client, GC2035_RIO6_reg_effect_normal,
		ARRAY_SIZE(GC2035_RIO6_reg_effect_normal),
		MSM_CAMERA_I2C_BYTE_DATA);
	}
}


static void gc2035_rio6_set_antibanding(struct msm_sensor_ctrl_t *s_ctrl, int value)
{
	pr_debug("%s %d", __func__, value);

	s_ctrl->sensor_i2c_client->i2c_func_tbl->
	i2c_write_conf_tbl(
	s_ctrl->sensor_i2c_client, GC2035_RIO6_reg_antibanding[value],
	ARRAY_SIZE(GC2035_RIO6_reg_antibanding[value]),
	MSM_CAMERA_I2C_BYTE_DATA);
}
static void gc2035_rio6_set_scene_mode(struct msm_sensor_ctrl_t *s_ctrl, int value)
{
	pr_debug("%s %d", __func__, value);
	switch (value) {
	case MSM_CAMERA_SCENE_MODE_OFF: {
		s_ctrl->sensor_i2c_client->i2c_func_tbl->
		i2c_write_conf_tbl(
		s_ctrl->sensor_i2c_client, GC2035_RIO6_reg_scene_auto,
		ARRAY_SIZE(GC2035_RIO6_reg_scene_auto),
		MSM_CAMERA_I2C_BYTE_DATA);
					break;
	}
	case MSM_CAMERA_SCENE_MODE_NIGHT: {
		s_ctrl->sensor_i2c_client->i2c_func_tbl->
		i2c_write_conf_tbl(
		s_ctrl->sensor_i2c_client, GC2035_RIO6_reg_scene_night,
		ARRAY_SIZE(GC2035_RIO6_reg_scene_night),
		MSM_CAMERA_I2C_BYTE_DATA);
					break;
	}
	case MSM_CAMERA_SCENE_MODE_LANDSCAPE: {
		s_ctrl->sensor_i2c_client->i2c_func_tbl->
		i2c_write_conf_tbl(
		s_ctrl->sensor_i2c_client, GC2035_RIO6_reg_scene_landscape,
		ARRAY_SIZE(GC2035_RIO6_reg_scene_landscape),
		MSM_CAMERA_I2C_BYTE_DATA);
					break;
	}
	case MSM_CAMERA_SCENE_MODE_PORTRAIT: {
		s_ctrl->sensor_i2c_client->i2c_func_tbl->
		i2c_write_conf_tbl(
		s_ctrl->sensor_i2c_client, GC2035_RIO6_reg_scene_portrait,
		ARRAY_SIZE(GC2035_RIO6_reg_scene_portrait),
		MSM_CAMERA_I2C_BYTE_DATA);
					break;
	}
	default:
		s_ctrl->sensor_i2c_client->i2c_func_tbl->
		i2c_write_conf_tbl(
		s_ctrl->sensor_i2c_client, GC2035_RIO6_reg_scene_auto,
		ARRAY_SIZE(GC2035_RIO6_reg_scene_auto),
		MSM_CAMERA_I2C_BYTE_DATA);
	}
}

static void gc2035_rio6_set_white_balance_mode(struct msm_sensor_ctrl_t *s_ctrl,
	int value)
{
	pr_debug("%s %d", __func__, value);
	switch (value) {
	case MSM_CAMERA_WB_MODE_AUTO: {
		s_ctrl->sensor_i2c_client->i2c_func_tbl->
		i2c_write_conf_tbl(
		s_ctrl->sensor_i2c_client, GC2035_RIO6_reg_wb_auto,
		ARRAY_SIZE(GC2035_RIO6_reg_wb_auto),
		MSM_CAMERA_I2C_BYTE_DATA);
		break;
	}
	case MSM_CAMERA_WB_MODE_INCANDESCENT: {
		s_ctrl->sensor_i2c_client->i2c_func_tbl->
		i2c_write_conf_tbl(
		s_ctrl->sensor_i2c_client, GC2035_RIO6_reg_wb_home,
		ARRAY_SIZE(GC2035_RIO6_reg_wb_home),
		MSM_CAMERA_I2C_BYTE_DATA);
		break;
	}
	case MSM_CAMERA_WB_MODE_DAYLIGHT: {
		s_ctrl->sensor_i2c_client->i2c_func_tbl->
		i2c_write_conf_tbl(
		s_ctrl->sensor_i2c_client, GC2035_RIO6_reg_wb_sunny,
		ARRAY_SIZE(GC2035_RIO6_reg_wb_sunny),
		MSM_CAMERA_I2C_BYTE_DATA);
					break;
	}
	case MSM_CAMERA_WB_MODE_FLUORESCENT: {
		s_ctrl->sensor_i2c_client->i2c_func_tbl->
		i2c_write_conf_tbl(
		s_ctrl->sensor_i2c_client, GC2035_RIO6_reg_wb_office,
		ARRAY_SIZE(GC2035_RIO6_reg_wb_office),
		MSM_CAMERA_I2C_BYTE_DATA);
					break;
	}
	case MSM_CAMERA_WB_MODE_CLOUDY_DAYLIGHT: {
		s_ctrl->sensor_i2c_client->i2c_func_tbl->
		i2c_write_conf_tbl(
		s_ctrl->sensor_i2c_client, GC2035_RIO6_reg_wb_cloudy,
		ARRAY_SIZE(GC2035_RIO6_reg_wb_cloudy),
		MSM_CAMERA_I2C_BYTE_DATA);
					break;
	}
	default:
		s_ctrl->sensor_i2c_client->i2c_func_tbl->
		i2c_write_conf_tbl(
		s_ctrl->sensor_i2c_client, GC2035_RIO6_reg_wb_auto,
		ARRAY_SIZE(GC2035_RIO6_reg_wb_auto),
		MSM_CAMERA_I2C_BYTE_DATA);
	}
}


static struct v4l2_subdev_info gc2035_rio6_subdev_info[] = {
	{
		.code   = V4L2_MBUS_FMT_YUYV8_2X8,
		.colorspace = V4L2_COLORSPACE_JPEG,
		.fmt    = 1,
		.order    = 0,
	},
};


static const struct i2c_device_id gc2035_rio6_i2c_id[] = {
	{GC2035_RIO6_SENSOR_NAME, (kernel_ulong_t)&gc2035_rio6_s_ctrl},
	{ }
};

static int32_t msm_gc2035_rio6_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	return msm_sensor_i2c_probe(client, id, &gc2035_rio6_s_ctrl);
}

static struct i2c_driver gc2035_rio6_i2c_driver = {
	.id_table = gc2035_rio6_i2c_id,
	.probe  = msm_gc2035_rio6_i2c_probe,
	.driver = {
		.name = GC2035_RIO6_SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client gc2035_rio6_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
};

static const struct of_device_id gc2035_rio6_dt_match[] = {
	{.compatible = "qcom,gc2035_rio6", .data = &gc2035_rio6_s_ctrl},
	{}
};

MODULE_DEVICE_TABLE(of, gc2035_rio6_dt_match);

static struct platform_driver gc2035_rio6_platform_driver = {
	.driver = {
		.name = "qcom,gc2035_rio6",
		.owner = THIS_MODULE,
		.of_match_table = gc2035_rio6_dt_match,
	},
};

static int32_t gc2035_rio6_platform_probe(struct platform_device *pdev)
{
	int32_t rc;
	const struct of_device_id *match;

	match = of_match_device(gc2035_rio6_dt_match, &pdev->dev);
	rc = msm_sensor_platform_probe(pdev, match->data);
	return rc;
}

static int __init gc2035_rio6_init_module(void)
{
	int32_t rc;
	pr_info("%s:%d\n", __func__, __LINE__);
	
	rc = platform_driver_probe(&gc2035_rio6_platform_driver,
		gc2035_rio6_platform_probe);
	if (!rc)
		return rc;
	pr_err("%s:%d rc %d\n", __func__, __LINE__, rc);
	return i2c_add_driver(&gc2035_rio6_i2c_driver);
}

static void __exit gc2035_rio6_exit_module(void)
{
	pr_info("%s:%d\n", __func__, __LINE__);
	if (gc2035_rio6_s_ctrl.pdev) {
		msm_sensor_free_sensor_data(&gc2035_rio6_s_ctrl);
		platform_driver_unregister(&gc2035_rio6_platform_driver);
	} else
		i2c_del_driver(&gc2035_rio6_i2c_driver);
	return;
}

int32_t gc2035_rio6_sensor_config(struct msm_sensor_ctrl_t *s_ctrl,
	void __user *argp)
{
	struct sensorb_cfg_data *cdata = (struct sensorb_cfg_data *)argp;
	long rc = 0;
	int32_t i = 0;
    
	mutex_lock(s_ctrl->msm_sensor_mutex);
	CDBG("%s:%d %s cfgtype = %d\n", __func__, __LINE__,
		s_ctrl->sensordata->sensor_name, cdata->cfgtype);
	switch (cdata->cfgtype) {
	case CFG_GET_SENSOR_INFO:
		CDBG("=====> INcase: [CFG_GET_SENSOR_INFO]\n");
		memcpy(cdata->cfg.sensor_info.sensor_name,
			s_ctrl->sensordata->sensor_name,
			sizeof(cdata->cfg.sensor_info.sensor_name));
		cdata->cfg.sensor_info.session_id =
			s_ctrl->sensordata->sensor_info->session_id;
		for (i = 0; i < SUB_MODULE_MAX; i++)
			cdata->cfg.sensor_info.subdev_id[i] =
				s_ctrl->sensordata->sensor_info->subdev_id[i];
		CDBG("%s:%d sensor name %s\n", __func__, __LINE__,
			cdata->cfg.sensor_info.sensor_name);
		CDBG("%s:%d session id %d\n", __func__, __LINE__,
			cdata->cfg.sensor_info.session_id);
		for (i = 0; i < SUB_MODULE_MAX; i++)
			CDBG("%s:%d subdev_id[%d] %d\n", __func__, __LINE__, i,
				cdata->cfg.sensor_info.subdev_id[i]);

		break;
	case CFG_SET_INIT_SETTING:
		CDBG("=====> wangpl INcase: [CFG_SET_INIT_SETTING]\n");
		/* 1. Write Recommend settings */
		/* 2. Write change settings */

/*[BUGFIX]-Mod-BEGIN by TCTNB.WPL, bug553835, 2013/11/20 */
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
		    i2c_write_conf_tbl(
		    s_ctrl->sensor_i2c_client, gc2035_rio6_recommend_settings_0,
		    ARRAY_SIZE(gc2035_rio6_recommend_settings_0),
		    MSM_CAMERA_I2C_BYTE_DATA);

			mdelay(150);

		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
		    i2c_write_conf_tbl(
		    s_ctrl->sensor_i2c_client, gc2035_rio6_recommend_settings_1,
		    ARRAY_SIZE(gc2035_rio6_recommend_settings_1),
		    MSM_CAMERA_I2C_BYTE_DATA);
/*[BUGFIX]-Mod-END by TCTNB.WPL, bug553835, 2013/11/20 */

		break;
	case CFG_SET_RESOLUTION: {
		int32_t val;
		uint16_t reg_03 = 0;
		uint16_t reg_04 = 0;
		uint16_t reg_shutter_snapshot = 0;
		uint16_t reg_shutter_preview = 0;

		CDBG("=====> INcase: [CFG_SET_RESOLUTION]\n");
		if (copy_from_user(&val,
			(void *)cdata->cfg.setting, sizeof(int))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}
		CDBG("=====>INcase: [CFG_SET_RESOLUTION], val=<%d>\n", val);

		if (val == 1) // val==1, preview setting
		{
//
		CDBG("=====>preview before, reg_03=<0x%x>\n", reg_03);
		CDBG("=====>preview before, reg_04=<0x%x>\n", reg_04);

		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_read(
			s_ctrl->sensor_i2c_client,
			0x03,
			&reg_03, MSM_CAMERA_I2C_BYTE_DATA);
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_read(
			s_ctrl->sensor_i2c_client,
			0x04,
			&reg_04, MSM_CAMERA_I2C_BYTE_DATA);

		reg_shutter_preview = (reg_03 << 8) | (reg_04);
		reg_shutter_preview = reg_shutter_preview << 1;
		reg_03 = (reg_shutter_preview >> 8) & 0x00FF;
		reg_04 = reg_shutter_preview & 0x00FF;

		CDBG("=====>preview, reg_03=<0x%x>\n", reg_03);
		CDBG("=====>preview, reg_04=<0x%x>\n", reg_04);

		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write(
			s_ctrl->sensor_i2c_client,
			0x03,
			reg_03, MSM_CAMERA_I2C_BYTE_DATA);
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write(
			s_ctrl->sensor_i2c_client,
			0x04,
			reg_04, MSM_CAMERA_I2C_BYTE_DATA);

		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
			i2c_write_conf_tbl(
			s_ctrl->sensor_i2c_client, gc2035_rio6_svga_settings,
			ARRAY_SIZE(gc2035_rio6_svga_settings),
			MSM_CAMERA_I2C_BYTE_DATA);
		}
		else if (val == 0) // val==0, snapshot setting
		{
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
			i2c_write_conf_tbl(
			s_ctrl->sensor_i2c_client, gc2035_rio6_uxga_settings_0,
			ARRAY_SIZE(gc2035_rio6_uxga_settings_0),
			MSM_CAMERA_I2C_BYTE_DATA);

		CDBG("=====>snapshot before, reg_03=<0x%x>\n", reg_03);
		CDBG("=====>snapshot before, reg_04=<0x%x>\n", reg_04);

		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_read(
			s_ctrl->sensor_i2c_client,
			0x03,
			&reg_03, MSM_CAMERA_I2C_BYTE_DATA);
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_read(
			s_ctrl->sensor_i2c_client,
			0x04,
			&reg_04, MSM_CAMERA_I2C_BYTE_DATA);
		CDBG("=====>read, reg_03=<0x%x>\n", reg_03);
		CDBG("=====>read, reg_04=<0x%x>\n", reg_04);

		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
			i2c_write_conf_tbl(
			s_ctrl->sensor_i2c_client, gc2035_rio6_uxga_settings_1,
			ARRAY_SIZE(gc2035_rio6_uxga_settings_1),
			MSM_CAMERA_I2C_BYTE_DATA);

		reg_shutter_snapshot = (reg_03 << 8) | (reg_04);
		reg_shutter_snapshot = reg_shutter_snapshot >> 1;
		reg_03 = (reg_shutter_snapshot >> 8) & 0x00FF;
		reg_04 = reg_shutter_snapshot & 0x00FF;

		CDBG("=====>after, reg_03=<0x%x>\n", reg_03);
		CDBG("=====>after, reg_04=<0x%x>\n", reg_04);

		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write(
			s_ctrl->sensor_i2c_client,
			0x03,
			reg_03, MSM_CAMERA_I2C_BYTE_DATA);
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write(
			s_ctrl->sensor_i2c_client,
			0x04,
			reg_04, MSM_CAMERA_I2C_BYTE_DATA);
		}
		break;
		}
	case CFG_SET_STOP_STREAM:
		CDBG("=====> INcase: [CFG_SET_STOP_STREAM]\n");
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
			i2c_write_conf_tbl(
			s_ctrl->sensor_i2c_client, gc2035_rio6_stop_settings,
			ARRAY_SIZE(gc2035_rio6_stop_settings),
			MSM_CAMERA_I2C_BYTE_DATA);
		break;
	case CFG_SET_START_STREAM:
		CDBG("=====> INcase: [CFG_SET_START_STREAM]\n");
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
			i2c_write_conf_tbl(
			s_ctrl->sensor_i2c_client, gc2035_rio6_start_settings,
			ARRAY_SIZE(gc2035_rio6_start_settings),
			MSM_CAMERA_I2C_BYTE_DATA);
		break;
	case CFG_GET_SENSOR_INIT_PARAMS:
		CDBG("=====> INcase: [CFG_GET_SENSOR_INIT_PARAMS]\n");
		cdata->cfg.sensor_init_params =
			*s_ctrl->sensordata->sensor_init_params;
		CDBG("%s:%d init params mode %d pos %d mount %d\n", __func__,
			__LINE__,
			cdata->cfg.sensor_init_params.modes_supported,
			cdata->cfg.sensor_init_params.position,
			cdata->cfg.sensor_init_params.sensor_mount_angle);
		break;
	case CFG_SET_SLAVE_INFO: {
		struct msm_camera_sensor_slave_info sensor_slave_info;
		struct msm_sensor_power_setting_array *power_setting_array;
		int slave_index = 0;
		CDBG("=====> INcase: [CFG_SET_SLAVE_INFO]\n");
		if (copy_from_user(&sensor_slave_info,
		    (void *)cdata->cfg.setting,
		    sizeof(struct msm_camera_sensor_slave_info))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}
		/* Update sensor slave address */
		if (sensor_slave_info.slave_addr) {
			s_ctrl->sensor_i2c_client->cci_client->sid =
				sensor_slave_info.slave_addr >> 1;
		}

		/* Update sensor address type */
		s_ctrl->sensor_i2c_client->addr_type =
			sensor_slave_info.addr_type;

		/* Update power up / down sequence */
		s_ctrl->power_setting_array =
			sensor_slave_info.power_setting_array;
		power_setting_array = &s_ctrl->power_setting_array;
		power_setting_array->power_setting = kzalloc(
			power_setting_array->size *
			sizeof(struct msm_sensor_power_setting), GFP_KERNEL);
		if (!power_setting_array->power_setting) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -ENOMEM;
			break;
		}
		if (copy_from_user(power_setting_array->power_setting,
		    (void *)sensor_slave_info.power_setting_array.power_setting,
		    power_setting_array->size *
		    sizeof(struct msm_sensor_power_setting))) {
			kfree(power_setting_array->power_setting);
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}
		s_ctrl->free_power_setting = true;
		CDBG("%s sensor id %x\n", __func__,
			sensor_slave_info.slave_addr);
		CDBG("%s sensor addr type %d\n", __func__,
			sensor_slave_info.addr_type);
		CDBG("%s sensor reg %x\n", __func__,
			sensor_slave_info.sensor_id_info.sensor_id_reg_addr);
		CDBG("%s sensor id %x\n", __func__,
			sensor_slave_info.sensor_id_info.sensor_id);
		for (slave_index = 0; slave_index <
			power_setting_array->size; slave_index++) {
			CDBG("%s i %d power setting %d %d %ld %d\n", __func__,
				slave_index,
				power_setting_array->power_setting[slave_index].
				seq_type,
				power_setting_array->power_setting[slave_index].
				seq_val,
				power_setting_array->power_setting[slave_index].
				config_val,
				power_setting_array->power_setting[slave_index].
				delay);
		}
		kfree(power_setting_array->power_setting);
		break;
	}
	case CFG_WRITE_I2C_ARRAY: {
		struct msm_camera_i2c_reg_setting conf_array;
		struct msm_camera_i2c_reg_array *reg_setting = NULL;
		CDBG("=====> INcase: [CFG_WRITE_I2C_ARRAY]\n");

		if (copy_from_user(&conf_array,
			(void *)cdata->cfg.setting,
			sizeof(struct msm_camera_i2c_reg_setting))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}

		reg_setting = kzalloc(conf_array.size *
			(sizeof(struct msm_camera_i2c_reg_array)), GFP_KERNEL);
		if (!reg_setting) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -ENOMEM;
			break;
		}
		if (copy_from_user(reg_setting, (void *)conf_array.reg_setting,
			conf_array.size *
			sizeof(struct msm_camera_i2c_reg_array))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			kfree(reg_setting);
			rc = -EFAULT;
			break;
		}

		conf_array.reg_setting = reg_setting;
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write_table(
			s_ctrl->sensor_i2c_client, &conf_array);
		kfree(reg_setting);
		break;
	}
	case CFG_WRITE_I2C_SEQ_ARRAY: {
		struct msm_camera_i2c_seq_reg_setting conf_array;
		struct msm_camera_i2c_seq_reg_array *reg_setting = NULL;
		CDBG("=====> INcase: [CFG_WRITE_I2C_SEQ_ARRAY]\n");

		if (copy_from_user(&conf_array,
			(void *)cdata->cfg.setting,
			sizeof(struct msm_camera_i2c_seq_reg_setting))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}

		reg_setting = kzalloc(conf_array.size *
			(sizeof(struct msm_camera_i2c_seq_reg_array)),
			GFP_KERNEL);
		if (!reg_setting) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -ENOMEM;
			break;
		}
		if (copy_from_user(reg_setting, (void *)conf_array.reg_setting,
			conf_array.size *
			sizeof(struct msm_camera_i2c_seq_reg_array))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			kfree(reg_setting);
			rc = -EFAULT;
			break;
		}

		conf_array.reg_setting = reg_setting;
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
			i2c_write_seq_table(s_ctrl->sensor_i2c_client,
			&conf_array);
		kfree(reg_setting);
		break;
	}

	case CFG_POWER_UP:
		CDBG("=====> INcase: [CFG_POWER_UP]\n");
		if (s_ctrl->func_tbl->sensor_power_up)
			rc = s_ctrl->func_tbl->sensor_power_up(s_ctrl);
		else
			rc = -EFAULT;
		break;

	case CFG_POWER_DOWN:
		CDBG("=====> INcase: [CFG_POWER_DOWN]\n");
		if (s_ctrl->func_tbl->sensor_power_down)
			rc = s_ctrl->func_tbl->sensor_power_down(
				s_ctrl);
		else
			rc = -EFAULT;
		break;

	case CFG_SET_STOP_STREAM_SETTING: {
		struct msm_camera_i2c_reg_setting *stop_setting =
			&s_ctrl->stop_setting;
		struct msm_camera_i2c_reg_array *reg_setting = NULL;
		CDBG("=====> INcase: [CFG_SET_STOP_STREAM_SETTING]\n");
		if (copy_from_user(stop_setting, (void *)cdata->cfg.setting,
		    sizeof(struct msm_camera_i2c_reg_setting))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}

		reg_setting = stop_setting->reg_setting;
		stop_setting->reg_setting = kzalloc(stop_setting->size *
			(sizeof(struct msm_camera_i2c_reg_array)), GFP_KERNEL);
		if (!stop_setting->reg_setting) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -ENOMEM;
			break;
		}
		if (copy_from_user(stop_setting->reg_setting,
		    (void *)reg_setting, stop_setting->size *
		    sizeof(struct msm_camera_i2c_reg_array))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			kfree(stop_setting->reg_setting);
			stop_setting->reg_setting = NULL;
			stop_setting->size = 0;
			rc = -EFAULT;
			break;
		}
		break;
		}
		case CFG_SET_SATURATION: {
			int32_t sat_lev;
		CDBG("=====> INcase: [CFG_SET_SATURATION]\n");
			if (copy_from_user(&sat_lev, (void *)cdata->cfg.setting,
				sizeof(int32_t))) {
				pr_err("%s:%d failed\n", __func__, __LINE__);
				rc = -EFAULT;
				break;
			}
		pr_debug("%s: Saturation Value is %d", __func__, sat_lev);
		gc2035_rio6_set_stauration(s_ctrl, sat_lev);
		break;
		}
		case CFG_SET_CONTRAST: {
			int32_t con_lev;
		CDBG("=====> INcase: [CFG_SET_CONTRAST]\n");
			if (copy_from_user(&con_lev, (void *)cdata->cfg.setting,
				sizeof(int32_t))) {
				pr_err("%s:%d failed\n", __func__, __LINE__);
				rc = -EFAULT;
				break;
			}
		pr_debug("%s: Contrast Value is %d", __func__, con_lev);
		gc2035_rio6_set_contrast(s_ctrl, con_lev);
		break;
		}
		case CFG_SET_SHARPNESS: {
			int32_t shp_lev;
		CDBG("=====> INcase: [CFG_SET_SHARPNESS]\n");
			if (copy_from_user(&shp_lev, (void *)cdata->cfg.setting,
				sizeof(int32_t))) {
				pr_err("%s:%d failed\n", __func__, __LINE__);
				rc = -EFAULT;
				break;
			}
		pr_debug("%s: Sharpness Value is %d", __func__, shp_lev);
		gc2035_rio6_set_sharpness(s_ctrl, shp_lev);
		break;
		}
		case CFG_SET_AUTOFOCUS: {
		CDBG("=====> INcase: [CFG_SET_AUTOFOCUS]\n");
		/* TO-DO: set the Auto Focus */
		pr_debug("%s: Setting Auto Focus", __func__);
		break;
		}
		case CFG_CANCEL_AUTOFOCUS: {
		CDBG("=====> INcase: [CFG_CANCEL_AUTOFOCUS]\n");
		/* TO-DO: Cancel the Auto Focus */
		pr_debug("%s: Cancelling Auto Focus", __func__);
		break;
		}
	case CFG_SET_ANTIBANDING: {
		int32_t anti_mode;
		CDBG("=====> INcase: [CFG_SET_ANTIBANDING]\n");

		if (copy_from_user(&anti_mode,
			(void *)cdata->cfg.setting,
			sizeof(int32_t))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}
		pr_debug("%s: anti-banding mode is %d", __func__,
			anti_mode);
		gc2035_rio6_set_antibanding(s_ctrl, anti_mode);
		break;
		}
	case CFG_SET_EXPOSURE_COMPENSATION: {
		int32_t ec_lev;
		CDBG("=====> INcase: [CFG_SET_EXPOSURE_COMPENSATION]\n");
		if (copy_from_user(&ec_lev, (void *)cdata->cfg.setting,
			sizeof(int32_t))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}
		pr_debug("%s: Exposure compensation Value is %d",
			__func__, ec_lev);
		gc2035_rio6_set_exposure_compensation(s_ctrl, ec_lev);
		break;
		}
	case CFG_SET_WHITE_BALANCE: {
		int32_t wb_mode;
		CDBG("=====> INcase: [CFG_SET_WHITE_BALANCE]\n");
		if (copy_from_user(&wb_mode, (void *)cdata->cfg.setting,
			sizeof(int32_t))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}
		pr_debug("%s: white balance is %d", __func__, wb_mode);
		gc2035_rio6_set_white_balance_mode(s_ctrl, wb_mode);
		break;
		}
	case CFG_SET_EFFECT: {
		int32_t effect_mode;
		CDBG("=====> INcase: [CFG_SET_EFFECT]\n");
		if (copy_from_user(&effect_mode, (void *)cdata->cfg.setting,
			sizeof(int32_t))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}
		pr_debug("%s: Effect mode is %d", __func__, effect_mode);
		gc2035_rio6_set_effect(s_ctrl, effect_mode);
		break;
		}
	case CFG_SET_BESTSHOT_MODE: {
		int32_t bs_mode;
		CDBG("=====> INcase: [CFG_SET_BESTSHOT_MODE]\n");
		if (copy_from_user(&bs_mode, (void *)cdata->cfg.setting,
			sizeof(int32_t))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}
		pr_debug("%s: best shot mode is %d", __func__, bs_mode);
		gc2035_rio6_set_scene_mode(s_ctrl, bs_mode);
		break;
		}
	case CFG_SET_ISO:
		CDBG("=====> INcase: [CFG_SET_ISO]\n");
		break;		
	default:
		rc = -EFAULT;
		break;
	}

	mutex_unlock(s_ctrl->msm_sensor_mutex);

	return rc;
}

static struct msm_sensor_fn_t gc2035_rio6_sensor_func_tbl = {
	.sensor_config = gc2035_rio6_sensor_config,
	.sensor_power_up = msm_sensor_power_up,
	.sensor_power_down = msm_sensor_power_down,
	.sensor_match_id = msm_sensor_match_id,
};

static struct msm_sensor_ctrl_t gc2035_rio6_s_ctrl = {
	.sensor_i2c_client = &gc2035_rio6_sensor_i2c_client,
	.power_setting_array.power_setting = gc2035_rio6_power_setting,
	.power_setting_array.size = ARRAY_SIZE(gc2035_rio6_power_setting),
	.msm_sensor_mutex = &gc2035_rio6_mut,
	.sensor_v4l2_subdev_info = gc2035_rio6_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(gc2035_rio6_subdev_info),
	.func_tbl = &gc2035_rio6_sensor_func_tbl,
};

module_init(gc2035_rio6_init_module);
module_exit(gc2035_rio6_exit_module);
MODULE_DESCRIPTION("Gcore 2MP YUV sensor driver");
MODULE_LICENSE("GPL v2");

