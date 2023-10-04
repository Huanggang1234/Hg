#ifndef HG_LOG_MODULE_H
#define HG_LOG_MODULE_H
#include<cstddef>
#include<cstdarg>
#include"hg.h"
struct hg_log_t;
struct cris_buf_t;
struct cris_str_t;


#define HG_LOG_ERROR     0
#define HG_LOG_WARNING   1
#define HG_LOG_NOTICE    2


void   hg_log_printf(int level,const char*format,...);
void   hg_error_log(int level,const char*format,...);

struct hg_log_t{
    int access_log_fd=0;
    int error_log_fd=0;
    int pre_access_log_fd=0;
    int pre_error_log_fd=0;
    int log_level=0;
 
    cris_buf_t  *access_buf=NULL;
    cris_buf_t  *error_buf=NULL;
    cris_str_t  *log_path=NULL;
};


extern hg_module_t  hg_log_module;

#endif




