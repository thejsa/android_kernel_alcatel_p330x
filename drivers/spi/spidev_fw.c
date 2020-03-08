/*
 * Simple synchronous userspace interface to SPI devices
 *
 * Copyright (C) 2006 SWAPP
 *	Andrea Paterniani <a.paterniani@swapp-eng.it>
 * Copyright (C) 2007 David Brownell (simplification, cleanup)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/compat.h>

#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#include <linux/delay.h>

#include <linux/gpio.h>
#include <linux/regulator/consumer.h>

#include <asm/uaccess.h>

//*******add by pu.li@tcl.com on 2014.4.28*********
#include <linux/clk.h>
//*******end of add*******

#define ICE40_FIRMWARE_DL 

//*********add by pu.li@tcl.com on 2014.4..28******
//#define ICE40_V_L5_1P2_ENABLE
#define ICE40_192_CLOCK_CTROL
//*********end of add*********

#ifdef ICE40_FIRMWARE_DL
#include "pop_lte.h"
#endif
/*
 * This supports access to SPI devices using normal userspace I/O calls.
 * Note that while traditional UNIX/POSIX I/O semantics are half duplex,
 * and often mask message boundaries, full SPI support requires full duplex
 * transfers.  There are several kinds of internal message boundaries to
 * handle chipselect management and other protocol options.
 *
 * SPI has a character major number assigned.  We allocate minor numbers
 * dynamically using a bitmask.  You must use hotplug tools, such as udev
 * (or mdev with busybox) to create and destroy the /dev/spidevB.C device
 * nodes, since there is no fixed association of minor numbers with any
 * particular SPI bus or device.
 */
#define SPIDEV_MAJOR			153	/* assigned */
#define N_SPI_MINORS			32	/* ... up to 256 */

static DECLARE_BITMAP(minors, N_SPI_MINORS);

#define SPI_CS_LAT 2
#define SPI_SCK_LAT 3
#define SPI_MO_LAT 0
#define SPI_MI_LAT 1
#define C_RESET_LAT 108

/* Bit masks for spi_device.mode management.  Note that incorrect
 * settings for some settings can cause *lots* of trouble for other
 * devices on a shared bus:
 *
 *  - CS_HIGH ... this device will be active when it shouldn't be
 *  - 3WIRE ... when active, it won't behave as it should
 *  - NO_CS ... there will be no explicit message boundaries; this
 *	is completely incompatible with the shared bus model
 *  - READY ... transfers may proceed when they shouldn't.
 *
 * REVISIT should changing those flags be privileged?
 */
#define SPI_MODE_MASK		(SPI_CPHA | SPI_CPOL | SPI_CS_HIGH \
				| SPI_LSB_FIRST | SPI_3WIRE | SPI_LOOP \
				| SPI_NO_CS | SPI_READY)

struct spidev_data {
	dev_t			devt;
	spinlock_t		spi_lock;
	struct spi_device	*spi;
	struct list_head	device_entry;

	/* buffer is NULL unless this device is open (users > 0) */
	struct mutex		buf_lock;
	unsigned		users;
	u8			*buffer;
	u8			*bufferrx;
};

#ifdef ICE40_FIRMWARE_DL

static struct spidev_data *ice40_g_spidev = NULL;
static u8 *frameware_bin ;
int img_size = ARRAY_SIZE(ice40_bin);

#endif

static LIST_HEAD(device_list);
static DEFINE_MUTEX(device_list_lock);

static unsigned bufsiz = 4096;
module_param(bufsiz, uint, S_IRUGO);
MODULE_PARM_DESC(bufsiz, "data bytes in biggest supported SPI message");

/*
 * This can be used for testing the controller, given the busnum and the
 * cs required to use. If those parameters are used, spidev is
 * dynamically added as device on the busnum, and messages can be sent
 * via this interface.
 */
static int busnum = 0;
module_param(busnum, int, S_IRUGO);
MODULE_PARM_DESC(busnum, "bus num of the controller");

static int chipselect = 0;
module_param(chipselect, int, S_IRUGO);
MODULE_PARM_DESC(chipselect, "chip select of the desired device");

static int maxspeed = 960000;
module_param(maxspeed, int, S_IRUGO);
MODULE_PARM_DESC(maxspeed, "max_speed of the desired device");

static int spimode = SPI_MODE_3;
module_param(spimode, int, S_IRUGO);
MODULE_PARM_DESC(spimode, "mode of the desired device");

