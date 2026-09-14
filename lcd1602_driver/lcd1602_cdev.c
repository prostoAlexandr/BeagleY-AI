#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alexandr Belikov");
MODULE_DESCRIPTION("LCD1602 I2C Driver - Step 3 Char Device /dev/lcd1602");
MODULE_VERSION("0.3");

#define LCD_BACKLIGHT 0x08
#define LCD_ENABLE    0x04
#define LCD_RS        0x01

static struct i2c_client *lcd_client;

static int lcd1602_write_pcf(struct i2c_client *client, u8 val)
{
    return i2c_smbus_write_byte(client, val);
}

/* Передача полубайта (4 бит) с формированием строба Enable */
static void lcd1602_send_nibble(struct i2c_client *client, u8 nibble, u8 rs)
{
    u8 data = (nibble & 0xF0) | rs | LCD_BACKLIGHT;

    lcd1602_write_pcf(client, data | LCD_ENABLE);
    usleep_range(1, 5);
    lcd1602_write_pcf(client, data & ~LCD_ENABLE);
    usleep_range(50, 100);
}

/* Передача байта команда/данные двумя полубайтами */
static void lcd1602_send_byte(struct i2c_client *client, u8 val, u8 rs)
{
    lcd1602_send_nibble(client, val & 0xF0, rs);
    lcd1602_send_nibble(client, (val << 4) & 0xF0, rs);
}

static void lcd1602_send_command(struct i2c_client *client, u8 cmd)
{
    lcd1602_send_byte(client, cmd, 0);
}

static void lcd1602_send_data(struct i2c_client *client, u8 data)
{
    lcd1602_send_byte(client, data, LCD_RS);
}

/* Процедура сброса и инициализации HD44780 по даташиту */
static void lcd1602_init_display(struct i2c_client *client)
{
    msleep(50); // Пауза после подачи питания

    // Аппаратный сброс в 4-битный режим (3 раза команда 0x30, затем 0x20)
    lcd1602_send_nibble(client, 0x30, 0);
    msleep(5);
    lcd1602_send_nibble(client, 0x30, 0);
    usleep_range(150, 200);
    lcd1602_send_nibble(client, 0x30, 0);
    usleep_range(150, 200);
    lcd1602_send_nibble(client, 0x20, 0); // Переход в 4-битный режим
    usleep_range(150, 200);

    // Конфигурация HD44780
    lcd1602_send_command(client, 0x28); // 4-бит режим, 2 строки, шрифт 5x8
    lcd1602_send_command(client, 0x0C); // Включить дисплей, курсор выключен
    lcd1602_send_command(client, 0x01); // Очистка экрана
    msleep(2);
    lcd1602_send_command(client, 0x06); // Авто-инкремент курсора
}

/* Реализация файловой операции write() для /dev/lcd1602 */
static ssize_t lcd1602_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    char kbuf[33] = {0};
    size_t len = min(count, (size_t)32);
    size_t i;

    if (!lcd_client)
        return -ENODEV;

    // Безопасное копирование данных из пространства пользователя в буфер ядра
    if (copy_from_user(kbuf, buf, len))
        return -EFAULT;

    // Очищаем экран перед записью нового текста
    lcd1602_send_command(lcd_client, 0x01);
    msleep(2);

    for (i = 0; i < len && kbuf[i] != '\n' && kbuf[i] != '\0'; i++) {
        // Переход на вторую строку после 16 символов
        if (i == 16) {
            lcd1602_send_command(lcd_client, 0xC0);
        }
        lcd1602_send_data(lcd_client, kbuf[i]);
    }

    return count;
}

static const struct file_operations lcd1602_fops = {
    .owner = THIS_MODULE,
    .write = lcd1602_write,
};

static struct miscdevice lcd1602_miscdev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = "lcd1602",
    .fops  = &lcd1602_fops,
    .mode  = 0666, /* Права rw-rw-rw- при каждом создании /dev/lcd1602 */
};

static int lcd1602_probe(struct i2c_client *client)
{
    int ret;

    pr_info("lcd1602: Initializing display at address 0x%02x\n", client->addr);

    lcd_client = client;
    lcd1602_init_display(client);

    // Тестовый вывод строки при успешной инициализации
    lcd1602_send_data(client, 'H');
    lcd1602_send_data(client, 'e');
    lcd1602_send_data(client, 'l');
    lcd1602_send_data(client, 'l');
    lcd1602_send_data(client, 'o');

    // Регистрация символьного устройства в подсистеме misc
    ret = misc_register(&lcd1602_miscdev);
    if (ret) {
        dev_err(&client->dev, "Failed to register misc device /dev/lcd1602\n");
        return ret;
    }

    pr_info("lcd1602: /dev/lcd1602 created successfully\n");
    return 0;
}

static void lcd1602_remove(struct i2c_client *client)
{
    pr_info("lcd1602: Removing device /dev/lcd1602 and cleaning up\n");

    misc_deregister(&lcd1602_miscdev);

    // Очистка экрана и отключение подсветки
    lcd1602_send_command(client, 0x01);
    msleep(2);
    lcd1602_write_pcf(client, 0x00);
    lcd_client = NULL;
}

static const struct of_device_id lcd1602_of_match[] = {
    { .compatible = "custom,lcd1602-marquee" },
    { }
};
MODULE_DEVICE_TABLE(of, lcd1602_of_match);

static struct i2c_driver lcd1602_driver = {
    .driver = {
        .name = "lcd1602_marquee",
        .of_match_table = lcd1602_of_match,
    },
    .probe = lcd1602_probe,
    .remove = lcd1602_remove,
};

module_i2c_driver(lcd1602_driver);