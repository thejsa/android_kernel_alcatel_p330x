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
#include <mach/board.h>
#include <linux/proc_fs.h>


#define ITO_TEST
#ifdef ITO_TEST

#include <open_test_ANA1_B_yeji.h>
#include <open_test_ANA1_yeji.h>
#include <open_test_ANA2_B_yeji.h>
#include <open_test_ANA2_yeji.h>
#include <open_test_ANA3_yeji.h>


static char const * const i2c_rsrcs[] = {"i2c_clk", "i2c_sda"};
struct qup_i2c_clk_path_vote {
	u32                         client_hdl;
	struct msm_bus_scale_pdata *pdata;
	bool                        reg_err;
};
struct qup_i2c_dev {
	struct device                *dev;
	void __iomem                 *base;		/* virtual */
	void __iomem                 *gsbi;		/* virtual */
	int                          in_irq;
	int                          out_irq;
	int                          err_irq;
	int                          num_irqs;
	struct clk                   *clk;
	struct clk                   *pclk;
	struct i2c_adapter           adapter;

	struct i2c_msg               *msg;
	int                          pos;
	int                          cnt;
	int                          err;
	int                          mode;
	int                          clk_ctl;
	int                          one_bit_t;
	int                          out_fifo_sz;
	int                          in_fifo_sz;
	int                          out_blk_sz;
	int                          in_blk_sz;
	int                          wr_sz;
	struct msm_i2c_platform_data *pdata;
	int                          suspended;
	int                          pwr_state;
	struct mutex                 mlock;
	void                         *complete;
	int                          i2c_gpios[ARRAY_SIZE(i2c_rsrcs)];
	struct qup_i2c_clk_path_vote clk_path_vote;
};


