#ifndef HG_HTTP_PROXY_MODULE_H
#define HG_HTTP_PROXY_MODULE_H
#include"../../base/include/cris_str.h"
#include"../../include/hg_http_module.h"
#include"../hg.h"

struct hg_http_proxy_loc_conf_t{       
       bool proxy=false;
       cris_str_t host;
       unsigned short port=0;
};


struct hg_upstream_t;
struct cris_buf_t;

struct hg_http_proxy_ctx_t{
       hg_upstream_t *up=NULL;
       cris_mpool_t *pool=NULL;

       bool set_cookie=false;

       cris_http_header_t *headers=NULL;//由头部组成的单链表

       cris_str_t  entire_response;
       
       unsigned int content_length_n=0;
       unsigned int response_start_pos=0;

       unsigned int pre=0;
       unsigned int parse_state=0;
       cris_http_header_t *tmp_header=NULL;
};


#define HG_RESPONSE_INIT            0
#define HG_RESPONSE_VERSION         1
#define HG_RESPONSE_VERSION_BK      2
#define HG_RESPONSE_CODE            3
#define HG_RESPONSE_CODE_BK         4
#define HG_RESPONSE_INFO            5
#define HG_RESPONSE_LR              6
#define HG_RESPONSE_CR              7
#define HG_RESPONSE_HEAD_NAME       8
#define HG_RESPONSE_HEAD_GAP        9
#define HG_RESPONSE_HEAD_CONTENT   10
#define HG_RESPONSE_LRLR           11
#define HG_RESPONSE_CRCR           12



int hg_http_response_parse(hg_http_proxy_ctx_t *ctx, cris_buf_t *buf);

extern hg_module_t hg_http_proxy_module;

#endif









