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
 * 13/08/30      Hu Jin       Add OTP support
 * 13/10/17      Hu Jin       mod for miata
 * 13/11/13      Hu Jin       sync to GS --------------------------------------------------------------------------------------
*/
// R/G and B/G of typical camera module is defined here, the typical camera module is selected by CameraAnalyzer.
#ifndef __OV5648_V4L2_TCT_OTP_H__
#define __OV5648_V4L2_TCT_OTP_H__

#define OTP_DEBUG_ON	0
#define RG_Ratio_Typical	379
#define BG_Ratio_Typical	350

static struct msm_camera_i2c_client *g_client;

struct otp_struct {
     int customer_id;
     int module_integrator_id;
     int lens_id;
     int rg_ratio;
     int bg_ratio;
     int user_data[2];
     int light_rg;
     int light_bg;
};

static int32_t my_otp_write(uint32_t addr, uint16_t data)
{
    int32_t rc=0;
	if (!g_client)
        printk("OTP: ov5648_TCT_client null\n");
    else{
#if 1
        rc = g_client->i2c_func_tbl->i2c_write(g_client, 
            									addr, data, MSM_CAMERA_I2C_BYTE_DATA);
		if(rc < 0)
            printk("OTP: write error\n");
#endif
    }
    return rc;
}

static int16_t my_otp_read(uint32_t addr)
{
    uint16_t data = 0;
    int32_t rc = 0;
    if (!g_client)
        printk("OTP: ov5648_TCT_client null\n");
    else{
        rc = g_client->i2c_func_tbl->i2c_read(g_client,
            									addr, &data, MSM_CAMERA_I2C_BYTE_DATA);
		if(rc < 0)
            printk("OTP: read error\n");
    }
    return data;
}

// index: index of otp group. (0, 1, 2)
// return:    0, group index is empty
//       1, group index has invalid data
//       2, group index has valid data
int check_otp(int index)
{
     int temp, i;
     int address;
     
     if (index<2)
     {
        // read otp --Bank 0
		my_otp_write(0x3d84, 0xc0);
		my_otp_write(0x3d85, 0x00);
		my_otp_write(0x3d86, 0x0f);

		my_otp_write(0x3d81, 0x01);
		mdelay(1);
		address = 0x3d05 + index*9;
     }
     else{
		// read otp --Bank 1
		my_otp_write(0x3d84, 0xc0);
		my_otp_write(0x3d85, 0x10);
		my_otp_write(0x3d86, 0x1f);
		my_otp_write(0x3d81, 0x01);
		mdelay(1);
		address = 0x3d05 + index*9-16;
     }
     temp = my_otp_read(address);
 
     // disable otp read
     my_otp_write(0x3d81, 0x00);

     // clear otp buffer
     for (i=0;i<16;i++) {
         my_otp_write(0x3d00 + i, 0x00);
     }
 
     if (!temp) {
         return 0;
     }
     else if ((!(temp & 0x80)) && (temp&0x7f)) {
         return 2;
		 
	  }
     else {
         return 1;
     }
}
 
// index: index of otp group. (0, 1, 2)
// return:    0, 
int read_otp(int index, struct otp_struct * otp_ptr)
{
     int i;
     int address;
 
     // read otp into buffer 
     if (index<2)
     {
         // read otp --Bank 0
         my_otp_write(0x3d84, 0xc0);
         my_otp_write(0x3d85, 0x00);
         my_otp_write(0x3d86, 0x0f);
         my_otp_write(0x3d81, 0x01);
         mdelay(1);
         address = 0x3d05 + index*9;
     }
     else{
         // read otp --Bank 1
         my_otp_write(0x3d84, 0xc0);
         my_otp_write(0x3d85, 0x10);
         my_otp_write(0x3d86, 0x1f);

		my_otp_write(0x3d81, 0x01);
         mdelay(1);
         address = 0x3d05 + index*9-16;
     }
     
     (*otp_ptr).customer_id = (my_otp_read(address) & 0x7f);
     (*otp_ptr).module_integrator_id = my_otp_read(address);
     (*otp_ptr).lens_id = my_otp_read(address + 1);
     (*otp_ptr).rg_ratio = (my_otp_read(address + 2)<<2) + (my_otp_read(address + 6)>>6) ;
     (*otp_ptr).bg_ratio = (my_otp_read(address + 3)<<2) +((my_otp_read(address + 6)>>4)& 0x03);
     (*otp_ptr).user_data[0] = my_otp_read(address + 4);
     (*otp_ptr).user_data[1] = my_otp_read(address + 5);
     (*otp_ptr).light_rg = (my_otp_read(address + 7)<<2) + ((my_otp_read(address + 6)>>2)& 0x03);
     (*otp_ptr).light_bg = (my_otp_read(address + 8)<<2) + (my_otp_read(address + 6)&0x03);


#if OTP_DEBUG_ON
     printk("\n OTP: ****************************\n");
     printk(" module_integrator_id=[0x%x] lens_id=[0x%x]\n\
          rg_ratio=[0x%x] bg_ratio=[0x%x] light_rg=[0x%x] light_bg=[0x%x]\n", \
         (*otp_ptr).module_integrator_id, (*otp_ptr).lens_id,\
         (*otp_ptr).rg_ratio, (*otp_ptr).bg_ratio,\
         (*otp_ptr).light_rg, (*otp_ptr).light_bg);
     printk(" \n****************************\n");
#endif
	 printk("OTP:rg=%d/%d,bg=%d/%d\n", (*otp_ptr).rg_ratio, RG_Ratio_Typical,\
	 									(*otp_ptr).bg_ratio, BG_Ratio_Typical);
     // disable otp read
     my_otp_write(0x3d81, 0x00);
 
     // clear otp buffer
     for (i=0;i<16;i++) {
         my_otp_write(0x3d00 + i, 0x00);
     }
 
     return 0; 
}
 
