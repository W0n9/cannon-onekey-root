/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __KSU_SELINUX_RESOLVE_H
#define __KSU_SELINUX_RESOLVE_H

#include <linux/types.h>

int ksu_selinux_resolve_init(void);
void ksu_selinux_resolve_exit(void);
void *ksu_selinux_state(void);
void ksu_selinux_set_enforce(bool enforce);
bool ksu_selinux_get_enforce();

#endif
