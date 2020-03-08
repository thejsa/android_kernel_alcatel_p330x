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
 * 14/01/22      Hu Jin       Add OTP support, disable all now
 * 14/02/11      Hu Jin       update it now
 * 14/04/04      Hu Jin       dis the LENC
 * --------------------------------------------------------------------------------------
*/
#ifndef __OV8858_EOS_V4L2_TCT_OTP_H__
#define __OV8858_EOS_V4L2_TCT_OTP_H__


#define HJ_DISLENC 0
//#define OTP_DEBUG_ON	0
#define OV8858_AWB_EN	1
#define OV8858_LENC_EN	1
#define OV8858_EOS_RG_Ratio_Typical	322
#define OV8858_EOS_BG_Ratio_Typical	308

static struct msm_camera_i2c_client *ov8858_eos_g_client;

struct ov8858_eos_otp_struct {
	int module_integrator_id;
	int lens_id;
	int production_year;
	int production_month;
	int production_day;
	int rg_ratio;
	int bg_ratio;
	int light_rg;
	int light_bg;
	int lenc[110];
	int VCM_start;
	int VCM_end;
	int VCM_dir;
};

#if (OV8858_AWB_EN | OV8858_LENC_EN)
static int32_t ov8858_eos_otp_write(uint32_t addr, uint16_t data)
{
    int32_t rc=0;
	if (!ov8858_eos_g_client)
        printk("OTP: ov5648_TCT_client null\n");
    else{
#if 1
        rc = ov8858_eos_g_client->i2c_func_tbl->i2c_write(ov8858_eos_g_client, 
            									addr, data, MSM_CAMERA_I2C_BYTE_DATA);
		if(rc < 0)
            printk("OTP: write error\n");
#endif
    }
    return rc;
}

static int16_t ov8858_eos_otp_read(uint32_t addr)
{
    uint16_t data = 0;
    int32_t rc = 0;
    if (!ov8858_eos_g_client)
        printk("OTP: ov5648_TCT_client null\n");
    else{
        rc = ov8858_eos_g_client->i2c_func_tbl->i2c_read(ov8858_eos_g_client,
            									addr, &data, MSM_CAMERA_I2C_BYTE_DATA);
		if(rc < 0)
            printk("OTP: read error\n");
    }
    return data;
}
#endif
#if 0
// index: index of otp group. (1, 2, 3)
// return:  0, group index is empty
// 1, group index has invalid data
// 2, group index has valid data
static int ov8858_eos_check_otp_info(int index)
{
	int flag, i;
	int address_start = 0x7010;
	int address_end = 0x7010;
	// read otp into buffer
	ov8858_eos_otp_write(0x3d84, 0xc0); // program disable, manual mode
	//partial mode OTP write start address 
	ov8858_eos_otp_write(0x3d88, (address_start>>8));
	ov8858_eos_otp_write(0x3d89, (address_start & 0xff));
	// partial mode OTP write end address
	ov8858_eos_otp_write(0x3d8A, (address_end>>8));
	ov8858_eos_otp_write(0x3d8B, (address_end & 0xff));
	 
	ov8858_eos_otp_write(0x3d81, 0x01); // read otp
	mdelay(5);
	flag = ov8858_eos_otp_read(0x7010);
	//select group
	if(index==1)
	{
	    flag = (flag>>6) & 0x03;
	}
	else if(index==2)
	{
	    flag = (flag>>4) & 0x03;
	}
	else
	{
	    flag = (flag>>2) & 0x03;
	}
		  
	// clear otp buffer
	for (i=address_start;i<=address_end;i++) {
		ov8858_eos_otp_write(i, 0x00);
	}
	if (flag == 0x00) {
		return 0;
	}
	else if (flag & 0x02) {
		return 1;
	}
	else {
		return 2;
	}
}
// index: index of otp group. (1, 2, 3)
// return:  0, group index is empty
// 1, group index has invalid data
// 2, group index has valid data
static int ov8858_eos_check_otp_VCM(int index)
{
	int flag, i;
	int address_start = 0x7030;
	int address_end = 0x7030;
	// read otp into buffer
	ov8858_eos_otp_write(0x3d84, 0xc0); // program disable, manual mode
	//partial mode OTP write start address 
	ov8858_eos_otp_write(0x3d88, (address_start>>8));
	ov8858_eos_otp_write(0x3d89, (address_start & 0xff));
	// partial mode OTP write end address
	ov8858_eos_otp_write(0x3d8A, (address_end>>8));
	ov8858_eos_otp_write(0x3d8B, (address_end & 0xff));
	ov8858_eos_otp_write(0x3d81, 0x01);
	mdelay(5);
	//select group
	flag = ov8858_eos_otp_read(0x7030);
	if(index==1)
	{
	    flag = (flag>>6) & 0x03;
	      }
	else if(index==2)
	{
	    flag = (flag>>4) & 0x03;
	      }
	else
	{
	    flag = (flag>>2) & 0x03;
	      }
	// clear otp buffer
	for (i=address_start;i<=address_end;i++) {
		ov8858_eos_otp_write(i, 0x00);
	}
	if (flag == 0x00) {
		return 0;
	}
	else if (flag & 0x02) {
		return 1;
	}
	else {
		return 2;
	}
}

