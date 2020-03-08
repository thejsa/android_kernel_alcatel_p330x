/******************************************************************************/
/*                                                               Date:07/2013 */
/*                             PRESENTATION                                   */
/*                                                                            */
/*      Copyright 2013 TCL Communication Technology Holdings Limited.         */
/*                                                                            */
/* This material is company confidential, cannot be reproduced in any form    */
/* without the written permission of TCL Communication Technology Holdings    */
/* Limited.                                                                   */
/*       Msg21xxA serial TouchScreen driver source file.                      */
/* -------------------------------------------------------------------------- */
/* Author:  MSG21 TP supplier  Mstar                                          */
/* E-Mail:  qijun.xu@tcl-mobile.com                                           */
/* -------------------------------------------------------------------------- */
/* ========================================================================== */
/* Modifications on Features list / Changes Request / Problems Report         */
/* -------------------------------------------------------------------------- */
/* date    | author         | key                | comment (what, where, why) */
/* --------|----------------|--------------------|--------------------------- */
/* 13/07/21| TP supplier    |                    | msg21 TP orginial driver   */
/* --------|----------------|--------------------|--------------------------- */
/* 13/07/31| Qijun Xu       | bug 487707         | modify driver to meet qcom */
/*         |                |                    | style,and running          */
/*---------|----------------|--------------------|--------------------------- */
/******************************************************************************/
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/timer.h>
#include <linux/gpio.h>

#include <linux/sysfs.h>
#include <linux/proc_fs.h>
#include <linux/init.h>
#include <linux/mutex.h>
#include <mach/gpio.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/slab.h>

#include <linux/syscalls.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/string.h>
#include <asm/unistd.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/earlysuspend.h>
#include <linux/regulator/consumer.h>
#include <linux/input/msg21xx.h>
#include <linux/of_gpio.h>
#include "Rio4G_V5.05_update.h"	/*[BUGFIX]-MOD by TCTSZ.WH,03/31/2014,PR-602310*/
/*[BUGFIX]-ADD BEGIN by TCTSZ.WH,03/18/2014,PR-602310*/
#if defined(CONFIG_FB)
#include <linux/notifier.h>
#include <linux/fb.h>
#elif defined(CONFIG_HAS_EARLYSUSPEND)
#include <linux/earlysuspend.h>
#endif
/*[BUGFIX]-ADD END by TCTSZ.WH,03/18/2014,PR-602310*/
/*[BUGFIX]-MOD BEGIN by TCTSZ.WH,03/31/2014,PR-602310*/
#define MSG21XX_AUTO_UPDATE
//#define MSG21XX_APK_UPDATE

//#include <mach/hwinfo.h>
//#include <mach/ldo.h>
unsigned int sprd_3rdparty_gpio_tp_rst;
unsigned int sprd_3rdparty_gpio_tp_irq;
/*[BUGFIX]-MOD END by TCTSZ.WH,03/31/2014,PR-602310*/

#define u8         unsigned char
#define u32        unsigned int
#define s32        signed int
#define REPORT_PACKET_LENGTH    (8)
//#define sprd_3rdparty_gpio_tp_irq       (sprd_3rdparty_gpio_tp_irq)
//#define sprd_3rdparty_gpio_tp_rst     (sprd_3rdparty_gpio_tp_rst)

static u16 ctp_x_max;
static u16 ctp_y_max;
unsigned short ctp_id = 2;
static u8  x_y_swap = 0;
#define SWAP_X_Y             (x_y_swap)
#define MS_TS_MSG21XX_X_MAX  (ctp_x_max)
#define MS_TS_MSG21XX_Y_MAX  (ctp_y_max)
//#define MS_TS_MSG21XX_X_MAX   (319)
//#define MS_TS_MSG21XX_Y_MAX   (479)
extern const char GroupData[];
static u8 temp[94][1024];

#define TOUCH_KEY_HOME        (102)
#define TOUCH_KEY_MENU        (139)
#define TOUCH_KEY_BACK        (158)
//#define   TOUCH_KEY_SEARCH      (217)
//[BUGFIX]-DEL  by TCTNB.XQJ,08/19/2013,FR-487707
//static u16 key[] = {/*TOUCH_KEY_SEARCH,*/ TOUCH_KEY_MENU, TOUCH_KEY_HOME, TOUCH_KEY_BACK};
/*[BUGFIX]-END by TCTNB.XQJ*/
/////////////////////////////////////////////////////
static u8 g_dwiic_info_data[1024];   // Buffer for info data
//////////////////////////////////////////////////////
struct sprd_i2c_setup_data {
    unsigned i2c_bus;  //the same number as i2c->adap.nr in adapter probe function
    unsigned short i2c_address;
    char type[I2C_NAME_SIZE];
};

//static struct sprd_i2c_setup_data msg_ts_setup={3, 0x26, "ms-msg21xx"};
/*[BUGFIX]-MOD BEGIN by TCTSZ.WH,03/31/2014,PR-602310*/
int msg21xx_irq = 0;
struct i2c_client *msg21xx_i2c_client;
/*[BUGFIX]-MOD END by TCTSZ.WH,03/31/2014,PR-602310*/
static struct work_struct msg21xx_wq;
static struct input_dev *input = NULL;

#define TP_DEVICE_INFO
#ifdef TP_DEVICE_INFO
    struct proc_dir_entry *tp_proc_file;
    char *tp_info;
#endif


struct msg21xx_ts_data
{
    uint16_t            addr;
    struct i2c_client  *client;
    struct input_dev   *input_dev;
    int                 use_irq;
    struct work_struct  work;
    int (*power)(int on);
    int (*get_int_status)(void);
    void (*reset_ic)(void);
    struct msg21xx_ts_platform_data *pdata ;
    struct regulator *vdd;
    struct regulator *vcc_i2c;
    bool		suspended;
#if defined(CONFIG_FB)
        struct notifier_block fb_notif;
#elif defined(CONFIG_HAS_EARLYSUSPEND)
      struct early_suspend early_suspend;
#endif
};

/*defined max numbers of touch points*/
#define MAX_TOUCH_FINGER 2

typedef struct
{
    u16 X;
    u16 Y;
} TouchPoint_t;

typedef struct
{
    u8 nTouchKeyMode;
    u8 nTouchKeyCode;
    u8 nFingerNum;
    TouchPoint_t Point[MAX_TOUCH_FINGER];
} TouchScreenInfo_t;

/******* firmware update part *********/
void set_tp_status_for_chg(int enable)
{
    return;
}

#define FW_ADDR_MSG21XX   (0xC4>>1)
#define FW_ADDR_MSG21XX_TP   (0x4C>>1)
#define FW_UPDATE_ADDR_MSG21XX   (0x92>>1)
#define TP_DEBUG    printk//(x)     //x
#define DBUG    printk//(x) //x
#ifdef MSG21XX_APK_UPDATE	/*[BUGFIX]-MOD by TCTSZ.WH,03/31/2014,PR-602310*/
static  char *fw_version;
#endif
static u8 temp[94][1024];
 u8  Fmr_Loader[1024];
     u32 crc_tab[256];

static int FwDataCnt;
struct class *firmware_class;
struct device *firmware_cmd_dev;

#define MSG21_VTG_MIN_UV   2700000
#define MSG21_VTG_MAX_UV   3300000
#define MSG21_I2C_VTG_MIN_UV   1800000
#define MSG21_I2C_VTG_MAX_UV   1800000

/*[BUGFIX]-ADD BEGIN by TCTSZ.WH,03/31/2014,PR-602310*/
#undef  KEY_POWER
#define 	KEY_POWER  KEY_UNLOCK
/*[BUGFIX]-ADD END by TCTSZ.WH,03/31/2014,PR-602310*/
#if 0
static int get_ctp_para(void)
{
    int ret = 0;
    unsigned short ctp_key[4];

    if(hwinfo_parser_fetch("lcd0_para", "lcd_width", &ctp_x_max, 1) <= 0)
    {
        printk("ctp_x_max get error\n");
        return -1;
    }
    if(hwinfo_parser_fetch("lcd0_para", "lcd_hight", &ctp_y_max, 1) <= 0)
    {
        printk("ctp_y_max get error\n");
        return -1;
    }
    if(hwinfo_parser_fetch("ctp_para", "msg2133_ctp_id", &ctp_id, 1) <= 0){
        printk("ctp_id read failed\n ");
        return -1;
    } 
    if(ctp_id == 1){
        x_y_swap = 0;
    }
    else{
            x_y_swap = 1;
    }
    printk("ctp_x_max = %d, ctp_y_max = %d..ctp_id = %d...................\n", ctp_x_max, ctp_y_max, ctp_id);
    
    if (hwinfo_parser_fetch("ctp_para", "msg2133_key_value", ctp_key, 4 * 2) <= 0)
    {
        printk("-----------ctp_para msg2133_key_value error\n");
    } else {
        key[0] = ctp_key[0];
        key[1] = ctp_key[1];
        key[2] = ctp_key[2];
        key[3] = ctp_key[3];
        printk("key1 = %d, 2 = %d, 3 = %d, 4 = %d...................\n", key[0], key[1], key[2],key[3]);
    }
#ifdef TP_DEVICE_INFO
    if((ret = hwinfo_parser_fetch("ctp_para", "msg2133_type", NULL, 0)) <= 0){
        printk("ctp para msg2133_type getfailed\n");
    }
    if(ret > 0){
        tp_info = (char *)kmalloc(ret * 2, GFP_KERNEL);
        if(hwinfo_parser_fetch("ctp_para", "msg2133_type", tp_info, ret * 2) <= 0){
        printk("msg2133 tp type info failed ...........................\n");
        }
        printk("msg2133_type :  ret = %d.....%s.................\n", ret, tp_info);
    }
#endif  
    return 0;
}
#endif 
static int  tp_pwron(struct msg21xx_ts_data *data)
{
    //LDO_SetVoltLevel(LDO_LDO_SIM2, LDO_VOLT_LEVEL0);
    //LDO_TurnOnLDO(LDO_LDO_SIM2);
    int rc;
        rc = regulator_enable(data->vcc_i2c);
    if (rc) {
        dev_err(&data->client->dev,
            "Regulator vcc_i2c enable failed rc=%d\n", rc);
        regulator_disable(data->vdd);
    }

    rc = regulator_enable(data->vdd);
    if (rc) {
        dev_err(&data->client->dev,
            "Regulator vdd enable failed rc=%d\n", rc);
        return rc;
    }

    return rc;

}

static int tp_power_init(struct msg21xx_ts_data *data, bool on)
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
        rc = regulator_set_voltage(data->vdd, MSG21_VTG_MIN_UV,
                       MSG21_VTG_MAX_UV);
        if (rc) {
            dev_err(&data->client->dev,
                "Regulator set_vtg failed vdd rc=%d\n", rc);
            return 0;

            goto reg_vdd_put;
        }
    }

    data->vcc_i2c = regulator_get(&data->client->dev, "vcc_i2c");
    if (IS_ERR(data->vcc_i2c)) {
        rc = PTR_ERR(data->vcc_i2c);
        dev_err(&data->client->dev,
            "Regulator get failed vcc_i2c rc=%d\n", rc);
        goto reg_vdd_set_vtg;
    }

    if (regulator_count_voltages(data->vcc_i2c) > 0) {
        rc = regulator_set_voltage(data->vcc_i2c, MSG21_I2C_VTG_MIN_UV,
                       MSG21_I2C_VTG_MAX_UV);
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
        regulator_set_voltage(data->vdd, 0, MSG21_VTG_MAX_UV);
reg_vdd_put:
    regulator_put(data->vdd);
    return rc;

pwr_deinit:
    if (regulator_count_voltages(data->vdd) > 0)
        regulator_set_voltage(data->vdd, 0, MSG21_VTG_MAX_UV);

    regulator_put(data->vdd);

    if (regulator_count_voltages(data->vcc_i2c) > 0)
        regulator_set_voltage(data->vcc_i2c, 0, MSG21_I2C_VTG_MAX_UV);

     regulator_put(data->vcc_i2c);
    return 0;
}
static int  tp_pwroff(struct msg21xx_ts_data *data)
{
    int rc;
    rc = regulator_disable(data->vdd);
    if (rc) {
         dev_err(&data->client->dev,
            "Regulator vdd disable failed rc=%d\n", rc);
        return rc;
    }

    rc = regulator_disable(data->vcc_i2c);
    if (rc) {
        dev_err(&data->client->dev,
            "Regulator vcc_i2c disable failed rc=%d\n", rc);
        regulator_enable(data->vdd);
    }
    return rc;
}

static void HalTscrCReadI2CSeq(u8 addr, u8* read_data, u16 size)
{
   //according to your platform.
    int rc;

    struct i2c_msg msgs[] =
    {
        {
            .addr = addr,
            .flags = I2C_M_RD,
            .len = size,
            .buf = read_data,
        },
    };

    rc = i2c_transfer(msg21xx_i2c_client->adapter, msgs, 1);
    if( rc < 0 )
    {
        printk("HalTscrCReadI2CSeq error %d\n", rc);
    }
}

static void HalTscrCDevWriteI2CSeq(u8 addr, u8* data, u16 size)
{
    //according to your platform.
    int rc;
    struct i2c_msg msgs[] =
    {
        {
            .addr = addr,
            .flags = 0,
            .len = size,
            .buf = data,
        },
    };
    rc = i2c_transfer(msg21xx_i2c_client->adapter, msgs, 1);
    if( rc < 0 )
    {
        printk("HalTscrCDevWriteI2CSeq error %d,addr = %d\n", rc,addr);
    }
}
#if 0
static void Get_Chip_Version(void)
{

    unsigned char dbbus_tx_data[3];
    unsigned char dbbus_rx_data[2];
  printk("[%s]: Enter!\n", __func__);
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0xCE;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, &dbbus_tx_data[0], 3);
    HalTscrCReadI2CSeq(FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2);
    if (dbbus_rx_data[1] == 0)
    {
        // it is Catch2
      //  TP_DEBUG(printk("*** Catch2 ***\n");)
        //FwVersion  = 2;// 2 means Catch2
          printk("[%s]: msg 111!\n", __func__);

    }
    else
    {
       printk("[%s]: msg 2222 !\n", __func__);

        // it is catch1
      //  TP_DEBUG(printk("*** Catch1 ***\n");)
        //FwVersion  = 1;// 1 means Catch1
    }

}

