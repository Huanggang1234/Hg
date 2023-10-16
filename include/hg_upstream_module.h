#ifndef HG_UPSTREAM_MODULE_H
#define HG_UPSTREAM_MODULE_H
#include<cstddef>

#define HG_UPSTREAM_INITIAL             0
#define HG_UPSTREAM_CREATE_REQUEST      1
#define HG_UPSTREAM_SEND_REQUEST        2
#define HG_UPSTREAM_PIPE_BACK           3
#define HG_UPSTREAM_PARSE               4
#define HG_UPSTREAM_END                 5
#define HG_UPSTREAM_ERROR               6


#define HG_UPSTREAM_PARSE_HEADER        0
#define HG_UPSTREAM_PARSE_BODY          1 


struct hg_connection_t;
struct cris_mpool_t;
struct cris_buf_t;
struct cris_http_request_t;
struct hg_upstream_t;
struct hg_upstream_conf_t;
struct hg_upstream_info_t;
struct hg_pipe_t;
struct hg_pipe_conf_t;


hg_upstream_t* hg_add_upstream(cris_http_request_t *r,hg_upstream_conf_t *conf);
hg_pipe_t* hg_add_pipe(hg_upstream_t *up);

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
     unsigned short retry_count=0;
     bool reused_buffer=false;//复用out缓冲

     hg_pipe_conf_t pipe_conf;

};



struct hg_upstream_t{
    
    void *data=NULL;
    cris_http_request_t *r=NULL;
    hg_connection_t *conn=NULL;
    cris_mpool_t *pool=NULL;

    hg_pipe_t *pipe=NULL;

    cris_str_t *host=NULL;
    unsigned short   port=0;
    unsigned short   retry_count=0;

    bool connect=false;

    hg_upstream_conf_t *conf=NULL;

    int upstream_state=HG_UPSTREAM_INITIAL;
    int parse_state=HG_UPSTREAM_PARSE_HEADER;    

    /****返回HG_OK进行下一阶段，HG_AGAIN继续处理,HG_ERROR产生错误并结束****/
    int (*hg_upstream_create_request)(cris_buf_t **,void*,hg_upstream_info_t *)=NULL;
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
















