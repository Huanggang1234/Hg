#ifndef HG_CONTROL_MODULE_H
#define HG_CONTROL_MODULE_H
#include"hg.h"
#include<sys/time.h>
#include<pthread.h>
struct hg_control_conf_t{
 
    int worker=2;

};

extern hg_module_t hg_control_module;


int hg_work();
int hg_master_process();
int hg_worker_process();

extern timeval control_time;
extern pthread_mutex_t *accept_mutex;

#endif