#endif
static void dbbusDWIICEnterSerialDebugMode(void)
{
    u8 data[5];

    // Enter the Serial Debug Mode
    data[0] = 0x53;
    data[1] = 0x45;
    data[2] = 0x52;
    data[3] = 0x44;
    data[4] = 0x42;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, data, 5);
}

static void dbbusDWIICStopMCU(void)
{
    u8 data[1];

    // Stop the MCU
    data[0] = 0x37;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, data, 1);
}

static void dbbusDWIICIICUseBus(void)
{
    u8 data[1];

    // IIC Use Bus
    data[0] = 0x35;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, data, 1);
}

static void dbbusDWIICIICReshape(void)
{
    u8 data[1];

    // IIC Re-shape
    data[0] = 0x71;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, data, 1);
}

static void dbbusDWIICIICNotUseBus(void)
{
    u8 data[1];

    // IIC Not Use Bus
    data[0] = 0x34;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, data, 1);
}

static void dbbusDWIICNotStopMCU(void)
{
    u8 data[1];

    // Not Stop the MCU
    data[0] = 0x36;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, data, 1);
}

static void dbbusDWIICExitSerialDebugMode(void)
{
    u8 data[1];

    // Exit the Serial Debug Mode
    data[0] = 0x45;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, data, 1);

    // Delay some interval to guard the next transaction
    udelay ( 150);//200 );        // delay about 0.2ms
}

static void drvISP_EntryIspMode(void)
{
    u8 bWriteData[5] =
    {
        0x4D, 0x53, 0x54, 0x41, 0x52
    };
    printk("\n******%s come in*******\n",__FUNCTION__);
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 5);
    udelay ( 150 );//200 );        // delay about 0.1ms
}

static u8 drvISP_Read(u8 n, u8* pDataToRead)    //First it needs send 0x11 to notify we want to get flash data back.
{
    u8 Read_cmd = 0x11;
    unsigned char dbbus_rx_data[2] = {0};
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &Read_cmd, 1);
    //msctpc_LoopDelay ( 1 );        // delay about 100us*****
    udelay( 800 );//200);
    if (n == 1)
    {
        HalTscrCReadI2CSeq(FW_UPDATE_ADDR_MSG21XX, &dbbus_rx_data[0], 2);
        *pDataToRead = dbbus_rx_data[0];
        TP_DEBUG("dbbus=%d,%d===drvISP_Read=====\n",dbbus_rx_data[0],dbbus_rx_data[1]);
    }
    else
    {
        HalTscrCReadI2CSeq(FW_UPDATE_ADDR_MSG21XX, pDataToRead, n);
    }

    return 0;
}

static void drvISP_WriteEnable(void)
{
    u8 bWriteData[2] =
    {
        0x10, 0x06
    };
    u8 bWriteData1 = 0x12;
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 2);
    udelay(150);//1.16
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);
}


static void drvISP_ExitIspMode(void)
{
    u8 bWriteData = 0x24;
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData, 1);
    udelay( 150 );//200);
}

static u8 drvISP_ReadStatus(void)
{
    u8 bReadData = 0;
    u8 bWriteData[2] =
    {
        0x10, 0x05
    };
    u8 bWriteData1 = 0x12;

    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 2);
    //msctpc_LoopDelay ( 1 );        // delay about 100us*****
    udelay(150);//200);
    drvISP_Read(1, &bReadData);
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);
    return bReadData;
}

#if 0
static void drvISP_BlockErase(u32 addr)
{
    u8 bWriteData[5] = { 0x00, 0x00, 0x00, 0x00, 0x00 };
    u8 bWriteData1 = 0x12;

    u32 timeOutCount=0;
    drvISP_WriteEnable();
    printk("\n******%s come in*******\n",__FUNCTION__);
    //Enable write status register
    bWriteData[0] = 0x10;
    bWriteData[1] = 0x50;
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 2);
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);

    //Write Status
    bWriteData[0] = 0x10;
    bWriteData[1] = 0x01;
    bWriteData[2] = 0x00;
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 3);
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);

    //Write disable
    bWriteData[0] = 0x10;
    bWriteData[1] = 0x04;
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 2);
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);
    //msctpc_LoopDelay ( 1 );        // delay about 100us*****
    udelay(150);//200);
    timeOutCount=0;
    while ( ( drvISP_ReadStatus() & 0x01 ) == 0x01 )
    {
        timeOutCount++;
        if ( timeOutCount >= 100000 ) break; /* around 1 sec timeout */
    }
    drvISP_WriteEnable();

    bWriteData[0] = 0x10;
    bWriteData[1] = 0xC7;//0xD8;        //Block Erase
    //bWriteData[2] = ((addr >> 16) & 0xFF) ;
    //bWriteData[3] = ((addr >> 8) & 0xFF) ;
    //bWriteData[4] = (addr & 0xFF) ;
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 2);
    //HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData, 5);
    HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);
        //msctpc_LoopDelay ( 1 );        // delay about 100us*****
    udelay(150);//200);
    timeOutCount=0;
    while ( ( drvISP_ReadStatus() & 0x01 ) == 0x01 )
    {
        timeOutCount++;
        if ( timeOutCount >= 500000 ) break; /* around 5 sec timeout */
    }
}
#endif
static void drvISP_Program(u16 k, u8* pDataToWrite)
{
    u16 i = 0;
    u16 j = 0;
    //u16 n = 0;
    u8 TX_data[133];
    u8 bWriteData1 = 0x12;
    u32 addr = k * 1024;
        u32 timeOutCount=0;
    for (j = 0; j < 8; j++)   //128*8 cycle
    {
        TX_data[0] = 0x10;
        TX_data[1] = 0x02;// Page Program CMD
        TX_data[2] = (addr + 128 * j) >> 16;
        TX_data[3] = (addr + 128 * j) >> 8;
        TX_data[4] = (addr + 128 * j);
        for (i = 0; i < 128; i++)
        {
            TX_data[5 + i] = pDataToWrite[j * 128 + i];
        }
        //msctpc_LoopDelay ( 1 );        // delay about 100us*****
        udelay(150);//200);
       
        timeOutCount=0;
        while ( ( drvISP_ReadStatus() & 0x01 ) == 0x01 )
        {
            timeOutCount++;
            if ( timeOutCount >= 100000 ) break; /* around 1 sec timeout */
        }
  
        drvISP_WriteEnable();
        HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, TX_data, 133);   //write 133 byte per cycle
        HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);
    }
}

#ifdef MSG21XX_APK_UPDATE	/*[BUGFIX]-ADD by TCTSZ.WH,03/31/2014,PR-602310*/
static ssize_t firmware_update_show ( struct device *dev,
                                      struct device_attribute *attr, char *buf )
{
    return sprintf ( buf, "%s\n", fw_version );
}
#endif	/*[BUGFIX]-ADD by TCTSZ.WH,03/31/2014,PR-602310*/
/*reset the chip*/
static void _HalTscrHWReset(void)
{
    gpio_direction_output(sprd_3rdparty_gpio_tp_rst, 1);
    gpio_set_value(sprd_3rdparty_gpio_tp_rst, 1);
    gpio_set_value(sprd_3rdparty_gpio_tp_rst, 0);
    mdelay(10);  /* Note that the RST must be in LOW 10ms at least */
    gpio_set_value(sprd_3rdparty_gpio_tp_rst, 1);
    /* Enable the interrupt service thread/routine for INT after 50ms */
    mdelay(50);
}

/*add by liukai for release the touch action*/
static void msg21xx_release(void)
{
    printk("[%s]: Enter!\n", __func__);
/*[BUGFIX]-DEL BEGRIN by TCTNB.XQJ,08/19/2013,FR-487707*/
    //input_report_key(input, TOUCH_KEY_MENU, 0);
    //input_report_key(input, TOUCH_KEY_HOME, 0);
    //input_report_key(input, TOUCH_KEY_BACK, 0);
    //input_report_key(input, TOUCH_KEY_SEARCH, 0);
/*[BUGFIX]-END by TCTNB.XQJ*/
    input_report_abs(input, ABS_MT_TOUCH_MAJOR, 0);
    //input_mt_sync(input);
    input_sync(input);
}
static void drvISP_Verify ( u16 k, u8* pDataToVerify )
{
    u16 i = 0, j = 0;
    u8 bWriteData[5] ={ 0x10, 0x03, 0, 0, 0 };
    u8 RX_data[256];
    u8 bWriteData1 = 0x12;
    u32 addr = k * 1024;
    u8 index = 0;
    u32 timeOutCount;
    for ( j = 0; j < 8; j++ ) //128*8 cycle
    {
        bWriteData[2] = ( u8 ) ( ( addr + j * 128 ) >> 16 );
        bWriteData[3] = ( u8 ) ( ( addr + j * 128 ) >> 8 );
        bWriteData[4] = ( u8 ) ( addr + j * 128 );
        udelay ( 100 );        // delay about 100us*****

        timeOutCount = 0;
        while ( ( drvISP_ReadStatus() & 0x01 ) == 0x01 )
        {
            timeOutCount++;
            if ( timeOutCount >= 100000 ) break; /* around 1 sec timeout */
        }

        HalTscrCDevWriteI2CSeq ( FW_UPDATE_ADDR_MSG21XX, bWriteData, 5 ); //write read flash addr
        udelay ( 100 );        // delay about 100us*****
        drvISP_Read ( 128, RX_data );
        HalTscrCDevWriteI2CSeq ( FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1 ); //cmd end
        for ( i = 0; i < 128; i++ ) //log out if verify error
        {
            if ( ( RX_data[i] != 0 ) && index < 10 )
            {
                //TP_DEBUG("j=%d,RX_data[%d]=0x%x\n",j,i,RX_data[i]);
                index++;
            }
            if ( RX_data[i] != pDataToVerify[128 * j + i] )
            {
                TP_DEBUG ( "k=%d,j=%d,i=%d===============Update Firmware Error================", k, j, i );
            }
        }
    }
}

static void drvISP_ChipErase(void)
{
    u8 bWriteData[5] = { 0x00, 0x00, 0x00, 0x00, 0x00 };
    u8 bWriteData1 = 0x12;
    u32 timeOutCount = 0;
    drvISP_WriteEnable();

    //Enable write status register
    bWriteData[0] = 0x10;
    bWriteData[1] = 0x50;
    HalTscrCDevWriteI2CSeq ( FW_UPDATE_ADDR_MSG21XX, bWriteData, 2 );
    HalTscrCDevWriteI2CSeq ( FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1 );

    //Write Status
    bWriteData[0] = 0x10;
    bWriteData[1] = 0x01;
    bWriteData[2] = 0x00;
    HalTscrCDevWriteI2CSeq ( FW_UPDATE_ADDR_MSG21XX, bWriteData, 3 );
    HalTscrCDevWriteI2CSeq ( FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1 );

    //Write disable
    bWriteData[0] = 0x10;
    bWriteData[1] = 0x04;
    HalTscrCDevWriteI2CSeq ( FW_UPDATE_ADDR_MSG21XX, bWriteData, 2 );
    HalTscrCDevWriteI2CSeq ( FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1 );
    udelay ( 100 );        // delay about 100us*****
    timeOutCount = 0;
    while ( ( drvISP_ReadStatus() & 0x01 ) == 0x01 )
    {
        timeOutCount++;
        if ( timeOutCount >= 100000 ) break; /* around 1 sec timeout */
    }
    drvISP_WriteEnable();

    bWriteData[0] = 0x10;
    bWriteData[1] = 0xC7;

    HalTscrCDevWriteI2CSeq ( FW_UPDATE_ADDR_MSG21XX, bWriteData, 2 );
    HalTscrCDevWriteI2CSeq ( FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1 );
    udelay ( 100 );        // delay about 100us*****
    timeOutCount = 0;
    while ( ( drvISP_ReadStatus() & 0x01 ) == 0x01 )
    {
        timeOutCount++;
        if ( timeOutCount >= 500000 ) break; /* around 5 sec timeout */
    }
}

/* update the firmware part, used by apk*/
/*show the fw version*/

