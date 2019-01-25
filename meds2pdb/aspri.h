/*
 * aspri.h - main SPRI header file
 *
 * Copyright (c) 2000, 2001, 2010, 2011 - Zephyr Software
 *
 * This file is part of the Strata dynamic code modification infrastructure.
 * This file may be used and modified for non-commercial purposes as long as
 * all copyright, permission, and nonwarranty notices are preserved.
 * Redistribution is prohibited without prior written consent from Zephyr
 * Software.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: Zephyr Software
 * e-mail: jwd@zephyr-software.com
 * URL   : http://www.zephyr-software.com/
 *
 */


/*
 * aspri.h
 * author: Jason Hiser, Anh Nguyen-Tuong
 */

#ifndef aspri_h
#define aspri_h

typedef struct aspri_address aspri_address_t;
struct aspri_address
{
	char *library_name;
	VirtualOffset_t offset;
	bool isCurrentPC;
};

#endif
