#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <linux/device.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alexandr Belikov");
MODULE_DESCRIPTION("LCD1602 I2C Driver - Step 6 Sysfs Controls");
MODULE_VERSION("0.6");

#define LCD_BACKLIGHT 0x08
#define LCD_ENABLE    0x04
#define LCD_RS        0x01

#define MARQUEE_BUF_SIZE 128
#define DISPLAY_WIDTH    16

static struct i2c_client *lcd_client;
static DEFINE_MUTEX(lcd_mutex);

static bool backlight_enabled = true;
static unsigned int scroll_delay_ms = 300;

static char marquee_buf[MARQUEE_BUF_SIZE] = {0};
static size_t marquee_len = 0;
static size_t marquee_pos = 0;
static struct delayed_work marquee_work;

static int lcd1602_write_pcf(struct i2c_client *client, u8 val)
{
    return i2c_smbus_write_byte(client, val);
}

/* Передача полубайта (4 бит) с формированием строба Enable */
static void lcd1602_send_nibble(struct i2c_client *client, u8 nibble, u8 rs)
{
    u8 bl_bit = backlight_enabled ? LCD_BACKLIGHT : 0x00;
    u8 data = (nibble & 0xF0) | rs | bl_bit;

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

/* Функция обработчика фоновой прокрутки бегущей строки */
static void lcd1602_marquee_work_func(struct work_struct *work)
{
    char window[DISPLAY_WIDTH + 1] = {0};
    size_t i;

    if (mutex_lock_interruptible(&lcd_mutex))
        return;

    if (!lcd_client || marquee_len <= DISPLAY_WIDTH) {
        mutex_unlock(&lcd_mutex);
        return;
    }

    // Формируем сдвинутое окно из 16 символов по кольцевому буферу
    for (i = 0; i < DISPLAY_WIDTH; i++) {
        window[i] = marquee_buf[(marquee_pos + i) % marquee_len];
    }

    // Возвращаем курсор в начало первой строки
    lcd1602_send_command(lcd_client, 0x80);

    for (i = 0; i < DISPLAY_WIDTH; i++) {
        lcd1602_send_data(lcd_client, window[i]);
    }

    // Инкремент позиции прокрутки
    marquee_pos = (marquee_pos + 1) % marquee_len;

    mutex_unlock(&lcd_mutex);

    // Планируем следующий шаг прокрутки
    schedule_delayed_work(&marquee_work, msecs_to_jiffies(scroll_delay_ms));
}

/* --- sysfs Обработчики атрибутов --- */

static ssize_t backlight_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    bool enabled;

    mutex_lock(&lcd_mutex);
    enabled = backlight_enabled;
    mutex_unlock(&lcd_mutex);

    return sysfs_emit(buf, "%d\n", enabled);
}

static ssize_t backlight_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    bool val;
    int ret;

    ret = kstrtobool(buf, &val);
    if (ret)
        return ret;

    if (mutex_lock_interruptible(&lcd_mutex))
        return -ERESTARTSYS;

    backlight_enabled = val;
    if (lcd_client) {
        /* Принудительно обновляем шину I2C для немедленной смены состояния подсветки */
        lcd1602_write_pcf(lcd_client, backlight_enabled ? LCD_BACKLIGHT : 0x00);
    }
    mutex_unlock(&lcd_mutex);

    return count;
}
static DEVICE_ATTR_RW(backlight);

static ssize_t scroll_delay_ms_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    unsigned int delay;

    mutex_lock(&lcd_mutex);
    delay = scroll_delay_ms;
    mutex_unlock(&lcd_mutex);

    return sysfs_emit(buf, "%u\n", delay);
}

static ssize_t scroll_delay_ms_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    unsigned int val;
    int ret;

    ret = kstrtouint(buf, 10, &val);
    if (ret)
        return ret;

    if (val < 10 || val > 5000)
        return -EINVAL;

    mutex_lock(&lcd_mutex);
    scroll_delay_ms = val;
    mutex_unlock(&lcd_mutex);

    return count;
}
static DEVICE_ATTR_RW(scroll_delay_ms);

