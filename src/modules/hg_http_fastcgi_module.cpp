#ifndef HG_HTTP_FASTCGI_MODULE_CPP
#define HG_HTTP_FASTCGI_MODULE_CPP

#include"../../include/modules/hg_http_fastcgi_module.h"
#include"../../base/cris_buf.h"
#include"../../base/cris_memery_pool.h"

#include"../../include/hg_define.h"
#include"../../include/hg.h"
#include"../../include/hg_http_module.h"
#include"../../include/hg_http_core_module.h"
#include"../../include/hg_upstream_module.h"
#include"../../include/hg_conf_parse.h"


int hg_http_fastcgi_initial(cris_http_request_t *r);


int hg_http_fastcgi_create_request(cris_buf_t **out_buffer,void*data);
int hg_http_fastcgi_parse(cris_buf_t **in_buffer,void*data);

int hg_http_fastcgi_post_parse_body(cris_buf_t **in_buffer,void*data);
int hg_http_fastcgi_post_upstream(void*data,int rc);


int hg_http_fatscgi_handler(cris_http_request_t *r);


FCGI_BeginRequestRecord  hg_fastcgi_begin={
     {
        FCGI_VERSION_1,
        FCGI_BEGIN_REQUEST,
	0,//id
	0,//id
	0,//length
        8,//length
	0,//plength
	0//reserved
     },
     {
        0,//role
	FCGI_RESPONDER,//role
	0,//flags
	{0,0,0,0,0}//reserved
     }
};

FCGI_Header hg_fastcgi_end_param={
     FCGI_VERSION_1,
     FCGI_PARAMS,
     0,
     0,
     0,
     0,
     0,
     0
};


FCGI_Header hg_fastcgi_end_stdin={
     FCGI_VERSION_1,
     FCGI_STDIN,
     0,
     0,
     0,
     0,
     0,
     0
};
 
FCGI_Header hg_fastcgi_stdin={
     FCGI_VERSION_1,
     FCGI_STDIN,
     0,
     0,
     0,
     0,
     0,
     0
};


FCGI_Header  *header_peel=NULL;


void *hg_http_fastcgi_create_loc_conf(hg_cycle_t *cycle);
int   hg_http_fastcgi_init_loc_conf(hg_module_t *module,hg_cycle_t *cycle,hg_http_conf_t *conf);

int   hg_http_fastcgi_set(hg_module_t *,hg_cycle_t *cycle,cris_conf_t *conf);


int hg_http_fastcgi_handler(cris_http_request_t *r);



hg_http_module_t  _hg_http_fastcgi_module={

	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	&hg_http_fastcgi_create_loc_conf,
	&hg_http_fastcgi_init_loc_conf,
	NULL,
	NULL,
	NULL
};

std::vector<hg_command_t> hg_http_fastcgi_commands={

       {
          std::string("fastcgi"),
          0,
	  &hg_http_fastcgi_set
       }
};

hg_module_t  hg_http_fastcgi_module={

	HG_HTTP_MODULE,
	0,
	0,
	&_hg_http_fastcgi_module,
	&hg_http_fastcgi_commands,
	NULL,
	NULL,
	NULL,
	NULL       
};


void* hg_http_fastcgi_create_loc_conf(hg_cycle_t *cycle){

     cris_mpool_t *pool=cycle->pool;
     
     hg_http_fastcgi_loc_conf_t *conf=new (pool->alloc(sizeof(hg_http_fastcgi_loc_conf_t)))hg_http_fastcgi_loc_conf_t();

     return (void*)conf;

}

int hg_http_fastcgi_init_loc_conf(hg_module_t *module,hg_cycle_t *cycle,hg_http_conf_t *conf){

 
     hg_http_fastcgi_loc_conf_t *loc=hg_get_loc_conf(hg_http_fastcgi_loc_conf_t,module,conf);   

     if(loc->on){
       printf("fastcgi init loc conf\n");
       
       printf("add spacial addr %p\n",&hg_http_fastcgi_handler);

       hg_http_add_spacial_request_handler(&hg_http_fastcgi_handler,conf);
     }
    return HG_OK;
}


