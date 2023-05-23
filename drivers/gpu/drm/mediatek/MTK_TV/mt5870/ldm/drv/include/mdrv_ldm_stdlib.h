/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#ifndef __MDRV_LDM_STDLIB_H__
#define __MDRV_LDM_STDLIB_H__
#include <linux/fs.h>

extern int atoi(const char *str);
extern bool is_path_exist(const char *path);
extern long read(struct file *readFilp, char *buf, size_t count);
extern long write(unsigned int fd, const char *buf, size_t count);
//extern long open(const char *filename, int flags);
extern struct file *open(const char *filename, int flags);
extern off_t lseek(struct file *pstFilp, off_t offset, unsigned int whence);
extern void close(struct file *pstFilp);

extern long ioctl(unsigned int fd, unsigned int cmd, unsigned long arg);
extern char *strerror(int err);
extern long get_errno(void);
#define errno	get_errno()
#endif
