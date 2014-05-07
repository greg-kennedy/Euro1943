/* message.h - does the little message box with speaker icons */

#ifndef _MESSAGE_H
#define _MESSAGE_H

#include "common_client.h"

int message_init();

void message_clear();
void message_post(int speaker, const char *message);
void message_draw();

void message_quit();

#endif
