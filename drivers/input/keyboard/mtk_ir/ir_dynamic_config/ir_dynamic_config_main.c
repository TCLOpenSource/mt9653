// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 * Author Vick.Sun <Vick.Sun@mediatek.com>
 */
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/kfifo.h>
#include <linux/reboot.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/init.h>

#include "../mtk_input_events.h"
#include "ir_dynamic_config.h"
#include "iniparser.h"
#include "../share_struct.h"

static int debug = 1;
#define STRINIT(g, h) (g h)
#if defined(CONFIG_DATA_SEPARATION)
#define PROJECT_ID_LEN 5
#define PROJECT_ID_BUF_SIZE	 50 //project_id.ini length
#define DATA_INDEX_NAME_BUF_SIZE  100//dataIndex.ini path length
#define IR_CONFIG_FILE_NAME_BUF_SIZE  50//ir_config.ini path length
#endif
module_param(debug, int, 0644);
MODULE_PARM_DESC(debug, "debug:\n\t0: error only\n\t1:debug info\n\t2:all\n");

#if defined(CONFIG_DATA_SEPARATION)
static char *config_file;
static char *purelinux_config_file;
#else

static char ir_config_path[IR_CONFIG_PATH_LEN] = CONFIG_MTK_TV_IR_CONFIG_PATH;
static char *config_file = ir_config_path;

static char *purelinux_config_file = "/config/ir_config.ini";
#endif
module_param(purelinux_config_file, charp, 0644);
MODULE_PARM_DESC(purelinux_config_file, "config file path");

module_param(config_file, charp, 0644);
MODULE_PARM_DESC(config_file, "config file path");

