#ifndef HG_HTTP_CORE_H
#define HG_HTTP_CORE_H
#include<vector>
#include"hg_http_module.h"
#include"../base/include/cris_str.h"
#include<unordered_map>
#include<functional>

struct hg_http_core_main_conf_t;
struct hg_http_core_srv_conf_t;
struct hg_http_core_loc_conf_t;
struct hg_http_loc_que_node_t;
struct hg_http_loc_tree_node_t;

struct hg_cycle_t;

extern  hg_module_t hg_http_core_module;


struct  hg_http_handler_t;

typedef int (*hg_http_request_pt)(cris_http_request_t *r);

typedef int (*hg_http_check_pt)(cris_http_request_t *r,hg_http_handler_t *ph);


int hg_http_read_body(cris_http_request_t *r,int (*callback)(cris_http_request_t *));
int hg_http_discard_body(cris_http_request_t *r);

/*

#define   HG_HTTP_POST_READ_PHASE                   0

#define   HG_HTTP_SERVER_REWRITE_PHASE              1

#define   HG_HTTP_FIND_CONFIG_PHASE                 2

#define   HG_HTTP_REWRITE_PHASE                     3

#define   HG_HTTP_POST_REWRITE_PHASE                4

#define   HG_HTTP_PREACCESS_PHASE                   5

#define   HG_HTTP_ACCESS_PHASE                      6

#define   HG_HTTP_POST_ACCESS_PHASE                 7

#define   HG_HTTP_TRY_FILES_PHASE                   8

#define   HG_HTTP_CONTENT_PHASE                     9

#define   HG_HTTP_RESPONSE_PHASE                   10

#define   HG_HTTP_LOG_PHASE                        11

#define   HG_HTTP_PHASE_NUM                        12


*/
int hg_http_add_request_handler(hg_http_request_pt handler,int phase);//添加请求阶段处理回调的接口函数;

//添加内容阶段的特别处理函数,该函数应该在http子模块中的init_loc_conf函数中调用
int hg_http_add_spacial_request_handler(hg_http_request_pt handler,hg_http_conf_t *conf);

struct  hg_http_handler_t{
    
        hg_http_check_pt    checker=NULL;
        hg_http_request_pt  handler=NULL;
        int next=0;
        int last_content_index=0;
        int last_access_index=0;
        int response_index=0;//响应阶段的
        int last=0;
};


struct hg_http_phase_engine_t{
        hg_http_handler_t    *handlers=NULL;

        int  server_rewrite_index=0;
        int  location_rewrite_index=0;
};


struct hg_http_core_main_conf_t{

    std::vector<hg_http_core_srv_conf_t*> server;

    hg_http_phase_engine_t           phase_engine;
    
    std::vector<hg_http_request_pt>  phases[HG_HTTP_PHASE_NUM];

    std::string  temp_path="temp/";
     
    unsigned long long bodyid=0;//包体文件id

};

struct hg_http_core_srv_conf_t{

    cris_str_t    server_name;//虚拟主机名

    short         port=80;//端口号

    hg_http_conf_t *ctx=NULL;//在解析到Server时创建的结构体

    hg_http_loc_tree_node_t *loc_tree=NULL;//静态平衡三叉树

};

enum  Permissions { ANY=0,ALL};

struct hg_http_core_loc_conf_t{

     cris_str_t   name;

     bool         use_regex=false;//是否是正则匹配
     bool         exact_match=false;//是否是精准匹配
     bool         limit_prefix=false;//是否是有限制的前缀匹配

     hg_http_conf_t         *ctx=NULL;//在解析到location时创建的结构体

     hg_http_loc_que_node_t *regexes=NULL;//子正则匹配队列

     //如果这是一个精准匹配的结点，那它不应该再有location子配置，及时有也不会将其放在前缀树上，正则同理
     hg_http_loc_que_node_t *locations=NULL;//常规子配置队列

     hg_http_core_loc_conf_t *pre=NULL;//父级配置

     hg_http_request_pt   content_handler=NULL;//特殊内容处理函数

     Permissions satisfy=ANY;
     bool      body_in_file=false;//将包体存放在缓存文件当中，默认关闭
     bool      static_file=false;
     int       filetype=TYPE_TEXT_PLAIN;
};

struct hg_http_loc_que_node_t{

    cris_str_t              name;

    hg_http_core_loc_conf_t *exact=NULL;
    hg_http_core_loc_conf_t *inclusive=NULL;

    bool                   set=false;//是否设置正则匹配队列
    hg_http_loc_que_node_t *regexes=NULL;//正则匹配队列,同级
   
    hg_http_loc_que_node_t *pre=NULL;
    hg_http_loc_que_node_t *next=NULL;

    friend bool operator<(const hg_http_loc_que_node_t&n1,const hg_http_loc_que_node_t&n2){

           if(n1.name<n2.name)
              return true;
           else if(n1.name==n2.name){
              if(n1.exact!=NULL)
                 return true;
              return false;
           }else
              return false;
          
           return false;
    }
};

struct hg_http_loc_tree_node_t{

       cris_str_t     url;

       hg_http_core_loc_conf_t *exact=NULL;
       hg_http_core_loc_conf_t *inclusive=NULL;

       bool  limit=false;//前缀限制
       //前缀树每一个节点，都会记录与自己同级别的正则匹配对应的配置
       hg_http_loc_que_node_t *regexes=NULL;//正则匹配队列

       hg_http_loc_tree_node_t *c_tree=NULL;//子树,以本节点的url为前缀的配置树
       //互不为前缀的配置树
       hg_http_loc_tree_node_t *left=NULL;
       hg_http_loc_tree_node_t *right=NULL;
};



#endif









