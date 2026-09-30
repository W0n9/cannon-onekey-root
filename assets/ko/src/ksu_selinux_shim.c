// SPDX-License-Identifier: GPL-2.0
/*
 * Runtime symbol resolution for SELinux internals not exported by the
 * cannon 4.14 stock kernel. All targets are TEXT symbols recoverable via
 * kallsyms_lookup_name (names verified against the stock Image kallsyms).
 */
#include <linux/kernel.h>
#include <linux/kallsyms.h>
#include <linux/printk.h>
#include <linux/string.h>

#include "ss/avtab.h"
#include "ss/hashtab.h"
#include "ss/ebitmap.h"
#include "include/avc_ss.h"
#include "include/security.h"

#include "ksu_selinux_shim.h"

#define KSU_SHIM(sym)                                                          \
	static typeof(sym) *ksu_p_##sym;                                       \
	typeof(sym) *ksu_get_p_##sym(void)                                     \
	{                                                                      \
		return ksu_p_##sym;                                            \
	}

/* must match the mangled names in the stock 4.14.186 Image kallsyms */
#define KSU_SHIM_LIST(X)                                                       \
	X(avtab_search_node)                                                   \
	X(avtab_search_node_next)                                              \
	X(avtab_insert_nonunique)                                              \
	X(hashtab_insert)                                                      \
	X(hashtab_search)                                                      \
	X(ebitmap_get_bit)                                                     \
	X(ebitmap_set_bit)                                                     \
	X(avc_ss_reset)                                                        \
	X(selinux_status_update_policyload)                                    \
	X(selnl_notify_policyload)

KSU_SHIM_LIST(KSU_SHIM)

#define KSU_SHIM_NAME(sym) #sym

int ksu_selinux_shim_init(void)
{
	int missing = 0;

	/*
	 * 39-r90: resolve the .cfi_jt trampoline first (the address of a
	 * CFI-jump-table entry is what the rest of this module already uses
	 * for indirect calls into the kernel — core_hook.c KSU_HOOK_SYMS
	 * and the syscall-table sanity check — and it is the only form that
	 * passes the module's own __cfi_check when called through function
	 * pointers: three boots (79e2241f / a028853a / edc5a69f) died in
	 * __cfi_check_fail with the bare-name address, because a bare
	 * kallsyms entry is the CFI-guarded function entry whose prologue
	 * hash does not match what the module's call sites expect. Fall
	 * back to the bare name only if no jump-table entry was emitted.
	 */
#define KSU_SHIM_RESOLVE(sym)                                                  \
	do {                                                                   \
		ksu_p_##sym = (void *)kallsyms_lookup_name(                    \
			KSU_SHIM_NAME(sym) ".cfi_jt");                         \
		if (!ksu_p_##sym)                                              \
			ksu_p_##sym = (void *)kallsyms_lookup_name(            \
				KSU_SHIM_NAME(sym));                           \
		if (!ksu_p_##sym) {                                            \
			pr_err("ksu_shim: unresolved %s\n",                    \
			       KSU_SHIM_NAME(sym));                            \
			missing++;                                             \
		}                                                              \
	} while (0);

	KSU_SHIM_LIST(KSU_SHIM_RESOLVE)

#undef KSU_SHIM_RESOLVE

	return missing ? -ENOENT : 0;
}

void ksu_selinux_shim_exit(void)
{
#define KSU_SHIM_CLEAR(sym) ksu_p_##sym = NULL;
	KSU_SHIM_LIST(KSU_SHIM_CLEAR)
#undef KSU_SHIM_CLEAR
}

/* wrappers.
 * 39-r90: every wrapper guards its pointer — shim_init() failures are
 * logged but non-fatal at load time, and a NULL blr here would be a
 * deferred panic. */
struct avtab_node *ksu_avtab_search_node(struct avtab *h,
					 struct avtab_key *key)
{
	if (!ksu_p_avtab_search_node)
		return NULL;
	return ksu_p_avtab_search_node(h, key);
}

struct avtab_node *ksu_avtab_search_node_next(struct avtab_node *node,
					      int specified)
{
	if (!ksu_p_avtab_search_node_next)
		return NULL;
	return ksu_p_avtab_search_node_next(node, specified);
}

struct avtab_node *ksu_avtab_insert_nonunique(struct avtab *h,
					      struct avtab_key *key,
					      struct avtab_datum *d)
{
	if (!ksu_p_avtab_insert_nonunique)
		return NULL;
	return ksu_p_avtab_insert_nonunique(h, key, d);
}

int ksu_hashtab_insert(struct hashtab *h, void *k, void *d)
{
	if (!ksu_p_hashtab_insert)
		return -1;
	return ksu_p_hashtab_insert(h, k, d);
}

void *ksu_hashtab_search(struct hashtab *h, const void *k)
{
	if (!ksu_p_hashtab_search)
		return NULL;
	return ksu_p_hashtab_search(h, k);
}

int ksu_ebitmap_get_bit(struct ebitmap *e, unsigned long bit)
{
	if (!ksu_p_ebitmap_get_bit)
		return 0;
	return ksu_p_ebitmap_get_bit(e, bit);
}

int ksu_ebitmap_set_bit(struct ebitmap *e, unsigned long bit, int value)
{
	if (!ksu_p_ebitmap_set_bit)
		return -1;
	return ksu_p_ebitmap_set_bit(e, bit, value);
}

int ksu_avc_ss_reset(struct selinux_avc *avc, u32 seqno)
{
	if (!ksu_p_avc_ss_reset)
		return 0;
	return ksu_p_avc_ss_reset(avc, seqno);
}

void ksu_selinux_status_update_policyload(struct selinux_state *state,
					  int seqno)
{
	if (!ksu_p_selinux_status_update_policyload)
		return;
	ksu_p_selinux_status_update_policyload(state, seqno);
}

void ksu_selnl_notify_policyload(u32 seqno)
{
	if (!ksu_p_selnl_notify_policyload)
		return;
	ksu_p_selnl_notify_policyload(seqno);
}

/*
 * clang-18 lowers memcmp(a, b, n) == 0 to bcmp(), which this 4.14 kernel
 * does not export. Provide a module-local one backed by memcmp.
 */
#undef bcmp
int bcmp(const void *s1, const void *s2, size_t n)
{
	return memcmp(s1, s2, n);
}
