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
 * 13/08/31      Hu Jin       Add driver for RoilTF's 1st sub CAM
 * --------------------------------------------------------------------------------------
*/
#include "msm_sensor.h"
#include "msm_cci.h"
#include "msm_camera_io_util.h"
#define GC0313_SENSOR_NAME "gc0313"
#define PLATFORM_DRIVER_NAME "msm_camera_gc0313"
#define gc0313_obj gc0313_##obj

/*#define CONFIG_MSMB_CAMERA_DEBUG*/
#undef CDBG
#ifdef CONFIG_MSMB_CAMERA_DEBUG
#define CDBG(fmt, args...) printk(fmt, ##args)
#else
#define CDBG(fmt, args...) do { } while (0)
#endif

DEFINE_MSM_MUTEX(gc0313_mut);
static struct msm_sensor_ctrl_t gc0313_s_ctrl;

static struct msm_sensor_power_setting gc0313_power_setting[] = {
	{
		.seq_type = SENSOR_VREG,
		.seq_val = CAM_VIO,
		.config_val = 0,
		.delay = 5,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_VDIG,
		.config_val = GPIO_OUT_LOW,
		.delay = 10,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_VDIG,
		.config_val = GPIO_OUT_HIGH,
		.delay = 10,
	},
	{
		.seq_type = SENSOR_VREG,
		.seq_val = CAM_VANA,
		.config_val = 0,
		.delay = 10,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_STANDBY,
		.config_val = GPIO_OUT_HIGH,
		.delay = 30,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_STANDBY,
		.config_val = GPIO_OUT_LOW,
		.delay = 30,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_LOW,
		.delay = 5,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_HIGH,
		.delay = 30,
	},
	{
		.seq_type = SENSOR_CLK,
		.seq_val = SENSOR_CAM_MCLK,
		.config_val = 24000000,
		.delay = 5,
	},
	{
		.seq_type = SENSOR_I2C_MUX,
		.seq_val = 0,
		.config_val = 0,
		.delay = 0,
	},
};


//static struct msm_camera_i2c_reg_conf gc0313_720p_settings[] = {

//};

#if 0
static struct msm_camera_i2c_reg_conf gc0313_full_settings[] = {
	//{0xfe, 0x00},
};
#endif
#if 1
static struct msm_camera_i2c_reg_conf gc0313_recommend_settings[] = {
	{0xfe, 0x80},
	{0xfe, 0x80},
	{0xfe, 0x80},
	{0xf1, 0xf0},
	{0xf2, 0x00},
	{0xf6, 0x03},
	{0xf7, 0x03},
	{0xfc, 0x1e},
	{0xfe, 0x00},
	{0x42, 0xfd},
	{0x77, 0x6f},
	{0x78, 0x40},
	{0x79, 0x54},
	{0x42, 0xff},
	/////////////////////////////////////////////////////
	////////////////// Window Setting ///////////////////
	/////////////////////////////////////////////////////
	{0xfe, 0x00},
	{0x0d, 0x01},
	{0x0e, 0xe8},
	{0x0f, 0x02},
	{0x10, 0x88},
	{0x05, 0x00},
	{0x06, 0xde},
	{0x07, 0x00},
	{0x08, 0x24},
	{0x09, 0x00},
	{0x0a, 0x00},
	{0x0b, 0x00}, //col_start
	{0x0c, 0x04},

	/////////////////////////////////////////////////////
	////////////////// Analog & CISCTL //////////////////
	/////////////////////////////////////////////////////
	{0x17, 0x14}, //[7]hsync_always [6]close_2_frame_dbrow
	 //[5:4]CFA_seqence [3:2]dark_CFA_seqence
	 //[1]updown [0]mirror
	{0x19, 0x04}, //06//AD pipl line nunber
	{0x1b, 0x48}, //decrease FPN
	{0x1f, 0x08}, //28//decrease lag//[7:6]comv_r
	 //[5]rsthigh_en [3]txlow_en
	{0x20, 0x01}, //[0]adclk_mode for decrease power noise
	{0x21, 0x48}, //40//decrease lag
	{0x22, 0x9a}, //[7:4]Vref [1:0]DAC25
	{0x23, 0x07}, //[5:4]opa_r [3:2]ref_r [1:0]vcm_r
	{0x24, 0x16}, //PAD driver

