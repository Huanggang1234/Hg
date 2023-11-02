#ifndef HG_CONF_PARSE_H
#define HG_CONF_PARSE_H
#include"../base/include/cris_str.h"
#include<list>
#include<cstddef>

struct cris_mpool_t;

struct cris_conf_t{

   cris_str_t name;
   std::list<cris_str_t> avgs;
   cris_mpool_t *pool;
   void *data=NULL;
   cris_conf_t(cris_mpool_t *p):pool(p){}
};


enum  cris_conf_state{
        start=0,
        naming,
        blank_avg,
        avging,
	end
};


char* cris_take_one_conf(char *start_,char *end,cris_conf_t *conf);
long long unit_convert(cris_str_t unit);

cris_str_t cris_filter_convert(cris_str_t token,cris_mpool_t *pool);


#endif