int hg_http_fastcgi_set(hg_module_t *module,hg_cycle_t *cycle,cris_conf_t *conf){

    int num=conf->avgs.size();
    if(num==0||num>1)
      return HG_OK;

    hg_http_module_t *http_m=(hg_http_module_t*)module->ctx;
    hg_http_conf_t   *parent_conf=http_m->ptr;

    hg_http_fastcgi_loc_conf_t *loc=hg_get_loc_conf(hg_http_fastcgi_loc_conf_t,module,parent_conf);

    cris_str_t on=conf->avgs.front();

    if(on==std::string("on"))
       loc->on=true;

    return HG_OK;
}



int hg_http_fastcgi_handler(cris_http_request_t *r){

    printf("fastcgi_content_handler\n");

    hg_upstream_t *up=hg_add_upstream(r);//添加上游模块信息

    hg_http_add_asyn_event(r,HG_ASYN_UPSTREAM);//添加异步事件

    hg_http_fastcgi_ctx_t *ctx=new (r->pool->qlloc(sizeof(hg_http_fastcgi_ctx_t)))hg_http_fastcgi_ctx_t();

    ctx->http_request=r;

    ctx->pool=r->pool;

    ctx->conf=hg_get_loc_conf(hg_http_fastcgi_loc_conf_t,(&hg_http_fastcgi_module),(r->loc_conf));

    ctx->request_id=1;

    up->data=(void*)ctx;

    up->hg_upstream_create_request=&hg_http_fastcgi_create_request;

    up->hg_upstream_parse_header=&hg_http_fastcgi_parse;

    up->hg_upstream_parse_body=&hg_http_fastcgi_post_parse_body;

    up->hg_upstream_post_upstream=&hg_http_fastcgi_post_upstream;   

    return HG_OK;
}



int hg_fastcgi_real_create(cris_buf_t **out_buffer,hg_http_fastcgi_ctx_t *ctx){

      printf("real create\n");

      cris_buf_t *buf=new (ctx->pool->qlloc(sizeof(cris_buf_t)))cris_buf_t(ctx->pool,512);

      printf("send buf %p\n",buf);

      cris_buf_t *body=ctx->http_request->body.temp;
      unsigned short length=ctx->http_request->content_length;

      *out_buffer=buf;
      ctx->buf=buf;

      unsigned char B0=ctx->request_id&0x00ff;
      unsigned char B1=ctx->request_id&0xff00;

      hg_fastcgi_begin.header.requestIdB0=B0;
      hg_fastcgi_begin.header.requestIdB1=B1;

      buf->append((char*)&hg_fastcgi_begin,16);

      hg_fastcgi_param *param=ctx->conf->params;

      while(param!=NULL){
      
           param->param.header.requestIdB0=B0;
           param->param.header.requestIdB1=B1;

	   buf->append((char*)&param->param,10);
           buf->append(param->name.str,param->name.len);
           buf->append(param->content.str,param->name.len);
           
	   param=param->next;
      }          
  
      hg_fastcgi_end_param.requestIdB0=B0;
      hg_fastcgi_end_param.requestIdB1=B1;;
      buf->append((char*)&hg_fastcgi_end_param,8);
      
      //没有包体
      if(ctx->http_request->content_length==0)
             goto ok;

      hg_fastcgi_stdin.requestIdB0=B0;
      hg_fastcgi_stdin.requestIdB1=B1;

      //将包体置为不可用状态
      ctx->http_request->body.state=HG_HTTP_BODY_UNAVAILABLE;

      hg_fastcgi_stdin.contentLengthB0=length&0x00ff;
      hg_fastcgi_stdin.contentLengthB1=length&0xff00;

      
      buf->append((char*)&hg_fastcgi_stdin,8);
      buf->append(body->start,body->used);
 
      ctx->content_length=ctx->http_request->content_length;
      ctx->recv_content_length=body->used;

      if(ctx->recv_content_length<ctx->content_length){
                              
                return HG_AGAIN;
      }


ok:

      buf->append((char*)&hg_fastcgi_end_stdin,8);

      return HG_OK;
}


