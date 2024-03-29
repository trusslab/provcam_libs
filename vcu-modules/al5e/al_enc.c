/*
 * al_enc.c userspace interface and probe / remove of the driver
 *
 * Copyright (C) 2019, Allegro DVT (www.allegrodvt.com)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <linux/io.h>
#include <linux/uaccess.h>

#include <linux/cdev.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/fcntl.h>
#include <linux/firmware.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/slab.h>
#include <linux/stddef.h>
#include <linux/vmalloc.h>

#include "al_enc_ioctl.h"
#include "al_alloc_ioctl.h"
#include "al_dmabuf.h"
#include "al_vcu.h"
#include "al_alloc.h"
#include "al_user.h"
#include "al_codec.h"
#include "enc_user.h"
#include "al_char.h"

#include "mcu_interface.h"
#include "al_mail.h"
#include "enc_mails_factory.h"

#define AL5E_FIRMWARE "al5e.fw"
#define AL5E_BOOTLOADER_FIRMWARE "al5e_b.fw"

int max_users_nb = MAX_USERS_NB;
static int al5e_codec_major;
static int al5e_codec_nr_devs = AL5_NR_DEVS;
static struct class *module_class;
unsigned int recorded_addr_4_enc = 0;
unsigned int recorded_fd_4_enc = 0;

static int ioctl_usage(struct al5_user *user, unsigned int cmd)
{
	pr_err("Unknown ioctl: 0x%.8X\n", cmd);
	pr_err("Known ioctl are:\n");
	pr_err("AL_MCU_CONFIG_CHANNEL:%.8lX\n",
	       (unsigned long)AL_MCU_CONFIG_CHANNEL);
	pr_err("AL_MCU_DESTROY_CHANNEL:%.8lX\n",
	       (unsigned long)AL_MCU_DESTROY_CHANNEL);
	pr_err("AL_MCU_ENCODE_ONE_FRM:%.8lX\n",
	       (unsigned long)AL_MCU_ENCODE_ONE_FRM);
	pr_err("AL_MCU_WAIT_FOR_STATUS:%.8lX\n",
	       (unsigned long)AL_MCU_WAIT_FOR_STATUS);
	pr_err("GET_DMA_FD:%.8lX\n",
	       (unsigned long)GET_DMA_FD);

	return -EINVAL;
}

static long al5e_ioctl(struct file *filp, unsigned int cmd,
		       unsigned long arg)
{
	struct al5_filp_data *filp_data = filp->private_data;
	struct al5_user *user = filp_data->user;
	struct al5_codec_desc *codec = filp_data->codec;
	long ret;

	switch (cmd) {
		struct al5_config_channel config_channel;
		struct al5_params encode_status;
		struct al5_encode_msg encode_msg;
		struct al5_reconstructed_info rec_msg;
		struct al5_buffer buffer_msg;
		u32 rec_fd;
	case AL_MCU_CONFIG_CHANNEL:

        // shiroha_printk("[shiroha]%s: cmd: %s, %d.\n", __func__, "AL_MCU_CONFIG_CHANNEL", AL_MCU_CONFIG_CHANNEL);

		ioctl_info("ioctl AL_MCU_CONFIG_CHANNEL from user %i",
			   user->uid);
		if (copy_from_user(&config_channel, (void *)arg,
				   sizeof(config_channel)))
			return -EFAULT;
		ret = al5e_user_create_channel(user, &config_channel.param,
					       &config_channel.status);
		if (copy_to_user((void *)arg, &config_channel,
				 sizeof(config_channel)))
			return -EFAULT;
		ioctl_info("end AL_MCU_CONFIG_CHANNEL for user %i", user->uid);
		return ret;

	case AL_MCU_DESTROY_CHANNEL:

        // shiroha_printk("[shiroha]%s: cmd: %s, %d.\n", __func__, "AL_MCU_DESTROY_CHANNEL", AL_MCU_DESTROY_CHANNEL);

		ioctl_info("ioctl AL_MCU_DESTROY_CHANNEL from user %i",
			   user->uid);
		ret = al5_user_destroy_channel(user, false);
		ioctl_info("end AL_MCU_DESTROY_CHANNEL for user %i", user->uid);
		return ret;

	case AL_MCU_WAIT_FOR_STATUS:

        // shiroha_printk("[shiroha]%s: cmd: %s, %d.\n", __func__, "AL_MCU_WAIT_FOR_STATUS", AL_MCU_WAIT_FOR_STATUS);

		ioctl_info("ioctl AL_MCU_WAIT_FOR_STATUS from user %i",
			   user->uid);
		if (copy_from_user(&encode_status, (void *)arg,
				   sizeof(encode_status)))
			return -EFAULT;
		ret = al5e_user_wait_for_status(user, &encode_status);
		if (copy_to_user((void *)arg, &encode_status,
				 sizeof(encode_status)))
			return -EFAULT;
		ioctl_info("end AL_MCU_WAIT_FOR_STATUS for user %i", user->uid);
		return ret;

	case AL_MCU_ENCODE_ONE_FRM:

        // shiroha_printk("[shiroha]%s: cmd: %s, %d.\n", __func__, "AL_MCU_ENCODE_ONE_FRM", AL_MCU_ENCODE_ONE_FRM);

		ioctl_info("ioctl AL_MCU_ENCODE_ONE_FRM from user %i",
			   user->uid);
		if (copy_from_user(&encode_msg, (void *)arg,
				   sizeof(encode_msg)))
			return -EFAULT;

        // encode_msg.addresses.opaque_params[0] = 0xd000000;
        // encode_msg.addresses.opaque_params[1] = 0xd0e1000;
        // encode_msg.addresses.opaque_params[0] = 0x40000000;
        // encode_msg.addresses.opaque_params[1] = 0x400e1000;
        // encode_msg.addresses.opaque_params[1] = 0x4b000;

        // for debugging only, sleeping 1 second
        // #include <linux/delay.h>
        // usleep_range(1000000, 1000001);

        // shiroha_printk("[shiroha]%s: encoding one frame with chan_uid: %d, params size: %d, addresses size: %d, address 0: 0x%x, address 1: 0x%x, first_4_bytes: 0x%x.\n", __func__, user->chan_uid, encode_msg.params.size, encode_msg.addresses.size, encode_msg.addresses.opaque_params[0], encode_msg.addresses.opaque_params[1], *(volatile u32*)(phys_to_virt(encode_msg.addresses.opaque_params[0])));
		ret = al5e_user_encode_one_frame(user, &encode_msg);
		if (copy_to_user((void *)arg, &encode_msg, sizeof(encode_msg)))
			return -EFAULT;
		ioctl_info("end AL_MCU_ENCODE_ONE_FRM for user %i", user->uid);
		return ret;

	case AL_MCU_GET_REC_PICTURE:

        // shiroha_printk("[shiroha]%s: cmd: %s, %d.\n", __func__, "AL_MCU_GET_REC_PICTURE", AL_MCU_GET_REC_PICTURE);

		ioctl_info("ioctl AL_MCU_GET_REC from user %i", user->uid);
		if (copy_from_user(&rec_msg, (void *)arg, sizeof(rec_msg)))
			return -EFAULT;
		ret = al5e_user_get_rec(user, &rec_msg);
		if (copy_to_user((void *)arg, &rec_msg, sizeof(rec_msg)))
			return -EFAULT;
		ioctl_info("end AL_MCU_GET_REC for user %i", user->uid);
		return ret;

	case AL_MCU_RELEASE_REC_PICTURE:

        // shiroha_printk("[shiroha]%s: cmd: %s, %d.\n", __func__, "AL_MCU_RELEASE_REC_PICTURE", AL_MCU_RELEASE_REC_PICTURE);

		ioctl_info("ioctl AL_MCU_GET_REC from user %i", user->uid);
		if (copy_from_user(&rec_fd, (void *)arg, sizeof(rec_fd)))
			return -EFAULT;
		return al5e_user_release_rec(user, rec_fd);
		ioctl_info("end AL_MCU_GET_REC from user %i", user->uid);

	case AL_MCU_PUT_STREAM_BUFFER:

        // shiroha_printk("[shiroha]%s: cmd: %s, %d.\n", __func__, "AL_MCU_PUT_STREAM_BUFFER", AL_MCU_PUT_STREAM_BUFFER);

		if (copy_from_user(&buffer_msg, (void *)arg,
				   sizeof(buffer_msg)))
			return -EFAULT;

		return al5e_user_put_stream_buffer(user, &buffer_msg);
        // return 0;

	case GET_DMA_FD:
    {
        // shiroha_printk("[shiroha]%s: cmd: %s, %d.\n", __func__, "GET_DMA_FD", GET_DMA_FD);

        // shiroha_printk("[shiroha]%s: dump_stack before.\n", __func__);
        // dump_stack();
        // shiroha_printk("[shiroha]%s: dump_stack after.\n", __func__);

		ret = al5_ioctl_get_dma_fd(codec->device, arg);
        // shiroha_printk("[shiroha]%s: al5_ioctl_get_dma_fd res: %d.\n", __func__, ret);

        struct al5_dma_info info;
        int err;

        if (copy_from_user(&info, (struct al5_dma_info *)arg, sizeof(info)))
            return -EFAULT;

        err = al5_allocate_dmabuf(codec->device, info.size, &info.fd);
        if (err)
            return err;

        err = al5_dmabuf_get_address(codec->device, info.fd, &info.phy_addr);
        if (err)
            return err;

        // if (info.size >= 1511424) {
        //     if (recorded_addr_4_enc) {
        //         shiroha_printk("[shiroha]%s: for phy_addr, replacing 0x%lx with 0x%lx; for fd, replacing %d with %d.\n", __func__, info.phy_addr, recorded_addr_4_enc, info.fd, recorded_fd_4_enc);
        //         // printk("[shiroha]%s: replacing 0x%lx with 0x%lx.\n", __func__, info.phy_addr, recorded_addr_4_enc);
        //         info.phy_addr = recorded_addr_4_enc;
        //         info.fd = recorded_fd_4_enc;
        //     } else {
        //         recorded_addr_4_enc = info.phy_addr;
        //         recorded_fd_4_enc = info.fd;
        //     }
        // }

        // shiroha_printk("[shiroha]%s: allocation size: %d, address: 0x%x.\n", __func__, info.size, info.phy_addr);
		// printk("[shiroha]%s: allocation size: %d, address: 0x%x.\n", __func__, info.size, info.phy_addr);

        if (copy_to_user((void *)arg, &info, sizeof(info)))
            return -EFAULT;

		return ret;
    }
	case GET_DMA_PHY:
    {
        // shiroha_printk("[shiroha]%s: cmd: %s, %d.\n", __func__, "GET_DMA_PHY", GET_DMA_PHY);

		// ret = al5_ioctl_get_dmabuf_dma_addr(codec->device, arg);

        struct al5_dma_info info;
        int err;

        if (copy_from_user(&info, (struct al5_dma_info *)arg, sizeof(info)))
            return -EFAULT;

        err = al5_dmabuf_get_address(codec->device, info.fd, &info.phy_addr);
        if (err)
            return err;

        // info.phy_addr = 0x40000000;
        info.phy_addr = 0xc400000;

        // shiroha_printk("[shiroha]%s: allocation size(?): %d, address: 0x%x, first_4_bytes: 0x%x.\n", __func__, info.size, info.phy_addr, *(volatile u32*)(phys_to_virt(info.phy_addr)));
		// printk("[shiroha]%s: allocation size(?): %d, address: 0x%x, first_4_bytes: 0x%x.\n", __func__, info.size, info.phy_addr, *(volatile u32*)(phys_to_virt(info.phy_addr)));

        if (copy_to_user((void *)arg, &info, sizeof(info)))
            return -EFAULT;

        return 0;

		return ret;
    }
	default:

        // shiroha_printk("[shiroha]%s: cmd: %s.\n", __func__, "default");

		return ioctl_usage(user, cmd);
	}
}

static const struct file_operations al5e_fops = {
	.owner		= THIS_MODULE,
	.open		= al5_codec_open,
	.release	= al5_codec_release,
	.unlocked_ioctl = al5e_ioctl,
	.compat_ioctl	= al5_codec_compat_ioctl,
};

static int al5e_setup_codec_cdev(struct al5_codec_desc *codec, int minor)
{
	struct device *device;
	dev_t dev = MKDEV(al5e_codec_major, minor);

	int err = al5_setup_codec_cdev(codec, &al5e_fops, THIS_MODULE,
				       al5e_codec_major,
				       minor);

	if (err)
		return err;

	device = device_create(module_class, NULL, dev, NULL, "allegroIP");
	if (IS_ERR(device)) {
		pr_err("device not created\n");
		al5_clean_up_codec_cdev(codec);
		return PTR_ERR(device);
	}

	return 0;
}

static int al5e_probe(struct platform_device *pdev)
{

    shiroha_printk("[shiroha]al5e_probe: attempt to probe al5e.\n");

	int err;
	static int current_minor;

	struct al5_codec_desc *codec;

	codec = devm_kzalloc(&pdev->dev,
			     sizeof(struct al5_codec_desc),
			     GFP_KERNEL);
	if (codec == NULL)
		return -ENOMEM;

	err = al5_codec_set_up(codec, pdev, max_users_nb);
	if (err) {
		dev_err(&pdev->dev, "Failed to setup codec");
		return err;
	}
	printk("[Shiroha]%s: about to call al5_codec_set_firmware.\n", __func__);
	err = al5_codec_set_firmware(codec, AL5E_FIRMWARE,
				     AL5E_BOOTLOADER_FIRMWARE);
	if (err) {
		dev_err(&pdev->dev, "Failed to setup firmware");
		al5_codec_tear_down(codec);
		return err;
	}

    shiroha_printk("[shiroha]%s: suballoc_buf size: %d, suballoc_buf dma_handle: 0x%lx, icache size: %d, icache dma_handle: 0x%lx, dcache_base_addr: 0x%lx.\n", __func__, codec->suballoc_buf->size, codec->suballoc_buf->dma_handle, codec->icache->size, codec->icache->dma_handle, codec->dcache_base_addr);

	err = al5e_setup_codec_cdev(codec, current_minor);
	if (err) {
		dev_err(&pdev->dev, "Failed to setup cdev");
		al5_codec_tear_down(codec);
		return err;
	}
	codec->minor = current_minor;
	++current_minor;

    shiroha_printk("[shiroha]al5e_probe: al5e is now probed.\n");

	return 0;
}

static int al5e_remove(struct platform_device *pdev)
{
	struct al5_codec_desc *codec = platform_get_drvdata(pdev);
	dev_t dev = MKDEV(al5e_codec_major, codec->minor);

	al5_codec_tear_down(codec);
	device_destroy(module_class, dev);
	al5_clean_up_codec_cdev(codec);

	return 0;
}

static const struct of_device_id al5e_of_match[] = {
	// { .compatible = "al,al5e" },
	{ .compatible = "al,al5e" },
	{ /* sentinel */ },
};

