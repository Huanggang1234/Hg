#ifndef HG_UPSTREAM_MODULE_H
#define HG_UPSTREAM_MODULE_H
#include<cstddef>
#include<vector>

#define HG_UPSTREAM_INITIAL             0
#define HG_UPSTREAM_RETRY               1
#define HG_UPSTREAM_CREATE_REQUEST      2
#define HG_UPSTREAM_SEND_REQUEST        3
#define HG_UPSTREAM_PIPE_BACK           4
#define HG_UPSTREAM_PARSE               5
#define HG_UPSTREAM_END                 6
#define HG_UPSTREAM_ERROR               7


#define HG_UPSTREAM_PARSE_HEADER        0
#define HG_UPSTREAM_PARSE_BODY          1 
#include"hg.h"


extern hg_module_t hg_upstream_module;

struct hg_http_conf_t;
struct hg_connection_t;
struct cris_mpool_t;
struct cris_buf_t;
struct cris_http_request_t;
struct hg_upstream_t;
struct hg_upstream_conf_t;
struct hg_upstream_info_t;
struct hg_pipe_t;
struct hg_pipe_conf_t;
struct hg_upstream_cluster_t;
struct hg_upstream_load_balance_t;

hg_upstream_t* hg_add_upstream(cris_http_request_t *r,hg_upstream_conf_t *conf);
hg_pipe_t* hg_add_pipe(hg_upstream_t *up);
int hg_upstream_add_balance(hg_upstream_load_balance_t *balance);


int hg_upstream_set_server(hg_http_conf_t *loc,cris_conf_t *conf);


#define HG_UPSTREAM_UNKOWN             0
#define HG_UPSTREAM_ADD_READ           1
#define HG_UPSTREAM_ADD_WRITE          2
#define HG_UPSTREAM_ADD_TIME           4

#define HG_UPSTREAM_NR_HANDLER         8
#define HG_UPSTREAM_NW_HANDLER        16

//超时关闭和超时继续
#define HG_UPSTREAM_TO_CLOSE          32
#define HG_UPSTREAM_TO_AGAIN          64
//使用管道传输
#define HG_UPSTREAM_USE_PIPE         128


int hg_upstream_activate(void *data);
int hg_upstream_finish(hg_upstream_t *upstream,int rc);
int hg_upstream_del_timeout(hg_upstream_t *upstream);
int hg_upstream_add_timeout(hg_upstream_t *upstream,unsigned int flag,unsigned long long msec);

struct hg_upstream_info_t{
     unsigned int flag=HG_UPSTREAM_UNKOWN;
     unsigned long long  msec;
}; 

/*
#define HG_PIPE_DEFAULT_UPSTREAM_TIMEOUT              30000
#define HG_PIPE_DEFAULT_DOWNSTREAM_TIMEOUT            30000
#define HG_PIPE_DEFAULT_BUFFER_SIZE                    8192
#define HG_PIPE_DEFAULT_MAX_FILE_BUFFER_SIZE  1024*1024*100
#define HG_PIPE_DEFAULT_LIMIT                    2147483647
#define HG_PIPE_DEFAULT_USE_FILE_BUFFER               false
#define HG_PIPE_DEFAULT_CONNECT_TIMEOUT               30000
#define HG_PIPE_DEFAULT_WRITE_TIMEOUT                 30000
#define HG_PIPE_DEFAULT_READ_TIMEOUT                  30000
#define HG_PIPE_DEFAULT_RECV_BUFFER_SIZE               8192
#define HG_PIPE_DEFAULT_REUSE_BUFFER                  false
*/

struct hg_upstream_addr_t{
     cris_str_t  ip;
     unsigned short port=0;
     hg_upstream_addr_t *next=NULL;
};

struct hg_upstream_server_t{
     cris_str_t  name;
     cris_str_t  domain;
     unsigned short domain_port;
     hg_upstream_addr_t *addrs=NULL;

     hg_upstream_cluster_t *cluster=NULL;//服务器所属集群
     unsigned int weight=1;
     hg_upstream_server_t *pre=NULL;
     hg_upstream_server_t *next=NULL;
};

struct hg_upstream_balance_ctx{
       
     hg_upstream_server_t *cur=NULL;
     hg_upstream_server_t *head=NULL;//服务器链表的头结点
     unsigned  int count=0;//当前服务器接收请求次数
};


typedef void* (*hg_upstream_create_data_pt)(cris_mpool_t *pool);
typedef int (*hg_upstream_init_balance_pt)(hg_upstream_cluster_t *cluster);

//负载均衡地址选择器
typedef hg_upstream_server_t* (*hg_upstream_load_balance_pt)(hg_upstream_cluster_t *cluster);

typedef hg_upstream_server_t* (*hg_upstream_balance_failure)(hg_upstream_server_t*server);

struct hg_upstream_load_balance_t{
     cris_str_t name;
     hg_upstream_load_balance_pt handler=NULL;
     hg_upstream_create_data_pt  create_data_handler=NULL;//负载均衡需要的依赖数据
     hg_upstream_init_balance_pt init_balance_handler=NULL;
     hg_upstream_balance_failure failure_handler=NULL;//连接失败后的回调函数
};

