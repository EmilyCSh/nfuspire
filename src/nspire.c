// SPDX-License-Identifier: GPL-3.0-only
/*
 *  nfuspire/src/nspire.c
 *
 *  Copyright (C) Emily <info@emy.sh>
 */

#include <errno.h>
#include <nfuspire/nspire.h>
#include <nspire.h>
#include <stdlib.h>
#include <unistd.h>

int nfuspire_error(int error) {
    switch (error) {
        case NSPIRE_ERR_SUCCESS:
            return 0;
        case -NSPIRE_ERR_TIMEOUT:
            return -ETIMEDOUT;
        case -NSPIRE_ERR_NOMEM:
            return -ENOMEM;
        case -NSPIRE_ERR_INVALID:
            return -EPERM;
        case -NSPIRE_ERR_LIBUSB:
            return -EIO;
        case -NSPIRE_ERR_NODEVICE:
            return -ENODEV;
        case -NSPIRE_ERR_INVALPKT:
            return -EIO;
        case -NSPIRE_ERR_NACK:
            return -EIO;
        case -NSPIRE_ERR_BUSY:
            return -EBUSY;
        case -NSPIRE_ERR_EXISTS:
            return -EEXIST;
        case -NSPIRE_ERR_NONEXIST:
            return -ENOENT;
        case -NSPIRE_ERR_OSFAILED:
            return -EPERM;
        default:
            return -EINVAL;
    }
}

int nfuspire_readdir(const char *path, void *buf, fuse_fill_dir_t filler) {
    int rc;
    struct nspire_dir_info *list;

    pthread_mutex_lock(&current_nfuspire_ctx->mutex);

    rc = nspire_dirlist(current_nfuspire_ctx->handle, path, &list);
    if (rc) {
        rc = nfuspire_error(rc);
        goto exit;
    }

    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);

    for (long unsigned int i = 0; i < list->num; i++) {
        filler(buf, list->items[i].name, NULL, 0, 0);
    }

    nspire_dirlist_free(list);
    rc = 0;

exit:
    pthread_mutex_unlock(&current_nfuspire_ctx->mutex);
    return rc;
}

int nfuspire_getattr(const char *path, struct stat *stbuf) {
    int rc;
    struct nspire_dir_item item;

    pthread_mutex_lock(&current_nfuspire_ctx->mutex);

    rc = nspire_attr(current_nfuspire_ctx->handle, path, &item);
    if (rc) {
        rc = nfuspire_error(rc);
        goto exit;
    }

    stbuf->st_size = item.size;
    stbuf->st_nlink = 1;
    stbuf->st_mode = (item.type == NSPIRE_DIR ? S_IFDIR : S_IFREG) | 0755;
    stbuf->st_mtime = stbuf->st_atime = stbuf->st_ctime = item.date;

    stbuf->st_uid = getuid();
    stbuf->st_gid = getgid();

    rc = 0;

exit:
    pthread_mutex_unlock(&current_nfuspire_ctx->mutex);
    return rc;
}

int nfuspire_mkdir(const char *path) {
    int rc;

    pthread_mutex_lock(&current_nfuspire_ctx->mutex);

    rc = nspire_dir_create(current_nfuspire_ctx->handle, path);

    pthread_mutex_unlock(&current_nfuspire_ctx->mutex);
    return nfuspire_error(rc);
}

int nfuspire_rmdir(const char *path) {
    int rc;

    pthread_mutex_lock(&current_nfuspire_ctx->mutex);

    rc = nspire_dir_delete(current_nfuspire_ctx->handle, path);

    pthread_mutex_unlock(&current_nfuspire_ctx->mutex);
    return nfuspire_error(rc);
}

int nfuspire_rename(const char *src, const char *dst) {
    int rc;

    pthread_mutex_lock(&current_nfuspire_ctx->mutex);

    rc = nspire_file_rename(current_nfuspire_ctx->handle, src, dst);

    pthread_mutex_unlock(&current_nfuspire_ctx->mutex);
    return nfuspire_error(rc);
}

int nfuspire_unlink(const char *path) {
    int rc;

    pthread_mutex_lock(&current_nfuspire_ctx->mutex);

    rc = nspire_file_delete(current_nfuspire_ctx->handle, path);

    pthread_mutex_unlock(&current_nfuspire_ctx->mutex);
    return nfuspire_error(rc);
}

