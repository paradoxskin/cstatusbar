#define first
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>

#include "config.h"

#define BAD(WORD) fprintf(stderr, WORD);exit(1);
#define ENDOFSTORY(WORD) {BAD(WORD)}

static char status[MAXCC];
static Display *dpy;

static void echo(char*);

void echo(char *text) {
    if (XStoreName(dpy, DefaultRootWindow(dpy), text) < 0) ENDOFSTORY("Write failed.")
    XFlush(dpy);
}

int main() {
    dpy = XOpenDisplay(NULL);
    if (!dpy) ENDOFSTORY("Can't open display.\n")
    echo(status);
    return 0;
}
