#define first
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <dirent.h>

#include "config.h"

typedef struct Option Option;

struct Option {
    unsigned int value;
    Option *next;
    char status[MAXCC];
};

// global
static Option *status;
static Display *dpy;
static int running;
static long full;
static short draw;
static pthread_mutex_t lock;
static pthread_cond_t cond;
static char bat_now[256];
static char bat_full[256];
static char bat_status[256];

// def
static void echo(const char*);
static Option *genoption();
static char *otos();
static void *fifo(void*);
static void *timer(void*);
static void *drawer(void*);
static void sendreq();
static unsigned int getbattery();
static void getssid(char[64]);
static unsigned int isonline();

// imf
void sendreq() {
    LOG(LOG_LEVEL_DEBUG, "[sendreq] try to get lock\n");
    pthread_mutex_lock(&lock);
    LOG(LOG_LEVEL_DEBUG, "[sendreq] get lock\n");
    draw = 1;
    LOG(LOG_LEVEL_DEBUG, "[sendreq] send cond\n");
    pthread_cond_signal(&cond);
    LOG(LOG_LEVEL_DEBUG, "[sendreq] send cond done\n");
    LOG(LOG_LEVEL_DEBUG, "[sendreq] try to release lock\n");
    pthread_mutex_unlock(&lock);
    LOG(LOG_LEVEL_DEBUG, "[sendreq] release lock\n");
}

void *drawer(void *args) {
    struct timespec timeout;
    while (running) {
        pthread_mutex_lock(&lock);
        while (draw == 0) {
            pthread_cond_wait(&cond, &lock);
        }

        LOG(LOG_LEVEL_DEBUG, "[drawer] try to draw\n");
        while (draw) {
            clock_gettime(CLOCK_REALTIME, &timeout);
            timeout.tv_nsec += 300000000;
            if (timeout.tv_nsec >= 1000000000) {
                timeout.tv_sec += 1;
                timeout.tv_nsec -= 1000000000;
            }
            LOG(LOG_LEVEL_DEBUG, "[drawer] wait to draw\n");
            if (pthread_cond_timedwait(&cond, &lock, &timeout) == ETIMEDOUT) {
                echo(otos());
                draw = 0;
                LOG(LOG_LEVEL_DEBUG, "[drawer] draw done\n");
                break;
            }
            LOG(LOG_LEVEL_DEBUG, "[drawer] still wait to draw\n");
        }
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

void echo(const char *text) {
    if (XStoreName(dpy, DefaultRootWindow(dpy), text) < 0) LOG(LOG_LEVEL_ERROR, "Write failed.\n");
    XFlush(dpy);
}

Option *genoption() {
    Option *o = NULL;
    o = calloc(1, sizeof(Option));
    if (o == NULL) LOG(LOG_LEVEL_ERROR, "Calloc memery error.\n");
    o->next = status;
    status = o;
    return o;
}

unsigned int getbattery() {
    unsigned int bat = 0;
    long cur = 0;
    char *st = malloc(sizeof(char)*12);
    FILE *fp = NULL;
    if (strcmp(bat_now, "ukn") == 0) {
        return 1 << 8 | 0x50;
    }
    if ((fp = fopen(bat_now, "r"))) {
        fscanf(fp, "%ld\n", &cur);
        fclose(fp);
        if (!full) {
            fp = fopen(bat_full, "r");
            fscanf(fp, "%ld\n", &full);
            fclose(fp);
        }
        fp = fopen(bat_status, "r");
        fscanf(fp, "%s\n", st);
        fclose(fp);
        bat = (unsigned int)cur * 100 / full;
        if (strcmp(st, "Charging") == 0) bat |= 1 << 7;
    }
    else {
        bat = 1 << 8 | 0x50;
    }
    return bat;
}

void getssid(char ssid[64]) {
    FILE *fp = popen(GETSSID, "r");
    ssid[0] = '\0';
    if (fp == NULL) return;
    if (fgets(ssid, 64, fp) != NULL) {
        ssid[strlen(ssid) -1] = ' ';
    }
    pclose(fp);
}

unsigned int isonline() {
    int sockfd;
    struct sockaddr_in server_addr;
    struct hostent *host;
    struct timeval timeout;
    host = gethostbyname(ONLINECHECKPOINT);
    if (host == NULL) return 0;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) return 0;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (const char *)&timeout, sizeof(timeout));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(80);
    server_addr.sin_addr = *((struct in_addr *)host->h_addr);
    memset(&(server_addr.sin_zero), 0, 8);
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
        close(sockfd);
        return 0;
    }
    close(sockfd);
    return 1;
}