/*[BUGFIX]-MOD BEGIN by TCTSZ.WH,03/18/2014,PR-602310*/
static ssize_t firmware_update_c2 ( size_t size )
/*[BUGFIX]-MOD END by TCTSZ.WH,03/18/2014,PR-602310*/
{
    u8 i;
    u8 dbbus_tx_data[4];
    unsigned char dbbus_rx_data[2] = {0};

    // set FRO to 50M
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x11;
    dbbus_tx_data[2] = 0xE2;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 3 );
    dbbus_rx_data[0] = 0;
    dbbus_rx_data[1] = 0;
    HalTscrCReadI2CSeq ( FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2 );
    TP_DEBUG ( "dbbus_rx_data[0]=0x%x", dbbus_rx_data[0] );
    dbbus_tx_data[3] = dbbus_rx_data[0] & 0xF7;  //Clear Bit 3
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // set MCU clock,SPI clock =FRO
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x22;
    dbbus_tx_data[3] = 0x00;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x23;
    dbbus_tx_data[3] = 0x00;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // Enable slave's ISP ECO mode
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x08;
    dbbus_tx_data[2] = 0x0c;
    dbbus_tx_data[3] = 0x08;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // Enable SPI Pad
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x02;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 3 );
    HalTscrCReadI2CSeq ( FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2 );
    TP_DEBUG ( "dbbus_rx_data[0]=0x%x", dbbus_rx_data[0] );
    dbbus_tx_data[3] = ( dbbus_rx_data[0] | 0x20 ); //Set Bit 5
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // WP overwrite
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x0E;
    dbbus_tx_data[3] = 0x02;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // set pin high
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x10;
    dbbus_tx_data[3] = 0x08;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    dbbusDWIICIICNotUseBus();
    dbbusDWIICNotStopMCU();
    dbbusDWIICExitSerialDebugMode();

    drvISP_EntryIspMode();
    drvISP_ChipErase();
    _HalTscrHWReset();
    mdelay ( 300 );

    // Program and Verify
    dbbusDWIICEnterSerialDebugMode();
    dbbusDWIICStopMCU();
    dbbusDWIICIICUseBus();
    dbbusDWIICIICReshape();

    // Disable the Watchdog
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x3C;
    dbbus_tx_data[2] = 0x60;
    dbbus_tx_data[3] = 0x55;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x3C;
    dbbus_tx_data[2] = 0x61;
    dbbus_tx_data[3] = 0xAA;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    //Stop MCU
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x0F;
    dbbus_tx_data[2] = 0xE6;
    dbbus_tx_data[3] = 0x01;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // set FRO to 50M
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x11;
    dbbus_tx_data[2] = 0xE2;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 3 );
    dbbus_rx_data[0] = 0;
    dbbus_rx_data[1] = 0;
    HalTscrCReadI2CSeq ( FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2 );
    TP_DEBUG ( "dbbus_rx_data[0]=0x%x", dbbus_rx_data[0] );
    dbbus_tx_data[3] = dbbus_rx_data[0] & 0xF7;  //Clear Bit 3
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // set MCU clock,SPI clock =FRO
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x22;
    dbbus_tx_data[3] = 0x00;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x23;
    dbbus_tx_data[3] = 0x00;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // Enable slave's ISP ECO mode
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x08;
    dbbus_tx_data[2] = 0x0c;
    dbbus_tx_data[3] = 0x08;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // Enable SPI Pad
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x02;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 3 );
    HalTscrCReadI2CSeq ( FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2 );
    TP_DEBUG ( "dbbus_rx_data[0]=0x%x", dbbus_rx_data[0] );
    dbbus_tx_data[3] = ( dbbus_rx_data[0] | 0x20 ); //Set Bit 5
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // WP overwrite
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x0E;
    dbbus_tx_data[3] = 0x02;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // set pin high
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x10;
    dbbus_tx_data[3] = 0x08;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    dbbusDWIICIICNotUseBus();
    dbbusDWIICNotStopMCU();
    dbbusDWIICExitSerialDebugMode();

    ///////////////////////////////////////
    // Start to load firmware
    ///////////////////////////////////////
    drvISP_EntryIspMode();

    for ( i = 0; i < 94; i++ ) // total  94 KB : 1 byte per R/W
    {
        drvISP_Program ( i, temp[i] ); // program to slave's flash
        drvISP_Verify ( i, temp[i] ); //verify data
    }
    TP_DEBUG ( "update OK\n" );
    drvISP_ExitIspMode();
    FwDataCnt = 0;
    enable_irq(msg21xx_irq);
    return size;
}

static u32 Reflect ( u32 ref, char ch ) //unsigned int Reflect(unsigned int ref, char ch)
{
    u32 value = 0;
    u32 i = 0;

    for ( i = 1; i < ( ch + 1 ); i++ )
    {
        if ( ref & 1 )
        {
            value |= 1 << ( ch - i );
        }
        ref >>= 1;
    }
    return value;
}

u32 Get_CRC ( u32 text, u32 prevCRC, u32 *crc32_table )
{
    u32  ulCRC = prevCRC;
    ulCRC = ( ulCRC >> 8 ) ^ crc32_table[ ( ulCRC & 0xFF ) ^ text];
    return ulCRC ;
}
static void Init_CRC32_Table ( u32 *crc32_table )
{
    u32 magicnumber = 0x04c11db7;
    u32 i = 0, j;

    for ( i = 0; i <= 0xFF; i++ )
    {
        crc32_table[i] = Reflect ( i, 8 ) << 24;
        for ( j = 0; j < 8; j++ )
        {
            crc32_table[i] = ( crc32_table[i] << 1 ) ^ ( crc32_table[i] & ( 0x80000000L ) ? magicnumber : 0 );
        }
        crc32_table[i] = Reflect ( crc32_table[i], 32 );
    }
}

typedef enum
{
    EMEM_ALL = 0,
    EMEM_MAIN,
    EMEM_INFO,
} EMEM_TYPE_t;

static void drvDB_WriteReg8Bit ( u8 bank, u8 addr, u8 data )
{
    u8 tx_data[4] = {0x10, bank, addr, data};
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, tx_data, 4 );
}

static void drvDB_WriteReg ( u8 bank, u8 addr, u16 data )
{
    u8 tx_data[5] = {0x10, bank, addr, data & 0xFF, data >> 8};
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, tx_data, 5 );
}

static unsigned short drvDB_ReadReg ( u8 bank, u8 addr )
{
    u8 tx_data[3] = {0x10, bank, addr};
    u8 rx_data[2] = {0};

    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, tx_data, 3 );
    HalTscrCReadI2CSeq ( FW_ADDR_MSG21XX, &rx_data[0], 2 );
    return ( rx_data[1] << 8 | rx_data[0] );
}

static int drvTP_erase_emem_c32 ( void )
{
    /////////////////////////
    //Erase  all
    /////////////////////////
    
    //enter gpio mode
    drvDB_WriteReg ( 0x16, 0x1E, 0xBEAF );

    // before gpio mode, set the control pin as the orginal status
    drvDB_WriteReg ( 0x16, 0x08, 0x0000 );
    drvDB_WriteReg8Bit ( 0x16, 0x0E, 0x10 );
    mdelay ( 10 ); //MCR_CLBK_DEBUG_DELAY ( 10, MCU_LOOP_DELAY_COUNT_MS );

    // ptrim = 1, h'04[2]
    drvDB_WriteReg8Bit ( 0x16, 0x08, 0x04 );
    drvDB_WriteReg8Bit ( 0x16, 0x0E, 0x10 );
    mdelay ( 10 ); //MCR_CLBK_DEBUG_DELAY ( 10, MCU_LOOP_DELAY_COUNT_MS );

    // ptm = 6, h'04[12:14] = b'110
    drvDB_WriteReg8Bit ( 0x16, 0x09, 0x60 );
    drvDB_WriteReg8Bit ( 0x16, 0x0E, 0x10 );

    // pmasi = 1, h'04[6]
    drvDB_WriteReg8Bit ( 0x16, 0x08, 0x44 );
    // pce = 1, h'04[11]
    drvDB_WriteReg8Bit ( 0x16, 0x09, 0x68 );
    // perase = 1, h'04[7]
    drvDB_WriteReg8Bit ( 0x16, 0x08, 0xC4 );
    // pnvstr = 1, h'04[5]
    drvDB_WriteReg8Bit ( 0x16, 0x08, 0xE4 );
    // pwe = 1, h'04[9]
    drvDB_WriteReg8Bit ( 0x16, 0x09, 0x6A );
    // trigger gpio load
    drvDB_WriteReg8Bit ( 0x16, 0x0E, 0x10 );

    return ( 1 );
}

/*[BUGFIX]-MOD BEGIN by TCTSZ.WH,03/18/2014,PR-602310*/
static ssize_t firmware_update_c32 ( size_t size,  EMEM_TYPE_t emem_type )
/*[BUGFIX]-MOD END by TCTSZ.WH,03/18/2014,PR-602310*/
{
//    u8  dbbus_tx_data[4];
   // u8  dbbus_rx_data[2] = {0};
      // Buffer for slave's firmware

    u32 i, j;
    u32 crc_main, crc_main_tp;
    u32 crc_info, crc_info_tp;
    u16 reg_data = 0;

    crc_main = 0xffffffff;
    crc_info = 0xffffffff;

#if 1
    /////////////////////////
    // Erase  all
    /////////////////////////
    drvTP_erase_emem_c32();
    mdelay ( 1000 ); //MCR_CLBK_DEBUG_DELAY ( 1000, MCU_LOOP_DELAY_COUNT_MS );

    //ResetSlave();
    _HalTscrHWReset();
    //drvDB_EnterDBBUS();
    dbbusDWIICEnterSerialDebugMode();
    dbbusDWIICStopMCU();
    dbbusDWIICIICUseBus();
    dbbusDWIICIICReshape();
    mdelay ( 300 );

    // Reset Watchdog
    drvDB_WriteReg8Bit ( 0x3C, 0x60, 0x55 );
    drvDB_WriteReg8Bit ( 0x3C, 0x61, 0xAA );

    /////////////////////////
    // Program
    /////////////////////////

    //polling 0x3CE4 is 0x1C70
    do
    {
        reg_data = drvDB_ReadReg ( 0x3C, 0xE4 );
    }
    while ( reg_data != 0x1C70 );


    drvDB_WriteReg ( 0x3C, 0xE4, 0xE38F );  // for all-blocks

    //polling 0x3CE4 is 0x2F43
    do
    {
        reg_data = drvDB_ReadReg ( 0x3C, 0xE4 );
    }
    while ( reg_data != 0x2F43 );


    //calculate CRC 32
    Init_CRC32_Table ( &crc_tab[0] );

    for ( i = 0; i < 33; i++ ) // total  33 KB : 2 byte per R/W
    {
        if ( i < 32 )   //emem_main
        {
            if ( i == 31 )
            {
                temp[i][1014] = 0x5A; //Fmr_Loader[1014]=0x5A;
                temp[i][1015] = 0xA5; //Fmr_Loader[1015]=0xA5;

                for ( j = 0; j < 1016; j++ )
                {
                    //crc_main=Get_CRC(Fmr_Loader[j],crc_main,&crc_tab[0]);
                    crc_main = Get_CRC ( temp[i][j], crc_main, &crc_tab[0] );
                }
            }
            else
            {
                for ( j = 0; j < 1024; j++ )
                {
                    //crc_main=Get_CRC(Fmr_Loader[j],crc_main,&crc_tab[0]);
                    crc_main = Get_CRC ( temp[i][j], crc_main, &crc_tab[0] );
                }
            }
        }
        else  // emem_info
        {
            for ( j = 0; j < 1024; j++ )
            {
                //crc_info=Get_CRC(Fmr_Loader[j],crc_info,&crc_tab[0]);
                crc_info = Get_CRC ( temp[i][j], crc_info, &crc_tab[0] );
            }
        }

        //drvDWIIC_MasterTransmit( DWIIC_MODE_DWIIC_ID, 1024, Fmr_Loader );
        HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX_TP, temp[i], 1024 );

        // polling 0x3CE4 is 0xD0BC
        do
        {
            reg_data = drvDB_ReadReg ( 0x3C, 0xE4 );
        }
        while ( reg_data != 0xD0BC );

        drvDB_WriteReg ( 0x3C, 0xE4, 0x2F43 );
    }

    //write file done
    drvDB_WriteReg ( 0x3C, 0xE4, 0x1380 );

    mdelay ( 10 ); //MCR_CLBK_DEBUG_DELAY ( 10, MCU_LOOP_DELAY_COUNT_MS );
    // polling 0x3CE4 is 0x9432
    do
    {
        reg_data = drvDB_ReadReg ( 0x3C, 0xE4 );
    }
    while ( reg_data != 0x9432 );

    crc_main = crc_main ^ 0xffffffff;
    crc_info = crc_info ^ 0xffffffff;

    // CRC Main from TP
    crc_main_tp = drvDB_ReadReg ( 0x3C, 0x80 );
    crc_main_tp = ( crc_main_tp << 16 ) | drvDB_ReadReg ( 0x3C, 0x82 );
 
    //CRC Info from TP
    crc_info_tp = drvDB_ReadReg ( 0x3C, 0xA0 );
    crc_info_tp = ( crc_info_tp << 16 ) | drvDB_ReadReg ( 0x3C, 0xA2 );

    TP_DEBUG ( "crc_main=0x%x, crc_info=0x%x, crc_main_tp=0x%x, crc_info_tp=0x%x\n",
               crc_main, crc_info, crc_main_tp, crc_info_tp );

    //drvDB_ExitDBBUS();
    if ( ( crc_main_tp != crc_main ) || ( crc_info_tp != crc_info ) )
    {
        printk ( "update FAILED\n" );
        _HalTscrHWReset();
        FwDataCnt = 0;
        enable_irq(msg21xx_irq);        
        return ( 0 );
    }

    printk ( "update OK\n" );
    _HalTscrHWReset();
    FwDataCnt = 0;
    enable_irq(msg21xx_irq);

    return size;
#endif
}

