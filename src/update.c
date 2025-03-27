// SPDX-License-Identifier: GPL-3.0-only
/*
 *  nfuspire/src/update.c
 *
 *  Copyright (C) Emily <info@emy.sh>
 */

#include <errno.h>
#include <nfuspire/nspire.h>
#include <nspire.h>
#include <stdio.h>
#include <stdlib.h>

int update_open(struct fuse_file_info *fi) {
    nfuspire_file_cache_t *cache;

    cache = calloc(1, sizeof(nfuspire_file_cache_t));
    if (!cache) {
        return -ENOMEM;
    }

    pthread_mutex_init(&cache->mutex, NULL);
    cache->size = 0;
    cache->need_sync = false;

    fi->fh = (typeof(fi->fh))cache;
    return 0;
}

int update_write(const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    int rc;
    nfuspire_file_cache_t *cache = (nfuspire_file_cache_t *)(fi->fh);
    unsigned char *new_cache_data;

    if (!cache) {
        return -EINVAL;
    }

    pthread_mutex_lock(&cache->mutex);

    if (offset + size > cache->size) {
        new_cache_data = realloc(cache->data, offset + size);
        if (!new_cache_data) {
            rc = -ENOMEM;
            goto exit;
        }

        cache->data = new_cache_data;
        cache->size = offset + size;
    }

    memcpy(cache->data + offset, buf, size);
    cache->need_sync = true;
    rc = size;

exit:
    pthread_mutex_unlock(&cache->mutex);
    return rc;
}

int update_fsync(struct fuse_file_info *fi) {
    int rc;
    nfuspire_file_cache_t *cache = (nfuspire_file_cache_t *)(fi->fh);

    if (!cache) {
        return -EINVAL;
    }

    if (!cache->need_sync) {
        return 0;
    }

    pthread_mutex_lock(&cache->mutex);
    pthread_mutex_lock(&current_nfuspire_ctx->mutex);

    rc = nspire_os_send(current_nfuspire_ctx->handle, cache->data, cache->size);

    cache->size = 0;
    cache->need_sync = false;
    rc = nfuspire_error(rc);

    pthread_mutex_unlock(&cache->mutex);
    pthread_mutex_unlock(&current_nfuspire_ctx->mutex);
    return rc;
}

int update_release(struct fuse_file_info *fi) {
    int rc;
    nfuspire_file_cache_t *cache = (nfuspire_file_cache_t *)(fi->fh);

    if (!cache) {
        return -EINVAL;
    }

    rc = update_fsync(fi);

    if (cache->data) {
        free(cache->data);
    }

    free(cache);
    fi->fh = 0;

    return rc;
}
