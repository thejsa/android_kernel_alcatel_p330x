/******************************************************************************/
/*                                                               Date:07/2013 */
/*                             PRESENTATION                                   */
/*                                                                            */
/*      Copyright 2013 TCL Communication Technology Holdings Limited.         */
/*                                                                            */
/* This material is company confidential, cannot be reproduced in any form    */
/* without the written permission of TCL Communication Technology Holdings    */
/* Limited.                                                                   */
/*       Msg21xxA serial TouchScreen driver hearder file.                     */
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
/* 13/07/31| Qijun Xu       |  bug 487707        | modify driver to meet qcom */
/*         |                |                    | style,and running          */
/*---------|----------------|--------------------|--------------------------- */
/******************************************************************************/

#ifndef __LINUX_MSG21xx_TS_H__
#define __LINUX_MSG21xx_TS_H__


struct msg21xx_ts_platform_data {
	unsigned long irqflags;
	u32 x_max;
	u32 y_max;
	unsigned irq_gpio;
	unsigned reset_gpio;
	u32 irq_flags;
	u32 reset_flags;
	bool i2c_pull_up;

};

struct fw_version {
	unsigned short major;
	unsigned short minor;
	unsigned short VenderID;
};
#endif