static int drvTP_erase_emem_c33 ( EMEM_TYPE_t emem_type )
{
    // stop mcu
    drvDB_WriteReg ( 0x0F, 0xE6, 0x0001 );

    //disable watch dog
    drvDB_WriteReg8Bit ( 0x3C, 0x60, 0x55 );
    drvDB_WriteReg8Bit ( 0x3C, 0x61, 0xAA );

    // set PROGRAM password
    drvDB_WriteReg8Bit ( 0x16, 0x1A, 0xBA );
    drvDB_WriteReg8Bit ( 0x16, 0x1B, 0xAB );

    //proto.MstarWriteReg(F1.loopDevice, 0x1618, 0x80);
    drvDB_WriteReg8Bit ( 0x16, 0x18, 0x80 );

    if ( emem_type == EMEM_ALL )
    {
        drvDB_WriteReg8Bit ( 0x16, 0x08, 0x10 ); //mark
    }

    drvDB_WriteReg8Bit ( 0x16, 0x18, 0x40 );
    mdelay ( 10 );

    drvDB_WriteReg8Bit ( 0x16, 0x18, 0x80 );

    // erase trigger
    if ( emem_type == EMEM_MAIN )
    {
        drvDB_WriteReg8Bit ( 0x16, 0x0E, 0x04 ); //erase main
    }
    else
    {
        drvDB_WriteReg8Bit ( 0x16, 0x0E, 0x08 ); //erase all block
    }

    return ( 1 );
}
#if 0
static int drvTP_read_emem_dbbus_c33 ( EMEM_TYPE_t emem_type, u16 addr, size_t size, u8 *p, size_t set_pce_high )
{
    u32 i;

    // Set the starting address ( must before enabling burst mode and enter riu mode )
    drvDB_WriteReg ( 0x16, 0x00, addr );

    // Enable the burst mode ( must before enter riu mode )
    drvDB_WriteReg ( 0x16, 0x0C, drvDB_ReadReg ( 0x16, 0x0C ) | 0x0001 );

    // Set the RIU password
    drvDB_WriteReg ( 0x16, 0x1A, 0xABBA );

    // Enable the information block if pifren is HIGH
    if ( emem_type == EMEM_INFO )
    {
        // Clear the PCE
        drvDB_WriteReg ( 0x16, 0x18, drvDB_ReadReg ( 0x16, 0x18 ) | 0x0080 );
        mdelay ( 10 );

        // Set the PIFREN to be HIGH
        drvDB_WriteReg ( 0x16, 0x08, 0x0010 );
    }

    // Set the PCE to be HIGH
    drvDB_WriteReg ( 0x16, 0x18, drvDB_ReadReg ( 0x16, 0x18 ) | 0x0040 );
    mdelay ( 10 );

    // Wait pce becomes 1 ( read data ready )
    while ( ( drvDB_ReadReg ( 0x16, 0x10 ) & 0x0004 ) != 0x0004 );

    for ( i = 0; i < size; i += 4 )
    {
        // Fire the FASTREAD command
        drvDB_WriteReg ( 0x16, 0x0E, drvDB_ReadReg ( 0x16, 0x0E ) | 0x0001 );

        // Wait the operation is done
        while ( ( drvDB_ReadReg ( 0x16, 0x10 ) & 0x0001 ) != 0x0001 );

        p[i + 0] = drvDB_ReadReg ( 0x16, 0x04 ) & 0xFF;
        p[i + 1] = ( drvDB_ReadReg ( 0x16, 0x04 ) >> 8 ) & 0xFF;
        p[i + 2] = drvDB_ReadReg ( 0x16, 0x06 ) & 0xFF;
        p[i + 3] = ( drvDB_ReadReg ( 0x16, 0x06 ) >> 8 ) & 0xFF;
    }

    // Disable the burst mode
    drvDB_WriteReg ( 0x16, 0x0C, drvDB_ReadReg ( 0x16, 0x0C ) & ( ~0x0001 ) );

    // Clear the starting address
    drvDB_WriteReg ( 0x16, 0x00, 0x0000 );

    //Always return to main block
    if ( emem_type == EMEM_INFO )
    {
        // Clear the PCE before change block
        drvDB_WriteReg ( 0x16, 0x18, drvDB_ReadReg ( 0x16, 0x18 ) | 0x0080 );
        mdelay ( 10 );
        // Set the PIFREN to be LOW
        drvDB_WriteReg ( 0x16, 0x08, drvDB_ReadReg ( 0x16, 0x08 ) & ( ~0x0010 ) );

        drvDB_WriteReg ( 0x16, 0x18, drvDB_ReadReg ( 0x16, 0x18 ) | 0x0040 );
        while ( ( drvDB_ReadReg ( 0x16, 0x10 ) & 0x0004 ) != 0x0004 );
    }

    // Clear the RIU password
    drvDB_WriteReg ( 0x16, 0x1A, 0x0000 );

    if ( set_pce_high )
    {
        // Set the PCE to be HIGH before jumping back to e-flash codes
        drvDB_WriteReg ( 0x16, 0x18, drvDB_ReadReg ( 0x16, 0x18 ) | 0x0040 );
        while ( ( drvDB_ReadReg ( 0x16, 0x10 ) & 0x0004 ) != 0x0004 );
    }

    return ( 1 );
}
#endif

static int drvTP_read_info_dwiic_c33 ( void )
{
    u8  dwiic_tx_data[5];
    //u8  dwiic_rx_data[4];
    u16 reg_data=0;
    //unsigned char dbbus_rx_data[2] = {0};
 
    mdelay ( 300 );
 
 
    // Stop Watchdog
    drvDB_WriteReg8Bit ( 0x3C, 0x60, 0x55 );
    drvDB_WriteReg8Bit ( 0x3C, 0x61, 0xAA );
 
    drvDB_WriteReg ( 0x3C, 0xE4, 0xA4AB );
 
drvDB_WriteReg ( 0x1E, 0x04, 0x7d60 );
 
    // TP SW reset
    drvDB_WriteReg ( 0x1E, 0x04, 0x829F );
mdelay (1);
    dwiic_tx_data[0] = 0x10;
    dwiic_tx_data[1] = 0x0F;
    dwiic_tx_data[2] = 0xE6;
    dwiic_tx_data[3] = 0x00;
    mdelay(15);
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dwiic_tx_data, 4 );
        // stop mcu
  //  drvDB_WriteReg ( 0x1E, 0xE6, 0x0001 );
    mdelay ( 100 );
TP_DEBUG ( "read infor 1\n");
    do{
        reg_data = drvDB_ReadReg ( 0x3C, 0xE4 );
    }
    while ( reg_data != 0x5B58 );
TP_DEBUG ( "read infor +++2\n");
    dwiic_tx_data[0] = 0x72;
 
   // dwiic_tx_data[3] = 0x04;
  //  dwiic_tx_data[4] = 0x00;
    dwiic_tx_data[3] = 0x00;
    dwiic_tx_data[4] = 0x80;
 
    for(reg_data=0;reg_data<8;reg_data++)
    {
    dwiic_tx_data[1] = 0x80+(((reg_data*128)&0xff00)>>8);
    dwiic_tx_data[2] = (reg_data*128)&0x00ff;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX_TP , dwiic_tx_data, 5 );
    mdelay (50 );
 
    // recive info data
    HalTscrCReadI2CSeq ( FW_ADDR_MSG21XX_TP, &g_dwiic_info_data[reg_data*128], 128);
    }
    return ( 1 );
}

#if 0
static int drvTP_info_updata_C33 ( u16 start_index, u8 *data, u16 size )
{
    // size != 0, start_index+size !> 1024
    u16 i;
    for ( i = 0; i < size; i++ )
    {
        g_dwiic_info_data[start_index] = * ( data + i );
        start_index++;
    }
    return ( 1 );
}
#endif

/*[BUGFIX]-MOD BEGIN by TCTSZ.WH,03/18/2014,PR-602310*/
static ssize_t firmware_update_c33 ( size_t size, EMEM_TYPE_t emem_type )
/*[BUGFIX]-MOD END by TCTSZ.WH,03/18/2014,PR-602310*/
{
//    u8  dbbus_tx_data[4];
//    u8  dbbus_rx_data[2] = {0};
    u8  life_counter[2];
    u32 i, j;
    u32 crc_main, crc_main_tp;
    u32 crc_info, crc_info_tp;
  
    int update_pass = 1;
    u16 reg_data = 0;

    crc_main = 0xffffffff;
    crc_info = 0xffffffff;

    drvTP_read_info_dwiic_c33();
    
    if ( g_dwiic_info_data[0] == 'M' && g_dwiic_info_data[1] == 'S' && g_dwiic_info_data[2] == 'T' && g_dwiic_info_data[3] == 'A' && g_dwiic_info_data[4] == 'R' && g_dwiic_info_data[5] == 'T' && g_dwiic_info_data[6] == 'P' && g_dwiic_info_data[7] == 'C' )
    {
        // updata FW Version
        //drvTP_info_updata_C33 ( 8, &temp[32][8], 5 );

        g_dwiic_info_data[8]=temp[32][8];
        g_dwiic_info_data[9]=temp[32][9];
        g_dwiic_info_data[10]=temp[32][10];
        g_dwiic_info_data[11]=temp[32][11];
        // updata life counter
        life_counter[1] = (( ( (g_dwiic_info_data[13] << 8 ) | g_dwiic_info_data[12]) + 1 ) >> 8 ) & 0xFF;
        life_counter[0] = ( ( (g_dwiic_info_data[13] << 8 ) | g_dwiic_info_data[12]) + 1 ) & 0xFF;
        g_dwiic_info_data[12]=life_counter[0];
        g_dwiic_info_data[13]=life_counter[1];
        //drvTP_info_updata_C33 ( 10, &life_counter[0], 3 );
        drvDB_WriteReg ( 0x3C, 0xE4, 0x78C5 );
        drvDB_WriteReg ( 0x1E, 0x04, 0x7d60 );
        // TP SW reset
        drvDB_WriteReg ( 0x1E, 0x04, 0x829F );

        mdelay ( 50 );

        //polling 0x3CE4 is 0x2F43
        do
        {
            reg_data = drvDB_ReadReg ( 0x3C, 0xE4 );

        }
        while ( reg_data != 0x2F43 );

        // transmit lk info data
        HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX_TP , &g_dwiic_info_data[0], 1024 );

        //polling 0x3CE4 is 0xD0BC
        do
        {
            reg_data = drvDB_ReadReg ( 0x3C, 0xE4 );
        }
        while ( reg_data != 0xD0BC );

    }

    //erase main
    drvTP_erase_emem_c33 ( EMEM_MAIN );
    mdelay ( 1000 );

    //ResetSlave();
    _HalTscrHWReset();

    //drvDB_EnterDBBUS();
    dbbusDWIICEnterSerialDebugMode();
    dbbusDWIICStopMCU();
    dbbusDWIICIICUseBus();
    dbbusDWIICIICReshape();
    mdelay ( 300 );

    /////////////////////////
    // Program
    /////////////////////////

    //polling 0x3CE4 is 0x1C70
    if ( ( emem_type == EMEM_ALL ) || ( emem_type == EMEM_MAIN ) )
    {
        do
        {
            reg_data = drvDB_ReadReg ( 0x3C, 0xE4 );
        }
        while ( reg_data != 0x1C70 );
    }

    switch ( emem_type )
    {
        case EMEM_ALL:
            drvDB_WriteReg ( 0x3C, 0xE4, 0xE38F );  // for all-blocks
            break;
        case EMEM_MAIN:
            drvDB_WriteReg ( 0x3C, 0xE4, 0x7731 );  // for main block
            break;
        case EMEM_INFO:
            drvDB_WriteReg ( 0x3C, 0xE4, 0x7731 );  // for info block

            drvDB_WriteReg8Bit ( 0x0F, 0xE6, 0x01 );

            drvDB_WriteReg8Bit ( 0x3C, 0xE4, 0xC5 ); //
            drvDB_WriteReg8Bit ( 0x3C, 0xE5, 0x78 ); //

            drvDB_WriteReg8Bit ( 0x1E, 0x04, 0x9F );
            drvDB_WriteReg8Bit ( 0x1E, 0x05, 0x82 );

            drvDB_WriteReg8Bit ( 0x0F, 0xE6, 0x00 );
            mdelay ( 100 );
            break;
    }

    // polling 0x3CE4 is 0x2F43
    do
    {
        reg_data = drvDB_ReadReg ( 0x3C, 0xE4 );
    }
    while ( reg_data != 0x2F43 );

    // calculate CRC 32
    Init_CRC32_Table ( &crc_tab[0] );

    for ( i = 0; i < 33; i++ ) // total  33 KB : 2 byte per R/W
    {
        if ( emem_type == EMEM_INFO )
            i = 32;

        if ( i < 32 )   //emem_main
        {
            if ( i == 31 )
            {
                temp[i][1014] = 0x5A; //Fmr_Loader[1014]=0x5A;
                temp[i][1015] = 0xA5; //Fmr_Loader[1015]=0xA5;

                for ( j = 0; j < 1016; j++ )
                {
                    //crc_main=Get_CRC(Fmr_Loader[j],crc_main,&crc_tab[0]);
                    crc_main = Get_CRC ( temp[i][j], crc_main, &crc_tab[0] );
                }
            }
            else
            {
                for ( j = 0; j < 1024; j++ )
                {
                    //crc_main=Get_CRC(Fmr_Loader[j],crc_main,&crc_tab[0]);
                    crc_main = Get_CRC ( temp[i][j], crc_main, &crc_tab[0] );
                }
            }
        }
        else  //emem_info
        {
            for ( j = 0; j < 1024; j++ )
            {
                //crc_info=Get_CRC(Fmr_Loader[j],crc_info,&crc_tab[0]);
                crc_info = Get_CRC ( g_dwiic_info_data[j], crc_info, &crc_tab[0] );
            }
            if ( emem_type == EMEM_MAIN ) break;
        }

        //drvDWIIC_MasterTransmit( DWIIC_MODE_DWIIC_ID, 1024, Fmr_Loader );
        HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX_TP, temp[i], 1024 );

        // polling 0x3CE4 is 0xD0BC
        do
        {
            reg_data = drvDB_ReadReg ( 0x3C, 0xE4 );
        }
        while ( reg_data != 0xD0BC );

        drvDB_WriteReg ( 0x3C, 0xE4, 0x2F43 );
    }

    if ( ( emem_type == EMEM_ALL ) || ( emem_type == EMEM_MAIN ) )
    {
        // write file done and check crc
        drvDB_WriteReg ( 0x3C, 0xE4, 0x1380 );
    }
    mdelay ( 10 ); //MCR_CLBK_DEBUG_DELAY ( 10, MCU_LOOP_DELAY_COUNT_MS );

    if ( ( emem_type == EMEM_ALL ) || ( emem_type == EMEM_MAIN ) )
    {
        // polling 0x3CE4 is 0x9432
        do
        {
            reg_data = drvDB_ReadReg ( 0x3C, 0xE4 );
        }while ( reg_data != 0x9432 );
    }

    crc_main = crc_main ^ 0xffffffff;
    crc_info = crc_info ^ 0xffffffff;

    if ( ( emem_type == EMEM_ALL ) || ( emem_type == EMEM_MAIN ) )
    {
        // CRC Main from TP
        crc_main_tp = drvDB_ReadReg ( 0x3C, 0x80 );
        crc_main_tp = ( crc_main_tp << 16 ) | drvDB_ReadReg ( 0x3C, 0x82 );

        // CRC Info from TP
        crc_info_tp = drvDB_ReadReg ( 0x3C, 0xA0 );
        crc_info_tp = ( crc_info_tp << 16 ) | drvDB_ReadReg ( 0x3C, 0xA2 );
    }
    TP_DEBUG ( "crc_main=0x%x, crc_info=0x%x, crc_main_tp=0x%x, crc_info_tp=0x%x\n",
               crc_main, crc_info, crc_main_tp, crc_info_tp );

    //drvDB_ExitDBBUS();

    update_pass = 1;
    if ( ( emem_type == EMEM_ALL ) || ( emem_type == EMEM_MAIN ) )
    {
        if ( crc_main_tp != crc_main )
            update_pass = 0;

        if ( crc_info_tp != crc_info )
            update_pass = 0;
    }

    if ( !update_pass )
    {
        printk ( "update FAILED\n" );
        _HalTscrHWReset();
        FwDataCnt = 0;
        enable_irq(msg21xx_irq);
        return ( 0 );
    }

    printk ( "update OK\n" );
    _HalTscrHWReset();
    FwDataCnt = 0;
    enable_irq(msg21xx_irq);
    return size;
}
/*[BUGFIX]-MOD BEGIN by TCTSZ.WH,03/18/2014,PR-602310*/
/*[BUGFIX]-ADD BEGIN by TCTSZ.WH,03/31/2014,PR-602310*/
#define _FW_UPDATE_C3_
#ifdef _FW_UPDATE_C3_
#ifdef MSG21XX_APK_UPDATE
static ssize_t firmware_update_store ( struct device *dev,
                                       struct device_attribute *attr, const char *buf, size_t size )
