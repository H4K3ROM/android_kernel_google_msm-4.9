/*
 * MNH Firmware for firmware authentication
 *
 * Copyright 2017 Google Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/string.h>
#include "mnh-crypto.h"

#define MISC_DEVICE_NAME "mnh_crypto"

static ssize_t verify_img_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count);

static void mnh_crypto_decode_error(struct device *dev, int err);

/* Device attributes */
DEVICE_ATTR_WO(verify_img);

static struct attribute *mnh_crypto_attrs[] = {
	&dev_attr_verify_img.attr,
	NULL
};
ATTRIBUTE_GROUPS(mnh_crypto);

static struct miscdevice mnh_crypto_miscdevice = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = MISC_DEVICE_NAME,
	.groups = mnh_crypto_groups,
};

/* echo -n "easel/Image">verify_img */
static ssize_t verify_img_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;

	err = mnh_crypto_verify_fw(dev, buf);
	if (!err) {
		dev_info(dev, "%s: Signature verfied OK\n", __func__);
		return count;
	}

	mnh_crypto_decode_error(dev, err);
	return err;
}

/* create sysfs group */
int mnh_crypto_config_sysfs(void)
{
	int err;
	struct device *dev;

	err = misc_register(&mnh_crypto_miscdevice);
	if (err) {
		pr_err("%s: failed to create misc device (%d)\n",
		       __func__, err);
		return err;
	}
	dev = mnh_crypto_miscdevice.this_device;

	return 0;
}
EXPORT_SYMBOL_GPL(mnh_crypto_config_sysfs);

static void mnh_crypto_decode_error(struct device *dev, int err)
{
	/* Decode return code information */
	if (err == -ENOKEY)
		dev_err(dev, "%s: Required key not available\n", __func__);
	else if (err == -EKEYEXPIRED)
		dev_err(dev, "%s: Key has expired\n", __func__);
	else if (err == -EKEYREVOKED)
		dev_err(dev, "%s: Key has been revoked\n", __func__);
	else if (err == -EKEYREJECTED)
		dev_err(dev, "%s: Key was rejected by service\n", __func__);
	else if (err == -ENOMEM)
		dev_err(dev, "%s: Out of memory\n", __func__);
	else if (err == -ECANCELED)
		dev_err(dev, "%s: Operation canceled\n", __func__);
	else
		dev_err(dev, "%s: Unknown error code %d\n", __func__, err);
}