	/////////////////////////////////////////////////////
	/////////////////// ISP Realated ////////////////////
	/////////////////////////////////////////////////////
	//////////////////////////////////
	{0x40, 0xdf},
	{0x41, 0x60},//20//22
	{0x42, 0xff},
	{0x44, 0x22}, //0x20
	{0x45, 0x00},
	{0x46, 0x02},
	{0x4d, 0x01},
	{0x4f, 0x01},
	{0x50, 0x01},
	{0x70, 0x70},

	/////////////////////////////////////////////////////
	/////////////////////// BLK /////////////////////////
	/////////////////////////////////////////////////////
	{0x26, 0xf7}, //[7]dark_current_mode:
	{0x27, 0x01}, //row select
	{0x28, 0x7f}, //BLK limit value
	{0x29, 0x38}, //[7]black_compress_en [6:0]global_offset
	{0x33, 0x1a}, //offset_ratio_G1
	{0x34, 0x1a}, //offset_ratio_G2
	{0x35, 0x1a}, //offset_ratio_R1
	{0x36, 0x1a}, //offset_ratio_B2
	{0x37, 0x1a},
	{0x38, 0x1a},
	{0x39, 0x1a},
	{0x3a, 0x1a},

	////////////////////////////////////////////////////
	//////////////////// Y Gamma ///////////////////////
	////////////////////////////////////////////////////
	{0xfe, 0x00},
	{0x63, 0x00},
	{0x64, 0x06},
	{0x65, 0x0f},
	{0x66, 0x21},
	{0x67, 0x34},
	{0x68, 0x47},
	{0x69, 0x59},
	{0x6a, 0x6c},
	{0x6b, 0x8e},
	{0x6c, 0xab},
	{0x6d, 0xc5},
	{0x6e, 0xe0},
	{0x6f, 0xfa},

	////////////////////////////////////////////////////
	////////////////// YUV to RGB //////////////////////
	////////////////////////////////////////////////////
	{0xb0, 0x13},
	{0xb1, 0x27},
	{0xb2, 0x07},
	{0xb3, 0xf5},
	{0xb4, 0xe9},
	{0xb5, 0x21},
	{0xb6, 0x21},
	{0xb7, 0xe3},
	{0xb8, 0xfb},

	////////////////////////////////////////////////////
	/////////////////////// DNDD ///////////////////////
	////////////////////////////////////////////////////
	{0x7e, 0x12},
	{0x7f, 0xc3},
	{0x82, 0x78},
	{0x84, 0x02},
	{0x89, 0xe4},//a4

	////////////////////////////////////////////////////
	////////////////////// INTPEE //////////////////////
	////////////////////////////////////////////////////
	{0x90, 0xbc},
	{0x92, 0x08},
	{0x94, 0x08},
	{0x95, 0x64},

	////////////////////////////////////////////////////
	/////////////////////// ASDE ///////////////////////
	////////////////////////////////////////////////////
	{0xfe, 0x01},
	{0x18, 0x01}, //luma_div
	{0xfe, 0x00},
	{0x9a, 0x20}, //Y offset limit
	{0x9c, 0x98}, //96//Y offset slope£¬DN b slope
	{0x9e, 0x08}, //0a//DN C slope
	{0xa2, 0x32}, //Edge effect slope
	{0xa4, 0x40}, //50//auto sa slope
	{0xaa, 0x50}, //30//ASDE low luma value OT th

	////////////////////////////////////////////////////
	//////////////////// RGB Gamma /////////////////////
	////////////////////////////////////////////////////

	{0xbf, 0x0b},
	{0xc0, 0x16},
	{0xc1, 0x29},
	{0xc2, 0x3c},
	{0xc3, 0x4f},
	{0xc4, 0x5f},
	{0xc5, 0x6f},
	{0xc6, 0x8a},
	{0xc7, 0x9f},
	{0xc8, 0xb4},
	{0xc9, 0xc6},
	{0xca, 0xd3},
	{0xcb, 0xdd},
	{0xcc, 0xe5},
	{0xcd, 0xf1},
	{0xce, 0xfa},
	{0xcf, 0xff},