#else	//#ifdef MSG21xx_AUTO_UPDATE
static ssize_t firmware_update_store (size_t size )
#endif
/*[BUGFIX]-ADD END by TCTSZ.WH,03/31/2014,PR-602310*/
{
    u8 dbbus_tx_data[4];
    unsigned char dbbus_rx_data[2] = {0};
    disable_irq(msg21xx_irq);

    _HalTscrHWReset();

    // Erase TP Flash first
    dbbusDWIICEnterSerialDebugMode();
    dbbusDWIICStopMCU();
    dbbusDWIICIICUseBus();
    dbbusDWIICIICReshape();
    mdelay ( 300 );

    // Disable the Watchdog
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x3C;
    dbbus_tx_data[2] = 0x60;
    dbbus_tx_data[3] = 0x55;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x3C;
    dbbus_tx_data[2] = 0x61;
    dbbus_tx_data[3] = 0xAA;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );
    // Stop MCU
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x0F;
    dbbus_tx_data[2] = 0xE6;
    dbbus_tx_data[3] = 0x01;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );
    /////////////////////////
    // Difference between C2 and C3
    /////////////////////////
    // c2:2133 c32:2133a(2) c33:2138
    //check id
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0xCC;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 3 );
    HalTscrCReadI2CSeq ( FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2 );
    if ( dbbus_rx_data[0] == 2 )//0x2133
    {
        // check version
        dbbus_tx_data[0] = 0x10;
        dbbus_tx_data[1] = 0x3C;
        dbbus_tx_data[2] = 0xEA;
        HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 3 );
        HalTscrCReadI2CSeq ( FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2 );
        TP_DEBUG ( "dbbus_rx version[0]=0x%x", dbbus_rx_data[0] );

        if ( dbbus_rx_data[0] == 3 ){
            return firmware_update_c33 (size, EMEM_MAIN );
        }
        else{

            return firmware_update_c32 (size, EMEM_ALL );
        }
    }
    else
    {
        return firmware_update_c2 (size );
    } 
}

/*[BUGFIX]-MOD END by TCTSZ.WH,03/18/2014,PR-602310*/
#else
static ssize_t firmware_update_store ( struct device *dev,
                                       struct device_attribute *attr, const char *buf, size_t size )
{
    u8 i;
    u8 dbbus_tx_data[4];
    unsigned char dbbus_rx_data[2] = {0};

    _HalTscrHWReset();

    // 1. Erase TP Flash first
    dbbusDWIICEnterSerialDebugMode();
    dbbusDWIICStopMCU();
    dbbusDWIICIICUseBus();
    dbbusDWIICIICReshape();
    mdelay ( 300 );

    // Disable the Watchdog
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x3C;
    dbbus_tx_data[2] = 0x60;
    dbbus_tx_data[3] = 0x55;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x3C;
    dbbus_tx_data[2] = 0x61;
    dbbus_tx_data[3] = 0xAA;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // Stop MCU
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x0F;
    dbbus_tx_data[2] = 0xE6;
    dbbus_tx_data[3] = 0x01;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // set FRO to 50M
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x11;
    dbbus_tx_data[2] = 0xE2;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 3 );
    dbbus_rx_data[0] = 0;
    dbbus_rx_data[1] = 0;
    HalTscrCReadI2CSeq ( FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2 );
    TP_DEBUG ( "dbbus_rx_data[0]=0x%x", dbbus_rx_data[0] );
    dbbus_tx_data[3] = dbbus_rx_data[0] & 0xF7;  //Clear Bit 3
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // set MCU clock,SPI clock =FRO
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x22;
    dbbus_tx_data[3] = 0x00;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x23;
    dbbus_tx_data[3] = 0x00;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // Enable slave's ISP ECO mode
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x08;
    dbbus_tx_data[2] = 0x0c;
    dbbus_tx_data[3] = 0x08;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // Enable SPI Pad
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x02;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 3 );
    HalTscrCReadI2CSeq ( FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2 );
    TP_DEBUG ( "dbbus_rx_data[0]=0x%x", dbbus_rx_data[0] );
    dbbus_tx_data[3] = ( dbbus_rx_data[0] | 0x20 ); //Set Bit 5
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // WP overwrite
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x0E;
    dbbus_tx_data[3] = 0x02;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // set pin high
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x10;
    dbbus_tx_data[3] = 0x08;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    dbbusDWIICIICNotUseBus();
    dbbusDWIICNotStopMCU();
    dbbusDWIICExitSerialDebugMode();

    drvISP_EntryIspMode();
    drvISP_ChipErase();
    _HalTscrHWReset();
    mdelay ( 300 );

    // 2.Program and Verify
    dbbusDWIICEnterSerialDebugMode();
    dbbusDWIICStopMCU();
    dbbusDWIICIICUseBus();
    dbbusDWIICIICReshape();

    // Disable the Watchdog
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x3C;
    dbbus_tx_data[2] = 0x60;
    dbbus_tx_data[3] = 0x55;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x3C;
    dbbus_tx_data[2] = 0x61;
    dbbus_tx_data[3] = 0xAA;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // Stop MCU
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x0F;
    dbbus_tx_data[2] = 0xE6;
    dbbus_tx_data[3] = 0x01;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // set FRO to 50M
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x11;
    dbbus_tx_data[2] = 0xE2;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 3 );
    dbbus_rx_data[0] = 0;
    dbbus_rx_data[1] = 0;
    HalTscrCReadI2CSeq ( FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2 );
    TP_DEBUG ( "dbbus_rx_data[0]=0x%x", dbbus_rx_data[0] );
    dbbus_tx_data[3] = dbbus_rx_data[0] & 0xF7;  //Clear Bit 3
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // set MCU clock,SPI clock =FRO
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x22;
    dbbus_tx_data[3] = 0x00;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x23;
    dbbus_tx_data[3] = 0x00;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // Enable slave's ISP ECO mode
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x08;
    dbbus_tx_data[2] = 0x0c;
    dbbus_tx_data[3] = 0x08;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // Enable SPI Pad
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x02;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 3 );
    HalTscrCReadI2CSeq ( FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2 );
    TP_DEBUG ( "dbbus_rx_data[0]=0x%x", dbbus_rx_data[0] );
    dbbus_tx_data[3] = ( dbbus_rx_data[0] | 0x20 ); //Set Bit 5
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // WP overwrite
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x0E;
    dbbus_tx_data[3] = 0x02;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    // set pin high
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x10;
    dbbus_tx_data[3] = 0x08;
    HalTscrCDevWriteI2CSeq ( FW_ADDR_MSG21XX, dbbus_tx_data, 4 );

    dbbusDWIICIICNotUseBus();
    dbbusDWIICNotStopMCU();
    dbbusDWIICExitSerialDebugMode();

    ///////////////////////////////////////
    // Start to load firmware
    ///////////////////////////////////////
    drvISP_EntryIspMode();

    for ( i = 0; i < 94; i++ ) // total  94 KB : 1 byte per R/W
    {
        drvISP_Program ( i, temp[i] ); // program to slave's flash
        drvISP_Verify ( i, temp[i] ); //verify data
    }
    TP_DEBUG ( "update OK\n" );
    drvISP_ExitIspMode();
    FwDataCnt = 0;
    
    return size;
}
#endif
#ifdef MSG21XX_APK_UPDATE	/*[BUGFIX]-ADD by TCTSZ.WH,03/31/2014,PR-602310*/
static DEVICE_ATTR(update, 0777, firmware_update_show, firmware_update_store);
#endif		/*[BUGFIX]-ADD by TCTSZ.WH,03/31/2014,PR-602310*/
#if 0
/*test=================*/
static ssize_t firmware_clear_show(struct device *dev,
                                    struct device_attribute *attr, char *buf)
{
    printk(" +++++++ [%s] Enter!++++++\n", __func__);
    u16 k=0,i = 0, j = 0;
    u8 bWriteData[5] =
    {
        0x10, 0x03, 0, 0, 0
    };
    u8 RX_data[256];
    u8 bWriteData1 = 0x12;
    u32 addr = 0;
    u32 timeOutCount=0;
    for (k = 0; k < 94; i++)   // total  94 KB : 1 byte per R/W
    {
        addr = k * 1024;
        for (j = 0; j < 8; j++)   //128*8 cycle
        {
            bWriteData[2] = (u8)((addr + j * 128) >> 16);
            bWriteData[3] = (u8)((addr + j * 128) >> 8);
            bWriteData[4] = (u8)(addr + j * 128);
            //msctpc_LoopDelay ( 1 );        // delay about 100us*****
            udelay(150);//200);

            timeOutCount=0;
            while ( ( drvISP_ReadStatus() & 0x01 ) == 0x01 )
            {
                timeOutCount++;
                if ( timeOutCount >= 100000 ) 
                    break; /* around 1 sec timeout */
            }
        
            HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, bWriteData, 5);    //write read flash addr
            //msctpc_LoopDelay ( 1 );        // delay about 100us*****
            udelay(150);//200);
            drvISP_Read(128, RX_data);
            HalTscrCDevWriteI2CSeq(FW_UPDATE_ADDR_MSG21XX, &bWriteData1, 1);    //cmd end
            for (i = 0; i < 128; i++)   //log out if verify error
            {
                if (RX_data[i] != 0xFF)
                {
                    //TP_DEBUG(printk("k=%d,j=%d,i=%d===============erase not clean================",k,j,i);)
                    printk("k=%d,j=%d,i=%d  erase not clean !!",k,j,i);
                }
            }
        }
    }
    TP_DEBUG("read finish\n");
    return sprintf(buf, "%s\n", fw_version);
}

static ssize_t firmware_clear_store(struct device *dev,
                                     struct device_attribute *attr, const char *buf, size_t size)
{

    u8 dbbus_tx_data[4];
    unsigned char dbbus_rx_data[2] = {0};
    printk(" +++++++ [%s] Enter!++++++\n", __func__);
    //msctpc_LoopDelay ( 100 );        // delay about 100ms*****

    // Enable slave's ISP ECO mode

    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x08;
    dbbus_tx_data[2] = 0x0c;
    dbbus_tx_data[3] = 0x08;
    
    // Disable the Watchdog
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x11;
    dbbus_tx_data[2] = 0xE2;
    dbbus_tx_data[3] = 0x00;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x3C;
    dbbus_tx_data[2] = 0x60;
    dbbus_tx_data[3] = 0x55;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x3C;
    dbbus_tx_data[2] = 0x61;
    dbbus_tx_data[3] = 0xAA;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

    //Stop MCU
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x0F;
    dbbus_tx_data[2] = 0xE6;
    dbbus_tx_data[3] = 0x01;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

    //Enable SPI Pad
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x02;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 3);
    HalTscrCReadI2CSeq(FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2);
    TP_DEBUG(printk("dbbus_rx_data[0]=0x%x", dbbus_rx_data[0]);)
    dbbus_tx_data[3] = (dbbus_rx_data[0] | 0x20);  //Set Bit 5
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x25;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 3);

    dbbus_rx_data[0] = 0;
    dbbus_rx_data[1] = 0;
    HalTscrCReadI2CSeq(FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2);
    TP_DEBUG(printk("dbbus_rx_data[0]=0x%x", dbbus_rx_data[0]);)
    dbbus_tx_data[3] = dbbus_rx_data[0] & 0xFC;  //Clear Bit 1,0
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

    //WP overwrite
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x0E;
    dbbus_tx_data[3] = 0x02;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);


    //set pin high
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x1E;
    dbbus_tx_data[2] = 0x10;
    dbbus_tx_data[3] = 0x08;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);
    //set FRO to 50M
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x11;
    dbbus_tx_data[2] = 0xE2;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 3);
    dbbus_rx_data[0] = 0;
    dbbus_rx_data[1] = 0;
    HalTscrCReadI2CSeq(FW_ADDR_MSG21XX, &dbbus_rx_data[0], 2);
    TP_DEBUG(printk("dbbus_rx_data[0]=0x%x", dbbus_rx_data[0]);)
    dbbus_tx_data[3] = dbbus_rx_data[0] & 0xF7;  //Clear Bit 1,0
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX, dbbus_tx_data, 4);

    dbbusDWIICIICNotUseBus();
    dbbusDWIICNotStopMCU();
    dbbusDWIICExitSerialDebugMode();

    ///////////////////////////////////////
    // Start to load firmware
    ///////////////////////////////////////
    drvISP_EntryIspMode();
    TP_DEBUG(printk("chip erase+\n");)
    drvISP_BlockErase(0x00000);
    TP_DEBUG(printk("chip erase-\n");)
    drvISP_ExitIspMode();
    return size;
}
static DEVICE_ATTR(clear, 0777, firmware_clear_show, firmware_clear_store);
#endif //0
/*test=================*/
/*Add by Tracy.Lin for update touch panel firmware and get fw version*/
#ifdef MSG21XX_APK_UPDATE	/*[BUGFIX]-ADD by TCTSZ.WH,03/31/2014,PR-602310*/
static ssize_t firmware_version_show(struct device *dev,
                                     struct device_attribute *attr, char *buf)
{
    TP_DEBUG("*** firmware_version_show fw_version = %s***\n", fw_version);
    return sprintf(buf, "%s\n", fw_version);
}