///////////////////////////////////////////////////////////////////////////
u8 bItoTestDebug = 0;
#define ITO_TEST_DEBUG(format, ...) \
{ \
    if(bItoTestDebug) \
    { \
        printk(KERN_INFO "ito_test ***" format "\n", ## __VA_ARGS__); \
        mdelay(5); \
    } \
}
#define ITO_TEST_DEBUG_MUST(format, ...)	printk(KERN_ERR "ito_test ***" format "\n", ## __VA_ARGS__);mdelay(5)

struct qup_i2c_dev *msg_i2c_dev ;
int bus_clk_rate_bak = 0;

s16  s16_raw_data_1[48] = {0};
s16  s16_raw_data_2[48] = {0};
s16  s16_raw_data_3[48] = {0};
u8 ito_test_keynum = 0;
u8 ito_test_dummynum = 0;
u8 ito_test_trianglenum = 0;
u8 ito_test_2r = 0;
u8 g_LTP = 1;	
uint16_t *open_1 = NULL;
uint16_t *open_1B = NULL;
uint16_t *open_2 = NULL;
uint16_t *open_2B = NULL;
uint16_t *open_3 = NULL;
u8 *MAP1 = NULL;
u8 *MAP2=NULL;
u8 *MAP3=NULL;
u8 *MAP40_1 = NULL;
u8 *MAP40_2 = NULL;
u8 *MAP40_3 = NULL;
u8 *MAP40_4 = NULL;
u8 *MAP41_1 = NULL;
u8 *MAP41_2 = NULL;
u8 *MAP41_3 = NULL;
u8 *MAP41_4 = NULL;


#define ITO_TEST_ADDR_TP  (0x4C>>1)
#define ITO_TEST_ADDR_REG (0xC4>>1)
#define REG_INTR_FIQ_MASK           0x04
#define FIQ_E_FRAME_READY_MASK      ( 1 << 8 )
#define MAX_CHNL_NUM (48)
#define BIT0  (1<<0)
#define BIT1  (1<<1)
#define BIT5  (1<<5)
#define BIT11 (1<<11)
#define BIT15 (1<<15)

extern struct i2c_client *msg21xx_i2c_client;
extern int msg21xx_irq;
extern unsigned int sprd_3rdparty_gpio_tp_rst;
extern unsigned int sprd_3rdparty_gpio_tp_irq;

static int ito_test_i2c_read(u8 addr, u8* read_data, u16 size)//modify : 根据项目修改 msg21xx_i2c_client
{
    int rc;
    u8 addr_before = msg21xx_i2c_client->addr;
    msg21xx_i2c_client->addr = addr;

    rc = i2c_master_recv(msg21xx_i2c_client, read_data, size);

    msg21xx_i2c_client->addr = addr_before;
    if( rc < 0 )
    {
        ITO_TEST_DEBUG_MUST("ito_test_i2c_read error %d,addr=%d\n", rc,addr);
    }
    return rc;
}

static int ito_test_i2c_write(u8 addr, u8* data, u16 size)//modify : 根据项目修改 msg21xx_i2c_client
{
    int rc;
    u8 addr_before = msg21xx_i2c_client->addr;
    msg21xx_i2c_client->addr = addr;

    rc = i2c_master_send(msg21xx_i2c_client, data, size);

    msg21xx_i2c_client->addr = addr_before;
    if( rc < 0 )
    {
        ITO_TEST_DEBUG_MUST("ito_test_i2c_write error %d,addr = %d,data[0]=%d\n", rc, addr,data[0]);
    }
    return rc;
}

static void ito_test_reset(void)//modify:根据项目修改
{
	gpio_direction_output(sprd_3rdparty_gpio_tp_rst, 1);
	gpio_set_value(sprd_3rdparty_gpio_tp_rst, 1);
	gpio_set_value(sprd_3rdparty_gpio_tp_rst, 0);
	mdelay(100);  
    ITO_TEST_DEBUG("reset tp\n");
	gpio_set_value(sprd_3rdparty_gpio_tp_rst, 1);
	mdelay(200);
}
static void ito_test_disable_irq(void)//modify:根据项目修改
{
	disable_irq_nosync(msg21xx_irq);
}
static void ito_test_enable_irq(void)//modify:根据项目修改
{
	enable_irq(msg21xx_irq);
}

void ito_test_set_iic_rate(u32 iicRate)//modify:根据平台修改,iic速率要求50K
{
	if (0 == bus_clk_rate_bak)
		{
			msg_i2c_dev = i2c_get_adapdata(msg21xx_i2c_client->adapter);
			bus_clk_rate_bak = msg_i2c_dev->pdata->clk_freq;
		}

		msg_i2c_dev->pdata->clk_freq = iicRate;//30000;

/*[BUGFIX]-ADD-END by TCTSZ.WH,2014/03/28,Change the i2c clock to read rawdata*/

	printk("clk = %d\n",msg_i2c_dev->pdata->clk_freq);

}

static void ito_test_WriteReg( u8 bank, u8 addr, u16 data )
{
    u8 tx_data[5] = {0x10, bank, addr, data & 0xFF, data >> 8};
    ito_test_i2c_write( ITO_TEST_ADDR_REG, &tx_data[0], 5 );
}
static void ito_test_WriteReg8Bit( u8 bank, u8 addr, u8 data )
{
    u8 tx_data[4] = {0x10, bank, addr, data};
    ito_test_i2c_write ( ITO_TEST_ADDR_REG, &tx_data[0], 4 );
}
static unsigned short ito_test_ReadReg( u8 bank, u8 addr )
{
    u8 tx_data[3] = {0x10, bank, addr};
    u8 rx_data[2] = {0};

    ito_test_i2c_write( ITO_TEST_ADDR_REG, &tx_data[0], 3 );
    ito_test_i2c_read ( ITO_TEST_ADDR_REG, &rx_data[0], 2 );
    return ( rx_data[1] << 8 | rx_data[0] );
}
static u32 ito_test_get_TpType(void)
{
    u8 tx_data[3] = {0};
    u8 rx_data[4] = {0};
    u32 Major = 0, Minor = 0;

    ITO_TEST_DEBUG("GetTpType\n");
        
    tx_data[0] = 0x53;
    tx_data[1] = 0x00;
    tx_data[2] = 0x2A;
    ito_test_i2c_write(ITO_TEST_ADDR_TP, &tx_data[0], 3);
    mdelay(50);
    ito_test_i2c_read(ITO_TEST_ADDR_TP, &rx_data[0], 4);
    Major = (rx_data[1]<<8) + rx_data[0];
    Minor = (rx_data[3]<<8) + rx_data[2];

    ITO_TEST_DEBUG("***TpTypeMajor = %d ***\n", Major);
    ITO_TEST_DEBUG("***TpTypeMinor = %d ***\n", Minor);
    
    return Major;
    
}

//modify:注意该项目tp数目
#define TP_OF_LIANCHUANG    (2)
#define TP_OF_JUNDA         (4)
#define TP_OF_YEJI         (5)
static u32 ito_test_choose_TpType(void)
{
    u32 tpType = 0;
    u8 i = 0;
    open_1 = NULL;
    open_1B = NULL;
    open_2 = NULL;
    open_2B = NULL;
    open_3 = NULL;
    MAP1 = NULL;
    MAP2 = NULL;
    MAP3 = NULL;
    MAP40_1 = NULL;
    MAP40_2 = NULL;
    MAP40_3 = NULL;
    MAP40_4 = NULL;
    MAP41_1 = NULL;
    MAP41_2 = NULL;
    MAP41_3 = NULL;
    MAP41_4 = NULL;
    ito_test_keynum = 0;
    ito_test_dummynum = 0;
    ito_test_trianglenum = 0;
    ito_test_2r = 0;

    for(i=0;i<10;i++)
    {
        tpType = ito_test_get_TpType();
        ITO_TEST_DEBUG("tpType=%d;i=%d;\n",tpType,i);
        if(TP_OF_YEJI==tpType)//modify:注意该项目tp数目
        {
            break;
        }
        else if(i<5)
        {
            mdelay(100);  
        }
        else
        {
            ito_test_reset();
        }
    }
if(TP_OF_YEJI==tpType)
    {
 		printk("----------tpType = 5\n\n");
        open_1 = open_1_yeji;
        open_1B = open_1B_yeji;
        open_2 = open_2_yeji;
        open_2B = open_2B_yeji;
        open_3 = open_3_yeji;
        MAP1 = MAP1_yeji;
        MAP2 = MAP2_yeji;
        MAP3 = MAP3_yeji;
        MAP40_1 = MAP40_1_yeji;
        MAP40_2 = MAP40_2_yeji;
        MAP40_3 = MAP40_3_yeji;
        MAP40_4 = MAP40_4_yeji;
        MAP41_1 = MAP41_1_yeji;
        MAP41_2 = MAP41_2_yeji;
        MAP41_3 = MAP41_3_yeji;
        MAP41_4 = MAP41_4_yeji;
        ito_test_keynum = NUM_KEY_YEJI;
        ito_test_dummynum = NUM_DUMMY_YEJI;
        ito_test_trianglenum = NUM_SENSOR_YEJI;
        ito_test_2r = ENABLE_2R_YEJI;
    }
    else
    {
        tpType = 0;
    }
    return tpType;
}
static void ito_test_EnterSerialDebugMode(void)
{
    u8 data[5];

    data[0] = 0x53;
    data[1] = 0x45;
    data[2] = 0x52;
    data[3] = 0x44;
    data[4] = 0x42;
    ito_test_i2c_write(ITO_TEST_ADDR_REG, &data[0], 5);

    data[0] = 0x37;
    ito_test_i2c_write(ITO_TEST_ADDR_REG, &data[0], 1);

    data[0] = 0x35;
    ito_test_i2c_write(ITO_TEST_ADDR_REG, &data[0], 1);

    data[0] = 0x71;
    ito_test_i2c_write(ITO_TEST_ADDR_REG, &data[0], 1);
}
static uint16_t ito_test_get_num( void )
{
    uint16_t    num_of_sensor,i;
    uint16_t 	RegValue1,RegValue2;
 
    num_of_sensor = 0;
        
    RegValue1 = ito_test_ReadReg( 0x11, 0x4A);
    ITO_TEST_DEBUG("ito_test_get_num,RegValue1=%d\n",RegValue1);
    if ( ( RegValue1 & BIT1) == BIT1 )
    {
    	RegValue1 = ito_test_ReadReg( 0x12, 0x0A);			
    	RegValue1 = RegValue1 & 0x0F;
    	
    	RegValue2 = ito_test_ReadReg( 0x12, 0x16);    		
    	RegValue2 = (( RegValue2 >> 1 ) & 0x0F) + 1;
    	
    	num_of_sensor = RegValue1 * RegValue2;
    }
	else
	{
	    for(i=0;i<4;i++)
	    {
	        num_of_sensor+=(ito_test_ReadReg( 0x12, 0x0A)>>(4*i))&0x0F;
	    }
	}
    ITO_TEST_DEBUG("ito_test_get_num,num_of_sensor=%d\n",num_of_sensor);
    return num_of_sensor;        
}
static void ito_test_polling( void )
{
    uint16_t    reg_int = 0x0000;
    uint8_t     dbbus_tx_data[5];
    uint8_t     dbbus_rx_data[4];
    uint16_t    reg_value;


    reg_int = 0;

    ito_test_WriteReg( 0x13, 0x0C, BIT15 );       
    ito_test_WriteReg( 0x12, 0x14, (ito_test_ReadReg(0x12,0x14) | BIT0) );         
            
    ITO_TEST_DEBUG("polling start\n");
    while( ( reg_int & BIT0 ) == 0x0000 )
    {
        dbbus_tx_data[0] = 0x10;
        dbbus_tx_data[1] = 0x3D;
        dbbus_tx_data[2] = 0x18;
        ito_test_i2c_write(ITO_TEST_ADDR_REG, dbbus_tx_data, 3);
        ito_test_i2c_read(ITO_TEST_ADDR_REG,  dbbus_rx_data, 2);
        reg_int = dbbus_rx_data[1];
    }
    ITO_TEST_DEBUG("polling end\n");
    reg_value = ito_test_ReadReg( 0x3D, 0x18 ); 
    ito_test_WriteReg( 0x3D, 0x18, reg_value & (~BIT0) );      
}
static uint16_t ito_test_get_data_out( int16_t* s16_raw_data )
{
    uint8_t     i,dbbus_tx_data[8];
    uint16_t    raw_data[48]={0};
    uint16_t    num_of_sensor;
    uint16_t    reg_int;
    uint8_t		dbbus_rx_data[96]={0};
  
    num_of_sensor = ito_test_get_num();
    if(num_of_sensor*2>96)
    {
        ITO_TEST_DEBUG("danger,num_of_sensor=%d\n",num_of_sensor);
        return num_of_sensor;
    }

    reg_int = ito_test_ReadReg( 0x3d, REG_INTR_FIQ_MASK<<1 ); 
    ito_test_WriteReg( 0x3d, REG_INTR_FIQ_MASK<<1, (reg_int & (uint16_t)(~FIQ_E_FRAME_READY_MASK) ) ); 
    ito_test_polling();
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x13;
    dbbus_tx_data[2] = 0x40;
    ito_test_i2c_write(ITO_TEST_ADDR_REG, dbbus_tx_data, 3);
    mdelay(20);
    ito_test_i2c_read(ITO_TEST_ADDR_REG, &dbbus_rx_data[0], (num_of_sensor * 2));
    mdelay(100);
    for(i=0;i<num_of_sensor * 2;i++)
    {
        ITO_TEST_DEBUG("dbbus_rx_data[%d]=%d\n",i,dbbus_rx_data[i]);
    }
 
    reg_int = ito_test_ReadReg( 0x3d, REG_INTR_FIQ_MASK<<1 ); 
    ito_test_WriteReg( 0x3d, REG_INTR_FIQ_MASK<<1, (reg_int | (uint16_t)FIQ_E_FRAME_READY_MASK ) ); 


    for( i = 0; i < num_of_sensor; i++ )
    {
        raw_data[i] = ( dbbus_rx_data[ 2 * i + 1] << 8 ) | ( dbbus_rx_data[2 * i] );
        s16_raw_data[i] = ( int16_t )raw_data[i];
    }
    
    return(num_of_sensor);
}


static void ito_test_send_data_in( uint8_t step )
{
    uint16_t	i;
    uint8_t 	dbbus_tx_data[512];
    uint16_t 	*Type1=NULL;        

    ITO_TEST_DEBUG("ito_test_send_data_in step=%d\n",step);
	if( step == 4 )
    {
        Type1 = &open_1[0];        
    }
    else if( step == 5 )
    {
        Type1 = &open_2[0];      	
    }
    else if( step == 6 )
    {
        Type1 = &open_3[0];      	
    }
    else if( step == 9 )
    {
        Type1 = &open_1B[0];        
    }
    else if( step == 10 )
    {
        Type1 = &open_2B[0];      	
    } 
     
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x11;
    dbbus_tx_data[2] = 0x00;    
    for( i = 0; i <= 0x3E ; i++ )
    {
        dbbus_tx_data[3+2*i] = Type1[i] & 0xFF;
        dbbus_tx_data[4+2*i] = ( Type1[i] >> 8 ) & 0xFF;    	
    }
    ito_test_i2c_write(ITO_TEST_ADDR_REG, dbbus_tx_data, 3+0x3F*2);
 
    dbbus_tx_data[2] = 0x7A * 2;
    for( i = 0x7A; i <= 0x7D ; i++ )
    {
        dbbus_tx_data[3+2*(i-0x7A)] = 0;
        dbbus_tx_data[4+2*(i-0x7A)] = 0;    	    	
    }
    ito_test_i2c_write(ITO_TEST_ADDR_REG, dbbus_tx_data, 3+8);  
    
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x12;
      
    dbbus_tx_data[2] = 5 * 2;
    dbbus_tx_data[3] = Type1[128+5] & 0xFF;
    dbbus_tx_data[4] = ( Type1[128+5] >> 8 ) & 0xFF;
    ito_test_i2c_write(ITO_TEST_ADDR_REG, dbbus_tx_data, 5);
    
    dbbus_tx_data[2] = 0x0B * 2;
    dbbus_tx_data[3] = Type1[128+0x0B] & 0xFF;
    dbbus_tx_data[4] = ( Type1[128+0x0B] >> 8 ) & 0xFF;
    ito_test_i2c_write(ITO_TEST_ADDR_REG, dbbus_tx_data, 5);
    
    dbbus_tx_data[2] = 0x12 * 2;
    dbbus_tx_data[3] = Type1[128+0x12] & 0xFF;
    dbbus_tx_data[4] = ( Type1[128+0x12] >> 8 ) & 0xFF;
    ito_test_i2c_write(ITO_TEST_ADDR_REG, dbbus_tx_data, 5);
    
    dbbus_tx_data[2] = 0x15 * 2;
    dbbus_tx_data[3] = Type1[128+0x15] & 0xFF;
    dbbus_tx_data[4] = ( Type1[128+0x15] >> 8 ) & 0xFF;
    ito_test_i2c_write(ITO_TEST_ADDR_REG, dbbus_tx_data, 5);        
}

static void ito_test_set_v( uint8_t Enable, uint8_t Prs)	
{
    uint16_t    u16RegValue;        
    
    
    u16RegValue = ito_test_ReadReg( 0x12, 0x08);   
    u16RegValue = u16RegValue & 0xF1; 							
    if ( Prs == 0 )
    {
    	ito_test_WriteReg( 0x12, 0x08, u16RegValue| 0x0C); 		
    }
    else if ( Prs == 1 )
    {
    	ito_test_WriteReg( 0x12, 0x08, u16RegValue| 0x0E); 		     	
    }
    else
    {
    	ito_test_WriteReg( 0x12, 0x08, u16RegValue| 0x02); 			
    }    
    
    if ( Enable )
    {
        u16RegValue = ito_test_ReadReg( 0x11, 0x06);    
        ito_test_WriteReg( 0x11, 0x06, u16RegValue| 0x03);   	
    }
    else
    {
        u16RegValue = ito_test_ReadReg( 0x11, 0x06);    
        u16RegValue = u16RegValue & 0xFC;					
        ito_test_WriteReg( 0x11, 0x06, u16RegValue);         
    }

}

static void ito_test_set_c( uint8_t Csub_Step )
{
    uint8_t i;
    uint8_t dbbus_tx_data[MAX_CHNL_NUM+3];
    uint8_t HighLevel_Csub = false;
    uint8_t Csub_new;
     
    dbbus_tx_data[0] = 0x10;
    dbbus_tx_data[1] = 0x11;        
    dbbus_tx_data[2] = 0x84;        
    for( i = 0; i < MAX_CHNL_NUM; i++ )
    {
		Csub_new = Csub_Step;        
        HighLevel_Csub = false;   
        if( Csub_new > 0x1F )
        {
            Csub_new = Csub_new - 0x14;
            HighLevel_Csub = true;
        }
           
        dbbus_tx_data[3+i] =    Csub_new & 0x1F;        
        if( HighLevel_Csub == true )
        {
            dbbus_tx_data[3+i] |= BIT5;
        }
    }
    ito_test_i2c_write(ITO_TEST_ADDR_REG, dbbus_tx_data, MAX_CHNL_NUM+3);

    dbbus_tx_data[2] = 0xB4;        
    ito_test_i2c_write(ITO_TEST_ADDR_REG, dbbus_tx_data, MAX_CHNL_NUM+3);
}

static void ito_test_sw( void )
{
    ito_test_WriteReg( 0x11, 0x00, 0xFFFF );
    ito_test_WriteReg( 0x11, 0x00, 0x0000 );
    mdelay( 50 );
}



static void ito_test_first( uint8_t item_id , int16_t* s16_raw_data)		
{
	uint8_t     loop;
//	uint8_t     dbbus_tx_data[9];
	uint8_t     i,j;
    int16_t     s16_raw_data_tmp[48]={0};
	uint8_t     num_of_sensor, num_of_sensor2,total_sensor;
	uint16_t	u16RegValue;
    uint8_t 	*pMapping=NULL;
    
    
	num_of_sensor = 0;
	num_of_sensor2 = 0;	
	
    ITO_TEST_DEBUG("ito_test_first item_id=%d\n",item_id);
	ito_test_WriteReg( 0x0F, 0xE6, 0x01 );

	ito_test_WriteReg( 0x1E, 0x24, 0x0500 );
	ito_test_WriteReg( 0x1E, 0x2A, 0x0000 );
	ito_test_WriteReg( 0x1E, 0xE6, 0x6E00 );
	ito_test_WriteReg( 0x1E, 0xE8, 0x0071 );
	    
    if ( item_id == 40 )    			
    {
        pMapping = &MAP1[0];
        if ( ito_test_2r )
		{
			total_sensor = ito_test_trianglenum/2; 
		}
		else
		{
		    total_sensor = ito_test_trianglenum/2 + ito_test_keynum + ito_test_dummynum;
		}
    }
    else if( item_id == 41 )    		
    {
        pMapping = &MAP2[0];
        if ( ito_test_2r )
		{
			total_sensor = ito_test_trianglenum/2; 
		}
		else
		{
		    total_sensor = ito_test_trianglenum/2 + ito_test_keynum + ito_test_dummynum;
		}
    }
    else if( item_id == 42 )    		
    {
        pMapping = &MAP3[0];      
        total_sensor =  ito_test_trianglenum + ito_test_keynum+ ito_test_dummynum; 
    }
        	    
	    
	loop = 1;
	if ( item_id != 42 )
	{
	    if(total_sensor>11)
        {
            loop = 2;
        }
	}	
    ITO_TEST_DEBUG("loop=%d\n",loop);
	for ( i = 0; i < loop; i++ )
	{
		if ( i == 0 )
		{
			ito_test_send_data_in( item_id - 36 );
		}
		else
		{ 
			if ( item_id == 40 ) 
				ito_test_send_data_in( 9 );
			else 		
				ito_test_send_data_in( 10 );
		}
	
		ito_test_set_v(1,0);    
		u16RegValue = ito_test_ReadReg( 0x11, 0x0E);    			
		ito_test_WriteReg( 0x11, 0x0E, u16RegValue | BIT11 );				 		
	
		if ( g_LTP == 1 )
	    	ito_test_set_c( 32 );	    	
		else	    	
	    	ito_test_set_c( 0 );
	    
		ito_test_sw();
		
		if ( i == 0 )	 
        {      
            num_of_sensor=ito_test_get_data_out(  s16_raw_data_tmp );
            ITO_TEST_DEBUG("num_of_sensor=%d;\n",num_of_sensor);
        }
		else	
        {      
            num_of_sensor2=ito_test_get_data_out(  &s16_raw_data_tmp[num_of_sensor] );
            ITO_TEST_DEBUG("num_of_sensor=%d;num_of_sensor2=%d\n",num_of_sensor,num_of_sensor2);
        }
	}
    for ( j = 0; j < total_sensor ; j ++ )
	{
		if ( g_LTP == 1 )
			s16_raw_data[pMapping[j]] = s16_raw_data_tmp[j] + 4096;
		else
			s16_raw_data[pMapping[j]] = s16_raw_data_tmp[j];	
	}	

	return;
}

typedef enum
{
	ITO_TEST_OK = 0,
	ITO_TEST_FAIL,
	ITO_TEST_GET_TP_TYPE_ERROR,
} ITO_TEST_RET;

ITO_TEST_RET ito_test_second (u8 item_id)
{
	u8 i = 0;
    
	s32  s16_raw_data_jg_tmp1 = 0;
	s32  s16_raw_data_jg_tmp2 = 0;
	s32  jg_tmp1_avg_Th_max =0;
	s32  jg_tmp1_avg_Th_min =0;
	s32  jg_tmp2_avg_Th_max =0;
	s32  jg_tmp2_avg_Th_min =0;

	u8  Th_Tri = 25;        
	u8  Th_bor = 25;        

	if ( item_id == 40 )    			
    {
        for (i=0; i<(ito_test_trianglenum/2)-2; i++)
        {
			s16_raw_data_jg_tmp1 += s16_raw_data_1[MAP40_1[i]];
		}
		for (i=0; i<2; i++)
        {
			s16_raw_data_jg_tmp2 += s16_raw_data_1[MAP40_2[i]];
		}
    }
    else if( item_id == 41 )    		
    {
        for (i=0; i<(ito_test_trianglenum/2)-2; i++)
        {
			s16_raw_data_jg_tmp1 += s16_raw_data_2[MAP41_1[i]];
		}
		for (i=0; i<2; i++)
        {
			s16_raw_data_jg_tmp2 += s16_raw_data_2[MAP41_2[i]];
		}
    }

	    jg_tmp1_avg_Th_max = (s16_raw_data_jg_tmp1 / ((ito_test_trianglenum/2)-2)) * ( 100 + Th_Tri) / 100 ;
	    jg_tmp1_avg_Th_min = (s16_raw_data_jg_tmp1 / ((ito_test_trianglenum/2)-2)) * ( 100 - Th_Tri) / 100 ;
        jg_tmp2_avg_Th_max = (s16_raw_data_jg_tmp2 / 2) * ( 100 + Th_bor) / 100 ;
	    jg_tmp2_avg_Th_min = (s16_raw_data_jg_tmp2 / 2 ) * ( 100 - Th_bor) / 100 ;
	
        ITO_TEST_DEBUG("item_id=%d;sum1=%d;max1=%d;min1=%d;sum2=%d;max2=%d;min2=%d\n",item_id,s16_raw_data_jg_tmp1,jg_tmp1_avg_Th_max,jg_tmp1_avg_Th_min,s16_raw_data_jg_tmp2,jg_tmp2_avg_Th_max,jg_tmp2_avg_Th_min);

	if ( item_id == 40 ) 
	{
		for (i=0; i<(ito_test_trianglenum/2)-2; i++)
	    {
			if (s16_raw_data_1[MAP40_1[i]] > jg_tmp1_avg_Th_max || s16_raw_data_1[MAP40_1[i]] < jg_tmp1_avg_Th_min) 
				return ITO_TEST_FAIL;
		}
		for (i=0; i<(ito_test_trianglenum/2)-3; i++)//modify: 注意sensor次序
        {
            if (s16_raw_data_1[MAP40_1[i]] > s16_raw_data_1[MAP40_1[i+1]] ) 
                return ITO_TEST_FAIL;
        }
		for (i=0; i<2; i++)
	    {
			if (s16_raw_data_1[MAP40_2[i]] > jg_tmp2_avg_Th_max || s16_raw_data_1[MAP40_2[i]] < jg_tmp2_avg_Th_min) 
				return ITO_TEST_FAIL;
		} 
	}

	if ( item_id == 41 ) 
	{
		for (i=0; i<(ito_test_trianglenum/2)-2; i++)
	    {
			if (s16_raw_data_2[MAP41_1[i]] > jg_tmp1_avg_Th_max || s16_raw_data_2[MAP41_1[i]] < jg_tmp1_avg_Th_min) 
				return ITO_TEST_FAIL;
		}
        for (i=0; i<(ito_test_trianglenum/2)-3; i++)//modify: 注意sensor次序
        {
            if (s16_raw_data_2[MAP41_1[i]] < s16_raw_data_2[MAP41_1[i+1]] ) 
                return ITO_TEST_FAIL;
        }

		for (i=0; i<2; i++)
	    {
			if (s16_raw_data_2[MAP41_2[i]] > jg_tmp2_avg_Th_max || s16_raw_data_2[MAP41_2[i]] < jg_tmp2_avg_Th_min) 
				return ITO_TEST_FAIL;
		} 
	}

	return ITO_TEST_OK;
	
}
ITO_TEST_RET ito_test_second_2r (u8 item_id)
{
	u8 i = 0;
    
	s32  s16_raw_data_jg_tmp1 = 0;
	s32  s16_raw_data_jg_tmp2 = 0;
	s32  s16_raw_data_jg_tmp3 = 0;
	s32  s16_raw_data_jg_tmp4 = 0;
	
	s32  jg_tmp1_avg_Th_max =0;
	s32  jg_tmp1_avg_Th_min =0;
	s32  jg_tmp2_avg_Th_max =0;
	s32  jg_tmp2_avg_Th_min =0;
	s32  jg_tmp3_avg_Th_max =0;
	s32  jg_tmp3_avg_Th_min =0;
	s32  jg_tmp4_avg_Th_max =0;
	s32  jg_tmp4_avg_Th_min =0;

	u8  Th_Tri = 25;    // non-border threshold    
	u8  Th_bor = 25;    // border threshold    

	if ( item_id == 40 )    			
    {
        for (i=0; i<(ito_test_trianglenum/4)-2; i++)
        {
			s16_raw_data_jg_tmp1 += s16_raw_data_1[MAP40_1[i]];  //first region: non-border 
		}
		for (i=0; i<2; i++)
        {
			s16_raw_data_jg_tmp2 += s16_raw_data_1[MAP40_2[i]];  //first region: border
		}

		for (i=0; i<(ito_test_trianglenum/4)-2; i++)
        {
			s16_raw_data_jg_tmp3 += s16_raw_data_1[MAP40_3[i]];  //second region: non-border
		}
		for (i=0; i<2; i++)
        {
			s16_raw_data_jg_tmp4 += s16_raw_data_1[MAP40_4[i]];  //second region: border
		}
    }



	
    else if( item_id == 41 )    		
    {
        for (i=0; i<(ito_test_trianglenum/4)-2; i++)
        {
			s16_raw_data_jg_tmp1 += s16_raw_data_2[MAP41_1[i]];  //first region: non-border
		}
		for (i=0; i<2; i++)
        {
			s16_raw_data_jg_tmp2 += s16_raw_data_2[MAP41_2[i]];  //first region: border
		}
		for (i=0; i<(ito_test_trianglenum/4)-2; i++)
        {
			s16_raw_data_jg_tmp3 += s16_raw_data_2[MAP41_3[i]];  //second region: non-border
		}
		for (i=0; i<2; i++)
        {
			s16_raw_data_jg_tmp4 += s16_raw_data_2[MAP41_4[i]];  //second region: border
		}
    }

	    jg_tmp1_avg_Th_max = (s16_raw_data_jg_tmp1 / ((ito_test_trianglenum/4)-2)) * ( 100 + Th_Tri) / 100 ;
	    jg_tmp1_avg_Th_min = (s16_raw_data_jg_tmp1 / ((ito_test_trianglenum/4)-2)) * ( 100 - Th_Tri) / 100 ;
        jg_tmp2_avg_Th_max = (s16_raw_data_jg_tmp2 / 2) * ( 100 + Th_bor) / 100 ;
	    jg_tmp2_avg_Th_min = (s16_raw_data_jg_tmp2 / 2) * ( 100 - Th_bor) / 100 ;
		jg_tmp3_avg_Th_max = (s16_raw_data_jg_tmp3 / ((ito_test_trianglenum/4)-2)) * ( 100 + Th_Tri) / 100 ;
	    jg_tmp3_avg_Th_min = (s16_raw_data_jg_tmp3 / ((ito_test_trianglenum/4)-2)) * ( 100 - Th_Tri) / 100 ;
        jg_tmp4_avg_Th_max = (s16_raw_data_jg_tmp4 / 2) * ( 100 + Th_bor) / 100 ;
	    jg_tmp4_avg_Th_min = (s16_raw_data_jg_tmp4 / 2) * ( 100 - Th_bor) / 100 ;
		
	
        ITO_TEST_DEBUG("item_id=%d;sum1=%d;max1=%d;min1=%d;sum2=%d;max2=%d;min2=%d;sum3=%d;max3=%d;min3=%d;sum4=%d;max4=%d;min4=%d;\n",item_id,s16_raw_data_jg_tmp1,jg_tmp1_avg_Th_max,jg_tmp1_avg_Th_min,s16_raw_data_jg_tmp2,jg_tmp2_avg_Th_max,jg_tmp2_avg_Th_min,s16_raw_data_jg_tmp3,jg_tmp3_avg_Th_max,jg_tmp3_avg_Th_min,s16_raw_data_jg_tmp4,jg_tmp4_avg_Th_max,jg_tmp4_avg_Th_min);




	if ( item_id == 40 ) 
	{
		for (i=0; i<(ito_test_trianglenum/4)-2; i++)
	    {
			if (s16_raw_data_1[MAP40_1[i]] > jg_tmp1_avg_Th_max || s16_raw_data_1[MAP40_1[i]] < jg_tmp1_avg_Th_min) 
				return ITO_TEST_FAIL;
		}
		
		for (i=0; i<2; i++)
	    {
			if (s16_raw_data_1[MAP40_2[i]] > jg_tmp2_avg_Th_max || s16_raw_data_1[MAP40_2[i]] < jg_tmp2_avg_Th_min) 
				return ITO_TEST_FAIL;
		} 
		
		for (i=0; i<(ito_test_trianglenum/4)-2; i++)
	    {
			if (s16_raw_data_1[MAP40_3[i]] > jg_tmp3_avg_Th_max || s16_raw_data_1[MAP40_3[i]] < jg_tmp3_avg_Th_min) 
				return ITO_TEST_FAIL;
		}
		
		for (i=0; i<2; i++)
	    {
			if (s16_raw_data_1[MAP40_4[i]] > jg_tmp4_avg_Th_max || s16_raw_data_1[MAP40_4[i]] < jg_tmp4_avg_Th_min) 
				return ITO_TEST_FAIL;
		} 
	}

	if ( item_id == 41 ) 
	{
		for (i=0; i<(ito_test_trianglenum/4)-2; i++)
	    {
			if (s16_raw_data_2[MAP41_1[i]] > jg_tmp1_avg_Th_max || s16_raw_data_2[MAP41_1[i]] < jg_tmp1_avg_Th_min) 
				return ITO_TEST_FAIL;
		}
		
		for (i=0; i<2; i++)
	    {
			if (s16_raw_data_2[MAP41_2[i]] > jg_tmp2_avg_Th_max || s16_raw_data_2[MAP41_2[i]] < jg_tmp2_avg_Th_min) 
				return ITO_TEST_FAIL;
		}
		
		for (i=0; i<(ito_test_trianglenum/4)-2; i++)
	    {
			if (s16_raw_data_2[MAP41_3[i]] > jg_tmp3_avg_Th_max || s16_raw_data_2[MAP41_3[i]] < jg_tmp3_avg_Th_min) 
				return ITO_TEST_FAIL;
		}
		
		for (i=0; i<2; i++)
	    {
			if (s16_raw_data_2[MAP41_4[i]] > jg_tmp4_avg_Th_max || s16_raw_data_2[MAP41_4[i]] < jg_tmp4_avg_Th_min) 
				return ITO_TEST_FAIL;
		} 

	}

	return ITO_TEST_OK;
	
}

int msg21xx_debug(void)
{
	return bItoTestDebug;
}

static ITO_TEST_RET ito_test_interface(void)
{
    ITO_TEST_RET ret = ITO_TEST_OK;
    uint16_t i = 0;
#ifdef DMA_IIC
    _msg_dma_alloc();
#endif
    ito_test_set_iic_rate(50000);
    ITO_TEST_DEBUG("start\n");
    ito_test_disable_irq();
	ito_test_reset();
    if(!ito_test_choose_TpType())
    {
        ITO_TEST_DEBUG("choose tpType fail\n");
        ret = ITO_TEST_GET_TP_TYPE_ERROR;
        goto ITO_TEST_END;
    }
    ito_test_EnterSerialDebugMode();
    mdelay(100);
    ITO_TEST_DEBUG("EnterSerialDebugMode\n");
    ito_test_WriteReg8Bit ( 0x0F, 0xE6, 0x01 );
    ito_test_WriteReg ( 0x3C, 0x60, 0xAA55 );
    ITO_TEST_DEBUG("stop mcu and disable watchdog V.005\n");   
    mdelay(50);
    
	for(i = 0;i < 48;i++)
	{
		s16_raw_data_1[i] = 0;
		s16_raw_data_2[i] = 0;
		s16_raw_data_3[i] = 0;
	}	
	
    ito_test_first(40, s16_raw_data_1);
    ITO_TEST_DEBUG("40 get s16_raw_data_1\n");
    if(ito_test_2r)
    {
        ret=ito_test_second_2r(40);
    }
    else
    {
        ret=ito_test_second(40);
    }
    if(ITO_TEST_FAIL==ret)
    {
        goto ITO_TEST_END;
    }
    
    ito_test_first(41, s16_raw_data_2);
    ITO_TEST_DEBUG("41 get s16_raw_data_2\n");
    if(ito_test_2r)
    {
        ret=ito_test_second_2r(41);
    }
    else
    {
        ret=ito_test_second(41);
    }
    if(ITO_TEST_FAIL==ret)
    {
        goto ITO_TEST_END;
    }
    
    ito_test_first(42, s16_raw_data_3);
    ITO_TEST_DEBUG("42 get s16_raw_data_3\n");
    
    ITO_TEST_END:
#ifdef DMA_IIC
    _msg_dma_free();
#endif
     ito_test_set_iic_rate(bus_clk_rate_bak);	//250000
	ito_test_reset();
    ito_test_enable_irq();
    ITO_TEST_DEBUG("end\n");
    return ret;
}

#define ITO_TEST_AUTHORITY 0777 
static struct proc_dir_entry *msg_ito_test = NULL;
static struct proc_dir_entry *debug = NULL;
#define PROC_MSG_ITO_TESE      "msg-ito-test"
#define PROC_ITO_TEST_DEBUG      "debug"
ITO_TEST_RET g_ito_test_ret = ITO_TEST_FAIL;

char result[30];
static int ito_test_proc_read_debug(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int cnt= 0;

    bItoTestDebug = 1;
    ITO_TEST_DEBUG_MUST("on debug bItoTestDebug = %d",bItoTestDebug);

    g_ito_test_ret = ito_test_interface();
    if(ITO_TEST_OK==g_ito_test_ret)
    {
    		strcpy(result,"ITO_TEST_OK\n");
        ITO_TEST_DEBUG_MUST("ITO_TEST_OK");
    }
    else if(ITO_TEST_FAIL==g_ito_test_ret)
    {
        ITO_TEST_DEBUG_MUST("ITO_TEST_FAIL");
		strcpy(result,"ITO_TEST_FAIL\n");
    }
    else if(ITO_TEST_GET_TP_TYPE_ERROR==g_ito_test_ret)
    {
        ITO_TEST_DEBUG_MUST("ITO_TEST_GET_TP_TYPE_ERROR");
		strcpy(result,"ITO_TEST_GET_TP_TYPE_ERROR\n");
    }
	cnt = sprintf(page,result);
	    bItoTestDebug = 0;
    ITO_TEST_DEBUG_MUST("on debug bItoTestDebug = %d",bItoTestDebug);
    *eof = 1;
    return cnt;
}

static int ito_test_proc_write_debug(struct file *file, const char *buffer, unsigned long count, void *data)
{    
    u16 i = 0;
    mdelay(5);
    ITO_TEST_DEBUG_MUST("ito_test_ret = %d",g_ito_test_ret);
    mdelay(5);
    for(i=0;i<48;i++)
    {
        ITO_TEST_DEBUG_MUST("data_1[%d]=%d;\n",i,s16_raw_data_1[i]);
    }
    mdelay(5);
    for(i=0;i<48;i++)
    {
        ITO_TEST_DEBUG_MUST("data_2[%d]=%d;\n",i,s16_raw_data_2[i]);
    }
    mdelay(5);
    for(i=0;i<48;i++)
    {
        ITO_TEST_DEBUG_MUST("data_3[%d]=%d;\n",i,s16_raw_data_3[i]);
    }
    mdelay(5);
    return count;
}

void ito_test_create_entry(void)
{
    msg_ito_test = proc_mkdir(PROC_MSG_ITO_TESE, NULL);
    debug = create_proc_entry(PROC_ITO_TEST_DEBUG, ITO_TEST_AUTHORITY, msg_ito_test);

    if (NULL==debug) 
    {
        ITO_TEST_DEBUG_MUST("create_proc_entry ITO TEST DEBUG failed\n");
    } 
    else 
    {
        debug->read_proc = ito_test_proc_read_debug;
        debug->write_proc = ito_test_proc_write_debug;
        ITO_TEST_DEBUG_MUST("create_proc_entry ITO TEST DEBUG OK\n");
    }
}
#endif