	////////////////////////////////////////////////////
	/////////////////////// AEC ////////////////////////
	////////////////////////////////////////////////////
	{0xfe, 0x01},
	{0x10, 0x00},//08
	{0x11, 0x11},
	{0x12, 0x10},
	{0x13, 0x88},//45
	{0x16, 0x18},
	{0x17, 0x88},
	{0x29, 0x00},
	{0x2a, 0x83},
	{0x2b, 0x02},
	{0x2c, 0x0c},
	{0x2d, 0x04},
	{0x2e, 0x18},
	{0x2f, 0x06},
	{0x30, 0x24},
	{0x31, 0x0a},
	{0x32, 0x3c},
	{0x33, 0x20},
	{0x3c, 0x60},
	{0x3e, 0x40},

	////////////////////////////////////////////////////
	/////////////////////// YCP ////////////////////////
	////////////////////////////////////////////////////
	{0xfe, 0x00},
	{0xd1, 0x38},
	{0xd2, 0x38},
	{0xde, 0x38},
	{0xd8, 0x15},
	{0xdd, 0x00},
	{0xe4, 0x8f},
	{0xe5, 0x50},

	////////////////////////////////////////////////////
	//////////////////// DARK & RC /////////////////////
	////////////////////////////////////////////////////
	{0xfe, 0x01},
	{0x40, 0x8f}, //[7]darksun enable,[5]value orsignal
	 //[4:0]edge_width
	{0x41, 0x83}, //[7:6]lock mode,[5:4]det_strength,[3:0]lock_th
	{0x42, 0xff},
	{0x43, 0x06}, //dark_th
	{0x44, 0x1f}, //ignore[10:8], exp_th[11:8]
	{0x45, 0xff}, //ignore[7:0]
	{0x46, 0xff}, //exp_th[7:0]
	{0x47, 0x04}, //min points

	////////////////////////////////////////////////////
	////////////////////// AWB /////////////////////////
	//////////////////////////////////////////////////// 
	{0x06, 0x0d}, //10
	{0x07, 0x06},
	{0x08, 0xa4}, //94
	{0x09, 0xf2},

	{0x50, 0xfd}, //ff//AWB RGB high 
	{0x51, 0x20}, //5//AWB RGB low
	{0x52, 0x20}, //24//28 //20 //18 //Y2C
	{0x53, 0x08}, //0d //10 //08 //0d //Y2C big
	{0x54, 0x10}, //10 //20 //C inter
	{0x55, 0x20}, //0c //10 //30 //40 //C inter big
	{0x56, 0x1b}, //10 //16//AWB C max
	{0x57, 0x20}, //10//AWB C max big
	{0x58, 0xfd}, //f8
	{0x59, 0x08}, //10//p1
	{0x5a, 0x11}, //14//16
	{0x5b, 0xf0}, //ec//f0//e8 //p2
	{0x5c, 0xe8}, //e0
	{0x5d, 0x10}, //20//p1 big
	{0x5e, 0x20},
	{0x5f, 0xe0}, //p2 big
	{0x60, 0x00},
	{0x67, 0x00}, //80//AWB_outdoor_mode

	{0x6d, 0x32}, //40//AWB_number_limit
	{0x6e, 0x08}, //AWB_C_number_limit 
	{0x6f, 0x08}, //AWB_adjust_mode
	{0x70, 0x40}, //10//AWB dark mode th
	{0x71, 0x83}, //82//big C block number
	{0x72, 0x25}, //26//2e//3c//show_and_mode
	{0x73, 0x62}, //adjust_speed,adjust_margin
	{0x74, 0x1b}, //03//23 [5:4]every_N 00:2,01:4,10:8,light_temp_mode

	{0x75, 0x48}, //R_5k_gain
	{0x76, 0x40}, //B_5k_gain
	{0x77, 0xc2}, //sin T
	{0x78, 0xa5}, //cos T

	{0x79, 0x18}, //AWB_X1_cut
	{0x7a, 0x40}, //AWB_X2_cut
	{0x7b, 0xb0}, //AWB_Y1_cut
	{0x7c, 0xf5}, //AWB_Y2_cut

	{0x81, 0x80}, //R_gain_limit
	{0x82, 0x60}, //G_gain_limit
	{0x83, 0xb0},

	{0x92, 0x00},
	{0xd5, 0x0C},
	{0xd6, 0x02},
	{0xd7, 0x06},
	{0xd8, 0x05},

	{0xdd, 0x00}, //12

	{0xfe, 0x00}, //page0

