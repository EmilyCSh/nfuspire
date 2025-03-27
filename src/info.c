// SPDX-License-Identifier: GPL-3.0-only
/*
 *  nfuspire/src/info.c
 *
 *  Copyright (C) Emily <info@emy.sh>
 */

#include <errno.h>
#include <fuse.h>
#include <limits.h>
#include <nfuspire/info.h>
#include <nfuspire/nspire.h>
#include <nspire.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static const char *info_files[] = {
    "storage_total",
    "storage_free",
    "ram_total",
    "ram_free",
    "version_os_major",
    "version_os_minor",
    "version_os_build",
    "version_os",
    "version_boot1_major",
    "version_boot1_minor",
    "version_boot1_build",
    "version_boot1",
    "version_boot2_major",
    "version_boot2_minor",
    "version_boot2_build",
    "version_boot2",
    "hw_type",
    "batt_status",
    "batt_is_charging",
    "clock_speed",
    "lcd_width",
    "lcd_height",
    "lcd_bbp",
    "lcd_sample_mode",
    "extensions_file",
    "extensions_os",
    "device_name",
    "electronic_id",
    "runlevel"
};

int info_readdir(void *buf, fuse_fill_dir_t filler) {
    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);

    for (size_t i = 0; i < sizeof(info_files) / sizeof(info_files[0]); i++) {
        filler(buf, info_files[i], NULL, 0, 0);
    }

    return 0;
}

int info_getattr(const char *path, struct stat *stbuf) {
    for (size_t i = 0; i < sizeof(info_files) / sizeof(info_files[0]); i++) {
        char fullpath[PATH_MAX];
        snprintf(fullpath, sizeof(fullpath), "/.well-known/info/%s", info_files[i]);

        if (strcmp(path, fullpath) == 0) {
            stbuf->st_mode = S_IFREG | 0444;
            stbuf->st_nlink = 1;
            stbuf->st_size = 1024;
            stbuf->st_uid = getuid();
            stbuf->st_gid = getgid();
            return 0;
        }
    }

    return -ENOENT;
}

