#ifndef HG_H
#define HG_H
#include<string>
#include<cstdio>
#include<cstdlib>
#include"../base/cris_memery_pool.h"
#include<vector>
struct hg_command_t;
struct hg_cycle_t;
struct hg_module_t;

struct hg_cycle_t;

typedef int (*hg_common_pt)(hg_module_t *module,hg_cycle_t *cycle);

typedef void* (*hg_conf_create_pt)(hg_cycle_t *cycle);

struct hg_module_t{
    int type;
    int index;
    int ctx_index;
    void *ctx=NULL;
    std::vector<hg_command_t> *commands=NULL; 
    hg_conf_create_pt  create_conf=NULL;
    hg_common_pt init_conf=NULL;
    hg_common_pt init_module=NULL;
    hg_common_pt init_process=NULL;
};

struct cris_conf_t;

typedef int (*hg_conf_handler_pt)(hg_module_t *module,hg_cycle_t *cycle,cris_conf_t *conf);



#define HG_CMD_MAIN                      1
#define HG_CMD_HTTP                      2
#define HG_CMD_SERVER                    4
#define HG_CMD_LOCATION                  8
#define HG_CMD_UPSTREAM                 16


struct hg_command_t{
     std::string conf_name;
     unsigned int info=0;     
     hg_conf_handler_pt   conf_handler=NULL;
};




struct hg_connection_t;
struct hg_event_t;
struct hg_listen_t;
struct cris_buf_t;

struct hg_cycle_t{

    void** conf_ctx=NULL;//全体模块配置的上下文

    std::vector<hg_module_t*> *modules=NULL;//全体模块 
     
    cris_mpool_t *pool=NULL;//内存池
    
    hg_connection_t *connections=NULL;//连接池数组 
    hg_event_t     *reads=NULL;
    hg_event_t     *writes=NULL;
    
    hg_connection_t *free_connections=NULL;//空闲的连接   
    hg_listen_t   *listens=NULL;//监听对象集合
   
    cris_buf_t    *conf_file=NULL;//配置文件在内存中的缓存 
    std::string   conf_path;//配置文件的路径

};




#define  hg_add_listen(ls,cy)  \
{\
   ls->next=cy->listens; \
   cy->listens=ls;\
}


#define  hg_get_module_conf(module,cycle)  (cycle->conf_ctx[module->index])



#endif