static struct spi_device *spi;

//***********add by pu.li@tcl.com on 2014.4.28**********
#ifdef ICE40_192_CLOCK_CTROL
struct device ice_dev_src = {.init_name = "ice_mmss_clk",};
struct device ice_dev = {.init_name = "ice_camss_clk",};
struct device ice_dev_xo = {.init_name = "ice_xo_clk",};
static struct clk * ice40_xo_clk = NULL;
static struct clk * ice40_mmss_clk = NULL;
static struct clk * ice40_camss_clk = NULL;
#endif //ICE40_192_CLOCK_CTROL
//***********end of add***********

/*--------------------------------add by xiaoyong.wu---------------------------------------*/

static ssize_t spidev_sync(struct spidev_data *spidev, struct spi_message *message);

//*******modify by pu.li@tcl.com on 2014.4.28*******
#ifdef ICE40_V_L5_1P2_ENABLE
static void ice40_source_func(int enable)
{
	int ret;
	struct regulator *vdd_lte;

	vdd_lte = regulator_get(NULL , "vdd_lte");
	if (!IS_ERR(vdd_lte)) {
		if (enable)
			ret = regulator_enable(vdd_lte);
		else
			ret = regulator_disable(vdd_lte);
		if (ret)
			printk("wxy------enable <%d> 1.2v error--------\n",enable);
		}
}
#endif //ICE40_V_L5_1P2_ENABLE
//******end of modify******/

#ifdef ICE40_FIRMWARE_DL
static int ice40_firmware_spi_DMA_transfer(void)
{
	struct spidev_data *spidev = ice40_g_spidev;
	struct spi_message ice40_msg;
	struct spi_transfer ice40_transfer;
	int ret = 0;
	memset(&ice40_transfer, 0, sizeof(ice40_transfer));
	spi_message_init(&ice40_msg);


	ice40_transfer.tx_buf = frameware_bin;
	ice40_transfer.rx_buf = NULL;
	ice40_transfer.len = img_size;

	spi_message_add_tail(&ice40_transfer, &ice40_msg);

	ice40_msg.spi = spidev->spi;
	ret = spidev_sync(spidev, &ice40_msg);
	if (ret < 0)
	{
		return -1;
	}
	return 0;
}

static int ice40_download_firmware(void)
{
	int ret = 0;

 	printk("wxy-----------down firmware-------\n");
 	gpio_request(SPI_CS_LAT,"cs down");
	gpio_tlmm_config(GPIO_CFG(SPI_CS_LAT, 0, GPIO_CFG_OUTPUT,GPIO_CFG_PULL_DOWN
, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_direction_output(SPI_CS_LAT, 0);
	
	udelay(100);
	gpio_direction_output(C_RESET_LAT, 0);
	udelay(1000);//200 ns at least
	gpio_direction_output(C_RESET_LAT, 1);
    udelay(1000);//800 ns at least

	gpio_tlmm_config(GPIO_CFG(SPI_CS_LAT, 1, GPIO_CFG_OUTPUT,GPIO_CFG_NO_PULL
, GPIO_CFG_6MA), GPIO_CFG_ENABLE);
	gpio_free(SPI_CS_LAT);
	
	ret = ice40_firmware_spi_DMA_transfer();
	if (ret < 0)
	{
		printk("[wxy] ~~ firmware download fail\n");
	}
	return ret;
}
#endif
/*----------------------------add end by xiaoyong.wu--------------------------------*/

/*
 * We can't use the standard synchronous wrappers for file I/O; we
 * need to protect against async removal of the underlying spi_device.
 */
static void spidev_complete(void *arg)
{
	complete(arg);
}

static ssize_t
spidev_sync(struct spidev_data *spidev, struct spi_message *message)
{
	DECLARE_COMPLETION_ONSTACK(done);
	int status;

	message->complete = spidev_complete;
	message->context = &done;

	spin_lock_irq(&spidev->spi_lock);
	if (spidev->spi == NULL)
		status = -ESHUTDOWN;
	else
		status = spi_async(spidev->spi, message);
	spin_unlock_irq(&spidev->spi_lock);

	if (status == 0) {
		wait_for_completion(&done);
		status = message->status;
		if (status == 0)
			status = message->actual_length;
	}
	return status;
}

static inline ssize_t
spidev_sync_write(struct spidev_data *spidev, size_t len)
{
	struct spi_transfer	t = {
			.tx_buf		= spidev->buffer,
			.len		= len,
		};
	struct spi_message	m;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	return spidev_sync(spidev, &m);
}

static inline ssize_t
spidev_sync_read(struct spidev_data *spidev, size_t len)
{
	struct spi_transfer	t = {
			.rx_buf		= spidev->buffer,
			.len		= len,
		};
	struct spi_message	m;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);
	return spidev_sync(spidev, &m);
}

/*-------------------------------------------------------------------------*/

/* Read-only message with current device setup */
static ssize_t
spidev_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	struct spidev_data	*spidev;
	ssize_t			status = 0;

	/* chipselect only toggles at start or end of operation */
	if (count > bufsiz)
		return -EMSGSIZE;

	spidev = filp->private_data;

	mutex_lock(&spidev->buf_lock);
	status = spidev_sync_read(spidev, count);
	if (status > 0) {
		unsigned long	missing;

		missing = copy_to_user(buf, spidev->buffer, status);
		if (missing == status)
			status = -EFAULT;
		else
			status = status - missing;
	}
	mutex_unlock(&spidev->buf_lock);

	return status;
}