	////////////////////////////////////////////////////
	////////////////////// LSC /////////////////////////
	////////////////////////////////////////////////////
	{0xfe, 0x01},
	{0xa0, 0x00},
	{0xa1, 0x3c},
	{0xa2, 0x50},
	{0xa3, 0x00},
	{0xa4, 0x00},
	{0xa5, 0x00},
	{0xa6, 0x00},
	{0xa7, 0x00},
	{0xa8, 0x12},
	{0xa9, 0x0b},
	{0xaa, 0x0c},
	{0xab, 0x0c},
	{0xac, 0x04},
	{0xad, 0x00},
	{0xae, 0x0a},
	{0xaf, 0x04},
	{0xb0, 0x00},
	{0xb1, 0x06},
	{0xb2, 0x00},
	{0xb3, 0x00},
	{0xb4, 0x3c},
	{0xb5, 0x40},
	{0xb6, 0x3a},
	{0xba, 0x2d},
	{0xbb, 0x1e},
	{0xbc, 0x1c},
	{0xc0, 0x1a},
	{0xc1, 0x17},
	{0xc2, 0x18},
	{0xc6, 0x0b},
	{0xc7, 0x09},
	{0xc8, 0x09},
	{0xb7, 0x35},
	{0xb8, 0x20},
	{0xb9, 0x20},
	{0xbd, 0x20},
	{0xbe, 0x20},
	{0xbf, 0x20},
	{0xc3, 0x00},
	{0xc4, 0x00},
	{0xc5, 0x00},
	{0xc9, 0x00},
	{0xca, 0x00},
	{0xcb, 0x00},

	//////////////////////////////////////////////////
	////////////////////// MIPI //////////////////////
	//////////////////////////////////////////////////
	{0xfe, 0x03},
	{0x01, 0x03},
	{0x02, 0x21},
	{0x03, 0x10},
	{0x04, 0x80},
	{0x05, 0x02},
	{0x06, 0x80},
	{0x11, 0x1e}, //LDI
	{0x12, 0x00}, //LWC
	{0x13, 0x05},
	{0x15, 0x10},
	{0x17, 0x00},
	{0x21, 0x01},
	{0x22, 0x02},
	{0x23, 0x01},
	{0x29, 0x02},
	{0x2a, 0x01},
	{0x10, 0x94},
	{0xfe, 0x00},
	{0x17, 0x14},

	{0xfe, 0x01},
	{0x33, 0x60},//[PLATFORM]-Mod by TCTNB.ZL, 2013/02/19, for bug403631*/
	{0x21, 0xf0},
	{0x22, 0x80},
	{0xfe, 0x00},
	{0x42, 0x77},//7f
	{0xd0, 0x40},
	{0xd1, 0x38},//32
	{0xd2, 0x38},//32
	{0xd3, 0x40},//36//40
	{0xd6, 0xed},//awb
	{0xd7, 0x19},//awb
	{0xd8, 0x16},//awb
	{0xfe, 0x01},
	{0x71, 0x82},
	{0x56, 0x1b},
	{0x67, 0x03},
	{0x69, 0xb0},
	{0x84, 0x80},
	{0x85, 0x58},
	{0x86, 0x4a},
	{0xfe, 0x00},

	{0x42, 0xfd},
	{0x77, 0x6f},
	{0x78, 0x40},
	{0x79, 0x54},
	{0x42, 0xff},

	////////20130503/////////
	{0xfe, 0x00},
	{0xaa, 0x60},
	{0x42, 0x7f},
	{0xd0, 0x40},
	{0x7e, 0x14},
	{0x7f, 0xc1},
	{0x95, 0x86},
	{0xfe, 0x01},
	{0x33, 0x60},
	{0xfe, 0x00},
};
#else

static struct msm_camera_i2c_reg_conf gc0313_recommend_settings_0[] = {
	{0xfe, 0x80},
	{0xfe, 0x80},
	{0xfe, 0x80},
	{0xfe, 0x10},
	{0xfe, 0x10},
	{0xf1, 0xf0},
	{0xf2, 0x00},
	{0xf6, 0x03},
	{0xf7, 0x03},
	{0xfc, 0x1e},
	{0xfe, 0x00},
	{0x42, 0xff},
};

static struct msm_camera_i2c_reg_conf gc0313_recommend_settings_1[] = {
	/////////////////////////////////////////////////////
	////////////////// Window Setting ///////////////////
	/////////////////////////////////////////////////////
	{0xfe, 0x00},
	{0x0d, 0x01},
	{0x0e, 0xe8},
	{0x0f, 0x02},
	{0x10, 0x88},
	{0x05, 0x00},
	{0x06, 0xde},
	{0x07, 0x00},
	{0x08, 0xa7},
	{0x09, 0x00},
	{0x0a, 0x00},
	{0x0b, 0x00},
	{0x0c, 0x04},

