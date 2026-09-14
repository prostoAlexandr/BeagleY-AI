#!/bin/bash

while :
do
      gpioset -t0 GPIO14=1
      sleep 1
      gpioset -t0 GPIO14=0
      sleep 1
done
