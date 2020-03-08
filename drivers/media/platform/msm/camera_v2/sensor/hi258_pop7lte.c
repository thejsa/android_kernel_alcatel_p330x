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
 */
#define pr_fmt(fmt) "%s:%d,"fmt, __func__, __LINE__

#include <mach/gpiomux.h>
#include "msm_sensor.h"
#include "msm_cci.h"
#include "msm_camera_io_util.h"

#define HI258_SENSOR_NAME "hi258_pop7lte"

DEFINE_MSM_MUTEX(hi258_pop7lte_mut);

#ifdef debuglog
#undef debuglog
#endif
#define debuglog(fmt, args...) printk(KERN_DEBUG "%s:%d,"fmt, __func__, __LINE__, ##args)

static struct msm_sensor_ctrl_t hi258_s_ctrl;

static struct msm_sensor_power_setting hi258_power_setting[] = {
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
		.seq_val = CAM_VANA,
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
		.delay = 0,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_STANDBY,
		.config_val = GPIO_OUT_LOW,
		.delay = 10,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_HIGH,
		.delay = 1,
	},
};

static struct msm_camera_i2c_reg_conf hi258_video_settings[] = {
	// 800*600	
	{0x03, 0x00}, 
	{0x10, 0x91}, 
	{0xd2, 0x05},  //isp_div[1:0] mipi_4x_div[3:2]	mipi_1x_div[4] pll_bias_opt[7:5]	

	{0x20, 0x00}, //Row H
	{0x21, 0x04}, //Row L
	{0x22, 0x00}, //Col H
	{0x23, 0x08}, //Col L
	{0x03, 0x18}, 
	{0x10, 0x00}, 
	{0x14, 0x0f}, 

	{0x03, 0x20}, //Page 20
	{0x83, 0x02}, //EXP Normal 33.33 fps 
	{0x84, 0xf9}, 
	{0x85, 0xb8}, 
	{0x86, 0x01}, //EXPMin 13000.00 fps
	{0x87, 0xf4}, 
	{0x88, 0x06}, //EXP Max 60hz 15.00 fps 
	{0x89, 0x97}, 
	{0x8a, 0x80}, 
	{0xa5, 0x05}, //EXP Max 50hz 16.67 fps 
	{0xa6, 0xf3}, 
	{0xa7, 0x70}, 
	{0x8B, 0xfd}, //EXP100 
	{0x8C, 0xe8}, 
	{0x8D, 0xd2}, //EXP120 
	{0x8E, 0xf0}, 
	{0x9c, 0x21}, //EXP Limit 764.71 fps 
	{0x9d, 0x34}, 
	{0x9e, 0x01}, //EXP Unit 
	{0x9f, 0xf4}, 
	{0xa3, 0x00}, //Outdoor Int 
	{0xa4, 0xd2}, 

	{0x03, 0x48}, 
	{0x30, 0x06}, 
	{0x31, 0x40}, 
	{0x03, 0x18}, //scaling off 	
	{0x10, 0x00}, 
};

static struct msm_camera_i2c_reg_conf hi258_preview_settings[] = {
	// 1600*1200	
	{0x03, 0x00}, 
	{0x10, 0x00}, 
	{0xd2, 0x01},  //isp_div[1:0] mipi_4x_div[3:2]  mipi_1x_div[4] pll_bias_opt[7:5]    
	{0x20, 0x00}, //Row H
	{0x21, 0x0a}, //Row L
	{0x22, 0x00}, //Col H
	{0x23, 0x0a}, //Col L
	{0x03, 0x18}, 
	{0x10, 0x00}, 
	{0x14, 0x00}, 
	
	{0x03, 0x20}, //Page 20
	{0x83, 0x02}, //EXP Normal 33.33 fps 
	{0x84, 0xf9}, 
	{0x85, 0xb8}, 
	{0x86, 0x01}, //EXPMin 13000.00 fps
	{0x87, 0xf4}, 
	{0x88, 0x09}, //EXP Max 60hz 10.00 fps 
	{0x89, 0xe3}, 
	{0x8a, 0x40}, 
	{0xa5, 0x09}, //EXP Max 50hz 10.00 fps 
	{0xa6, 0xeb}, 
	{0xa7, 0x10}, 
	{0x8B, 0xfd}, //EXP100 
	{0x8C, 0xe8}, 
	{0x8D, 0xd2}, //EXP120 
	{0x8E, 0xf0}, 
	{0x9c, 0x21}, //EXP Limit 764.71 fps 
	{0x9d, 0x34}, 
	{0x9e, 0x01}, //EXP Unit 
	{0x9f, 0xf4}, 
	{0xa3, 0x00}, //Outdoor Int 
	{0xa4, 0xd2}, 

	{0x03, 0x48}, 
	{0x30, 0x0c}, 
	{0x31, 0x80}, 
};

static struct msm_camera_i2c_reg_conf hi258_snapshot_settings[] = {
	// 1600*1200	
	{0x03, 0x00}, 
	{0x10, 0x00}, 
	{0x03, 0x00}, 
	{0xd2, 0x01}, //isp_div[1:0] mipi_4x_div[3:2]  mipi_1x_div[4] pll_bias_opt[7:5]  

	{0x20, 0x00}, //Row H
	{0x21, 0x0a}, //Row L
	{0x22, 0x00}, //Col H
	{0x23, 0x0a}, //Col L
	{0x03, 0x18}, 
	{0x10, 0x00}, 
	{0x14, 0x00}, 

	{0x03, 0x48}, 
	{0x30, 0x0c}, 
	{0x31, 0x80}, 
};

static struct msm_camera_i2c_reg_conf hi258_start_settings[] = {
	{0x03, 0x00},
	{0x01, 0x00},
};

static struct msm_camera_i2c_reg_conf hi258_stop_settings[] = {
	{0x03, 0x00},
	{0x01, 0x01},
};

static struct msm_camera_i2c_reg_conf hi258_recommend_settings[] = {
	{0x01, 0x01}, //sleep on
	{0x01, 0x03}, //sleep off
	{0x01, 0x01}, //sleep on
	// PAGE 20
	{0x03, 0x20}, // page 20
	{0x10, 0x1c}, // AE off 60hz

	// PAGE 22
	{0x03, 0x22}, // page 22
	{0x10, 0x69}, // AWB off

