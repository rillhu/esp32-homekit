#ifndef _SNTP_H_
#define _SNTP_H_

struct tm obtain_time(void);
void get_sntp_time_task(void *arg);

#endif