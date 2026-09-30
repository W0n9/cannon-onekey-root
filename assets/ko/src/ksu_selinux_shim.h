/* SPDX-License-Identifier: GPL-2.0 */
/*
 * cannon: runtime-resolution shim for SELinux internals that the stock
 * kernel does not export (they are all TEXT symbols, present in kallsyms;
 * CONFIG_KALLSYMS_ALL=n only hides DATA symbols).
 *
 * Include this header FIRST in sepolicy.c / rules.c so that every
 * declaration from security/selinux headers is renamed to the ksu_
 * runtime-resolved wrapper. The wrappers resolve once at module init.
 */
#ifndef __KSU_SELINUX_SHIM_H
#define __KSU_SELINUX_SHIM_H

#include <linux/types.h>

struct avtab;
struct avtab_key;
struct avtab_datum;
struct avtab_node;
struct hashtab;
struct ebitmap;
struct selinux_avc;
struct selinux_state;

struct avtab_node *ksu_avtab_search_node(struct avtab *h,
					 struct avtab_key *key);
struct avtab_node *ksu_avtab_search_node_next(struct avtab_node *node,
					      int specified);
struct avtab_node *ksu_avtab_insert_nonunique(struct avtab *h,
					      struct avtab_key *key,
					      struct avtab_datum *d);
int ksu_hashtab_insert(struct hashtab *h, void *k, void *d);
void *ksu_hashtab_search(struct hashtab *h, const void *k);
int ksu_ebitmap_get_bit(struct ebitmap *e, unsigned long bit);
int ksu_ebitmap_set_bit(struct ebitmap *e, unsigned long bit, int value);
int ksu_avc_ss_reset(struct selinux_avc *avc, u32 seqno);
void ksu_selinux_status_update_policyload(struct selinux_state *state,
					  int seqno);
void ksu_selnl_notify_policyload(u32 seqno);

int ksu_selinux_shim_init(void);
void ksu_selinux_shim_exit(void);

/* rename the kernel declarations to the ksu wrappers */
#define avtab_search_node ksu_avtab_search_node
#define avtab_search_node_next ksu_avtab_search_node_next
#define avtab_insert_nonunique ksu_avtab_insert_nonunique
#define hashtab_insert ksu_hashtab_insert
#define hashtab_search ksu_hashtab_search
#define ebitmap_get_bit ksu_ebitmap_get_bit
#define ebitmap_set_bit ksu_ebitmap_set_bit
#define avc_ss_reset ksu_avc_ss_reset
#define selinux_status_update_policyload ksu_selinux_status_update_policyload
#define selnl_notify_policyload ksu_selnl_notify_policyload

#endif
