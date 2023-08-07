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

int hg_upstream_activate(void *);

hg_upstream_t* hg_add_upstream(cris_http_request_t *r);

struct hg_upstream_conf_t{

    char * upstream_ip;
    unsigned short upstream_port;

};

struct hg_upstream_t{
    
    void *data=NULL;
    cris_http_request_t *r=NULL;
    hg_connection_t *conn=NULL;

    int upstream_state=HG_UPSTREAM_INITIAL;
    int parse_state=HG_UPSTREAM_PARSE_HEADER;
    
    /****返回HG_OK进行下一阶段，HG_AGAIN继续处理,HG_ERROR产生错误并结束****/
    int (*hg_upstream_create_request)(cris_buf_t **,void*)=NULL;
    int (*hg_upstream_parse_header)(cris_buf_t **,void*)=NULL;
    int (*hg_upstream_parse_body)(cris_buf_t **,void*)=NULL;
    int (*hg_upstream_post_parse_body)(cris_buf_t **,void*)=NULL;
    int (*hg_upstream_post_upstream)(void*,int)=NULL;

    hg_upstream_t *next=NULL;
};








#endif












