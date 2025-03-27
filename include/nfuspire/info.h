// SPDX-License-Identifier: GPL-3.0-only
/*
 *  nfuspire/include/nfuspire/info.h
 *
 *  Copyright (C) Emily <info@emy.sh>
 */

#ifndef NFUSPIRE_INFO_H_
#define NFUSPIRE_INFO_H_

int info_readdir(void *buf, fuse_fill_dir_t filler);
int info_getattr(const char *path, struct stat *stbuf);
int info_read(const char *path, char *buf, size_t size, off_t offset);

#endif // NFUSPIRE_INFO_H_
