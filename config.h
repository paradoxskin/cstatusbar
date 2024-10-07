#ifndef CSTATUSBAR_CONFIG_H
#define CSTATUSBAR_CONFIG_H

#define MAXCC 512
#define BATT_NOW "/sys/class/power_supply/BAT0/charge_now"
#define BATT_FULL "/sys/class/power_supply/BAT0/charge_full"
#define BATT_STATUS "/sys/class/power_supply/BAT0/status"
#define FIFO "/tmp/dwm.fifo"
#define GETSSID "nmcli connection show --active |grep wifi |awk '{print $1}'"
#define ONLINECHECKPOINT "bilibili.com"
#define VALUE_BEGIN ';'
#define VALUE_END '\n'

#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO 1
#define LOG_LEVEL_WARN 2
#define LOG_LEVEL_ERROR 3
#ifdef DEBUG
#define CURRENT_LOG_LEVEL LOG_LEVEL_DEBUG
#pragma message ("debug is enable")
#else
#define CURRENT_LOG_LEVEL LOG_LEVEL_ERROR
#endif

#define LOG(level, ...) \
    do { \
        if (level >= CURRENT_LOG_LEVEL) { \
            printf(__VA_ARGS__); \
            fflush(stdout); \
        } \
    } while (0)

#endif // CSTATUSBAR_CONFIG_H
