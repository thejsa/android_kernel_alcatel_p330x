/*
 *
 * FocalTech ft5x06 TouchScreen driver header file.
 *
 * Copyright (c) 2010  Focal tech Ltd.
 * Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef __LINUX_FT5X06_TS_H__
#define __LINUX_FT5X06_TS_H__

#define FT5X06_ID		0x55
#define FT5X16_ID		0x0A
#define FT5X36_ID		0x14
#define FT6X06_ID		0x06

struct fw_upgrade_info {
	bool auto_cal;
	u16 delay_aa;
	u16 delay_55;
	u8 upgrade_id_1;
	u8 upgrade_id_2;
	u16 delay_readid;
	u16 delay_erase_flash;
};

struct ft5x06_ts_platform_data {
	struct fw_upgrade_info info;
	const char *name;
	const char *fw_name;
	u32 irqflags;
	u32 irq_gpio;
	u32 irq_gpio_flags;
	u32 reset_gpio;
	u32 reset_gpio_flags;
#if defined CONFIG_TCT_8X26_EOS
/*[BUGFIX]-Add by TCTNB.XQJ,11/07/2013,FR-529341,for wakeup pin gpio problem */
    u32 wake_gpio; 
    u32 wake_gpio_flags;
/*[BUGFIX]-End by TCTNB.XQJ,*/
#endif
	u32 family_id;
	u32 x_max;
	u32 y_max;
	u32 x_min;
	u32 y_min;
	u32 panel_minx;
	u32 panel_miny;
	u32 panel_maxx;
	u32 panel_maxy;
	u32 group_id;
	u32 hard_rst_dly;
	u32 soft_rst_dly;
	u32 num_max_touches;
	bool fw_vkey_support;
	bool no_force_update;
	bool i2c_pull_up;
	bool ignore_id_check;
	int (*power_init) (bool);
	int (*power_on) (bool);
};

#endif

#define CONFIG_TOUCHSCREEN_FT5X06_FIRMWARE	1
#if defined CONFIG_TOUCHSCREEN_FT5X06_FIRMWARE
#define IC_FT5X06	0
#define IC_FT5606	1
#define IC_FT5316	2
#define IC_FT5X36	3

#define FT_UPGRADE_AA	0xAA
#define FT_UPGRADE_55 	0x55
#define FT_UPGRADE_EARSE_DELAY		2000

/*upgrade config of FT5606*/
#define FT5606_UPGRADE_AA_DELAY 		50
#define FT5606_UPGRADE_55_DELAY 		10
#define FT5606_UPGRADE_ID_1			0x79
#define FT5606_UPGRADE_ID_2			0x06
#define FT5606_UPGRADE_READID_DELAY 	100

/*upgrade config of FT5316*/
#define FT5316_UPGRADE_AA_DELAY 		50
#define FT5316_UPGRADE_55_DELAY 		40
#define FT5316_UPGRADE_ID_1			0x79
#define FT5316_UPGRADE_ID_2			0x07
#define FT5316_UPGRADE_READID_DELAY 	1

/*upgrade config of FT5x06(x=2,3,4)*/
#define FT5X06_UPGRADE_AA_DELAY 		50
#define FT5X06_UPGRADE_55_DELAY 		30
#define FT5X06_UPGRADE_ID_1			0x79
#define FT5X06_UPGRADE_ID_2			0x03
#define FT5X06_UPGRADE_READID_DELAY 	1

/*upgrade config of FT5X36*/
#define FT5X36_UPGRADE_AA_DELAY 		30
#define FT5X36_UPGRADE_55_DELAY 		30
#define FT5X36_UPGRADE_ID_1			0x79
#define FT5X36_UPGRADE_ID_2			0x11
#define FT5X36_UPGRADE_READID_DELAY 	10

#define DEVICE_IC_TYPE	IC_FT5X36

#define FTS_PACKET_LENGTH        128
#define FTS_SETTING_BUF_LEN        128

#define BL_VERSION_LZ4        0
#define BL_VERSION_Z7        1
#define BL_VERSION_GZF        2

#define FTS_TX_MAX				40
#define FTS_RX_MAX				40
#define FTS_DEVICE_MODE_REG	0x00
#define FTS_TXNUM_REG			0x03
#define FTS_RXNUM_REG			0x04
#define FTS_RAW_READ_REG		0x01
#define FTS_RAW_BEGIN_REG		0x10
#define FTS_VOLTAGE_REG		0x05

#define FTS_FACTORYMODE_VALUE		0x40
#define FTS_WORKMODE_VALUE		0x00

/*register address*/
#define FT5x0x_REG_FW_VER		0xA6
#define FT5x0x_REG_POINT_RATE	0x88
#define FT5X0X_REG_THGROUP	0x80

#define YEJI_VENDOR_TP          0x80
#define JUNDA_VENDOR_TP         0x85


#define FLIP_COVER_SWITCH
#define SYSFS_DEBUG
#define FTS_APK_DEBUG

/* [PLATFORM]-Mod-BGEIN by TCTNB.WPL,2013/11/29,refer to bug562523, for RIO6 TP FT5336 */
//#define AUTO_CLB
//#define FT_CAL_THROUGH_HOST
/* [PLATFORM]-Mod-BGEIN by TCTNB.WPL */

/*[BUGFIX]-Add-BEGIN by TCTNB.XQJ, 2013/12/05, refer to bug 564812 for tp upgrade*/
#define IS_TRULY_TP 0x5a
#define IS_BIEL_TP 0x3b
int fts_ctpm_fw_i_file_config(struct i2c_client *client);
int fts_ctpm_fw_upgrade_with_app_file(struct i2c_client *client, char *firmware_name);

int fts_ctpm_update_get_vid_fwid(struct i2c_client *client);
/*[BUGFIX]-Add-END by TCTNB.XQJ */
int fts_ctpm_update_project_setting(struct i2c_client *client);

int ft5x06_i2c_read(struct i2c_client *client, char *writebuf, int writelen, char *readbuf, int readlen);
int ft5x06_i2c_write(struct i2c_client *client, char *writebuf, int writelen);

int fts_ctpm_fw_upgrade_with_i_file(struct i2c_client *client);
u8 fts_ctpm_get_i_file_ver(void);
int fts_ctpm_auto_clb(struct i2c_client *client);
#ifdef FLIP_COVER_SWITCH
void fts_ctp_flip_cover_switch(bool on);
#endif

/*[BUGFIX]-Add Begin by TCTNB.WQF,2014/2/19, Call TP resume in LCD to make resume faster*/
#if defined(CONFIG_TCT_8X26_EOS) || defined(CONFIG_TCT_8X26_EOS_CUCC)
int ft5x06_ts_resume_by_lcd(void);
#endif
/*[BUGFIX]-Add End by TCTNB.WQF*/

/*create sysfs for debug*/
int ft5x0x_create_sysfs(struct i2c_client * client);
void ft5x0x_release_mutex(void);

#endif

