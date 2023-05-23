// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/firmware.h>
#include <linux/io.h>

#include "mtk_ini_parser.h"

#define FORMAT_MAX	2	/* 9 + \0 */
#define COUNT_MAX	5	/* 9999 + '\0' */
#define IDX_MAX		13	/* index_999 = + '\0' */

static u8 ini_get_format(const struct firmware *fw)
{
	char *buf;
	char fmt_str[] = "IniFormatType = ";
	char temp_str[FORMAT_MAX] = {0};
	char *loc = NULL;
	int pos;
	u8 type = 0;
	int ret;

	buf = (char *)fw->data;

	loc = strstr(buf, fmt_str);
	if (loc) {
		pos = loc - buf;
		pos += strlen(fmt_str);

		strncpy(temp_str, (buf + pos), sizeof(u8));
		temp_str[1] = '\0';

		ret = kstrtou8(temp_str, 10, &type);
		if (ret != 0) {
			pr_err("[INI PARSER]read format type fail\n");
			return -1;
		}
	}

	return type;
}

static int ini_get_count(const struct firmware *fw)
{
	char *buf;
	char cnt_str[] = "count = ";
	char cnt_str_end[] = ";";
	char temp_str[COUNT_MAX] = {0};
	char *loc = NULL, *loc_end = NULL;
	int pos, end_pos = 0, diff = 0, ret;
	u16 count = 0;

	buf = (char *)fw->data;

	loc = strstr(buf, cnt_str);
	if (loc) {
		pos = loc - buf;
		pos += strlen(cnt_str);

		loc_end = strstr(buf + pos, cnt_str_end);
		if (loc_end) {
			end_pos = loc_end - buf;
			diff = end_pos - pos;
			if (diff <= 0) {
				pr_err("[INI PARSER]get count fail\n");
				return -1;
			}
		} else {
			pr_err("[INI PARSER]get cnt_str_end %s fail\n", cnt_str_end);
			return -1;
		}

		strncpy(temp_str, (buf + pos), sizeof(int));
		temp_str[diff] = '\0';

		ret = kstrtou16(temp_str, 10, &count);
		if (ret != 0) {
			pr_err("[INI PARSER]read count value fail\n");
			return -1;
		}
	} else {
		pr_err("[INI PARSER]parse count fail\n");
		return -1;
	}

	return count;
}

static int ini_get_size(const struct firmware *fw, int count)
{
	char *buf;
	char idx_str[IDX_MAX];
	char idx_str_end[] = ",";
	char temp_str[COUNT_MAX] = {0};
	char *loc = NULL, *loc_end = NULL;
	int pos, end_pos = 0, i, diff = 0, ret;
	int total_size = 0;
	u16 size = 0;

	buf = (char *)fw->data;

	for (i = 0; i < count; i++) {
		ret = scnprintf(idx_str, sizeof(idx_str), "index_%d = ", i);
		if (ret < 0) {
			pr_err("[INI PARSER]scnprintf fail\n");
			return -1;
		}

		loc = strstr(buf, idx_str);
		if (loc) {
			pos = loc - buf;
			pos += strlen(idx_str);

			loc_end = strstr(buf + pos, idx_str_end);
			if (loc_end) {
				end_pos = loc_end - buf;
				diff = end_pos - pos;
				if (diff <= 0) {
					pr_err("[INI PARSER]get index fail\n");
					return -1;
				}
			} else {
				pr_err("[INI PARSER]get idx_str_end %s fail\n", idx_str_end);
				return -1;
			}

			strncpy(temp_str, (buf + pos), sizeof(int));
			temp_str[diff] = '\0';

			ret = kstrtou16(temp_str, 10, &size);
			if (ret != 0) {
				pr_err("[INI PARSER]read index fail\n");
				return -1;
			}
			buf += pos;
		} else {
			pr_err("[INI PARSER]parse %s fail\n", idx_str);
			return -1;
		}

		total_size += size;
	}
	total_size += count;	// add address count

	return total_size;
}

int ini_fill_init_table(const struct firmware *fw, u8 *init_table, int count)
{
	char *buf;
	char idx_str[IDX_MAX];
	char idx_str_end[] = ",";
	char value_str[] = "0x";
	char temp_str[COUNT_MAX] = {0};
	char *loc = NULL, *loc_end = NULL;
	int pos, end_pos = 0, i, j, diff = 0, ret;
	int total_size = 0;
	u16 size = 0;
	u16 u16Value = 0;

	buf = (char *)fw->data;

	for (i = 0; i < count; i++) {
		ret = scnprintf(idx_str, sizeof(idx_str), "index_%d = ", i);
		if (ret < 0) {
			pr_err("[INI PARSER]scnprintf fail\n");
			return -1;
		}

		loc = strstr(buf, idx_str);
		if (loc) {
			pos = loc - buf;
			pos += strlen(idx_str);

			loc_end = strstr(buf + pos, idx_str_end);
			if (loc_end) {
				end_pos = loc_end - buf;
				diff = end_pos - pos;
				if (diff <= 0) {
					pr_err("[INI PARSER]get index fail\n");
					return -1;
				}
			} else {
				pr_err("[INI PARSER]get idx_str_end %s fail\n", idx_str_end);
				return -1;
			}

			strncpy(temp_str, (buf + pos), sizeof(int));
			temp_str[diff] = '\0';

			ret = kstrtou16(temp_str, 10, &size);
			if (ret != 0) {
				pr_err("[INI PARSER]read index fail\n");
				return -1;
			}
			buf += pos;
			*init_table = size;
			init_table++;
		} else {
			pr_err("[INI PARSER]parse %s fail\n", idx_str);
			return -1;
		}

		/* get address & value */
		for (j = 0; j < (size + 1); j++) {
			loc = strstr(buf, value_str);
			if (loc) {
				pos = loc - buf;
				pos += strlen(value_str);

				strncpy(temp_str, (buf + pos), sizeof(u16));
				temp_str[2] = '\0';
				ret = kstrtou16(temp_str, 16, &u16Value);
				if (ret != 0) {
					pr_err("[ALSA MTK]read mask fail\n");
					return -1;
				}
				buf += pos;
				*init_table = u16Value;
				init_table++;
			}
		}
	}
	// end flag
	*init_table = 0;

	return 0;
}

u8 *mtk_parser(const struct firmware *fw)
{
	u8 *init_table;
	u8 type;
	int count, size, ret;

	if (fw == NULL) {
		pr_err("[INI PARSER]fw = NULL, request ini not avail\n");
		return NULL;
	}

	type = ini_get_format(fw);
	if (type < 0) {
		pr_err("[INI PARSER]can't get ini format\n");
		return NULL;
	}
	pr_err("[INI PARSER]ini format type %d\n", type);

	count = ini_get_count(fw);
	if (count <= 0) {
		pr_err("[INI PARSER]can't get ini count\n");
		return NULL;
	}

	size = ini_get_size(fw, count);
	if (size <= 0) {
		pr_err("[INI PARSER]can't get ini size\n");
		return NULL;
	}

	init_table = vmalloc(sizeof(u8) * (count + size + 1));
	if (init_table == NULL)
		return NULL;

	ret = ini_fill_init_table(fw, init_table, count);
	if (ret == 0)
		return init_table;

	return NULL;
}

