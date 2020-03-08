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
 * 13/12/02      Hu Jin       Rio4G main cam, ov5648ff
 * --------------------------------------------------------------------------------------
*/
#include "msm_sensor.h"
#define OV5648_SENSOR_NAME "OV5648FF"
DEFINE_MSM_MUTEX(OV5648FF_mut);
static struct msm_sensor_ctrl_t OV5648FF_s_ctrl;

static struct msm_sensor_power_setting OV5648FF_power_setting[] = {
//0:  t0
	{
		.seq_type = SENSOR_VREG,
		.seq_val = CAM_VIO,
		.config_val = 0,
		.delay = 0,
	},
//1: t1 t1-t0>=0
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_VANA,
		.config_val = GPIO_OUT_LOW,
		.delay = 0,
	},
	{
		.seq_type = SENSOR_VREG,
		.seq_val = CAM_VIO,
		.config_val = 1,
		.delay = 10,//must ensure stable before next
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_VANA,
		.config_val = GPIO_OUT_HIGH,
		.delay = 5,
	},
//2: t2 Dovdd 1.8v NC. P18.note6.
//3: af
    {
        .seq_type = SENSOR_GPIO,
        .seq_val = SENSOR_GPIO_AF_PWDM,
        .config_val = GPIO_OUT_LOW,
        .delay = 1,
    },
    {
        .seq_type = SENSOR_GPIO,
        .seq_val = SENSOR_GPIO_AF_PWDM,
        .config_val = GPIO_OUT_HIGH,
        .delay = 1,
    },

//3: t3 low >= 5ms
    {
        .seq_type = SENSOR_GPIO,
        .seq_val = SENSOR_GPIO_STANDBY,
        .config_val = GPIO_OUT_LOW,
        .delay = 5,
    },
    {
        .seq_type = SENSOR_GPIO,
        .seq_val = SENSOR_GPIO_STANDBY,
        .config_val = GPIO_OUT_HIGH,
        .delay = 1,
    },
//4: t4 low >= 1ms // reset does not matter
    {
        .seq_type = SENSOR_GPIO,
        .seq_val = SENSOR_GPIO_RESET,
        .config_val = GPIO_OUT_LOW,
        .delay = 2,
    },
    {
        .seq_type = SENSOR_GPIO,
        .seq_val = SENSOR_GPIO_RESET,
        .config_val = GPIO_OUT_HIGH,
        .delay = 10,
    },
//5: 
    {
        .seq_type = SENSOR_CLK,
        .seq_val = SENSOR_CAM_MCLK,
        .config_val = 24000000,
        .delay = 2,
    },
//6: 
	{
		.seq_type = SENSOR_I2C_MUX,
		.seq_val = 0,
		.config_val = 0,
		.delay = 25,
	},
};

static struct v4l2_subdev_info OV5648FF_subdev_info[] = {
	{
		.code   = V4L2_MBUS_FMT_SBGGR10_1X10,
		.colorspace = V4L2_COLORSPACE_JPEG,
		.fmt    = 1,
		.order    = 0,
	},
};

static struct msm_camera_i2c_client OV5648FF_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

static const struct i2c_device_id OV5648FF_i2c_id[] = {
	{OV5648_SENSOR_NAME,
		(kernel_ulong_t)&OV5648FF_s_ctrl},
	{ }
};

static int32_t msm_OV5648FF_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	return msm_sensor_i2c_probe(client, id, &OV5648FF_s_ctrl);
}

static struct i2c_driver OV5648FF_i2c_driver = {
	.id_table = OV5648FF_i2c_id,
	.probe  = msm_OV5648FF_i2c_probe,
	.driver = {
		.name = OV5648_SENSOR_NAME,
	},
};

static struct msm_sensor_ctrl_t OV5648FF_s_ctrl = {
	.sensor_i2c_client = &OV5648FF_sensor_i2c_client,
	.power_setting_array.power_setting = OV5648FF_power_setting,
	.power_setting_array.size =
			ARRAY_SIZE(OV5648FF_power_setting),
	.msm_sensor_mutex = &OV5648FF_mut,
	.sensor_v4l2_subdev_info = OV5648FF_subdev_info,
	.sensor_v4l2_subdev_info_size =
			ARRAY_SIZE(OV5648FF_subdev_info),
};

static const struct of_device_id OV5648FF_dt_match[] = {
	{
		.compatible = "qcom,OV5648FF",
		.data = &OV5648FF_s_ctrl
	},
	{}
};

MODULE_DEVICE_TABLE(of, OV5648FF_dt_match);

static struct platform_driver OV5648FF_platform_driver = {
	.driver = {
		.name = "qcom,OV5648FF",
		.owner = THIS_MODULE,
		.of_match_table = OV5648FF_dt_match,
	},
};

static int32_t OV5648FF_platform_probe(struct platform_device *pdev)
{
	int32_t rc = 0;
	const struct of_device_id *match;

	match = of_match_device(OV5648FF_dt_match, &pdev->dev);
	rc = msm_sensor_platform_probe(pdev, match->data);
	return rc;
}

static int __init OV5648FF_init_module(void)
{
	int32_t rc = 0;

	rc = platform_driver_probe(&OV5648FF_platform_driver,
		OV5648FF_platform_probe);
	if (!rc)
		return rc;
	return i2c_add_driver(&OV5648FF_i2c_driver);
}

static void __exit OV5648FF_exit_module(void)
{
	if (OV5648FF_s_ctrl.pdev) {
		msm_sensor_free_sensor_data(&OV5648FF_s_ctrl);
		platform_driver_unregister(&OV5648FF_platform_driver);
	} else
		i2c_del_driver(&OV5648FF_i2c_driver);
	return;
}

module_init(OV5648FF_init_module);
module_exit(OV5648FF_exit_module);
MODULE_DESCRIPTION("OV5648FF");
MODULE_LICENSE("GPL v2");