/* Write-only message with current device setup */
static ssize_t
spidev_write(struct file *filp, const char __user *buf,
		size_t count, loff_t *f_pos)
{
	struct spidev_data	*spidev;
	ssize_t			status = 0;
	unsigned long		missing;

	/* chipselect only toggles at start or end of operation */
	if (count > bufsiz)
		return -EMSGSIZE;

	spidev = filp->private_data;

	mutex_lock(&spidev->buf_lock);
	missing = copy_from_user(spidev->buffer, buf, count);
	if (missing == 0) {
		status = spidev_sync_write(spidev, count);
	} else
		status = -EFAULT;
	mutex_unlock(&spidev->buf_lock);

	return status;
}

static int spidev_message(struct spidev_data *spidev,
		struct spi_ioc_transfer *u_xfers, unsigned n_xfers)
{
	struct spi_message	msg;
	struct spi_transfer	*k_xfers;
	struct spi_transfer	*k_tmp;
	struct spi_ioc_transfer *u_tmp;
	unsigned		n, total;
	u8			*buf, *bufrx;
	int			status = -EFAULT;

	spi_message_init(&msg);
	k_xfers = kcalloc(n_xfers, sizeof(*k_tmp), GFP_KERNEL);
	if (k_xfers == NULL)
		return -ENOMEM;

	/* Construct spi_message, copying any tx data to bounce buffer.
	 * We walk the array of user-provided transfers, using each one
	 * to initialize a kernel version of the same transfer.
	 */
	buf = spidev->buffer;
	bufrx = spidev->bufferrx;
	total = 0;
	for (n = n_xfers, k_tmp = k_xfers, u_tmp = u_xfers;
			n;
			n--, k_tmp++, u_tmp++) {
		k_tmp->len = u_tmp->len;

		total += k_tmp->len;
		if (total > bufsiz) {
			status = -EMSGSIZE;
			goto done;
		}

		if (u_tmp->rx_buf) {
			k_tmp->rx_buf = bufrx;
			if (!access_ok(VERIFY_WRITE, (u8 __user *)
						(uintptr_t) u_tmp->rx_buf,
						u_tmp->len))
				goto done;
		}
		if (u_tmp->tx_buf) {
			k_tmp->tx_buf = buf;
			if (copy_from_user(buf, (const u8 __user *)
						(uintptr_t) u_tmp->tx_buf,
					u_tmp->len))
				goto done;
		}
		buf += k_tmp->len;
		bufrx += k_tmp->len;
		u_tmp->speed_hz = 960000;
		k_tmp->cs_change = !!u_tmp->cs_change;
		k_tmp->bits_per_word = u_tmp->bits_per_word;
		k_tmp->delay_usecs = u_tmp->delay_usecs;
		k_tmp->speed_hz = u_tmp->speed_hz;
//#ifdef VERBOSE
		dev_err(&spidev->spi->dev,
			"  xfer len %zd %s%s%s%dbits %u usec %uHz\n",
			u_tmp->len,
			u_tmp->rx_buf ? "rx " : "",
			u_tmp->tx_buf ? "tx " : "",
			u_tmp->cs_change ? "cs " : "",
			u_tmp->bits_per_word ? : spidev->spi->bits_per_word,
			u_tmp->delay_usecs,
			u_tmp->speed_hz ? : spidev->spi->max_speed_hz);
//#endif
		spi_message_add_tail(k_tmp, &msg);
	}

