/*
 * Generic Driver for GPIO Button Array (\ACPI\PNP0C40) used on tablets etc.
 *
 * Copyright (c) 2014, John Wells <jfw@jfwhome.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/gpio.h>

#include <linux/acpi.h>
#include <acpi/acpi_bus.h>
#include <acpi/acpi_drivers.h>

#include <linux/gpio/consumer.h>
#include <linux/gpio_keys.h>
#include <linux/input.h>


#define PREFIX "ACPI: "
#define ACPI_GPIO_BUTTON_ARRAY_CLASS "buttonarray"
#define GPIO_BUTTON_ARRAY_MAX_BUTTONS 5


static int gpio_button_array_add(struct acpi_device *device);
static int gpio_button_array_remove(struct acpi_device *device);

ACPI_MODULE_NAME("gpio_button_array");


static const struct acpi_device_id btnarray_device_ids[] = {
	/* "Windows compatible button array" */
	{ "PNP0C40", 0 },
	{  }
};

MODULE_DEVICE_TABLE(acpi, btnarray_device_ids);


static struct acpi_driver gpio_button_array_driver = {
	.name		= "GPIO Tablet Button Array",
	.ids		= btnarray_device_ids,
	.ops		= {
				.add =		gpio_button_array_add,
				.remove = 	gpio_button_array_remove,
			},
	.owner	= THIS_MODULE
};


/* Platform device... */

static struct gpio_keys_platform_data *pdata;
static struct platform_device buttons_platform_device = {
   .name = "gpio-keys",
   .id = 0,
   .dev = {
	  .platform_data = &pdata
   },
};


/*
 * Extract up to GPIO_BUTTON_ARRAY_MAX buttons from the ACPI entry. 
 * According to the spec, there should be up to 5: Power, Home, Vol Up, Vol Down, Rotation Lock
 * The first two should be able to wake the device.
 * Once found, bind them to a gpio-keys platform device.
*/
static int gpio_button_array_add(struct acpi_device *device)
{
	int nbuttons = 0;
	int error = 0;
	static struct gpio_desc *desc;
	struct gpio_keys_button *button;

	char *labels[GPIO_BUTTON_ARRAY_MAX] = {"Power_Button", "Home_Button", "Volume_Up", "Volume_Down", "Screen_Lock"};
	int keycodes[GPIO_BUTTON_ARRAY_MAX] = {KEY_POWER, KEY_LEFTMETA, KEY_VOLUMEUP, KEY_VOLUMEDOWN, KEY_SCREENLOCK};

	pdata = kzalloc(sizeof(*pdata) + nbuttons * (sizeof *button), GFP_KERNEL);

	printk("Looking for GPIO buttons\n");


	if (!pdata) {
		error = -ENOMEM;
		goto err_out;
	}

	for (nbuttons = 0; nbuttons < GPIO_BUTTON_ARRAY_MAX; nbuttons++) {
		desc = devm_gpiod_get_index(&device->dev, NULL, nbuttons); 
		if (IS_ERR(desc)) {
			break;
		}

		button = &pdata->buttons[nbuttons];

		button->desc = labels[nbuttons]; 
		button->gpio = desc_to_gpio(desc);
		button->active_low = 0;
		button->code = keycodes[nbuttons];
		button->type = (nbuttons == 0) ? EV_PWR : ((nbuttons == 4) ? EV_SW : EV_KEY);
		button->wakeup = (nbuttons < 2) ? 1 : 0;

	}
	

	if (nbuttons == 0) {
		dev_err(&device->dev, "Failed to get any gpio descriptors for GPIO\n");
		error = -ENODEV;
		goto err_free_pdata;
	} 
	
	pdata->nbuttons = nbuttons;
	pdata->rep = 1;
	
	buttons_platform_device.dev.platform_data = pdata;
	
	
	printk("Adding button array!\n");
	platform_device_register(&buttons_platform_device);
	printk("Finished adding button array!\n"); 	


	return 0;

err_free_pdata:
	kfree(pdata);
	
err_out:
	return error;
	
}


static int gpio_button_array_remove(struct acpi_device *device)
{

	platform_device_unregister(&buttons_platform_device);

	return 0;
}

static int __init gpio_button_array_init(void)
{
	int result = 0;

	result = acpi_bus_register_driver(&gpio_button_array_driver);
	if (result < 0) {
			ACPI_DEBUG_PRINT((ACPI_DB_ERROR,
							  "Error registering driver\n"));
			return -ENODEV;
	}
	return 0;
}



static void __exit gpio_button_array_exit(void)
{
	acpi_bus_unregister_driver(&gpio_button_array_driver);
}

module_init(gpio_button_array_init);
module_exit(gpio_button_array_exit);

MODULE_AUTHOR("John Wells <jfw@jfwhome.com>");
MODULE_DESCRIPTION("Driver for Generic Tablet Button Array Device");
MODULE_LICENSE("GPL v2");
