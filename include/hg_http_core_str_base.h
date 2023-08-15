#ifndef HG_HTTP_CORE_STR_BASE_H
#define HG_HTTP_CORE_STR_BASE_H
#include<unordered_map>
#include<string>
#include"hg_define.h"


const char *HG_HTTP_200_OK_STR="HTTP/1.1 200 OK\r\n";


const char *HG_HTTP_403_FORBIDDEN_STR="HTTP/1.1 403 Forbidden\r\n";


const char *HG_HTTP_404_NOT_FOUND_STR="HTTP/1.1 404 Not Found\r\n";


const char *HG_HTTP_500_INTERNAL_ERROR_STR="HTTP/1.1 500 Internal Server Error\r\n";


#define HG_200_LEN    17
#define HG_403_LEN    24
#define HG_404_LEN    24
#define HG_500_LEN    36


const char * HG_RESPONSE_LINE[]={
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      HG_HTTP_200_OK_STR,
      HG_HTTP_403_FORBIDDEN_STR,
      HG_HTTP_404_NOT_FOUND_STR,
      HG_HTTP_500_INTERNAL_ERROR_STR
};


unsigned int HG_RESPONSE_LINE_LEN[]={
      0,
      0,
      0,
      0,
      0,
      HG_200_LEN,
      HG_403_LEN,
      HG_404_LEN,
      HG_500_LEN
};



#endif