static ssize_t firmware_version_store(struct device *dev,
                                      struct device_attribute *attr, const char *buf, size_t size)
{
    unsigned char dbbus_tx_data[3];
    unsigned char dbbus_rx_data[4] ;
    unsigned short major=0, minor=0;
/*
    dbbusDWIICEnterSerialDebugMode();
    dbbusDWIICStopMCU();
    dbbusDWIICIICUseBus();
    dbbusDWIICIICReshape();

*/
    fw_version = kzalloc(sizeof(char), GFP_KERNEL);

    //Get_Chip_Version();
    dbbus_tx_data[0] = 0x53;
    dbbus_tx_data[1] = 0x00;
    dbbus_tx_data[2] = 0x2A;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX_TP, &dbbus_tx_data[0], 3);
    HalTscrCReadI2CSeq(FW_ADDR_MSG21XX_TP, &dbbus_rx_data[0], 4);

    major = (dbbus_rx_data[1]<<8)+dbbus_rx_data[0];
    minor = (dbbus_rx_data[3]<<8)+dbbus_rx_data[2];

    TP_DEBUG("***major = %d ***\n", major);
    TP_DEBUG("***minor = %d ***\n", minor);
    sprintf(fw_version,"%03d%03d", major, minor);
    //TP_DEBUG(printk("***fw_version = %s ***\n", fw_version);)

    return size;
}
static DEVICE_ATTR(version, 0777, firmware_version_show, firmware_version_store);

/*[BUGFIX]-MOD BEGIN by TCTSZ.WH,03/18/2014,PR-602310*/


static ssize_t firmware_data_show(struct device *dev,
                                  struct device_attribute *attr, char *buf)
{
    return FwDataCnt;
}

static ssize_t firmware_data_store(struct device *dev,
                                   struct device_attribute *attr, const char *buf, size_t size)
 #elif defined(MSG21XX_AUTO_UPDATE)	//#ifdef MSG21XX_AUTO_UPDATE
static ssize_t firmware_data_store( const char *buf, size_t size)
#endif	//end of MSG21XX_AUTO_UPDATE
{
/*[BUGFIX]-ADD BEGIN by TCTSZ.WH,03/31/2014,PR-602310*/
#ifdef MSG21XX_AUTO_UPDATE
    int i = 0;
    for (i = 0; i < 94; i++)
#endif
    {
#ifdef MSG21XX_AUTO_UPDATE
        memcpy(temp[FwDataCnt], &buf[i*1024], 1024);
#elif defined(MSG21XX_APK_UPDATE)
	memcpy(temp[FwDataCnt], buf, 1024);
#endif
	FwDataCnt++;
	printk("***  firmware_data_store FwDataCnt = %d ***\n", FwDataCnt);
    }
    return size;
/*[BUGFIX]-ADD END by TCTSZ.WH,03/31/2014,PR-602310*/
}
/*[BUGFIX]-MOD END by TCTSZ.WH,03/18/2014,PR-602310*/
#ifdef MSG21XX_APK_UPDATE 
static DEVICE_ATTR(data, 0777, firmware_data_show, firmware_data_store);
#endif

/*[BUGFIX]-ADD BEGIN by TCTSZ.WH,03/18/2014,PR-602310*/
static void get_chip_version(struct fw_version *fw_ver)
{
    unsigned char dbbus_tx_data[3];
    unsigned char dbbus_rx_data[4] ;

    dbbus_tx_data[0] = 0x53;
    dbbus_tx_data[1] = 0x00;
    dbbus_tx_data[2] = 0x2A;
    HalTscrCDevWriteI2CSeq(FW_ADDR_MSG21XX_TP, &dbbus_tx_data[0], 3);
    HalTscrCReadI2CSeq(FW_ADDR_MSG21XX_TP, &dbbus_rx_data[0], 4);

    fw_ver->major = (dbbus_rx_data[1]<<8)+dbbus_rx_data[0];
    fw_ver->minor = (dbbus_rx_data[3]<<8)+dbbus_rx_data[2];

    printk("%s, major = 0x%x, minor = 0x%x\n", __func__, fw_ver->major, fw_ver->minor);
}

/*[BUGFIX]-ADD BEGIN by TCTSZ.WH,03/31/2014,PR-602310*/
//send 0x5800 to 0x26
static void tp_enter_doze(void)
{
    unsigned char dbbus_tx_data[3];

    dbbus_tx_data[0] = 0x58;
    dbbus_tx_data[1] = 0x00;
	dbbus_tx_data[2] = 0x01;
    HalTscrCDevWriteI2CSeq(0x26, &dbbus_tx_data[0], 3);  //FW_ADDR_MSG21XX_TP,
}
/*[BUGFIX]-ADD END by TCTSZ.WH,03/31/2014,PR-602310*/

static void msg21xx_suspend(struct msg21xx_ts_data *data)
{
    if (data->suspended) {
           dev_info(&data->client->dev, "Already in suspend state\n");
           return;
    }
/*[BUGFIX]-MOD by TCTSZ.WH,03/31/2014,PR-602310*/
    tp_enter_doze();
    data->suspended = 1;
#if 0
    disable_irq(msg21xx_irq);
    gpio_direction_output(sprd_3rdparty_gpio_tp_rst, 0);
    gpio_set_value(sprd_3rdparty_gpio_tp_rst, 0);

    msleep(20);  /* Note that the RST must be in LOW 10ms at least */
    tp_pwroff(data);

#endif
/*[BUGFIX]-MOD by TCTSZ.WH,03/31/2014,PR-602310*/
}

static void msg21xx_resume(struct msg21xx_ts_data *data)
{
    if (!data->suspended) {
            dev_info(&data->client->dev, "Already in awake state\n");
            return;
    }

    tp_pwron(data);

    gpio_set_value(sprd_3rdparty_gpio_tp_rst, 1);
//  ret = gpio_get_value(sprd_3rdparty_gpio_tp_rst);
//  TP_DEBUG(printk("[%s]sprd_3rdparty_gpio_tp_rst value = %d\n", __func__, ret);)
    msleep(20);  /* Note that the RST must be in LOW 10ms at least */
    gpio_set_value(sprd_3rdparty_gpio_tp_rst, 0);
    msleep(20);  /* Note that the RST must be in LOW 10ms at least */
    gpio_set_value(sprd_3rdparty_gpio_tp_rst, 1);
    /* Enable the interrupt service thread/routine for INT after 50ms */
    msleep(200);
    enable_irq(msg21xx_irq);

data->suspended = 0;
    printk("resume ok ----------msg2133 \n");
}

#if defined(CONFIG_FB)
static int fb_notifier_callback(struct notifier_block *self,
                                 unsigned long event, void *data)
{
        struct fb_event *evdata = data;
        int *blank;
        struct msg21xx_ts_data *msg21_data =
                container_of(self, struct msg21xx_ts_data, fb_notif);

        if (evdata && evdata->data && event == FB_EVENT_BLANK &&
                        msg21_data && msg21_data->client) {
                blank = evdata->data;
                if (*blank == FB_BLANK_UNBLANK)
                        msg21xx_resume(msg21_data);
                else if (*blank == FB_BLANK_POWERDOWN)
                        msg21xx_suspend(msg21_data);
        }

        return 0;
}
#elif defined(CONFIG_HAS_EARLYSUSPEND)
static void msg21xx_early_suspend(struct early_suspend *handler)
{
        struct msg21xx_ts_data *msg21_data = container_of(handler,
                                                   struct msg21xx_ts_data,
                                                   early_suspend);

        msg21xx_suspend(msg21_data);
}

static void msg21xx_late_resume(struct early_suspend *handler)
{
        struct msg21xx_ts_data *msg21_data = container_of(handler,
                                                   struct msg21xx_ts_data,
                                                   early_suspend);

        msg21xx_resume(msg21_data);
}
#endif
/*[BUGFIX]-ADD END by TCTSZ.WH,03/18/2014,PR-602310*/

static u8 Calculate_8BitsChecksum( u8 *msg, s32 s32Length )
{
    s32 s32Checksum = 0;
    s32 i;

    for ( i = 0 ; i < s32Length; i++ )
    {
        s32Checksum += msg[i];
    }

    return (u8)( ( -s32Checksum ) & 0xFF );
}
static void msg2133_xy(u16 *x, u16 *y)
{
    *x = MS_TS_MSG21XX_X_MAX - *x;
    *y = MS_TS_MSG21XX_Y_MAX - *y;
}
#define FT_PRESS        0x7F
static u32 bakx,baky;

