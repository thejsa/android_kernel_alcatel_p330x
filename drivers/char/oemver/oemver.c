/******************************************************************************/
/*                                                               Date:09/2013 */
/*                             PRESENTATION                                   */
/*                                                                            */
/*      Copyright 2012 TCL Communication Technology Holdings Limited.         */
/*                                                                            */
/* This material is company confidential, cannot be reproduced in any form    */
/* without the written permission of TCL Communication Technology Holdings    */
/* Limited.                                                                   */
/*                                                                            */
/* -------------------------------------------------------------------------- */
/* Author:  Ting Li                                                           */
/* E-Mail:  Ting.Li@tcl-mobile.com                                            */
/* Role  :  versionapi                                                        */
/* Reference documents :                                            */
/* -------------------------------------------------------------------------- */
/* Comments:                                                                  */
/* File    : kernel/drivers/char/oemver/oemver.c                              */
/* Labels  :                                                                  */
/* -------------------------------------------------------------------------- */
/* ========================================================================== */
/* Modifications on Features list / Changes Request / Problems Report         */
/* -------------------------------------------------------------------------- */
/* date    | author         | key              | comment (what, where, why)   */
/* --------|----------------|--------------------|--------------------------- */
/* 13/09/06| Ting.Li        | FR-488341        | Support the magic key *#3228#*/
/* 14/18/03| Yongliang.wang | PR-615674        | distinguish area latam or emea*/
/*---------|----------------|--------------------|--------------------------- */
/******************************************************************************/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/device.h>
#include <mach/msm_smem.h>
#include <linux/gpio.h>

#include "oemver.h"
#include <linux/miscdevice.h>

static int oemver_open(struct inode* inode, struct file* filp) {
    return 0;
}

static int oemver_release(struct inode* inode, struct file* filp) {
    return 0;
}

static ssize_t oemver_read(struct file* filp, char __user *buf, size_t count, loff_t* f_pos) {
    struct image_version_entry * ptable = NULL;
    int len = SMEM_IMAGE_VERSION_TABLE_SIZE;
    int num = count < IMAGE_OEM_VERSION_STRING_LENGTH ? count : IMAGE_OEM_VERSION_STRING_LENGTH;

    ptable = (struct image_version_entry *)
        (smem_get_entry(SMEM_IMAGE_VERSION_TABLE, &len));

    ptable += IMAGE_INDEX_MPSS;

    if (ptable)
        memcpy(buf, ptable->image_oem_version_string,num);

    printk(KERN_INFO"%s, image_oem_version_string=%s\n", __func__, ptable->image_oem_version_string);
    return num;
}

static struct file_operations oemver_fops = {
    .owner = THIS_MODULE,
    .open = oemver_open,
    .release = oemver_release,
    .read = oemver_read,
};


static struct miscdevice oemver = {
    MISC_DYNAMIC_MINOR,
    "oemver",
    &oemver_fops
};

#define DEVICE_AREA_ID 56
static struct class * device_info_class;
static struct device * device_info_dev;
static int area_id = -1;
static ssize_t sys_area_info_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int ret;
    ret = snprintf(buf, 50, "%d\n", area_id);
    printk(">- %s = %s", __func__, buf);
    return ret;
}
static int get_area_id(void)
{

    int rc;
    rc = gpio_request(DEVICE_AREA_ID, "device_area_id");
    if(rc){
        printk(">- request gpio56 failed, rc=%d\n", rc);
		goto err;
    }
    rc = gpio_tlmm_config(GPIO_CFG(
			DEVICE_AREA_ID, 0,
			GPIO_CFG_INPUT,
			GPIO_CFG_NO_PULL,
			GPIO_CFG_2MA),
			GPIO_CFG_ENABLE);
	if (rc) {
		printk(">- %s: unable to config tlmm = %d\n",
			__func__, DEVICE_AREA_ID);
		goto err;
	}

	rc = gpio_direction_input(DEVICE_AREA_ID);
	if (rc) {
		printk(">- set_direction for gpio56 failed, rc=%d\n",
		       rc);
		goto err;
	}
    area_id = gpio_get_value(DEVICE_AREA_ID);
	printk(">- area_id = %d \n", area_id);

	gpio_free(DEVICE_AREA_ID);
	return 1;

err:
	gpio_free(DEVICE_AREA_ID);
	return -ENODEV;
}
static DEVICE_ATTR(area_id_info, 0644, sys_area_info_show, NULL);

static int __init oemver_init(void){

    int ret;

    ret = misc_register(&oemver);
    if (ret) {
        printk(KERN_ERR "fram: can't misc_register on minor=%d\n",
            MISC_DYNAMIC_MINOR);
        return ret;
    }

	get_area_id();

	device_info_class = class_create(THIS_MODULE, "area_info");
    if (IS_ERR(device_info_class))
        pr_err("Failed to create class(device_info_class))!\n");
    device_info_dev = device_create(device_info_class, NULL, 0, NULL, "area_id");
    if (IS_ERR(device_info_dev))
        pr_err("Failed to create device(lcd_ce_ctrl)!\n");

    if (device_create_file(device_info_dev, &dev_attr_area_id_info) < 0)
       pr_err("Failed to create device file(%s)!\n", dev_attr_area_id_info.attr.name);

//	dev_set_drvdata(device_info_dev, pdev);

    printk(KERN_INFO "oemver driver init success");
    return ret;
}


static void __exit oemver_exit(void) {

    printk(KERN_ALERT"Destroy oemver device.\n");
    misc_deregister(&oemver);
}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("oemver Driver");

module_init(oemver_init);
module_exit(oemver_exit);











