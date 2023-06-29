#ifndef HG_HTTP_CORE_STR_BASE_H
#define HG_HTTP_CORE_STR_BASE_H
#include<unordered_map>
#include<string>
#include"hg_define.h"
struct str_info{
   char const*  str;
   int len;
};

str_info h200={
   "HTTP/1.1 200 OK\r\n",
   17
};


str_info h403={
   "HTTP/1.1 403 Forbidden\r\n",
    24
};

str_info h404={
   "HTTP/1.1 404 Not Found\r\n",
    24
};

str_info h500={
   "HTTP/1.1 500 Internal Server Error\r\n",
    36
};

std::unordered_map<int,str_info> hg_rp_line={
    {
      HG_HTTP_OK,
      h200
    },
    {
      HG_HTTP_FORBIDDEN,
      h403
    },
    {
      HG_HTTP_NOT_FOUND,
      h404
    },
    {
      HG_HTTP_INTERNAL_SERVER_ERROR,
      h500
    }
};


str_info _type_html={
  "Content-Type: text/html\r\n",
   25
};

str_info _type_plain={
  "Content-Type: text/plain\r\n",
   26
};

str_info _type_jpg={
  "Content-Type: image/jpg\r\n",
   25
};

str_info _type_webp={
  "Content-Type: image/webp\r\n",
   26
};

std::unordered_map<int,str_info> hg_types={
    {
      TYPE_TEXT_HTML,
      _type_html
    },
    {
      TYPE_TEXT_PLAIN,
      _type_plain
    },
    {
      TYPE_IMAGE_JPG,
      _type_jpg
    },
    {
      TYPE_IMAGE_WEBP,
      _type_webp,
    }
};


std::unordered_map<std::string,int> set_types={
    {
       "text/plain",
        TYPE_TEXT_PLAIN
    },
    {
       "text/html",
        TYPE_TEXT_HTML
    },
    {
       "image/jpg",
        TYPE_IMAGE_JPG
    },
    {
       "image/webp",
        TYPE_IMAGE_WEBP
    }
};






#endif





