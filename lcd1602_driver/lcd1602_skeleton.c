#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alexandr Belikov");
MODULE_DESCRIPTION("LCD1602 I2C Driver - Step 1 Skeleton");
MODULE_VERSION("0.1");

/* В современных ядрах probe принимает только struct i2c_client * */
static int lcd1602_probe(struct i2c_client *client)
{
    pr_info("lcd1602: Probe successfully called for device at address 0x%02x\n", client->addr);
    return 0;
}

static void lcd1602_remove(struct i2c_client *client)
{
    pr_info("lcd1602: Remove called for device at address 0x%02x\n", client->addr);
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