	/////////////////////////////////////////////////////
	////////////////// Analog & CISCTL //////////////////
	/////////////////////////////////////////////////////
	{0x17, 0x14},
	{0x19, 0x04},
	{0x1b, 0x48},
	{0x1f, 0x08},
	{0x20, 0x01},
	{0x21, 0x48},
	{0x22, 0x9a},
	{0x23, 0x07},
	{0x24, 0x16},

	/////////////////////////////////////////////////////
	/////////////////// ISP Realated ////////////////////
	/////////////////////////////////////////////////////
	//////////////////////////////////
	//{0x40, 0xdf},
	//{0x41, 0x26},
	//{0x42, 0xff},
	{0x44, 0x22}, //0x20
	{0x45, 0x00},
	{0x46, 0x02},
	{0x4d, 0x01},
	//{0x4f, 0x01},
	{0x50, 0x01},
	//{0x70, 0x70},

	/////////////////////////////////////////////////////
	/////////////////////// BLK /////////////////////////
	/////////////////////////////////////////////////////
	{0x26, 0xf7},
	{0x27, 0x01},
	{0x28, 0x7f},
	{0x29, 0x38},
	{0x33, 0x1a},
	{0x34, 0x1a},
	{0x35, 0x1a},
	{0x36, 0x1a},
	{0x37, 0x1a},
	{0x38, 0x1a},
	{0x39, 0x1a},
	{0x3a, 0x1a},

	////////////////////////////////////////////////////
	//////////////////// Y Gamma ///////////////////////
	////////////////////////////////////////////////////
	{0xfe, 0x00},
	{0x63, 0x00},
	{0x64, 0x06},
	{0x65, 0x0f},
	{0x66, 0x21},
	{0x67, 0x34},
	{0x68, 0x47},
	{0x69, 0x59},
	{0x6a, 0x6c},
	{0x6b, 0x8e},
	{0x6c, 0xab},
	{0x6d, 0xc5},
	{0x6e, 0xe0},
	{0x6f, 0xfa},

	////////////////////////////////////////////////////
	////////////////// YUV to RGB //////////////////////
	////////////////////////////////////////////////////
	{0xb0, 0x13},
	{0xb1, 0x27},
	{0xb2, 0x07},
	{0xb3, 0xf6},
	{0xb4, 0xe0},
	{0xb5, 0x29},
	{0xb6, 0x24},
	{0xb7, 0xdf},
	{0xb8, 0xfd},

	////////////////////////////////////////////////////
	/////////////////////// DNDD ///////////////////////
	////////////////////////////////////////////////////
	{0x7e, 0x12},
	{0x7f, 0xc3},
	//{0x82, 0x78},
	{0x84, 0x02},
	{0x89, 0xa4},

	////////////////////////////////////////////////////
	////////////////////// INTPEE //////////////////////
	////////////////////////////////////////////////////
	//{0x90, 0xbc},
	{0x92, 0x08},
	{0x94, 0x08},
	//{0x95, 0x64},

	////////////////////////////////////////////////////
	/////////////////////// ASDE ///////////////////////
	////////////////////////////////////////////////////
	{0xfe, 0x01},
	{0x18, 0x01},
	{0xfe, 0x00},
	{0x9a, 0x20},
	{0x9c, 0x98},
	{0x9e, 0x08},
	{0xa2, 0x32},
	{0xa4, 0x40},
	{0xaa, 0x50},

	////////////////////////////////////////////////////
	//////////////////// RGB Gamma /////////////////////
	////////////////////////////////////////////////////
	#if 0
	{0xbf, 0x0e},
	{0xc0, 0x1c},
	{0xc1, 0x34},
	{0xc2, 0x48},
	{0xc3, 0x5a},
	{0xc4, 0x6b},
	{0xc5, 0x7b},
	{0xc6, 0x95},
	{0xc7, 0xab},
	{0xc8, 0xbf},
	{0xc9, 0xce},
	{0xca, 0xd9},
	{0xcb, 0xe4},
	{0xcc, 0xec},
	{0xcd, 0xf7},
	{0xce, 0xfd},
	{0xcf, 0xff},
	#endif

