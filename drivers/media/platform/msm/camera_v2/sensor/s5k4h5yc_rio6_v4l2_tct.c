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
 * 13/10/31      Hu Jin       for RIO6 1st main CAM
 * 13/12/03      Hu Jin       new
 * --------------------------------------------------------------------------------------
*/
#include "msm_sensor.h"
#define S5K4H5YC_SENSOR_NAME "s5k4h5yc_rio6"
DEFINE_MSM_MUTEX(s5k4h5yc_mut);
static struct msm_sensor_ctrl_t s5k4h5yc_s_ctrl;

static struct msm_sensor_power_setting s5k4h5yc_power_setting[] = {
//0:  t0
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
		.seq_type = SENSOR_VREG,
		.seq_val = CAM_VAF,
		.config_val = 0,
		.delay = 0,
	},
//1: t1 t1-t0>=0
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
//4: t4 low >= 1ms
#if 0
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
        .delay = 1,
    },
#endif
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
		.delay = 18,
	},
};

static struct v4l2_subdev_info s5k4h5yc_subdev_info[] = {
	{
		.code   = V4L2_MBUS_FMT_SBGGR10_1X10,
		.colorspace = V4L2_COLORSPACE_JPEG,
		.fmt    = 1,
		.order    = 0,
	},
};

static struct msm_camera_i2c_client s5k4h5yc_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

static const struct i2c_device_id s5k4h5yc_i2c_id[] = {
	{S5K4H5YC_SENSOR_NAME,
		(kernel_ulong_t)&s5k4h5yc_s_ctrl},
	{ }
};

static int32_t msm_s5k4h5yc_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	return msm_sensor_i2c_probe(client, id, &s5k4h5yc_s_ctrl);
}

static struct i2c_driver s5k4h5yc_i2c_driver = {
	.id_table = s5k4h5yc_i2c_id,
	.probe  = msm_s5k4h5yc_i2c_probe,
	.driver = {
		.name = S5K4H5YC_SENSOR_NAME,
	},
};

static struct msm_sensor_ctrl_t s5k4h5yc_s_ctrl = {
	.sensor_i2c_client = &s5k4h5yc_sensor_i2c_client,
	.power_setting_array.power_setting = s5k4h5yc_power_setting,
	.power_setting_array.size =
			ARRAY_SIZE(s5k4h5yc_power_setting),
	.msm_sensor_mutex = &s5k4h5yc_mut,
	.sensor_v4l2_subdev_info = s5k4h5yc_subdev_info,
	.sensor_v4l2_subdev_info_size =
			ARRAY_SIZE(s5k4h5yc_subdev_info),
};

static const struct of_device_id s5k4h5yc_dt_match[] = {
	{
		.compatible = "qcom,s5k4h5yc_rio6",
		.data = &s5k4h5yc_s_ctrl
	},
	{}
};

MODULE_DEVICE_TABLE(of, s5k4h5yc_dt_match);

static struct platform_driver s5k4h5yc_platform_driver = {
	.driver = {
		.name = "qcom,s5k4h5yc_rio6",
		.owner = THIS_MODULE,
		.of_match_table = s5k4h5yc_dt_match,
	},
};

static int32_t s5k4h5yc_platform_probe(struct platform_device *pdev)
{
	int32_t rc = 0;
	const struct of_device_id *match;

	match = of_match_device(s5k4h5yc_dt_match, &pdev->dev);
	rc = msm_sensor_platform_probe(pdev, match->data);

    return rc;
}

static int __init s5k4h5yc_init_module(void)
{
	int32_t rc = 0;

	rc = platform_driver_probe(&s5k4h5yc_platform_driver,
		s5k4h5yc_platform_probe);
	if (!rc)
		return rc;
	return i2c_add_driver(&s5k4h5yc_i2c_driver);
}

static void __exit s5k4h5yc_exit_module(void)
{
	if (s5k4h5yc_s_ctrl.pdev) {
		msm_sensor_free_sensor_data(&s5k4h5yc_s_ctrl);
		platform_driver_unregister(&s5k4h5yc_platform_driver);
	} else
		i2c_del_driver(&s5k4h5yc_i2c_driver);
	return;
}

module_init(s5k4h5yc_init_module);
module_exit(s5k4h5yc_exit_module);
MODULE_DESCRIPTION("s5k4h5yc_rio6");
MODULE_LICENSE("GPL v2");