static struct attribute *lcd1602_attrs[] = {
    &dev_attr_backlight.attr,
    &dev_attr_scroll_delay_ms.attr,
    NULL,
};
ATTRIBUTE_GROUPS(lcd1602);

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
    char kbuf[MARQUEE_BUF_SIZE] = {0};
    size_t len = min(count, (size_t)(MARQUEE_BUF_SIZE - 1));
    size_t i;

    // Останавливаем предыдущую бегущую строку перед приемом новых данных
    cancel_delayed_work_sync(&marquee_work);

    if (mutex_lock_interruptible(&lcd_mutex))
        return -ERESTARTSYS;

    if (!lcd_client) {
        mutex_unlock(&lcd_mutex);
        return -ENODEV;
    }

    // Безопасное копирование данных из пространства пользователя в буфер ядра
    if (copy_from_user(kbuf, buf, len)) {
        mutex_unlock(&lcd_mutex);
        return -EFAULT;
    }

    // Очищаем символ новой строки на конце
    while (len > 0 && (kbuf[len - 1] == '\n' || kbuf[len - 1] == '\r')) {
        kbuf[len - 1] = '\0';
        len--;
    }

    // Очищаем экран перед записью нового текста
    lcd1602_send_command(lcd_client, 0x01);
    msleep(2);

    if (len <= DISPLAY_WIDTH) {
        // Статический вывод, если длина не превышает ширину дисплея
        for (i = 0; i < len; i++) {
            lcd1602_send_data(lcd_client, kbuf[i]);
        }
    } else {
        // Подготовка буфера для закольцованной бегущей строки (текст + разделитель)
        memset(marquee_buf, 0, sizeof(marquee_buf));
        snprintf(marquee_buf, sizeof(marquee_buf), "%s   ", kbuf);
        marquee_len = strlen(marquee_buf);
        marquee_pos = 0;

        // Выводим первый кадр
        for (i = 0; i < DISPLAY_WIDTH; i++) {
            lcd1602_send_data(lcd_client, marquee_buf[i]);
        }

        // Запускаем асинхронную прокрутку
        schedule_delayed_work(&marquee_work, msecs_to_jiffies(scroll_delay_ms));
    }

    mutex_unlock(&lcd_mutex);
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

/* В современных ядрах probe принимает только struct i2c_client * */
static int lcd1602_probe(struct i2c_client *client)
{
    int ret;

    pr_info("lcd1602: Initializing display at address 0x%02x\n", client->addr);

    INIT_DELAYED_WORK(&marquee_work, lcd1602_marquee_work_func);

    mutex_lock(&lcd_mutex);
    lcd_client = client;
    lcd1602_init_display(client);

    // Тестовый вывод строки при успешной инициализации
    lcd1602_send_data(client, 'H');
    lcd1602_send_data(client, 'e');
    lcd1602_send_data(client, 'l');
    lcd1602_send_data(client, 'l');
    lcd1602_send_data(client, 'o');
    mutex_unlock(&lcd_mutex);

    // Регистрация символьного устройства в подсистеме misc
    ret = misc_register(&lcd1602_miscdev);
    if (ret) {
        dev_err(&client->dev, "Failed to register misc device /dev/lcd1602\n");
        mutex_lock(&lcd_mutex);
        lcd_client = NULL;
        mutex_unlock(&lcd_mutex);
        return ret;
    }

    pr_info("lcd1602: /dev/lcd1602 created successfully\n");
    return 0;
}

static void lcd1602_remove(struct i2c_client *client)
{
    pr_info("lcd1602: Removing device /dev/lcd1602 and cleaning up\n");

    // Отмена незавершившихся воркеров прокрутки перед удалением
    cancel_delayed_work_sync(&marquee_work);

    misc_deregister(&lcd1602_miscdev);

    mutex_lock(&lcd_mutex);
    if (lcd_client) {
        // Очистка экрана и отключение подсветки
        lcd1602_send_command(client, 0x01);
        msleep(2);
        lcd1602_write_pcf(client, 0x00);
        lcd_client = NULL;
    }
    mutex_unlock(&lcd_mutex);
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
        .dev_groups = lcd1602_groups,
    },
    .probe = lcd1602_probe,
    .remove = lcd1602_remove,
    .shutdown = lcd1602_remove,
};

module_i2c_driver(lcd1602_driver);