// index: index of otp group. (1, 2, 3)
// otp_ptr: pointer of ov8858_eos_otp_struct
// return: 0, 
static int ov8858_eos_read_otp_info(int index, struct ov8858_eos_otp_struct *otp_ptr)
{
	int i;
	int address_start;
	int address_end;
	// read otp into buffer
	ov8858_eos_otp_write(0x3d84, 0xc0); // program disable, manual mode

	//select group
	if(index==1)
	{
	    address_start = 0x7011;
		address_end = 0x7015;
	}
	else if(index==2)
	{
	    address_start = 0x7016;
		address_end = 0x701a;
	}
	else
	{
	    address_start = 0x701b;
		address_end = 0x701f;
	}
	//partial mode OTP write start address 
	ov8858_eos_otp_write(0x3d88, (address_start>>8));
	ov8858_eos_otp_write(0x3d89, (address_start & 0xff));
	// partial mode OTP write end address
	ov8858_eos_otp_write(0x3d8A, (address_end>>8));
	ov8858_eos_otp_write(0x3d8B, (address_end & 0xff));
	ov8858_eos_otp_write(0x3d81, 0x01); // load otp into buffer
	mdelay(5);
	(*otp_ptr).module_integrator_id = ov8858_eos_otp_read(address_start);
	(*otp_ptr).lens_id = ov8858_eos_otp_read(address_start + 1);
	(*otp_ptr).production_year = ov8858_eos_otp_read(address_start + 2);
	(*otp_ptr).production_month = ov8858_eos_otp_read(address_start + 3);
	(*otp_ptr).production_day = ov8858_eos_otp_read(address_start + 4);
#if OTP_DEBUG_ON
     printk("\n OTP: ****************************\n");
     printk(" module_integrator_id=[0x%x] lens_id=[0x%x]\n\
          rg_ratio=[%d] bg_ratio=[%d]\n", \
         (*otp_ptr).module_integrator_id, (*otp_ptr).lens_id,\
         (*otp_ptr).production_month, (*otp_ptr).production_day);
     printk(" \n****************************\n");
#endif
	// clear otp buffer
	for (i=address_start;i<=address_end;i++) {
		ov8858_eos_otp_write(i, 0x00);
	}
	return 0;
}

// index: index of otp group. (1, 2, 3)
// code: 0 start code, 1 stop code
// return:  0
static int ov8858_eos_read_otp_VCM(int index, struct ov8858_eos_otp_struct * otp_ptr)
{
	int i, temp;
	int address_start;
	int address_end;
	// read otp into buffer
	ov8858_eos_otp_write(0x3d84, 0xc0); // program disable, manual mode
	//check group
	if(index==1)
	{
	    address_start = 0x7031;
		address_end = 0x7033;
	}
	else if(index==2)
	{
	    address_start = 0x7034;
		address_end = 0x7036;
	}
	else
	{
	    address_start = 0x7037;
		address_end = 0x7039;
	}
	//partial mode OTP write start address 
	ov8858_eos_otp_write(0x3d88, (address_start>>8));
	ov8858_eos_otp_write(0x3d89, (address_start & 0xff));
	// partial mode OTP write end address
	ov8858_eos_otp_write(0x3d8A, (address_end>>8));


	ov8858_eos_otp_write(0x3d8B, (address_end & 0xff));
	ov8858_eos_otp_write(0x3d81, 0x01); // load otp into buffer
	mdelay(5);
	//flag and lsb of VCM start code
	temp = ov8858_eos_otp_read(address_start + 2);
	(* otp_ptr).VCM_start  = (ov8858_eos_otp_read(address_start)<<2) | ((temp>>6) & 0x03);
	(* otp_ptr).VCM_end  = (ov8858_eos_otp_read(address_start + 1) << 2) | ((temp>>4) & 0x03);
	(* otp_ptr).VCM_dir = (temp>>2) & 0x03;
	// clear otp buffer
	for (i=address_start;i<=address_end;i++) {
		ov8858_eos_otp_write(i, 0x00);
	}
	return 0;
}

