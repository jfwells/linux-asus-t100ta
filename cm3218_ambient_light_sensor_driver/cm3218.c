/*
 * A iio driver for CM3218 Ambient Light Sensor.
 *
 * IIO driver for monitoring ambient light intensity in lux.
 *
 * Copyright (c) 2013, Capella Microsystems Inc.
 *  Modified by John Wells <jfw@jfwhome.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA	02110-1301, USA.
 */

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>

#include <linux/acpi.h>
#include <acpi/acpi_bus.h>
#include <acpi/acpi_drivers.h>

/* SMBus ARA Address */
#define	CM3218_ADDR_ARA			0x0C

/* CM3218 CMD Registers */
#define	CM3218_REG_ADDR_CMD		0x00
#define	CM3218_CMD_ALS_SD		0x0001
#define	CM3218_CMD_ALS_INT_EN		0x0002
#define	CM3218_CMD_ALS_IT_SHIFT		6
#define	CM3218_CMD_ALS_IT_MASK		(3 << CM3218_CMD_ALS_IT_SHIFT)
#define	CM3218_CMD_ALS_IT_05T		(0 << CM3218_CMD_ALS_IT_SHIFT)
#define	CM3218_CMD_ALS_IT_1T		(1 << CM3218_CMD_ALS_IT_SHIFT)
#define	CM3218_CMD_ALS_IT_2T		(2 << CM3218_CMD_ALS_IT_SHIFT)
#define	CM3218_CMD_ALS_IT_4T		(3 << CM3218_CMD_ALS_IT_SHIFT)
#define	CM3218_DEFAULT_CMD		(CM3218_CMD_ALS_IT_1T)

#define	CM3218_REG_ADDR_ALS_WH		0x01
#define	CM3218_DEFAULT_ALS_WH		0xFFFF

#define	CM3218_REG_ADDR_ALS_WL		0x02
#define	CM3218_DEFAULT_ALS_WL		0x0000

#define	CM3218_REG_ADDR_ALS		0x04

#define	CM3218_REG_ADDR_STATUS		0x06

#define	CM3218_REG_ADDR_ID		0x07

/* Software parameters */
#define	CM3218_MAX_CACHE_REGS		(0x03+1)	/* Reg.0x00 to 0x03 */

static const unsigned short normal_i2c[] = {
	0x10, 0x48, I2C_CLIENT_END };

struct cm3218_chip {
	struct i2c_client	*client;
	struct mutex		lock;
	unsigned int		lensfactor;
	bool			suspended;
	u16			reg_cache[CM3218_MAX_CACHE_REGS];
};

static int cm3218_read_ara(struct i2c_client *client)
{
	int status;
	unsigned short addr;

	addr = client->addr;
	client->addr = CM3218_ADDR_ARA;
	status = i2c_smbus_read_byte(client);
	client->addr = addr;

	if (status < 0)
		return -ENODEV;

	return 0;
}

static int cm3218_write(struct i2c_client *client, u8 reg, u16 value)
{
	u16 regval;
	int ret;
	struct cm3218_chip *chip = iio_priv(i2c_get_clientdata(client));

	dev_dbg(&client->dev,
		"Write to device register 0x%02X with 0x%04X\n", reg, value);
	regval = cpu_to_le16(value);
	ret = i2c_smbus_write_word_data(client, reg, regval);
	if (ret) {
		dev_err(&client->dev, "Write to device fails: 0x%x\n", ret);
	} else {
		/* Update cache */
		if (reg < CM3218_MAX_CACHE_REGS)
			chip->reg_cache[reg] = value;
	}

	return ret;
}

static int cm3218_read(struct i2c_client *client, u8 reg)
{
	int regval;
	int status;
	struct cm3218_chip *chip = iio_priv(i2c_get_clientdata(client));

	if (reg < CM3218_MAX_CACHE_REGS) {
		regval = chip->reg_cache[reg];
	} else {
		status = i2c_smbus_read_word_data(client, reg);
		if (status < 0) {
			dev_err(&client->dev,
				"Error in reading Reg.0x%02X\n", reg);
			return status;
		}
		regval = le16_to_cpu(status);

		dev_dbg(&client->dev,
			"Read from device register 0x%02X = 0x%04X\n",
			reg, regval);
	}

	return regval;
}


static int cm3218_read_lux(struct i2c_client *client, int *lux)
{
	struct cm3218_chip *chip = iio_priv(i2c_get_clientdata(client));
	int lux_data;

	lux_data = cm3218_read(client, CM3218_REG_ADDR_ALS);
	if (lux_data < 0) {
		dev_err(&client->dev, "Error in reading Lux DATA\n");
		return lux_data;
	}

	dev_vdbg(&client->dev, "lux = %u\n", lux_data);

	if (lux_data < 0)
		return lux_data;

	*lux  = lux_data * chip->lensfactor;
	*lux /= 1000;
	return 0;
}


/* Channel IO */
static int cm3218_write_raw(struct iio_dev *indio_dev,
			      struct iio_chan_spec const *chan,
			      int val,
			      int val2,
			      long mask)
{
	struct cm3218_chip *chip = iio_priv(indio_dev);
	int ret = -EINVAL;

	mutex_lock(&chip->lock);
	if (mask == IIO_CHAN_INFO_CALIBSCALE && chan->type == IIO_LIGHT) {
		chip->lensfactor = val;
		ret = 0;
	}
	mutex_unlock(&chip->lock);

	return 0;
}

