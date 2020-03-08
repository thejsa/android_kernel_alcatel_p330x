/*
 * Generic GPIO card-detect helper header
 *
 * Copyright (C) 2011, Guennadi Liakhovetski <g.liakhovetski@gmx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef MMC_CD_GPIO_H
#define MMC_CD_GPIO_H

struct mmc_host;
/* [PLATFORM]-ADD-BEGIN by TCTNB.WJ, 2013/3/4, PR-581680 & 608411 */
#if defined(CONFIG_TCT_8X26_RIO4G)\
    || defined(CONFIG_TCT_8X26_RIO6)\
    || defined(CONFIG_TCT_8X26_RIO6_TF)
int mmc_cd_get_status(struct mmc_host *host);
#endif
/* [PLATFORM]-ADD-END by TCTNB.WJ */
int mmc_cd_gpio_request(struct mmc_host *host, unsigned int gpio);
void mmc_cd_gpio_free(struct mmc_host *host);

#endif