	////////////////////////////////////////////////////
	/////////////////////// AEC ////////////////////////
	////////////////////////////////////////////////////
	{0xfe, 0x01},
	{0x10, 0x08},
	{0x11, 0x11},
	{0x12, 0x13},
	//{0x13, 0x45},
	{0x16, 0x18},
	{0x17, 0x88},
	{0x29, 0x00},
	{0x2a, 0x83},
	{0x2b, 0x02},
	{0x2c, 0x8f},
	{0x2d, 0x03},
	{0x2e, 0x95},
	{0x2f, 0x06},
	{0x30, 0x24},
	{0x31, 0x0c},
	{0x32, 0x48},
	{0x33, 0x20},
	{0x3c, 0x60},
	{0x3e, 0x40},

	////////////////////////////////////////////////////
	/////////////////////// YCP ////////////////////////
	////////////////////////////////////////////////////
	{0xfe, 0x00},
	//{0xd1, 0x36},
	//{0xd2, 0x36},
	{0xde, 0x38},
	{0xd8, 0x15},
	{0xdd, 0x00},
	{0xe4, 0x8f},
	{0xe5, 0x50},

	////////////////////////////////////////////////////
	//////////////////// DARK & RC /////////////////////
	////////////////////////////////////////////////////
	{0xfe, 0x01},
	{0x40, 0x8f},
	{0x41, 0x83},
	//{0x42, 0xff},
	{0x43, 0x06},
	{0x44, 0x1f},
	{0x45, 0xff},
	{0x46, 0xff},
	{0x47, 0x04},

	////////////////////////////////////////////////////
	////////////////////// AWB /////////////////////////
	////////////////////////////////////////////////////
	{0x06, 0x0d},
	{0x07, 0x06},
	{0x08, 0xa4},
	{0x09, 0xf2},
	{0x50, 0xfd},
	{0x51, 0x20},
	{0x52, 0x24},
	{0x53, 0x08},
	{0x54, 0x0b},
	{0x55, 0x0f},
	{0x56, 0x0b},
	{0x57, 0x20},
	{0x58, 0xf6},
	{0x59, 0x0b},
	{0x5a, 0x11},
	{0x5b, 0xf0},
	{0x5c, 0xe8},
	{0x5d, 0x10},
	{0x5e, 0x20},
	{0x5f, 0xe0},
	{0x67, 0x00},
	{0x6d, 0x32},
	{0x6e, 0x08},
	{0x6f, 0x08},
	{0x70, 0x40},
	{0x71, 0x83},
	{0x72, 0x26},
	{0x73, 0x62},
	{0x74, 0x03},
	{0x75, 0x48},
	{0x76, 0x40},
	{0x77, 0xc2},
	{0x78, 0xa5},
	{0x79, 0x18},
	{0x7a, 0x40},
	{0x7b, 0xb0},
	{0x7c, 0xf5},
	{0x81, 0x80},
	{0x82, 0x60},
	{0x83, 0x80},
	{0x92, 0x00},
	{0xd5, 0x0C},
	{0xd6, 0x02},
	{0xd7, 0x06},
	{0xd8, 0x05},
	{0xdd, 0x00},
	{0xfe, 0x00},

	////////////////////////////////////////////////////
	////////////////////// LSC /////////////////////////
	////////////////////////////////////////////////////
	{0xfe, 0x01},
	{0xa0, 0x00},
	{0xa1, 0x3c},
	{0xa2, 0x50},
	{0xa3, 0x00},
	{0xa4, 0x00},
	{0xa5, 0x00},
	{0xa6, 0x00},
	{0xa7, 0x00},
	{0xa8, 0x00},
	{0xa9, 0x00},
	{0xaa, 0x00},
	{0xab, 0x00},
	{0xac, 0x00},
	{0xad, 0x00},
	{0xae, 0x00},
	{0xaf, 0x00},
	{0xb0, 0x00},
	{0xb1, 0x00},
	{0xb2, 0x00},
	{0xb3, 0x00},
	{0xb4, 0x21},
	{0xb5, 0x1e},
	{0xb6, 0x18},
	{0xba, 0x28},
	{0xbb, 0x24},
	{0xbc, 0x1c},
	{0xc0, 0x15},
	{0xc1, 0x14},
	{0xc2, 0x11},
	{0xc6, 0x12},
	{0xc7, 0x12},
	{0xc8, 0x11},
	{0xb7, 0x20},
	{0xb8, 0x20},
	{0xb9, 0x20},
	{0xbd, 0x20},
	{0xbe, 0x20},
	{0xbf, 0x20},
	{0xc3, 0x00},
	{0xc4, 0x00},
	{0xc5, 0x00},
	{0xc9, 0x00},
	{0xca, 0x00},
	{0xcb, 0x00},