int hg_fastcgi_rd_body(cris_buf_t **out_buffer,hg_http_fastcgi_ctx_t *ctx){

      printf("rd body\n");
       
      hg_connection_t *conn=ctx->http_request->conn;

      cris_buf_t *conn_buf=conn->in_buffer;

      cris_http_request_t *r=ctx->http_request;

      int rc=HG_ERROR;

      unsigned short  length=0;

      hg_fastcgi_stdin.requestIdB0=ctx->request_id&0x00ff;
      hg_fastcgi_stdin.requestIdB1=ctx->request_id&0xff00;

      int off=ctx->buf->used;

      ctx->buf->append((char*)&hg_fastcgi_stdin,8);

      conn->in_buffer=ctx->buf;

      int cnt=hg_recv(conn);

      conn->in_buffer=conn_buf;


      if(cnt==HG_ERROR)
         return HG_ERROR;

      if(r->recv_body+cnt>=r->content_length){

            unsigned res=r->content_length-r->recv_body;

            char *s=ctx->buf->last-cnt+res;

	    r->recv_body=r->content_length;
            
	    //将接收的多余包体的内容，添加到原连接缓冲上去
	    conn_buf->append(s,cnt-res);
            
	    ctx->buf->last=s;
            ctx->buf->used=ctx->buf->last-ctx->buf->start;
            ctx->buf->res=ctx->buf->capacity-ctx->buf->used;

	    length=res;

            rc=HG_OK;

      }else{
      
            r->recv_body=r->recv_body+cnt;
      
            length=cnt;

	    rc=HG_AGAIN;
      }

      
      header_peel=(FCGI_Header*)(ctx->buf->start+off);

      header_peel->contentLengthB0=length&0x00ff;
      header_peel->contentLengthB1=length&0xff00;

      if(rc==HG_OK){

          ctx->buf->append((char*)&hg_fastcgi_end_stdin,8); 

      }
      

      return rc;
}




int hg_http_fastcgi_create_request(cris_buf_t **out_buffer,void*data){

       hg_http_fastcgi_ctx_t *ctx=(hg_http_fastcgi_ctx_t*)data;
   
       int rc=HG_ERROR;

       switch(ctx->state){
       
           case HG_FCGI_INITIAL:

                if((rc=hg_fastcgi_real_create(out_buffer,ctx))==HG_OK)
		   ctx->state=HG_FCGI_PARSE_HEADER;
		else if(rc=HG_AGAIN)
		   ctx->state=HG_FCGI_RD_BODY;

		break;
      
           case HG_FCGI_RD_BODY:

                if((rc=hg_fastcgi_rd_body(out_buffer,ctx))==HG_OK)
		   ctx->state=HG_FCGI_PARSE_HEADER;
                 
		break;

	   default:
                return HG_ERROR;
       }
       
       return rc;
}





int hg_fastcgi_parse_header(cris_buf_t **in_buffer,hg_http_fastcgi_ctx_t *ctx){

      cris_buf_t *buf=*in_buffer;

      int cnt=buf->available();

      if(cnt<8){
      
           return HG_AGAIN;
 
      }else{
      
           header_peel=(FCGI_Header*)buf->cur;

	   ctx->type=header_peel->type;

           ctx->content_length=(header_peel->contentLengthB1<<8)|(header_peel->contentLengthB0);

           ctx->recv_content_length=0;          

	   ctx->padding_length=header_peel->paddingLength;

	   ctx->recv_padding=0;

           buf->cur=buf->cur+8;
      }


      return HG_OK;
}