MODULE_DEVICE_TABLE(of, al5e_of_match);

static struct platform_driver al5e_platform_driver = {
	.probe			= al5e_probe,
	.remove			= al5e_remove,
	.driver			=       {
		.name		= "al5e",
		.of_match_table = of_match_ptr(al5e_of_match),
	},
};

static int setup_chrdev_region(void)
{
	return al5_setup_chrdev_region(&al5e_codec_major, 0,
				       al5e_codec_nr_devs, "al5e");
}

static int create_module_class(void)
{
	module_class = class_create(THIS_MODULE, "allegro_encode_class");
	if (IS_ERR(module_class))
		return PTR_ERR(module_class);

	return 0;
}

static void destroy_module_class(void)
{
	class_destroy(module_class);
}

static int __init al5e_init(void)
{

    shiroha_printk("[shiroha]al5e_init: al5e driver init.\n");

	int err;

	err = setup_chrdev_region();
	if (err)
		return err;

	err = create_module_class();
	if (err)
		return err;

	return platform_driver_register(&al5e_platform_driver);
}

static void __exit al5e_exit(void)
{
	dev_t devno = MKDEV(al5e_codec_major, 0);

	platform_driver_unregister(&al5e_platform_driver);
	destroy_module_class();
	unregister_chrdev_region(devno, al5e_codec_nr_devs);
}

module_init(al5e_init);
module_exit(al5e_exit);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Kevin Grandemange");
MODULE_AUTHOR("Sebastien Alaiwan");
MODULE_AUTHOR("Antoine Gruzelle");
MODULE_AUTHOR("Shiroha Liu");
MODULE_DESCRIPTION("Allegro Codec Driver modified by Shiroha");
