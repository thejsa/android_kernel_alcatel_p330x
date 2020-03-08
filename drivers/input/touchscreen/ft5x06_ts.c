/*
 *
 * FocalTech ft5x06 TouchScreen driver.
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

#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/firmware.h>
#include <linux/debugfs.h>
#include <linux/input/ft5x06_ts.h>
#ifdef FTS_APK_DEBUG
#include <linux/proc_fs.h>
#include <linux/netdevice.h>
#include <linux/syscalls.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <asm/unistd.h>
#endif

#if defined(CONFIG_FB)
#include <linux/notifier.h>
#include <linux/fb.h>

#elif defined(CONFIG_HAS_EARLYSUSPEND)
#include <linux/earlysuspend.h>
/* Early-suspend level */
#define FT_SUSPEND_LEVEL 1
#endif

#define FT_DRIVER_VERSION	0x02

#define FT_META_REGS		3
#define FT_ONE_TCH_LEN		6
#define FT_TCH_LEN(x)		(FT_META_REGS + FT_ONE_TCH_LEN * x)

#define FT_PRESS		0x7F
#define FT_MAX_ID		0x0F
#define FT_TOUCH_X_H_POS	3
#define FT_TOUCH_X_L_POS	4
#define FT_TOUCH_Y_H_POS	5
#define FT_TOUCH_Y_L_POS	6
#define FT_TD_STATUS		2
#define FT_TOUCH_EVENT_POS	3
#define FT_TOUCH_ID_POS		5
#define FT_TOUCH_DOWN		0
#define FT_TOUCH_CONTACT	2

/*register address*/
#define FT_REG_DEV_MODE		0x00
#define FT_DEV_MODE_REG_CAL	0x02
#define FT_REG_ID		0xA3
#define FT_REG_PMODE		0xA5
#define FT_REG_FW_VER		0xA6
#define FT_REG_POINT_RATE	0x88
#define FT_REG_THGROUP		0x80
#define FT_REG_ECC		0xCC
#define FT_REG_RESET_FW		0x07
#define FT_REG_FW_MAJ_VER	0xB1
#define FT_REG_FW_MIN_VER	0xB2
#define FT_REG_FW_SUB_MIN_VER	0xB3
#define FT_REG_HOST_CAL     0xB6
/*[BUGFIX]-Add-BEGIN by TCTNB.XQJ, 2013/12/19, refer to bug 551940 double click wake and unlock*/
#ifdef CONFIG_8X26_TP_GESTURE
#if defined(CONFIG_TCT_8X26_RIO6) || defined(CONFIG_TCT_8X26_RIO6_TF)
#define FT5X06_REG_GESTURE_SET    0xd0
#define FT5X06_REG_GESTURE_STATE    0xd3
#define  GESTURE_V 0x14
#define  GESTURE_DB 0x24
#define  GESTURE_C 0x18
#else
#define FT5X06_REG_GESTURE_SET    0x96
#define FT5X06_REG_GESTURE_STATE    0x01
#define  GESTURE_V 0x14
#define  GESTURE_DB 0x16
#define  GESTURE_C 0x18
#endif
#endif
/*[BUGFIX]-End by TCTNB.XQJ*/
/* power register bits*/
#define FT_PMODE_ACTIVE		0x00
#define FT_PMODE_MONITOR	0x01
#define FT_PMODE_STANDBY	0x02
#define FT_PMODE_HIBERNATE	0x03
#define FT_FACTORYMODE_VALUE	0x40
#define FT_WORKMODE_VALUE	0x00
#define FT_RST_CMD_REG1		0xFC
#define FT_RST_CMD_REG2		0xBC
#define FT_READ_ID_REG		0x90
#define FT_ERASE_APP_REG	0x61
#define FT_ERASE_PANEL_REG	0x63
#define FT_FW_START_REG		0xBF
#ifdef	FLIP_COVER_SWITCH
#define FT_REG_COVER_SWITCH	0xB3
#endif

#define FT_STATUS_NUM_TP_MASK	0x0F

#define FT_VTG_MIN_UV		2600000
#define FT_VTG_MAX_UV		3300000
#define FT_I2C_VTG_MIN_UV	1800000
#define FT_I2C_VTG_MAX_UV	1800000

#define FT_COORDS_ARR_SIZE	4
#define MAX_BUTTONS		4

#define FT_8BIT_SHIFT		8
#define FT_4BIT_SHIFT		4
#define FT_FW_NAME_MAX_LEN	50

#define FT5316_ID		0x0A
#define FT5306I_ID		0x55
#define FT6X06_ID		0x06

#define FT_UPGRADE_AA		0xAA
#define FT_UPGRADE_55		0x55

#define FT_FW_MIN_SIZE		8
#define FT_FW_MAX_SIZE		32768

/* Firmware file is not supporting minor and sub minor so use 0 */
#define FT_FW_FILE_MAJ_VER(x)	((x)->data[(x)->size - 2])
#define FT_FW_FILE_MIN_VER(x)	0
#define FT_FW_FILE_SUB_MIN_VER(x) 0

#define FT_FW_CHECK(x)		\
	(((x)->data[(x)->size - 8] ^ (x)->data[(x)->size - 6]) == 0xFF \
	&& (((x)->data[(x)->size - 7] ^ (x)->data[(x)->size - 5]) == 0xFF \
	&& (((x)->data[(x)->size - 3] ^ (x)->data[(x)->size - 4]) == 0xFF)))

#define FT_MAX_TRIES		5
#define FT_RETRY_DLY		20

#define FT_MAX_WR_BUF		10
#define FT_MAX_RD_BUF		2
#define FT_FW_PKT_LEN		128
#define FT_FW_PKT_META_LEN	6
#define FT_FW_PKT_DLY_MS	20
#define FT_FW_LAST_PKT		0x6ffa
#define FT_EARSE_DLY_MS		100
#define FT_55_AA_DLY_NS		5000

#define FT_UPGRADE_LOOP		30
#define FT_CAL_START		0x04
#define FT_CAL_FIN		0x00
#define FT_CAL_STORE		0x05
#define FT_CAL_RETRY		100
#define FT_REG_CAL		0x00
#define FT_CAL_MASK		0x70

#define FT_INFO_MAX_LEN		512

#define FT_BLOADER_SIZE_OFF	12
#define FT_BLOADER_NEW_SIZE	30
#define FT_DATA_LEN_OFF_OLD_FW	8
#define FT_DATA_LEN_OFF_NEW_FW	14
#define FT_FINISHING_PKT_LEN_OLD_FW	6
#define FT_FINISHING_PKT_LEN_NEW_FW	12
#define FT_MAGIC_BLOADER_Z7	0x7bfa
#define FT_MAGIC_BLOADER_LZ4	0x6ffa
#define FT_MAGIC_BLOADER_GZF_30	0x7ff4
#define FT_MAGIC_BLOADER_GZF	0x7bf4

enum {
	FT_BLOADER_VERSION_LZ4 = 0,
	FT_BLOADER_VERSION_Z7 = 1,
	FT_BLOADER_VERSION_GZF = 2,
};

enum {
	FT_FT5336_FAMILY_ID_0x11 = 0x11,
	FT_FT5336_FAMILY_ID_0x12 = 0x12,
	FT_FT5336_FAMILY_ID_0x13 = 0x13,
	FT_FT5336_FAMILY_ID_0x14 = 0x14,
};

#define FT_STORE_TS_INFO(buf, id, name, max_tch, group_id, fw_vkey_support, \
			fw_name, fw_maj, fw_min, fw_sub_min) \
			snprintf(buf, FT_INFO_MAX_LEN, \
				"controller\t= focaltech\n" \
				"model\t\t= 0x%x\n" \
				"name\t\t= %s\n" \
				"max_touches\t= %d\n" \
				"drv_ver\t\t= 0x%x\n" \
				"group_id\t= 0x%x\n" \
				"fw_vkey_support\t= %s\n" \
				"fw_name\t\t= %s\n" \
				"fw_ver\t\t= %d.%d.%d\n", id, name, \
				max_tch, FT_DRIVER_VERSION, group_id, \
				fw_vkey_support, fw_name, fw_maj, fw_min, \
				fw_sub_min)

#define FT_DEBUG_DIR_NAME	"ts_debug"

struct ft5x06_ts_data {
	struct i2c_client *client;
	struct input_dev *input_dev;
	const struct ft5x06_ts_platform_data *pdata;
	struct regulator *vdd;
	struct regulator *vcc_ldo5;
	struct regulator *vcc_i2c;
	char fw_name[FT_FW_NAME_MAX_LEN];
	bool loading_fw;
	u8 family_id;
	struct dentry *dir;
	u16 addr;
	bool suspended;
	char *ts_info;
	u8 *tch_data;
	u32 tch_data_len;
	u8 fw_ver[3];
#if defined(CONFIG_FB)
	struct notifier_block fb_notif;
#elif defined(CONFIG_HAS_EARLYSUSPEND)
	struct early_suspend early_suspend;
#endif
};

#ifdef FLIP_COVER_SWITCH
struct i2c_client *flip_cover_client;
int flip_cover_switch = 0;
int power_on = 0;
extern int global_tp_usable;
struct delayed_work flip_cover_on_wq;
struct delayed_work flip_cover_off_wq;
#endif

/*[BUGFIX]-Add-BEGIN by TCTNB.XQJ, 2013/12/19, refer to bug 551940 double click wake and unlock*/
#ifdef CONFIG_8X26_TP_GESTURE
#undef  KEY_POWER
#define KEY_POWER  KEY_UNLOCK
struct i2c_client  *ft_g_client;
#define DEBUG_TP_GESTURE
#ifdef DEBUG_TP_GESTURE
static int GestureINTCount=0;
static int GestureConfirmedCount=0;
#define TP_GESTURE_DBG(fmt, arg...) printk(KERN_ERR "[TP_GSTURE] %s: " fmt "\n", __func__, ## arg)
#else
#define TP_GESTURE_DBG(mt, arg...) do {} while (0)
#endif

struct tp_gesture_data{
    u8 option;
    u8 flag;
    u8 stateReg_val;
};

static struct  tp_gesture_data gesture={
    .option=0x00,
    .flag=0x00,
    .stateReg_val=0x00,
 };

struct class *gt_dclick_class;
struct device *gt_dclick_dev;
static ssize_t dbclick_switch_show ( struct device *dev,
                                      struct device_attribute *attr, char *buf )
{
    return sprintf ( buf, "%d\n", gesture.option);
}
static ssize_t dbclick_switch_store ( struct device *dev,
                                       struct device_attribute *attr, const char *buf, size_t count )
{
   struct ft5x06_ts_data *data;


#ifdef FEATURE_TCTNB_MMITEST
    return 0;
#endif

	data = (struct ft5x06_ts_data *)i2c_get_clientdata(ft_g_client);
    if (NULL == buf)
        return -EINVAL;
    if (data->suspended) //if in sleep state,return error
          return -EINVAL;
    if (0 == count)
        return 0;
    if( buf[0] == '0')
    {
        gesture.option = 0x00;//dblick_switch=0;
        device_init_wakeup(&ft_g_client->dev, 0);
        TP_GESTURE_DBG("=>gesture.option= %d\n",gesture.option);
    }
    else if(buf[0]=='1')
    {
        gesture.option = 0x01;
        device_init_wakeup(&ft_g_client->dev, 1);
        TP_GESTURE_DBG("=>gesture.option= %d\n",gesture.option);
    }
    else
    {
        pr_err("invalid  command! \n");
        return -1;
    }
    return count;
}
static DEVICE_ATTR(dbclick, 0664, dbclick_switch_show, dbclick_switch_store);

