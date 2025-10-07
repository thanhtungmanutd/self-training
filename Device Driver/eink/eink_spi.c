#include <linux/init.h>
#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/kernel.h>
#include <linux/fb.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include "eink.h"

static eink_t eink;

static ssize_t eink_fb_write(struct fb_info *info, const char __user *buf,
			    		  	 size_t count, loff_t *ppos)
{
	u8 *dest = kzalloc(info->var.xres * info->var.yres / 8, GFP_KERNEL);		
	if (!dest)
		return -ENOMEM;

	int c = copy_from_user(dest, buf, count);
	if (c) 
		return -EFAULT;
		
	eink_image(&eink, dest);
	kfree(dest);
	return 0;
}

struct fb_info *info;

static struct fb_ops eink_ops = {
	.owner = THIS_MODULE,
	.fb_write = eink_fb_write,
};

static struct of_device_id eink_of_match[] = {
	{.compatible = "waveshare,eink",},
	{},
};
MODULE_DEVICE_TABLE(of, eink_of_match);

static int eink_dev_probe(struct spi_device *spi) {
	eink.spidev = spi;

	eink.cs = devm_gpiod_get(&spi->dev, "cs", GPIOD_OUT_HIGH);
	eink.rst = devm_gpiod_get(&spi->dev, "reset", GPIOD_OUT_LOW);
	eink.dc = devm_gpiod_get(&spi->dev, "dc", GPIOD_OUT_LOW);
	eink.busy = devm_gpiod_get(&spi->dev, "busy", GPIOD_IN);

	info = framebuffer_alloc(0, &eink.spidev->dev);

	// fb device var
	info->var.xres = EINK_DISPLAY_WIDTH;
	info->var.yres = EINK_DISPLAY_HEIGHT;
	info->var.bits_per_pixel = 1;

	// device file operations
	info->fbops = &eink_ops;

	register_framebuffer(info);

	eink_init(&eink);
	pr_info("Load module to kernel\n");

    return 0;
}

static void eink_dev_remove(struct spi_device *spi) {
	unregister_framebuffer(info);
	framebuffer_release(info);
	pr_info("Unload module from kernel\n");
}

static struct spi_driver eink_spi_driver = {
	// .id_table = bme688_device_ids,
	.probe = eink_dev_probe,
	.remove = eink_dev_remove,
	.driver = {
		.name = "eink",
		.of_match_table = eink_of_match,
	},
};

module_spi_driver(eink_spi_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("TungDT53");
MODULE_DESCRIPTION("Mikroe E-Paper Device Driver");