// SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause)
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
#include <linux/string.h>
#include <linux/namei.h>
#include <linux/syscalls.h>
#include <linux/file.h>
#include <linux/version.h>
//#include <linux/fs.h>
#include <linux/err.h>
#include <linux/version.h>

MODULE_IMPORT_NS(VFS_internal_I_am_really_a_filesystem_and_am_NOT_a_driver);

static long errno;


long get_errno(void)
{
	return errno;
}
int atoi(const char *str)
{
	int i = 0;

	if (kstrtoint(str, 10, &i) != 0)
		return 0;

	return i;
}
bool is_path_exist(const char *path)
{
	struct path stPath;
	int s32Error;

	if (!path || !*path)
		return false;

	s32Error = kern_path(path, LOOKUP_FOLLOW, &stPath);
	if (s32Error)
		return false;

	path_put(&stPath);
	return true;
}

long read(struct file *readFilp, char *buf, size_t count)
{
	mm_segment_t stFs = get_fs();
//	struct file *pstFilp;
	long cnt;

	set_fs(KERNEL_DS);
	errno = 0;
//	pstFilp = fget(fd);
	cnt = kernel_read(readFilp, buf, count, &readFilp->f_pos);
	fput(readFilp);
	set_fs(stFs);
	if (cnt < 0) {
		/*printk(KERN_INFO"read file fail:%d:%ld\n", fd, cnt);*/
		errno = cnt;
		cnt = -1;
	}
	return cnt;
}
long write(unsigned int fd, const char *buf, size_t count)
{
	mm_segment_t stFs = get_fs();
	struct file *pstFilp;
	long cnt;

	set_fs(KERNEL_DS);
	errno = 0;
	pstFilp = fget(fd);
	cnt = kernel_write(pstFilp, buf, count, &pstFilp->f_pos);
	fput(pstFilp);
	set_fs(stFs);
	if (cnt < 0) {
		/*printk(KERN_INFO"write file fail:%d:%ld\n", fd, cnt);*/
		errno = cnt;
		cnt = -1;
	}
	return cnt;
}

struct file *open(const char *filename, int flags)
{
	struct file *openFilp = NULL;
	mm_segment_t stFs;

	errno = 0;
	if (!is_path_exist(filename)) {
		errno = -ENOENT;
		return -1;
	}
	stFs = get_fs();
	set_fs(KERNEL_DS);

	openFilp = filp_open(filename, flags, 0);

	if (IS_ERR(openFilp)) {
		errno = -1;
		return NULL;
	}

	set_fs(stFs);
	return openFilp;
}
off_t lseek(struct file *pstFilp, off_t offset, unsigned int whence)
{
	mm_segment_t stFs = get_fs();
	long ret;

	set_fs(KERNEL_DS);
	errno = 0;
	ret = pstFilp->f_op->llseek(pstFilp, offset, whence);
	fput(pstFilp);
	set_fs(stFs);
	if (ret < 0) {
		/*printk(KERN_INFO"lseek file fail:%d:%ld\n", fd, ret);*/
		errno = ret;
		ret = -1;
	}
	return ret;
}

void close(struct file *pstFilp)
{
	filp_close(pstFilp, NULL);

}

long ioctl(unsigned int fd, unsigned int cmd, unsigned long arg)
{
	struct file *pstFilp = fget(fd);
	mm_segment_t stFs = get_fs();
	long ret;

	set_fs(KERNEL_DS);
	errno = 0;

	ret = vfs_ioctl(pstFilp, cmd, arg);

	set_fs(stFs);
	fput(pstFilp);

	if (ret < 0) {
		errno = ret;
		ret = -1;
	}
	return ret;
}
char *strerror(int err)
{
	static char buf[32];

	snprintf(buf, 32, "kernel errno:%d\n", err);
	return buf;
}