	{0x03, 0x00}, //Dummy 750us
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},

	{0x08, 0x0f}, //131002
	{0x09, 0x07}, //131002 pad strength 77->07
	{0x0a, 0x00}, //131002 pad strength 07->00

	//PLL Setting
	{0x03, 0x00}, 
	{0xd0, 0x05}, //PLL pre_div 1/6 = 4 Mhz
	{0xd1, 0x34}, //PLL maim_div 
	{0xd2, 0x01}, //isp_div[1:0] mipi_4x_div[3:2]  mipi_1x_div[4] pll_bias_opt[7:5]  
	{0xd3, 0x20}, //isp_clk_inv[0]	mipi_4x_inv[1]	mipi_1x_inv[2]
	{0xd0, 0x85},
	{0xd0, 0x85},
	{0xd0, 0x85},
	{0xd0, 0x95},

	{0x03, 0x00}, //Dummy 750us
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},
	{0x03, 0x00},


	///// PAGE 20 /////
	{0x03, 0x20}, //page 20
	{0x10, 0x1c}, //AE off 50hz

	///// PAGE 22 /////
	{0x03, 0x22}, //page 22
	{0x10, 0x69}, //AWB off

	///// Initial Start /////
	///// PAGE 0 Start /////
	{0x03, 0x00}, //page 0
	{0x10, 0x00}, //pre1+sub1
	{0x11, 0x93}, //0x90}, //Windowing On + 1Frame Skip
	{0x12, 0x04}, //Rinsing edge 0x04 // Falling edge 0x00
	{0x14, 0x05},

	{0x20, 0x00}, //Row H
	{0x21, 0x0a}, //0x02}, //Row L
	{0x22, 0x00}, //Col H
	{0x23, 0x0a}, //0x04}, //Col L

	{0x24, 0x04}, //Window height_H //= 1200
	{0x25, 0xb0}, //Window height_L //
	{0x26, 0x06}, //Window width_H	//= 1600
	{0x27, 0x40}, //Window wight_L

	{0x28, 0x04}, //Full row start y-flip 
	{0x29, 0x01}, //Pre1 row start no-flip
	{0x2a, 0x02}, //Pre1 row start y-flip 

	{0x2b, 0x04}, //Full wid start x-flip 
	{0x2c, 0x04}, //Pre2 wid start no-flip
	{0x2d, 0x02}, //Pre2 wid start x-flip 

	{0x40, 0x01}, //Hblank 376
	{0x41, 0x78}, 
	{0x42, 0x00}, //Vblank 20
	{0x43, 0x14}, 
	{0x50, 0x00}, //Test Pattern

	///// BLC /////
	{0x80, 0x2e},
	{0x81, 0x7e},
	{0x82, 0x90},
	{0x83, 0x00},
	{0x84, 0xcc}, //20130604 0x0c->0xcc
	{0x85, 0x00},
	{0x86, 0x00},
	{0x87, 0x0f},
	{0x88, 0x34},
	{0x8a, 0x0b},
	{0x8e, 0x80}, //Pga Blc Hold

	{0x90, 0x0a}, //BLC_TIME_TH_ON
	{0x91, 0x0a}, //BLC_TIME_TH_OFF
	{0x92, 0x78}, //BLC_AG_TH_ON
	{0x93, 0x70}, //BLC_AG_TH_OFF
	{0x96, 0xdc}, //BLC Outdoor Th On
	{0x97, 0xfe}, //BLC Outdoor Th Off
	{0x98, 0x38},

	//OutDoor  BLC
	{0x99, 0x43}, //R,Gr,B,Gb Offset

	//Dark BLC
	{0xa0, 0x02}, //R,Gr,B,Gb Offset

	//Normal BLC
	{0xa8, 0x00}, //R,Gr,B,Gb Offset
	///// PAGE 0 END /////

	///// PAGE 2 START /////
	{0x03, 0x02},
	{0x10, 0x00},
	{0x13, 0x00},
	{0x14, 0x00},
	{0x18, 0xcc},
	{0x19, 0x01}, // pmos switch on (for cfpn)
	{0x1A, 0x39}, //20130604 0x09->0xcc
	{0x1B, 0x00},
	{0x1C, 0x1a}, // for ncp
	{0x1D, 0x14}, // for ncp
	{0x1E, 0x30}, // for ncp
	{0x1F, 0x10},

	{0x20, 0x77},
	{0x21, 0xde},
	{0x22, 0xa7},
	{0x23, 0x30},
	{0x24, 0x77},
	{0x25, 0x10},
	{0x26, 0x10},
	{0x27, 0x3c},
	{0x2b, 0x80},
	{0x2c, 0x02},
	{0x2d, 0x58},
	{0x2e, 0x11},//20130604 0xde->0x11
	{0x2f, 0x11},//20130604 0xa7->0x11

	{0x30, 0x00},
	{0x31, 0x99},
	{0x32, 0x00},
	{0x33, 0x00},
	{0x34, 0x22},
	{0x36, 0x75},
	{0x38, 0x88},
	{0x39, 0x88},
	{0x3d, 0x03},
	{0x3f, 0x02},

	{0x49, 0xc1},//20130604 0x87->0xd1 --> mode Change Issue modify -> 0xc1 
	{0x4a, 0x10},

	{0x50, 0x21},
	{0x53, 0xb1},
	{0x54, 0x10},
	{0x55, 0x1c}, // for ncp
	{0x56, 0x11},
	{0x58, 0x3a},//20130604 add
	{0x59, 0x38},//20130604 add
	{0x5d, 0xa2},
	{0x5e, 0x5a},

	{0x60, 0x87},
	{0x61, 0x98},
	{0x62, 0x88},
	{0x63, 0x96},
	{0x64, 0x88},
	{0x65, 0x96},
	{0x67, 0x3f},
	{0x68, 0x3f},
	{0x69, 0x3f},

	{0x72, 0x89},
	{0x73, 0x95},
	{0x74, 0x89},
	{0x75, 0x95},
	{0x7C, 0x84},
	{0x7D, 0xaf},

	{0x80, 0x01},
	{0x81, 0x7a},
	{0x82, 0x13},
	{0x83, 0x24},
	{0x84, 0x78},
	{0x85, 0x7c},

	{0x92, 0x44},
	{0x93, 0x59},
	{0x94, 0x78},
	{0x95, 0x7c},

	{0xA0, 0x02},
	{0xA1, 0x74},
	{0xA4, 0x74},
	{0xA5, 0x02},
	{0xA8, 0x85},
	{0xA9, 0x8c},
	{0xAC, 0x10},
	{0xAD, 0x16},

	{0xB0, 0x99},
	{0xB1, 0xa3},
	{0xB4, 0x9b},
	{0xB5, 0xa2},
	{0xB8, 0x9b},
	{0xB9, 0x9f},
	{0xBC, 0x9b},
	{0xBD, 0x9f},

	{0xc4, 0x29},
	{0xc5, 0x40},
	{0xc6, 0x5c},
	{0xc7, 0x72},
	{0xc8, 0x2a},
	{0xc9, 0x3f},
	{0xcc, 0x5d},
	{0xcd, 0x71},

	{0xd0, 0x10},
	{0xd1, 0x14},
	{0xd2, 0x20},
	{0xd3, 0x00},
	{0xd4, 0x0a}, //DCDC_TIME_TH_ON
	{0xd5, 0x0a}, //DCDC_TIME_TH_OFF 
	{0xd6, 0x78}, //DCDC_AG_TH_ON
	{0xd7, 0x70}, //DCDC_AG_TH_OFF
	{0xdc, 0x00},
	{0xdd, 0xa3},
	{0xde, 0x00},
	{0xdf, 0x84},

	{0xe0, 0xa4},
	{0xe1, 0xa4},
	{0xe2, 0xa4},
	{0xe3, 0xa4},
	{0xe4, 0xa4},
	{0xe5, 0x01},
	{0xe8, 0x00},
	{0xe9, 0x00},
	{0xea, 0x77},

	{0xF0, 0x00},
	{0xF1, 0x00},
	{0xF2, 0x00},

	///// PAGE 2 END /////


	///// PAGE 10 START /////
	{0x03, 0x10}, //page 10
	{0x10, 0x03}, //S2D enable _ YUYV Order 
	{0x11, 0x03},
	{0x12, 0x30},
	{0x13, 0x03},

	{0x20, 0x00},
	{0x21, 0x40},
	{0x22, 0x0f},
	{0x24, 0x20},
	{0x25, 0x10},
	{0x26, 0x01},
	{0x27, 0x02},
	{0x28, 0x11},

	{0x40, 0x00},
	{0x41, 0x00}, //D-YOffset Th
	{0x42, 0x04}, //Cb Offset
	{0x43, 0x04}, //Cr Offset
	{0x44, 0x80},
	{0x45, 0x80},
	{0x46, 0xf0},
	{0x48, 0x90},
	{0x4a, 0x80},

	{0x50, 0xa0}, //D-YOffset AG

	{0x60, 0x0f}, //Outdoor Saturation
	{0x61, 0x88}, //Sat B
	{0x62, 0x8b}, //Sat R
	{0x63, 0x60}, //Auto-De Color

	{0x66, 0x42},
	{0x67, 0x22},

	{0x6a, 0x58}, //White Protection Offset Dark/Indoor
	{0x74, 0x0c}, //White Protection Offset Outdoor
	{0x76, 0x01}, //White Protection Enable
	///// PAGE 10 END /////

	///// PAGE 11 START /////
	{0x03, 0x11}, //page 11

	//LPF Auto Control
	{0x20, 0x00},
	{0x21, 0x00},
	{0x26, 0x5a}, // Double_AG
	{0x27, 0x58}, // Double_AG
	{0x28, 0x0f},
	{0x29, 0x10},
	{0x2b, 0x30},
	{0x2c, 0x32},

	//GBGR 
	{0x70, 0x2b},
	{0x74, 0x30},
	{0x75, 0x18},
	{0x76, 0x30},
	{0x77, 0xff},
	{0x78, 0xa0},
	{0x79, 0xff}, //Dark GbGr Th
	{0x7a, 0x30},
	{0x7b, 0x20},
	{0x7c, 0xf4}, //Dark Dy Th B[7:4]
	{0x7d, 0x02},
	{0x7e, 0xb0},
	{0x7f, 0x10},
	///// PAGE 11 END /////

	///// PAGE 12 START /////
	{0x03, 0x12}, //page 11

	//YC2D
	{0x10, 0x03}, //Y DPC Enable
	{0x11, 0x08}, //
	{0x12, 0x10}, //0x30 -> 0x10
	{0x20, 0x53}, //Y_lpf_enable
	{0x21, 0x03}, //C_lpf_enable_on
	{0x22, 0xe6}, //YC2D_CrCbY_Dy

	{0x23, 0x1b}, //Outdoor Dy Th
	{0x24, 0x1a}, //Indoor Dy Th // For reso Limit 0x20
	{0x25, 0x28}, //Dark Dy Th

	//Outdoor LPF Flat
	{0x30, 0xff}, //Y Hi Th
	{0x31, 0x00}, //Y Lo Th
	{0x32, 0xf0}, //Std Hi Th //Reso Improve Th Low //50
	{0x33, 0x00}, //Std Lo Th
	{0x34, 0x00}, //Median ratio

	//Indoor LPF Flat
	{0x35, 0xff}, //Y Hi Th
	{0x36, 0x00}, //Y Lo Th
	{0x37, 0xff}, //Std Hi Th //Reso Improve Th Low //50
	{0x38, 0x00}, //Std Lo Th
	{0x39, 0x00}, //Median ratio

	//Dark LPF Flat
	{0x3a, 0xff}, //Y Hi Th
	{0x3b, 0x00}, //Y Lo Th
	{0x3c, 0x93}, //Std Hi Th //Reso Improve Th Low //50
	{0x3d, 0x00}, //Std Lo Th
	{0x3e, 0x00}, //Median ratio

	//Outdoor Cindition
	{0x46, 0xa0}, //Out Lum Hi
	{0x47, 0x40}, //Out Lum Lo

	//Indoor Cindition
	{0x4c, 0xb0}, //Indoor Lum Hi
	{0x4d, 0x40}, //Indoor Lum Lo

	//Dark Cindition
	{0x52, 0xb0}, //Dark Lum Hi
	{0x53, 0x50}, //Dark Lum Lo

	//C-Filter
	{0x70, 0x10}, //Outdoor(2:1) AWM Th Horizontal
	{0x71, 0x0a}, //Outdoor(2:1) Diff Th Vertical
	{0x72, 0x10}, //Indoor,Dark1 AWM Th Horizontal
	{0x73, 0x0a}, //Indoor,Dark1 Diff Th Vertical
	{0x74, 0x14}, //Dark(2:3) AWM Th Horizontal
	{0x75, 0x0c}, //Dark(2:3) Diff Th Vertical

	//DPC
	{0x90, 0x7d},
	{0x91, 0x34},
	{0x99, 0x28},
	{0x9c, 0x14},
	{0x9d, 0x15},
	{0x9e, 0x28},
	{0x9f, 0x28},
	{0xb0, 0x0e}, //Zipper noise Detault change (0x75->0x0e)
	{0xb8, 0x44},
	{0xb9, 0x15},
	///// PAGE 12 END /////

	///// PAGE 13 START /////
	{0x03, 0x13}, //page 13

	{0x80, 0xfd}, //Sharp2D enable _ YUYV Order
	{0x81, 0x07}, //Sharp2D Clip/Limit
	{0x82, 0x73}, //Sharp2D Filter
	{0x83, 0x00}, //Sharp2D Low Clip
	{0x85, 0x00},

	{0x92, 0x33}, //Sharp2D Slop n/p
	{0x93, 0x30}, //Sharp2D LClip
	{0x94, 0x02}, //Sharp2D HiClip1 Th
	{0x95, 0xf0}, //Sharp2D HiClip2 Th
	{0x96, 0x1e}, //Sharp2D HiClip2 Resolution
	{0x97, 0x40}, 
	{0x98, 0x80},
	{0x99, 0x40},

	//Sharp Lclp
	{0xa2, 0x04}, //Outdoor Lclip_N
	{0xa3, 0x05}, //Outdoor Lclip_P
	{0xa4, 0x04}, //Indoor Lclip_N 0x03 For reso Limit 0x0e
	{0xa5, 0x05}, //Indoor Lclip_P 0x0f For reso Limit 0x0f
	{0xa6, 0x80}, //Dark Lclip_N
	{0xa7, 0x80}, //Dark Lclip_P

	//Outdoor Slope
	{0xb6, 0x28}, //Lum negative Hi
	{0xb7, 0x25}, //Lum negative middle
	{0xb8, 0x24}, //Lum negative Low
	{0xb9, 0x28}, //Lum postive Hi
	{0xba, 0x25}, //Lum postive middle
	{0xbb, 0x24}, //Lum postive Low

	//Indoor Slope
	{0xbc, 0x08}, //Lum negative Hi
	{0xbd, 0x20}, //Lum negative middle
	{0xbe, 0x14}, //Lum negative Low
	{0xbf, 0x08}, //Lum postive Hi
	{0xc0, 0x20}, //Lum postive middle
	{0xc1, 0x04}, //Lum postive Low

	//Dark Slope
	{0xc2, 0x08}, //Lum negative Hi
	{0xc3, 0x10}, //Lum negative middle
	{0xc4, 0x08}, //Lum negative Low
	{0xc5, 0x08}, //Lum postive Hi
	{0xc6, 0x10}, //Lum postive middle
	{0xc7, 0x08}, //Lum postive Low
	///// PAGE 13 END /////

	///// PAGE 14 START /////
	{0x03, 0x14}, //page 14
	{0x10, 0x09},
	{0x11, 0x00},
	{0x20, 0x64},
	{0x21, 0xa0},
	{0x22, 0x69},//80
	{0x23, 0x55},//6c
	{0x24, 0x51},//6e
	{0x25, 0xf0},
	{0x26, 0xf0},
	///// PAGE 14 END /////

	/////// PAGE 15 START ///////
	{0x03, 0x15}, //page 15
	{0x10, 0x0f},
	{0x14, 0x52},
	{0x15, 0x42},
	{0x16, 0x32},
	{0x17, 0x2f},

	//CMC
	{0x30, 0x8e},
	{0x31, 0x75},
	{0x32, 0x25},
	{0x33, 0x15},
	{0x34, 0x5a},
	{0x35, 0x05},
	{0x36, 0x07},
	{0x37, 0x40},
	{0x38, 0x85},

	//CMC OFS
	{0x40, 0x95},
	{0x41, 0x1f},
	{0x42, 0x8a},
	{0x43, 0x86},
	{0x44, 0x0a},
	{0x45, 0x84},
	{0x46, 0x87},
	{0x47, 0x9b},
	{0x48, 0x23},

	//CMC POFS
	{0x50, 0x8c},
	{0x51, 0x0c},
	{0x52, 0x00},
	{0x53, 0x07},
	{0x54, 0x17},
	{0x55, 0x9d},
	{0x56, 0x00},
	{0x57, 0x0b},
	{0x58, 0x89},
	///// PAGE 15 END /////

	///// PAGE 16 START /////
	{0x03, 0x16}, //page 16
	{0x10, 0x31},
	{0x18, 0x37},// Double_AG 5e->37
	{0x19, 0x36},// Double_AG 5e->36
	{0x1a, 0x0e},
	{0x1b, 0x01},
	{0x1c, 0xdc},
	{0x1d, 0xfe},

	//Indoor
	{0x30, 0x00},
	{0x31, 0x09},
	{0x32, 0x19},//15
	{0x33, 0x26},//33
	{0x34, 0x53},
	{0x35, 0x6c},
	{0x36, 0x81},
	{0x37, 0x94},
	{0x38, 0xa4},
	{0x39, 0xb3},
	{0x3a, 0xc0},
	{0x3b, 0xcb},
	{0x3c, 0xd5},
	{0x3d, 0xde},
	{0x3e, 0xe6},
	{0x3f, 0xee},
	{0x40, 0xf3},
	{0x41, 0xf8},
	{0x42, 0xff},


	//Outdoor
	{0x50, 0x00},
	{0x51, 0x05},
	{0x52, 0x1b},
	{0x53, 0x36},
	{0x54, 0x5a},
	{0x55, 0x75},
	{0x56, 0x89},
	{0x57, 0x9c},
	{0x58, 0xac},
	{0x59, 0xb8},
	{0x5a, 0xc7},
	{0x5b, 0xd2},
	{0x5c, 0xdc},
	{0x5d, 0xe5},
	{0x5e, 0xed},
	{0x5f, 0xf2},
	{0x60, 0xf7},
	{0x61, 0xf9},
	{0x62, 0xfa},

	//Dark
	{0x70, 0x1e},
	{0x71, 0x1e},
	{0x72, 0x20},
	{0x73, 0x2e},
	{0x74, 0x55},
	{0x75, 0x6e},
	{0x76, 0x83},
	{0x77, 0x96},
	{0x78, 0xa6},
	{0x79, 0xb5},
	{0x7a, 0xc2},
	{0x7b, 0xcd},
	{0x7c, 0xd7},
	{0x7d, 0xe0},
	{0x7e, 0xe8},
	{0x7f, 0xf0},
	{0x80, 0xf6},
	{0x81, 0xfa},
	{0x82, 0xff},
	///// PAGE 16 END /////

	///// PAGE 17 START /////
	{0x03, 0x17}, //page 17
	{0xc1, 0x00},
	{0xc4, 0x4b},
	{0xc5, 0x3f},
	{0xc6, 0x02},
	{0xc7, 0x20},
	///// PAGE 17 END /////

	///// PAGE 18 START /////
	{0x03, 0x18}, //page 18
	{0x10, 0x00},	//Scale Off

	///// PAGE 18 END /////

	{0x03, 0x19}, //Page 0x19
	{0x10, 0x7f}, //mcmc_ctl1
	{0x11, 0x7f}, //mcmc_ctl2
	{0x12, 0x1e}, //mcmc_delta1
	{0x13, 0x32}, //mcmc_center1
	{0x14, 0x1e}, //mcmc_delta2
	{0x15, 0x6e}, //mcmc_center2
	{0x16, 0x0a}, //mcmc_delta3
	{0x17, 0xb8}, //mcmc_center3
	{0x18, 0x1e}, //mcmc_delta4
	{0x19, 0xe6}, //mcmc_center4
	{0x1a, 0x9e}, //mcmc_delta5
	{0x1b, 0x22}, //mcmc_center5
	{0x1c, 0x9e}, //mcmc_delta6
	{0x1d, 0x5e}, //mcmc_center6
	{0x1e, 0x3b}, //mcmc_sat_gain1
	{0x1f, 0x3e}, //mcmc_sat_gain2
	{0x20, 0x4b}, //mcmc_sat_gain3
	{0x21, 0x52}, //mcmc_sat_gain4
	{0x22, 0x59}, //mcmc_sat_gain5
	{0x23, 0x46}, //mcmc_sat_gain6
	{0x24, 0x00}, //mcmc_hue_angle1
	{0x25, 0x01}, //mcmc_hue_angle2
	{0x26, 0x0e}, //mcmc_hue_angle3
	{0x27, 0x04}, //mcmc_hue_angle4
	{0x28, 0x00}, //mcmc_hue_angle5
	{0x29, 0x8c}, //mcmc_hue_angle6
	{0x53, 0x10}, //mcmc_ctl3
	{0x6c, 0xff}, //mcmc_lum_ctl1
	{0x6d, 0x3f}, //mcmc_lum_ctl2
	{0x6e, 0x00}, //mcmc_lum_ctl3
	{0x6f, 0x00}, //mcmc_lum_ctl4
	{0x70, 0x00}, //mcmc_lum_ctl5
	{0x71, 0x3f}, //rg1_lum_gain_wgt_th1
	{0x72, 0x3f}, //rg1_lum_gain_wgt_th2
	{0x73, 0x3f}, //rg1_lum_gain_wgt_th3
	{0x74, 0x3f}, //rg1_lum_gain_wgt_th4
	{0x75, 0x30}, //rg1_lum_sp1
	{0x76, 0x50}, //rg1_lum_sp2
	{0x77, 0x80}, //rg1_lum_sp3
	{0x78, 0xb0}, //rg1_lum_sp4
	{0x79, 0x3f}, //rg2_gain_wgt_th1
	{0x7a, 0x3f}, //rg2_gain_wgt_th2
	{0x7b, 0x3f}, //rg2_gain_wgt_th3
	{0x7c, 0x3f}, //rg2_gain_wgt_th4
	{0x7d, 0x28}, //rg2_lum_sp1
	{0x7e, 0x50}, //rg2_lum_sp2
	{0x7f, 0x80}, //rg2_lum_sp3
	{0x80, 0xb0}, //rg2_lum_sp4
	{0x81, 0x28}, //rg3_gain_wgt_th1
	{0x82, 0x3f}, //rg3_gain_wgt_th2
	{0x83, 0x3f}, //rg3_gain_wgt_th3
	{0x84, 0x3f}, //rg3_gain_wgt_th4
	{0x85, 0x28}, //rg3_lum_sp1
	{0x86, 0x50}, //rg3_lum_sp2
	{0x87, 0x80}, //rg3_lum_sp3
	{0x88, 0xb0}, //rg3_lum_sp4
	{0x89, 0x1a}, //rg4_gain_wgt_th1
	{0x8a, 0x28}, //rg4_gain_wgt_th2
	{0x8b, 0x3f}, //rg4_gain_wgt_th3
	{0x8c, 0x3f}, //rg4_gain_wgt_th4
	{0x8d, 0x10}, //rg4_lum_sp1
	{0x8e, 0x30}, //rg4_lum_sp2
	{0x8f, 0x60}, //rg4_lum_sp3
	{0x90, 0x90}, //rg4_lum_sp4
	{0x91, 0x1a}, //rg5_gain_wgt_th1
	{0x92, 0x28}, //rg5_gain_wgt_th2
	{0x93, 0x3f}, //rg5_gain_wgt_th3
	{0x94, 0x3f}, //rg5_gain_wgt_th4
	{0x95, 0x28}, //rg5_lum_sp1
	{0x96, 0x50}, //rg5_lum_sp2
	{0x97, 0x80}, //rg5_lum_sp3
	{0x98, 0xb0}, //rg5_lum_sp4
	{0x99, 0x1a}, //rg6_gain_wgt_th1
	{0x9a, 0x28}, //rg6_gain_wgt_th2
	{0x9b, 0x3f}, //rg6_gain_wgt_th3
	{0x9c, 0x3f}, //rg6_gain_wgt_th4
	{0x9d, 0x28}, //rg6_lum_sp1
	{0x9e, 0x50}, //rg6_lum_sp2
	{0x9f, 0x80}, //rg6_lum_sp3
	{0xa0, 0xb0}, //rg6_lum_sp4
	{0xa1, 0x00}, //mcmc_allgain_ctl

	{0xa2, 0x00},
	{0xe5, 0x80}, //add 20120709 Bit[7] On MCMC --> YC2D_LPF

	/////// PAGE 20 START ///////
	{0x03, 0x20},
	{0x10, 0x1c},
	{0x11, 0x14},
	{0x18, 0x30},
	{0x20, 0x25}, //8x8 Ae weight 0~7 Outdoor / Weight Outdoor On B[5]
	{0x21, 0x30},
	{0x22, 0x10},
	{0x23, 0x00},

	{0x28, 0xf7},
	{0x29, 0x0d},
	{0x2a, 0xff},
	{0x2b, 0x04}, //Adaptive Off,1/100 Flicker

	{0x2c, 0x83}, //AE After CI
	{0x2d, 0x03},
	{0x2e, 0x13},
	{0x2f, 0x0b},

	{0x30, 0x78},
	{0x31, 0xd7},
	{0x32, 0x10},
	{0x33, 0x2e},
	{0x34, 0x20},
	{0x35, 0xd4},
	{0x36, 0xfe},
	{0x37, 0x32},
	{0x38, 0x04},
	{0x39, 0x22},
	{0x3a, 0xde},
	{0x3b, 0x22},
	{0x3c, 0xde},
	{0x3d, 0xe1},

	{0x3e, 0xc9}, //Option of changing Exp max
	{0x41, 0x23}, //Option of changing Exp max

	{0x50, 0x45},
	{0x51, 0x88},

	{0x56, 0x1f}, // for tracking
	{0x57, 0xa6}, // for tracking
	{0x58, 0x1a}, // for tracking
	{0x59, 0x7a}, // for tracking

	{0x5a, 0x04},
	{0x5b, 0x04},

	{0x5e, 0xc7},
	{0x5f, 0x95},

	{0x62, 0x10},
	{0x63, 0xc0},
	{0x64, 0x10},
	{0x65, 0x8a},
	{0x66, 0x58},
	{0x67, 0x58},

	{0x70, 0x50}, //6c
	{0x71, 0x81}, //81(+4),89(-4)

	{0x76, 0x32},
	{0x77, 0xa1},
	{0x78, 0x22}, //24
	{0x79, 0x30}, // Y Target 70 => 25, 72 => 26 //
	{0x7a, 0x23}, //23
	{0x7b, 0x22}, //22
	{0x7d, 0x23},

	{0x83, 0x02}, //EXP Normal 33.33 fps 
	{0x84, 0xf9}, 
	{0x85, 0xb8}, 
	{0x86, 0x01}, //EXPMin 13000.00 fps
	{0x87, 0xf4}, 
	{0x88, 0x09}, //EXP Max 60hz 10.00 fps 
	{0x89, 0xe3}, 
	{0x8a, 0x40}, 
	{0xa5, 0x09}, //EXP Max 50hz 10.00 fps 
	{0xa6, 0xeb}, 
	{0xa7, 0x10}, 
	{0x8B, 0xfd}, //EXP100 
	{0x8C, 0xe8}, 
	{0x8D, 0xd2}, //EXP120 
	{0x8E, 0xf0}, 
	{0x9c, 0x21}, //EXP Limit 764.71 fps 
	{0x9d, 0x34}, 
	{0x9e, 0x01}, //EXP Unit 
	{0x9f, 0xf4}, 
	{0xa3, 0x00}, //Outdoor Int 
	{0xa4, 0xd2}, 	
	
	{0xb0, 0x15},
	{0xb1, 0x14},
	{0xb2, 0x80},
	{0xb3, 0x15},
	{0xb4, 0x16},
	{0xb5, 0x3c},
	{0xb6, 0x29},
	{0xb7, 0x23},
	{0xb8, 0x20},
	{0xb9, 0x1e},
	{0xba, 0x1c},
	{0xbb, 0x1b},
	{0xbc, 0x1b},
	{0xbd, 0x1a},

	{0xc0, 0x10},
	{0xc1, 0x40},
	{0xc2, 0x40},
	{0xc3, 0x40},
	{0xc4, 0x06},

	{0xc6, 0x80}, //Exp max 1frame target AG

	{0xc8, 0x80},
	{0xc9, 0x80},
	///// PAGE 20 END /////

	///// PAGE 21 START /////
	{0x03, 0x21}, //page 21

	//Indoor Weight
	{0x20, 0x11},
	{0x21, 0x11},
	{0x22, 0x11},
	{0x23, 0x11},
	{0x24, 0x12},
	{0x25, 0x22},
	{0x26, 0x22},
	{0x27, 0x21},
	{0x28, 0x12},
	{0x29, 0x55},
	{0x2a, 0x55},
	{0x2b, 0x21},
	{0x2c, 0x12},
	{0x2d, 0x57},
	{0x2e, 0x75},
	{0x2f, 0x21},
	{0x30, 0x12},
	{0x31, 0x57},
	{0x32, 0x75},
	{0x33, 0x21},
	{0x34, 0x12},
	{0x35, 0x55},
	{0x36, 0x55},
	{0x37, 0x21},
	{0x38, 0x12},
	{0x39, 0x22},
	{0x3a, 0x22},
	{0x3b, 0x21},
	{0x3c, 0x11},
	{0x3d, 0x11},
	{0x3e, 0x11},
	{0x3f, 0x11},


	//Outdoor Weight
	{0x40, 0x11},
	{0x41, 0x11},
	{0x42, 0x11},
	{0x43, 0x11},
	{0x44, 0x14},
	{0x45, 0x44},
	{0x46, 0x44},
	{0x47, 0x41},
	{0x48, 0x14},
	{0x49, 0x44},
	{0x4a, 0x44},
	{0x4b, 0x41},
	{0x4c, 0x14},
	{0x4d, 0x47},
	{0x4e, 0x74},
	{0x4f, 0x41},
	{0x50, 0x14},
	{0x51, 0x47},
	{0x52, 0x74},
	{0x53, 0x41},
	{0x54, 0x14},
	{0x55, 0x44},
	{0x56, 0x44},
	{0x57, 0x41},
	{0x58, 0x14},
	{0x59, 0x44},
	{0x5a, 0x44},
	{0x5b, 0x41},
	{0x5c, 0x11},
	{0x5d, 0x11},
	{0x5e, 0x11},
	{0x5f, 0x11},


	///// PAGE 22 START /////
	{0x03, 0x22}, //page 22
	{0x10, 0xfd},
	{0x11, 0x2e},
	{0x19, 0x00},
	{0x20, 0x30}, //For AWB Speed
	{0x21, 0x80},
	{0x22, 0x00},
	{0x23, 0x00},
	{0x24, 0x01},
	{0x25, 0x7e}, //for Tracking setting

	{0x30, 0x80},
	{0x31, 0x80},
	{0x38, 0x11},
	{0x39, 0x34},
	{0x40, 0xe4}, //Stb Yth
	{0x41, 0x33}, //Stb cdiff
	{0x42, 0x22}, //Stb csum
	{0x43, 0xf3}, //Unstb Yth
	{0x44, 0x55}, //Unstb cdiff
	{0x45, 0x33}, //Unstb csum
	{0x46, 0x00},
	{0x47, 0xa2},
	{0x48, 0x02},
	{0x49, 0x0a},

	{0x60, 0x04},
	{0x61, 0xc4},
	{0x62, 0x04},
	{0x63, 0x92},
	{0x66, 0x04},
	{0x67, 0xc4},
	{0x68, 0x04},
	{0x69, 0x92},

	{0x80, 0x38},
	{0x81, 0x20},
	{0x82, 0x38},

	{0x83, 0x58},
	{0x84, 0x1a},//21
	{0x85, 0x4f},
	{0x86, 0x1a},

	{0x87, 0x42},//4d
	{0x88, 0x30},
	{0x89, 0x27},
	{0x8a, 0x18},

	{0x8b, 0x3d},
	{0x8c, 0x32},
	{0x8d, 0x24},
	{0x8e, 0x1d},

	{0x8f, 0x4d}, //4e
	{0x90, 0x46}, //4d
	{0x91, 0x40}, //4c
	{0x92, 0x3a}, //4a
	{0x93, 0x2f}, //46
	{0x94, 0x21},
	{0x95, 0x19},
	{0x96, 0x16},
	{0x97, 0x13},
	{0x98, 0x12},
	{0x99, 0x11},
	{0x9a, 0x10},

	{0x9b, 0x88},
	{0x9c, 0x88},
	{0x9d, 0x48},
	{0x9e, 0x38},
	{0x9f, 0x30},

	{0xa0, 0x70},
	{0xa1, 0x54},
	{0xa2, 0x6f},
	{0xa3, 0xff},

	{0xa4, 0x14}, //1536fps
	{0xa5, 0x2c}, //698fps
	{0xa6, 0xcf}, //148fps

	{0xad, 0x40},
	{0xae, 0x4a},

	{0xaf, 0x28}, //Low temp Rgain
	{0xb0, 0x26}, //Low temp Rgain

	{0xb1, 0x00},
	{0xb4, 0xbf}, //For Tracking AWB Weight
	{0xb8, 0x91}, //(0+,1-)High Cb , (0+,1-)Low Cr
	{0xb9, 0x00},
	/////// PAGE 22 END ///////

	//// MIPI Setting /////
	{0x03, 0x48},
	{0x39, 0x4f}, //lvds_bias_ctl	 [2:0]mipi_tx_bias	 [4:3]mipi_vlp_sel	 [6:5]mipi_vcm_sel
	{0x10, 0x1c}, //lvds_ctl_1		 [5]mipi_pad_disable [4]lvds_en [0]serial_data_len 
	{0x11, 0x10}, //lvds_ctl_2		 [4]mipi continous mode setting
	//{0x14, 0x00} //ser_out_ctl_1	[2:0]serial_sout_a_phase   [6:4]serial_cout_a_phase

	{0x16, 0x00}, //lvds_inout_ctl1  [0]vs_packet_pos_sel [1]data_neg_sel [4]first_vsync_end_opt
	{0x18, 0x80}, //lvds_inout_ctl3
	{0x19, 0x00}, //lvds_inout_ctl4
	{0x1a, 0xf0}, //lvds_time_ctl
	{0x24, 0x1e}, //long_packet_id

	//====== MIPI Timing Setting =========
	{0x36, 0x01}, //clk_tlpx_time_dp
	{0x37, 0x05}, //clk_tlpx_time_dn
	{0x34, 0x04}, //clk_prepare_time
	{0x32, 0x15}, //clk_zero_time
	{0x35, 0x04}, //clk_trail_time
	{0x33, 0x0d}, //clk_post_time

	{0x1c, 0x01}, //tlps_time_l_dp
	{0x1d, 0x0b}, //tlps_time_l_dn
	{0x1e, 0x06}, //hs_zero_time
	{0x1f, 0x09}, //hs_trail_time

	//long_packet word count 
	{0x30, 0x0c},
	{0x31, 0x80}, //long_packet word count

	/////// PAGE 20 ///////
	{0x03, 0x20},
	{0x10, 0x9c}, //AE On 50hz

	/////// PAGE 22 ///////
	{0x03, 0x22},
	{0x10, 0xe9}, //AWB On

	{0x03, 0x00},
	{0x01, 0x00},
};