int hg_fastcgi_parse_real_body(cris_buf_t **in_buffer,hg_http_fastcgi_ctx_t *ctx){

 
      cris_buf_t *buf=*in_buffer;

      hg_connection_t *conn=ctx->http_request->conn;
      
      int res=ctx->content_length-ctx->recv_content_length;

      int cnt=0;

      int rc=HG_ERROR;

      if(res>0){     
           if(buf->available()<res){
      
                 conn->out_buffer=buf;

                 cnt=hg_send(conn);

		 conn->out_buffer=NULL;

         	 if(cnt==HG_ERROR)
	             return HG_ERROR;

	         ctx->recv_content_length=ctx->recv_content_length+cnt;
            
                 rc=HG_AGAIN;

           }else{
      
                 cris_buf_t temp(ctx->pool,buf->cur,res);

	         conn->out_buffer=&temp;

                 cnt=hg_send(conn);

	         conn->out_buffer=NULL;

                 if(cnt==HG_ERROR)

                    return HG_ERROR;

                 else if(cnt<res){

                    ctx->recv_content_length=ctx->recv_content_length+cnt;
	   
	            rc=HG_AGAIN;
	     
         	}else{//cnt==res

                    ctx->recv_content_length=ctx->content_length;

		    res=0;

	        }	
            
                buf->cur=buf->cur+cnt;
         }
      }

      if(res==0){
      
            res=ctx->padding_length-ctx->recv_padding;     
                       
            if(buf->available()>=res){
	    
                   buf->cur=buf->cur+res;

		   ctx->recv_padding=ctx->padding_length;

		   rc=HG_OK;
	    
	    }else{

                   ctx->recv_padding=ctx->recv_padding+buf->available();
	    
	           buf->cur=buf->last;
                      
		   rc=HG_AGAIN;
	    }
      }


      if(res<0)
        rc=HG_ERROR;

      return rc;
}


int hg_fastcgi_parse_protocol_end(cris_buf_t **in_buffer,hg_http_fastcgi_ctx_t *ctx){


      cris_buf_t *buf=*in_buffer;

      if(buf->available()<8)
        return HG_AGAIN;

      FCGI_EndRequestBody *p=(FCGI_EndRequestBody*)buf->cur;

      ctx->app_status=(p->appStatusB3<<24)|(p->appStatusB2<<16)|(p->appStatusB1<<8)|(p->appStatusB0);
      ctx->protocol_status=p->protocolStatus;
     
      buf->cur=buf->cur+8;

      return HG_OK;
}




int hg_http_fastcgi_parse(cris_buf_t **in_buffer,void*data){
 
     hg_http_fastcgi_ctx_t *ctx=(hg_http_fastcgi_ctx_t*)data;

     int state=ctx->state;

     int rc=HG_ERROR;

     bool again=true;

     while(again){

          switch(state){
     
               case HG_FCGI_PARSE_HEADER:
     
                    if((rc=hg_fastcgi_parse_header(in_buffer,ctx))==HG_OK){
		    
		        if(ctx->type==FCGI_STDOUT){
	                  
			   if(ctx->content_length!=0)
	 		     state=HG_FCGI_PARSE_REAL_BODY;
			
			}else if(ctx->type==FCGI_END_REQUEST){
                            
			    state=HG_FCGI_PARSE_PROTOCOL_END;
			
			}else
			    rc=HG_ERROR;
		    }

                    break;

               case HG_FCGI_PARSE_REAL_BODY:

                    if((rc=hg_fastcgi_parse_real_body(in_buffer,ctx))==HG_OK){
		    
		       state=HG_FCGI_PARSE_HEADER;
		    
		    }

                    break;
                        
               case HG_FCGI_PARSE_PROTOCOL_END:

                    if((rc=hg_fastcgi_parse_protocol_end(in_buffer,ctx))==HG_OK){
		    
		       state=HG_FCGI_PARSE_END;
		    }
     
                    break;
                       
               default:

                   rc=HG_ERROR;
		   state=HG_FCGI_PARSE_END;

         }

	 if(rc!=HG_OK||state==HG_FCGI_PARSE_END)
	        again=false;
    }

    ctx->state=state;

    return rc;
}






int hg_http_fastcgi_post_parse_body(cris_buf_t **in_buffer,void*data){

       return HG_OK;
}



int hg_http_fastcgi_post_upstream(void*data,int rc){

       if(rc==HG_OK){
       
            ((hg_http_fastcgi_ctx_t*)data)->http_request->skip_response=true;
       
       }

       return HG_OK;
}


#endif


