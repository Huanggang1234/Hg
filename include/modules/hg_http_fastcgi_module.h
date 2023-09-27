#ifndef HG_HTTP_FASTCGI_MODULE_H
#define HG_HTTP_FASTCGI_MODULE_H

//自定义
#define FCGI_UNKOWN_REQUEST      0

#define FCGI_BEGIN_REQUEST       1
#define FCGI_ABORT_REQUEST       2
#define FCGI_END_REQUEST         3
#define FCGI_PARAMS              4
#define FCGI_STDIN               5
#define FCGI_STDOUT              6
#define FCGI_STDERR              7
#define FCGI_DATA                8
#define FCGI_GET_VALUES          9
#define FCGI_GET_VALUES_RESULT  10
#define FCGI_UNKNOWN_TYPE       11
#define FCGI_MAXTYPE (FCGI_UNKNOWN_TYPE)

typedef struct{
   unsigned char version;
   unsigned char type;
   unsigned char requestIdB1;
   unsigned char requestIdB0;
   unsigned char contentLengthB1;
   unsigned char contentLengthB0;
   unsigned char paddingLength;
   unsigned char reserved;
}FCGI_Header;

#define FCGI_HEADER_LEN   8

#define FCGI_VERSION_1    1

typedef struct{
    unsigned char roleB1;
    unsigned char roleB0;
    unsigned char flags;
    unsigned char reserved[5];
}FCGI_BeginRequestBody;

typedef struct{
    FCGI_Header header;
    FCGI_BeginRequestBody body;
}FCGI_BeginRequestRecord;

#define FCIG_KEEP_CONN    1


#define FCGI_KEEP_CONN    1

#define FCGI_RESPONDER    1
#define FCGI_AUTHORIZER   2
#define FCGI_FILTER       3

typedef struct{
    unsigned char appStatusB3;
    unsigned char appStatusB2;
    unsigned char appStatusB1;
    unsigned char appStatusB0;
    unsigned char protocolStatus;
    unsigned char reserved[3];
}FCGI_EndRequestBody;

typedef struct{
    FCGI_Header header;
    FCGI_EndRequestBody body;
}FCGI_EndRequestRecord;


#define FCGI_REQUEST_COMPLETE 0
#define FCGI_CANT_MAX_CONN    1
#define FCGI_OVERLOADED       2
#define FCGI_UNKOWN_ROLE      3


#define FCGI_MAX_CONNS  "FCGI_MAX_CONNS"
#define FCGI_MAX_REQS   "FCGI_MAX_REQS"
#define FCGI_MPXS_CONNS "FCGI_MPXS_CONNS"

typedef struct{
  unsigned char type;
  unsigned char reserved[7];
}FCGI_UnkownTypeBody;

typedef struct{
  FCGI_Header header;
  FCGI_UnkownTypeBody body;
}FCGI_UnkownTypeRecord;

typedef struct{
  unsigned char nameLengthB0;
  unsigned char valueLengthB0;
}FCGI_NameValueLen;

typedef struct{
  FCGI_Header header;
  FCGI_NameValueLen len;
}FCGI_ParamRecord;

/*******************************************************/

#include"../../base/include/cris_str.h"
#include"../hg_epoll_module.h"


#define HG_FCGI_VAR_UNKOWN             0
#define HG_FCGI_VAR_CONTENT_LENGTH     1
#define HG_FCGI_VAR_COOKIE             2
#define HG_FCGI_VAR_CONTENT_TYPE       3
#define HG_FCGI_VAR_URL_PARAM          4
#define HG_FCGI_VAR_METHOD             5
#define HG_FCGI_VAR_FILE_NAME          6

struct hg_fastcgi_param{
    
    cris_str_t  name;
    cris_str_t  content;
    FCGI_ParamRecord  param;

    bool use_variable=false;
    unsigned int  variable=0;
   
    hg_fastcgi_param *next=NULL;
};

struct hg_http_fastcgi_loc_conf_t{
     
    cris_str_t  host;
    unsigned short port=0;

    cris_str_t  authorization_host;
    unsigned short authorization_port=0;

    hg_fastcgi_param *params=NULL;
    bool     on=false;
    bool     authorization=false;

/***********************************/
};

struct cris_http_request_t;
struct cris_mpool_t;
struct cris_buf_t;
struct hg_upstream_t;

#define HG_FCGI_INITIAL            0
#define HG_FCGI_RD_BODY            1
#define HG_FCGI_FORWARD            2
#define HG_FCGI_PARSE_HEADER       3
#define HG_FCGI_PARSE_REAL_BODY    4
#define HG_FCGI_PARSE_PROTOCOL_END 5
#define HG_FCGI_PARSE_END          6

struct hg_http_fastcgi_ctx_t{

      cris_http_request_t *http_request=NULL;
      cris_mpool_t *pool=NULL;

      hg_http_fastcgi_loc_conf_t *conf=NULL;
     
      cris_buf_t *buf=NULL;

      cris_buf_t *response=NULL;

      hg_upstream_t *upstream=NULL;

      bool listen_origin=false;

      bool access_upstream=false;//与上游传输是否成功
/***********以下为一些必须的成员**************/

      int state=HG_FCGI_INITIAL;
      unsigned char  type=FCGI_UNKOWN_REQUEST;//当前正在解析的请求的类型

      unsigned short request_id=0;

      unsigned short content_length=0;
      unsigned short recv_content_length=0;
      unsigned char padding_length=0;
      unsigned char recv_padding=0;

      unsigned int   app_status=0;
      unsigned char  protocol_status=0;

      unsigned int   content_off=0;
      bool     forward_upstream=true;//是否直接转发上游响应
      bool     forward_body=true;
};


extern hg_module_t hg_http_fastcgi_module;









#endif