void *fifo(void *args) {
    Option **options = (Option **)args;
    Option *light = options[0];
    Option *volumn = options[1];
    Option *fcitx = options[2];
    Option *hook = options[3];
    Option *todo = options[4];
    Option *beast = options[5];
    Option *beast_mask = options[6];

    strcpy(light->status, "^c#ffff99^00 ");
    strcpy(volumn->status, "^c#a8c8ff^00 ");
    strcpy(fcitx->status, "^c#443326^ ");
    strcpy(hook->status, "^c#ffffff^ ");
    strcpy(todo->status, "^c#00bbbb^ ");
    strcpy(beast->status, "^c#a77777^  ");
    strcpy(beast_mask->status, "^b#a73333^^d^");

    int fd;
    char buffer[1024];
    unsigned int pos = 0, half = 0;
    unsigned int len, nvui, tmp;
    char key;
    char value[MAXCC];
    LOG(LOG_LEVEL_DEBUG, "[fifo] start handle.\n");

    while (running) {
        fd = open(FIFO, O_RDONLY);
        if (fd == -1) LOG(LOG_LEVEL_ERROR, "Fifo not found.\n");
        len = read(fd, buffer, sizeof(buffer));
        for (tmp = 0; tmp < len; tmp++) {
            if (half) {
                if (buffer[tmp] != '\n') {
                    value[pos++] = buffer[tmp];
                    continue;
                }
                value[pos] = '\0';
                LOG(LOG_LEVEL_DEBUG, "[fifo] read %c -> %s\n", key, value);
                switch (key) {
                    case 'H':
                        if (pos == 0) strcpy(hook->status +10, "\ue007 ");
                        else strcat(strcpy(hook->status +10, value), " \uea7d ");
                        break;
                    case 'T':
                        if (pos == 0) strcpy(todo->status +10, "\uebb1 ");
                        else strcat(strcpy(todo->status +10, value), " \uf0a8 ");
                        break;
                    case 'L':
                        nvui = strlen(value);
                        if (nvui == 2) {
                            light->status[10] = value[0];
                            light->status[11] = value[1];
                        }
                        else if (nvui == 1){
                            light->status[10] = '0';
                            light->status[11] = value[0];
                        }
                        else {
                            light->status[10] = '9';
                            light->status[11] = '9';
                        }
                        break;
                    case 'V':
                        nvui = strlen(value);
                        if (nvui == 2) {
                            volumn->status[10] = value[0];
                            volumn->status[11] = value[1];
                        }
                        else if (nvui == 1){
                            volumn->status[10] = '0';
                            volumn->status[11] = value[0];
                        }
                        else {
                            volumn->status[10] = '9';
                            volumn->status[11] = '9';
                        }
                        break;
                    case 'F':
                        if (value[0] != fcitx->value) {
                            fcitx->value = value[0];
                            if (value[0] - '0') strcpy(fcitx->status +10, "^b#fb8feb^󱌱");
                            else strcpy(fcitx->status +10, " ");
                        }
                        break;
                    case 'B':
                        if (value[0] == '0') {
                            strcpy(beast->status + 5, "7777^\uf204  ");
                        }
                        else if (value[0] == '2') {
                            beast_mask->status[10] = '^';
                        }
                        else {
                            strcpy(beast->status + 5, "3333^\uf205  ");
                            beast_mask->status[10] = '\0';
                        }
                        break;
                    default:
                        break;
                }
                half = 0;
                LOG(LOG_LEVEL_DEBUG, "[fifo] send %c -> %s\n", key, value);
                sendreq();
                LOG(LOG_LEVEL_DEBUG, "[fifo] send %c -> %s done\n", key, value);
                continue;
            }
            if (buffer[tmp] == ';') {
                half = 1;
                pos = 0;
                continue;
            }
            key = buffer[tmp];
        }
        close(fd);
    }
    return NULL;
}