static void DBclick_attr_create(void)
{
    gt_dclick_class = class_create(THIS_MODULE, "TP-UNLOCK");
    if (IS_ERR(gt_dclick_class))
        pr_err("Failed to create class(gt_dclick_class)!\n");
    gt_dclick_dev = device_create(gt_dclick_class,
                                     NULL, 0, NULL, "device");
    if (IS_ERR(gt_dclick_dev))
        pr_err("Failed to create device(gt_dclick_dev)!\n");

       // update
    if (device_create_file(gt_dclick_dev, &dev_attr_dbclick) < 0)
        pr_err("Failed to create device file(%s)!\n", dev_attr_dbclick.attr.name);
}
#endif
/*[BUGFIX]-Add-BEGIN by TCTNB.XQJ*/

#if defined CONFIG_TOUCHSCREEN_FT5X06_FIRMWARE
/*[BUGFIX]-Add-BEGIN by TCTNB.XQJ, 2013/12/05, refer to bug 564812 for tp upgrade*/
int TP_VENDOR = 0;
/*[BUGFIX]-Add-BEGIN by TCTNB.XQJ*/
int ft5x06_i2c_read(struct i2c_client *client, char *writebuf,
			   int writelen, char *readbuf, int readlen)
#else
static int ft5x06_i2c_read(struct i2c_client *client, char *writebuf,
			   int writelen, char *readbuf, int readlen)
#endif
{
	int ret;

	if (writelen > 0) {
		struct i2c_msg msgs[] = {
			{
				 .addr = client->addr,
				 .flags = 0,
				 .len = writelen,
				 .buf = writebuf,
			 },
			{
				 .addr = client->addr,
				 .flags = I2C_M_RD,
				 .len = readlen,
				 .buf = readbuf,
			 },
		};
		ret = i2c_transfer(client->adapter, msgs, 2);
		if (ret < 0)
			dev_err(&client->dev, "%s: i2c read error.\n",
				__func__);
	} else {
		struct i2c_msg msgs[] = {
			{
				 .addr = client->addr,
				 .flags = I2C_M_RD,
				 .len = readlen,
				 .buf = readbuf,
			 },
		};
		ret = i2c_transfer(client->adapter, msgs, 1);
		if (ret < 0)
			dev_err(&client->dev, "%s:i2c read error.\n", __func__);
	}
	return ret;
}

#if defined CONFIG_TOUCHSCREEN_FT5X06_FIRMWARE
int ft5x06_i2c_write(struct i2c_client *client, char *writebuf,
			    int writelen)
#else
static int ft5x06_i2c_write(struct i2c_client *client, char *writebuf,
			    int writelen)
#endif
{
	int ret;

	struct i2c_msg msgs[] = {
		{
			 .addr = client->addr,
			 .flags = 0,
			 .len = writelen,
			 .buf = writebuf,
		 },
	};
	ret = i2c_transfer(client->adapter, msgs, 1);
	if (ret < 0)
		dev_err(&client->dev, "%s: i2c write error.\n", __func__);

	return ret;
}

static int ft5x0x_write_reg(struct i2c_client *client, u8 addr, const u8 val)
{
	u8 buf[2] = {0};

	buf[0] = addr;
	buf[1] = val;

	return ft5x06_i2c_write(client, buf, sizeof(buf));
}

static int ft5x0x_read_reg(struct i2c_client *client, u8 addr, u8 *val)
{
	return ft5x06_i2c_read(client, &addr, 1, val, 1);
}

static void ft5x06_update_fw_ver(struct ft5x06_ts_data *data)
{
	struct i2c_client *client = data->client;
	u8 reg_addr;
	int err;

	reg_addr = FT_REG_FW_MAJ_VER;
	err = ft5x06_i2c_read(client, &reg_addr, 1, &data->fw_ver[0], 1);
	if (err < 0)
		dev_err(&client->dev, "fw major version read failed");

	reg_addr = FT_REG_FW_MIN_VER;
	err = ft5x06_i2c_read(client, &reg_addr, 1, &data->fw_ver[1], 1);
	if (err < 0)
		dev_err(&client->dev, "fw minor version read failed");

	reg_addr = FT_REG_FW_SUB_MIN_VER;
	err = ft5x06_i2c_read(client, &reg_addr, 1, &data->fw_ver[2], 1);
	if (err < 0)
		dev_err(&client->dev, "fw sub minor version read failed");

	dev_info(&client->dev, "Firmware version = %d.%d.%d\n",
		data->fw_ver[0], data->fw_ver[1], data->fw_ver[2]);
}

static irqreturn_t ft5x06_ts_interrupt(int irq, void *dev_id)
{
	struct ft5x06_ts_data *data = dev_id;
	struct input_dev *ip_dev;
	int rc, i;
	u32 id, x, y, pressure, status, num_touches;
	u8 reg = 0x00, *buf;
	int fingerdown = 0;
#ifdef CONFIG_8X26_TP_GESTURE /*[BUGFIX]-Add  by TCTNB.XQJ, 2013/12/19, refer to bug 551940 double click wake and unlock*/
      u8 reg_value;
#endif
	if (!data) {
		pr_err("%s: Invalid data\n", __func__);
		return IRQ_HANDLED;
	}
/*[BUGFIX]-Add-BEGIN by TCTNB.XQJ, 2013/12/19, refer to bug 551940 double click wake and unlock*/
#ifdef CONFIG_8X26_TP_GESTURE
    if((0x01==gesture.option)&&(0x01==gesture.flag))//gesture INT
    {
#ifdef DEBUG_TP_GESTURE
        GestureINTCount++;
        TP_GESTURE_DBG("GestureINTCount = %d\n",GestureINTCount);
#endif
        //msleep(5);
         reg = FT5X06_REG_GESTURE_STATE;
        rc = ft5x06_i2c_read(data->client, &reg, 1, &reg_value, 1);
        if(GESTURE_DB == reg_value)
        {
            gesture.stateReg_val=reg_value;
            input_report_key(data->input_dev, KEY_POWER, 1);
            input_report_key(data->input_dev, KEY_POWER, 0);
            input_sync(data->input_dev);
#ifdef DEBUG_TP_GESTURE
            GestureConfirmedCount++;
            TP_GESTURE_DBG("GestureConfirmedCount = %d\n",GestureConfirmedCount);
            TP_GESTURE_DBG("reg_addr 0x01= 0x%04x\n",reg_value);
#endif
        }
        else
        {
            pr_err("=>error  reg_addr 0x01= 0x%04x\n",reg_value);
        }
         return IRQ_HANDLED;
      }
#endif
/*[BUGFIX]-Add-End  by TCTNB.XQJ*/
	ip_dev = data->input_dev;
	buf = data->tch_data;

	rc = ft5x06_i2c_read(data->client, &reg, 1,
			buf, data->tch_data_len);
	if (rc < 0) {
		dev_err(&data->client->dev, "%s: read data fail\n", __func__);
		return IRQ_HANDLED;
	}

	for (i = 0; i < data->pdata->num_max_touches; i++) {
		id = (buf[FT_TOUCH_ID_POS + FT_ONE_TCH_LEN * i]) >> 4;
		if (id >= FT_MAX_ID)
			break;
                //modify  by jackywei wei.biao.sz @tcl.com [SW] Bug#: 672729
		x = (buf[FT_TOUCH_X_H_POS + FT_ONE_TCH_LEN * i] & 0x0F) << 8 |
			(buf[FT_TOUCH_X_L_POS + FT_ONE_TCH_LEN * i]);
		y = (buf[FT_TOUCH_Y_H_POS + FT_ONE_TCH_LEN * i] & 0x0F) << 8 |
			(buf[FT_TOUCH_Y_L_POS + FT_ONE_TCH_LEN * i]);

		status = buf[FT_TOUCH_EVENT_POS + FT_ONE_TCH_LEN * i] >> 6;

		num_touches = buf[FT_TD_STATUS] & FT_STATUS_NUM_TP_MASK;

		//pr_err("x=<%d>, y=<%d>, status=<%d>, num_touches=<%d>, id=<%d>\n", x, y, status, num_touches, id);

		if (status == FT_TOUCH_DOWN || status == FT_TOUCH_CONTACT) {
			pressure = FT_PRESS;
			fingerdown++;
		} else {
			pressure = 0;
		}
	
		//modify by rongxiao.deng
                //delete  by jackywei wei.biao.sz @tcl.com [SW] Bug#: 672729
		//y = 600 -y;
		input_report_abs(ip_dev, ABS_MT_TRACKING_ID, id);
		input_report_abs(ip_dev, ABS_MT_POSITION_X, x);// exchange by rongxiao.deng
		input_report_abs(ip_dev, ABS_MT_POSITION_Y, y);
		//modify end
		input_report_abs(ip_dev, ABS_MT_PRESSURE, pressure);
		input_report_abs(ip_dev, ABS_MT_TOUCH_MAJOR, pressure);
		input_mt_sync(ip_dev);
		//pr_err("x=<%d>,y=<%d>, pressure=<%d>\n",x,y,pressure);//temp
	}

	input_report_key(ip_dev, BTN_TOUCH, !!fingerdown);
	input_sync(ip_dev);

	return IRQ_HANDLED;
}

static int ft5x06_power_on(struct ft5x06_ts_data *data, bool on)
{
	int rc;

	if (!on)
		goto power_off;

	rc = regulator_enable(data->vdd);
	if (rc) {
		dev_err(&data->client->dev,
			"Regulator vdd enable failed rc=%d\n", rc);
		return rc;
	}

#if 1
	rc = regulator_enable(data->vcc_ldo5);
	if (rc) {
		dev_err(&data->client->dev,
			"Regulator vdd enable failed rc=%d\n", rc);
		return rc;
	}
#endif

	rc = regulator_enable(data->vcc_i2c);
	if (rc) {
		dev_err(&data->client->dev,
			"Regulator vcc_i2c enable failed rc=%d\n", rc);
		regulator_disable(data->vdd);
	}
	power_on = 1;
	return rc;

power_off:
	rc = regulator_disable(data->vdd);
	if (rc) {
		dev_err(&data->client->dev,
			"Regulator vdd disable failed rc=%d\n", rc);
		return rc;
	}

#if 1
	rc = regulator_disable(data->vcc_ldo5);
	if (rc) {
		dev_err(&data->client->dev,
			"Regulator vdd disable failed rc=%d\n", rc);
		return rc;
	}
#endif

	rc = regulator_disable(data->vcc_i2c);
	if (rc) {
		dev_err(&data->client->dev,
			"Regulator vcc_i2c disable failed rc=%d\n", rc);
		regulator_enable(data->vdd);
	}

	power_on = 0;
	return rc;
}

