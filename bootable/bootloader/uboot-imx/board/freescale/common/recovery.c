/*
 * Freescale Android Recovery mode checking routing
 *
 * Copyright (C) 2010 Freescale Semiconductor, Inc.
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */
#include <common.h>
#include <malloc.h>
#include "recovery.h"
#ifdef CONFIG_MXC_KPD
#include <mxc_keyb.h>
#endif

extern int check_recovery_cmd_file(void);
extern int cleanup_before_recovery(void);

extern enum boot_device get_boot_device(void);
int factory_test = 0;

#ifdef CONFIG_MXC_KPD

#define PRESSED_HOME	0x01
#define PRESSED_POWER	0x02
#define RECOVERY_KEY_MASK (PRESSED_HOME | PRESSED_POWER)

inline int test_key(int value, struct kpp_key_info *ki)
{
	return (ki->val == value) && (ki->evt == KDepress);
}

int check_key_pressing(void)
{
	struct kpp_key_info *key_info;
	int state = 0, keys, i;

	mxc_kpp_init();

	puts("Detecting HOME+POWER key for recovery ...\n");

	/* Check for home + power */
	keys = mxc_kpp_getc(&key_info);
	if (keys < 2)
		return 0;

	for (i = 0; i < keys; i++) {
		if (test_key(CONFIG_POWER_KEY, &key_info[i]))
			state |= PRESSED_HOME;
		else if (test_key(CONFIG_HOME_KEY, &key_info[i]))
			state |= PRESSED_POWER;
	}

	free(key_info);

	if ((state & RECOVERY_KEY_MASK) == RECOVERY_KEY_MASK)
		return 1;

	return 0;
}
#else
/* If not using mxc keypad, currently we will detect power key on board */
int check_key_pressing(void)
{
	return 0;
}
#endif
extern struct reco_envs supported_reco_envs[];

int backup_status = 0;
void setup_recovery_env(void)
{
	char *env, *boot_args, *boot_cmd;
	int bootdev = get_boot_device();
	if( 1 == backup_status) {
		printf("setup env for recovery update..\n");
		boot_args = supported_reco_envs[bootdev].args;
		boot_cmd = supported_reco_envs[bootdev].cmd;
	} else if( 2 == backup_status) {
		printf("setup env for recovery backup..\n");
		boot_args = CONFIG_ANDROID_RECOVERY_BOOTARGS_MMC_BACKUP;
		boot_cmd = CONFIG_ANDROID_RECOVERY_BOOTCMD_MMC_BACKUP;
	} else if( 3 == backup_status) {
		printf("setup env for recovery cleanboot..\n");
		boot_args = CONFIG_ANDROID_RECOVERY_BOOTARGS_MMC_CLEANBOOT;
		boot_cmd = CONFIG_ANDROID_RECOVERY_BOOTCMD_MMC_CLEANBOOT;
	} else {
		printf("setup env for recovery self update..\n");
		boot_args = CONFIG_ANDROID_RECOVERY_BOOTARGS_MMC_RECOVERY;
		boot_cmd = CONFIG_ANDROID_RECOVERY_BOOTCMD_MMC_RECOVERY;
	}
	setenv("bootcmd_android_recovery", boot_cmd);
	setenv("bootargs_android_recovery", boot_args);
	setenv("bootcmd", "run bootcmd_android_recovery");
	cleanup_before_recovery();
}

/* export to lib_arm/board.c */
void check_recovery_mode(void)
{
	int ret = 0;
	factory_test = 0;
	backup_status = 0;
	ret = tractor_check_key_pressing();
	if(ret == UPDATE_SYSTEM) {
		/*while menu key is put down, it will burn recovery self mandatorily*/
		backup_status = 1;
		setup_recovery_env();
	} else if (ret == BACKUP_MODE) {
		backup_status = 2;
		setup_recovery_env();
	} else if (ret == CLEAN_MODE) {
		backup_status = 3;
		setup_recovery_env();
	} else if (ret == UPDATE_RECOVERY)  {
		backup_status = 4;
		setup_recovery_env();
	}
	printf("backup_status:%d\n",backup_status);
}