	//////////////////////////////////////////////////
	////////////////////// MIPI //////////////////////
	//////////////////////////////////////////////////
	{0xfe, 0x03},
	{0x01, 0x03},
	{0x02, 0x21},
	{0x03, 0x10},
	{0x04, 0x80},
	{0x05, 0x02},
	{0x06, 0x80},
	{0x11, 0x1e},
	{0x12, 0x00},
	{0x13, 0x05},
	{0x15, 0x12},
	{0x17, 0x00},
	{0x21, 0x01},
	{0x22, 0x02},
	{0x23, 0x01},
	{0x29, 0x02},
	{0x2a, 0x01},
	{0x10, 0x94},
	{0xfe, 0x00},
	{0x17, 0x14},
};
#endif
static struct v4l2_subdev_info gc0313_subdev_info[] = {
	{
		.code   = V4L2_MBUS_FMT_YUYV8_2X8,
		.colorspace = V4L2_COLORSPACE_JPEG,
		.fmt    = 1,
		.order    = 0,
	},
};
#if 0
static struct msm_camera_i2c_reg_conf gc0313_config_change_settings[] = {
    
};
#endif
static const struct i2c_device_id gc0313_i2c_id[] = {
	{GC0313_SENSOR_NAME, (kernel_ulong_t)&gc0313_s_ctrl},
	{ }
};

static int32_t msm_gc0313_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	return msm_sensor_i2c_probe(client, id, &gc0313_s_ctrl);
}