	status = spidev_sync(spidev, &msg);
	if (status < 0)
		goto done;

	/* copy any rx data out of bounce buffer */
	buf = spidev->bufferrx;
	for (n = n_xfers, u_tmp = u_xfers; n; n--, u_tmp++) {
		if (u_tmp->rx_buf) {
			if (__copy_to_user((u8 __user *)
					(uintptr_t) u_tmp->rx_buf, buf,
					u_tmp->len)) {
				status = -EFAULT;
				goto done;
			}
		}
		buf += u_tmp->len;
	}
	status = total;

done:
	kfree(k_xfers);
	return status;
}

static long
spidev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int			err = 0;
	int			retval = 0;
	struct spidev_data	*spidev;
	struct spi_device	*spi;
	u32			tmp;
	unsigned		n_ioc;
	struct spi_ioc_transfer	*ioc;

	/* Check type and command number */
	if (_IOC_TYPE(cmd) != SPI_IOC_MAGIC)
		return -ENOTTY;

	/* Check access direction once here; don't repeat below.
	 * IOC_DIR is from the user perspective, while access_ok is
	 * from the kernel perspective; so they look reversed.
	 */
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE,
				(void __user *)arg, _IOC_SIZE(cmd));
	if (err == 0 && _IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ,
				(void __user *)arg, _IOC_SIZE(cmd));
	if (err)
		return -EFAULT;

	/* guard against device removal before, or while,
	 * we issue this ioctl.
	 */
	spidev = filp->private_data;
	spin_lock_irq(&spidev->spi_lock);
	spi = spi_dev_get(spidev->spi);
	spin_unlock_irq(&spidev->spi_lock);

	if (spi == NULL)
		return -ESHUTDOWN;

	/* use the buffer lock here for triple duty:
	 *  - prevent I/O (from us) so calling spi_setup() is safe;
	 *  - prevent concurrent SPI_IOC_WR_* from morphing
	 *    data fields while SPI_IOC_RD_* reads them;
	 *  - SPI_IOC_MESSAGE needs the buffer locked "normally".
	 */
	mutex_lock(&spidev->buf_lock);

	switch (cmd) {
	/* read requests */
	case SPI_IOC_RD_MODE:
              printk("[wxy] --- SPI_IOC_RD_MODE = %d\n", cmd);
		retval = __put_user(spi->mode & SPI_MODE_MASK,
					(__u8 __user *)arg);
		break;
	case SPI_IOC_RD_LSB_FIRST:
              printk("[wxy] --- SPI_IOC_RD_LSB_FIRST = %d\n", cmd);
		retval = __put_user((spi->mode & SPI_LSB_FIRST) ?  1 : 0,
					(__u8 __user *)arg);
		break;
	case SPI_IOC_RD_BITS_PER_WORD:
              printk("[wxy] --- SPI_IOC_RD_BITS_PER_WORD = %d\n", cmd);
		retval = __put_user(spi->bits_per_word, (__u8 __user *)arg);
		break;
	case SPI_IOC_RD_MAX_SPEED_HZ:
              printk("[wxy] --- SPI_IOC_RD_MAX_SPEED_HZ = %d\n", cmd);
		retval = __put_user(spi->max_speed_hz, (__u32 __user *)arg);
		break;

	/* write requests */
	case SPI_IOC_WR_MODE:
              printk("[wxy] --- SPI_IOC_WR_MODE = %d\n", cmd);
		retval = __get_user(tmp, (u8 __user *)arg);
		if (retval == 0) {
			u8	save = spi->mode;

			if (tmp & ~SPI_MODE_MASK) {
				retval = -EINVAL;
				break;
			}

			tmp |= spi->mode & ~SPI_MODE_MASK;
			//spi->mode = (u8)tmp;            when testing ,disable mode from APK  by wxy
			spi->mode = 1;
			retval = spi_setup(spi);
			if (retval < 0)
				spi->mode = save;
			else
				dev_dbg(&spi->dev, "spi mode %02x\n", tmp);
		}
		break;
	case SPI_IOC_WR_LSB_FIRST:
              printk("[wxy] --- SPI_IOC_WR_LSB_FIRST = %d\n", cmd);
		retval = __get_user(tmp, (__u8 __user *)arg);
		if (retval == 0) {
			u8	save = spi->mode;

			if (tmp)
				spi->mode |= SPI_LSB_FIRST;
			else
				spi->mode &= ~SPI_LSB_FIRST;
			retval = spi_setup(spi);
			if (retval < 0)
				spi->mode = save;
			else
				dev_dbg(&spi->dev, "%csb first\n",
						tmp ? 'l' : 'm');
		}
		break;
	case SPI_IOC_WR_BITS_PER_WORD:
              printk("[wxy] --- SPI_IOC_WR_BITS_PER_WORD = %d\n", cmd);
		retval = __get_user(tmp, (__u8 __user *)arg);
		if (retval == 0) {
			u8	save = spi->bits_per_word;

			spi->bits_per_word = tmp;
			retval = spi_setup(spi);
			if (retval < 0)
				spi->bits_per_word = save;
			else
				dev_dbg(&spi->dev, "%d bits per word\n", tmp);
		}
		break;
	case SPI_IOC_WR_MAX_SPEED_HZ:
              printk("[wxy] --- SPI_IOC_WR_MAX_SPEED_HZ = %d\n", cmd);
		retval = __get_user(tmp, (__u32 __user *)arg);
		if (retval == 0) {
			u32	save = spi->max_speed_hz;

			spi->max_speed_hz = 960000;         // when testing . disable 10K set from apk  by wxy
			retval = spi_setup(spi);
			if (retval < 0)
				spi->max_speed_hz = save;
			else
				dev_dbg(&spi->dev, "%d Hz (max)\n", tmp);
		}
		break;

	default:
              printk("[wxy] --- default = %d\n", cmd);
		/* segmented and/or full-duplex I/O request */
		if (_IOC_NR(cmd) != _IOC_NR(SPI_IOC_MESSAGE(0))
				|| _IOC_DIR(cmd) != _IOC_WRITE) {
			retval = -ENOTTY;
			break;
		}

		tmp = _IOC_SIZE(cmd);
		if ((tmp % sizeof(struct spi_ioc_transfer)) != 0) {
			retval = -EINVAL;
			break;
		}
		n_ioc = tmp / sizeof(struct spi_ioc_transfer);
		if (n_ioc == 0)
			break;

		/* copy into scratch area */
		ioc = kmalloc(tmp, GFP_KERNEL);
		if (!ioc) {
			retval = -ENOMEM;
			break;
		}
		if (__copy_from_user(ioc, (void __user *)arg, tmp)) {
			kfree(ioc);
			retval = -EFAULT;
			break;
		}

		/* translate to spi_message, execute */
		printk("[wxy] --- spidev_message = \n");
		retval = spidev_message(spidev, ioc, n_ioc);
		kfree(ioc);
		break;
	}

	mutex_unlock(&spidev->buf_lock);
	spi_dev_put(spi);
	return retval;
}

