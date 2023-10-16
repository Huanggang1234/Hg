#ifndef HG_HTTP_PROXY_MODULE_CPP
#define HG_HTTP_PROXY_MODULE_CPP
#include"../../include/hg.h"
#include"../../include/hg_define.h"
#include"../../include/hg_conf_parse.h"
#include"../../include/modules/hg_http_proxy_module.h"
#include"../../include/hg_http_module.h"
#include"../../include/hg_http_core_module.h"
#include"../../include/hg_upstream_module.h"
#include"../../include/hg_conf_parse.h"
#include"../../include/hg_http_parse.h"
#include<cstdlib>
#include<cstdio>
#include"../../include/hg_epoll_module.h"


static int hg_http_proxy_set_pass(hg_module_t *module,hg_cycle_t *cycle,cris_conf_t *conf);

void *hg_http_proxy_create_loc_conf(hg_cycle_t *cycle);
int hg_http_proxy_init_loc_conf(hg_module_t *module,hg_cycle_t *cycle,hg_http_conf_t *conf);

int hg_http_proxy_handler(cris_http_request_t *r);

int hg_http_proxy_create_request(cris_buf_t **out_buffer,void*data,hg_upstream_info_t *info);
int hg_http_proxy_parse(cris_buf_t **out_buffer,void*data,hg_upstream_info_t *info);
int hg_http_proxy_parse_body(cris_buf_t **in_buffer,void *data,hg_upstream_info_t *info);
int hg_http_proxy_post_upstream(void*data,int rc);

static std::vector<hg_command_t> commands={
    {
       std::string("proxy_pass"),
       HG_CMD_LOCATION,
       &hg_http_proxy_set_pass
    }
};


static hg_http_module_t hg_http_proxy_ctx={

	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	&hg_http_proxy_create_loc_conf,
	&hg_http_proxy_init_loc_conf,
	NULL,
	NULL,
	NULL
};

hg_module_t hg_http_proxy_module={
      HG_HTTP_MODULE,
      0,
      0,
      &hg_http_proxy_ctx,
      &commands,
      NULL,
      NULL,
      NULL,
      NULL      
};



static int hg_http_proxy_set_pass(hg_module_t *module,hg_cycle_t *cycle,cris_conf_t *conf){

      int num=conf->avgs.size();

      if(num!=2)
         return HG_ERROR;
      
      hg_http_module_t *http_m=(hg_http_module_t*)module->ctx;
      hg_http_conf_t *parent_conf=http_m->ptr;

      hg_http_proxy_loc_conf_t *loc=hg_get_loc_conf(hg_http_proxy_loc_conf_t,module,parent_conf);

      loc->host=conf->avgs.front();

      conf->avgs.pop_front();

      cris_str_t port=conf->avgs.front();
       
      loc->port=atoi(port.str);

      loc->proxy=true;

      return HG_OK;
}


void* hg_http_proxy_create_loc_conf(hg_cycle_t *cycle){

      cris_mpool_t *pool=cycle->pool;

      return (void*)(new (pool->qlloc(sizeof(hg_http_proxy_loc_conf_t)))hg_http_proxy_loc_conf_t());
}

int hg_http_proxy_init_loc_conf(hg_module_t *module,hg_cycle_t *cycle,hg_http_conf_t *conf){

      hg_http_proxy_loc_conf_t *loc=hg_get_loc_conf(hg_http_proxy_loc_conf_t,module,conf);
       
      if(loc->proxy)
        hg_http_add_spacial_request_handler(&hg_http_proxy_handler,conf);

      return HG_OK;
}




int hg_http_proxy_handler(cris_http_request_t *r){

    hg_upstream_t *up=hg_add_upstream(r,NULL);//添加上游模块信息

    hg_http_add_asyn_event(r,HG_ASYN_UPSTREAM);//添加异步事件

    hg_http_proxy_loc_conf_t *loc=hg_get_loc_conf(hg_http_proxy_loc_conf_t,(&hg_http_proxy_module),(r->loc_conf));

    hg_http_proxy_ctx_t *ctx=new (r->pool->qlloc(sizeof(hg_http_proxy_ctx_t)))hg_http_proxy_ctx_t();

    ctx->up=up;
    ctx->pool=r->pool;

    up->data=(void*)ctx;

    up->host=&loc->host;

    up->port=loc->port;

    up->hg_upstream_create_request=&hg_http_proxy_create_request;

    up->hg_upstream_parse_header=&hg_http_proxy_parse;

    up->hg_upstream_parse_body=&hg_http_proxy_parse_body;

    up->hg_upstream_post_upstream=&hg_http_proxy_post_upstream;  

    return HG_OK;
}