static int ft5x06_power_init(struct ft5x06_ts_data *data, bool on)
{
	int rc;

	if (!on)
		goto pwr_deinit;

	data->vdd = regulator_get(&data->client->dev, "vdd");
	if (IS_ERR(data->vdd)) {
		rc = PTR_ERR(data->vdd);
		dev_err(&data->client->dev,
			"Regulator get failed vdd rc=%d\n", rc);
		return rc;
	}

	if (regulator_count_voltages(data->vdd) > 0) {
		rc = regulator_set_voltage(data->vdd, FT_VTG_MIN_UV,
					   FT_VTG_MAX_UV);
		if (rc) {
			dev_err(&data->client->dev,
				"Regulator set_vtg failed vdd rc=%d\n", rc);
			goto reg_vdd_put;
		}
	}

	rc = regulator_set_optimum_mode(data->vdd, 200000);
	if(rc)
		printk(KERN_ERR "set optimum mode failed\n");

	data->vcc_i2c = regulator_get(&data->client->dev, "vcc_i2c");
	if (IS_ERR(data->vcc_i2c)) {
		rc = PTR_ERR(data->vcc_i2c);
		dev_err(&data->client->dev,
			"Regulator get failed vcc_i2c rc=%d\n", rc);
		goto reg_vdd_set_vtg;
	}

	if (regulator_count_voltages(data->vcc_i2c) > 0) {
		rc = regulator_set_voltage(data->vcc_i2c, FT_I2C_VTG_MIN_UV,
					   FT_I2C_VTG_MAX_UV);
		if (rc) {
			dev_err(&data->client->dev,
			"Regulator set_vtg failed vcc_i2c rc=%d\n", rc);
			goto reg_vcc_i2c_put;
		}
	}

	data->vcc_ldo5 = regulator_get(&data->client->dev, "vcc_ldo5");
	if (IS_ERR(data->vcc_ldo5)) {
		rc = PTR_ERR(data->vcc_ldo5);
		dev_err(&data->client->dev,
			"Regulator get failed vcc_ldo5 rc=%d\n", rc);
		goto reg_vdd_set_vtg;
	}


	if (regulator_count_voltages(data->vcc_ldo5) > 0) {
		rc = regulator_set_voltage(data->vcc_ldo5, 1200000,
					   1200000);
		if (rc) {
			dev_err(&data->client->dev,
			"Regulator set_vtg failed vcc_i2c rc=%d\n", rc);
			goto reg_vcc_i2c_put;
		}
	}

	return 0;

reg_vcc_i2c_put:
	regulator_put(data->vcc_i2c);
reg_vdd_set_vtg:
	if (regulator_count_voltages(data->vdd) > 0)
		regulator_set_voltage(data->vdd, 0, FT_VTG_MAX_UV);
reg_vdd_put:
	regulator_put(data->vdd);
	return rc;

pwr_deinit:
	if (regulator_count_voltages(data->vdd) > 0)
		regulator_set_voltage(data->vdd, 0, FT_VTG_MAX_UV);

	regulator_put(data->vdd);

	if (regulator_count_voltages(data->vcc_i2c) > 0)
		regulator_set_voltage(data->vcc_i2c, 0, FT_I2C_VTG_MAX_UV);
	regulator_put(data->vcc_i2c);
	return 0;
}

#ifdef CONFIG_PM
static int ft5x06_ts_suspend(struct device *dev)
{
	struct ft5x06_ts_data *data = dev_get_drvdata(dev);
 /*[BUGFIX]-Mod Begin by TCTNB.WPL,2013/11/16, bug556002*/
	char txbuf[2];
 /*[BUGFIX]-Mod End by TCTNB.WPL,2013/11/16*/

	int err;

	if (data->loading_fw) {
		dev_info(dev, "Firmware loading in process...\n");
		return 0;
	}

	if (data->suspended) {
		dev_info(dev, "Already in suspend state\n");
		return 0;
	}
/*[BUGFIX]-Add-BEGIN by TCTNB.XQJ, 2013/12/19, refer to bug 551940 double click wake and unlock*/
#ifdef CONFIG_8X26_TP_GESTURE
    if(gesture.option==0)
        disable_irq(data->client->irq);//double click is not set,
#else
	disable_irq(data->client->irq);
#endif
 /*[BUGFIX]-Mod Begin by TCTNB.WPL,2013/11/16, bug556002*/
#if 0
	/* release all touches */
	for (i = 0; i < data->pdata->num_max_touches; i++) {
		input_mt_slot(data->input_dev, i);
		input_mt_report_slot_state(data->input_dev, MT_TOOL_FINGER, 0);
	}
#endif
	input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR, 0); //release point
 /*[BUGFIX]-Mod End by TCTNB.WPL,2013/11/16*/
	input_mt_report_pointer_emulation(data->input_dev, false);
	input_sync(data->input_dev);
#ifdef CONFIG_8X26_TP_GESTURE
    if(gesture.option==1)//double click is set,it need wake up by double click,it need other setting
    {
        gesture.flag=0x01;
        txbuf[0] = FT5X06_REG_GESTURE_SET;
        txbuf[1] = 0x55;// enable tp gesture
        ft5x06_i2c_write(data->client, txbuf, sizeof(txbuf));
        gesture.flag=0x01;
        TP_GESTURE_DBG("=>enter gesture mode ok!! \n\n");

        if (device_may_wakeup(dev))
        {
            err=enable_irq_wake(data->client->irq);
            TP_GESTURE_DBG("=> enable_irq_wake  data->client->irq= %d \n",data->client->irq);
            TP_GESTURE_DBG("=> enable_irq_wake return= %d \n",err);
        }
          data->suspended = true;
          return err ;
       }
#endif
/*[BUGFIX]-End by TCTNB.XQJ*/
	if (gpio_is_valid(data->pdata->reset_gpio)) {
		txbuf[0] = FT_REG_PMODE;
		txbuf[1] = FT_PMODE_HIBERNATE;
		ft5x06_i2c_write(data->client, txbuf, sizeof(txbuf));
	}

	if (data->pdata->power_on) {
		err = data->pdata->power_on(false);
		if (err) {
			dev_err(dev, "power off failed");
			goto pwr_off_fail;
		}
	} else {
		err = ft5x06_power_on(data, false);
		if (err) {
			dev_err(dev, "power off failed");
			goto pwr_off_fail;
		}
	}

	data->suspended = true;

	return 0;

pwr_off_fail:
	if (gpio_is_valid(data->pdata->reset_gpio)) {
		gpio_set_value_cansleep(data->pdata->reset_gpio, 0);
		msleep(data->pdata->hard_rst_dly);
		gpio_set_value_cansleep(data->pdata->reset_gpio, 1);
	}
	enable_irq(data->client->irq);
	return err;
}

static int ft5x06_ts_resume(struct device *dev)
{
	struct ft5x06_ts_data *data = dev_get_drvdata(dev);
	int err;
#ifdef CONFIG_8X26_TP_GESTURE /*[BUGFIX]-Add by TCTNB.XQJ, 2013/12/19, refer to bug 551940 double click wake and unlock*/
    char txbuf[2];
#endif
	if (!data->suspended) {
		dev_dbg(dev, "Already in awake state\n");
		return 0;
	}
/*[BUGFIX]-Add-BEGIN by TCTNB.XQJ, 2013/12/19, refer to bug 551940 double click wake and unlock*/
#ifdef CONFIG_8X26_TP_GESTURE
    if(gesture.option==1)//double click is set,it need wake up by double click,it need other setting
    {
        gesture.flag=0x00;
        txbuf[0] = FT5X06_REG_GESTURE_SET;
        txbuf[1] = 0x00; //disable gesture
        ft5x06_i2c_write(data->client, txbuf, sizeof(txbuf));
        gesture.flag = 0x00;//clean flag
        TP_GESTURE_DBG("=>exit gesture mode!Set flag =%d \n",gesture.flag);
        data->suspended = false;
        msleep(100);
		return 0 ;
     }
#endif
/*[BUGFIX]-End by TCTNB.XQJ*/

	if (data->pdata->power_on) {
		err = data->pdata->power_on(true);
		if (err) {
			dev_err(dev, "power on failed");
			return err;
		}
	} else {
		err = ft5x06_power_on(data, true);
		if (err) {
			dev_err(dev, "power on failed");
			return err;
		}
	}

	if (gpio_is_valid(data->pdata->reset_gpio)) {
		gpio_set_value_cansleep(data->pdata->reset_gpio, 0);
		msleep(data->pdata->hard_rst_dly);
		gpio_set_value_cansleep(data->pdata->reset_gpio, 1);
	}

	msleep(data->pdata->soft_rst_dly);
	enable_irq(data->client->irq);

	data->suspended = false;
 /*[BUGFIX]-Mod Begin by TCTNB.WPL,2013/11/15, bug549728, modify by FAE*/
	msleep(250);
/*[BUGFIX]-Mod-END by TCTNB.WPL*/
#ifdef FLIP_COVER_SWITCH
	fts_ctp_flip_cover_switch((bool)flip_cover_switch);
#endif

	return 0;
}

#if defined(CONFIG_FB)
static int fb_notifier_callback(struct notifier_block *self,
				 unsigned long event, void *data)
{
	struct fb_event *evdata = data;
	int *blank;
	struct ft5x06_ts_data *ft5x06_data =
		container_of(self, struct ft5x06_ts_data, fb_notif);

	if (evdata && evdata->data && event == FB_EVENT_BLANK &&
			ft5x06_data && ft5x06_data->client) {
		blank = evdata->data;
		if (*blank == FB_BLANK_UNBLANK)
			ft5x06_ts_resume(&ft5x06_data->client->dev);
		else if (*blank == FB_BLANK_POWERDOWN)
			ft5x06_ts_suspend(&ft5x06_data->client->dev);
	}

	return 0;
}
#elif defined(CONFIG_HAS_EARLYSUSPEND)
static void ft5x06_ts_early_suspend(struct early_suspend *handler)
{
	struct ft5x06_ts_data *data = container_of(handler,
						   struct ft5x06_ts_data,
						   early_suspend);

	ft5x06_ts_suspend(&data->client->dev);
}

static void ft5x06_ts_late_resume(struct early_suspend *handler)
{
	struct ft5x06_ts_data *data = container_of(handler,
						   struct ft5x06_ts_data,
						   early_suspend);

	ft5x06_ts_resume(&data->client->dev);
}
#endif

static const struct dev_pm_ops ft5x06_ts_pm_ops = {
#if (!defined(CONFIG_FB) && !defined(CONFIG_HAS_EARLYSUSPEND))
	.suspend = ft5x06_ts_suspend,
	.resume = ft5x06_ts_resume,
#endif
};
#endif

static int ft5x06_auto_cal(struct i2c_client *client)
{
	struct ft5x06_ts_data *data = i2c_get_clientdata(client);
	u8 temp = 0, i;

	/* set to factory mode */
	msleep(2 * data->pdata->soft_rst_dly);
	ft5x0x_write_reg(client, FT_REG_DEV_MODE, FT_FACTORYMODE_VALUE);
	msleep(data->pdata->soft_rst_dly);

	/* start calibration */
	ft5x0x_write_reg(client, FT_DEV_MODE_REG_CAL, FT_CAL_START);
	msleep(2 * data->pdata->soft_rst_dly);
	for (i = 0; i < FT_CAL_RETRY; i++) {
		ft5x0x_read_reg(client, FT_REG_CAL, &temp);
		/*return to normal mode, calibration finish */
		if (((temp & FT_CAL_MASK) >> FT_4BIT_SHIFT) == FT_CAL_FIN)
			break;
	}

	/*calibration OK */
	msleep(2 * data->pdata->soft_rst_dly);
	ft5x0x_write_reg(client, FT_REG_DEV_MODE, FT_FACTORYMODE_VALUE);
	msleep(data->pdata->soft_rst_dly);

	/* store calibration data */
	ft5x0x_write_reg(client, FT_DEV_MODE_REG_CAL, FT_CAL_STORE);
	msleep(2 * data->pdata->soft_rst_dly);

	/* set to normal mode */
	ft5x0x_write_reg(client, FT_REG_DEV_MODE, FT_WORKMODE_VALUE);
	msleep(2 * data->pdata->soft_rst_dly);

	return 0;
}