void *timer(void *args) {
    Option **options = (Option **)args;
    Option *clock = options[0];
    Option *battery = options[1];
    Option *wlan = options[2];

    strcpy(clock->status, " 00:00 ^b#8b8feb^^c#443326^零^d^");
    strcpy(battery->status, "^c#aa0000^\uf244 ^c#bbbbbb^");
    strcpy(wlan->status, "^c#aa0000^󰕑 ");

    time_t now;
    struct tm *local;
    unsigned int nvalue, pos, tmp = 0;
    char ssid[64];
    char weekday[] = "\u65e5\u4e00\u4e8c\u4e09\u56db\u4e94\u516d";
    char batt[] = "\uf244\uf243\uf242\uf241\uf240";
    char batc[] = "006699ccdd";
    char wlant[] = "0000^󰕑dd00^󰞉";
    LOG(LOG_LEVEL_DEBUG, "[timer] start run\n");
    while (running) {
        time(&now);
        local = localtime(&now);
        clock->status[1] = '0' + local->tm_hour/10;
        clock->status[2] = '0' + local->tm_hour%10;
        clock->status[4] = '0' + local->tm_min/10;
        clock->status[5] = '0' + local->tm_min%10;
        memcpy(clock->status +27, weekday +3*local->tm_wday, 3);
        nvalue = getbattery();
        if (battery->value != nvalue) {
            battery->value = nvalue;
            if (nvalue >> 8) { // bat not found
                battery->status[3] = 'f';
                battery->status[4] = 'f';
                memcpy(battery->status +10, "\ueb2d", 3);
                nvalue &= 0x7f;
                for (tmp = 100; nvalue/tmp == 0; tmp/=10);
                for (pos = 24; tmp; tmp/=10, pos+=1) battery->status[pos] = '*';
                battery->status[pos] = '\0';
                nvalue = (nvalue +5)/25;
            }
            else if (nvalue >> 7) { // charge
                battery->status[3] = 'f';
                battery->status[4] = 'f';
                memcpy(battery->status +10, "\uf492", 3);
                nvalue &= 0x7f;
                for (tmp = 100; nvalue/tmp == 0; tmp/=10);
                for (pos = 24; tmp; tmp/=10, pos+=1) battery->status[pos] = '0' + nvalue /tmp %10;
                battery->status[pos] = '\0';
                nvalue = (nvalue +5)/25;
            }
            else {
                battery->status[3] = 'a';
                battery->status[4] = 'a';
                battery->status[24] = '\0';
                nvalue = (nvalue +5)/25;
                memcpy(battery->status +10, batt +3*nvalue, 3);
            }
            memcpy(battery->status +5, batc +2*nvalue, 2);
        }
        LOG(LOG_LEVEL_DEBUG, "[timer] echo for time\n");
        echo(otos());
        LOG(LOG_LEVEL_DEBUG, "[timer] echo done\n");
        memcpy(wlan->status + 5, wlant + isonline() * 9, 9);
        getssid(ssid);
        LOG(LOG_LEVEL_DEBUG, "[timer] ssid: %s", ssid);
        strcpy(wlan->status + 15, ssid);
        pos = strlen(ssid);
        if (pos) {
            for (tmp = 0, pos -= 1; pos; pos--) {
                tmp += ssid[pos];
            }
        }
        if (wlan->value != tmp) {
            wlan->value = tmp;
            LOG(LOG_LEVEL_DEBUG, "[timer] send for ssid\n");
            sendreq();
            LOG(LOG_LEVEL_DEBUG, "[timer] send for ssid done\n");
        }

        time(&now);
        local = localtime(&now);
        sleep(60 - local->tm_sec);
    }
    return NULL;
}

char *otos() {
    char* c = NULL;
    c = calloc(MAXCC, sizeof(char));
    if (!c) LOG(LOG_LEVEL_ERROR, "Calloc memery error.\n");
    unsigned int l = 0;
    unsigned int nl;
    Option *o = status;
    for (; o; o = o->next) {
        //o->status
        nl = strlen(o->status);
        if (l + nl > MAXCC) break;
        strcpy(c + l, o->status);
        l += nl;
    }
    c[l] = '\0';
    return c;
}

void find_bat_file() {
    char bat_path[64] = "/sys/class/power_supply/";
    (void)strcat(bat_path, BAT);
    DIR *bat_dir = opendir(bat_path);
    if (bat_dir == NULL) {
        LOG(LOG_LEVEL_ERROR, "BAT file not found.\n");
        return;
    }
    struct dirent *file;
    while ((file = readdir(bat_dir)) != NULL) {
        if (strcmp(file->d_name, "charge_full") == 0) {
            snprintf(bat_full, sizeof(bat_full), "%s/charge_full", bat_path);
            snprintf(bat_now, sizeof(bat_now), "%s/charge_now", bat_path);
            snprintf(bat_status, sizeof(bat_status), "%s/status", bat_path);
        }
        else if (strcmp(file->d_name, "energy_full") == 0) {
            snprintf(bat_full, sizeof(bat_full), "%s/energy_full", bat_path);
            snprintf(bat_now, sizeof(bat_now), "%s/energy_now", bat_path);
            snprintf(bat_status, sizeof(bat_status), "%s/status", bat_path);
        }
    }
    if (strlen(bat_full) == 0) {
        LOG(LOG_LEVEL_ERROR, "UNKNOW BAT TYPE\n");
        strcpy(bat_now, "ukn");
    }
}

int main() {
    dpy = XOpenDisplay(NULL);
    if (!dpy) {
        LOG(LOG_LEVEL_ERROR, "Can't open display.\n");
        return 1;
    }

    // init
    find_bat_file();
    Option *fcitx = genoption();
    Option *time = genoption();
    Option *battery = genoption();
    Option *volumn = genoption();
    Option *light = genoption();
    Option *beast = genoption();
    Option *wlan = genoption();
    Option *todo = genoption();
    Option *hook = genoption();
    Option *beast_mask = genoption();
    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&cond, NULL);
    draw = 0;
    running = 1;

    // args
    Option* arg_fifo[7] = {light, volumn, fcitx, hook, todo, beast, beast_mask};
    Option* arg_timer[3] = {time, battery, wlan};

    // thread
    pthread_t thread_fifo, thread_timer, thread_drawer;
    pthread_create(&thread_drawer, NULL, drawer, NULL);
    pthread_create(&thread_fifo, NULL, fifo, arg_fifo);
    pthread_create(&thread_timer, NULL, timer, arg_timer);

    // loop
    pthread_join(thread_drawer, NULL);
    pthread_join(thread_fifo, NULL);
    pthread_join(thread_timer, NULL);
    return 0;
}
