/**
 * This file contains basic ioctls to user space and data structures used.
 *
 * Copyright (C) 2011, Texas Instruments, Incorporated
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __TI81XX_EP_HLPR__
#define __TI81XX_EP_HLPR__

#undef u32
#define u32 unsigned int

/* ioctls defined for driver as well as user space */
#define TI81XX_RC_START_ADDR_AREA	_IOWR('P', 1, struct ti81xx_start_addr_area)
#ifndef _DM81XX_EXCLUDE_
#define TI81XX_RC_SEND_MSI		_IOWR('P', 2, unsigned int)
#endif

/**
 * ti81xx_start_addr_area: dedicated area's start address realated information
 * @start_addr_virt: kernel virtual address
 * @start_addr_phy: correspomding physical address
 *
 * this structure contains information about start address of dedicated
 * physical memory.
 */

struct ti81xx_start_addr_area {
	u32 start_addr_virt;
	u32 start_addr_phy;
};

#endif