#ifdef CONFIG_COMPAT
static long
spidev_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	return spidev_ioctl(filp, cmd, (unsigned long)compat_ptr(arg));
}
#else
#define spidev_compat_ioctl NULL
#endif /* CONFIG_COMPAT */

static int spidev_open(struct inode *inode, struct file *filp)
{
	struct spidev_data	*spidev;
	int			ret,status = -ENXIO;
	//**********modify by pu.li@tcl.com on 2014.4.25*********
#ifdef ICE40_V_L5_1P2_ENABLE
	printk("wxy-------powerOn L5 1.2v-------------\n");
	ice40_source_func(1);
#endif //ICE40_V_L5_1P2_ENABLE
	//***********end of modify********/
	mutex_lock(&device_list_lock);

	list_for_each_entry(spidev, &device_list, device_entry) {
		if (spidev->devt == inode->i_rdev) {
			status = 0;
			break;
		}
	}
	if (status == 0) {
		if (!spidev->buffer) {
			spidev->buffer = kmalloc(bufsiz, GFP_KERNEL);
			if (!spidev->buffer) {
				printk("kmalloc fail\n");
				dev_dbg(&spidev->spi->dev, "open/ENOMEM\n");
				status = -ENOMEM;
			}
		}
		if (!spidev->bufferrx) {
			spidev->bufferrx = kmalloc(bufsiz, GFP_KERNEL);
			if (!spidev->bufferrx) {
				dev_dbg(&spidev->spi->dev, "open/ENOMEM\n");
				kfree(spidev->buffer);
				spidev->buffer = NULL;
				status = -ENOMEM;
			}
		}
		if (status == 0) {
			spidev->users++;
			filp->private_data = spidev;
			nonseekable_open(inode, filp);
		}
	} else
		pr_debug("spidev: nothing for minor %d\n", iminor(inode));

	mutex_unlock(&device_list_lock);
#ifdef ICE40_FIRMWARE_DL
	ice40_g_spidev = spidev;

	spidev->spi->mode = SPI_MODE_3;
	spi_setup(spidev->spi);
	mutex_lock(&spidev->buf_lock);
	ret = ice40_download_firmware();
	if (ret == 0)
	{
		if(!gpio_get_value(12)){
			printk("wxy------down FW first time error------------\n");
			ice40_download_firmware();
			}
		//here, maybe we need check cdone pin gpio 12 to check the real chip status
		printk("wxy------down FW ok------------\n");
	}
	mutex_unlock(&spidev->buf_lock);
#endif	
	//*******trun on the 19.2Mhz clock, add by pu.li@tcl.com on 2014.4.28*******
#ifdef ICE40_192_CLOCK_CTROL
	if( !IS_ERR( ice40_xo_clk ) )
	{
		ret = clk_prepare_enable(ice40_xo_clk);
		if( ret )
		{
			printk( "~~~~~~~~Enable xo clock failed!~~~~~~~~\n" );
			return ret;
		}
		else
		{
			printk( "~~~~~~~~Enable xo clock succesfully!~~~~~~~~\n" );
			clk_set_rate(ice40_mmss_clk,  19200000);
				
			if( !IS_ERR( ice40_mmss_clk ) )
			{
				ret = clk_prepare_enable(ice40_mmss_clk);
				if( ret )
				{
					printk( "~~~~~~~~Enable src clock failed!~~~~~~~~\n" );
					return ret;
				}
				else
				{
					printk( "~~~~~~~~Enable src clock succesfully!~~~~~~~~\n" );
					if( !IS_ERR( ice40_camss_clk ) )
					{
						ret = clk_prepare_enable(ice40_camss_clk);
						if( ret )
						{
							printk( "~~~~~~~~Enable 19.2Mhz clock failed!~~~~~~~~\n" );
						}
						else
							printk( "~~~~~~~~Enable 19.2Mhz clock succesfully!~~~~~~~~\n" );
					}
				}
			}	
		}
	}
#endif //ICE40_192_CLOCK_CTROL
	//*******end of add******
	return status;
}