static int cm3218_read_raw(struct iio_dev *indio_dev,
			     struct iio_chan_spec const *chan,
			     int *val,
			     int *val2,
			     long mask)
{
	int ret = -EINVAL;
	struct cm3218_chip *chip = iio_priv(indio_dev);
	struct i2c_client *client = chip->client;

	mutex_lock(&chip->lock);
	if (chip->suspended) {
		mutex_unlock(&chip->lock);
		return -EBUSY;
	}
	switch (mask) {
	case IIO_CHAN_INFO_PROCESSED:
		ret = cm3218_read_lux(client, val);
		printk("gotit %d", ret);
		if (!ret)
			ret = IIO_VAL_INT;
		break;
	case IIO_CHAN_INFO_CALIBSCALE:
		*val = chip->lensfactor;
		ret = IIO_VAL_INT_PLUS_MICRO;
		break;
	default:
		break;
	}
	mutex_unlock(&chip->lock);
	return ret;
}


static const struct iio_chan_spec cm3218_channels[] = {
	{
		.type = IIO_LIGHT,
		.indexed = 1,
		.channel = 0,
		.info_mask_separate = 
			BIT(IIO_CHAN_INFO_PROCESSED) | 
			BIT(IIO_CHAN_INFO_CALIBSCALE),
	}
};


static int cm3218_chip_init(struct i2c_client *client)
{
	struct cm3218_chip *chip = iio_priv(i2c_get_clientdata(client));
	int status, i;

	memset(chip->reg_cache, 0, sizeof(chip->reg_cache));

	for (i = 0; i < 5; i++) {
		status = cm3218_write(client, CM3218_REG_ADDR_CMD,
				CM3218_CMD_ALS_SD);
		if (status >= 0)
			break;
		cm3218_read_ara(client);
	}

	status = cm3218_write(client, CM3218_REG_ADDR_CMD, CM3218_DEFAULT_CMD);
	if (status < 0) {
		dev_err(&client->dev, "Init CM3218 CMD fails\n");
		return status;
	}


	/* Clean interrupt status */
	cm3218_read(client, CM3218_REG_ADDR_STATUS);

	return 0;
}

static const struct iio_info cm3218_info = {
	.driver_module = THIS_MODULE,
	.read_raw = &cm3218_read_raw,
	.write_raw = &cm3218_write_raw,
};

static int cm3218_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct cm3218_chip *chip;
	struct iio_dev *indio_dev;
	int err;

	indio_dev = iio_device_alloc(sizeof(*chip));
	if (indio_dev == NULL) {
		dev_err(&client->dev, "iio allocation fails\n");
		return -ENOMEM;
	}
	chip = iio_priv(indio_dev);
	chip->client = client;
	i2c_set_clientdata(client, indio_dev);

	mutex_init(&chip->lock);

	chip->lensfactor = 1000;
	chip->suspended = false;

	err = cm3218_chip_init(client);
	if (err)
		goto exit_iio_free;

	indio_dev->info = &cm3218_info;
	indio_dev->channels = cm3218_channels;
	indio_dev->num_channels = ARRAY_SIZE(cm3218_channels);
	indio_dev->name = id->name;
	indio_dev->dev.parent = &client->dev;
	indio_dev->modes = INDIO_DIRECT_MODE;
	err = iio_device_register(indio_dev);
	if (err) {
		dev_err(&client->dev, "iio registration fails\n");
		goto exit_iio_free;
	}

	return 0;
exit_iio_free:
	iio_device_free(indio_dev);
	return err;
}


static int cm3218_remove(struct i2c_client *client)
{
	struct iio_dev *indio_dev = i2c_get_clientdata(client);

	dev_dbg(&client->dev, "%s()\n", __func__);
	iio_device_unregister(indio_dev);
	iio_device_free(indio_dev);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int cm3218_suspend(struct device *dev)
{
	struct cm3218_chip *chip = iio_priv(dev_get_drvdata(dev));

	mutex_lock(&chip->lock);

	/* Since this driver uses only polling commands, we are by default in
	 * auto shutdown (ie, power-down) mode.
	 * So we do not have much to do here.
	 */
	chip->suspended = true;

	mutex_unlock(&chip->lock);
	return 0;
}

static int cm3218_resume(struct device *dev)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct cm3218_chip *chip = iio_priv(indio_dev);
	struct i2c_client *client = chip->client;
	int err;

	mutex_lock(&chip->lock);

	err = cm3218_chip_init(client);
	if (!err)
		chip->suspended = false;

	mutex_unlock(&chip->lock);
	return err;
}

static SIMPLE_DEV_PM_OPS(cm3218_pm_ops, cm3218_suspend, cm3218_resume);
#define CM3218_PM_OPS (&cm3218_pm_ops)
#else
#define CM3218_PM_OPS NULL
#endif

static const struct i2c_device_id cm3218_id[] = {
	{"cm3218", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, cm3218_id);

static const struct of_device_id cm3218_of_match[] = {
	{ .compatible = "invn,cm3218", },
	{ },
};
MODULE_DEVICE_TABLE(of, cm3218_of_match);

#ifdef CONFIG_ACPI
static struct acpi_device_id cm3218_acpi_match[] = {
	{ "CPLM3218", 0 },
	{ },
};

MODULE_DEVICE_TABLE(acpi, cm3218_acpi_match);
#endif

static struct i2c_driver cm3218_driver = {
	.driver	 = {
			.name = "cm3218",
			.pm = CM3218_PM_OPS,
			.owner = THIS_MODULE,
			.of_match_table = cm3218_of_match,
			.acpi_match_table = ACPI_PTR(cm3218_acpi_match),
		    },
	.probe		= cm3218_probe,
	.remove		= cm3218_remove,
	.id_table       = cm3218_id,
	.address_list   = normal_i2c,
};

module_i2c_driver(cm3218_driver);

MODULE_DESCRIPTION("CM3218 Ambient Light Sensor driver");
MODULE_LICENSE("GPL");
