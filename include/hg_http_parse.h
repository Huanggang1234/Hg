#ifndef HG_HTTP_PARSE_H
#define HG_HTTP_PARSE_H

#include"hg_define.h"

enum{
    
    request_start=0,

    method,

    blank_before_url,

    url,

    url_param,

    blank_before_edition,

    edition,

    edition_cr,

    edition_lf,

    head_name_start,

    name,

    gap,

    avg,

    avg_cr,

    avg_lf,

    cr_before_body,

    lf_before_body,
    
};//请求的解析状态

struct cris_http_request_t;
struct cris_buf_t;

int hg_http_request_parse(cris_http_request_t *r,cris_buf_t *buf);//返回HG_OK表已经解析出完整的,HG_AGAIN表希望继续输入，HG_ERROR解析出现错误


#endif



