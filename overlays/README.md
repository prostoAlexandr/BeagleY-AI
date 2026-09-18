led-0 - red led
led-1 - green led

dtc -@ -I dts -O dtb -o beagley-leds-monitor.dtbo beagley-leds-monitor.dts
mv beagley-leds-monitor.dtbo /boot/firmware/overlays/

$ tail /boot/firmware/extlinux/extlinux.conf
    #on the same line
    fdtoverlays /overlays/lcd1602_overlay.dtbo /overlays/beagley-leds-monitor.dtbo
