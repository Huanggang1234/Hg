#ifndef HG_HTTP_MODULE_H
#define HG_HTTP_MODULE_H
#include"../base/include/cris_str.h"
#include"hg_http_parse.h"
#include"hg.h"

struct hg_http_module_t;
struct hg_http_header_t;
struct hg_http_headers_t;
struct hg_http_request_t;
struct hg_http_conf_t;
struct hg_http_response_t;
struct hg_http_asyn_event_t;
struct hg_upstream_t;


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





extern hg_module_t hg_http_module;

void* hg_http_create_conf(hg_cycle_t *cycle);
//int   hg_http_init_module(hg_module_t *m,hg_cycle_t *cycle);
extern int num_http_modules;//http模块的数量

struct hg_http_module_t{
    int   (*preconfiguration)(hg_cycle_t*);

    int   (*postconfiguration)(hg_module_t*,hg_cycle_t*,hg_http_conf_t*);

    void* (*create_main_conf)(hg_cycle_t*);

    int   (*init_main_conf)(hg_module_t*,hg_cycle_t*,hg_http_conf_t*);

    void* (*create_srv_conf)(hg_cycle_t*);

    int   (*init_srv_conf)(hg_module_t*,hg_cycle_t*,hg_http_conf_t*);

    void* (*create_loc_conf)(hg_cycle_t*);

    int   (*init_loc_conf)(hg_module_t*,hg_cycle_t*,hg_http_conf_t*);

    int   (*merge_srv_conf)(void*,void*);

    int   (*merge_loc_conf)(void*,void*);
 
    hg_http_conf_t *ptr=NULL;
};


#define  hg_get_main_conf(type,module,conf)  ((type*)conf->main_conf[module->ctx_index])
#define  hg_get_srv_conf(type,module,conf)   ((type*)conf->srv_conf[module->ctx_index])
#define  hg_get_loc_conf(type,module,conf)   ((type*)conf->loc_conf[module->ctx_index])


struct hg_http_conf_t{
   void** main_conf=NULL;
   void** srv_conf=NULL;
   void** loc_conf=NULL;
};

struct cris_http_header_t{
    cris_str_t  name;
    cris_str_t  content; 
    cris_http_header_t *next=NULL;
};


//原始的字符串头部,均指向原始的缓冲地址
struct cris_http_headers_t{

     cris_http_header_t *accept=NULL;//6客户端可接收的媒体资源类型

     cris_http_header_t *accept_encoding=NULL;//15客户端可接受的编码方法

     cris_http_header_t *accept_language=NULL;//15客户端可接受的语言

     cris_http_header_t *cookie=NULL;//6

     cris_http_header_t *user_agent=NULL;//10向服务器发送关于浏览器版本的一些信息
        
     cris_http_header_t *cache_control=NULL;//13控制浏览器的缓存

     cris_http_header_t *connection=NULL;//10长连接，如keep-alive

     cris_http_header_t *host=NULL;//请求的主机名

     cris_http_header_t *content_type=NULL;//12

     cris_http_header_t *content_length=NULL;//14

     cris_http_header_t *headers=NULL;//其他的头部组成链表
    
};


typedef enum{

    cris_000=0,//默认值

    cris_100,//继续，请求者应当继续提出请求。服务器以收到请求的一部分，正在等待其余部分

    cris_200,//成功，通常表示服务器提供了请求的网页

    cris_201,//已创建，请求成功，服务器创建了新的资源

    cris_202,//已接受，服务器已经接受请求，但尚未处理

    cris_204,//无内容，服务器成功的处理了请求，但没有返回任何内容

    cris_205,//重置内容，服务器成功的处理了请求，但没有返回任何内容

    cris_301,//永久移动，请求的网页已经永久移动到新的位置，响应会自动将请求转移到新的位置

    cris_304,//未修改，自从上次请求后，请求的网页没有修改过，服务器返回此响应时，不会返回网页内容

    cris_400,//错误请求，客户端请求语法错误

    cris_401,//未授权，对于需要登录的网页可能需要此响应

    cris_403,//禁止，理解请求但拒绝

    cris_404,//未找到，

    cris_408,//请求超时，服务器等待请求时，发生超时

    cris_411,//需要有效长度，

    cris_414,//请求的url过长，

    cris_500,//服务器内部错误，

    cris_501,//尚未实施，列如无法识别请求方法  

    cris_502,//服务不可用，从上游服务器收到无效响应

    cris_504,//网关超时，服务器作为网关或代理，没有及时从上游服务器收到请求

    cris_505//http版本不受支持，

}rpcode;



