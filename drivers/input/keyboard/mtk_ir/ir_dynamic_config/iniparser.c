// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author Vick.Sun <Vick.Sun@mediatek.com>
 */

//////////////////////////////////////////////////
///
/// file	iniparser.c
/// @brief	Parse Ini config file
/// @author Vick.Sun@MStar Semiconductor Inc.
//////////////////////////////////////////////////
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <linux/slab.h>

#include "iniparser.h"

#define MALLOC _malloc
#define FREE kfree
#define PRINT printk

static struct kmem_cache *_malloc(size_t size)
{
	return kmalloc(size, GFP_KERNEL);
}

static const char *strtrim(char *str)
{
	char *p = str + strlen(str) - 1;

	while (p != str && (isspace(*p) || *p == '\"')) {
		*p = '\0';
		p--;
	}
	p = str;
	while (*p != '\0' && (isspace(*p) || *p == '\"'))
		p++;

	if (p != str) {
		char *d = str;
		char *s = p;

		while (*s != '\0') {
			*d = *s;
			s++;
			d++;
		}
		*d = '\0';
	}

	return str;
}

int alloc_and_init_ini_tree(struct _IniSectionNode_List **root, char *p)
{
	enum _State {
		STAT_NONE = 0,
		STAT_GROUP,
		STAT_KEY,
		STAT_VAL,
		STAT_COMMENT
	} state = STAT_NONE;
	char *group_start = p;
	char *key_start = p;
	char *val_start = p;
	struct _IniSectionNode_List *current_section = NULL;
	struct _IniKeyNode_List *current_key = NULL;

	while (*p != '\0') {
		if (*p == '#' || *p == ';') {
			/*skip comment*/
			while (*p != '\0' && *p != '\n') {
				*p = 0;
				p++;
			}
			if (*p == '\0')
				break;
		}
		switch (state) {
		case STAT_NONE:
		{
			if (*p == '[') {
				state = STAT_GROUP;
				group_start = p + 1;
				if (current_section == NULL) {
					current_section =
					(struct _IniSectionNode_List *)
					MALLOC(sizeof(struct _IniSectionNode_List));
					if (!current_section) {
						PRINT("return on OOM!\n");
						return -1;
					}
					memset(current_section, 0,
					sizeof(struct _IniSectionNode_List));
					*root = current_section;
				} else {
					current_section->next =
						(struct _IniSectionNode_List *)
						MALLOC(sizeof(struct _IniSectionNode_List));
					if (!current_section->next) {
						PRINT("return on OOM!\n");
						return -1;
					}
					memset(current_section->next, 0,
					sizeof(struct _IniSectionNode_List));
					current_section = current_section->next;
				}
			} else if (!isspace(*p)) {
				key_start = p;
				if (current_key == NULL) {
					current_key = (struct _IniKeyNode_List *)
					MALLOC(sizeof(struct _IniKeyNode_List));
					if (!current_key) {
						PRINT("return on OOM!\n");
						return -1;
					}
					memset(current_key, 0, sizeof(struct _IniKeyNode_List));
					if (current_section == NULL) {
						current_section =
							(struct _IniSectionNode_List *)
							MALLOC(sizeof(struct _IniSectionNode_List));
					if (!current_section) {
						PRINT("return on OOM!\n");
						FREE(current_key);
						return -1;
					}
						memset(current_section, 0,
							sizeof(struct _IniSectionNode_List));
						*root = current_section;
					}
					current_section->keys = current_key;
				} else {
					current_key->next =
					(struct _IniKeyNode_List *)
					MALLOC(sizeof(struct _IniKeyNode_List));
					if (!current_key->next) {
						PRINT("return on OOM!\n");
						return -1;
					}
					memset(current_key->next, 0,
					sizeof(struct _IniKeyNode_List));
					current_key = current_key->next;
				}
				state = STAT_KEY;
			}
			break;
		}
		case STAT_GROUP:
		{
			if (*p == ']') {
				*p = '\0';
				current_section->name = strtrim(group_start);
				current_key = NULL;
				state = STAT_NONE;
			}
			break;
		}
		case STAT_KEY:
		{
			if (*p == '=') {
				*p = '\0';
				val_start = p + 1;
				current_key->name = strtrim(key_start);
				state = STAT_VAL;
			}
			break;
		}
		case STAT_VAL:
		{
			if (*p == '\n' || *p == '#' || *p == ';') {
				if (*p != '\n') {
					/*skip comment*/
					while (*p != '\0' && *p != '\n') {
						*p = '\0';
						p++;
					}
				}
				*p = '\0';
				current_key->value = strtrim(val_start);
				state = STAT_NONE;
				break;
			}
		}
		default:
			break;
		}
		p++;
	}
	if (state == STAT_VAL)
		current_key->value = strtrim(val_start);
	return 0;
}
EXPORT_SYMBOL_GPL(alloc_and_init_ini_tree);
void release_ini_tree(struct _IniSectionNode_List *root)
{
	struct _IniSectionNode_List *current_section = root;
	struct _IniKeyNode_List *current_key;
	struct _IniSectionNode_List *last_section = NULL;
	struct _IniKeyNode_List *last_key = NULL;

	while (current_section != NULL) {
		current_key = current_section->keys;
		while (current_key != NULL) {
			last_key = current_key;
			current_key = current_key->next;
			FREE(last_key);
			last_key = NULL;
		}
		last_section = current_section;
		current_section = current_section->next;
		FREE(last_section);
		last_section = NULL;
	}
}
EXPORT_SYMBOL_GPL(release_ini_tree);
struct _IniSectionNode_List *get_section
	(struct _IniSectionNode_List *root, const char *name)
{
	struct _IniSectionNode_List *current_section;

	current_section = root;
	while (current_section != NULL) {
		if (!strcmp(name, current_section->name))
			return current_section;
		current_section = current_section->next;
	}
	return NULL;
}
EXPORT_SYMBOL_GPL(get_section);
size_t get_section_num(struct _IniSectionNode_List *root)
{
	size_t i = 0;
	struct _IniSectionNode_List *current_section = root;

	while (current_section != NULL) {
		i++;
		current_section = current_section->next;
	}

	return i;
}
EXPORT_SYMBOL_GPL(get_section_num);
void foreach_section(struct _IniSectionNode_List *root, void (*fun)(struct _IniSectionNode_List *,
				struct _IniSectionNode_List *, const char *))
{
	struct _IniSectionNode_List *current_section;

