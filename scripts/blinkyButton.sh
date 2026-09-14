#!/bin/bash

while :
do
   if gpioget --bias=pull-up GPIO18 | grep -q "inactive" 
   then
      gpioset -t0 GPIO14=1
   else
      gpioset -t0 GPIO14=0
   fi
done
