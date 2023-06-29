#ifndef HG_CONF_PARSE_H
#define HG_CONF_PARSE_H
#include"../base/include/cris_str.h"
#include<list>
#include<cstddef>

struct cris_conf_t{

   cris_str_t name;
   std::list<cris_str_t> avgs;

   cris_conf_t()=default;   
};


enum  cris_conf_state{
        start=0,
        naming,
        blank_avg,
        avging,
        end
};


char* cris_take_one_conf(char *start_,char *end,cris_conf_t *conf);



#endif














