// SPDX-License-Identifier: GPL-3.0-only
/*
 *  nfuspire/include/nfuspire/update.h
 *
 *  Copyright (C) Emily <info@emy.sh>
 */

#ifndef NFUSPIRE_UPDATE_H_
#define NFUSPIRE_UPDATE_H_

#include <fuse.h>
#include <nfuspire/nspire.h>

int update_open(struct fuse_file_info *fi);
int update_write(const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int update_fsync(struct fuse_file_info *fi);
int update_release(struct fuse_file_info *fi);

#endif // NFUSPIRE_UPDATE_H_
