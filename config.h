#ifndef first
#include "cstatusbar.c"
#endif

#define MAXCC 512
#define BATT_NOW "/sys/class/power_supply/BAT0/charge_now"
#define BATT_FULL "/sys/class/power_supply/BAT0/charge_full"
#define BATT_STATUS "/sys/class/power_supply/BAT0/status"
#define FIFO "/tmp/dwm.fifo"
#define GETSSID "nmcli connection show --active |grep wifi |awk '{print $1}'"
#define ONLINECHECKPOINT "bilibili.com"
#define VALUE_BEGIN ';'
#define VALUE_END '\n'