#endif

#if OV8858_AWB_EN
// index: index of otp group. (1, 2, 3)
// return:  0, group index is empty
// 1, group index has invalid data
// 2, group index has valid data
static int ov8858_eos_check_otp_wb(int index)
{
	int flag, i;
	int address_start = 0x7020;
	int address_end = 0x7020;
	// read otp into buffer
	ov8858_eos_otp_write(0x3d84, 0xc0); // program disable, manual mode
	//partial mode OTP write start address 
	ov8858_eos_otp_write(0x3d88, (address_start>>8));
	ov8858_eos_otp_write(0x3d89, (address_start & 0xff));
	// partial mode OTP write end address
	ov8858_eos_otp_write(0x3d8A, (address_end>>8));
	ov8858_eos_otp_write(0x3d8B, (address_end & 0xff));
	ov8858_eos_otp_write(0x3d81, 0x01); // read OTP
	mdelay(5);
	//select group
	flag = ov8858_eos_otp_read(0x7020);
	if(index==1)
	{
	    flag = (flag>>6) & 0x03;
	      }
	else if(index==2)
	{
	    flag = (flag>>4) & 0x03;
	      }
	else
	{
	    flag =( flag>>6) & 0x03;
	      }
	// clear otp buffer
	for (i=address_start;i<=address_end;i++) {
		ov8858_eos_otp_write(i, 0x00);
	}
	if (flag == 0x00) {
		return 0;
	}
	else if (flag & 0x02) {
		return 1;
	}
	else {
		return 2;
	}
}

// index: index of otp group. (1, 2, 3)
// otp_ptr: pointer of ov8858_eos_otp_struct
// return:  0, 
static int ov8858_eos_read_otp_wb(int index, struct ov8858_eos_otp_struct *otp_ptr)
{
	int i, temp;
	int address_start;
	int address_end;
	// read otp into buffer
	ov8858_eos_otp_write(0x3d84, 0xc0); // program disable, manual mode
	//select group
	if(index==1)
	{
	    address_start = 0x7021;
		address_end = 0x7025;
	      }
	else if(index==2)
	{
	    address_start = 0x7026;
		address_end = 0x702a;
	}
	else
	{
	    address_start = 0x702b;
		address_end = 0x702f;
	}
	//partial mode OTP write start address 
	ov8858_eos_otp_write(0x3d88, (address_start>>8));
	ov8858_eos_otp_write(0x3d89, (address_start & 0xff));
	// partial mode OTP write end address
	ov8858_eos_otp_write(0x3d8A, (address_end>>8));
	ov8858_eos_otp_write(0x3d8B, (address_end & 0xff));
	ov8858_eos_otp_write(0x3d81, 0x01); // load otp into buffer
	mdelay(5);
	temp = ov8858_eos_otp_read(address_start + 4);
	(*otp_ptr).rg_ratio = (ov8858_eos_otp_read(address_start )<<2) + ((temp>>6) & 0x03);
	(*otp_ptr).bg_ratio = (ov8858_eos_otp_read(address_start + 1)<<2) + ((temp>>4) & 0x03);
	(*otp_ptr).light_rg = (ov8858_eos_otp_read(address_start + 2) <<2) + ((temp>>2) & 0x03);
	(*otp_ptr).light_bg = (ov8858_eos_otp_read(address_start + 3)<<2) +  (temp & 0x03);
	printk("OTP:rg=%d/%d,bg=%d/%d\n", (*otp_ptr).rg_ratio, OV8858_EOS_RG_Ratio_Typical,\
        (*otp_ptr).bg_ratio, OV8858_EOS_BG_Ratio_Typical);
	// clear otp buffer
	for (i=address_start;i<=address_end;i++) {
		ov8858_eos_otp_write(i, 0x00);
	}
	return 0;
}

// R_gain, sensor red gain of AWB, 0x400 =1
// G_gain, sensor green gain of AWB, 0x400 =1
// B_gain, sensor blue gain of AWB, 0x400 =1
// return 0;
static int ov8858_eos_update_awb_gain(int R_gain, int G_gain, int B_gain)
{
	if (R_gain>0x400) {
		ov8858_eos_otp_write(0x5018, R_gain>>8);
		ov8858_eos_otp_write(0x5019, R_gain & 0x00ff);
	}
	if (G_gain>0x400) {
		ov8858_eos_otp_write(0x501A, G_gain>>8);
		ov8858_eos_otp_write(0x501B, G_gain & 0x00ff);
	}
	if (B_gain>0x400) {
		ov8858_eos_otp_write(0x501C, B_gain>>8);
		ov8858_eos_otp_write(0x501D, B_gain & 0x00ff);
	}
	return 0;
}