typedef int (*hg_http_request_handler)(cris_http_request_t *r);


struct hg_connection_t;
struct hg_http_core_srv_conf_t;

#define hg_add_new_head(r,head) {    \
 head->next=r->headers_in.headers;   \
 r->headers_in.headers=head;         \
}


struct hg_http_response_t{

       int access_code=0;//必填       
       int content_type=0;//最好填上
       long content_length=0;//缓冲式包体必填
       int header_length=0;
       off_t body_send=0;
       int head_send=0;

       bool has_body=false;//是否传输包体，必填
       bool file=false;//是否传输文件
       bool send_head=false;

       
       cris_http_header_t *headers=NULL;

       int fd=0;//传输文件的描述符
  
       cris_buf_t *body=NULL;//包体，缓冲式包体必填
};


#define HG_HTTP_BODY_UNCOMPLETE   0
#define HG_HTTP_BODY_COMPLETE     1
#define HG_HTTP_BODY_UNAVAILABLE  2

struct hg_http_body_t{

       int state=HG_HTTP_BODY_UNCOMPLETE;
       cris_buf_t *temp=NULL;
       cris_str_t *name=NULL;
       int fd=0;
       int wrote=0;//已经写入缓冲文件的数量
       bool on=false;//缓存文件是否打开
       int (*callback)(void *,int)=NULL;
};

#define HG_ASYN_DISCARD_BODY     0
#define HG_ASYN_RECIVE_BODY      1
#define HG_ASYN_UPSTREAM         2

struct hg_http_asyn_event_t{
       int  type=0;
       hg_http_asyn_event_t *next=NULL;
};


#define HG_RESPONSE_INITIAL      0
#define HG_RESPONSE_SEND_HEADER  1
#define HG_RESPONSE_SEND_BODY    2
#define HG_RESPONSE_SEND_FILE    3
#define HG_RESPONSE_END          4

struct module_data{

       void *data;
       unsigned long long id=0;
       module_data *next=NULL;
};

struct cris_http_request_t{

     cris_str_t method;

     void *data=NULL;//供第三方模块挂载数据的指针

     char  *pre=NULL;//解析请求时用到的自由指针

     cris_http_header_t *head_tmp=NULL;//解析请求时用到的自由指针

     cris_str_t   entire_request;

     cris_str_t   url;

     cris_str_t   url_param;

     cris_http_headers_t  headers_in;//请求的头部信息

     hg_connection_t     *conn=NULL;

     hg_http_request_handler   read_handler=NULL;
     hg_http_request_handler   write_handler=NULL;

     hg_http_conf_t      *loc_conf=NULL;//location级别的配置
     hg_http_core_srv_conf_t *server=NULL;

     cris_http_request_t *main=NULL;//原始请求
     cris_http_request_t *parent=NULL;//父请求

     int  content_length=0;//包体的总长

     cris_mpool_t *pool=NULL;

     hg_http_body_t body;

     hg_http_response_t response;

     hg_upstream_t    *upstream=NULL;

     hg_http_asyn_event_t   *asyn_event=NULL;
 
     int  response_state=HG_RESPONSE_INITIAL;

     bool skip_response=false;//跳过响应阶段
    
     bool responsing=false;//是否在响应当中

     int  recv_body=0;

     int   count=0;//引用计数

     int   count_rewrite=0;//重写次数

     int   parse_state=0;

     int   phase_handler=0;

     int   access_code=0;

//     int   body_in_file=0;//接收包体是否存放在缓存文件当中，默认不存放

     int   response_code=0;

     int   (*content_handler)(cris_http_request_t *)=NULL;

};




#endif