static struct v4l2_subdev_info hi258_subdev_info[] = {
	{
		.code   = V4L2_MBUS_FMT_YUYV8_2X8,
		.colorspace = V4L2_COLORSPACE_JPEG,
		.fmt	= 1,
		.order	= 0,
	},
};


static const struct i2c_device_id hi258_i2c_id[] = {
	{HI258_SENSOR_NAME, (kernel_ulong_t)&hi258_s_ctrl},
	{ }
};

static int32_t msm_hi258_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	return msm_sensor_i2c_probe(client, id, &hi258_s_ctrl);
}

static struct i2c_driver hi258_i2c_driver = {
	.id_table = hi258_i2c_id,
	.probe  = msm_hi258_i2c_probe,
	.driver = {
		.name = HI258_SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client hi258_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
};

static const struct of_device_id hi258_dt_match[] = {
	{.compatible = "qcom,hi258_pop7lte", .data = &hi258_s_ctrl},
	{}
};

MODULE_DEVICE_TABLE(of, hi258_dt_match);

static struct platform_driver hi258_platform_driver = {
	.driver = {
		.name = "qcom,hi258_pop7lte",
		.owner = THIS_MODULE,
		.of_match_table = hi258_dt_match,
	},
};

static int32_t hi258_platform_probe(struct platform_device *pdev)
{
	int32_t rc;
	const struct of_device_id *match;

	match = of_match_device(hi258_dt_match, &pdev->dev);
	rc = msm_sensor_platform_probe(pdev, match->data);
	return rc;
}

static int __init hi258_init_module(void)
{
	int32_t rc;

	debuglog("enter\n");
	rc = platform_driver_probe(&hi258_platform_driver,
		hi258_platform_probe);
	if (!rc)
		return rc;
	debuglog("platform probe fail, then i2c probe\n");
	return i2c_add_driver(&hi258_i2c_driver);
}

static void __exit hi258_exit_module(void)
{
	if (hi258_s_ctrl.pdev) {
		msm_sensor_free_sensor_data(&hi258_s_ctrl);
		platform_driver_unregister(&hi258_platform_driver);
	} else
		i2c_del_driver(&hi258_i2c_driver);
	return;
}

static int32_t hi258_sensor_match_id(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	uint16_t chipid = 0;
	
	rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_read(
			s_ctrl->sensor_i2c_client,
			s_ctrl->sensordata->slave_info->sensor_id_reg_addr,
			&chipid, MSM_CAMERA_I2C_BYTE_DATA);
	if (rc < 0) {
		pr_err("%s: read id failed\n", s_ctrl->sensordata->sensor_name);
		return rc;
	}

	debuglog("read id: %x expected id: %x\n", chipid, s_ctrl->sensordata->slave_info->sensor_id);
	if (chipid != s_ctrl->sensordata->slave_info->sensor_id) {
		pr_err("msm_sensor_match_id chip id doesnot match\n");
		return -ENODEV;
	}
	return rc;
}

static void hi258_i2c_write_table(struct msm_sensor_ctrl_t *s_ctrl,
		struct msm_camera_i2c_reg_conf *table,
		int num)
{
	int i = 0;
	int rc = 0;
	for (i = 0; i < num; ++i) {
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
			i2c_write(
			s_ctrl->sensor_i2c_client, table->reg_addr,
			table->reg_data,
			MSM_CAMERA_I2C_BYTE_DATA);
		if (rc < 0) {
			msleep(100);
			rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
			i2c_write(
				s_ctrl->sensor_i2c_client, table->reg_addr,
				table->reg_data,
				MSM_CAMERA_I2C_BYTE_DATA);
		}
		table++;
	}
}

static struct msm_camera_i2c_reg_conf hi258_reg_saturation_0[] = {
	{0x03, 0x10},
	{0x61, 0x50},
	{0x62, 0x50},
};

static struct msm_camera_i2c_reg_conf hi258_reg_saturation_1[] = {
	{0x03, 0x10},
	{0x61, 0x60},
	{0x62, 0x60},
};

static struct msm_camera_i2c_reg_conf hi258_reg_saturation_2[] = {
	{0x03, 0x10},
	{0x61, 0x70},
	{0x62, 0x70},
};

static struct msm_camera_i2c_reg_conf hi258_reg_saturation_3[] = {
	{0x03, 0x10},
	{0x61, 0x80},
	{0x62, 0x80},
};

static struct msm_camera_i2c_reg_conf hi258_reg_saturation_4[] = {
	{0x03, 0x10},
	{0x61, 0x98},
	{0x62, 0x98},
};

static struct msm_camera_i2c_reg_conf hi258_reg_saturation_5[] = {
	{0x03, 0x10},
	{0x61, 0xb0},
	{0x62, 0xb0},
};

static struct msm_camera_i2c_reg_conf hi258_reg_saturation_6[] = {
	{0x03, 0x10},
	{0x61, 0xc0},
	{0x62, 0xc0},
};

static struct msm_camera_i2c_reg_conf hi258_reg_contrast_0[] = {
	{0x03, 0x10},
	{0x48, 0x50},
};

static struct msm_camera_i2c_reg_conf hi258_reg_contrast_1[] = {
	{0x03, 0x10},
	{0x48, 0x60},
};

static struct msm_camera_i2c_reg_conf hi258_reg_contrast_2[] = {
	{0x03, 0x10},
	{0x48, 0x70},
};

static struct msm_camera_i2c_reg_conf hi258_reg_contrast_3[] = {
	{0x03, 0x10},
	{0x48, 0x80},
};

static struct msm_camera_i2c_reg_conf hi258_reg_contrast_4[] = {
	{0x03, 0x10},
	{0x48, 0x90},
};

static struct msm_camera_i2c_reg_conf hi258_reg_contrast_5[] = {
	{0x03, 0x10},
	{0x48, 0xa0},
};

static struct msm_camera_i2c_reg_conf hi258_reg_contrast_6[] = {
	{0x03, 0x10},
	{0x48, 0xb0},
};

static struct msm_camera_i2c_reg_conf hi258_reg_sharpness_0[] = {
	{0x03, 0x13},
	{0xa2, 0x08},
	{0xa3, 0x0a},
	{0xa4, 0x08},
	{0xa5, 0x0a},
	{0xa6, 0x0a},
	{0xa7, 0x0a},
};

static struct msm_camera_i2c_reg_conf hi258_reg_sharpness_1[] = {
	{0x03, 0x13},
	{0xa2, 0x04},
	{0xa3, 0x05},
	{0xa4, 0x04},
	{0xa5, 0x05},
	{0xa6, 0x80},
	{0xa7, 0x80},
};

static struct msm_camera_i2c_reg_conf hi258_reg_sharpness_2[] = {
	{0x03, 0x13},
	{0xa2, 0x02},
	{0xa3, 0x02},
	{0xa4, 0x02},
	{0xa5, 0x02},
	{0xa6, 0x06},
	{0xa7, 0x06},
};

static struct msm_camera_i2c_reg_conf hi258_reg_iso_auto[] = {
	{0x03, 0x10},
	{0x4a, 0x80},
};

static struct msm_camera_i2c_reg_conf hi258_reg_iso_100[] = {
	{0x03, 0x10},
	{0x4a, 0x60},
};

static struct msm_camera_i2c_reg_conf hi258_reg_iso_200[] = {
	{0x03, 0x10},
	{0x4a, 0x70},
};

static struct msm_camera_i2c_reg_conf hi258_reg_iso_400[] = {
	{0x03, 0x10},
	{0x4a, 0x80},
};

static struct msm_camera_i2c_reg_conf hi258_reg_iso_800[] = {
	{0x03, 0x10},
	{0x4a, 0x90},
};

static struct msm_camera_i2c_reg_conf hi258_reg_iso_1600[] = {
	{0x03, 0x10},
	{0x4a, 0xa0},
};

static struct msm_camera_i2c_reg_conf hi258_reg_exposure_compensation_0[] = {
	{0x03, 0x10},
	{0x40, 0xe0},
};

static struct msm_camera_i2c_reg_conf hi258_reg_exposure_compensation_1[] = {
	{0x03, 0x10},
	{0x40, 0xc8},
};

static struct msm_camera_i2c_reg_conf hi258_reg_exposure_compensation_2[] = {
	{0x03, 0x10},
	{0x40, 0xb0},
};

static struct msm_camera_i2c_reg_conf hi258_reg_exposure_compensation_3[] = {
	{0x03, 0x10},
	{0x40, 0x98},
};

static struct msm_camera_i2c_reg_conf hi258_reg_exposure_compensation_4[] = {
	{0x03, 0x10},
	{0x40, 0x80},
};

static struct msm_camera_i2c_reg_conf hi258_reg_exposure_compensation_5[] = {
	{0x03, 0x10},
	{0x40, 0x18},
};

static struct msm_camera_i2c_reg_conf hi258_reg_exposure_compensation_6[] = {
	{0x03, 0x10},
	{0x40, 0x30},
};

static struct msm_camera_i2c_reg_conf hi258_reg_exposure_compensation_7[] = {
	{0x03, 0x10},
	{0x40, 0x48},
};

static struct msm_camera_i2c_reg_conf hi258_reg_exposure_compensation_8[] = {
	{0x03, 0x10},
	{0x40, 0x60},
};

static struct msm_camera_i2c_reg_conf hi258_reg_antibanding_60hz[] = {
	{0x03, 0x20},
	{0x10, 0x8c},
};

static struct msm_camera_i2c_reg_conf hi258_reg_antibanding_50hz[] = {
	{0x03, 0x20},
	{0x10, 0x9c},
};

static struct msm_camera_i2c_reg_conf hi258_reg_effect_off[] = {
	{0x03, 0x10},
	{0x11, 0x03},
	{0x12, 0x30},
	{0x13, 0x03},
	{0x44, 0x80},
	{0x45, 0x80},
	{0x4a, 0x80},
};

static struct msm_camera_i2c_reg_conf hi258_reg_effect_sepia[] = {
	{0x03, 0x10},
	{0x11, 0x03},
	{0x12, 0x33},
	{0x13, 0x02},
	{0x44, 0x70},
	{0x45, 0x98},
	{0x4a, 0x80},
};

static struct msm_camera_i2c_reg_conf hi258_reg_effect_negative[] = {
	{0x03, 0x10},
	{0x11, 0x03},
	{0x12, 0x38},
	{0x13, 0x02},
	{0x14, 0x00},
	{0x4a, 0x80},
};

static struct msm_camera_i2c_reg_conf hi258_reg_effect_mono[] = {
	{0x03, 0x10},
	{0x11, 0x03},
	{0x12, 0x33},
	{0x13, 0x02},
	{0x44, 0x80},
	{0x45, 0x80},
	{0x4a, 0x80},
};

static struct msm_camera_i2c_reg_conf hi258_reg_wb_auto[] = {
	{0x03, 0x22},
	{0x11, 0x2e},
	{0x83, 0x58},
	{0x84, 0x16},
	{0x85, 0x4f},
	{0x86, 0x20},
};

static struct msm_camera_i2c_reg_conf hi258_reg_wb_cloudy[] = {
	{0x03, 0x22},
	{0x11, 0x28},
	{0x80, 0x71},
	{0x82, 0x2b},
	{0x83, 0x72},
	{0x84, 0x70},
	{0x85, 0x2b},
	{0x86, 0x28},
};

static struct msm_camera_i2c_reg_conf hi258_reg_wb_daylight[] = {
	{0x03, 0x22},
	{0x11, 0x28},
	{0x80, 0x59},
	{0x82, 0x29},
	{0x83, 0x60},
	{0x84, 0x50},
	{0x85, 0x2f},
	{0x86, 0x23},
};

static struct msm_camera_i2c_reg_conf hi258_reg_wb_incandescent[] = {
	{0x03, 0x22},
	{0x11, 0x28},
	{0x80, 0x29},
	{0x82, 0x54},
	{0x83, 0x2e},
	{0x84, 0x23},
	{0x85, 0x58},
	{0x86, 0x4f},
};

static struct msm_camera_i2c_reg_conf hi258_reg_wb_fluorescent[] = {
	{0x03, 0x22},
	{0x11, 0x28},
	{0x80, 0x41},
	{0x82, 0x42},
	{0x83, 0x44},
	{0x84, 0x34},
	{0x85, 0x46},
	{0x86, 0x3a},
};

static void hi258_set_stauration(struct msm_sensor_ctrl_t *s_ctrl, int value)
{
	debuglog("%d", value);
	switch (value) {
		case 0:
		case 1:
		case 2:
			hi258_i2c_write_table(s_ctrl, &hi258_reg_saturation_0[0],
				ARRAY_SIZE(hi258_reg_saturation_0));
			break;
		case 3:
			hi258_i2c_write_table(s_ctrl, &hi258_reg_saturation_1[0],
				ARRAY_SIZE(hi258_reg_saturation_1));
			break;
		case 4:
			hi258_i2c_write_table(s_ctrl, &hi258_reg_saturation_2[0],
				ARRAY_SIZE(hi258_reg_saturation_2));
			break;
		case 5: //default
			hi258_i2c_write_table(s_ctrl, &hi258_reg_saturation_3[0],
				ARRAY_SIZE(hi258_reg_saturation_3));
			break;
		case 6:
			hi258_i2c_write_table(s_ctrl, &hi258_reg_saturation_4[0],
				ARRAY_SIZE(hi258_reg_saturation_4));
			break;
		case 7:
			hi258_i2c_write_table(s_ctrl, &hi258_reg_saturation_5[0],
				ARRAY_SIZE(hi258_reg_saturation_5));
			break;
		case 8:
		case 9:
		case 10:
			hi258_i2c_write_table(s_ctrl, &hi258_reg_saturation_6[0],
				ARRAY_SIZE(hi258_reg_saturation_6));
			break;
		default:
			break;
	}
}

static void hi258_set_contrast(struct msm_sensor_ctrl_t *s_ctrl, int value)
{
	debuglog("%d", value);
	switch (value) {
		case 0:
		case 1:
		case 2:
			hi258_i2c_write_table(s_ctrl, &hi258_reg_contrast_0[0],
				ARRAY_SIZE(hi258_reg_contrast_0));
			break;
		case 3:
			hi258_i2c_write_table(s_ctrl, &hi258_reg_contrast_1[0],
				ARRAY_SIZE(hi258_reg_contrast_1));
			break;
		case 4:
			hi258_i2c_write_table(s_ctrl, &hi258_reg_contrast_2[0],
				ARRAY_SIZE(hi258_reg_contrast_2));
			break;
		case 5: //default
			hi258_i2c_write_table(s_ctrl, &hi258_reg_contrast_3[0],
				ARRAY_SIZE(hi258_reg_contrast_3));
			break;
		case 6:
			hi258_i2c_write_table(s_ctrl, &hi258_reg_contrast_4[0],
				ARRAY_SIZE(hi258_reg_contrast_4));
			break;
		case 7:
			hi258_i2c_write_table(s_ctrl, &hi258_reg_contrast_5[0],
				ARRAY_SIZE(hi258_reg_contrast_5));
			break;
		case 8:
		case 9:
		case 10:
			hi258_i2c_write_table(s_ctrl, &hi258_reg_contrast_6[0],
				ARRAY_SIZE(hi258_reg_contrast_6));
			break;
		default:
			break;
	}
}

static void hi258_set_sharpness(struct msm_sensor_ctrl_t *s_ctrl, int value)
{
	debuglog("%d", value);
	switch (value) {
		case 0:
			hi258_i2c_write_table(s_ctrl, &hi258_reg_sharpness_0[0],
				ARRAY_SIZE(hi258_reg_sharpness_0));
			break;
		case 12: //default
			hi258_i2c_write_table(s_ctrl, &hi258_reg_sharpness_1[0],
				ARRAY_SIZE(hi258_reg_sharpness_1));
			break;
		case 24:
		case 36:
			hi258_i2c_write_table(s_ctrl, &hi258_reg_sharpness_2[0],
				ARRAY_SIZE(hi258_reg_sharpness_2));
			break;
		default:
			break;
	}
}

static void hi258_set_iso(struct msm_sensor_ctrl_t *s_ctrl, int value)
{
	debuglog("%d", value);
	switch (value) {
		case 0: //default
			hi258_i2c_write_table(s_ctrl, &hi258_reg_iso_auto[0],
				ARRAY_SIZE(hi258_reg_iso_auto));
			break;
		case 2:
			hi258_i2c_write_table(s_ctrl, &hi258_reg_iso_100[0],
				ARRAY_SIZE(hi258_reg_iso_100));
			break;
		case 3:
			hi258_i2c_write_table(s_ctrl, &hi258_reg_iso_200[0],
				ARRAY_SIZE(hi258_reg_iso_200));
			break;
		case 4:
			hi258_i2c_write_table(s_ctrl, &hi258_reg_iso_400[0],
				ARRAY_SIZE(hi258_reg_iso_400));
			break;
		case 5 :
			hi258_i2c_write_table(s_ctrl, &hi258_reg_iso_800[0],
				ARRAY_SIZE(hi258_reg_iso_800));
			break;
		case 6:
			hi258_i2c_write_table(s_ctrl, &hi258_reg_iso_1600[0],
				ARRAY_SIZE(hi258_reg_iso_1600));
			break;
		default:
			break;
	}
}

static void hi258_set_exposure_compensation(struct msm_sensor_ctrl_t *s_ctrl,
	int value)
{
	debuglog("%d", value);
	switch (value) {
		case -12:
			hi258_i2c_write_table(s_ctrl, &hi258_reg_exposure_compensation_0[0],
				ARRAY_SIZE(hi258_reg_exposure_compensation_0));
			break;
		case -9:
		case -8:
			hi258_i2c_write_table(s_ctrl, &hi258_reg_exposure_compensation_1[0],
				ARRAY_SIZE(hi258_reg_exposure_compensation_1));
			break;
		case -6:
		case -4:
			hi258_i2c_write_table(s_ctrl, &hi258_reg_exposure_compensation_2[0],
				ARRAY_SIZE(hi258_reg_exposure_compensation_2));
			break;
		case -3:
			hi258_i2c_write_table(s_ctrl, &hi258_reg_exposure_compensation_3[0],
				ARRAY_SIZE(hi258_reg_exposure_compensation_3));
			break;
		case 0: //default
			hi258_i2c_write_table(s_ctrl, &hi258_reg_exposure_compensation_4[0],
				ARRAY_SIZE(hi258_reg_exposure_compensation_4));
			break;
		case 3:
			hi258_i2c_write_table(s_ctrl, &hi258_reg_exposure_compensation_5[0],
				ARRAY_SIZE(hi258_reg_exposure_compensation_5));
			break;
		case 4:
		case 6:
			hi258_i2c_write_table(s_ctrl, &hi258_reg_exposure_compensation_6[0],
				ARRAY_SIZE(hi258_reg_exposure_compensation_6));
			break;
		case 8:
		case 9:
			hi258_i2c_write_table(s_ctrl, &hi258_reg_exposure_compensation_7[0],
				ARRAY_SIZE(hi258_reg_exposure_compensation_7));
			break;
		case 12:
			hi258_i2c_write_table(s_ctrl, &hi258_reg_exposure_compensation_8[0],
				ARRAY_SIZE(hi258_reg_exposure_compensation_8));
			break;
		default:
			break;
	}
}

static void hi258_set_effect(struct msm_sensor_ctrl_t *s_ctrl, int value)
{
	debuglog("%d", value);
	switch (value) {
		case MSM_CAMERA_EFFECT_MODE_OFF: //default
			hi258_i2c_write_table(s_ctrl, &hi258_reg_effect_off[0],
				ARRAY_SIZE(hi258_reg_effect_off));
			break;
		case MSM_CAMERA_EFFECT_MODE_MONO: 
			hi258_i2c_write_table(s_ctrl, &hi258_reg_effect_mono[0],
				ARRAY_SIZE(hi258_reg_effect_mono));
			break;
		case MSM_CAMERA_EFFECT_MODE_NEGATIVE: 
			hi258_i2c_write_table(s_ctrl, &hi258_reg_effect_negative[0],
				ARRAY_SIZE(hi258_reg_effect_negative));
			break;
		case MSM_CAMERA_EFFECT_MODE_SEPIA: 
			hi258_i2c_write_table(s_ctrl, &hi258_reg_effect_sepia[0],
				ARRAY_SIZE(hi258_reg_effect_sepia));
			break;
		default:
			break;
	}
}

static void hi258_set_scene_mode(struct msm_sensor_ctrl_t *s_ctrl, int value)
{
	debuglog("%d", value);
}

static void hi258_set_antibanding(struct msm_sensor_ctrl_t *s_ctrl, int value)
{
	debuglog("%d", value);
	switch (value) {
		case 1: 
			hi258_i2c_write_table(s_ctrl, &hi258_reg_antibanding_60hz[0],
				ARRAY_SIZE(hi258_reg_antibanding_60hz));
			break;
		case 2: //default
			hi258_i2c_write_table(s_ctrl, &hi258_reg_antibanding_50hz[0],
				ARRAY_SIZE(hi258_reg_antibanding_50hz));
			break;
		default:
			break;
	}
}

static void hi258_set_white_balance_mode(struct msm_sensor_ctrl_t *s_ctrl,
	int value)
{
	debuglog("%d", value);
	switch (value) {
		case MSM_CAMERA_WB_MODE_AUTO: //default
			hi258_i2c_write_table(s_ctrl, &hi258_reg_wb_auto[0],
				ARRAY_SIZE(hi258_reg_wb_auto));
			break;
		case MSM_CAMERA_WB_MODE_INCANDESCENT: 
			hi258_i2c_write_table(s_ctrl, &hi258_reg_wb_incandescent[0],
				ARRAY_SIZE(hi258_reg_wb_incandescent));
			break;
		case MSM_CAMERA_WB_MODE_DAYLIGHT: 
			hi258_i2c_write_table(s_ctrl, &hi258_reg_wb_daylight[0],
				ARRAY_SIZE(hi258_reg_wb_daylight));
			break;
		case MSM_CAMERA_WB_MODE_FLUORESCENT: 
			hi258_i2c_write_table(s_ctrl, &hi258_reg_wb_fluorescent[0],
				ARRAY_SIZE(hi258_reg_wb_fluorescent));
						break;
		case MSM_CAMERA_WB_MODE_CLOUDY_DAYLIGHT: 
			hi258_i2c_write_table(s_ctrl, &hi258_reg_wb_cloudy[0],
				ARRAY_SIZE(hi258_reg_wb_cloudy));
			break;
		default:
			break;
	}
}

static int32_t hi258_sensor_config(struct msm_sensor_ctrl_t *s_ctrl,
	void __user *argp)
{
	struct sensorb_cfg_data *cdata = (struct sensorb_cfg_data *)argp;
	long rc = 0;
	int32_t i = 0;

	mutex_lock(s_ctrl->msm_sensor_mutex);
	debuglog("%s cfgtype = %d\n", s_ctrl->sensordata->sensor_name, cdata->cfgtype);
	switch (cdata->cfgtype) {
		case CFG_GET_SENSOR_INFO:
			memcpy(cdata->cfg.sensor_info.sensor_name,
				s_ctrl->sensordata->sensor_name,
				sizeof(cdata->cfg.sensor_info.sensor_name));
			cdata->cfg.sensor_info.session_id =
				s_ctrl->sensordata->sensor_info->session_id;
			for (i = 0; i < SUB_MODULE_MAX; i++)
				cdata->cfg.sensor_info.subdev_id[i] =
					s_ctrl->sensordata->sensor_info->subdev_id[i];
			debuglog("sensor name %s\n", cdata->cfg.sensor_info.sensor_name);
			debuglog("session id %d\n", cdata->cfg.sensor_info.session_id);
			for (i = 0; i < SUB_MODULE_MAX; i++)
				debuglog("subdev_id[%d] %d\n", i, cdata->cfg.sensor_info.subdev_id[i]);
			break;
		case CFG_SET_INIT_SETTING:
			debuglog("=====> INcase: [CFG_SET_INIT_SETTING]\n");
			/* 1. Write Recommend settings */
			/* 2. Write change settings */
			rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
			    i2c_write_conf_tbl(
			    s_ctrl->sensor_i2c_client, hi258_recommend_settings,
			    ARRAY_SIZE(hi258_recommend_settings),
			    MSM_CAMERA_I2C_BYTE_DATA);
			break;
		case CFG_SET_RESOLUTION: {
			int32_t val;
			if (copy_from_user(&val,
				(void *)cdata->cfg.setting, sizeof(int))) {
				pr_err("failed\n");
				rc = -EFAULT;
				break;
			}
			debuglog("=====> INcase: [CFG_SET_RESOLUTION], val=<%d>\n", val);
			if (0 == val) {
				rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
					i2c_write_conf_tbl(
					s_ctrl->sensor_i2c_client, hi258_snapshot_settings,
					ARRAY_SIZE(hi258_snapshot_settings),
					MSM_CAMERA_I2C_BYTE_DATA);
			} else if (1 == val) {
				rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
					i2c_write_conf_tbl(
					s_ctrl->sensor_i2c_client, hi258_preview_settings,
					ARRAY_SIZE(hi258_preview_settings),
					MSM_CAMERA_I2C_BYTE_DATA);
			} else if (2 == val) {
				rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
					i2c_write_conf_tbl(
					s_ctrl->sensor_i2c_client, hi258_video_settings,
					ARRAY_SIZE(hi258_video_settings),
					MSM_CAMERA_I2C_BYTE_DATA);
			}
		}
			break;
		case CFG_SET_STOP_STREAM:
			debuglog("=====> INcase: [CFG_SET_STOP_STREAM]\n");
	/*[BUGFIX]-Mod-BEGIN by TCTNB.WPL, bug564473, 2013/11/30 */
			rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
				i2c_write_conf_tbl(
				s_ctrl->sensor_i2c_client, hi258_stop_settings,
				ARRAY_SIZE(hi258_stop_settings),
				MSM_CAMERA_I2C_BYTE_DATA);
	/*[BUGFIX]-Mod-END by TCTNB.WPL */
			break;
		case CFG_SET_START_STREAM:
			debuglog("=====> INcase: [CFG_SET_START_STREAM]\n");
	/*[BUGFIX]-Mod-BEGIN by TCTNB.WPL, bug564473, 2013/11/30 */
			rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
				i2c_write_conf_tbl(
				s_ctrl->sensor_i2c_client, hi258_start_settings,
				ARRAY_SIZE(hi258_start_settings),
				MSM_CAMERA_I2C_BYTE_DATA);
	/*[BUGFIX]-Mod-END by TCTNB.WPL */
			break;
		case CFG_GET_SENSOR_INIT_PARAMS:
			cdata->cfg.sensor_init_params =
				*s_ctrl->sensordata->sensor_init_params;
			debuglog("init params mode %d pos %d mount %d\n", 
				cdata->cfg.sensor_init_params.modes_supported,
				cdata->cfg.sensor_init_params.position,
				cdata->cfg.sensor_init_params.sensor_mount_angle);
			break;
		case CFG_SET_SLAVE_INFO: {
			struct msm_camera_sensor_slave_info sensor_slave_info;
			struct msm_sensor_power_setting_array *power_setting_array;
			int slave_index = 0;
			if (copy_from_user(&sensor_slave_info,
			    (void *)cdata->cfg.setting,
			    sizeof(struct msm_camera_sensor_slave_info))) {
				pr_err("failed\n");
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
				pr_err("failed\n");
				rc = -ENOMEM;
				break;
			}
			if (copy_from_user(power_setting_array->power_setting,
			    (void *)sensor_slave_info.power_setting_array.power_setting,
			    power_setting_array->size *
			    sizeof(struct msm_sensor_power_setting))) {
				kfree(power_setting_array->power_setting);
				pr_err("failed\n");
				rc = -EFAULT;
				break;
			}
			s_ctrl->free_power_setting = true;
			debuglog("sensor id %x\n", sensor_slave_info.slave_addr);
			debuglog("sensor addr type %d\n", sensor_slave_info.addr_type);
			debuglog("sensor reg %x\n", sensor_slave_info.sensor_id_info.sensor_id_reg_addr);
			debuglog("sensor id %x\n", sensor_slave_info.sensor_id_info.sensor_id);
			for (slave_index = 0; slave_index <
				power_setting_array->size; slave_index++) {
				debuglog("i %d power setting %d %d %ld %d\n", 
					slave_index,
					power_setting_array->power_setting[slave_index].seq_type,
					power_setting_array->power_setting[slave_index].seq_val,
					power_setting_array->power_setting[slave_index].config_val,
					power_setting_array->power_setting[slave_index].delay);
			}
			kfree(power_setting_array->power_setting);
		}
			break;
		case CFG_WRITE_I2C_ARRAY: {
			struct msm_camera_i2c_reg_setting conf_array;
			struct msm_camera_i2c_reg_array *reg_setting = NULL;
			debuglog("=====> INcase: [CFG_WRITE_I2C_ARRAY]\n");
			if (copy_from_user(&conf_array,
				(void *)cdata->cfg.setting,
				sizeof(struct msm_camera_i2c_reg_setting))) {
				pr_err("failed\n");
				rc = -EFAULT;
				break;
			}
			reg_setting = kzalloc(conf_array.size *
				(sizeof(struct msm_camera_i2c_reg_array)), GFP_KERNEL);
			if (!reg_setting) {
				pr_err("failed\n");
				rc = -ENOMEM;
				break;
			}
			if (copy_from_user(reg_setting, (void *)conf_array.reg_setting,
				conf_array.size *
				sizeof(struct msm_camera_i2c_reg_array))) {
				pr_err("failed\n");
				kfree(reg_setting);
				rc = -EFAULT;
				break;
			}
			conf_array.reg_setting = reg_setting;
			rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write_table(
				s_ctrl->sensor_i2c_client, &conf_array);
			kfree(reg_setting);
		}
			break;
		case CFG_WRITE_I2C_SEQ_ARRAY: {
			struct msm_camera_i2c_seq_reg_setting conf_array;
			struct msm_camera_i2c_seq_reg_array *reg_setting = NULL;
			debuglog("=====> INcase: [CFG_WRITE_I2C_SEQ_ARRAY]\n");
			if (copy_from_user(&conf_array,
				(void *)cdata->cfg.setting,
				sizeof(struct msm_camera_i2c_seq_reg_setting))) {
				pr_err("failed\n");
				rc = -EFAULT;
				break;
			}
			reg_setting = kzalloc(conf_array.size *
				(sizeof(struct msm_camera_i2c_seq_reg_array)),
				GFP_KERNEL);
			if (!reg_setting) {
				pr_err("failed\n");
				rc = -ENOMEM;
				break;
			}
			if (copy_from_user(reg_setting, (void *)conf_array.reg_setting,
				conf_array.size *
				sizeof(struct msm_camera_i2c_seq_reg_array))) {
				pr_err("failed\n");
				kfree(reg_setting);
				rc = -EFAULT;
				break;
			}

			conf_array.reg_setting = reg_setting;
			rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
				i2c_write_seq_table(s_ctrl->sensor_i2c_client,
				&conf_array);
			kfree(reg_setting);
		}
			break;
		case CFG_POWER_UP:
			debuglog("=====> INcase: [CFG_POWER_UP]\n");
			if (s_ctrl->func_tbl->sensor_power_up)
				rc = s_ctrl->func_tbl->sensor_power_up(s_ctrl);
			else
				rc = -EFAULT;
			break;
		case CFG_POWER_DOWN:
			debuglog("=====> INcase: [CFG_POWER_DOWN]\n");
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
			debuglog("=====> INcase: [CFG_SET_STOP_STREAM_SETTING]\n");
			if (copy_from_user(stop_setting, (void *)cdata->cfg.setting,
			    sizeof(struct msm_camera_i2c_reg_setting))) {
				pr_err("failed\n");
				rc = -EFAULT;
				break;
			}

			reg_setting = stop_setting->reg_setting;
			stop_setting->reg_setting = kzalloc(stop_setting->size *
				(sizeof(struct msm_camera_i2c_reg_array)), GFP_KERNEL);
			if (!stop_setting->reg_setting) {
				pr_err("failed\n");
				rc = -ENOMEM;
				break;
			}
			if (copy_from_user(stop_setting->reg_setting,
			    (void *)reg_setting, stop_setting->size *
			    sizeof(struct msm_camera_i2c_reg_array))) {
				pr_err("failed\n");
				kfree(stop_setting->reg_setting);
				stop_setting->reg_setting = NULL;
				stop_setting->size = 0;
				rc = -EFAULT;
				break;
			}
		}
			break;
		case CFG_SET_SATURATION: {
			int32_t sat_lev;
			if (copy_from_user(&sat_lev, (void *)cdata->cfg.setting,
				sizeof(int32_t))) {
				pr_err("failed\n");
				rc = -EFAULT;
				break;
			}
			hi258_set_stauration(s_ctrl, sat_lev);
		}
			break;
		case CFG_SET_CONTRAST: {
			int32_t con_lev;
			if (copy_from_user(&con_lev, (void *)cdata->cfg.setting,
				sizeof(int32_t))) {
				pr_err("failed\n");
				rc = -EFAULT;
				break;
			}
			hi258_set_contrast(s_ctrl, con_lev);
		}
			break;
		case CFG_SET_SHARPNESS: {
			int32_t shp_lev;
			if (copy_from_user(&shp_lev, (void *)cdata->cfg.setting,
				sizeof(int32_t))) {
				pr_err("failed\n");
				rc = -EFAULT;
				break;
			}
			hi258_set_sharpness(s_ctrl, shp_lev);
		}
			break;
		case CFG_SET_ISO: {
			int32_t iso_lev;
			if (copy_from_user(&iso_lev, (void *)cdata->cfg.setting,
				sizeof(int32_t))) {
				pr_err("failed\n");
				rc = -EFAULT;
				break;
			}
			hi258_set_iso(s_ctrl, iso_lev);
		}
			break;	
		case CFG_SET_EXPOSURE_COMPENSATION: {
			int32_t ec_lev;
			if (copy_from_user(&ec_lev, (void *)cdata->cfg.setting,
				sizeof(int32_t))) {
				pr_err("failed\n");
				rc = -EFAULT;
				break;
			}
			hi258_set_exposure_compensation(s_ctrl, ec_lev);
		}
			break;
		case CFG_SET_EFFECT: {
			int32_t effect_mode;
			if (copy_from_user(&effect_mode, (void *)cdata->cfg.setting,
				sizeof(int32_t))) {
				pr_err("failed\n");
				rc = -EFAULT;
				break;
			}
			hi258_set_effect(s_ctrl, effect_mode);
		}
			break;
		case CFG_SET_ANTIBANDING: {
			int32_t anti_mode;
			if (copy_from_user(&anti_mode,
				(void *)cdata->cfg.setting,
				sizeof(int32_t))) {
				pr_err("failed\n");
				rc = -EFAULT;
				break;
			}
			hi258_set_antibanding(s_ctrl, anti_mode);
		}
			break;
		case CFG_SET_BESTSHOT_MODE: {
			int32_t bs_mode;
			if (copy_from_user(&bs_mode, (void *)cdata->cfg.setting,
				sizeof(int32_t))) {
				pr_err("failed\n");
				rc = -EFAULT;
				break;
			}
			hi258_set_scene_mode(s_ctrl, bs_mode);
		}
			break;	
		case CFG_SET_WHITE_BALANCE: {
			int32_t wb_mode;
			if (copy_from_user(&wb_mode, (void *)cdata->cfg.setting,
				sizeof(int32_t))) {
				pr_err("failed\n");
				rc = -EFAULT;
				break;
			}
			hi258_set_white_balance_mode(s_ctrl, wb_mode);
		}
			break;
		default:
			rc = -EFAULT;
			break;
	}