// call this function after OV8858 initialization
// return value: 0 update success
// 1, no OTP
static int ov8858_eos_update_otp_wb(void)
{
	struct ov8858_eos_otp_struct current_otp;
	int i;
	int otp_index;// bank 1,2,3
	int temp;
	int R_gain, G_gain, B_gain, G_gain_R, G_gain_B;
	int rg,bg;
	// R/G and B/G of current camera module is read out from sensor OTP
	// check first OTP with valid data
	for(i=1;i<4;i++) {
		temp = ov8858_eos_check_otp_wb(i);
		if (temp == 2) {
			otp_index = i;
			break;
		}
	}
	if (i==4) {
		// no valid wb OTP data
		printk("OTP: no valid wb OTP data\n");
		return 1;
	}
	// set right bank
	ov8858_eos_read_otp_wb(otp_index, &current_otp);
	if(current_otp.light_rg==0) {
		// no light source information in OTP, light factor = 1
		rg = current_otp.rg_ratio;
	}
	else {
        printk("OTP: light_rg = 0\n");
		rg = current_otp.rg_ratio * (current_otp.light_rg + 512) / 1024;
	}
	if(current_otp.light_bg==0) {
		// not light source information in OTP, light factor = 1
		bg = current_otp.bg_ratio;
	}
	else {
        printk("OTP: light_bg = 0\n");
		bg = current_otp.bg_ratio * (current_otp.light_bg + 512) / 1024;
	}
	//calculate G gain
	//0x400 = 1x gain
	if(bg < OV8858_EOS_BG_Ratio_Typical) {
	if (rg< OV8858_EOS_RG_Ratio_Typical) {
		// current_otp.bg_ratio < BG_Ratio_typical &&  
		// current_otp.rg_ratio < RG_Ratio_typical
	    G_gain = 0x400;
		B_gain = 0x400 * OV8858_EOS_BG_Ratio_Typical / bg;
		R_gain = 0x400 * OV8858_EOS_RG_Ratio_Typical / rg; 
	}
	else {
		// current_otp.bg_ratio < BG_Ratio_typical &&  
		// current_otp.rg_ratio >= RG_Ratio_typical
		R_gain = 0x400;
		G_gain = 0x400 * rg / OV8858_EOS_RG_Ratio_Typical;
		B_gain = G_gain * OV8858_EOS_BG_Ratio_Typical /bg;
	}
	}
	else {
		if (rg < OV8858_EOS_RG_Ratio_Typical) {
			// current_otp.bg_ratio >= BG_Ratio_typical &&  
			// current_otp.rg_ratio < RG_Ratio_typical
			B_gain = 0x400;
			G_gain = 0x400 * bg / OV8858_EOS_BG_Ratio_Typical;
			R_gain = G_gain * OV8858_EOS_RG_Ratio_Typical / rg;
		}
		else {
	        // current_otp.bg_ratio >= BG_Ratio_typical &&  
			// current_otp.rg_ratio >= RG_Ratio_typical
			G_gain_B = 0x400 * bg / OV8858_EOS_BG_Ratio_Typical;
            G_gain_R = 0x400 * rg / ov5648_miata_RG_Ratio_Typical;
	        if(G_gain_B > G_gain_R ) {
				B_gain = 0x400;
				G_gain = G_gain_B;
				R_gain = G_gain * OV8858_EOS_RG_Ratio_Typical /rg;
			}
			else {
				R_gain = 0x400;
				G_gain = G_gain_R;
				B_gain = G_gain * OV8858_EOS_BG_Ratio_Typical / bg;
			}
		}    
	}
	ov8858_eos_update_awb_gain(R_gain, G_gain, B_gain);
    
	return 0;
}
#endif

