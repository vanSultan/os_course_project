#include "pcd8544.h"

static uint8_t pcd8544_buffer[LCD_WIDTH * LCD_HEIGHT / 8] = {0,};
static uint8_t textsize = 1;
static uint8_t textcolor = 1;
static uint8_t cursor_x = 0;
static uint8_t cursor_y = 0;

const char this_driver_name[] = "pcd8544";

struct pcd8544_dev {
	dev_t devt;
	struct cdev cdev;
	struct class *class;
};

static struct pcd8544_dev pcd8544_dev;

static void lcd_clear(void) {
	uint32_t i;
	for (i = 0; i < LCD_WIDTH * LCD_HEIGHT / 8 ; i++)
		pcd8544_buffer[i] = 0;
	cursor_y = cursor_x = 0;
}

static void set_pixel(uint8_t x, uint8_t y, uint8_t color)
{
	if ((x >= LCD_WIDTH) || (y >= LCD_HEIGHT))
		return;

	if (color)
		pcd8544_buffer[x + (y / 8) * LCD_WIDTH] |= _BV(y % 8);
	else
		pcd8544_buffer[x + (y / 8) * LCD_WIDTH] &= ~_BV(y % 8);
}

static void lcd_draw_char(uint8_t x, uint8_t y, char c)
{
	uint8_t i;
	uint8_t j;
	uint8_t d;

	if (y >= LCD_HEIGHT) return;
	if ((x + 5) >= LCD_WIDTH) return;

	for (i = 0; i < 5; i++)
	{
		d = *(font + (c * 5) + i);
		for (j = 0; j < 8; j++)
		{
			if (d & _BV(j))
				set_pixel(x + i, y + j, textcolor);
			else
				set_pixel(x + i, y + j, !textcolor);
		}
	}

	for (j = 0; j < 8; j++)
		set_pixel(x + 5, y + j, !textcolor);
}

static void lcd_write(uint8_t c)
{
	if (c == '\n') {
		cursor_y += textsize * 8;
		cursor_x = 0;
	} else if (c == '\r') {
		// skip
	} else {
		lcd_draw_char(cursor_x, cursor_y, c);
		cursor_x += textsize * 6;

		if (cursor_x >= (LCD_WIDTH - 5)) {
			cursor_x = 0;
			cursor_y += 8;
		}
		if (cursor_y >= LCD_HEIGHT)
			cursor_y = 0;
	}
}

static void lcd_draw_string(uint8_t x, uint8_t y, char *c)
{
	cursor_x = x;
	cursor_y = y;

	while (*c)
		lcd_write(*c++);
}

static void byte2lcd(bool cd, unsigned char data)
{
	uint8_t i;
	unsigned pattern = 0b10000000;

	if (cd)
		gpio_set_value(GPIO_DC, true);
	else
		gpio_set_value(GPIO_DC, false);

	for (i = 0; i < 8; i++)
	{
		gpio_set_value(GPIO_CLK, false);
		if (data & pattern)
			gpio_set_value(GPIO_DIN, true);
		else
			gpio_set_value(GPIO_DIN, false);

		udelay(4);
		gpio_set_value(GPIO_CLK, true);
		udelay(4);

		pattern >>= 1;
	}
}

static void lcd_display(void)
{
	uint8_t col = 0;
	uint8_t p;

	for (p = 0; p < 6; p++) {
		byte2lcd(LCD_COMMAND, PCD8544_SETYADDR | p);
		byte2lcd(LCD_COMMAND, PCD8544_SETXADDR | col);

		for (col = 0; col <= LCD_WIDTH - 1; col++) {
			byte2lcd(LCD_DATA, pcd8544_buffer[(LCD_WIDTH * p) + col]);
		}
	}

	byte2lcd(LCD_COMMAND, PCD8544_SETYADDR);
}

static ssize_t pcd8544_write(struct file *filp, const char *buff, size_t len, loff_t *f_pos)
{
	int x;
	int y;
	int c;
	int line_num;

	char *user_buff = kmalloc(++len * sizeof(char), GFP_KERNEL);
	memset(user_buff, 0, len);

	if (copy_from_user(user_buff, buff, len)) {
		len = -EFAULT;
	}
	user_buff[len - 2] = 0;

	if (sscanf(user_buff, "%d %d %d", &x, &y, &c) == 3) {
		textcolor = c > 0 ? BLACK : WHITE;
		if (x < 0 || y < 0) {
			lcd_clear();
		} else {
			set_pixel(x, y, c);
		}
	} else if (sscanf(user_buff, "%d ", &line_num) == 1) {
		textcolor = BLACK;
		if (--line_num < 0)
			lcd_draw_string(cursor_x, cursor_y, user_buff + 2);
		else
			lcd_draw_string(0, line_num * 8, user_buff + 2);
	}

	kfree(user_buff);
	lcd_display();

	return len;
}

