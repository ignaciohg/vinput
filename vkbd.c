
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/spinlock.h>

#include "vinput.h"

#define VINPUT_KBD		"vkbd"
#define VINPUT_RELEASE		0
#define VINPUT_PRESS		1

static unsigned short vkeymap[KEY_MAX];

static int vinput_vkbd_init(struct vinput *vinput)
{
	int i;
	int err = 0;

	vinput->input->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REP);
	vinput->input->keycodesize = sizeof(unsigned short);
	vinput->input->keycodemax = KEY_MAX;
	vinput->input->keycode = vkeymap;

	for (i = 0; i < KEY_MAX; i++) {
		set_bit(vkeymap[i], vinput->input->keybit);
	}

	if (input_register_device(vinput->input)) {
		printk(KERN_ERR "cannot register vinput input device\n");
		err = -ENODEV;
	}

	return err;
}

static int vinput_vkbd_read(struct vinput *vinput, char *buff, int len)
{
	spin_lock(&vinput->lock);
	len = snprintf(buff, len, "%+ld\n", vinput->last_entry);
	spin_unlock(&vinput->lock);

	return len;
}

static int vinput_vkbd_send(struct vinput *vinput, char *buff, int len)
{
	short key = 0;
	short type = VINPUT_PRESS;

	if (buff[0] == '+')
		key = simple_strtol(buff + 1, NULL, 10);
	else
		key = simple_strtol(buff, NULL, 10);

	spin_lock(&vinput->lock);
	vinput->last_entry = key;
	spin_unlock(&vinput->lock);

	if (key < 0) {
		type = VINPUT_RELEASE;
		key = -key;
	}

	dev_info(vinput->dev, "Event %s code %d\n",
		 (type == VINPUT_RELEASE) ? "VINPUT_RELEASE" : "VINPUT_PRESS",
		 key);

	input_report_key(vinput->input, key, type);
	input_sync(vinput->input);

	return len;
}

static struct vinput_ops vkbd_ops = {
	.init = vinput_vkbd_init,
	.send = vinput_vkbd_send,
	.read = vinput_vkbd_read,
};

static struct vinput_device vkbd_dev = {
	.name = VINPUT_KBD,
	.ops = &vkbd_ops,
};

static int __init vkbd_init(void)
{
	int i;

	for (i = 0; i < KEY_MAX; i++)
		vkeymap[i] = i;
	return vinput_register(&vkbd_dev);
}

static void __exit vkbd_end(void)
{
	vinput_unregister(&vkbd_dev);
}

module_init(vkbd_init);
module_exit(vkbd_end);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tristan Lelong <tristan.lelong@blunderer.org>");
MODULE_DESCRIPTION("emulate input events thru /dev/vkbd");
