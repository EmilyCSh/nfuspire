// SPDX-License-Identifier: GPL-3.0-only
/*
 *  nfuspire/src/main.c
 *
 *  Copyright (C) Emily <info@emy.sh>
 */

#include <errno.h>
#include <fuse.h>
#include <nfuspire/info.h>
#include <nfuspire/nspire.h>
#include <nfuspire/update.h>
#include <nspire.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define STARTS_WITH(str, prefix) (strncmp((str), (prefix), strlen((prefix))) == 0)

static int fuse_readdir(
    const char *path, void *buf, fuse_fill_dir_t filler, __attribute__((unused)) off_t offset,
    __attribute__((unused)) struct fuse_file_info *fi, __attribute__((unused)) enum fuse_readdir_flags flags
) {
    if (strcmp(path, "/") == 0) {
        filler(buf, ".well-known", NULL, 0, 0);
    } else if (strcmp(path, "/.well-known") == 0) {
        filler(buf, ".", NULL, 0, 0);
        filler(buf, "..", NULL, 0, 0);
        filler(buf, "info", NULL, 0, 0);
        filler(buf, "os_update", NULL, 0, 0);
        return 0;
    } else if (strcmp(path, "/.well-known/info") == 0) {
        return info_readdir(buf, filler);
    }

    return nfuspire_readdir(path, buf, filler);
}

static int fuse_getattr(const char *path, struct stat *stbuf, __attribute__((unused)) struct fuse_file_info *fi) {
    memset(stbuf, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0 || strcmp(path, "/.well-known") == 0 || strcmp(path, "/.well-known/info") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        stbuf->st_uid = getuid();
        stbuf->st_gid = getgid();
        return 0;
    }

    if (strcmp(path, "/.well-known/os_update") == 0) {
        stbuf->st_mode = S_IFREG | 0222;
        stbuf->st_nlink = 1;
        stbuf->st_size = 0;
        stbuf->st_uid = getuid();
        stbuf->st_gid = getgid();
        return 0;
    }

    if (STARTS_WITH(path, "/.well-known/info/")) {
        return info_getattr(path, stbuf);
    }

    return nfuspire_getattr(path, stbuf);
}

static int fuse_mkdir(const char *path, __attribute__((unused)) mode_t mode) {
    if (STARTS_WITH(path, "/.well-known")) {
        return -EINVAL;
    }

    return nfuspire_mkdir(path);
}

static int fuse_rmdir(const char *path) {
    if (STARTS_WITH(path, "/.well-known")) {
        return -EINVAL;
    }

    return nfuspire_rmdir(path);
}

static int fuse_rename(const char *src, const char *dst, __attribute__((unused)) unsigned int flags) {
    if (STARTS_WITH(src, "/.well-known") || STARTS_WITH(dst, "/.well-known")) {
        return -EINVAL;
    }

    return nfuspire_rename(src, dst);
}

static int fuse_unlink(const char *path) {
    if (STARTS_WITH(path, "/.well-known")) {
        return -EINVAL;
    }

    return nfuspire_unlink(path);
}

static int fuse_statfs(__attribute__((unused)) const char *path, struct statvfs *info) {
    struct nspire_devinfo devinfo = current_nfuspire_ctx->devinfo;

    memset(info, 0, sizeof(*info));

    info->f_bsize = info->f_frsize = 1 * 1024;
    info->f_blocks = devinfo.storage.total / info->f_bsize;
    info->f_bfree = info->f_bavail = devinfo.storage.free / info->f_bsize;
    info->f_namemax = sizeof(((struct nspire_dir_item *)0)->name) - 1;

    return 0;
}

static int
fuse_create(const char *path, __attribute__((unused)) mode_t mode, __attribute__((unused)) struct fuse_file_info *fi) {
    if (STARTS_WITH(path, "/.well-known")) {
        return -EINVAL;
    }

    return nfuspire_create(path, fi);
}

int fuse_open(const char *path, struct fuse_file_info *fi) {
    if (strcmp(path, "/.well-known/os_update") == 0) {
        return update_open(fi);
    }

    if (STARTS_WITH(path, "/.well-known")) {
        return 0;
    }

    return nfuspire_open(path, fi);
}

static int fuse_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    if (strcmp(path, "/.well-known/os_update") == 0) {
        return -EINVAL;
    }

    if (STARTS_WITH(path, "/.well-known/info/")) {
        return info_read(path, buf, size, offset);
    }

    return nfuspire_read(buf, size, offset, fi);
}

static int fuse_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    if (strcmp(path, "/.well-known/os_update") == 0) {
        return update_write(buf, size, offset, fi);
    }

    if (STARTS_WITH(path, "/.well-known")) {
        return -EINVAL;
    }

    return nfuspire_write(buf, size, offset, fi);
}

static int fuse_fsync(const char *path, __attribute__((unused)) int isdatasync, struct fuse_file_info *fi) {
    if (strcmp(path, "/.well-known/os_update") == 0) {
        return update_fsync(fi);
    }

    if (STARTS_WITH(path, "/.well-known")) {
        return -EINVAL;
    }

    return nfuspire_fsync(path, fi);
}

static int fuse_release(const char *path, struct fuse_file_info *fi) {
    if (strcmp(path, "/.well-known/os_update") == 0) {
        return update_release(fi);
    }

    if (STARTS_WITH(path, "/.well-known")) {
        return 0;
    }

    return nfuspire_release(path, fi);
}

static int fuse_truncate(const char *path, off_t size, __attribute__((unused)) struct fuse_file_info *fi) {
    if (STARTS_WITH(path, "/.well-known")) {
        return -EINVAL;
    }

    return nfuspire_truncate(path, size);
}

static int fuse_utimens(
    __attribute__((unused)) const char *path, __attribute__((unused)) const struct timespec tv[2],
    __attribute__((unused)) struct fuse_file_info *fi
) {
    return 0;
}

static struct fuse_operations fuse_oper = {
    .getattr = fuse_getattr,
    .readdir = fuse_readdir,
    .mkdir = fuse_mkdir,
    .rmdir = fuse_rmdir,
    .rename = fuse_rename,
    .unlink = fuse_unlink,
    .statfs = fuse_statfs,
    .create = fuse_create,
    .open = fuse_open,
    .read = fuse_read,
    .write = fuse_write,
    .fsync = fuse_fsync,
    .release = fuse_release,
    .truncate = fuse_truncate,
    .utimens = fuse_utimens,
};

int main(int argc, char *argv[]) {
    int rc;
    nfuspire_ctx_t *ctx;

    ctx = malloc(sizeof(nfuspire_ctx_t));
    if (!ctx) {
        perror("Out of memory");
        return -ENOMEM;
    }

    rc = nspire_init(&(ctx->handle));
    if (rc != NSPIRE_ERR_SUCCESS) {
        perror(nspire_strerror(rc));
        return -1;
    }

    rc = nspire_device_info(ctx->handle, &(ctx->devinfo));
    if (rc != NSPIRE_ERR_SUCCESS) {
        perror(nspire_strerror(rc));
        return nfuspire_error(rc);
    }

    pthread_mutex_init(&ctx->mutex, NULL);

    rc = fuse_main(argc, argv, &fuse_oper, ctx);

    nspire_free(ctx->handle);
    free(ctx);

    return rc;
}
