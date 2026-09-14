#!/bin/bash

while :
do
   if gpioget --bias=pull-up GPIO18 | grep -q "inactive" 
   then
      echo "Button Pressed!"
   fi
   sleep 1
done
