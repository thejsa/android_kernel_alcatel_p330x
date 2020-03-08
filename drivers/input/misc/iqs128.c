/*
 *  iqs128.c - Linux kernel modules for Azoteq IQS128000E0TSR proximity(touch IC) sensor
 *
 *  Copyright (C) 2012~2014 junfeng.zhou / tcl <junfeng.zhou.sz@tcl.com>
 *
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/debugfs.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/wakelock.h>
#include <linux/mutex.h>
#include <linux/switch.h>
#define DRIVER_VERSION  "1.0.0 20140210"

#define GPIO_PS 27
#define GPIO_POWER 62
#define DEVICE_NAME "IQS128_SWITCH"

int gpiops_value = -1;
int iqs_irq = -1;
int irq, err = -EIO;
int gpio_power;
struct dentry *fsinterface;
struct dentry *fsinterface1;
struct mutex io_lock;
int enable=0;


static ssize_t iqs_switch_print_name(struct switch_dev *sdev, char *buf)
{
	return sprintf(buf, "DEVICE_NAME\n");
}

static struct switch_dev iqs_switch_driver = {
		.name		= DEVICE_NAME,
		.print_name = iqs_switch_print_name,
};

//#define PS_POLL
#ifdef PS_POLL
struct wake_lock ps_nosuspend_wl;
struct wake_lock ps_wakelock;
struct work_struct ps_work;
struct workqueue_struct *ps_wq;
struct hrtimer ps_timer;
ktime_t ps_poll_delay;



static void ps_work_func(struct work_struct *work)
{
    mutex_lock(&io_lock);
    gpiops_value = gpio_get_value(GPIO_PS);
    //printk(KERN_INFO "%s,gpiops_value = %d",__func__,gpiops_value);
    mutex_unlock(&io_lock);
}

static enum hrtimer_restart ps_timer_func(struct hrtimer *timer)
{
	queue_work(ps_wq, &ps_work);
	hrtimer_forward_now(&ps_timer, ps_poll_delay);
	return HRTIMER_RESTART;
}
#endif


static int debug_data_set(void *_data, u64 val)
{
    
	return 0;
}

static int debug_data_get(void *_data, u64 *val)
{
    mutex_lock(&io_lock);
	*val = (u64)gpiops_value;
	mutex_unlock(&io_lock);
	//printk(KERN_INFO "%s gpiops_value = %d;",__func__,gpiops_value);
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(debug_data_fops, debug_data_get,
			debug_data_set, "0x%02llX\n");

static int debug_data_set1(void *_data, u64 val)
{
    int temp_val = (int)val;
    printk(KERN_INFO "%sXXX temp_val = %d",__func__,temp_val);
    mutex_lock(&io_lock);
    if(val == enable)
    {
        mutex_unlock(&io_lock);
        return 0;
    }
    if(val == 1)
    {
    	//err = gpio_direction_output(gpio_power,1);
        gpio_set_value(gpio_power,1);
#ifndef PS_POLL
        enable_irq(irq);
#endif
#ifdef PS_POLL
        hrtimer_start(&ps_timer, ps_poll_delay, HRTIMER_MODE_REL);
#endif

        enable = 1;
    }
    else if(val == 0)
    {
        enable = 0;
        //gpio_free(GPIO_POWER);
        
#ifndef PS_POLL
        disable_irq_nosync(irq);
#endif
#ifdef PS_POLL
        hrtimer_cancel(&ps_timer);
#endif
        gpio_set_value(gpio_power,0);
    }
    mutex_unlock(&io_lock);
	return 0;
}

static int debug_data_get1(void *_data, u64 *val)
{
   
	printk(KERN_INFO "%sXXX",__func__);
    mutex_lock(&io_lock);
    *val = (u64)enable;
    mutex_unlock(&io_lock);
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(debug_data_fops1, debug_data_get1,
			debug_data_set1, "%llu\n");
#ifndef PS_POLL
static irqreturn_t iqs_irq_handler(int irq, void *data)
{
    disable_irq_nosync(irq);
    mutex_lock(&io_lock);
    gpiops_value = -1;
	gpiops_value = gpio_get_value(GPIO_PS);
    //add uevent handle
    switch_set_state(&iqs_switch_driver, gpiops_value);  
	mutex_unlock(&io_lock);
	//printk(KERN_INFO "%s,gpiops_value = %d",__func__,gpiops_value);
	enable_irq(irq);
	return IRQ_HANDLED;
}
#endif


static int __init iqs128_init(void)
{
    int gpio_pin;
    irq = 0;
    printk(KERN_INFO "%s,init",__func__);
    mutex_init(&io_lock);
    gpio_pin = GPIO_PS;
    gpio_power = GPIO_POWER;
    
	err = switch_dev_register(&iqs_switch_driver);
	if (err < 0)
		goto err_switch_dev_register;

    
    err = gpio_request(gpio_power,"PS_POWER_EN");        
	if(err < 0)
	{
		printk(KERN_ERR "%s: gpio_request, err=%d", __func__, err);
		return err;
	}
	err = gpio_direction_output(gpio_power,0);
	if(err < 0)
	{
		printk(KERN_ERR "%s: gpio_direction_input, err=%d", __func__, err);
		return err;
	}		
	enable = 0;
#ifndef PS_POLL
    irq = gpio_to_irq(gpio_pin);
	iqs_irq = irq;
	printk(KERN_INFO "%s: gpio_pin #=%d, irq=%d\n",__func__, gpio_pin, irq);	
	if (irq <= 0)
	{
		printk(KERN_ERR "irq number is not specified, irq # = %d, int pin=%d\n",irq, gpio_pin);
		return irq;
	}
	
	err = gpio_request(gpio_pin,"iqs128-int");        
	if(err < 0)
	{
		printk(KERN_ERR "%s: gpio_request, err=%d", __func__, err);
		return err;
	}
	err = gpio_direction_input(gpio_pin);
	if(err < 0)
	{
		printk(KERN_ERR "%s: gpio_direction_input, err=%d", __func__, err);
		return err;
	}		
	
	err = request_any_context_irq(irq, iqs_irq_handler, IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING, DEVICE_NAME, NULL);
	if (err < 0) 
	{
		printk(KERN_WARNING "%s: request_any_context_irq(%d) failed for (%d)\n", __func__, irq, err);		
		goto err_request_any_context_irq;
	}
    disable_irq(irq);
#endif
#ifdef PS_POLL
    ps_wq = create_singlethread_workqueue("ps_wq");
	INIT_WORK(&ps_work, ps_work_func);
	hrtimer_init(&ps_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	ps_poll_delay = ns_to_ktime(200 * NSEC_PER_MSEC);
	ps_timer.function = ps_timer_func;
#endif
	//disable_irq(irq);
	//create a file(/sys/kernel/debug/iqs_gpiops) for mmitest 
	//0x00 near
	//0x01 far
	fsinterface = debugfs_create_file("iqs_gpiops", S_IFREG | S_IRUGO,NULL, NULL, &debug_data_fops);
	if (fsinterface == NULL || IS_ERR(fsinterface)) {
		pr_err("debugfs_create_file failed: rc=%ld\n", PTR_ERR(fsinterface));
		err = PTR_ERR(fsinterface);
		goto free_debug_dir;
	}
	//modify(add) by junfeng.zhou.sz for cts test with 630980 begin . 20140327 
	fsinterface1 = debugfs_create_file("iqs_enable", S_IFREG | S_IRUGO | S_IWUSR,NULL, NULL, &debug_data_fops1);
	//modify(add) by junfeng.zhou.sz for cts test with 630980 end .  
	if (fsinterface1 == NULL || IS_ERR(fsinterface1)) {
		pr_err("debugfs_create_file failed: rc=%ld\n", PTR_ERR(fsinterface1));
		err = PTR_ERR(fsinterface1);
		goto free_debug_dir1;
	}
	return 0;
free_debug_dir1:
    debugfs_remove(fsinterface1);	
free_debug_dir:
    debugfs_remove(fsinterface);
#ifndef PS_POLL    
err_request_any_context_irq:	
	gpio_free(GPIO_PS);
	free_irq(irq,NULL);
#endif
	gpio_free(GPIO_POWER);
    switch_dev_unregister(&iqs_switch_driver);
err_switch_dev_register:
	pr_err("SWITCH: Failed to register driver,IQS128\n");
	return err;
}

static void __exit iqs128_exit(void)
{
#ifndef PS_POLL
    if(iqs_irq != -1)
        free_irq(iqs_irq,NULL);
    gpio_free(GPIO_PS);
#endif
#ifdef PS_POLL
    hrtimer_cancel(&ps_timer);
#endif
    if (!(fsinterface == NULL || IS_ERR(fsinterface)))
        debugfs_remove(fsinterface);	
    
	gpio_free(GPIO_POWER);
	switch_dev_unregister(&iqs_switch_driver);
}

module_init(iqs128_init);
module_exit(iqs128_exit);
MODULE_AUTHOR("junfeng.zhou <junfeng.zhou.sz@tcl.com>");
MODULE_DESCRIPTION(" iqs128 Proximity Sensor(touch IC) driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRIVER_VERSION);