static int spidev_release(struct inode *inode, struct file *filp)
{
	struct spidev_data	*spidev;
	int			status = 0;

	mutex_lock(&device_list_lock);
	spidev = filp->private_data;
	filp->private_data = NULL;
	/* last close? */
	spidev->users--;
	if (!spidev->users) {
		int		dofree;

		kfree(spidev->buffer);
		spidev->buffer = NULL;
		kfree(spidev->bufferrx);
		spidev->bufferrx = NULL;

		/* ... after we unbound from the underlying device? */
		spin_lock_irq(&spidev->spi_lock);
		dofree = (spidev->spi == NULL);
		spin_unlock_irq(&spidev->spi_lock);

		if (dofree)
			kfree(spidev);
		//*********modify by pu.li@tcl.com on 2014.4.25*******
#ifdef ICE40_V_L5_1P2_ENABLE
		printk("wxy-------powerDown L5 1.2v-------------\n");
		ice40_source_func(0);
#endif //ICE40_V_L5_1P2_ENABLE
		//**********end of modify**********/
	}
	mutex_unlock(&device_list_lock);
	//********turn off the 19.2Mhz clock, add by pu.li@tcl.com on 2014.4.28******
#ifdef ICE40_192_CLOCK_CTROL
	if( !IS_ERR( ice40_camss_clk) )
	{
		printk( "~~~~~~~~Disable 19.2Mhz clock!~~~~~~~~\n" );
		clk_disable_unprepare(ice40_camss_clk);
	}
	else
		printk( "~~~~~~~~Disable 19.2Mhz clock failed!~~~~~~~~\n" );

	
	if( !IS_ERR( ice40_mmss_clk) )
	{
		printk( "~~~~~~~~Disable src clock!~~~~~~~~\n" );
		clk_disable_unprepare(ice40_mmss_clk);
	}
	else
		printk( "~~~~~~~~Disable src clock failed!~~~~~~~~\n" );
	
	if( !IS_ERR( ice40_xo_clk) )
	{
		printk( "~~~~~~~~Disable xo clock!~~~~~~~~\n" );
		clk_disable_unprepare(ice40_xo_clk);
	}
	else
		printk( "~~~~~~~~Disable xo clock failed!~~~~~~~~\n" );
#endif //ICE40_192_CLOCK_CTROL
	//********end of modify********
	//********add by pu.li@tcl.com on 2014.5.7,pull down the reset gpio*********
	gpio_direction_output(C_RESET_LAT, 0);
	//********end of modify********
	return status;
}