int hg_http_proxy_create_request(cris_buf_t **out_buffer,void*data,hg_upstream_info_t *info){

    hg_http_proxy_ctx_t *ctx=(hg_http_proxy_ctx_t*)data;

    hg_upstream_t *up=ctx->up;

    cris_http_request_t *r=up->r;

    hg_connection_t *conn=r->conn;

    hg_pipe_t *pipe=hg_add_pipe(up);

    cris_buf_t *buf=r->conn->in_buffer;

    pipe->raw=new (r->pool->qlloc(sizeof(cris_buf_t)))cris_buf_t(r->pool,r->entire_request.str,r->entire_request.len+r->recv_body);

//    pipe->raw=buf;
    pipe->res_upstream=r->entire_request.len+r->content_length;//完整的请求长度+包体长度

     pipe->in_driver=r->conn;
     pipe->out_driver=up->conn;

     info->flag|=HG_UPSTREAM_USE_PIPE;

    //不使用文件缓冲
     return HG_AGAIN;//必须使用HG_AGAIN
}


int hg_http_proxy_parse(cris_buf_t **out_buffer,void*data,hg_upstream_info_t *info){

    
    hg_http_proxy_ctx_t *ctx=(hg_http_proxy_ctx_t*)data;
   
    hg_upstream_t *up=ctx->up;

    int rc=hg_http_response_parse(ctx,*out_buffer);

    if(rc==HG_AGAIN){
        info->flag|=HG_UPSTREAM_ADD_READ;
	info->flag|=HG_UPSTREAM_ADD_TIME;
	info->flag|=HG_UPSTREAM_TO_CLOSE;
	info->msec=5000;
	return HG_AGAIN;
    }
    
    if(rc==HG_ERROR){
        return HG_ERROR;
    }

    hg_pipe_t *pipe=hg_add_pipe(up);

    cris_buf_t *buf=*out_buffer;  

    buf->cur=buf->start;

    pipe->raw=new (pipe->pool->qlloc(sizeof(cris_buf_t)))cris_buf_t(*buf);

    pipe->res_upstream=ctx->entire_response.len+ctx->content_length_n;

    printf("res %d used=%d\n",pipe->res_upstream,buf->used);

    pipe->in_driver=up->conn;
    pipe->out_driver=up->r->conn;

    info->flag|=HG_UPSTREAM_USE_PIPE;

    return HG_AGAIN;
/*
    printf("hg_http_proxy_parse\n");

    hg_upstream_t *up=(hg_upstream_t*)data;

    cris_buf_t *buf=*out_buffer;

    int cnt=buf->available();

    if(cnt==0){
        printf("读0\n");
        info->flag|=HG_UPSTREAM_ADD_READ;
        return HG_AGAIN;
    }


    int fdup=up->conn->fd;
    int fddown=up->r->conn->fd;

    int flag=0;
    flag=fcntl(fdup,F_GETFL);
    flag=flag&(~O_NONBLOCK);
    fcntl(fdup,F_SETFL,flag);

    flag=fcntl(fddown,F_GETFL);
    flag=flag&(~O_NONBLOCK);
    fcntl(fddown,F_SETFL,flag);

    
    while(cnt>0){
       cnt=write(fddown,buf->cur,cnt);
       printf("proxy write %d\n",cnt);
       buf->take(cnt);
       cnt=read(fdup,buf->last,buf->surplus());
       buf->extend(cnt);
    }

    return HG_OK;
*/
}

int hg_http_proxy_parse_body(cris_buf_t **out_buffer,void*data,hg_upstream_info_t *info){

    return HG_OK;
}


int hg_http_proxy_post_upstream(void*data,int rc){

    ((hg_http_proxy_ctx_t*)data)->up->r->skip_response=true;
    return HG_OK;
}