static void msg21xx_data_disposal(struct work_struct *work)
{
    int tempx;
    int tempy;

    u8 val[8] = {0};
    u8 Checksum = 0;
    u8 i;
    u32 delta_x = 0, delta_y = 0;
    u32 u32X = 0;
    u32 u32Y = 0;
    //u8 touchkeycode = 0;//[BUGFIX]-DEL BEGRIN by TCTNB.XQJ,08/19/2013,FR-487707
    TouchScreenInfo_t *touchData;
    static u32 preKeyStatus=0;
    //static u32 preFingerNum=0;


    touchData = kzalloc(sizeof(TouchScreenInfo_t), GFP_KERNEL);
    memset(touchData, 0, sizeof(TouchScreenInfo_t));

    i2c_master_recv(msg21xx_i2c_client,&val[0],REPORT_PACKET_LENGTH);
    Checksum = Calculate_8BitsChecksum(&val[0], 7); //calculate checksum

 /*[BUGFIX]-ADD BEGIN by TCTSZ.WH,03/31/2014,PR-602310*/
    if((0x52 == val[0]) && (0xFF == val[1]) && (0xFF == val[2]) && (0xFF == val[3]) && (0xFF == val[4]) && (0x58 == val[5]) && (0xFF == val[6]) && (0x5B == val[7]))
    {
	printk("%s,%d\n",__func__,__LINE__);

                                input_report_key(input, KEY_POWER, 1);
                                input_sync(input);
                                input_report_key(input, KEY_POWER, 0);
                                input_sync(input);

    }
    else if ((Checksum == val[7]) && (val[0] == 0x52))   //check the checksum  of packet
/*[BUGFIX]-ADD END by TCTSZ.WH,03/31/2014,PR-602310*/
	{
        u32X = (((val[1] & 0xF0) << 4) | val[2]);         //parse the packet to coordinates
        u32Y = (((val[1] & 0x0F) << 8) | val[3]);
        delta_x = (((val[4] & 0xF0) << 4) | val[5]);
        delta_y = (((val[4] & 0x0F) << 8) | val[6]);

if(SWAP_X_Y){
        tempy = u32X;
        tempx = u32Y;
        u32X = tempx;
        u32Y = tempy;
        
        tempy = delta_x;
        tempx = delta_y;
        delta_x = tempx;
        delta_y = tempy;
}
        /*val[0] id; val[1]-val[3]the first point abs val[4]-val[6] the second point abs
        when val[1]-val[4],val[6]is 0xFF, keyevent,val[5] to judge which key press.
        val[1]-val[6] all are 0xFF, release touch*/
        if ((val[1] == 0xFF) && (val[2] == 0xFF) && (val[3] == 0xFF) && (val[4] == 0xFF) && (val[6] == 0xFF))
        {
            touchData->Point[0].X = 0; // final X coordinate
            touchData->Point[0].Y = 0; // final Y coordinate

            if ((val[5] != 0x0) && val[5] != 0xFF) /*val[5] is key value*/
            {/*0x0 is key up, 0xff is touchscreen up*/
                //DBUG("touch key down! val[5] = %d \n", val[5]);
//              touchData->nTouchKeyMode = 1; //TouchKeyMode
//              touchData->nTouchKeyCode = val[5]; //TouchKeyCode
                touchData->nFingerNum = 1;

                touchData->nTouchKeyCode = val[5];
                /*
                 * del by guanhua
                 *
                 switch(val[5]){
                 case 4:
                 touchData->nTouchKeyCode = 4;
                 break;
                 case 8:
                 touchData->nTouchKeyCode = 8;
                 break;
                 case 1:
                 touchData->nTouchKeyCode = 1;
                 break;
                 case 2:
                 touchData->nTouchKeyCode = 2;
                 break;                 
                 }
                 */
            }
            else
            { /*key up or touch up*/
                // DBUG(" touch key up \n");//.16
                touchData->nFingerNum = 0; //touch end
                touchData->nTouchKeyCode = 0; //TouchKeyCode
                touchData->nTouchKeyMode = 0; //TouchKeyMode    
                //printk("1 release\n");            
                //msg21xx_release();
            }
        } else  {
            touchData->nTouchKeyMode = 0; //Touch on screen...

            if ((delta_x == 0) && (delta_y == 0))
            {   /*one touch point*/
                touchData->nFingerNum = 1; //one touch
/* [BUGFIX]-Mod Begin by fangyou.wang 2014/02/18,for bug602310,used for rio4g*/
                //touchData->Point[0].X = (u32X * MS_TS_MSG21XX_X_MAX) / 2048;
                //touchData->Point[0].Y = (u32Y * MS_TS_MSG21XX_Y_MAX) / 2048;
                touchData->Point[0].X = MS_TS_MSG21XX_X_MAX-(u32X * MS_TS_MSG21XX_X_MAX) / 2048;
                touchData->Point[0].Y =MS_TS_MSG21XX_Y_MAX-(u32Y * MS_TS_MSG21XX_Y_MAX) / 2048;
/* [BUGFIX]-Mod End by fangyou.wang 2014/02/18*/
               //DBUG("****[%s]: u32X = %d, u32Y = %d\n", __func__, u32X, u32Y);
                //DBUG("[%s]: x = %d, y = %d\n", __func__, touchData->Point[0].X, touchData->Point[0].Y);
            }
            else
            { /*two touch points*/
                u32 x2, y2;
                touchData->nFingerNum = 2; //two touch
                /* Finger 1 */
/* [BUGFIX]-Mod Begin by fangyou.wang 2014/02/18,for bug602310,used for rio4g*/
                touchData->Point[0].X = MS_TS_MSG21XX_X_MAX-(u32X * MS_TS_MSG21XX_X_MAX) / 2048;
                touchData->Point[0].Y = MS_TS_MSG21XX_Y_MAX-(u32Y * MS_TS_MSG21XX_Y_MAX) / 2048;
/* [BUGFIX]-Mod End by fangyou.wang 2014/02/18*/
                //DBUG("[%s]: x = %d, y = %d\n", __func__, touchData->Point[0].X, touchData->Point[0].Y);
                /* Finger 2 */
                if (delta_x > 2048)     //transform the unsigh value to sign value
                {
                    delta_x -= 4096;
                }
                if (delta_y > 2048)
                {
                    delta_y -= 4096;
                }

                x2 = (u32)(u32X + delta_x);
                y2 = (u32)(u32Y + delta_y);
/* [BUGFIX]-Mod Begin by fangyou.wang 2014/02/18,for bug602310,used for rio4g*/
                touchData->Point[1].X = MS_TS_MSG21XX_X_MAX-(x2 * MS_TS_MSG21XX_X_MAX) / 2048;//2048;
                touchData->Point[1].Y = MS_TS_MSG21XX_Y_MAX-(y2 * MS_TS_MSG21XX_Y_MAX) / 2048;//2048;
/* [BUGFIX]-Mod End by fangyou.wang 2014/02/18*/
            }
        }

        //report...
            if((touchData->nFingerNum) == 0)   //touch end
            {
                //printk("2 release!..........\n");
                preKeyStatus=0; //clear key status..
            
                msg21xx_release();
            }
            else //touch on screen
            {
/*[BUGFIX]-DEL BEGIN by TCTNB.XQJ,08/19/2013,FR-487707*/
#if 0 
                //printk("key point...\n");
                if(touchData->nTouchKeyCode != 0)
                {
                    if (touchData->nTouchKeyCode == 4)
                        touchkeycode = key[0];           //TOUCH_KEY_MENU;
                    if (touchData->nTouchKeyCode == 1)
                        touchkeycode = key[2];           //TOUCH_KEY_BACK;
                    if (touchData->nTouchKeyCode == 2)
                        touchkeycode = key[1];           //TOUCH_KEY_HOME;
                //  if (touchData->nTouchKeyCode == 8)
                        //touchkeycode = key[0];           //TOUCH_KEY_SEARCH;   

                    if(preKeyStatus!=touchkeycode)
                    {
                             // printk("useful key code report touch key code = %d, prekey = %d \n",touchkeycode, preKeyStatus);
                                preKeyStatus=touchkeycode;
                                input_report_key(input, touchkeycode, 1);
                    
                    }
                //   input_sync(input);
                }
                else
                {
#endif		
/*[BUGFIX]-END by TCTNB.XQJ*/
                    input_report_key(input, BTN_TOUCH, 1);
                    //printk("touchData->nFingerNum = %d....ctp_id=%d...........\n", touchData->nFingerNum,ctp_id);
                    for(i = 0;i < (touchData->nFingerNum);i++) {
                    //  input_report_abs(input, ABS_MT_TOUCH_MAJOR, 255);
                        if(ctp_id == 2)
                            msg2133_xy(&touchData->Point[i].X, &touchData->Point[i].Y);
/*[BUGFIX]-DEL BEGIN by TCTNB.XQJ,08/19/2013,FR-487707,change from key value to x,y*/                 
				  if(touchData->nTouchKeyCode != 0)
                  {
                    if (touchData->nTouchKeyCode == 4)
                    {
                         //touchkeycode = key[0];           //TOUCH_KEY_MENU;
/* [BUGFIX]-Mod Begin by fangyou.wang 2014/02/18,for bug602310,used for rio4g*/
                         touchData->Point[0].X=450;
                         touchData->Point[0].Y=1000;
/* [BUGFIX]-Mod End by fangyou.wang 2014/02/18*/
                    }
                    if (touchData->nTouchKeyCode == 1)
                    {
                         //touchkeycode = key[2];           //TOUCH_KEY_BACK;
                         touchData->Point[0].X=40;
                         touchData->Point[0].Y=1000;

                    }
                    if (touchData->nTouchKeyCode == 2)
                    {
                         //touchkeycode = key[1];           //TOUCH_KEY_HOME;
/* [BUGFIX]-Mod Begin by fangyou.wang 2014/02/18,for bug602310,used for rio4g*/
                         touchData->Point[0].X=240;
                         touchData->Point[0].Y=1000;
/* [BUGFIX]-Mod End by fangyou.wang 2014/02/18*/
                    }
                   }
/*[BUGFIX]-END by TCTNB.XQJ*/
                        input_report_abs(input, ABS_MT_TOUCH_MAJOR, FT_PRESS);//1);
                        //DBUG("[%s]: x = %d, y = %d\n", __func__, touchData->Point[i].X, touchData->Point[i].Y);
                        input_report_abs(input, ABS_MT_POSITION_X, touchData->Point[i].X);
                        input_report_abs(input, ABS_MT_POSITION_Y, touchData->Point[i].Y);
                        input_report_abs(input, ABS_MT_PRESSURE,FT_PRESS);
                            bakx=touchData->Point[0].X;
                    baky=touchData->Point[0].Y;
                        input_mt_sync(input);
                    }
               // }
                input_sync(input);
            }
    }
    else
    {
        printk("val[0] = 0x%x, val[7] = 0x%x, Checksum = 0x%x\n", val[0], val[7], Checksum);
        printk(KERN_ERR "err status in tp\n");
    }
    kfree(touchData);
    enable_irq(msg21xx_irq);
}

static int msg21xx_ts_open(struct input_dev *dev)
{
    return 0;
}

static void msg21xx_ts_close(struct input_dev *dev)
{
    printk("msg21xx_ts_close\n");
}


static int msg21xx_init_input(void)
{
    int err = 0;
    input = input_allocate_device();
    if (!input) {
        dev_err(&msg21xx_i2c_client->dev, "failed to allocate input device\n");
        goto exit_input_dev_alloc_failed;
    }
    
    input->name = "ms-msg21xx";/*[BUGFIX]-MOD by TCTNB.XQJ,08/19/2013,FR-487707*/
    input->phys = "I2C";
    input->id.bustype = BUS_I2C;
    input->dev.parent = &msg21xx_i2c_client->dev;
    input->open = msg21xx_ts_open;
    input->close = msg21xx_ts_close;

    set_bit(EV_ABS, input->evbit);
    set_bit(EV_SYN, input->evbit);
    set_bit(EV_KEY, input->evbit);
    set_bit(TOUCH_KEY_BACK, input->keybit);
    set_bit(TOUCH_KEY_MENU, input->keybit);
    set_bit(INPUT_PROP_DIRECT, input->propbit);
    set_bit(TOUCH_KEY_HOME, input->keybit);
    //set_bit(TOUCH_KEY_SEARCH, input->keybit);
    set_bit(BTN_TOUCH, input->keybit);

    input_set_abs_params(input, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
    input_set_abs_params(input, ABS_MT_POSITION_X, 0, MS_TS_MSG21XX_X_MAX, 0, 0);
    input_set_abs_params(input, ABS_MT_POSITION_Y, 0, MS_TS_MSG21XX_Y_MAX, 0, 0);

    input_set_abs_params(input, ABS_MT_PRESSURE, 0, FT_PRESS, 0, 0);
    input_set_abs_params(input, ABS_MT_TRACKING_ID, 0,
                 2, 0, 0);//touch  point count

    err = input_register_device(input);
    if (err) {
        dev_err(&msg21xx_i2c_client->dev,
        "msg_ts_probe: failed to register input device: %s\n",
        dev_name(&msg21xx_i2c_client->dev));
        goto exit_input_register_device_failed;
    }
    return 0;

exit_input_register_device_failed:
    input_free_device(input);
exit_input_dev_alloc_failed:
    return -1;
}
static irqreturn_t msg21xx_interrupt(int irq, void *dev_id)
{
    disable_irq_nosync(msg21xx_irq);
    schedule_work(&msg21xx_wq);
    return IRQ_HANDLED;
}

/////////////////////////
/***********************************************************************************************
    add by liukai for virtualkeys 
***********************************************************************************************/
static struct kobject *properties_kobj;

static ssize_t msg21xx_virtual_keys_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
 /*[BUGFIX]-DEL BEGRIN by TCTNB.XQJ,08/19/2013,FR-487707,change vk defintion*/ 
        return snprintf(buf, 200,
/* [BUGFIX]-Mod Begin by fangyou.wang 2014/02/18,for bug602310,used for rio4g*/
	__stringify(EV_KEY) ":" __stringify(KEY_MENU)  ":400:900:100:120"
	":" __stringify(EV_KEY) ":" __stringify(KEY_HOME)   ":240:900:100:120"
	":" __stringify(EV_KEY) ":" __stringify(KEY_BACK)   ":80:900:100:120"
//	__stringify(EV_KEY) ":" __stringify(KEY_MENU)  ":460:1000:100:60"
//	":" __stringify(EV_KEY) ":" __stringify(KEY_HOME)   ":260:1000:100:60"
//	":" __stringify(EV_KEY) ":" __stringify(KEY_BACK)   ":70:1000:100:60"
/* [BUGFIX]-Mod End by fangyou.wang 2014/02/18*/
	"\n");
/*[BUGFIX]-END by TCTNB.XQJ*/
}

static struct kobj_attribute msg21xx_virtual_keys_attr = {
    .attr = {
        .name = "virtualkeys.ms-msg21xx",
        .mode = S_IRUGO,
    },
    .show = &msg21xx_virtual_keys_show,
};

static struct attribute *msg21xx_properties_attrs[] = {
    &msg21xx_virtual_keys_attr.attr,
    NULL
};

static struct attribute_group msg21xx_properties_attr_group = {
    .attrs = msg21xx_properties_attrs
};

static int msg21xx_virtual_keys_init(void) 
{
   
    int ret=0;
    properties_kobj = kobject_create_and_add("board_properties", NULL);
    if (properties_kobj)
        ret = sysfs_create_group(properties_kobj,&msg21xx_properties_attr_group);
    if (!properties_kobj || ret)
        printk(KERN_ERR "failed to create board_properties\n");
    return ret;
}

/////////////////////////

static s8 msg2133_i2c_test(struct i2c_client *client)
{
    int rc;
    char temp;
    
    struct i2c_msg msgs[] =
    {
        {
            .addr = client->addr,//0xc4 >> 1,
            .flags = 0,//I2C_M_RD,
            .len = 1,
            .buf = &temp,
        },
    };
    printk("client addr : %x........................\n", client->addr);
    rc = i2c_transfer(msg21xx_i2c_client->adapter, msgs, 1);
    
    if( rc < 0 )
    {
        printk("HalTscrCReadI2CSeq error %d\n", rc);
        return rc;
    }

    msleep(10);
    return 0;
}
// proc file create tp_info...
#ifdef TP_DEVICE_INFO
static int tp_info_read_proc_t(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    return sprintf(page, "%s%s", "msg2133_", tp_info);
}
static int tp_info_write_proc_t(struct file *file, const char __user *buffer, unsigned long count, void *data)
{
    return 0;
}
#endif

static void tp_reset(void)
{
    gpio_set_value(sprd_3rdparty_gpio_tp_rst, 1);
    gpio_set_value(sprd_3rdparty_gpio_tp_rst, 0);
    msleep(200);  /* Note that the RST must be in LOW 10ms at least */
    gpio_set_value(sprd_3rdparty_gpio_tp_rst, 1);
    /* Enable the interrupt service thread/routine for INT after 50ms */
    msleep(50);
}

/* [BUGFIX]-Mod Begin by fangyou.wang 2014/02/18,for bug602310,used for rio4g 2nd tp rest pin err*/
static int tp_config_pins(void)
{
    //int tp_irq;
    if (gpio_request(sprd_3rdparty_gpio_tp_rst, "gpio_tp_rst") < 0) {
        printk("msg21xx gpio tp_rst request err\n");
        //goto err_request_tprst;
		return -1;
    }
    if (gpio_request(sprd_3rdparty_gpio_tp_irq, "gpio_tp_irq") < 0) {
        printk("msg21xx gpio tp_irq request err\n");
        goto err_request_tpirq;
    }
/*  
    tp_irq = sprd_alloc_gpio_irq(sprd_3rdparty_gpio_tp_irq);
    if (tp_irq < 0) {
        printk("sprd_alloc_gpio_irq err\n");
        goto err_alloc_gpio_irq;
    }
*/
    gpio_direction_output(sprd_3rdparty_gpio_tp_rst, 1);
    gpio_direction_input(sprd_3rdparty_gpio_tp_irq);
    
    //tp_irq;
     return 0;
//err_alloc_gpio_irq:
//  gpio_free(sprd_3rdparty_gpio_tp_irq);
err_request_tpirq:
//    gpio_free(sprd_3rdparty_gpio_tp_irq);
//err_request_tprst:
    gpio_free(sprd_3rdparty_gpio_tp_rst);
    return -2;
}
/* [BUGFIX]-Mod END by fangyou.wang 2014/02/18,for bug602310*/

static void tp_unconfig_pins(int tp_irq)
{
    gpio_free(sprd_3rdparty_gpio_tp_rst);
    gpio_free(sprd_3rdparty_gpio_tp_irq);
    if (tp_irq)
        gpio_free(tp_irq);
}

static int tp_msg21_parse_dt(struct device *dev,
                struct msg21xx_ts_platform_data *pdata)
{
    struct device_node *np = dev->of_node;
     u32 temp_val;
    int rc;
    pdata->i2c_pull_up = of_property_read_bool(np,
            "touchscreen,i2c-pull-up");

    rc = of_property_read_u32(np, "touchscreen,x_max", &temp_val);
    if (rc && (rc != -EINVAL)) {
        dev_err(dev, "Unable to read panel X dimension\n");
        return rc;
    } else {
        pdata->x_max= temp_val;
    }

    rc = of_property_read_u32(np, "touchscreen,y_max", &temp_val);
    if (rc && (rc != -EINVAL)) {
        dev_err(dev, "Unable to read panel Y dimension\n");
        return rc;
    } else {
        pdata->y_max = temp_val;
    }

	/* reset, irq gpio info */
	pdata->reset_gpio = of_get_named_gpio_flags(np,
			"touchscreen,reset-gpio", 0, &pdata->reset_flags);
	pdata->irq_gpio = of_get_named_gpio_flags(np,
			"touchscreen,irq-gpio", 0, &pdata->irq_flags);
  
    return 0;
}

/*[BUGFIX]-ADD BEGIN by TCTSZ.WH,4/1/2014,PR-602310,Change I2C speed when upgrading fw*/
static int fw_upgrade_flag = 0;
int msg21xx_fw_upgrade(void)
{
	return fw_upgrade_flag;
}

extern int bus_clk_rate_bak;
extern void ito_test_set_iic_rate(u32 iicRate);
/*[BUGFIX]-ADD BEGIN by TCTSZ.WH,4/1/2014,PR-602310*/
extern void ito_test_create_entry(void);	/*[BUGFIX]-ADD by TCTSZ.WH,03/31/2014,PR-602310*/
static int __devinit msg21xx_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int  err = 0;
    struct msg21xx_ts_platform_data *pdata = client->dev.platform_data;
    struct msg21xx_ts_data *data;
    int time = 0;

/*[BUGFIX]-ADD BEGIN by TCTSZ.WH,03/18/2014,PR-602310*/
#ifdef MSG21XX_AUTO_UPDATE		/*[BUGFIX]-ADD by TCTSZ.WH,03/31/2014,PR-602310*/
    struct fw_version fw_new = {0,0,0};
#endif
    struct fw_version fw_old = {0,0,0};
    struct fw_version *fw_ver;
/*[BUGFIX]-ADD END by TCTSZ.WH,03/18/2014,PR-602310*/

    if (client->dev.of_node) {
        pdata = devm_kzalloc(&client->dev,
            sizeof(*pdata),
            GFP_KERNEL);
        if (!pdata) {
            dev_err(&client->dev, "Failed to allocate memory\n");
            return -ENOMEM;
        }
        
	err = tp_msg21_parse_dt(&client->dev, pdata);
        if (err)
            return err;
    } else {
        pdata = client->dev.platform_data;
    }
      data = kzalloc(sizeof(struct msg21xx_ts_data), GFP_KERNEL);
    if (!data) {
        dev_err(&client->dev, "Not enough memory\n");
        return -ENOMEM;
    }
    data->input_dev = input;
    data->client = client;
    data->pdata = pdata;

    tp_power_init(data,true);
      
    tp_pwron(data);

    sprd_3rdparty_gpio_tp_rst=pdata->reset_gpio;
    sprd_3rdparty_gpio_tp_irq =pdata->irq_gpio;
/* [BUGFIX]-Mod Begin by fangyou.wang 2014/02/18,for bug602310,used for rio4g 2nd tp rest pin err*/
    if(tp_config_pins())
     {
        printk("%s: msg_ts_config_pins failed\n",__func__);
        err = -ENOMEM;
        goto exit_config_pins_failed;
    }
/* [BUGFIX]-Mod END by fangyou.wang 2014/02/18,for bug602310*/
    msg21xx_irq = client->irq;
    if(msg21xx_irq < 0)
    {
        printk("%s: msg_ts_config_pins failed\n",__func__);
        err = -ENOMEM;
        goto exit_config_pins_failed;   
    }
    ctp_x_max=pdata->x_max;
    ctp_y_max=pdata->y_max;
    tp_reset();

    msg21xx_i2c_client = client;

     if(msg2133_i2c_test(client) < 0)
    {
        printk("msg2133 corressponding failed\n");
        goto msg2133_i2c_read_failed;
    }

/*[BUGFIX]-ADD BEGIN by TCTSZ.WH,03/18/2014,PR-602310*/
//Get Chip Version
    fw_ver = &fw_old;
    while(time < 10)
    {
    	get_chip_version(fw_ver);
	time ++;
	if((fw_old.major != 0) || (fw_old.major != 0))
		break;
	msleep(50);
    }
    printk("time = %d, fw_old.version = [0x%x].[0x%x]\n", time, fw_old.major,fw_old.minor);

#ifdef MSG21XX_AUTO_UPDATE		/*[BUGFIX]-ADD by TCTSZ.WH,03/31/2014,PR-602310*/
    fw_new.major = GroupData[0x7f4e];
    fw_new.minor = GroupData[0x7f50];

    printk("%s, fw_new.version = [0x%x].[0x%x]\n", __func__, fw_new.major, fw_new.minor);

/*[BUGFIX]-MOD BEGIN by TCTSZ.WH,4/1/2014,PR-602310, Change I2C speed to 100kHz when upgrading fw*/
    if((fw_old.major < fw_new.major) || ((fw_old.major == fw_new.major) && (fw_old.minor < fw_new.minor)))
    {
    	fw_upgrade_flag = 1;
	ito_test_set_iic_rate(100000);
	firmware_data_store(GroupData,sizeof(GroupData));	/*[BUGFIX]-MOD by TCTSZ.WH,03/31/2014,PR-602310*/
	firmware_update_store( sizeof(GroupData));	/*[BUGFIX]-MOD by TCTSZ.WH,03/31/2014,PR-602310*/
	msleep(300);
	tp_reset();
	ito_test_set_iic_rate(bus_clk_rate_bak);	//250000
	fw_upgrade_flag = 0;
    }
/*[BUGFIX]-MOD BEGIN by TCTSZ.WH,4/1/2014,PR-602310*/
#endif

    data->suspended = 0;
/*[BUGFIX]-ADD END by TCTSZ.WH,03/18/2014,PR-602310*/
    
    INIT_WORK(&msg21xx_wq, msg21xx_data_disposal);
    err = msg21xx_init_input();
    if(err < 0)
    {
        printk("msg2133 input init failed\n");
        goto msg2133_input_init_failed;
    }

    err=msg21xx_virtual_keys_init();

#ifdef TP_DEVICE_INFO
    tp_proc_file = create_proc_entry("tp_info", 0666, NULL);
    if(tp_proc_file != NULL){
        tp_proc_file->read_proc = tp_info_read_proc_t;
        tp_proc_file->write_proc = tp_info_write_proc_t;
    }else {
        printk("tp device info proc file created failed..............\n");
        goto tp_info_proc_file_failed;
    }
#endif


    err = request_irq(msg21xx_irq, msg21xx_interrupt,
                            IRQF_TRIGGER_RISING, "msg21xx", NULL);
    if (err != 0) {
        printk("%s: cannot register irq\n", __func__);
        goto msg21xx_irq_request_failed;
    }

/*[BUGFIX]-ADD BEGIN by TCTSZ.WH,03/18/2014,PR-602310*/
#if defined(CONFIG_FB)
        data->fb_notif.notifier_call = fb_notifier_callback;

        err = fb_register_client(&data->fb_notif);

        if (err)
                dev_err(&client->dev, "Unable to register fb_notifier: %d\n",
                        err);
#elif defined(CONFIG_HAS_EARLYSUSPEND)   
    data->early_suspend.level = EARLY_SUSPEND_LEVEL_STOP_DRAWING;
    data->early_suspend.suspend = msg21xx_early_suspend;
    data->early_suspend.resume = msg21xx_early_resume;
    register_early_suspend(&data->early_suspend);
#endif
/*[BUGFIX]-ADD END by TCTSZ.WH,03/18/2014,PR-602310*/

    /*********  frameware upgrade *********/
 #ifdef MSG21XX_APK_UPDATE		/*[BUGFIX]-MOD by TCTSZ.WH,03/31/2014,PR-602310*/
    firmware_class = class_create(THIS_MODULE, "ms-touchscreen-msg20xx");
    if (IS_ERR(firmware_class))
        pr_err("Failed to create class(firmware)!\n");
    firmware_cmd_dev = device_create(firmware_class,
                                     NULL, 0, NULL, "device");
    if (IS_ERR(firmware_cmd_dev))
        pr_err("Failed to create device(firmware_cmd_dev)!\n");

    // version
    if (device_create_file(firmware_cmd_dev, &dev_attr_version) < 0)
        pr_err("Failed to create device file(%s)!\n", dev_attr_version.attr.name);
    // update
    if (device_create_file(firmware_cmd_dev, &dev_attr_update) < 0)
        pr_err("Failed to create device file(%s)!\n", dev_attr_update.attr.name);
    // data
    if (device_create_file(firmware_cmd_dev, &dev_attr_data) < 0)
        pr_err("Failed to create device file(%s)!\n", dev_attr_data.attr.name);
    // clear
 //   if (device_create_file(firmware_cmd_dev, &dev_attr_clear) < 0)
 //       pr_err("Failed to create device file(%s)!\n", dev_attr_clear.attr.name);

    dev_set_drvdata(firmware_cmd_dev, NULL);
#endif
    input_set_capability(input, EV_KEY, KEY_POWER);	/*[BUGFIX]-ADD by TCTSZ.WH,03/31/2014,PR-602310*/

    ito_test_create_entry();	/*[BUGFIX]-ADD by TCTSZ.WH,03/31/2014,PR-602310*/
    printk("[TP] msg2331 probe OK!\n");

   
    return 0;

msg21xx_irq_request_failed:
#ifdef TP_DEVICE_INFO
    remove_proc_entry("tp_info", NULL);
tp_info_proc_file_failed:
#endif
    input_unregister_device(input);
    input_free_device(input);
msg2133_input_init_failed:
    cancel_work_sync(&msg21xx_wq);
msg2133_i2c_read_failed:
    msg21xx_i2c_client = NULL;
    tp_unconfig_pins(msg21xx_irq);
exit_config_pins_failed:
    tp_pwroff(data);
    return err;
}