//上游服务器集群
struct hg_upstream_cluster_t{
     cris_str_t name;
     cris_mpool_t *pool=NULL;
     std::vector<hg_upstream_server_t*> servers;
     hg_upstream_load_balance_t *balance=NULL;
     hg_upstream_load_balance_pt balance_handler=NULL;
     void*data;
     bool shared=false;
     unsigned short  count=0;//服务器数量
};


struct hg_pipe_conf_t{
    unsigned int upstream_timeout=30000;
    unsigned int downstream_timeout=30000;
    unsigned int buffer_size=4096*2;//8192b,8kb
    unsigned int max_file_buffer_size=1024*1024*100;//100mb   
    unsigned int limit=2147483647;//该位用于限速   
    bool         use_file_buffer=false;
};


struct hg_upstream_conf_t{

     unsigned int connect_timeout=30000;       
     unsigned int write_timeout=30000;
     unsigned int read_timeout=30000;
     unsigned int recv_buffer_size=8192;//默认接收缓冲大小
     unsigned short retry_count=0;//单台服务器的单个ip地址的重试次数
     unsigned short server_retry_count=65535;//使用集群时，默认重试服务器的个数
     hg_upstream_cluster_t *cluster=NULL;
     hg_upstream_server_t *server=NULL;//独立server

     bool reuse_buffer=false;//复用out缓冲
     hg_pipe_conf_t pipe_conf;

};



struct hg_upstream_t{
    
    void *data=NULL;
    cris_http_request_t *r=NULL;
    hg_connection_t *conn=NULL;
    cris_mpool_t *pool=NULL;

    hg_pipe_t *pipe=NULL;

    cris_str_t *host=NULL;//指定ip
    unsigned short   port=0;//
    unsigned short   retry_count=0;
    unsigned short   server_retry_count=0;

    bool connect=false;
    bool skip_retry=false;
    hg_upstream_server_t *server=NULL;//表示当前使用的服务器

    hg_upstream_conf_t *conf=NULL;

    int upstream_state=HG_UPSTREAM_INITIAL;
    int parse_state=HG_UPSTREAM_PARSE_HEADER;    

    /****返回HG_OK进行下一阶段，HG_AGAIN继续处理,HG_ERROR产生错误并结束****/
    int (*hg_upstream_create_request)(cris_buf_t **,void*,hg_upstream_info_t *)=NULL;
    int (*hg_upstream_retry_request)(void*)=NULL;
    int (*hg_upstream_parse_header)(cris_buf_t **,void*,hg_upstream_info_t *)=NULL;
    int (*hg_upstream_parse_body)(cris_buf_t **,void*,hg_upstream_info_t *)=NULL;
    int (*hg_upstream_post_upstream)(void*,int)=NULL;

    hg_upstream_t *next=NULL;
};


struct hg_pipe_res_t{
    int cnt;//本次过滤的数据量
    int rc;//函数运行状态
    int res;//剩余需要过滤的数据量
};


typedef hg_pipe_res_t(*hg_pipe_filter_pt)(cris_buf_t *des,cris_buf_t *src,void *ctx);


#define HG_PIPE_NO_RAW         1
#define HG_PIPE_CANNT_WRITE    2
#define HG_PIPE_WAIT_WRITE     4
#define HG_PIPE_RECV_FINISH    8
#define HG_PIPE_IN_FINISH     16
#define HG_PIPE_OUT_FINISH    32
#define HG_PIPE_USE_FILE      64

struct hg_pipe_t{

    void* out_filter_ctx=NULL;
    void* in_filter_ctx=NULL;
    hg_upstream_t *upstream;
    hg_pipe_conf_t *conf=NULL;

    cris_mpool_t *pool=NULL;

    cris_buf_t *raw=NULL;
    cris_buf_t *in=NULL;
    cris_buf_t *out=NULL;
    cris_buf_t *pkt=NULL;//过滤完成形成的包

    cris_buf_t *origin_in_buf=NULL;//in驱动连接上的原始缓冲in
    cris_buf_t *origin_out_buf=NULL;

    int  file_fd=0;
    unsigned int  file_size=0;
    unsigned int  file_cur_pos=0;
    unsigned int  file_res_size=0;

    unsigned int  res_upstream=0;
    unsigned int  res_downstream=0;
    
    unsigned long long send_time=0;//上次的发送时间
    unsigned long long wait_time=0;//需要等待的时间

    unsigned flag=0;

    void* origin_in_data=NULL;//
    void* origin_out_data=NULL;//

    hg_pipe_filter_pt  out_filter=NULL;
    hg_pipe_filter_pt  in_filter=NULL;

    hg_connection_t *in_driver=NULL;
    hg_connection_t *out_driver=NULL;
    
    bool need=false;//是否需要读驱动
    bool canntwrite=false;//是否处于无法写状态,防止共用缓冲时，重复过滤
    bool error=false;//在使用文件缓冲的模式下，该成员用于确认，对端事件是否正常

    bool cmpt_in=false;//是否完成in过滤
    bool cmpt_out=false;//是否完成out过滤
};






#endif
















