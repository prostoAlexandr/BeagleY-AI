# -*- coding: utf-8 -*-
import time
import gpiod

CHIP_PATH = "/dev/gpiochip3"
PIN_OFFSET = 14

config = {
    PIN_OFFSET: gpiod.LineSettings(
        direction=gpiod.line.Direction.OUTPUT
    )
}

with gpiod.request_lines(
    path=CHIP_PATH,
    consumer="beagle-blink",
    config=config
) as line_request:

    print(f"Blinking LED on {CHIP_PATH}, pin {PIN_OFFSET}. Press Ctrl+C to exit.")
    
    try:
        while True:
            line_request.set_value(PIN_OFFSET, gpiod.line.Value.ACTIVE)
            time.sleep(1)
            line_request.set_value(PIN_OFFSET, gpiod.line.Value.INACTIVE)
            time.sleep(1)
    except KeyboardInterrupt:
        print("\nExiting...")
