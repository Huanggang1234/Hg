#ifndef HG_UPSTREAM_MODULE_H
#define HG_UPSTREAM_MODULE_H
#include<cstddef>

#define HG_UPSTREAM_INITIAL             0
#define HG_UPSTREAM_CREATE_REQUEST      1
#define HG_UPSTREAM_SEND_REQUEST        2
#define HG_UPSTREAM_PARSE               3
#define HG_UPSTREAM_END                 4

#define HG_UPSTREAM_PARSE_HEADER        0
#define HG_UPSTREAM_PARSE_BODY          1 


struct hg_connection_t;
struct cris_mpool_t;
struct cris_buf_t;
struct cris_http_request_t;
struct hg_upstream_t;
struct hg_upstream_conf_t;
struct hg_upstream_info_t;

hg_upstream_t* hg_add_upstream(cris_http_request_t *r);



#define HG_UPSTREAM_UNKOWN             0
#define HG_UPSTREAM_ADD_READ           1
#define HG_UPSTREAM_ADD_WRITE          2
#define HG_UPSTREAM_ADD_TIME           4

#define HG_UPSTREAM_NR_HANDLER         8
#define HG_UPSTREAM_NW_HANDLER        16

//超时关闭和超时继续
#define HG_UPSTREAM_TO_CLOSE          32
#define HG_UPSTREAM_TO_AGAIN          64



int hg_upstream_activate(void *data);
int hg_upstream_finish(hg_upstream_t *upstream,int rc);
int hg_upstream_del_timeout(hg_upstream_t *upstream);
int hg_upstream_add_timeout(hg_upstream_t *upstream,unsigned int flag,unsigned long long msec);


struct hg_upstream_info_t{
     unsigned int flag=HG_UPSTREAM_UNKOWN;
     unsigned long long  msec;
}; 


struct hg_upstream_t{
    
    void *data=NULL;
    cris_http_request_t *r=NULL;
    hg_connection_t *conn=NULL;

    cris_str_t *host=NULL;
    unsigned short   port=0;
    unsigned short   retry_count=0;

    bool connect=false;

    int connect_timeout=10000;
    int write_timeout=10000;
    int read_timeout=10000;
    int recv_buf_size=65536;

    int upstream_state=HG_UPSTREAM_INITIAL;
    int parse_state=HG_UPSTREAM_PARSE_HEADER;    

    /****返回HG_OK进行下一阶段，HG_AGAIN继续处理,HG_ERROR产生错误并结束****/
    int (*hg_upstream_create_request)(cris_buf_t **,void*,hg_upstream_info_t *)=NULL;
    int (*hg_upstream_parse_header)(cris_buf_t **,void*,hg_upstream_info_t *)=NULL;
    int (*hg_upstream_parse_body)(cris_buf_t **,void*,hg_upstream_info_t *)=NULL;
    int (*hg_upstream_post_upstream)(void*,int)=NULL;

    hg_upstream_t *next=NULL;
};








#endif












