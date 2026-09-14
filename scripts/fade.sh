#!/bin/bash

PWMPIN="/dev/hat/pwm/GPIO14"


echo 1000 > $PWMPIN/period
echo 0 > $PWMPIN/duty_cycle
echo 0 > $PWMPIN/enable
sleep 1

for i in {1..500};
do
        echo $i > $PWMPIN/duty_cycle
        echo 1 > $PWMPIN/enable
        echo $i
        sleep 0.0005
done

for i in {500..1};
do
    echo $i > $PWMPIN/duty_cycle
    echo 1 > $PWMPIN/enable
    echo $i
    sleep 0.0005
done