int info_read(const char *path, char *buf, size_t size, off_t offset) {
    struct nspire_devinfo devinfo = current_nfuspire_ctx->devinfo;
    char data[1024];

    if (strcmp(path, "/.well-known/info/storage_total") == 0) {
        snprintf(data, sizeof(data), "%lu\n", devinfo.storage.total);
    } else if (strcmp(path, "/.well-known/info/storage_free") == 0) {
        snprintf(data, sizeof(data), "%lu\n", devinfo.storage.free);
    } else if (strcmp(path, "/.well-known/info/ram_total") == 0) {
        snprintf(data, sizeof(data), "%lu\n", devinfo.ram.total);
    } else if (strcmp(path, "/.well-known/info/ram_free") == 0) {
        snprintf(data, sizeof(data), "%lu\n", devinfo.ram.free);
    } else if (strcmp(path, "/.well-known/info/version_os_major") == 0) {
        snprintf(data, sizeof(data), "%d\n", devinfo.versions[NSPIRE_VER_OS].major);
    } else if (strcmp(path, "/.well-known/info/version_os_minor") == 0) {
        snprintf(data, sizeof(data), "%d\n", devinfo.versions[NSPIRE_VER_OS].minor);
    } else if (strcmp(path, "/.well-known/info/version_os_build") == 0) {
        snprintf(data, sizeof(data), "%d\n", devinfo.versions[NSPIRE_VER_OS].build);
    } else if (strcmp(path, "/.well-known/info/version_os") == 0) {
        snprintf(
            data, sizeof(data), "%d.%d.%d\n", devinfo.versions[NSPIRE_VER_OS].major,
            devinfo.versions[NSPIRE_VER_OS].minor, devinfo.versions[NSPIRE_VER_OS].build
        );
    } else if (strcmp(path, "/.well-known/info/version_boot1_major") == 0) {
        snprintf(data, sizeof(data), "%d\n", devinfo.versions[NSPIRE_VER_BOOT1].major);
    } else if (strcmp(path, "/.well-known/info/version_boot1_minor") == 0) {
        snprintf(data, sizeof(data), "%d\n", devinfo.versions[NSPIRE_VER_BOOT1].minor);
    } else if (strcmp(path, "/.well-known/info/version_boot1_build") == 0) {
        snprintf(data, sizeof(data), "%d\n", devinfo.versions[NSPIRE_VER_BOOT1].build);
    } else if (strcmp(path, "/.well-known/info/version_boot1") == 0) {
        snprintf(
            data, sizeof(data), "%d.%d.%d\n", devinfo.versions[NSPIRE_VER_BOOT1].major,
            devinfo.versions[NSPIRE_VER_BOOT1].minor, devinfo.versions[NSPIRE_VER_BOOT1].build
        );
    } else if (strcmp(path, "/.well-known/info/version_boot2_major") == 0) {
        snprintf(data, sizeof(data), "%d\n", devinfo.versions[NSPIRE_VER_BOOT2].major);
    } else if (strcmp(path, "/.well-known/info/version_boot2_minor") == 0) {
        snprintf(data, sizeof(data), "%d\n", devinfo.versions[NSPIRE_VER_BOOT2].minor);
    } else if (strcmp(path, "/.well-known/info/version_boot2_build") == 0) {
        snprintf(data, sizeof(data), "%d\n", devinfo.versions[NSPIRE_VER_BOOT2].build);
    } else if (strcmp(path, "/.well-known/info/version_boot2") == 0) {
        snprintf(
            data, sizeof(data), "%d.%d.%d\n", devinfo.versions[NSPIRE_VER_BOOT2].major,
            devinfo.versions[NSPIRE_VER_BOOT2].minor, devinfo.versions[NSPIRE_VER_BOOT2].build
        );
    } else if (strcmp(path, "/.well-known/info/hw_type") == 0) {
        if (devinfo.hw_type == NSPIRE_CAS) {
            strcpy(data, "cas");
        } else if (devinfo.hw_type == NSPIRE_NONCAS) {
            strcpy(data, "noncas");
        } else if (devinfo.hw_type == NSPIRE_CASCX) {
            strcpy(data, "cascx");
        } else if (devinfo.hw_type == NSPIRE_NONCASCX) {
            strcpy(data, "noncascx");
        } else if (devinfo.hw_type == 0x1C) {
            strcpy(data, "cascx2");
        } else if (devinfo.hw_type == 0x2C) {
            strcpy(data, "noncascx2");
        } else {
            strcpy(data, "unknown");
        }
    } else if (strcmp(path, "/.well-known/info/batt_status") == 0) {
        if (devinfo.batt.status == NSPIRE_BATT_POWERED) {
            strcpy(data, "powered");
        } else if (devinfo.batt.status == NSPIRE_BATT_LOW) {
            strcpy(data, "low");
        } else if (devinfo.batt.status == NSPIRE_BATT_OK) {
            strcpy(data, "ok");
        } else {
            strcpy(data, "unknown");
        }
    } else if (strcmp(path, "/.well-known/info/batt_is_charging") == 0) {
        snprintf(data, sizeof(data), "%u\n", devinfo.batt.is_charging);
    } else if (strcmp(path, "/.well-known/info/clock_speed") == 0) {
        snprintf(data, sizeof(data), "%u\n", devinfo.clock_speed);
    } else if (strcmp(path, "/.well-known/info/lcd_width") == 0) {
        snprintf(data, sizeof(data), "%u\n", devinfo.lcd.width);
    } else if (strcmp(path, "/.well-known/info/lcd_height") == 0) {
        snprintf(data, sizeof(data), "%u\n", devinfo.lcd.height);
    } else if (strcmp(path, "/.well-known/info/lcd_bbp") == 0) {
        snprintf(data, sizeof(data), "%u\n", devinfo.lcd.bbp);
    } else if (strcmp(path, "/.well-known/info/lcd_sample_mode") == 0) {
        snprintf(data, sizeof(data), "%u\n", devinfo.lcd.sample_mode);
    } else if (strcmp(path, "/.well-known/info/extensions_file") == 0) {
        strcpy(data, devinfo.extensions.file);
    } else if (strcmp(path, "/.well-known/info/extensions_os") == 0) {
        strcpy(data, devinfo.extensions.os);
    } else if (strcmp(path, "/.well-known/info/device_name") == 0) {
        strcpy(data, devinfo.device_name);
    } else if (strcmp(path, "/.well-known/info/electronic_id") == 0) {
        strcpy(data, devinfo.electronic_id);
    } else if (strcmp(path, "/.well-known/info/runlevel") == 0) {
        if (devinfo.runlevel == NSPIRE_RUNLEVEL_RECOVERY) {
            strcpy(data, "recovery");
        } else if (devinfo.runlevel == NSPIRE_RUNLEVEL_OS) {
            strcpy(data, "os");
        } else {
            strcpy(data, "unknown");
        }
    } else {
        return -ENOENT;
    }

    size_t len = strlen(data);

    if (offset < 0 || (size_t)offset >= len)
        return 0;

    if (offset + size > len)
        size = len - offset;

    memcpy(buf, data + offset, size);
    return size;
}