static int pcd8544_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int pcd8544_probe(void)
{
	gpio_request(GPIO_RESET, "PCD8544 Reset Pin");
	gpio_direction_output(GPIO_RESET, 0);
	gpio_request(GPIO_DC, "PCD8544 Data/Command Pin");
	gpio_direction_output(GPIO_DC, 0);
	gpio_request(GPIO_CS, "PCD8544 ChipSelect Pin");
	gpio_direction_output(GPIO_CS, 0);
	gpio_request(GPIO_DIN, "PCD8544 DataInput Pin");
	gpio_direction_output(GPIO_DIN, 0);
	gpio_request(GPIO_CLK, "PCD8544 Clock Pin");
	gpio_direction_output(GPIO_CLK, 0);

	gpio_set_value(GPIO_RESET, 1);

	gpio_set_value(GPIO_CS, false);
	gpio_set_value(GPIO_CLK, false);
	gpio_set_value(GPIO_RESET, false);
	udelay(5);
	gpio_set_value(GPIO_RESET, true);

	byte2lcd(LCD_COMMAND, 0x21);	// LCD Extended Commands
	byte2lcd(LCD_COMMAND, 0xb1);	// Set LCD Cop (Contrast).	//0xb1
	byte2lcd(LCD_COMMAND, 0x04);	// Set Temp coefficent.		//0x04
	byte2lcd(LCD_COMMAND, 0x14);	// LCD bias mode 1:48. 		//0x13
	byte2lcd(LCD_COMMAND, 0x0c);	// LCD in normal mode. 0x0d inverse mode
	byte2lcd(LCD_COMMAND, 0x20);
	byte2lcd(LCD_COMMAND, 0x0c);

	lcd_clear();
	lcd_display();

	return 0;
}

static int pcd8544_remove(void)
{
	gpio_free(GPIO_RESET);
	gpio_free(GPIO_DC);
	gpio_free(GPIO_CS);
	gpio_free(GPIO_DIN);
	gpio_free(GPIO_CLK);

	return 0;
}

static const struct file_operations pcd8544_fops = {
	.owner =	THIS_MODULE,
	.write = 	pcd8544_write,
	.open =		pcd8544_open,
};

static int __init pcd8544_init_cdev(void)
{
	int error;

	pcd8544_dev.devt = MKDEV(0, 0);

	error = alloc_chrdev_region(&pcd8544_dev.devt, 0, 1, this_driver_name);
	if (error < 0) {
		printk(KERN_ALERT "alloc_chrdev_region() failed: %d \n", error);
		return -1;
	}

	cdev_init(&pcd8544_dev.cdev, &pcd8544_fops);
	pcd8544_dev.cdev.owner = THIS_MODULE;

	error = cdev_add(&pcd8544_dev.cdev, pcd8544_dev.devt, 1);
	if (error) {
		printk(KERN_ALERT "cdev_add() failed: %d\n", error);
		unregister_chrdev_region(pcd8544_dev.devt, 1);
		return -1;
	}

	return 0;
}

static int __init pcd8544_init_class(void)
{
	pcd8544_dev.class = class_create(THIS_MODULE, this_driver_name);

	if (!pcd8544_dev.class) {
		printk(KERN_ALERT "class_create() failed\n");
		return -1;
	}

	if (!device_create(pcd8544_dev.class, NULL, pcd8544_dev.devt, NULL,
		this_driver_name)) {
		printk(KERN_ALERT "device_create(..., %s) failed\n", this_driver_name);
		class_destroy(pcd8544_dev.class);
		return -1;
	}

	return 0;
}

static int __init pcd8544_init(void)
{
	memset(&pcd8544_dev, ' ', sizeof(pcd8544_dev));
	pcd8544_probe();

	if (pcd8544_init_cdev() < 0)
		goto fail_1;

	if (pcd8544_init_class() < 0)
		goto fail_2;

	printk(KERN_INFO "[pcd8544] PCD8544 was install\n");

	return 0;

fail_2:
	cdev_del(&pcd8544_dev.cdev);
	unregister_chrdev_region(pcd8544_dev.devt, 1);

fail_1:
	pcd8544_remove();
	return -1;
}

static void __exit pcd8544_exit(void)
{
	device_destroy(pcd8544_dev.class, pcd8544_dev.devt);
	class_destroy(pcd8544_dev.class);

	cdev_del(&pcd8544_dev.cdev);
	unregister_chrdev_region(pcd8544_dev.devt, 1);

	pcd8544_remove();

	printk(KERN_INFO "[pcd8544] PCD8544 was remove\n");
}

module_exit(pcd8544_exit);
module_init(pcd8544_init);

MODULE_AUTHOR("Sultan Zhumabaev");
MODULE_DESCRIPTION("PCD8544 driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1");