inline void hg_http_proxy_add_header(hg_http_proxy_ctx_t *ctx,cris_http_header_t *header){
      
       header->next=ctx->headers;
       ctx->headers=header;
}




int hg_http_response_parse(hg_http_proxy_ctx_t *ctx, cris_buf_t *buf){


    unsigned int state=ctx->parse_state;
    char *s=buf->start;
    char *p=buf->cur;
    char *e=buf->last;
    char ch;

    for(;p<e;p++){
    
          ch=*p;
    
          switch(state){
	  
	     case HG_RESPONSE_INIT:
                   ctx->response_start_pos=p-s;
                   state=HG_RESPONSE_VERSION;
		   break;

             case HG_RESPONSE_VERSION:
                   if(ch==' ')
                      state=HG_RESPONSE_VERSION_BK;
                   break;

             case HG_RESPONSE_VERSION_BK:
                   if(ch>='1'&&ch<='9')
		      state=HG_RESPONSE_CODE;
		   else if(ch==' '){
		   
		   }else
		      return HG_ERROR;
		      
		   break;

	     case HG_RESPONSE_CODE:
                   if(ch==' ')
		      state=HG_RESPONSE_CODE_BK;
		   else if(ch>='0'&&ch<='9'){
		   
		   }else
		      return HG_ERROR;
                   
		   break;
          
	     case HG_RESPONSE_CODE_BK:
		   if(ch!=' ')
		      state=HG_RESPONSE_INFO;
                   break;

	     case HG_RESPONSE_INFO:
                   if(ch=='\r')
		      state=HG_RESPONSE_LR;
		   break;

	     case HG_RESPONSE_LR:
                   if(ch=='\n')
		      state=HG_RESPONSE_CR;
		   else
		      return HG_ERROR;
                   break;

	     case HG_RESPONSE_CR:
	           if(ch=='\r')
		      state=HG_RESPONSE_LRLR;
                   else{
		      ctx->pre=p-s;
		      state=HG_RESPONSE_HEAD_NAME;
		   }
                   break;

	     case HG_RESPONSE_HEAD_NAME:
                   if(ch==':'){
		   
		          ctx->tmp_header=new (ctx->pool->qlloc(sizeof(cris_http_header_t)))cris_http_header_t();
			  ctx->tmp_header->name.str=&s[ctx->pre];
                          ctx->tmp_header->name.len=p-ctx->tmp_header->name.str;
			  state=HG_RESPONSE_HEAD_GAP;
		   }

                   break;

	     case HG_RESPONSE_HEAD_GAP:
                   if(ch!=' '){
		   
		          ctx->pre=p-s;
		          state=HG_RESPONSE_HEAD_CONTENT;
		   }

		   break;

	     case HG_RESPONSE_HEAD_CONTENT:
                   if(ch=='\r'){
		   
		        ctx->tmp_header->content.str=&s[ctx->pre];     		   
		        ctx->tmp_header->content.len=p-ctx->tmp_header->content.str;
			hg_http_proxy_add_header(ctx,ctx->tmp_header);

                        switch(ctx->tmp_header->name.len){
			     //Set-Cookie
			     case 10:
                                  if(strncmp(ctx->tmp_header->name.str,"Set-Cookie",10)==0){
				     ctx->set_cookie=true;
				  }
				  break;
			     //Content-Length
			     case 14:
			          if(strncmp(ctx->tmp_header->name.str,"Content-Length",14)==0){
				     ctx->content_length_n=atoi(ctx->tmp_header->content.str);	  
				  }
				  break;
			}

		        state=HG_RESPONSE_LR;
		   }

		   break;

	     case HG_RESPONSE_LRLR:
                   if(ch=='\n')
		     state=HG_RESPONSE_CRCR;
		   else
		     return HG_ERROR;

                   break;

             default:

	           return HG_ERROR;
	  }

          if(state==HG_RESPONSE_CRCR){
	  
	        buf->cur=p+1;
                ctx->entire_response.str=&s[ctx->response_start_pos];
                ctx->entire_response.len=p-ctx->entire_response.str+1;
	        ctx->parse_state=state;
		return HG_OK;
	  }
    }
    //这里不能使用take函数,应为可能会导致内存复用,而这块内存还要供转发使用
    buf->cur=p;
    ctx->parse_state=state;

    return HG_AGAIN;
}










#endif