static int ft5x06_fw_upgrade_start(struct i2c_client *client,
			const u8 *data, u32 data_len)
{
	struct ft5x06_ts_data *ts_data = i2c_get_clientdata(client);
	struct fw_upgrade_info info = ts_data->pdata->info;
	u8 reset_reg;
	u8 w_buf[FT_MAX_WR_BUF] = {0}, r_buf[FT_MAX_RD_BUF] = {0};
	u8 pkt_buf[FT_FW_PKT_LEN + FT_FW_PKT_META_LEN];
	int i, j, temp;
	u32 pkt_num, pkt_len;
	u8 is_5336_new_bootloader = false;
	u8 is_5336_fwsize_30 = false;
	u8 fw_ecc;

	/* determine firmware size */
	if (*(data + data_len - FT_BLOADER_SIZE_OFF) == FT_BLOADER_NEW_SIZE)
		is_5336_fwsize_30 = true;
	else
		is_5336_fwsize_30 = false;

	for (i = 0, j = 0; i < FT_UPGRADE_LOOP; i++) {
		msleep(FT_EARSE_DLY_MS);
		/* reset - write 0xaa and 0x55 to reset register */
		if (ts_data->family_id == FT6X06_ID)
			reset_reg = FT_RST_CMD_REG2;
		else
			reset_reg = FT_RST_CMD_REG1;

		ft5x0x_write_reg(client, reset_reg, FT_UPGRADE_AA);
		msleep(info.delay_aa);

		ft5x0x_write_reg(client, reset_reg, FT_UPGRADE_55);
		if (i <= (FT_UPGRADE_LOOP / 2))
			msleep(info.delay_55 + i * 3);
		else
			msleep(info.delay_55 - (i - (FT_UPGRADE_LOOP / 2)) * 2);

		/* Enter upgrade mode */
		w_buf[0] = FT_UPGRADE_55;
		ft5x06_i2c_write(client, w_buf, 1);
		usleep(FT_55_AA_DLY_NS);
		w_buf[0] = FT_UPGRADE_AA;
		ft5x06_i2c_write(client, w_buf, 1);

		/* check READ_ID */
		msleep(info.delay_readid);
		w_buf[0] = FT_READ_ID_REG;
		w_buf[1] = 0x00;
		w_buf[2] = 0x00;
		w_buf[3] = 0x00;

		ft5x06_i2c_read(client, w_buf, 4, r_buf, 2);

		if (r_buf[0] != info.upgrade_id_1
			|| r_buf[1] != info.upgrade_id_2) {
			dev_err(&client->dev, "Upgrade ID mismatch(%d), IC=0x%x 0x%x, info=0x%x 0x%x\n",
				i, r_buf[0], r_buf[1],
				info.upgrade_id_1, info.upgrade_id_2);
		} else
			break;
	}

	if (i >= FT_UPGRADE_LOOP) {
		dev_err(&client->dev, "Abort upgrade\n");
		return -EIO;
	}

	w_buf[0] = 0xcd;
	ft5x06_i2c_read(client, w_buf, 1, r_buf, 1);

	if (r_buf[0] <= 4)
		is_5336_new_bootloader = FT_BLOADER_VERSION_LZ4;
	else if (r_buf[0] == 7)
		is_5336_new_bootloader = FT_BLOADER_VERSION_Z7;
	else if (r_buf[0] >= 0x0f &&
		((ts_data->family_id == FT_FT5336_FAMILY_ID_0x11) ||
		(ts_data->family_id == FT_FT5336_FAMILY_ID_0x12) ||
		(ts_data->family_id == FT_FT5336_FAMILY_ID_0x13) ||
		(ts_data->family_id == FT_FT5336_FAMILY_ID_0x14)))
		is_5336_new_bootloader = FT_BLOADER_VERSION_GZF;
	else
		is_5336_new_bootloader = FT_BLOADER_VERSION_LZ4;

	dev_dbg(&client->dev, "bootloader type=%d, r_buf=0x%x, family_id=0x%x\n",
		is_5336_new_bootloader, r_buf[0], ts_data->family_id);
	/* is_5336_new_bootloader = FT_BLOADER_VERSION_GZF; */

	/* erase app and panel paramenter area */
	w_buf[0] = FT_ERASE_APP_REG;
	ft5x06_i2c_write(client, w_buf, 1);
	msleep(info.delay_erase_flash);

	if (is_5336_fwsize_30) {
		w_buf[0] = FT_ERASE_PANEL_REG;
		ft5x06_i2c_write(client, w_buf, 1);
	}
	msleep(FT_EARSE_DLY_MS);

	/* program firmware */
	if (is_5336_new_bootloader == FT_BLOADER_VERSION_LZ4
		|| is_5336_new_bootloader == FT_BLOADER_VERSION_Z7)
		data_len = data_len - FT_DATA_LEN_OFF_OLD_FW;
	else
		data_len = data_len - FT_DATA_LEN_OFF_NEW_FW;

	pkt_num = (data_len) / FT_FW_PKT_LEN;
	pkt_len = FT_FW_PKT_LEN;
	pkt_buf[0] = FT_FW_START_REG;
	pkt_buf[1] = 0x00;
	fw_ecc = 0;

	for (i = 0; i < pkt_num; i++) {
		temp = i * FT_FW_PKT_LEN;
		pkt_buf[2] = (u8) (temp >> FT_8BIT_SHIFT);
		pkt_buf[3] = (u8) temp;
		pkt_buf[4] = (u8) (pkt_len >> FT_8BIT_SHIFT);
		pkt_buf[5] = (u8) pkt_len;

		for (j = 0; j < FT_FW_PKT_LEN; j++) {
			pkt_buf[6 + j] = data[i * FT_FW_PKT_LEN + j];
			fw_ecc ^= pkt_buf[6 + j];
		}

		ft5x06_i2c_write(client, pkt_buf,
				FT_FW_PKT_LEN + FT_FW_PKT_META_LEN);
		msleep(FT_FW_PKT_DLY_MS);
	}

	/* send remaining bytes */
	if ((data_len) % FT_FW_PKT_LEN > 0) {
		temp = pkt_num * FT_FW_PKT_LEN;
		pkt_buf[2] = (u8) (temp >> FT_8BIT_SHIFT);
		pkt_buf[3] = (u8) temp;
		temp = (data_len) % FT_FW_PKT_LEN;
		pkt_buf[4] = (u8) (temp >> FT_8BIT_SHIFT);
		pkt_buf[5] = (u8) temp;

		for (i = 0; i < temp; i++) {
			pkt_buf[6 + i] = data[pkt_num * FT_FW_PKT_LEN + i];
			fw_ecc ^= pkt_buf[6 + i];
		}

		ft5x06_i2c_write(client, pkt_buf, temp + FT_FW_PKT_META_LEN);
		msleep(FT_FW_PKT_DLY_MS);
	}

	/* send the finishing packet */
	if (is_5336_new_bootloader == FT_BLOADER_VERSION_LZ4 ||
		is_5336_new_bootloader == FT_BLOADER_VERSION_Z7) {
		for (i = 0; i < FT_FINISHING_PKT_LEN_OLD_FW; i++) {
			if (is_5336_new_bootloader  == FT_BLOADER_VERSION_Z7)
				temp = FT_MAGIC_BLOADER_Z7 + i;
			else if (is_5336_new_bootloader ==
						FT_BLOADER_VERSION_LZ4)
				temp = FT_MAGIC_BLOADER_LZ4 + i;
			pkt_buf[2] = (u8)(temp >> 8);
			pkt_buf[3] = (u8)temp;
			temp = 1;
			pkt_buf[4] = (u8)(temp >> 8);
			pkt_buf[5] = (u8)temp;
			pkt_buf[6] = data[data_len + i];
			fw_ecc ^= pkt_buf[6];

			ft5x06_i2c_write(client,
				pkt_buf, temp + FT_FW_PKT_META_LEN);
			msleep(FT_FW_PKT_DLY_MS);
		}
	} else if (is_5336_new_bootloader == FT_BLOADER_VERSION_GZF) {
		for (i = 0; i < FT_FINISHING_PKT_LEN_NEW_FW; i++) {
			if (is_5336_fwsize_30)
				temp = FT_MAGIC_BLOADER_GZF_30 + i;
			else
				temp = FT_MAGIC_BLOADER_GZF + i;
			pkt_buf[2] = (u8)(temp >> 8);
			pkt_buf[3] = (u8)temp;
			temp = 1;
			pkt_buf[4] = (u8)(temp >> 8);
			pkt_buf[5] = (u8)temp;
			pkt_buf[6] = data[data_len + i];
			fw_ecc ^= pkt_buf[6];

			ft5x06_i2c_write(client,
				pkt_buf, temp + FT_FW_PKT_META_LEN);
			msleep(FT_FW_PKT_DLY_MS);

		}
	}

	/* verify checksum */
	w_buf[0] = FT_REG_ECC;
	ft5x06_i2c_read(client, w_buf, 1, r_buf, 1);
	if (r_buf[0] != fw_ecc) {
		dev_err(&client->dev, "ECC error! dev_ecc=%02x fw_ecc=%02x\n",
					r_buf[0], fw_ecc);
		return -EIO;
	}

	/* reset */
	w_buf[0] = FT_REG_RESET_FW;
	ft5x06_i2c_write(client, w_buf, 1);
	msleep(ts_data->pdata->soft_rst_dly);

	dev_info(&client->dev, "Firmware upgrade successful\n");

	return 0;
}

static int ft5x06_fw_upgrade(struct device *dev, bool force)
{
	struct ft5x06_ts_data *data = dev_get_drvdata(dev);
	const struct firmware *fw = NULL;
	int rc;
	u8 fw_file_maj, fw_file_min, fw_file_sub_min;
	bool fw_upgrade = false;

	rc = request_firmware(&fw, data->fw_name, dev);
	if (rc < 0) {
		dev_err(dev, "Request firmware failed - %s (%d)\n",
						data->fw_name, rc);
		return rc;
	}

	if (fw->size < FT_FW_MIN_SIZE || fw->size > FT_FW_MAX_SIZE) {
		dev_err(dev, "Invalid firmware size (%d)\n", fw->size);
		rc = -EIO;
		goto rel_fw;
	}

	fw_file_maj = FT_FW_FILE_MAJ_VER(fw);
	fw_file_min = FT_FW_FILE_MIN_VER(fw);
	fw_file_sub_min = FT_FW_FILE_SUB_MIN_VER(fw);

	dev_info(dev, "Current firmware: %d.%d.%d", data->fw_ver[0],
				data->fw_ver[1], data->fw_ver[2]);
	dev_info(dev, "New firmware: %d.%d.%d", fw_file_maj,
				fw_file_min, fw_file_sub_min);

	if (force) {
		fw_upgrade = true;
	} else if (data->fw_ver[0] == fw_file_maj) {
			if (data->fw_ver[1] < fw_file_min)
				fw_upgrade = true;
			else if (data->fw_ver[2] < fw_file_sub_min)
				fw_upgrade = true;
			else
				dev_info(dev, "No need to upgrade\n");
	} else
		dev_info(dev, "Firmware versions do not match\n");

	if (!fw_upgrade) {
		dev_info(dev, "Exiting fw upgrade...\n");
		rc = -EFAULT;
		goto rel_fw;
	}

	/* start firmware upgrade */
	if (FT_FW_CHECK(fw)) {
		rc = ft5x06_fw_upgrade_start(data->client, fw->data, fw->size);
		if (rc < 0)
			dev_err(dev, "update failed (%d). try later...\n", rc);
		else if (data->pdata->info.auto_cal)
			ft5x06_auto_cal(data->client);
	} else {
		dev_err(dev, "FW format error\n");
		rc = -EIO;
	}

	ft5x06_update_fw_ver(data);

	FT_STORE_TS_INFO(data->ts_info, data->family_id, data->pdata->name,
			data->pdata->num_max_touches, data->pdata->group_id,
			data->pdata->fw_vkey_support ? "yes" : "no",
			data->pdata->fw_name, data->fw_ver[0],
			data->fw_ver[1], data->fw_ver[2]);
rel_fw:
	release_firmware(fw);
	return rc;
}

