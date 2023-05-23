// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/sync_file.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/anon_inodes.h>
#include "mdw_usr.h"
#include "mdw_cmd.h"
#include "mdw_cmn.h"

extern struct mdw_usr_mgr u_mgr;
struct apu_poll_desc {
	struct mdw_apu_cmd *c;
	struct mdw_usr *u;
};

static int apu_file_release(struct inode *inode, struct file *file)
{
	struct apu_poll_desc *d = file->private_data;

	if (d) {
		file->private_data = NULL;
		kfree(d);
	}
	return 0;
}

static unsigned int apu_file_poll(struct file *file, poll_table *wait)
{
	struct apu_poll_desc *d = file->private_data;
	struct mdw_usr *u;
	struct mdw_apu_cmd *c;
	struct list_head *tmp = NULL, *list_ptr = NULL;

	if (d == NULL)
		return POLLIN;

	mutex_lock(&u_mgr.mtx);

	/* Check user */
	list_for_each_safe(list_ptr, tmp, &u_mgr.list) {
		u = list_entry(list_ptr, struct mdw_usr, m_item);
		mdw_flw_debug("poll usr(0x%llx/0x%llx) matching...\n", u, d->u);
		if (u == d->u)
			break;
		u = NULL;
	}


	if (u == NULL)
		goto out;

	/* Check cmd */
	mutex_lock(&u->mtx);
	list_for_each_safe(list_ptr, tmp, &u->cmd_list) {
		c = list_entry(list_ptr, struct mdw_apu_cmd, u_item);
		mdw_flw_debug("poll cmd(0x%llx/0x%llx) matching...\n", c, d->c);

		if (c == d->c)
			break;
		c = NULL;
	}
	mutex_unlock(&u->mtx);

	if (c == NULL)
		goto out;

	mdw_wait_cmd(d->c);
out:
	mutex_unlock(&u_mgr.mtx);
	return POLLIN;
}

static const struct file_operations apu_sync_file_fops = {
	.release = apu_file_release,
	.poll = apu_file_poll,
};

int apu_sync_file_create(struct mdw_apu_cmd *c)
{
	struct apu_poll_desc *desc;
	int ret = 0;
	int fd = get_unused_fd_flags(O_CLOEXEC);

	if (fd < 0)
		return -EINVAL;

	desc = kzalloc(sizeof(struct apu_poll_desc), GFP_KERNEL);
	desc->c = c;
	desc->u = c->usr;
	c->file = anon_inode_getfile("apu_file", &apu_sync_file_fops, desc, 0);


	if (c->file == NULL) {
		put_unused_fd(fd);
		ret = -EINVAL;
	} else {
		fd_install(fd, c->file);
		c->uf_hdr->fd = fd;
	}
	return 0;
}
