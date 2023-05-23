/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 *
 */

#ifndef __INI_PARSER__
#define __INI_PARSER__
struct _IniKeyNode_List {
	const char *name;
	const char *value;
	struct _IniKeyNode_List *next;
};

struct _IniSectionNode_List {
	const char *name;
	struct _IniKeyNode_List *keys;
	struct _IniSectionNode_List *next;
};

int alloc_and_init_ini_tree(struct _IniSectionNode_List **root, char *p);
void release_ini_tree(struct _IniSectionNode_List *root);
struct _IniSectionNode_List *get_section(struct _IniSectionNode_List *root, const char *name);
size_t get_section_num(struct _IniSectionNode_List *root);
void foreach_section(struct _IniSectionNode_List *root, void (*fun)(struct _IniSectionNode_List *,
			struct _IniSectionNode_List *, const char *));
void foreach_key(struct _IniSectionNode_List *root, struct _IniSectionNode_List *node, void (*fun)
			(struct _IniSectionNode_List *, const char *, const char *));
const char *get_key_value(struct _IniSectionNode_List *node, const char *name);
size_t get_key_num(struct _IniSectionNode_List *node);
void dump_ini(struct _IniSectionNode_List *root);
#endif
