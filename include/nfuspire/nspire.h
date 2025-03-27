// SPDX-License-Identifier: GPL-3.0-only
/*
 *  nfuspire/include/nfuspire/nspire.h
 *
 *  Copyright (C) Emily <info@emy.sh>
 */

#ifndef NFUSPIRE_NSPIRE_H_
#define NFUSPIRE_NSPIRE_H_

#include <fuse.h>
#include <nspire.h>
#include <pthread.h>

typedef struct nfuspire_ctx {
    nspire_handle_t *handle;
    struct nspire_devinfo devinfo;
    pthread_mutex_t mutex;
} nfuspire_ctx_t;

typedef struct nfuspire_file_cache {
    pthread_mutex_t mutex;
    bool need_sync;
    size_t size;
    unsigned char *data;
} nfuspire_file_cache_t;

#define current_nfuspire_ctx ((nfuspire_ctx_t *)(fuse_get_context()->private_data))

int nfuspire_error(int error);
int nfuspire_readdir(const char *path, void *buf, fuse_fill_dir_t filler);
int nfuspire_getattr(const char *path, struct stat *stbuf);
int nfuspire_mkdir(const char *path);
int nfuspire_rmdir(const char *path);
int nfuspire_rename(const char *src, const char *dst);
int nfuspire_unlink(const char *path);
int nfuspire_create(const char *path, struct fuse_file_info *fi);
int nfuspire_open(const char *path, struct fuse_file_info *fi);
int nfuspire_read(char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int nfuspire_write(const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int nfuspire_fsync(const char *path, struct fuse_file_info *fi);
int nfuspire_release(const char *path, struct fuse_file_info *fi);
int nfuspire_truncate(const char *path, off_t size);

#endif // NFUSPIRE_NSPIRE_H_
