#ifndef HG_HTTP_TEST_MODULE_CPP
#define HG_HTTP_TEST_MODULE_CPP
#include"../../include/hg.h"
#include"../../include/hg_http_module.h"
#include"../../include/modules/hg_http_test_module.h"
#include"../../include/hg_epoll_module.h"



int hg_http_test_switch_set(hg_module_t *module,hg_cycle_t *cycle,cris_conf_t *conf);

void* hg_http_test_create_loc_conf(hg_cycle_t *cycle);
int hg_http_test_init_loc_conf(hg_module_t *module,hg_cycle_t *cycle,hg_http_conf_t *conf);
int hg_http_test_postconfiguration(hg_module_t *module,hg_cycle_t *cycle,hg_http_conf_t *conf);//在解析完配置文件后，向流水线引擎中添加处理函数


int hg_http_test_request_handler(cris_http_request_t *r);



hg_http_module_t  hg_http_test_module={

	NULL,
	&hg_http_test_postconfiguration,
	NULL,
	NULL,
	NULL,
	NULL,
	&hg_http_test_create_loc_conf,
	&hg_http_test_init_loc_conf,
	NULL,
	NULL,
	NULL
};

std::vector<hg_command_t> hg_http_test_commands={

	{
	    std::string("test_module"),
	    0,
	    &hg_http_test_switch_set
	}
};

hg_module_t  hg_test_module={

	HG_HTTP_MODULE,
	0,
	0,
	&hg_http_test_module,
	&hg_http_test_commands,
	NULL,
	NULL,
	NULL,
	NULL
};


int hg_http_test_switch_set(hg_module_t *module,hg_cycle_t *cycle,cris_conf_t *conf){

    int num=conf->avgs.size();
    if(num==0||num>1)
      return HG_OK;
    hg_http_module_t *http_m=(hg_http_module_t*)module->ctx;
    hg_http_conf_t   *parent_conf=http_m->ptr;

    hg_http_test_loc_conf_t *loc=hg_get_loc_conf(hg_http_test_loc_conf_t,module,parent_conf);

    cris_str_t on=conf->avgs.front();

    if(on==std::string("on"))
       loc->test_on=true;

    return HG_OK;

};


void* hg_http_test_create_loc_conf(hg_cycle_t *cycle){

    cris_mpool_t *pool=cycle->pool;

    hg_http_test_loc_conf_t *conf=new (pool->alloc(sizeof(hg_http_test_loc_conf_t)))hg_http_test_loc_conf_t();

    return (void*)conf;
}

int hg_http_test_init_loc_conf(hg_module_t *module,hg_cycle_t *cycle,hg_http_conf_t *conf){

    printf("test init loc conf\n");
    return HG_OK;
}

int hg_http_test_postconfiguration(hg_module_t *module,hg_cycle_t *cycle,hg_http_conf_t *conf){

     hg_http_add_request_handler(&hg_http_test_request_handler,HG_HTTP_CONTENT_PHASE);
     
     return HG_OK;
}


#endif









