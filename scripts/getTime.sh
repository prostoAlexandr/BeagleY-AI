HWTIME=$(sudo hwclock)
echo "RTC - ${HWTIME} "

SYSTIME=$(date +%Y-%m-%d' '%H:%M:%S.%N%:z)
echo "SYS - ${SYSTIME} "