	current_section = root;
	while (current_section != NULL) {
		fun(root, current_section, current_section->name);
		current_section = current_section->next;
	}
}
EXPORT_SYMBOL_GPL(foreach_section);
void foreach_key(struct _IniSectionNode_List *root,
	struct _IniSectionNode_List *node,
	void (*fun)(struct _IniSectionNode_List *,
	const char *, const char *))
{
	struct _IniKeyNode_List *current_key;

	if (node && node->keys) {
		current_key = node->keys;
		while (current_key != NULL) {
			fun(root, current_key->name, current_key->value);
			current_key = current_key->next;
		}
	}
}
EXPORT_SYMBOL_GPL(foreach_key);
const char *get_key_value(struct _IniSectionNode_List *node, const char *name)
{
	struct _IniKeyNode_List *current_key;

	if (node && node->keys) {
		current_key = node->keys;
		while (current_key != NULL) {
			if (!strcmp(current_key->name, name))
				return current_key->value;
			current_key = current_key->next;
		}
	}
	return NULL;
}
EXPORT_SYMBOL_GPL(get_key_value);
size_t get_key_num(struct _IniSectionNode_List *node)
{
	size_t i = 0;
	struct _IniKeyNode_List *current_key;

	if (node && node->keys) {
		current_key = node->keys;
		while (current_key != NULL) {
			i++;
			current_key = current_key->next;
		}
	}
	return i;
}
EXPORT_SYMBOL_GPL(get_key_num);
static void print_key_value(struct _IniSectionNode_List *root, const char *name,
					const char *value)
{
	PRINT("%s=%s\n", name, value);
}
static void print_section(struct _IniSectionNode_List *root,
	struct _IniSectionNode_List *node,	const char *name)
{
	PRINT("[%s]\n", name);
	foreach_key(root, node, print_key_value);
}

void dump_ini(struct _IniSectionNode_List *root)
{
	foreach_section(root, print_section);
}
EXPORT_SYMBOL_GPL(dump_ini);