	mutex_unlock(s_ctrl->msm_sensor_mutex);

	return rc;
}

static int32_t hi258_sensor_power_up(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0, index = 0;
	struct msm_sensor_power_setting_array *power_setting_array = NULL;
	struct msm_sensor_power_setting *power_setting = NULL;
	struct msm_camera_sensor_board_info *data = s_ctrl->sensordata;
	s_ctrl->stop_setting_valid = 0;

	debuglog("enter\n");
	power_setting_array = &s_ctrl->power_setting_array;

	if (data->gpio_conf->cam_gpiomux_conf_tbl != NULL) {
		debuglog("mux install\n");
		msm_gpiomux_install(
			(struct msm_gpiomux_config *)
			data->gpio_conf->cam_gpiomux_conf_tbl,
			data->gpio_conf->cam_gpiomux_conf_tbl_size);
	}

	rc = msm_camera_request_gpio_table(
		data->gpio_conf->cam_gpio_req_tbl,
		data->gpio_conf->cam_gpio_req_tbl_size, 1);
	if (rc < 0) {
		pr_err("request gpio failed\n");
		return rc;
	}
	for (index = 0; index < power_setting_array->size; index++) {
		power_setting = &power_setting_array->power_setting[index];
		switch (power_setting->seq_type) {
		case SENSOR_CLK:
			if (power_setting->seq_val >= s_ctrl->clk_info_size) {
				pr_err("clk index %d >= max %d\n",
					power_setting->seq_val,
					s_ctrl->clk_info_size);
				goto power_up_failed;
			}
			if (power_setting->config_val)
				s_ctrl->clk_info[power_setting->seq_val].
					clk_rate = power_setting->config_val;

			rc = msm_cam_clk_enable(s_ctrl->dev,
				&s_ctrl->clk_info[0],
				(struct clk **)&power_setting->data[0],
				s_ctrl->clk_info_size,
				1);
			if (rc < 0) {
				pr_err("clk enable failed\n");
				goto power_up_failed;
			}
			break;
		case SENSOR_GPIO:
			if (power_setting->seq_val >= SENSOR_GPIO_MAX ||
				!data->gpio_conf->gpio_num_info) {
				pr_err("gpio index %d >= max %d\n",
					power_setting->seq_val,
					SENSOR_GPIO_MAX);
				goto power_up_failed;
			}
			debuglog("gpio set val %d\n",
				data->gpio_conf->gpio_num_info->gpio_num[power_setting->seq_val]);
			gpio_set_value_cansleep(
				data->gpio_conf->gpio_num_info->gpio_num[power_setting->seq_val],
				power_setting->config_val);
			break;
		case SENSOR_VREG:
			if (power_setting->seq_val >= CAM_VREG_MAX) {
				pr_err("vreg index %d >= max %d\n",
					power_setting->seq_val,
					SENSOR_GPIO_MAX);
				goto power_up_failed;
			}
			msm_camera_config_single_vreg(s_ctrl->dev,
				&data->cam_vreg[power_setting->seq_val],
				(struct regulator **)&power_setting->data[0],
				1);
			break;
		default:
			pr_err("error power seq type %d\n", power_setting->seq_type);
			break;
		}
		if (power_setting->delay > 20) {
			msleep(power_setting->delay);
		} else if (power_setting->delay) {
			usleep_range(power_setting->delay * 1000,
				(power_setting->delay * 1000) + 1000);
		}
	}

	if (s_ctrl->sensor_device_type == MSM_CAMERA_PLATFORM_DEVICE) {
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_util(
			s_ctrl->sensor_i2c_client, MSM_CCI_INIT);
		if (rc < 0) {
			pr_err("cci_init failed\n");
			goto power_up_failed;
		}
	}

	if (s_ctrl->func_tbl->sensor_match_id)
		rc = s_ctrl->func_tbl->sensor_match_id(s_ctrl);
	else
		rc = msm_sensor_match_id(s_ctrl);
	if (rc < 0) {
		pr_err("match id failed rc %d\n", rc);
		goto power_up_failed;
	}

	debuglog("exit\n");
	return 0;
power_up_failed:
	pr_err("failed\n");
	if (s_ctrl->sensor_device_type == MSM_CAMERA_PLATFORM_DEVICE) {
		s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_util(
			s_ctrl->sensor_i2c_client, MSM_CCI_RELEASE);
	}

	for (index--; index >= 0; index--) {
		power_setting = &power_setting_array->power_setting[index];
		switch (power_setting->seq_type) {
		case SENSOR_CLK:
			msm_cam_clk_enable(s_ctrl->dev,
				&s_ctrl->clk_info[0],
				(struct clk **)&power_setting->data[0],
				s_ctrl->clk_info_size,
				0);
			break;
		case SENSOR_GPIO:
			gpio_set_value_cansleep(
				data->gpio_conf->gpio_num_info->gpio_num[power_setting->seq_val], 
				GPIOF_OUT_INIT_LOW);
			break;
		case SENSOR_VREG:
			msm_camera_config_single_vreg(s_ctrl->dev,
				&data->cam_vreg[power_setting->seq_val],
				(struct regulator **)&power_setting->data[0],
				0);
			break;
		default:
			pr_err("error power seq type %d\n", power_setting->seq_type);
			break;
		}
		if (power_setting->delay > 20) {
			msleep(power_setting->delay);
		} else if (power_setting->delay) {
			usleep_range(power_setting->delay * 1000,
				(power_setting->delay * 1000) + 1000);
		}
	}
	msm_camera_request_gpio_table(
		data->gpio_conf->cam_gpio_req_tbl,
		data->gpio_conf->cam_gpio_req_tbl_size, 0);
	return rc;
}