int fts_ctpm_auto_upg(struct i2c_client *client)
{
        unsigned char uc_host_fm_ver;
        unsigned char uc_tp_fm_ver;
        unsigned char version_list_junda_vendor[] = {0x00, 0x11, 0x12};  //Keep the junda vendor old version list that allows to be updated.
        unsigned char version_list_yeji_vendor[] = {0x00, 0x60, 0x62, 0x63, 0x64};  //Keep the yeji vendor old version list that allows to be updated.
        int i,i_ret = -1;

        ft5x0x_read_reg(client, FT_REG_FW_VER, &uc_tp_fm_ver);
        if(0 == uc_tp_fm_ver)
        {
                printk("ft5x0x_read uc_tp_fm_ver failed\n");
                //return -1;
        }
        uc_host_fm_ver = fts_ctpm_get_i_file_ver();
        printk("Firmware version in host:0x%x\n", uc_host_fm_ver);

        switch(TP_VENDOR){
                        case JUNDA_VENDOR_TP:
				printk("JUNDA VENDOR's TP firmware upgrade \n");
                                for (i = 0; i < sizeof(version_list_junda_vendor)/sizeof(version_list_junda_vendor[0]); i++)
                                {
                                        if (uc_tp_fm_ver == version_list_junda_vendor[i])
                                        {
                                                i_ret = fts_ctpm_fw_upgrade_with_i_file(client);
                                                if (i_ret == 0)
                                                {
                                                        printk("[FTS] upgrade to new version 0x%x\n", uc_host_fm_ver);
                                                }
                                                else
                                                {
                                                        printk("[FTS] upgrade failed ret=%d.\n", i_ret);
                                                }
                                                break;
                                        }
                                }
                                break;
                        case YEJI_VENDOR_TP:
				printk("YEJI VENDOR's TP firmware upgrade \n");
                                for (i = 0; i < sizeof(version_list_yeji_vendor)/sizeof(version_list_yeji_vendor[0]); i++)
                                {
                                        if (uc_tp_fm_ver == version_list_yeji_vendor[i])
                                        {
                                                i_ret = fts_ctpm_fw_upgrade_with_i_file(client);
                                                if (i_ret == 0)
                                                {
                                                        printk("[FTS] upgrade to new version 0x%x\n", uc_host_fm_ver);
                                                }
                                                else
                                                {
                                                        printk("[FTS] upgrade failed ret=%d.\n", i_ret);
                                                }
                                                        break;
                                        }
                                }
                                break;
                        default:
                                break;
                }
    return 0;
}

static ssize_t ft5x06_update_fw_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct ft5x06_ts_data *data = dev_get_drvdata(dev);
	return snprintf(buf, 2, "%d\n", data->loading_fw);
}

static ssize_t ft5x06_update_fw_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
	struct ft5x06_ts_data *data = dev_get_drvdata(dev);
	unsigned long val;
	int rc;

	if (size > 2)
		return -EINVAL;

	rc = kstrtoul(buf, 10, &val);
	if (rc != 0)
		return rc;

	if (data->suspended) {
		dev_info(dev, "In suspend state, try again later...\n");
		return size;
	}

	mutex_lock(&data->input_dev->mutex);
	if (!data->loading_fw  && val) {
		data->loading_fw = true;
		ft5x06_fw_upgrade(dev, false);
		data->loading_fw = false;
	}
	mutex_unlock(&data->input_dev->mutex);

	return size;
}

static DEVICE_ATTR(update_fw, 0664, ft5x06_update_fw_show,
				ft5x06_update_fw_store);

static ssize_t ft5x06_force_update_fw_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
	struct ft5x06_ts_data *data = dev_get_drvdata(dev);
	unsigned long val;
	int rc;

	if (size > 2)
		return -EINVAL;

	rc = kstrtoul(buf, 10, &val);
	if (rc != 0)
		return rc;

	mutex_lock(&data->input_dev->mutex);
	if (!data->loading_fw  && val) {
		data->loading_fw = true;
		ft5x06_fw_upgrade(dev, true);
		data->loading_fw = false;
	}
	mutex_unlock(&data->input_dev->mutex);

	return size;
}

static DEVICE_ATTR(force_update_fw, 0664, ft5x06_update_fw_show,
				ft5x06_force_update_fw_store);

static ssize_t ft5x06_fw_name_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct ft5x06_ts_data *data = dev_get_drvdata(dev);
	return snprintf(buf, FT_FW_NAME_MAX_LEN - 1, "%s\n", data->fw_name);
}

static ssize_t ft5x06_fw_name_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
	struct ft5x06_ts_data *data = dev_get_drvdata(dev);

	if (size > FT_FW_NAME_MAX_LEN - 1)
		return -EINVAL;

	strlcpy(data->fw_name, buf, size);
	if (data->fw_name[size-1] == '\n')
		data->fw_name[size-1] = 0;

	return size;
}

static DEVICE_ATTR(fw_name, 0664, ft5x06_fw_name_show, ft5x06_fw_name_store);

static bool ft5x06_debug_addr_is_valid(int addr)
{
	if (addr < 0 || addr > 0xFF) {
		pr_err("FT reg address is invalid: 0x%x\n", addr);
		return false;
	}

	return true;
}

static int ft5x06_debug_data_set(void *_data, u64 val)
{
	struct ft5x06_ts_data *data = _data;

	mutex_lock(&data->input_dev->mutex);

	if (ft5x06_debug_addr_is_valid(data->addr))
		dev_info(&data->client->dev,
			"Writing into FT registers not supported\n");

	mutex_unlock(&data->input_dev->mutex);

	return 0;
}