static const struct file_operations spidev_fops = {
	.owner =	THIS_MODULE,
	/* REVISIT switch to aio primitives, so that userspace
	 * gets more complete API coverage.  It'll simplify things
	 * too, except for the locking.
	 */
	.write =	spidev_write,
	.read =		spidev_read,
	.unlocked_ioctl = spidev_ioctl,
	.compat_ioctl = spidev_compat_ioctl,
	.open =		spidev_open,
	.release =	spidev_release,
	.llseek =	no_llseek,
};

/*-------------------------------------------------------------------------*/

/* The main reason to have this class is to make mdev/udev create the
 * /dev/spidevB.C character device nodes exposing our userspace API.
 * It also simplifies memory management.
 */

static struct class *spidev_class;

/*-------------------------------------------------------------------------*/

static int __devinit spidev_probe(struct spi_device *spi)
{
	struct spidev_data	*spidev;
	int			status;
	unsigned long		minor;
//*********add by pu.li@tcl.com on 2014.5.1*******
#ifdef ICE40_192_CLOCK_CTROL
	int ret;
#endif //ICE40_192_CLOCK_CTROL
//*********end of add*********
	
	gpio_request(C_RESET_LAT,"C_RESET_LAT");
	/* Allocate driver data */
	spidev = kzalloc(sizeof(*spidev), GFP_KERNEL);
	if (!spidev)
		return -ENOMEM;

	/* Initialize the driver data */
	spidev->spi = spi;
	spin_lock_init(&spidev->spi_lock);
	mutex_init(&spidev->buf_lock);

	INIT_LIST_HEAD(&spidev->device_entry);

	/* If we can allocate a minor number, hook up this device.
	 * Reusing minors is fine so long as udev or mdev is working.
	 */
	mutex_lock(&device_list_lock);
	minor = find_first_zero_bit(minors, N_SPI_MINORS);
	if (minor < N_SPI_MINORS) {
		struct device *dev;

		spidev->devt = MKDEV(SPIDEV_MAJOR, minor);
		dev = device_create(spidev_class, &spi->dev, spidev->devt,
				    spidev, "spidev%d.%d",
				    spi->master->bus_num, spi->chip_select);
		status = IS_ERR(dev) ? PTR_ERR(dev) : 0;
	} else {
		dev_dbg(&spi->dev, "no minor number available!\n");
		status = -ENODEV;
	}
	if (status == 0) {
		set_bit(minor, minors);
		list_add(&spidev->device_entry, &device_list);
	}
	mutex_unlock(&device_list_lock);
	
	//********modify by pu.li@tcl.com on 2014.4.28,get the 19.2Mhz clock*******
#ifdef ICE40_192_CLOCK_CTROL
	ice40_xo_clk = clk_get( &ice_dev_xo, "ice40_xo" );
	if( IS_ERR(ice40_xo_clk) )
	{
		ret = PTR_ERR( ice40_xo_clk );
		printk( "~~~~~~~~Can not get the xo clock!~~~~~~~~\n" );
		return ret;
	}
	else
	{
		printk( "~~~~~~~~Get the xo clock succesfully!~~~~~~~~\n" );
		ice40_mmss_clk = clk_get( &ice_dev_src, "ice40_mmss" );
		if( IS_ERR(ice40_mmss_clk) )
		{
			ret = PTR_ERR( ice40_mmss_clk );
			printk( "~~~~~~~~Can not get the src clock!~~~~~~~~\n" );
			return ret;
		}
		else
		{
			printk( "~~~~~~~~Get the src clock succesfully!~~~~~~~~\n" );
			ice40_camss_clk = clk_get( &ice_dev, "ice40_camss" );
			if( IS_ERR(ice40_camss_clk) )
			{
				ret = PTR_ERR( ice40_camss_clk );
				printk( "~~~~~~~~Can not get the 19.2Mhz clock!~~~~~~~~\n" );
				return ret;
			}
			else
				printk( "~~~~~~~~Get the 19.2Mhz clock succesfully!~~~~~~~~\n" );
		}
	}
#endif //ICE40_192_CLOCK_CTROL
	//********end of modify*******
	
	if (status == 0)
		spi_set_drvdata(spi, spidev);
	else
		kfree(spidev);
	return status;
}