#define DBG_ALL(__fmt, args...) \
	do { \
		if (debug >= 2) { \
			pr_info("[IR config]"__fmt, ##args); \
		} \
	} while (0)
#define DBG_INFO(__fmt, args...) \
	do { \
		if (debug >= 1) { \
			pr_info("[IR config]"__fmt, ##args); \
		} \
	} while (0)
#define DBG_ERR(__fmt, args...) \
	do { \
		if (debug >= 0) { \
			pr_info("[IR config]"__fmt, ##args); \
		} \
	} while (0)

static struct IR_Profile_s *ir_profiles;
static int ir_profile_num;
static int ir_profile_parse_num;
struct key_map_list *key_map_lists;

static int by_pass_kernel_keymap_num;
static int by_pass_kernel_keymap_parse_num;
static struct by_pass_kernel_keymap_Profile_s *by_pass_kernel_keymap_profiles;

#if defined(CONFIG_MTK_PM_TODO_FIX)
static struct IR_PM51_Profile_s *ir_PM51_profiles;
static int ir_PM51_profile_num;
static int ir_PM51_profile_parse_num;
static unsigned char ir_PM51_cfg_data[32] = {0};
static unsigned char *ir_p16 = &ir_PM51_cfg_data[IR_HEAD16_KEY_PM_CFG_OFFSET];
static unsigned char *ir_p32 = &ir_PM51_cfg_data[IR_HEAD32_KEY_PM_CFG_OFFSET];
#endif

struct key_value_s IR_KeysList[] = {
#include "input_keys_available.h"
};

#if defined(CONFIG_MTK_PM_TODO_FIX)
unsigned int get_ir_keycode(unsigned int scancode)
{
	int i = 0;
	unsigned int ir_header_code = 0;
	unsigned int keycode = KEY_RESERVED;

	if (ir_PM51_profiles == NULL) {
		DBG_ERR("ir_PM51_profiles is NULL\n");
		return keycode;
	}

	DBG_ALL("ir_PM51_profile_parse_num:%d\n", ir_PM51_profile_parse_num);
	//Search IR Header from PM51 profiles first
	for (i = 0; i < ir_PM51_profile_parse_num; i++) {
		if (scancode == ir_PM51_profiles[i].u32PowerKey) {
			ir_header_code = ir_PM51_profiles[i].u32Header;
			DBG_ALL("Match ir_PM51_profiles success, scancode:%u, ir_header_code:%u\n",
						scancode, ir_header_code);
			break;
		}
	}
	if (i == ir_PM51_profile_parse_num) {
		DBG_ERR("Match ir_PM51_profiles failed\n");
		return keycode;
	}

	//Get keycode from ir_header_code and scancode
	keycode = MIRC_Get_Keycode(ir_header_code, scancode);
	return keycode;
}
EXPORT_SYMBOL(get_ir_keycode);
#endif

static int str2ul(const char *str, u32 *val)
{
	unsigned long tmpval;

	tmpval = *val;
	if (kstrtol(str, 0, &tmpval) == -1)
		return -1;
	*val = tmpval;
	return 0;
}

static void parse_all_keymaps(struct _IniSectionNode_List *root,
		const char *name, const char *value)
{
	struct _IniKeyNode_List *k;
	const char *enable;
	const char *protocol;
	const char *header;
	const char *speed;
	const char *keymap_name;
	struct _IniSectionNode_List *keymap_sec;
	int keymap_size;
	struct key_map_table *table;
	int i;
	struct IR_Profile_s *curr_ir_profile = &ir_profiles[ir_profile_parse_num];
	struct _IniSectionNode_List *ir_config = get_section(root, value);

	if (ir_profile_parse_num > 15)
		return;
	if (ir_config == NULL) {
		DBG_ERR("no section named %s\n", value);
		return;
	}
	/**
	 * Keymap is Enabled?
	 */
	enable = get_key_value(ir_config, "Enable");
	if (enable && (*enable == 'f' || *enable == 'F' ||
		*enable == '0' || *enable == 'n' || *enable == 'N')) {
		return;
	}
	curr_ir_profile->u8Enable = true;
	/**
	 * Parse Protocol
	 */
	protocol = get_key_value(ir_config, "Protocol");
	if (protocol == NULL) {
		DBG_ERR("no Protocol for %s\n", ir_config->name);
		return;
	}
	if (str2ul(protocol, &curr_ir_profile->eIRType)) {
		DBG_ERR("Protocol for %s format error.\n", ir_config->name);
		return;
	}
	DBG_INFO("Protocol value:0x%02x\n", curr_ir_profile->eIRType);

	/**
	 * Parse Header code
	 */
	header = get_key_value(ir_config, "Header");
	if (header == NULL) {
		DBG_ERR("no Header for %s\n", ir_config->name);
		return;
	}
	if (str2ul(header, &curr_ir_profile->u32HeadCode)) {
		DBG_ERR("header code format err for %s\n", ir_config->name);
		return;
	}
	DBG_INFO("Header code:0x%x\n", curr_ir_profile->u32HeadCode);
	/**
	 * Parse IR Speed
	 */
	speed = get_key_value(ir_config, "Speed");
	if (speed == NULL)
		DBG_ERR("Speed for %s is empty. use default.\n", ir_config->name);
	else {
		if (str2ul(speed, &curr_ir_profile->u32IRSpeed)) {
			DBG_ERR("IR Speed format err for %s\n", ir_config->name);
			return;
		}
		DBG_INFO("Speed:0x%x\n", curr_ir_profile->u32IRSpeed);
	}
	/**
	 * Parse Keymap
	 */
	keymap_name = get_key_value(ir_config, "Keymap");
	if (keymap_name == NULL) {
		DBG_ERR("no Keymap key for %s\n", ir_config->name);
		return;
	}
	keymap_sec = get_section(root, keymap_name);
	if (keymap_sec == NULL) {
		DBG_ERR("no Keymap section for %s\n", keymap_name);
		return;
	}
	keymap_size = get_key_num(keymap_sec);
	if (keymap_size == 0) {
		DBG_ERR("no keys in section %s\n", keymap_name);
		return;
	}
	table = kmalloc(sizeof(struct key_map_table) * keymap_size, GFP_KERNEL);
	if (table == NULL) {
		DBG_ERR(KERN_ERR"OOM!\n");
		return;
	}
	memset(table, 0, sizeof(struct key_map_table) * keymap_size);
	key_map_lists[ir_profile_parse_num].map.scan = table;
	for (k = keymap_sec->keys; k != NULL; k = k->next) {
		for (i = 0; i < ARRAY_SIZE(IR_KeysList); i++) {
			if (!strcmp(k->name, IR_KeysList[i].key)) {
				table->keycode = IR_KeysList[i].value;

				if (str2ul(k->value, &table->scancode)) {
					DBG_ERR("scan code format err for %s\n", keymap_sec->name);
					return;
				}
				DBG_ALL("KEY name:%s,KEY val:0x%x,scan code:0x%x\n", k->name,
							table->keycode, table->scancode);
				table++;
				break;
			}
		}
		if (i == ARRAY_SIZE(IR_KeysList)) {
			if (str2ul(k->name, &table->keycode)) {
				DBG_ERR("non-standard key format err for %s\n", k->name);
				return;
			}
			if (str2ul(k->value, &table->scancode)) {
				DBG_ERR("non-standard key's value format err for %s=%s\n",
						k->name, k->value);
				return;
			}
			DBG_ALL("KEY name:%s,KEY val:0x%x,scan code:0x%x\n", k->name,
					table->keycode, table->scancode);
			table++;
		}
	}
	key_map_lists[ir_profile_parse_num].map.headcode = curr_ir_profile->u32HeadCode;
	key_map_lists[ir_profile_parse_num].map.size = keymap_size;
	key_map_lists[ir_profile_parse_num].map.name = kstrdup(keymap_name, GFP_KERNEL);
	DBG_INFO("register config:%s with Header code 0x%x\n",
			ir_config->name, curr_ir_profile->u32HeadCode);
	DBG_INFO("register keymap:%s\n", keymap_name);
	MIRC_IRCustomer_Add(curr_ir_profile);
	MIRC_Map_Register(&key_map_lists[ir_profile_parse_num]);
	ir_profile_parse_num++;
	DBG_INFO("register keymap:%s ------------\n", keymap_name);
}

static void parse_by_pass_kernel_keymap_config(struct _IniSectionNode_List *root,
			const char *name, const char *value)
{
	const char *hw_dec;
	struct by_pass_kernel_keymap_Profile_s *curr_pass_kernel_keymap_profile =
			&by_pass_kernel_keymap_profiles[by_pass_kernel_keymap_parse_num];

	struct _IniSectionNode_List *by_pass_kernel_keymap_config = get_section(root, value);

	if (by_pass_kernel_keymap_parse_num > 1)
		return;
	if (by_pass_kernel_keymap_config == NULL) {
		DBG_ERR("no section named %s\n", value);
		return;
	}

	if (by_pass_kernel_keymap_parse_num > 1)
		return;
	hw_dec = get_key_value(by_pass_kernel_keymap_config, "Hwdec_enable");

	if (hw_dec == NULL) {
		DBG_ERR("no Hwdec_enable for %s\n", by_pass_kernel_keymap_config->name);
		return;
	}

	if (str2ul(hw_dec, &curr_pass_kernel_keymap_profile->eDECType)) {
		DBG_ERR("Hwdec_enable for %s format error.\n", by_pass_kernel_keymap_config->name);
		return;
	}
	DBG_INFO("Hwdec_enable value:0x%02x\n", curr_pass_kernel_keymap_profile->eDECType);
	if (curr_pass_kernel_keymap_profile->eDECType) {
		MIRC_Map_free();
		MIRC_Decoder_free();
		MIRC_Dis_support_ir();
	}
	by_pass_kernel_keymap_parse_num++;
}

#if defined(CONFIG_MTK_PM_TODO_FIX)
static void parse_PM51_all_keymaps(struct _IniSectionNode_List *root,
	const char *name, const char *value)
{
	const char *protocol;
	const char *header;
	const char *power_key;
	struct IR_PM51_Profile_s *curr_ir_PM51_profile =
		&ir_PM51_profiles[ir_PM51_profile_parse_num];
	struct _IniSectionNode_List *ir_PM51_config = get_section(root, value);

	if (ir_PM51_profile_parse_num > (IR_SUPPORT_PM_NUM_MAX - 1))
		return;
	if (ir_PM51_config == NULL) {
		DBG_ERR("no section named %s\n", value);
		return;
	}
	/**
	 * Parse Protocol
	 */
	protocol = get_key_value(ir_PM51_config, "Protocol");
	if (protocol == NULL) {
		DBG_ERR("no Protocol for %s\n", ir_PM51_config->name);
		return;
	}
	if (str2ul(protocol, &curr_ir_PM51_profile->eIRType)) {
		DBG_ERR("Protocol for %s format error.\n", ir_PM51_config->name);
		return;
	}
	DBG_INFO("Protocol value:0x%02x\n", curr_ir_PM51_profile->eIRType);
	/**
	 * Parse Header code
	 */
	header = get_key_value(ir_PM51_config, "Header");
	if (header == NULL) {
		DBG_ERR("no Header for %s\n", ir_PM51_config->name);
		return;
	}
	if (str2ul(header, &curr_ir_PM51_profile->u32Header)) {
		DBG_ERR("header code format err for %s\n", ir_PM51_config->name);
		return;
	}
	DBG_INFO("Header code:0x%x\n", curr_ir_PM51_profile->u32Header);
	/**
	 * Parse PowerKey
	 */
	power_key = get_key_value(ir_PM51_config, "Key");
	if (power_key == NULL) {
		DBG_ERR("no PowerKey for %s\n", ir_PM51_config->name);
		return;
	}
	if (str2ul(power_key, &curr_ir_PM51_profile->u32PowerKey)) {
		DBG_ERR("PowerKey code format err for %s\n", ir_PM51_config->name);
		return;
	}
	DBG_INFO("PowerKey code:0x%x\n", curr_ir_PM51_profile->u32PowerKey);
	if ((curr_ir_PM51_profile->u32Header > 0x0000ffff) || (ir_PM51_profile_parse_num ==
								(IR_SUPPORT_PM_NUM_MAX - 1))) {
		*ir_p32++ = (u8)(curr_ir_PM51_profile->eIRType & 0xff);
		*ir_p32++ = (u8)((curr_ir_PM51_profile->u32Header >> 24) & 0xff);
		*ir_p32++ = (u8)((curr_ir_PM51_profile->u32Header >> 16) & 0xff);
		*ir_p32++ = (u8)((curr_ir_PM51_profile->u32Header >> 8) & 0xff);
		*ir_p32++ = (u8)(curr_ir_PM51_profile->u32Header & 0xff);
		*ir_p32++ = (u8)((curr_ir_PM51_profile->u32PowerKey >> 8) & 0xff);
		*ir_p32++ = (u8)(curr_ir_PM51_profile->u32PowerKey & 0xff);
	} else {
		*ir_p16++ = (u8)(curr_ir_PM51_profile->eIRType & 0xff);
		*ir_p16++ = (u8)((curr_ir_PM51_profile->u32Header >> 8) & 0xff);
		*ir_p16++ = (u8)(curr_ir_PM51_profile->u32Header & 0xff);
		*ir_p16++ = (u8)((curr_ir_PM51_profile->u32PowerKey >> 8) & 0xff);
		*ir_p16++ = (u8)(curr_ir_PM51_profile->u32PowerKey & 0xff);
	}
	ir_PM51_profile_parse_num++;
}
#endif

#if defined(CONFIG_DATA_SEPARATION)
static void getProjectId(char **data)
{
	char *str = NULL;
	char *tmp_command_line = NULL;

	tmp_command_line = kmalloc(strlen(saved_command_line) + 1, GFP_KERNEL);
	memset(tmp_command_line, 0, strlen(saved_command_line) + 1);
	memcpy(tmp_command_line, saved_command_line, strlen(saved_command_line) + 1);
	str = strstr(tmp_command_line, "cusdata_projectid");
	if (str) {
		*data = kmalloc(PROJECT_ID_LEN + 1, GFP_KERNEL);
		memset(*data, 0, PROJECT_ID_LEN + 1);
		char *pTypeStart = strchr(str, '=');

		if (pTypeStart) {
			if (strlen(pTypeStart) > PROJECT_ID_LEN) {
				char *pTypeEnd = strchr(pTypeStart+1, ' ');

				if (!pTypeEnd) {
					kfree(tmp_command_line);
					kfree(*data);
					return;
				}
				*pTypeEnd = '\0';
			}
			memcpy(*data, pTypeStart+1, strlen(pTypeStart));
			pr_notice("%s, projectID = %s\n", __func__, *data);
			str = NULL;
			pTypeStart = NULL;
			pTypeEnd = NULL;
			kfree(tmp_command_line);
			return;
		}
		kfree(*data);
	}
	kfree(tmp_command_line);
	pr_notice("%s, can not get project id from bootargs\n", __func__);
}


static void getDataIndexIniFile(char **data)
{
	char *project_id = NULL;
//When accessing user memory, we need to make sure the entire area really
//is in user-level space.
//KERNEL_DS addr user-level space need less than TASK_SIZE
	mm_segment_t fs = get_fs();

	set_fs(KERNEL_DS);
	*data = kzalloc(DATA_INDEX_NAME_BUF_SIZE+1, GFP_KERNEL);
	if (!(*data)) {
		set_fs(fs);
		kfree(project_id);
		return false;
	}
	getProjectId(&project_id);
	snprintf(*data, DATA_INDEX_NAME_BUF_SIZE, "/cusdata/config/dataIndex/dataIndex_%s.ini",
					project_id);
	kfree(project_id);
	set_fs(fs);
	return true;
}

#endif

extern int MIRC_support_ir_max_timeout(void);


static int __init ir_dynamic_config_init(void)
{
	return 0;
}

static void __exit ir_dynamic_config_exit(void)
{
	int i;

	for (i = 0; i < ir_profile_parse_num; i++) {
		if (key_map_lists != NULL) {
			if (key_map_lists[i].map.scan != NULL) {
				MIRC_Map_UnRegister(&key_map_lists[i]);
				kfree(key_map_lists[i].map.scan);
				key_map_lists[i].map.scan = NULL;
			}
			if (key_map_lists[i].map.name != NULL) {
				DBG_INFO("unregister keymap:%s\n", key_map_lists[i].map.name);
				kfree(key_map_lists[i].map.name);
				key_map_lists[i].map.name = NULL;
			}
		}
	}
/* TODO check
 *	if (key_map_lists) {
 *		kfree(key_map_lists);
 *		key_map_lists = NULL;
 *	}
 */
	if (ir_profiles) {
		for (i = 0; i < ir_profile_parse_num; i++) {
			pr_info("%s,register config with Header code:%x\n"
					, __func__, ir_profiles[i].u32HeadCode);
			MIRC_IRCustomer_Remove(&ir_profiles[i]);
		}
		kfree(ir_profiles);
		ir_profiles = NULL;
	}
}

module_init(ir_dynamic_config_init);
module_exit(ir_dynamic_config_exit);
MODULE_IMPORT_NS(VFS_internal_I_am_really_a_filesystem_and_am_NOT_a_driver);
MODULE_DESCRIPTION("IR dynamic config module for MTK");
MODULE_AUTHOR("Vick Sun <vick.sun@mediatek.com>");
MODULE_LICENSE("GPL v2");