static int32_t hi258_sensor_power_down(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t index = 0;
	struct msm_sensor_power_setting_array *power_setting_array = NULL;
	struct msm_sensor_power_setting *power_setting = NULL;
	struct msm_camera_sensor_board_info *data = s_ctrl->sensordata;
	s_ctrl->stop_setting_valid = 0;

	debuglog("enter\n");
	power_setting_array = &s_ctrl->power_setting_array;

	s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write(s_ctrl->sensor_i2c_client, \
			0x03, 0x02, MSM_CAMERA_I2C_BYTE_DATA);
	s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write(s_ctrl->sensor_i2c_client, \
			0x55, 0x10, MSM_CAMERA_I2C_BYTE_DATA);

	if (s_ctrl->sensor_device_type == MSM_CAMERA_PLATFORM_DEVICE) {
		s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_util(
			s_ctrl->sensor_i2c_client, MSM_CCI_RELEASE);
	}
#if 1
	gpio_set_value_cansleep(data->gpio_conf->gpio_num_info->gpio_num[SENSOR_GPIO_RESET], 
		GPIO_OUT_LOW);
	mdelay(1);
	gpio_set_value_cansleep(data->gpio_conf->gpio_num_info->gpio_num[SENSOR_GPIO_STANDBY], 
		GPIO_OUT_HIGH);
#endif
	for (index = (power_setting_array->size - 1); index >= 0; index--) {
		power_setting = &power_setting_array->power_setting[index];
		switch (power_setting->seq_type) {
		case SENSOR_CLK:
			msm_cam_clk_enable(s_ctrl->dev,
				&s_ctrl->clk_info[0],
				(struct clk **)&power_setting->data[0],
				s_ctrl->clk_info_size,
				0);
			break;
		case SENSOR_GPIO:
#if 0
			if (power_setting->seq_val >= SENSOR_GPIO_MAX ||
				!data->gpio_conf->gpio_num_info) {
				pr_err("gpio index %d >= max %d\n",
					power_setting->seq_val,
					SENSOR_GPIO_MAX);
				continue;
			}
			gpio_set_value_cansleep(
				data->gpio_conf->gpio_num_info->gpio_num[power_setting->seq_val], 
				GPIOF_OUT_INIT_LOW);
#endif
			break;
		case SENSOR_VREG:
			if (power_setting->seq_val >= CAM_VREG_MAX) {
				pr_err("vreg index %d >= max %d\n",
					power_setting->seq_val,
					SENSOR_GPIO_MAX);
				continue;
			}
			msm_camera_config_single_vreg(s_ctrl->dev,
				&data->cam_vreg[power_setting->seq_val],
				(struct regulator **)&power_setting->data[0],
				0);
			break;
		default:
			pr_err("error power seq type %d\n",	power_setting->seq_type);
			break;
		}
		if (power_setting->delay > 20) {
			msleep(power_setting->delay);
		} else if (power_setting->delay) {
			usleep_range(power_setting->delay * 1000,
				(power_setting->delay * 1000) + 1000);
		}
	}
	msm_camera_request_gpio_table(
		data->gpio_conf->cam_gpio_req_tbl,
		data->gpio_conf->cam_gpio_req_tbl_size, 0);

	debuglog("exit\n");
	return 0;
}

static struct msm_sensor_fn_t hi258_sensor_func_tbl = {
	.sensor_config = hi258_sensor_config,
	.sensor_power_up = hi258_sensor_power_up,
	.sensor_power_down = hi258_sensor_power_down,
	.sensor_match_id = hi258_sensor_match_id,
};

static struct msm_sensor_ctrl_t hi258_s_ctrl = {
	.sensor_i2c_client = &hi258_sensor_i2c_client,
	.power_setting_array.power_setting = hi258_power_setting,
	.power_setting_array.size = ARRAY_SIZE(hi258_power_setting),
	.msm_sensor_mutex = &hi258_pop7lte_mut,
	.sensor_v4l2_subdev_info = hi258_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(hi258_subdev_info),
	.func_tbl = &hi258_sensor_func_tbl,
};

module_init(hi258_init_module);
module_exit(hi258_exit_module);
MODULE_DESCRIPTION("Gcore 2MP YUV sensor driver");
MODULE_LICENSE("GPL v2");