static int __devexit spidev_remove(struct spi_device *spi)
{
	struct spidev_data	*spidev = spi_get_drvdata(spi);

	/* make sure ops on existing fds can abort cleanly */
	spin_lock_irq(&spidev->spi_lock);
	spidev->spi = NULL;
	spi_set_drvdata(spi, NULL);
	spin_unlock_irq(&spidev->spi_lock);

	/* prevent new opens */
	mutex_lock(&device_list_lock);
	list_del(&spidev->device_entry);
	device_destroy(spidev_class, spidev->devt);
	clear_bit(MINOR(spidev->devt), minors);
	if (spidev->users == 0)
		kfree(spidev);
	gpio_free(C_RESET_LAT);         // add by wxy for reset free
	mutex_unlock(&device_list_lock);

	return 0;
}

static struct spi_driver spidev_spi_driver = {
	.driver = {
		.name =		"spidev",
		.owner =	THIS_MODULE,
	},
	.probe =	spidev_probe,
	.remove =	__devexit_p(spidev_remove),

	/* NOTE:  suspend/resume methods are not necessary here.
	 * We don't do anything except pass the requests to/from
	 * the underlying controller.  The refrigerator handles
	 * most issues; the controller driver handles the rest.
	 */
};

/*-------------------------------------------------------------------------*/

static int __init spidev_init(void)
{
	int status;

	/* Claim our 256 reserved device numbers.  Then register a class
	 * that will key udev/mdev to add/remove /dev nodes.  Last, register
	 * the driver which manages those device numbers.
	 */
	BUILD_BUG_ON(N_SPI_MINORS > 256);
	status = register_chrdev(SPIDEV_MAJOR, "spi", &spidev_fops);
	if (status < 0)
		return status;

	spidev_class = class_create(THIS_MODULE, "spidev");
	if (IS_ERR(spidev_class)) {
		status = PTR_ERR(spidev_class);
		goto error_class;
	}

	status = spi_register_driver(&spidev_spi_driver);
	if (status < 0)
		goto error_register;

	if (busnum != -1 && chipselect != -1) {
		struct spi_board_info chip = {
					.modalias	= "spidev",
					.mode		= spimode,
					.bus_num	= busnum,
					.chip_select	= chipselect,
					.max_speed_hz	= maxspeed,
		};

		struct spi_master *master;

		master = spi_busnum_to_master(busnum);
		if (!master) {
			status = -ENODEV;
			goto error_busnum;
		}

		/* We create a virtual device that will sit on the bus */
		spi = spi_new_device(master, &chip);
		if (!spi) {
			status = -EBUSY;
			goto error_mem;
		}
		dev_dbg(&spi->dev, "busnum=%d cs=%d bufsiz=%d maxspeed=%d",
			busnum, chipselect, bufsiz, maxspeed);
	}
	
#ifdef ICE40_FIRMWARE_DL
	frameware_bin = kmalloc(img_size, GFP_KERNEL);
	if (!frameware_bin){
		printk("[wxy]---kmalloc for fw fail -----\n");
		status = -ENOMEM;
		goto error_mem;
	}
	memcpy(frameware_bin,&(ice40_bin[0]),img_size);
#endif
	return 0;
error_mem:
error_busnum:
	spi_unregister_driver(&spidev_spi_driver);
error_register:
	class_destroy(spidev_class);
error_class:
	unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver.driver.name);
	return status;
}
late_initcall(spidev_init);

static void __exit spidev_exit(void)
{
	if (spi) {
		spi_unregister_device(spi);
		spi = NULL;
	}
	spi_unregister_driver(&spidev_spi_driver);
	class_destroy(spidev_class);
	unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver.driver.name);

}
module_exit(spidev_exit);

MODULE_AUTHOR("Andrea Paterniani, <a.paterniani@swapp-eng.it>");
MODULE_DESCRIPTION("User mode SPI device interface");
MODULE_LICENSE("GPL");
MODULE_ALIAS("spi:spidev");