// R_gain, sensor red gain of AWB, 0x400 =1
// G_gain, sensor green gain of AWB, 0x400 =1
// B_gain, sensor blue gain of AWB, 0x400 =1
// return 0;
int update_awb_gain(int R_gain, int G_gain, int B_gain)
{
     if (R_gain>0x400) {
         my_otp_write(0x5186, R_gain>>8);
         my_otp_write(0x5187, R_gain & 0x00ff);
     }
 
     if (G_gain>0x400) {
         my_otp_write(0x5188, G_gain>>8);
         my_otp_write(0x5189, G_gain & 0x00ff);
     }
 
     if (B_gain>0x400) {
         my_otp_write(0x518a, B_gain>>8);
         my_otp_write(0x518b, B_gain & 0x00ff);
     }
     return 0;
}
 
// call this function after OV5648 initialization
// return value: 0 update success
//       1, no OTP
int update_otp(struct msm_camera_i2c_client *client)
{
     struct otp_struct current_otp;
     int i;
     int otp_index;
     int temp;
	  int R_gain, G_gain, B_gain, G_gain_R, G_gain_B;
     int rg,bg;
 
     // R/G and B/G of current camera module is read out from sensor OTP
     // check first OTP with valid data
     for(i=0;i<3;i++) {
         temp = check_otp(i);
         if (temp == 2) {
              otp_index = i;
              break;
         }
     }
     if (i==3) {
         // no valid wb OTP data
         printk("OTP: no valid OTP data\n");
         return 1;
     }
 
     read_otp(otp_index, &current_otp);
 
     if(current_otp.light_rg==0) {
         // no light source information in OTP
         rg = current_otp.rg_ratio;
     }
     else {
         // light source information found in OTP
         rg = current_otp.rg_ratio * (current_otp.light_rg +512) / 1024;
     }
 
     if(current_otp.light_bg==0) {
         // no light source information in OTP
         bg = current_otp.bg_ratio;
     }
     else {
         // light source information found in OTP
         bg = current_otp.bg_ratio * (current_otp.light_bg +512) / 1024;
     }
     //calculate G gain
     //0x400 = 1x gain
     if(bg < BG_Ratio_Typical) {
         if (rg< RG_Ratio_Typical) {
              // current_otp.bg_ratio < BG_Ratio_typical &&  
              // current_otp.rg_ratio < RG_Ratio_typical
              G_gain = 0x400;
              B_gain = 0x400 * BG_Ratio_Typical / bg;
				R_gain = 0x400 * RG_Ratio_Typical / rg; 
         }
         else {
              // current_otp.bg_ratio < BG_Ratio_typical &&  
              // current_otp.rg_ratio >= RG_Ratio_typical
         R_gain = 0x400;
              G_gain = 0x400 * rg / RG_Ratio_Typical;
         B_gain = G_gain * BG_Ratio_Typical /bg;
         }
     }
	 
   else {
         if (rg < RG_Ratio_Typical) {
              // current_otp.bg_ratio >= BG_Ratio_typical &&  
              // current_otp.rg_ratio < RG_Ratio_typical
         B_gain = 0x400;
         G_gain = 0x400 * bg / BG_Ratio_Typical;
         R_gain = G_gain * RG_Ratio_Typical / rg;
         }
         else {
              // current_otp.bg_ratio >= BG_Ratio_typical &&  
              // current_otp.rg_ratio >= RG_Ratio_typical
         G_gain_B = 0x400 * bg / BG_Ratio_Typical;
              G_gain_R = 0x400 * rg / RG_Ratio_Typical;
 
         if(G_gain_B > G_gain_R ) {
                       B_gain = 0x400;
                       G_gain = G_gain_B;
                       R_gain = G_gain * RG_Ratio_Typical /rg;
              }
         else {
                   R_gain = 0x400;
                       G_gain = G_gain_R;
                   B_gain = G_gain * BG_Ratio_Typical / bg;
              }
    }    
     }

   update_awb_gain(R_gain, G_gain, B_gain);
     return 0;
}

static int ov5648_1st_update_otp(struct msm_camera_i2c_client *client)
{
    g_client = client;
    update_otp(client);
    
	return 0;
}

#endif