static int ft5x06_debug_data_get(void *_data, u64 *val)
{
	struct ft5x06_ts_data *data = _data;
	int rc;
	u8 reg;

	mutex_lock(&data->input_dev->mutex);

	if (ft5x06_debug_addr_is_valid(data->addr)) {
		rc = ft5x0x_read_reg(data->client, data->addr, &reg);
		if (rc < 0)
			dev_err(&data->client->dev,
				"FT read register 0x%x failed (%d)\n",
				data->addr, rc);
		else
			*val = reg;
	}

	mutex_unlock(&data->input_dev->mutex);

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(debug_data_fops, ft5x06_debug_data_get,
			ft5x06_debug_data_set, "0x%02llX\n");

static int ft5x06_debug_addr_set(void *_data, u64 val)
{
	struct ft5x06_ts_data *data = _data;

	if (ft5x06_debug_addr_is_valid(val)) {
		mutex_lock(&data->input_dev->mutex);
		data->addr = val;
		mutex_unlock(&data->input_dev->mutex);
	}

	return 0;
}

static int ft5x06_debug_addr_get(void *_data, u64 *val)
{
	struct ft5x06_ts_data *data = _data;

	mutex_lock(&data->input_dev->mutex);

	if (ft5x06_debug_addr_is_valid(data->addr))
		*val = data->addr;

	mutex_unlock(&data->input_dev->mutex);

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(debug_addr_fops, ft5x06_debug_addr_get,
			ft5x06_debug_addr_set, "0x%02llX\n");

static int ft5x06_debug_suspend_set(void *_data, u64 val)
{
	struct ft5x06_ts_data *data = _data;

	mutex_lock(&data->input_dev->mutex);

	if (val)
		ft5x06_ts_suspend(&data->client->dev);
	else
		ft5x06_ts_resume(&data->client->dev);

	mutex_unlock(&data->input_dev->mutex);

	return 0;
}

static int ft5x06_debug_suspend_get(void *_data, u64 *val)
{
	struct ft5x06_ts_data *data = _data;

	mutex_lock(&data->input_dev->mutex);
	*val = data->suspended;
	mutex_unlock(&data->input_dev->mutex);

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(debug_suspend_fops, ft5x06_debug_suspend_get,
			ft5x06_debug_suspend_set, "%lld\n");

static int ft5x06_debug_dump_info(struct seq_file *m, void *v)
{
	struct ft5x06_ts_data *data = m->private;

	seq_printf(m, "%s\n", data->ts_info);

	return 0;
}

static int debugfs_dump_info_open(struct inode *inode, struct file *file)
{
	return single_open(file, ft5x06_debug_dump_info, inode->i_private);
}

static const struct file_operations debug_dump_info_fops = {
	.owner		= THIS_MODULE,
	.open		= debugfs_dump_info_open,
	.read		= seq_read,
	.release	= single_release,
};

#ifdef CONFIG_OF
static int ft5x06_get_dt_coords(struct device *dev, char *name,
				struct ft5x06_ts_platform_data *pdata)
{
	u32 coords[FT_COORDS_ARR_SIZE];
	struct property *prop;
	struct device_node *np = dev->of_node;
	int coords_size, rc;

	prop = of_find_property(np, name, NULL);
	if (!prop)
		return -EINVAL;
	if (!prop->value)
		return -ENODATA;

	coords_size = prop->length / sizeof(u32);
	if (coords_size != FT_COORDS_ARR_SIZE) {
		dev_err(dev, "invalid %s\n", name);
		return -EINVAL;
	}

	rc = of_property_read_u32_array(np, name, coords, coords_size);
	if (rc && (rc != -EINVAL)) {
		dev_err(dev, "Unable to read %s\n", name);
		return rc;
	}

	if (!strcmp(name, "focaltech,panel-coords")) {
		pdata->panel_minx = coords[0];
		pdata->panel_miny = coords[1];
		pdata->panel_maxx = coords[2];
		pdata->panel_maxy = coords[3];
	} else if (!strcmp(name, "focaltech,display-coords")) {
		pdata->x_min = coords[0];
		pdata->y_min = coords[1];
		pdata->x_max = coords[2];
		pdata->y_max = coords[3];
	} else {
		dev_err(dev, "unsupported property %s\n", name);
		return -EINVAL;
	}

	return 0;
}

static int ft5x06_parse_dt(struct device *dev,
			struct ft5x06_ts_platform_data *pdata)
{
	int rc;
	struct device_node *np = dev->of_node;
	struct property *prop;
	u32 temp_val, num_buttons;
	u32 button_map[MAX_BUTTONS];

	pdata->name = "focaltech";
	rc = of_property_read_string(np, "focaltech,name", &pdata->name);
	if (rc && (rc != -EINVAL)) {
		dev_err(dev, "Unable to read name\n");
		return rc;
	}

	rc = ft5x06_get_dt_coords(dev, "focaltech,panel-coords", pdata);
	if (rc && (rc != -EINVAL))
		return rc;

	rc = ft5x06_get_dt_coords(dev, "focaltech,display-coords", pdata);
	if (rc)
		return rc;

	pdata->i2c_pull_up = of_property_read_bool(np,
						"focaltech,i2c-pull-up");

	pdata->no_force_update = of_property_read_bool(np,
						"focaltech,no-force-update");
	/* reset, irq gpio info */
	pdata->reset_gpio = of_get_named_gpio_flags(np, "focaltech,reset-gpio",
				0, &pdata->reset_gpio_flags);
	if (pdata->reset_gpio < 0)
		return pdata->reset_gpio;
	pdata->irq_gpio = of_get_named_gpio_flags(np, "focaltech,irq-gpio",
				0, &pdata->irq_gpio_flags);
	if (pdata->irq_gpio < 0)
		return pdata->irq_gpio;

	pdata->fw_name = "ft_fw.bin";
	rc = of_property_read_string(np, "focaltech,fw-name", &pdata->fw_name);
	if (rc && (rc != -EINVAL)) {
		dev_err(dev, "Unable to read fw name\n");
		return rc;
	}

	rc = of_property_read_u32(np, "focaltech,group-id", &temp_val);
	if (!rc)
		pdata->group_id = temp_val;
	else
		return rc;

	rc = of_property_read_u32(np, "focaltech,hard-reset-delay-ms",
							&temp_val);
	if (!rc)
		pdata->hard_rst_dly = temp_val;
	else
		return rc;

	rc = of_property_read_u32(np, "focaltech,soft-reset-delay-ms",
							&temp_val);
	if (!rc)
		pdata->soft_rst_dly = temp_val;
	else
		return rc;

	rc = of_property_read_u32(np, "focaltech,num-max-touches", &temp_val);
	if (!rc)
		pdata->num_max_touches = temp_val;
	else
		return rc;

	rc = of_property_read_u32(np, "focaltech,fw-delay-aa-ms", &temp_val);
	if (rc && (rc != -EINVAL)) {
		dev_err(dev, "Unable to read fw delay aa\n");
		return rc;
	} else if (rc != -EINVAL)
		pdata->info.delay_aa =  temp_val;

	rc = of_property_read_u32(np, "focaltech,fw-delay-55-ms", &temp_val);
	if (rc && (rc != -EINVAL)) {
		dev_err(dev, "Unable to read fw delay 55\n");
		return rc;
	} else if (rc != -EINVAL)
		pdata->info.delay_55 =  temp_val;

	rc = of_property_read_u32(np, "focaltech,fw-upgrade-id1", &temp_val);
	if (rc && (rc != -EINVAL)) {
		dev_err(dev, "Unable to read fw upgrade id1\n");
		return rc;
	} else if (rc != -EINVAL)
		pdata->info.upgrade_id_1 =  temp_val;

	rc = of_property_read_u32(np, "focaltech,fw-upgrade-id2", &temp_val);
	if (rc && (rc != -EINVAL)) {
		dev_err(dev, "Unable to read fw upgrade id2\n");
		return rc;
	} else if (rc != -EINVAL)
		pdata->info.upgrade_id_2 =  temp_val;

	rc = of_property_read_u32(np, "focaltech,fw-delay-readid-ms",
							&temp_val);
	if (rc && (rc != -EINVAL)) {
		dev_err(dev, "Unable to read fw delay read id\n");
		return rc;
	} else if (rc != -EINVAL)
		pdata->info.delay_readid =  temp_val;

	rc = of_property_read_u32(np, "focaltech,fw-delay-era-flsh-ms",
							&temp_val);
	if (rc && (rc != -EINVAL)) {
		dev_err(dev, "Unable to read fw delay erase flash\n");
		return rc;
	} else if (rc != -EINVAL)
		pdata->info.delay_erase_flash =  temp_val;

	pdata->info.auto_cal = of_property_read_bool(np,
					"focaltech,fw-auto-cal");

	pdata->fw_vkey_support = of_property_read_bool(np,
						"focaltech,fw-vkey-support");

	pdata->ignore_id_check = of_property_read_bool(np,
						"focaltech,ignore-id-check");

	rc = of_property_read_u32(np, "focaltech,family-id", &temp_val);
	if (!rc)
		pdata->family_id = temp_val;
	else
		return rc;

	prop = of_find_property(np, "focaltech,button-map", NULL);
	if (prop) {
		num_buttons = prop->length / sizeof(temp_val);
		if (num_buttons > MAX_BUTTONS)
			return -EINVAL;

		rc = of_property_read_u32_array(np,
			"focaltech,button-map", button_map,
			num_buttons);
		if (rc) {
			dev_err(dev, "Unable to read key codes\n");
			return rc;
		}
	}

	return 0;
}
#else
static int ft5x06_parse_dt(struct device *dev,
			struct ft5x06_ts_platform_data *pdata)
{
	return -ENODEV;
}
#endif
/*#[BUGFIX]-BGEIN by TCTNB.XQJ,10/25/2013,FR-529341,modify  vk.compitable with different TP*/
/*#[BUGFIX]-BGEIN by TCTNB.XQJ,09/29/2013,FR-523019,add VK setting for vk.11/08,modify */
/*#[BUGFIX]-MOd  by TCTNB.XQJ,12/13/2013,FR-571832,add VK setting for vk for rio6_tf */
/////////////////////////
/***********************************************************************************************
    add by tcl.xqj  for virtualkeys 
***********************************************************************************************/
#if defined CONFIG_TCT_8X26_MIATA
static ssize_t ft5x06_virtual_keys_register(struct kobject *kobj,
			     struct kobj_attribute *attr,
			     char *buf)
{
	printk(KERN_ERR "%s: 444\n", __func__);
	return snprintf(buf, 200,
	__stringify(EV_KEY) ":" __stringify(KEY_MENU)  ":440:1000:100:120"
	":" __stringify(EV_KEY) ":" __stringify(KEY_HOME)   ":270:1000:100:120"
	":" __stringify(EV_KEY) ":" __stringify(KEY_BACK)   ":100:1000:100:120"
	"\n");
}
#else
static ssize_t ft5x06_virtual_keys_register(struct kobject *kobj,
			     struct kobj_attribute *attr,
			     char *buf)
{
	printk(KERN_ERR "%s: 444\n", __func__);
	return snprintf(buf, 200,
	__stringify(EV_KEY) ":" __stringify(KEY_MENU)  ":500:1350:100:120"
	":" __stringify(EV_KEY) ":" __stringify(KEY_HOME)   ":350:1350:100:120"
	":" __stringify(EV_KEY) ":" __stringify(KEY_BACK)   ":150:1350:100:120"
	"\n");
}
#endif
static struct kobj_attribute ft5x06_virtual_keys_attr = {
	.attr = {
		.name = "virtualkeys.ft5x06_ts",
		.mode = S_IRUGO,
	},
	.show = &ft5x06_virtual_keys_register,
};

static struct attribute *ft5x06_virtual_key_properties_attrs[] = {
	&ft5x06_virtual_keys_attr.attr,
	NULL,
};

static struct attribute_group ft5x06_virtual_key_properties_attr_group = {
	.attrs = ft5x06_virtual_key_properties_attrs,
};


static void __init ft5x06_init_vkeys_8x26(void)
{
	int rc = 0;
	static struct kobject *ft5x06_virtual_key_properties_kobj;


	ft5x06_virtual_key_properties_kobj =
			kobject_create_and_add("board_properties", NULL);

	if (ft5x06_virtual_key_properties_kobj)
		rc = sysfs_create_group(ft5x06_virtual_key_properties_kobj,
				&ft5x06_virtual_key_properties_attr_group);

	if (!ft5x06_virtual_key_properties_kobj || rc)
		pr_err("%s: failed to create board_properties\n", __func__);

}

#ifdef FLIP_COVER_SWITCH
static void flip_cover_on_work_func(struct work_struct *work)
{
	int err;
	u8 reg_addr;
	u8 reg_value = 0;
	u8 starty_offset = 0; 
	u8 terminatey_offset = 600 >> 3; 

	if(1==power_on)
	{
		ft5x0x_write_reg(flip_cover_client, FT_REG_COVER_SWITCH, 1);
		ft5x0x_write_reg(flip_cover_client, 0xb6, starty_offset);
		ft5x0x_write_reg(flip_cover_client, 0xb7, terminatey_offset);
		reg_addr = FT_REG_COVER_SWITCH;
		err = ft5x0x_read_reg(flip_cover_client, reg_addr, &reg_value);
		if(err < 0)
			printk("%s read register failed\n", __func__);
		else
			printk("setting flip cover function done.reg_value = %x\n", reg_value);
	}
	flip_cover_switch = 1;
}

static void flip_cover_off_work_func(struct work_struct *work)
{
	int err;
	u8 reg_addr;
	u8 reg_value = 0;

	if(1==power_on)
	{
		ft5x0x_write_reg(flip_cover_client, FT_REG_COVER_SWITCH, 0);
		reg_addr = FT_REG_COVER_SWITCH;
		err = ft5x0x_read_reg(flip_cover_client, reg_addr, &reg_value);
		if(err < 0)
			printk("%s read register failed\n", __func__);
		else
			printk("setting flip cover function done.reg_value = %x\n", reg_value);
	}
	flip_cover_switch = 0;
}

void fts_ctp_flip_cover_switch(bool on)
{
	if(on)
	{
		schedule_delayed_work(&flip_cover_on_wq, msecs_to_jiffies(500));
	}
	else if(!on)
	{	
		schedule_delayed_work(&flip_cover_off_wq, msecs_to_jiffies(500));
	}
}
#endif
/*create apk debug channel*/
#ifdef FTS_APK_DEBUG
#define PROC_UPGRADE						0
#define PROC_READ_REGISTER			1
#define PROC_WRITE_REGISTER	    2
#define PROC_RAWDATA						3
#define PROC_AUTOCLB						4
#define PROC_UPGRADE_INFO 			5
#define PROC_WRITE_DATA        	6
#define PROC_READ_DATA        	7

#define PROC_NAME	"ft5x0x-debug"
#define PROC_NAME1	"boot_status"
static unsigned char proc_operate_mode = PROC_UPGRADE;
static struct proc_dir_entry *fts_proc_entry;
//static struct proc_dir_entry *fts_proc_entry1;

static int g_upgrade_successful = 0;
/*interface of write proc*/
static int fts_debug_write(struct file *filp, 
	const char __user *buff, unsigned long len, void *data)
{
	struct i2c_client *client = (struct i2c_client *)fts_proc_entry->data;
	unsigned char writebuf[FTS_PACKET_LENGTH];
	int buflen = len;
	int writelen = 0;
	int ret = 0;
	
	if (copy_from_user(&writebuf, buff, buflen)) {
		dev_err(&client->dev, "%s:copy from user error\n", __func__);
		return -EFAULT;
	}
	proc_operate_mode = writebuf[0];
	
	switch (proc_operate_mode) {
	case PROC_UPGRADE:
		{
			char upgrade_file_path[128];
			memset(upgrade_file_path, 0, sizeof(upgrade_file_path));
			sprintf(upgrade_file_path, "%s", writebuf + 1);
			upgrade_file_path[buflen-1] = '\0';
			//dev_dbg("%s\n", upgrade_file_path);
			disable_irq(client->irq);

			g_upgrade_successful = 0;
			ret = fts_ctpm_fw_upgrade_with_app_file(client, upgrade_file_path);

			enable_irq(client->irq);
			if (ret < 0) {
				dev_err(&client->dev, "%s:upgrade failed.\n", __func__);
				return ret;
			}
			else
			{
				g_upgrade_successful = 1;
			}
		}
		break;
	case PROC_READ_REGISTER:
		writelen = 1;
		//dev_dbg("%s:register addr=0x%02x\n", __func__, writebuf[1]);
		ret = ft5x06_i2c_write(client, writebuf + 1, writelen);
		if (ret < 0) {
			dev_err(&client->dev, "%s:write iic error\n", __func__);
			return ret;
		}
		break;
	case PROC_WRITE_REGISTER:
		writelen = 2;
		ret = ft5x06_i2c_write(client, writebuf + 1, writelen);
		if (ret < 0) {
			dev_err(&client->dev, "%s:write iic error\n", __func__);
			return ret;
		}
		break;
	case PROC_RAWDATA:
		break;
#ifdef AUTO_CLB
	case PROC_AUTOCLB:
		fts_ctpm_auto_clb(client);
		break;
#endif
	case PROC_UPGRADE_INFO:
		break;
  case PROC_READ_DATA:                                                    
  case PROC_WRITE_DATA:                                                   
 		//dev_dbg("%s,%d:PROC_READ_DATA,PROC_WRITE_DATA\n",__func__,__LINE__);      
    writelen = len - 1;                                                
    ret = ft5x06_i2c_write(client, writebuf + 1, writelen);            
    if (ret < 0) {                                                     
          dev_err(&client->dev, "%s:write iic error\n", __func__);     
          return ret;                                                  
    }                                                                  
    break;                                                             
	default:
		break;
	}
	

	return len;
}

/*interface of read proc*/
static int fts_debug_read( char *page, char **start,
	off_t off, int count, int *eof, void *data )
{
	struct i2c_client *client = (struct i2c_client *)fts_proc_entry->data;
	int ret = 0;//, err = 0;
	//u8 tx = 0;//, rx = 0;
	//int i;
	unsigned char raw_buf[512];
	//char buf[4088];
	int num_read_chars = 0;
	int readlen = 0;
	u8 regvalue = 0x00, regaddr = 0x00;
	
	switch (proc_operate_mode) {
	case PROC_UPGRADE:
		/*after calling fts_debug_write to upgrade*/
		regaddr = 0xA6;
		ret = ft5x0x_read_reg(client, regaddr, &regvalue);
		if (ret < 0)
			num_read_chars = sprintf(raw_buf, "%s", "get fw version failed.\n");
		else
			num_read_chars = sprintf(raw_buf, "current fw version:0x%02x\n", regvalue);
		break;
	case PROC_READ_REGISTER:
		readlen = 1;
		ret = ft5x06_i2c_read(client, NULL, 0, raw_buf, readlen);
		if (ret < 0) {
			dev_err(&client->dev, "%s:read iic error\n", __func__);
			return ret;
		} else
			//dev_dbg("%s:value=0x%02x\n", __func__, raw_buf[0]);
		num_read_chars = 1;
		break;
	case PROC_RAWDATA:
		break;
	case PROC_UPGRADE_INFO:
		if(1 == g_upgrade_successful)
			raw_buf[0] = 1;
		else
			raw_buf[0] = 0;
		//dev_dbg("%s:value=0x%02x\n", __func__, raw_buf[0]);
		num_read_chars = 1;
		break;
  case PROC_READ_DATA:                                                              
		//dev_dbg("%s,%d:PROC_READ_DATA\n",__func__,__LINE__);                                
    readlen = count;                                                             
    ret = ft5x06_i2c_read(client, NULL, 0, raw_buf, readlen);                        
    if (ret < 0) {                                                               
          dev_err(&client->dev, "%s:read iic error\n", __func__);                
          return ret;                                                            
    }                                                                            
                                                                                 
    num_read_chars = readlen;                                                    
    break;                                                                       
  case PROC_WRITE_DATA:                                                             
		//dev_dbg("%s,%d:PROC_WRITE_DATA\n",__func__,__LINE__);                               
    break;                                                                       
	default:
		break;
	}
	
	memcpy(page, raw_buf, num_read_chars);

	return num_read_chars;
}

int fts_create_apk_debug_channel(struct i2c_client * client)
{
	fts_proc_entry = create_proc_entry(PROC_NAME, 0777, NULL);
	if (NULL == fts_proc_entry) {
		dev_err(&client->dev, "Couldn't create proc entry!\n");
		return -ENOMEM;
	} else {
		dev_info(&client->dev, "Create proc entry success!\n");
		fts_proc_entry->data = client;
		fts_proc_entry->write_proc = fts_debug_write;
		fts_proc_entry->read_proc = fts_debug_read;
	}
	return 0;
}

void fts_release_apk_debug_channel(void)
{
	if (fts_proc_entry)
		remove_proc_entry(PROC_NAME, NULL);
}
#endif

/////////////////////////
/*[BUGFIX]-END by TCTNB.XQJ*/
static int ft5x06_ts_probe(struct i2c_client *client,
			   const struct i2c_device_id *id)
{
	struct ft5x06_ts_platform_data *pdata;
	struct ft5x06_ts_data *data;
	struct input_dev *input_dev;
	struct dentry *temp;
	u8 reg_value;
	u8 reg_addr;
	int err, len, ret;
	printk("%s\n", __func__);

	if (client->dev.of_node) {
		pdata = devm_kzalloc(&client->dev,
			sizeof(struct ft5x06_ts_platform_data), GFP_KERNEL);
		if (!pdata) {
			dev_err(&client->dev, "Failed to allocate memory\n");
			return -ENOMEM;
		}

		err = ft5x06_parse_dt(&client->dev, pdata);
		if (err) {
			dev_err(&client->dev, "DT parsing failed\n");
			return err;
		}
	} else
		pdata = client->dev.platform_data;

	if (!pdata) {
		dev_err(&client->dev, "Invalid pdata\n");
		return -EINVAL;
	}

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "I2C not supported\n");
		return -ENODEV;
	}

	data = devm_kzalloc(&client->dev,
			sizeof(struct ft5x06_ts_data), GFP_KERNEL);
	if (!data) {
		dev_err(&client->dev, "Not enough memory\n");
		return -ENOMEM;
	}

	if (pdata->fw_name) {
		len = strlen(pdata->fw_name);
		if (len > FT_FW_NAME_MAX_LEN - 1) {
			dev_err(&client->dev, "Invalid firmware name\n");
			return -EINVAL;
		}

		strlcpy(data->fw_name, pdata->fw_name, len + 1);
	}

	data->tch_data_len = FT_TCH_LEN(pdata->num_max_touches);
	data->tch_data = devm_kzalloc(&client->dev,
				data->tch_data_len, GFP_KERNEL);
	if (!data) {
		dev_err(&client->dev, "Not enough memory\n");
		return -ENOMEM;
	}

	input_dev = input_allocate_device();
	if (!input_dev) {
		dev_err(&client->dev, "failed to allocate input device\n");
		return -ENOMEM;
	}

	data->input_dev = input_dev;
	data->client = client;
	data->pdata = pdata;

	input_dev->name = "ft5x06_ts";
	input_dev->id.bustype = BUS_I2C;
	input_dev->dev.parent = &client->dev;

	input_set_drvdata(input_dev, data);
	i2c_set_clientdata(client, data);

	__set_bit(EV_KEY, input_dev->evbit);
	__set_bit(EV_ABS, input_dev->evbit);
	__set_bit(BTN_TOUCH, input_dev->keybit);
	__set_bit(INPUT_PROP_DIRECT, input_dev->propbit);
 /*[BUGFIX]-Del Begin by TCTNB.WPL,2013/11/16, bug556002*/
//	input_mt_init_slots(input_dev, pdata->num_max_touches);
 /*[BUGFIX]-Del End by TCTNB.WPL,2013/11/16*/
	input_set_abs_params(input_dev, ABS_MT_POSITION_X, pdata->x_min,
			     pdata->x_max, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, pdata->y_min,
			     pdata->y_max, 0, 0);
 /*[BUGFIX]-Add Begin by TCTNB.WPL,2013/11/16, bug556002*/
	input_set_abs_params(input_dev, ABS_MT_TRACKING_ID, 0,
			     5, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, FT_PRESS, 0, 0);
 /*[BUGFIX]-Add End by TCTNB.WPL,2013/11/16*/

	err = input_register_device(input_dev);
	if (err) {
		dev_err(&client->dev, "Input device registration failed\n");
		goto free_inputdev;
	}

	if (pdata->power_init) {
		err = pdata->power_init(true);
		if (err) {
			dev_err(&client->dev, "power init failed");
			goto unreg_inputdev;
		}
	} else {
		err = ft5x06_power_init(data, true);
		if (err) {
			dev_err(&client->dev, "power init failed");
			goto unreg_inputdev;
		}
	}

	if (pdata->power_on) {
		err = pdata->power_on(true);
		if (err) {
			dev_err(&client->dev, "power on failed");
			goto pwr_deinit;
		}
	} else {
		err = ft5x06_power_on(data, true);
		if (err) {
			dev_err(&client->dev, "power on failed");
			goto pwr_deinit;
		}
	}

	if (gpio_is_valid(pdata->irq_gpio)) {
		err = gpio_request(pdata->irq_gpio, "ft5x06_irq_gpio");
		if (err) {
			dev_err(&client->dev, "irq gpio request failed");
			goto pwr_off;
		}
		err = gpio_direction_input(pdata->irq_gpio);
		if (err) {
			dev_err(&client->dev,
				"set_direction for irq gpio failed\n");
			goto free_irq_gpio;
		}
	}
	if (gpio_is_valid(pdata->reset_gpio)) {
		err = gpio_request(pdata->reset_gpio, "ft5x06_reset_gpio");
		if (err) {
			dev_err(&client->dev, "reset gpio request failed");
			goto free_irq_gpio;
		}

		err = gpio_direction_output(pdata->reset_gpio, 0);
		if (err) {
			dev_err(&client->dev,
				"set_direction for reset gpio failed\n");
			goto free_reset_gpio;
		}
		msleep(data->pdata->hard_rst_dly);
		gpio_set_value_cansleep(data->pdata->reset_gpio, 1);
	}

 /*[BUGFIX]-Mod Begin by TCTNB.WPL,2013/11/15, bug549728, modify by FAE*/
	/* make sure CTP already finish startup process */
	//msleep(data->pdata->soft_rst_dly);
	msleep(250);
/*[BUGFIX]-Add-END by TCTNB.WPL*/

	/* check the controller id */
	reg_addr = FT_REG_ID;
	err = ft5x06_i2c_read(client, &reg_addr, 1, &reg_value, 1);
	if (err < 0) {
		dev_err(&client->dev, "version read failed");
		goto free_reset_gpio;
	}

	dev_info(&client->dev, "Device ID = 0x%x\n", reg_value);

	if ((pdata->family_id != reg_value) && (!pdata->ignore_id_check)) {
		dev_err(&client->dev, "%s:Unsupported controller\n", __func__);
		//goto free_reset_gpio;
	}

	data->family_id = pdata->family_id;

	
	reg_addr = FT_REG_FW_VER;
	err = ft5x06_i2c_read(client, &reg_addr, 1, &reg_value, 1);
	if (err < 0) {
		dev_err(&client->dev, "TP FW version read failed");
		goto free_reset_gpio;
	}

	dev_info(&client->dev, "FW version in TP = 0x%x\n", reg_value);

       reg_addr = 0xA8;
       err = ft5x06_i2c_read(client, &reg_addr, 1, &reg_value, 1);
       if (err < 0) {
            dev_err(&client->dev, "TP FW version read failed");
            goto free_reset_gpio;
        }
      TP_VENDOR=reg_value;
      dev_info(&client->dev, "[FTS] bootloader vendor ID=0x%x\n", TP_VENDOR);
	if(0 == TP_VENDOR)
	{
		printk("!!!!!!The firmware was destroied, check which VENDOR, and then force recover the firmware\n");
		ret = fts_ctpm_update_get_vid_fwid(client);
		if(0x80 == ret )
		{
			 TP_VENDOR = YEJI_VENDOR_TP;
		}
		else if(0x85 == ret )
		{
			 TP_VENDOR = JUNDA_VENDOR_TP;
		}
	}

#if defined CONFIG_TOUCHSCREEN_FT5X06_FIRMWARE
	/* make sure CTP already finish startup process */
	//msleep(200);
      err=fts_ctpm_fw_i_file_config(client);/*[BUGFIX]-Add by TCTNB.XQJ, 2013/12/05, refer to bug 564812 for tp upgrade*/
      if(err<0)
      {
         dev_err(&client->dev, "tp firmware configure failed");
       //	goto free_reset_gpio;
      }
	err = fts_ctpm_auto_upg(client);
#endif
#ifdef FLIP_COVER_SWITCH
	flip_cover_client = client;
	INIT_DELAYED_WORK(&flip_cover_on_wq, flip_cover_on_work_func);
	INIT_DELAYED_WORK(&flip_cover_off_wq, flip_cover_off_work_func);
#endif
#ifdef FTS_APK_DEBUG
	fts_create_apk_debug_channel(client);
#endif

/*
	dev_info(&client->dev, "FW_ver in code = 0x%x\n", FW_ver);
	printk("ft5x06 FW upgrade begin...\n");
	if(0x80 == TP_VENDOR)
	err = fts_ctpm_fw_upgrade_with_i_file(client);         //upgrade firmware
	else
	printk("for the moument, VENDOR junda's TP dosen't need to upgrade, will add this function next week\n");
	printk("ft5x06 FW upgrade end...\n");
	if (err != 0){
		printk(KERN_ERR "ft5x06 upgrade failed\n");
		//goto free_reset_gpio;
	}

	reg_addr = FT_REG_FW_VER;
	err = ft5x06_i2c_read(client, &reg_addr, 1, &reg_value, 1);
	if (err < 0) {
		dev_err(&client->dev, "TP FW version read failed");
		goto free_reset_gpio;
	} else {
		dev_info(&client->dev, "new Firmware version in TP = 0x%x\n\n", reg_value);
	}
*/

#ifdef FT_CAL_THROUGH_HOST
    reg_addr = FT_REG_HOST_CAL;
    err = ft5x06_i2c_read(client, &reg_addr, 1, &reg_value, 1);
    if (err < 0) {
        dev_err(&client->dev, "calibration flag read failed\n");
        goto free_reset_gpio;
    } else {
        dev_info(&client->dev, "calibration flag = 0x%x\n", reg_value);
    }

    if (reg_value != 0xaa)
    {
        ft5x0x_write_reg(client, FT_REG_HOST_CAL, 0xaa);
        printk("calibrating begin...\n");
        fts_ctpm_auto_clb(client);
        printk("calibrating stop...\n");
    }
#endif

	err = request_threaded_irq(client->irq, NULL,
				   ft5x06_ts_interrupt, IRQF_TRIGGER_FALLING | IRQF_SHARED,/*pdata->irqflags,#[BUGFIX]-Modify maybe crash problem,by TCTNB.XQJ,10/25/2013,FR-529341*/
				   client->dev.driver->name, data);
	if (err) {
		dev_err(&client->dev, "request irq failed\n");
		goto free_reset_gpio;
	}

	err = device_create_file(&client->dev, &dev_attr_fw_name);
	if (err) {
		dev_err(&client->dev, "sys file creation failed\n");
		goto irq_free;
	}

	err = device_create_file(&client->dev, &dev_attr_update_fw);
	if (err) {
		dev_err(&client->dev, "sys file creation failed\n");
		goto free_fw_name_sys;
	}

	err = device_create_file(&client->dev, &dev_attr_force_update_fw);
	if (err) {
		dev_err(&client->dev, "sys file creation failed\n");
		goto free_update_fw_sys;
	}

	err = ft5x0x_create_sysfs(client);
	if (err) {
		dev_err(&client->dev, "sys file creation failed\n");
		goto irq_free;
	}

	data->dir = debugfs_create_dir(FT_DEBUG_DIR_NAME, NULL);
	if (data->dir == NULL || IS_ERR(data->dir)) {
		pr_err("debugfs_create_dir failed(%ld)\n", PTR_ERR(data->dir));
		err = PTR_ERR(data->dir);
		goto free_force_update_fw_sys;
	}

	temp = debugfs_create_file("addr", S_IRUSR | S_IWUSR, data->dir, data,
				   &debug_addr_fops);
	if (temp == NULL || IS_ERR(temp)) {
		pr_err("debugfs_create_file failed: rc=%ld\n", PTR_ERR(temp));
		err = PTR_ERR(temp);
		goto free_debug_dir;
	}

	temp = debugfs_create_file("data", S_IRUSR | S_IWUSR, data->dir, data,
				   &debug_data_fops);
	if (temp == NULL || IS_ERR(temp)) {
		pr_err("debugfs_create_file failed: rc=%ld\n", PTR_ERR(temp));
		err = PTR_ERR(temp);
		goto free_debug_dir;
	}

	temp = debugfs_create_file("suspend", S_IRUSR | S_IWUSR, data->dir,
					data, &debug_suspend_fops);
	if (temp == NULL || IS_ERR(temp)) {
		pr_err("debugfs_create_file failed: rc=%ld\n", PTR_ERR(temp));
		err = PTR_ERR(temp);
		goto free_debug_dir;
	}

	temp = debugfs_create_file("dump_info", S_IRUSR | S_IWUSR, data->dir,
					data, &debug_dump_info_fops);
	if (temp == NULL || IS_ERR(temp)) {
		pr_err("debugfs_create_file failed: rc=%ld\n", PTR_ERR(temp));
		err = PTR_ERR(temp);
		goto free_debug_dir;
	}

	data->ts_info = devm_kzalloc(&client->dev,
				FT_INFO_MAX_LEN, GFP_KERNEL);
	if (!data->ts_info) {
		dev_err(&client->dev, "Not enough memory\n");
		goto free_debug_dir;
	}

 /*[BUGFIX]-Del Begin by TCTNB.WPL,2013/11/15, bug549728, modify by FAE*/
#if 0
	/*get some register information */
	reg_addr = FT_REG_POINT_RATE;
	ft5x06_i2c_read(client, &reg_addr, 1, &reg_value, 1);
	if (err < 0)
		dev_err(&client->dev, "report rate read failed");

	dev_info(&client->dev, "report rate = %dHz\n", reg_value * 10);

	reg_addr = FT_REG_THGROUP;
	err = ft5x06_i2c_read(client, &reg_addr, 1, &reg_value, 1);
	if (err < 0)
		dev_err(&client->dev, "threshold read failed");

	dev_dbg(&client->dev, "touch threshold = %d\n", reg_value * 4);

	ft5x06_update_fw_ver(data);
#endif
/*[BUGFIX]-Del-END by TCTNB.WPL*/

	FT_STORE_TS_INFO(data->ts_info, data->family_id, data->pdata->name,
			data->pdata->num_max_touches, data->pdata->group_id,
			data->pdata->fw_vkey_support ? "yes" : "no",
			data->pdata->fw_name, data->fw_ver[0],
			data->fw_ver[1], data->fw_ver[2]);

#if defined(CONFIG_FB)
	data->fb_notif.notifier_call = fb_notifier_callback;

	err = fb_register_client(&data->fb_notif);

	if (err)
		dev_err(&client->dev, "Unable to register fb_notifier: %d\n",
			err);
#elif defined(CONFIG_HAS_EARLYSUSPEND)
	data->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN +
						    FT_SUSPEND_LEVEL;
	data->early_suspend.suspend = ft5x06_ts_early_suspend;
	data->early_suspend.resume = ft5x06_ts_late_resume;
	register_early_suspend(&data->early_suspend);
#endif
        ft5x06_init_vkeys_8x26();/*#[BUGFIX]-ADD by TCTNB.XQJ,09/29/2013,FR-523019,add VK setting for vk.*/
/*[BUGFIX]-Add-BEGIN by TCTNB.XQJ, 2013/12/19, refer to bug 551940 double click wake and unlock*/
#ifdef CONFIG_8X26_TP_GESTURE
       DBclick_attr_create();
       ft_g_client = client;
 //   device_init_wakeup(&client->dev, 1);//move to set double click function
    input_set_capability(data->input_dev, EV_KEY, KEY_POWER);

#endif
/*[BUGFIX]-End by TCTNB.XQJ*/
	return 0;

free_debug_dir:
	debugfs_remove_recursive(data->dir);
free_force_update_fw_sys:
	device_remove_file(&client->dev, &dev_attr_force_update_fw);
free_update_fw_sys:
	device_remove_file(&client->dev, &dev_attr_update_fw);
free_fw_name_sys:
	device_remove_file(&client->dev, &dev_attr_fw_name);
irq_free:
	free_irq(client->irq, data);
free_reset_gpio:
	if (gpio_is_valid(pdata->reset_gpio))
		gpio_free(pdata->reset_gpio);
free_irq_gpio:
	if (gpio_is_valid(pdata->irq_gpio))
		gpio_free(pdata->irq_gpio);
pwr_off:
	if (pdata->power_on)
		pdata->power_on(false);
	else
		ft5x06_power_on(data, false);
pwr_deinit:
	if (pdata->power_init)
		pdata->power_init(false);
	else
		ft5x06_power_init(data, false);
unreg_inputdev:
	input_unregister_device(input_dev);
	input_dev = NULL;
free_inputdev:
	input_free_device(input_dev);
#ifdef FLIP_COVER_SWITCH
	global_tp_usable = 0;
	cancel_delayed_work_sync(&flip_cover_on_wq);
	cancel_delayed_work_sync(&flip_cover_off_wq);
#endif
	return err;
}

static int __devexit ft5x06_ts_remove(struct i2c_client *client)
{
	struct ft5x06_ts_data *data = i2c_get_clientdata(client);

	debugfs_remove_recursive(data->dir);
	device_remove_file(&client->dev, &dev_attr_force_update_fw);
	device_remove_file(&client->dev, &dev_attr_update_fw);
	device_remove_file(&client->dev, &dev_attr_fw_name);

#if defined(CONFIG_FB)
	if (fb_unregister_client(&data->fb_notif))
		dev_err(&client->dev, "Error occurred while unregistering fb_notifier.\n");
#elif defined(CONFIG_HAS_EARLYSUSPEND)
	unregister_early_suspend(&data->early_suspend);
#endif
#ifdef FTS_APK_DEBUG
	fts_release_apk_debug_channel();
#endif

	free_irq(client->irq, data);

	if (gpio_is_valid(data->pdata->reset_gpio))
		gpio_free(data->pdata->reset_gpio);

	if (gpio_is_valid(data->pdata->irq_gpio))
		gpio_free(data->pdata->irq_gpio);

	if (data->pdata->power_on)
		data->pdata->power_on(false);
	else
		ft5x06_power_on(data, false);

	if (data->pdata->power_init)
		data->pdata->power_init(false);
	else
		ft5x06_power_init(data, false);

	input_unregister_device(data->input_dev);

	return 0;
}

static const struct i2c_device_id ft5x06_ts_id[] = {
	{"ft5x06_ts", 0},
	{},
};

MODULE_DEVICE_TABLE(i2c, ft5x06_ts_id);

#ifdef CONFIG_OF
static struct of_device_id ft5x06_match_table[] = {
	{ .compatible = "focaltech,5x06",},
	{ },
};
#else
#define ft5x06_match_table NULL
#endif

static struct i2c_driver ft5x06_ts_driver = {
	.probe = ft5x06_ts_probe,
	.remove = __devexit_p(ft5x06_ts_remove),
	.driver = {
		   .name = "ft5x06_ts",
		   .owner = THIS_MODULE,
		.of_match_table = ft5x06_match_table,
#ifdef CONFIG_PM
		   .pm = &ft5x06_ts_pm_ops,
#endif
		   },
	.id_table = ft5x06_ts_id,
};

static int __init ft5x06_ts_init(void)
{
	return i2c_add_driver(&ft5x06_ts_driver);
}
module_init(ft5x06_ts_init);

static void __exit ft5x06_ts_exit(void)
{
	i2c_del_driver(&ft5x06_ts_driver);
}
module_exit(ft5x06_ts_exit);

MODULE_DESCRIPTION("FocalTech ft5x06 TouchScreen driver");
MODULE_LICENSE("GPL v2");