int nfuspire_create(const char *path, struct fuse_file_info *fi) {
    int rc;
    struct nspire_dir_item item;

    pthread_mutex_lock(&current_nfuspire_ctx->mutex);

    rc = nspire_attr(current_nfuspire_ctx->handle, path, &item);
    if (rc) {
        if (rc == -NSPIRE_ERR_NONEXIST) {
            rc = nspire_file_write(current_nfuspire_ctx->handle, path, nullptr, 0);
            if (rc) {
                rc = nfuspire_error(rc);
                goto exit;
            }
        } else {
            rc = nfuspire_error(rc);
            goto exit;
        }
    }

    pthread_mutex_unlock(&current_nfuspire_ctx->mutex);
    return nfuspire_open(path, fi);
exit:
    pthread_mutex_unlock(&current_nfuspire_ctx->mutex);
    return rc;
}

int nfuspire_open(const char *path, struct fuse_file_info *fi) {
    int rc;
    nfuspire_file_cache_t *cache;
    struct nspire_dir_item item;

    pthread_mutex_lock(&current_nfuspire_ctx->mutex);

    rc = nspire_attr(current_nfuspire_ctx->handle, path, &item);
    if (rc) {
        rc = nfuspire_error(rc);
        goto exit;
    }

    cache = calloc(1, sizeof(nfuspire_file_cache_t));
    if (!cache) {
        rc = -ENOMEM;
        goto exit;
    }

    cache->data = malloc(item.size);
    if (!cache->data) {
        rc = -ENOMEM;
        goto exit_free;
    }

    pthread_mutex_init(&cache->mutex, NULL);
    cache->size = item.size;
    cache->need_sync = false;

    if (cache->size) {
        rc = nspire_file_read(current_nfuspire_ctx->handle, path, cache->data, cache->size, &cache->size);
        if (rc) {
            rc = nfuspire_error(rc);
            goto exit_free;
        }
    }

    fi->fh = (typeof(fi->fh))cache;
    rc = 0;
    goto exit;

exit_free:
    if (cache->data) {
        free(cache->data);
    }

    if (cache) {
        free(cache);
    }
exit:
    pthread_mutex_unlock(&current_nfuspire_ctx->mutex);
    return rc;
}

int nfuspire_read(char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    nfuspire_file_cache_t *cache = (nfuspire_file_cache_t *)(fi->fh);

    if (!cache) {
        return -EINVAL;
    }

    pthread_mutex_lock(&cache->mutex);

    memset(buf, 0, size);

    if (offset + size > cache->size) {
        size = cache->size - offset;
    }

    memcpy(buf, cache->data + offset, size);

    pthread_mutex_unlock(&cache->mutex);
    return size;
}

int nfuspire_write(const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
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

int nfuspire_fsync(const char *path, struct fuse_file_info *fi) {
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

    rc = nspire_file_write(current_nfuspire_ctx->handle, path, cache->data, cache->size);
    if (!rc) {
        cache->need_sync = false;
    }

    rc = nfuspire_error(rc);

    pthread_mutex_unlock(&cache->mutex);
    pthread_mutex_unlock(&current_nfuspire_ctx->mutex);
    return rc;
}

int nfuspire_release(const char *path, struct fuse_file_info *fi) {
    int rc;
    nfuspire_file_cache_t *cache = (nfuspire_file_cache_t *)(fi->fh);

    if (!cache) {
        return -EINVAL;
    }

    rc = nfuspire_fsync(path, fi);

    if (cache->data) {
        free(cache->data);
    }

    free(cache);
    fi->fh = 0;

    return rc;
}

int nfuspire_truncate(const char *path, off_t size) {
    int rc;
    void *data = nullptr;
    struct nspire_dir_item item;

    pthread_mutex_lock(&current_nfuspire_ctx->mutex);

    if (!size) {
        rc = nspire_file_write(current_nfuspire_ctx->handle, path, nullptr, 0);
        rc = nfuspire_error(rc);
        goto exit;
    }

    rc = nspire_attr(current_nfuspire_ctx->handle, path, &item);
    if (rc) {
        rc = nfuspire_error(rc);
        goto exit;
    }

    data = malloc(size);
    if (!data) {
        rc = -ENOMEM;
        goto exit;
    }

    memset(data, 0, size);

    rc = nspire_file_read(current_nfuspire_ctx->handle, path, data, size, nullptr);
    if (rc) {
        rc = nfuspire_error(rc);
        goto exit;
    }

    rc = nspire_file_write(current_nfuspire_ctx->handle, path, data, size);
    if (rc) {
        rc = nfuspire_error(rc);
        goto exit;
    }

    rc = 0;

exit:
    if (data) {
        free(data);
    }

    pthread_mutex_unlock(&current_nfuspire_ctx->mutex);
    return rc;
}
