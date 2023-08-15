#ifndef HG_HTTP_PARSE
#define HG_HTTP_PARSE
#include"../include/hg_http_parse.h"
#include"../include/hg_http_module.h"
#include"../base/cris_buf.h"

#define  str3_cmp(m,c0,c1,c2)             \
            m[0]==c0&&m[1]==c1&&m[2]==c2

#define  str4_cmp(m,c0,c1,c2,c3)          \
            m[0]==c0&&m[1]==c1&&m[2]==c2&&m[3]==c3

#define  str6_cmp(m,c0,c1,c2,c3,c4,c5)       \
            m[0]==c0&&m[1]==c1&&m[2]==c2&&m[3]==c3&&m[4]==c4&&m[5]==c5

#define  str10_cmp(m,c0,c1,c2,c3,c4,c5,c6,c7,c8,c9)    \
            m[0]==c0&&m[1]==c1&&m[2]==c2&&m[3]==c3&&m[4]==c4&&   \
            m[5]==c5&&m[6]==c6&&m[7]==c7&&m[8]==c8&&m[9]==c9





int hg_http_request_parse(cris_http_request_t *r,cris_buf_t *buf){

   
   cris_mpool_t  *pool=r->pool;

   char *p=buf->cur;
   char *e=buf->last;
    
   int state=r->parse_state;


    for(;p<e;p++){


            char ch= *p;

        switch(state) {

            case request_start:

                  r->entire_request.str=p;//整个请求缓冲的起始位置，记录该信息有助于快速转发
                  r->pre=p;
                      
                  state=method;

                  break;

            case method:

                  if(ch==' '){

                       state=blank_before_url;
                        
                       r->method.str=r->pre;
                       r->method.len=p-r->pre;
                  }
                    
                  break;

            case blank_before_url:
                  
                  if(ch=='/'){
                       
                      state=url;                  
  
                      r->url.str=p;     
                      r->pre=p;
               
                  }else
                      goto error_request;
                    
                  break;                 

            case url :

                  if(ch==' '){

                       r->url.len=p-r->pre;

                       state=blank_before_edition;

                  }

                  if(ch=='?'){

		       r->url_param.str=p+1;
                       state=url_param;
		  }

                  break;

            case url_param:

                  if(ch==' '){
		  
                        r->url.len=p-r->pre;
                        r->url_param.len=p-r->url_param.str;
		          
                        if(r->url_param.len==0)
			   r->url_param.str=NULL;
                        
			state=blank_before_edition;
		  }

                  break;

            case blank_before_edition:

                  if(ch=='H'){

                      state=edition;

                  }else
                      goto error_request;

                  break;

            case edition:
 
                  if(ch=='\r'){
                      
                      state=edition_cr;

                  }

                  break;            

            case edition_cr:
                          
                  if(ch=='\n'){
                   
                      state=head_name_start;
                  }else
                      goto error_request;
                  
                  break;

            case edition_lf:

                  break;     
         
            case head_name_start:

                  r->pre=p;

                  state=name;

                  break;

            case name:
            
                  if(ch==':'){
                            
                     state=gap;

                  }
                         
                 break;

            case gap://该状态下p所指的是分隔符后面一个字符,该符号是一个空格

		 r->head_tmp=(cris_http_header_t*)pool->qlloc(sizeof(cris_http_header_t));

                 r->head_tmp->name.str=r->pre;

                 r->head_tmp->name.len=p-r->pre-1;

                 r->pre=p+1;//记录参数起始位置，分隔符后有一个空格跳过

                 state=avg;

                 break;

            case avg:

                 if(ch=='\r'){

                      state=avg_cr;

                      r->head_tmp->content.str=r->pre;

                      r->head_tmp->content.len=p-r->pre;

                      switch(r->head_tmp->name.len){

                           case 4:
    
                                 if(str4_cmp(r->head_tmp->name.str,'H','o','s','t'))
                                     r->headers_in.host=r->head_tmp;
                                 else
                                     hg_add_new_head(r,r->head_tmp);         
                            
                                 break;

                           case 6:
                                                                       
                                 if(str6_cmp(r->head_tmp->name.str,'A','c','c','e','p','t'))
                                     r->headers_in.accept=r->head_tmp;
                                 else if(str6_cmp(r->head_tmp->name.str,'C','o','o','k','i','e'))
                                     r->headers_in.cookie=r->head_tmp;
                                 else
                                     hg_add_new_head(r,r->head_tmp);
  
                                 break;

                           case 10:                              
                                 if(str10_cmp(r->head_tmp->name.str,'C','o','n','n','e','c','t','i','o','n'))
                                     r->headers_in.connection=r->head_tmp;
                                 else if(str10_cmp(r->head_tmp->name.str,'U','s','e','r','-','a','g','e','n','t'))
                                     r->headers_in.user_agent=r->head_tmp;
                                 else
                                     hg_add_new_head(r,r->head_tmp);
                                     
                                 break;

                           case 12:
                                    
                                  if(!strncmp(r->head_tmp->name.str,"Content-Type",12))
                                     r->headers_in.content_type=r->head_tmp;
                                  else                        
                                     hg_add_new_head(r,r->head_tmp);
                                  
                                 break;

                           case 13:

                                  if(!strncmp(r->head_tmp->name.str,"Cache-Control",13))
                                     r->headers_in.cache_control=r->head_tmp;
                                  else                        
                                     hg_add_new_head(r,r->head_tmp);
                                  
                                 break;


                           case 14:

                                  if(!strncmp(r->head_tmp->name.str,"Content-Length",14))
                                     r->headers_in.content_length=r->head_tmp;
                                  else                        
                                     hg_add_new_head(r,r->head_tmp);
                                  
                                 break;

                           case 15:

                                  if(!strncmp(r->head_tmp->name.str,"Accept-Encoding",15))
                                     r->headers_in.accept_encoding=r->head_tmp;
                                  else if(!strncmp(r->head_tmp->name.str,"Accept-Language",15))
                                     r->headers_in.accept_language=r->head_tmp;
                                  else                        
                                     hg_add_new_head(r,r->head_tmp);
                                  
                                 break;

                           default:
                                 hg_add_new_head(r,r->head_tmp);
                                 break;

                      }


                 }

                 break;

            case avg_cr:
                  
                  if(ch=='\n')
                     state=avg_lf;
                  else
                     goto error_request;

                  break;                  

            case avg_lf:

                  if(ch!='\r'){//仍然是头部的情况
                     r->pre=p;
 
                     state=name;//将状态转移到name,并记录该位置

                  }else
                     state=cr_before_body;

                  break;

            case cr_before_body:

                  if(ch=='\n')
                        
                      state=lf_before_body;
                  else
                      goto error_request;

                  break;

            default:
                  goto error_request;

        }
       
        if(state==lf_before_body)
               break;
    }

    if(p==e){//如果没有解析完成，保存状态等待下一次回调

       r->parse_state=state;

       buf->cur=buf->last;

       return HG_AGAIN;


    }

    if(state==lf_before_body){

        r->entire_request.len=p-r->entire_request.str+1;
    
        //*p等于\n
        buf->cur=p+1;    

       /* 测试代码  打印链表中的头部信息  printf("lf_before_body\n");

        cris_node_t *node=&r->headers_in.headers_list.head;        

        while(node->next!=NULL){

            cris_http_header_t *header=(cris_http_header_t*)node->next->data;
            cris_str_print(&header->name);
            printf("+");
            cris_str_print(&header->content);
            node=node->next;
        }*/

        return  HG_OK;

    }


    error_request:
 
        return  HG_ERROR;
}

#endif