static struct i2c_driver gc0313_i2c_driver = {
	.id_table = gc0313_i2c_id,
	.probe  = msm_gc0313_i2c_probe,
	.driver = {
		.name = GC0313_SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client gc0313_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
};

static const struct of_device_id gc0313_dt_match[] = {
	{.compatible = "qcom,gc0313", .data = &gc0313_s_ctrl},
	{}
};

MODULE_DEVICE_TABLE(of, gc0313_dt_match);

static struct platform_driver gc0313_platform_driver = {
	.driver = {
		.name = "qcom,gc0313",
		.owner = THIS_MODULE,
		.of_match_table = gc0313_dt_match,
	},
};

static int32_t gc0313_platform_probe(struct platform_device *pdev)
{
	int32_t rc;
	const struct of_device_id *match;
	match = of_match_device(gc0313_dt_match, &pdev->dev);
	rc = msm_sensor_platform_probe(pdev, match->data);
	return rc;
}

static int __init gc0313_init_module(void)
{
	int32_t rc;
	pr_info("%s:%d\n", __func__, __LINE__);
	rc = platform_driver_probe(&gc0313_platform_driver,
		gc0313_platform_probe);
	if (!rc)
		return rc;
	pr_err("%s:%d rc %d\n", __func__, __LINE__, rc);
	return i2c_add_driver(&gc0313_i2c_driver);
}

static void __exit gc0313_exit_module(void)
{
	pr_info("%s:%d\n", __func__, __LINE__);
	if (gc0313_s_ctrl.pdev) {
		msm_sensor_free_sensor_data(&gc0313_s_ctrl);
		platform_driver_unregister(&gc0313_platform_driver);
	} else
		i2c_del_driver(&gc0313_i2c_driver);
	return;
}

int32_t gc0313_sensor_config(struct msm_sensor_ctrl_t *s_ctrl,
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
		CDBG("=====> INcase: [CFG_SET_INIT_SETTING]\n");
		/* 1. Write Recommend settings */
		/* 2. Write change settings */
#if 0
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
			i2c_write_conf_tbl(
			s_ctrl->sensor_i2c_client, gc0313_recommend_settings_0,
			ARRAY_SIZE(gc0313_recommend_settings_0),
			MSM_CAMERA_I2C_BYTE_DATA);
		msleep(50);

		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
			i2c_write_conf_tbl(
			s_ctrl->sensor_i2c_client, gc0313_recommend_settings_1,
			ARRAY_SIZE(gc0313_recommend_settings_1),
			MSM_CAMERA_I2C_BYTE_DATA);

		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
			i2c_write_conf_tbl(
			s_ctrl->sensor_i2c_client, gc0313_full_settings,
			ARRAY_SIZE(gc0313_full_settings),
			MSM_CAMERA_I2C_BYTE_DATA);
#else

		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
			i2c_write_conf_tbl(
			s_ctrl->sensor_i2c_client, gc0313_recommend_settings,
			ARRAY_SIZE(gc0313_recommend_settings),
			MSM_CAMERA_I2C_BYTE_DATA);
#endif
#if 0
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
			i2c_write_conf_tbl(
			s_ctrl->sensor_i2c_client,
			gc0313_config_change_settings,
			ARRAY_SIZE(gc0313_config_change_settings),
			MSM_CAMERA_I2C_BYTE_DATA);
#endif
		break;
	case CFG_SET_RESOLUTION:
		CDBG("=====> INcase: [CFG_SET_RESOLUTION]\n");
#if 0
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
			i2c_write_conf_tbl(
			s_ctrl->sensor_i2c_client, gc0313_720p_settings,
			ARRAY_SIZE(gc0313_720p_settings),
			MSM_CAMERA_I2C_BYTE_DATA);
#endif
		break;
	case CFG_SET_STOP_STREAM:
		CDBG("=====> INcase: [CFG_SET_STOP_STREAM]\n");
		break;
	case CFG_SET_START_STREAM:
		CDBG("=====> INcase: [CFG_SET_START_STREAM]\n");

#if 0
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write_conf_tbl(
		s_ctrl->sensor_i2c_client, gc0313_recommend_settings,
		ARRAY_SIZE(gc0313_recommend_settings),
		MSM_CAMERA_I2C_BYTE_DATA);
		//msleep(50);
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write_conf_tbl(
		s_ctrl->sensor_i2c_client, gc0313_full_settings,
		ARRAY_SIZE(gc0313_full_settings),
		MSM_CAMERA_I2C_BYTE_DATA);
#endif
#if 0
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
			i2c_write_conf_tbl(
			s_ctrl->sensor_i2c_client,
			gc0313_config_change_settings,
			ARRAY_SIZE(gc0313_config_change_settings),
			MSM_CAMERA_I2C_BYTE_DATA);
#endif
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
#if 1
		CDBG("=====> INcase: [CFG_POWER_DOWN]\n");
		if (s_ctrl->func_tbl->sensor_power_down)
			rc = s_ctrl->func_tbl->sensor_power_down(
				s_ctrl);
		else
			rc = -EFAULT;
#endif
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
	case CFG_SET_ANTIBANDING:
		CDBG("=====> INcase: [CFG_SET_ANTIBANDING]\n");
		break;
	case CFG_SET_EXPOSURE_COMPENSATION:
		CDBG("=====> INcase: [CFG_SET_EXPOSURE_COMPENSATION]\n");
		break;
	case CFG_SET_WHITE_BALANCE:
		CDBG("=====> INcase: [CFG_SET_WHITE_BALANCE]\n");
		break;
	case CFG_SET_EFFECT:
		CDBG("=====> INcase: [CFG_SET_EFFECT]\n");
		break;	
	case CFG_SET_BESTSHOT_MODE:
		CDBG("=====> INcase: [CFG_SET_BESTSHOT_MODE]\n");
		break;	
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

static struct msm_sensor_fn_t gc0313_sensor_func_tbl = {
	.sensor_config = gc0313_sensor_config,
	.sensor_power_up = msm_sensor_power_up,
	.sensor_power_down = msm_sensor_power_down,
	.sensor_match_id = msm_sensor_match_id,
};

static struct msm_sensor_ctrl_t gc0313_s_ctrl = {
	.sensor_i2c_client = &gc0313_sensor_i2c_client,
	.power_setting_array.power_setting = gc0313_power_setting,
	.power_setting_array.size = ARRAY_SIZE(gc0313_power_setting),
	.msm_sensor_mutex = &gc0313_mut,
	.sensor_v4l2_subdev_info = gc0313_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(gc0313_subdev_info),
	.func_tbl = &gc0313_sensor_func_tbl,
};

module_init(gc0313_init_module);
module_exit(gc0313_exit_module);
MODULE_DESCRIPTION("Gcore 0.3MP YUV sensor driver");
MODULE_LICENSE("GPL v2");