#if OV8858_LENC_EN
// index: index of otp group. (1, 2, 3)
// return:  0, group index is empty
// 1, group index has invalid data
// 2, group index has valid data
static int ov8858_eos_check_otp_lenc(int index)
{
	int flag, i;
	int address_start = 0x703a;
	int address_end = 0x703a;
	// read otp into buffer
	ov8858_eos_otp_write(0x3d84, 0xc0); // program disable, manual mode
	//partial mode OTP write start address 
	ov8858_eos_otp_write(0x3d88, (address_start>>8));
	ov8858_eos_otp_write(0x3d89, (address_start & 0xff));
	// partial mode OTP write end address
	ov8858_eos_otp_write(0x3d8A, (address_end>>8));
	ov8858_eos_otp_write(0x3d8B, (address_end & 0xff));
	ov8858_eos_otp_write(0x3d81, 0x01);
	mdelay(5);
	flag = ov8858_eos_otp_read(0x703a);
	if(index==1)
	{
	    flag = (flag>>6) & 0x03;
	}
	else if(index==2)
	{
	    flag = (flag>>4) & 0x03;
	}
	else
	{
	    flag = (flag>> 2)& 0x03;
	}
	// clear otp buffer
	for (i=address_start;i<=address_end;i++) {
		ov8858_eos_otp_write(i, 0x00);
	}
	if (flag == 0x00) {
		return 0;
	}
	else if (flag & 0x02) {
		return 1;
	}
	else {
		return 2;
	}
}

// index: index of otp group. (1, 2, 3)
// otp_ptr: pointer of ov8858_eos_otp_struct
// return:  0, 
static int ov8858_eos_read_otp_lenc(int index, struct ov8858_eos_otp_struct *otp_ptr)
{
	int i;
	int address_start;
	int address_end;
	// read otp into buffer
	ov8858_eos_otp_write(0x3d84, 0xc0); // program disable, manual mode
	//select group
	if(index==1)
	{
	    address_start = 0x703b;
		address_end = 0x70a8;
	}
	else if(index==2)
	{
	    address_start = 0x70a9;
		address_end = 0x7116;
	}
	else if(index==3)
	{
	    address_start = 0x7117;
		address_end = 0x7184;
	}
	//partial mode OTP write start address 
	ov8858_eos_otp_write(0x3d88, (address_start>>8));
	ov8858_eos_otp_write(0x3d89, (address_start & 0xff));
	// partial mode OTP write end address
	ov8858_eos_otp_write(0x3d8A, (address_end>>8));
	ov8858_eos_otp_write(0x3d8B, (address_end & 0xff));
	ov8858_eos_otp_write(0x3d81, 0x01); // load otp into buffer
	mdelay(5);
	for(i=0;i<110;i++) {
		(* otp_ptr).lenc[i]=ov8858_eos_otp_read(address_start + i);
	}
	// clear otp buffer

	for (i=address_start;i<=address_end;i++) {
		ov8858_eos_otp_write(i, 0x00);
	}
	return 0;
}

// otp_ptr: pointer of ov8858_eos_otp_struct
static int ov8858_eos_update_lenc(struct ov8858_eos_otp_struct * otp_ptr)
{
#if HJ_DISLENC
	int i, temp;
	temp = ov8858_eos_otp_read(0x5000);
	temp = 0x80 | temp;
	ov8858_eos_otp_write(0x5000, temp);
	for(i=0;i<62;i++) {
		ov8858_eos_otp_write(0x5800 + i, (*otp_ptr).lenc[i]);
	}
#endif
	return 0;
}

// call this function after OV8858 initialization
// return value: 0 update success
// 1, no OTP
static int ov8858_eos_update_otp_lenc(void)
{
	struct ov8858_eos_otp_struct current_otp;
	int i;
	int otp_index;
	int temp;
	// check first lens correction OTP with valid data
	for(i=1;i<=3;i++) {
		temp = ov8858_eos_check_otp_lenc(i);
		if (temp == 2) {
			otp_index = i;
			break;
		}
	}
	if (i>3) {
		// no  valid WB OTP data
		return 1;
	}
	ov8858_eos_read_otp_lenc(otp_index, &current_otp);
	ov8858_eos_update_lenc(&current_otp);
	// success
	return 0;
} 
#endif

static void ov8858_eos_OTP_all_calibration(struct msm_camera_i2c_client *i2c_client)
{
    //int ret1;
#if OV8858_AWB_EN
	ov8858_eos_update_otp_wb();
#endif
#if OV8858_LENC_EN
    ov8858_eos_update_otp_lenc();
#endif
}

void ov8858_eos_TCT_OTP_calibration(struct msm_camera_i2c_client *i2c_client)
{
    ov8858_eos_g_client = i2c_client;
	ov8858_eos_otp_write(0x100, 0x1);
	mdelay(66);
    ov8858_eos_OTP_all_calibration(i2c_client);
	ov8858_eos_otp_write(0x100, 0x0);
	//...
}
#endif