static int __devexit msg21xx_remove(struct i2c_client *client)
{
    struct msg21xx_ts_data *data = i2c_get_clientdata(msg21xx_i2c_client);
/*[BUGFIX]-ADD BEGIN by TCTSZ.WH,03/18/2014,PR-602310*/
#if defined(CONFIG_FB)
    if (fb_unregister_client(&data->fb_notif))
           dev_err(&client->dev, "Error occurred while unregistering fb_notifier.\n");
#elif defined(CONFIG_HAS_EARLYSUSPEND)
    unregister_early_suspend(&data->early_suspend);
#endif
/*[BUGFIX]-ADD END by TCTSZ.WH,03/18/2014,PR-602310*/

#ifdef TP_DEVICE_INFO
    remove_proc_entry("tp_info", NULL);
    if(tp_info != NULL)
        kfree(tp_info);
#endif
    input_unregister_device(input);
    input_free_device(input);
    cancel_work_sync(&msg21xx_wq);
    msg21xx_i2c_client = NULL;
    tp_unconfig_pins(msg21xx_irq);
    tp_pwroff(data);

    return 0;
}
/*[BUGFIX]-BGEIN by TCTNB.XQJ,08/12/2013,FR-487707,msg21 tp development,add for new ARCH.*/
#ifdef CONFIG_OF
static struct of_device_id msg21_match_table[] = {
	{ .compatible = "touchscreen,msg21xxx",},
	{ },
};
#else
#define msg21_match_table NULL
#endif
/*[BUGFIX]-END by TCTNB.XQJ*/
static const struct i2c_device_id msg21xx_id[] = {
    { "ms-msg21xx", 0x26},
    { }
};
MODULE_DEVICE_TABLE(i2c, msg21xx_id);

static struct i2c_driver msg21xx_driver = {
    .driver = {
           .name = "ms-msg21xx",
           .owner = THIS_MODULE,
           .of_match_table = msg21_match_table,//[BUGFIX]-BGEIN by TCTNB.XQJ,08/12/2013,FR-487707,msg21 tp development,add for new ARCH.
    },
    .probe = msg21xx_probe,
    .remove = __devexit_p(msg21xx_remove),
    .id_table = msg21xx_id,
};

#if 0
static int sprd_add_i2c_device(struct sprd_i2c_setup_data *i2c_set_data, struct i2c_driver *driver)
{
    struct i2c_board_info info;
    struct i2c_adapter *adapter;
    struct i2c_client *client;
    int err;

    printk("%s : i2c_bus=%d; slave_address=0x%x; i2c_name=%s",__func__,i2c_set_data->i2c_bus, \
            i2c_set_data->i2c_address, i2c_set_data->type);

    memset(&info, 0, sizeof(struct i2c_board_info));
    info.addr = i2c_set_data->i2c_address;
    strlcpy(info.type, i2c_set_data->type, I2C_NAME_SIZE);

    adapter = i2c_get_adapter( i2c_set_data->i2c_bus);
    if (!adapter) {
        printk("%s: can't get i2c adapter %d\n",
            __func__,  i2c_set_data->i2c_bus);
        err = -ENODEV;
        goto err_driver;
    }

    client = i2c_new_device(adapter, &info);
    if (!client) {
        printk("%s:  can't add i2c device at 0x%x\n",
            __func__, (unsigned int)info.addr);
        err = -ENODEV;
        goto err_driver;
    }

    i2c_put_adapter(adapter);

    err = i2c_add_driver(driver);
    if (err != 0) {
        printk("%s: can't add i2c driver\n", __func__);
        err = -ENODEV;
        goto err_driver;
    }   

    return 0;

err_driver:
    return err;
}
#endif
void sprd_del_i2c_device(struct i2c_client *client, struct i2c_driver *driver)
{
    printk("%s : slave_address=0x%x; i2c_name=%s",__func__, client->addr, client->name);
    i2c_unregister_device(client);
    i2c_del_driver(driver);
}

static int __init msg21xx_init(void)
{
    printk("%s\n", __func__);
    /*if(get_ctp_para() < 0)
    {
        printk("%s: get_ctp_para err\n", __func__);
        return -1;
    }*/
    return i2c_add_driver(&msg21xx_driver); //sprd_add_i2c_device(&msg_ts_setup, &msg21xx_driver);//
}

static void __exit msg21xx_exit(void)
{
    sprd_del_i2c_device(msg21xx_i2c_client, &msg21xx_driver);
}

module_init(msg21xx_init);
module_exit(msg21xx_exit);

MODULE_AUTHOR("Mstar semiconductor");
MODULE_DESCRIPTION("Driver for msg21xx Touchscreen Controller");
MODULE_LICENSE("GPL");
