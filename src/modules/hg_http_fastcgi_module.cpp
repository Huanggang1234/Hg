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


int hg_http_fastcgi_create_request(cris_buf_t **out_buffer,void*data,hg_upstream_info_t *info);
int hg_http_fastcgi_parse(cris_buf_t **in_buffer,void*data,hg_upstream_info_t *info);

int hg_http_fastcgi_post_parse_body(cris_buf_t **in_buffer,void*data,hg_upstream_info_t *info);
int hg_http_fastcgi_post_upstream(void*data,int rc);

int hg_http_fastcgi_retry_handler(void*data);

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

int   hg_http_fastcgi_set(hg_module_t *module,hg_cycle_t *cycle,cris_conf_t *conf);
int   hg_http_fastcgi_param_set(hg_module_t *module,hg_cycle_t *cycle,cris_conf_t *conf);

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
          std::string("fastcgi-pass"),
          HG_CMD_LOCATION,
	  &hg_http_fastcgi_set
       },
       {
          std::string("fastcgi-param"),
	  HG_CMD_LOCATION,
          &hg_http_fastcgi_param_set 
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


std::vector<std::string>  hg_http_fastcgi_variables={

             std::string("$content-length"),
             std::string("$cookie"),
	     std::string("$content-type"),
	     std::string("$url-param"),
	     std::string("$method"),
	     std::string("$file-name")
};


unsigned long long  hg_http_fastcgi_id_record=0;




void* hg_http_fastcgi_create_loc_conf(hg_cycle_t *cycle){

     cris_mpool_t *pool=cycle->pool;
     
     hg_http_fastcgi_loc_conf_t *conf=new (pool->alloc(sizeof(hg_http_fastcgi_loc_conf_t)))hg_http_fastcgi_loc_conf_t();

     return (void*)conf;

}


int hg_http_fastcgi_init_loc_conf(hg_module_t *module,hg_cycle_t *cycle,hg_http_conf_t *conf){

 
     hg_http_fastcgi_loc_conf_t *loc=hg_get_loc_conf(hg_http_fastcgi_loc_conf_t,module,conf);   

     if(loc->on)
       hg_http_add_spacial_request_handler(&hg_http_fastcgi_handler,conf);
     
    return HG_OK;
}



int hg_http_fastcgi_set(hg_module_t *module,hg_cycle_t *cycle,cris_conf_t *conf){

    hg_http_module_t *http_m=(hg_http_module_t*)module->ctx;
    hg_http_conf_t   *parent_conf=http_m->ptr;

    hg_http_fastcgi_loc_conf_t *loc=hg_get_loc_conf(hg_http_fastcgi_loc_conf_t,module,parent_conf);

    if(hg_upstream_set_server(parent_conf,conf)==HG_ERROR)
	  return HG_ERROR;

    loc->on=true;

    return HG_OK;
}



int   hg_http_fastcgi_param_set(hg_module_t *module,hg_cycle_t *cycle,cris_conf_t *conf){


     int argc=conf->avgs.size();

     if(argc!=2)
        return HG_ERROR;

     
     hg_http_module_t *http_m=(hg_http_module_t*)module->ctx;
     hg_http_conf_t   *parent_conf=http_m->ptr;
     hg_http_fastcgi_loc_conf_t *loc=hg_get_loc_conf(hg_http_fastcgi_loc_conf_t,module,parent_conf);
    
     cris_mpool_t *pool=cycle->pool;
 
     hg_fastcgi_param *param=new (pool->qlloc(sizeof(hg_fastcgi_param)))hg_fastcgi_param();

     
     cris_str_t &name=conf->avgs.front();
     conf->avgs.pop_front();

     cris_str_t &content=conf->avgs.front();

     if(name.len>256)
       return HG_ERROR;
     
     param->name=name;
     param->param.header.version=FCGI_VERSION_1;
     param->param.header.type=FCGI_PARAMS;
     param->param.len.nameLengthB0=name.len;


     if(content.str[0]!='$'){
     
          if(content.len>256)
	    return HG_ERROR;
          
          param->content=content;
	  param->param.len.valueLengthB0=content.len;
	  
	  unsigned short length=2+name.len+content.len;

	  param->param.header.contentLengthB0=length&0x00ff;
	  param->param.header.contentLengthB1=length&0xff00;
           

     }else{

          param->use_variable=true;
     
          int len=hg_http_fastcgi_variables.size();

          bool found=false;

	  for(int i=0;i<len&&(!found);i++){
	   
	      if(content==hg_http_fastcgi_variables[i]){
	      
	            found=true;
	            param->variable=i+1;
	      }	 
	  }
     
     }

     param->next=loc->params;
     loc->params=param;
     return HG_OK;

}


int hg_http_fastcgi_handler(cris_http_request_t *r){

    hg_http_fastcgi_ctx_t *ctx=new (r->pool->qlloc(sizeof(hg_http_fastcgi_ctx_t)))hg_http_fastcgi_ctx_t();

    ctx->http_request=r;

    ctx->pool=r->pool;

    ctx->conf=hg_get_loc_conf(hg_http_fastcgi_loc_conf_t,(&hg_http_fastcgi_module),(r->loc_conf));

    ctx->request_id=(hg_http_fastcgi_id_record++)%65536+1;

    hg_upstream_t *up=hg_add_upstream(r,NULL);//添加上游模块信息

    hg_http_add_asyn_event(r,HG_ASYN_UPSTREAM);//添加异步事件

    ctx->upstream=up;

    up->data=(void*)ctx;

    up->hg_upstream_create_request=&hg_http_fastcgi_create_request;

    up->hg_upstream_parse_header=&hg_http_fastcgi_parse;

    up->hg_upstream_parse_body=&hg_http_fastcgi_post_parse_body;

    up->hg_upstream_post_upstream=&hg_http_fastcgi_post_upstream;  

    up->hg_upstream_retry_request=&hg_http_fastcgi_retry_handler;

    return HG_OK;
}

int hg_http_fastcgi_retry_handler(void*data){

    hg_http_fastcgi_ctx_t *ctx=(hg_http_fastcgi_ctx_t*)data;
    memset(ctx,0,sizeof(hg_http_fastcgi_ctx_t));
    ctx->forward_upstream=true;
    ctx->forward_body=true;
    return HG_OK;
}



int hg_fastcgi_create_param(cris_buf_t *buf,hg_fastcgi_param *param,cris_http_request_t *r){


        if(!param->use_variable){

    	     buf->append((char*)&param->param,10);
             buf->append(param->name.str,param->name.len);
             buf->append(param->content.str,param->content.len);

	     return HG_OK;

        }

        cris_str_t content;

	
             switch(param->variable){
	     
	          case HG_FCGI_VAR_UNKOWN:
		        return HG_ERROR;

                  case HG_FCGI_VAR_CONTENT_LENGTH:
		        
			if(r->headers_in.content_length==NULL)
			    return HG_OK;

                        content=r->headers_in.content_length->content;

			break;

                  case HG_FCGI_VAR_COOKIE:

                        if(r->headers_in.cookie==NULL)
			    return HG_OK;

                        content=r->headers_in.cookie->content;
                        
			break;

                  case HG_FCGI_VAR_CONTENT_TYPE:
		        
			if(r->headers_in.content_type==NULL)
			    return HG_OK;

                        content=r->headers_in.content_type->content;

			break;

		  case HG_FCGI_VAR_URL_PARAM:
                        
                        if(r->url_param.len==0)
			    return HG_OK;

                        content=r->url_param;

                        break;

                  case HG_FCGI_VAR_METHOD:
                         
                        content=r->method;

			break;

                  case HG_FCGI_VAR_FILE_NAME:

		        if(r->file_name.str==NULL)
			    return HG_OK;

                        content=r->file_name;

                        break;

		  default:
		       return HG_ERROR;

	     }	
	
        

        if(content.len>256)
	  return HG_ERROR;

	param->content=content;
        
	unsigned short  length=2+param->name.len+content.len;

        param->param.header.contentLengthB0=length&0x00ff;
	param->param.header.contentLengthB1=length&0xff00;
        
	param->param.len.valueLengthB0=content.len;

    	buf->append((char*)&param->param,10);
        buf->append(param->name.str,param->name.len);
        buf->append(param->content.str,param->content.len);

        return HG_OK;

}


int hg_fastcgi_origin_read_handler(hg_event_t *ev){
   
      hg_http_fastcgi_ctx_t *ctx=(hg_http_fastcgi_ctx_t*)ev->data;

      cris_http_request_t *r=ctx->http_request;

      hg_connection_t *conn=r->conn;
      
      cris_buf_t *conn_buf=conn->in_buffer;

      unsigned short  length=0;

      hg_upstream_del_timeout(ctx->upstream);//删除定时事件


      hg_fastcgi_stdin.requestIdB0=ctx->request_id&0x00ff;
      hg_fastcgi_stdin.requestIdB1=ctx->request_id&0xff00;


      int off=ctx->buf->used;

      ctx->buf->append((char*)&hg_fastcgi_stdin,8);

      conn->in_buffer=ctx->buf;

      int cnt=hg_recv(conn);

      conn->in_buffer=conn_buf;

      if(cnt==HG_ERROR||cnt==HG_DISCONNECTED){
        
	   //这里不用移出原始连接在epoll的监听，回调函数中会进行关闭
           hg_upstream_finish(ctx->upstream,HG_ERROR);
	   return HG_ERROR;

      }  


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

      }else{
      
            r->recv_body=r->recv_body+cnt;
      
            length=cnt;

      }
      
      header_peel=(FCGI_Header*)(ctx->buf->start+off);

      header_peel->contentLengthB0=length&0x00ff;
      header_peel->contentLengthB1=length&0xff00;

      if(r->recv_body==r->content_length){

          ctx->buf->append((char*)&hg_fastcgi_end_stdin,8); 

      }

      hg_upstream_activate(ctx->upstream);

      return HG_OK;

}


int hg_fastcgi_real_create(cris_buf_t **out_buffer,hg_http_fastcgi_ctx_t *ctx,hg_upstream_info_t *info){

      cris_buf_t *buf=new (ctx->pool->qlloc(sizeof(cris_buf_t)))cris_buf_t(ctx->pool,512);

      cris_buf_t *body=ctx->http_request->body.temp;
      unsigned short length=ctx->http_request->recv_body;

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

           int rc=hg_fastcgi_create_param(buf,param,ctx->http_request);

	   if(rc==HG_ERROR)
	       return HG_ERROR;
         
	   param=param->next;
      }          
  
      hg_fastcgi_end_param.requestIdB0=B0;
      hg_fastcgi_end_param.requestIdB1=B1;;
      buf->append((char*)&hg_fastcgi_end_param,8);
      
      //没有包体,或则不传输包体
      if((!ctx->forward_body)||ctx->http_request->content_length==0)
             goto ok;

      hg_fastcgi_stdin.requestIdB0=B0;
      hg_fastcgi_stdin.requestIdB1=B1;

      //将包体置为不可用状态
      ctx->http_request->body.state=HG_HTTP_BODY_UNAVAILABLE;

      hg_fastcgi_stdin.contentLengthB0=length&0x00ff;
      hg_fastcgi_stdin.contentLengthB1=length&0xff00;

      
      buf->append((char*)&hg_fastcgi_stdin,8);
      buf->append(body->start,length);
 
      ctx->content_length=ctx->http_request->content_length;
      ctx->recv_content_length=length;

      if(ctx->recv_content_length<ctx->content_length){//请求没有构造完成
                
                ctx->http_request->conn->read->data=(void*)ctx;
		ctx->http_request->conn->read->handler=&hg_fastcgi_origin_read_handler;

                add_read(ctx->http_request->conn);

                info->flag|=HG_UPSTREAM_ADD_TIME|HG_UPSTREAM_TO_CLOSE;
                info->msec=3000;

		ctx->listen_origin=true;

                return HG_AGAIN;
      }

ok:

      buf->append((char*)&hg_fastcgi_end_stdin,8);

      return HG_OK;
}


int hg_fastcgi_rd_body(cris_buf_t **out_buffer,hg_http_fastcgi_ctx_t *ctx,hg_upstream_info_t *info){
       

    cris_http_request_t *r=ctx->http_request;

    cris_buf_t *buf=*out_buffer;

    if(r->recv_body==r->content_length){

         if(ctx->listen_origin)//如果还在监听中就不再监听
            del_conn(r->conn);

         ctx->listen_origin=false;

         return HG_OK;
    }


    if(buf->available()>(buf->capacity>>1)){//如果缓冲的使用率大于50%,则先停止对原始连接的监听，先发送现有数据

        del_conn(r->conn);
        
	ctx->listen_origin=false;//因为这行代码，不能去掉下面的return

        info->flag|=HG_UPSTREAM_ADD_WRITE|HG_UPSTREAM_ADD_TIME|HG_UPSTREAM_TO_CLOSE;
        info->msec=3000;

	return HG_AGAIN;
    }

    buf->reuse();

    //发送缓冲的数据低于一半，则监听原始连接获取新数据
    if(!ctx->listen_origin){
    
         add_read(r->conn);
        
	 ctx->listen_origin=true;

         r->conn->read->data=(void*)ctx;
         r->conn->read->handler=&hg_fastcgi_origin_read_handler;
         
    }

    info->flag|=HG_UPSTREAM_ADD_TIME|HG_UPSTREAM_TO_CLOSE;
    info->msec=3000;

    return HG_AGAIN;
}




int hg_http_fastcgi_create_request(cris_buf_t **out_buffer,void*data,hg_upstream_info_t *info){

       hg_http_fastcgi_ctx_t *ctx=(hg_http_fastcgi_ctx_t*)data;
   
       int rc=HG_ERROR;

       switch(ctx->state){
       
           case HG_FCGI_INITIAL:

                if((rc=hg_fastcgi_real_create(out_buffer,ctx,info))==HG_OK)
		   ctx->state=HG_FCGI_PARSE_HEADER;
		else if(rc==HG_AGAIN)
		   ctx->state=HG_FCGI_RD_BODY;

		break;
      
           case HG_FCGI_RD_BODY:

                if((rc=hg_fastcgi_rd_body(out_buffer,ctx,info))==HG_OK)
		   ctx->state=HG_FCGI_PARSE_HEADER;
                 
		break;

	   default:
                return HG_ERROR;
       }
       
       return rc;
}





int hg_fastcgi_parse_header(cris_buf_t **in_buffer,hg_http_fastcgi_ctx_t *ctx,hg_upstream_info_t *info){

      cris_buf_t *buf=*in_buffer;

      int cnt=buf->available();

      if(cnt<8){

           info->flag|=HG_UPSTREAM_ADD_READ|HG_UPSTREAM_ADD_TIME|HG_UPSTREAM_TO_CLOSE;
           info->msec=2000;

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


int hg_fastcgi_write_origin(hg_http_fastcgi_ctx_t *ctx){

      cris_buf_t *buf=ctx->buf;

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


int hg_fastcgi_keep_in_memery(hg_http_fastcgi_ctx_t *ctx){

    
    cris_buf_t *inbuf=ctx->buf;


    if(ctx->content_off==0){
    
       ctx->content_off=inbuf->cur-inbuf->start; 
    
    }

    if(inbuf->available()>=ctx->content_length+ctx->padding_length){
    
            
	 ctx->response=new (ctx->pool->qlloc(sizeof(cris_buf_t)))cris_buf_t(ctx->pool,(inbuf->start+ctx->content_off),ctx->content_length);

         ctx->recv_content_length=ctx->content_length;

	 ctx->recv_padding=ctx->padding_length;

         inbuf->cur=inbuf->cur+ctx->content_length+ctx->padding_length;

	 return HG_OK;

    }

    return HG_AGAIN;
}


int hg_fastcgi_origin_write_handler(hg_event_t *ev){

    hg_http_fastcgi_ctx_t *ctx=(hg_http_fastcgi_ctx_t *)ev->data;

    hg_upstream_del_timeout(ctx->upstream);//先删除定时

    int rc=hg_fastcgi_write_origin(ctx);

    if(rc==HG_ERROR){

        //这里不用移出原始连接在epoll的监听，回调函数中会进行关闭
        hg_upstream_finish(ctx->upstream,HG_ERROR);
	return HG_ERROR;
    }

    if(rc==HG_AGAIN){
    
          cris_buf_t *buf=ctx->buf;

          if(buf->available()>(buf->capacity>>1)){

                 hg_upstream_add_timeout(ctx->upstream,HG_UPSTREAM_ADD_TIME|HG_UPSTREAM_TO_CLOSE,3000);

                 return HG_AGAIN;
          }
          //激活上游对象，再次接收数据
          return hg_upstream_activate((void*)ctx->upstream);
    }

    //如果按照正确的处理流程，这里不会产生HG_OK的返回值,也不会执行下面的语句

    return hg_upstream_activate((void*)ctx->upstream);

}


int hg_fastcgi_parse_real_body(cris_buf_t **in_buffer,hg_http_fastcgi_ctx_t *ctx,hg_upstream_info_t *info){

        
       int rc=HG_ERROR;

       ctx->buf=*in_buffer;

       if(ctx->forward_upstream)
          rc=hg_fastcgi_write_origin(ctx);
       else
          rc=hg_fastcgi_keep_in_memery(ctx);
 
       if(rc==HG_AGAIN){

           cris_buf_t *buf=ctx->buf;

           if((buf->available()>(buf->capacity>>1))&&ctx->forward_upstream){//缓冲空间占用率大于50%
	   
               hg_connection_t *conn=ctx->http_request->conn;

	       conn->write->data=(void*)ctx;
	       conn->write->handler=&hg_fastcgi_origin_write_handler;

               add_write(conn);

	       ctx->listen_origin=true;

	       info->flag|=HG_UPSTREAM_ADD_TIME|HG_UPSTREAM_TO_CLOSE;
	       info->msec=3000;

	   }else{
	   
	       info->flag|=HG_UPSTREAM_ADD_READ|HG_UPSTREAM_ADD_TIME|HG_UPSTREAM_TO_CLOSE;	   
	       info->msec=3000;
	   }
      }

      return rc;
}


int hg_fastcgi_parse_protocol_end(cris_buf_t **in_buffer,hg_http_fastcgi_ctx_t *ctx,hg_upstream_info_t *info){


      cris_buf_t *buf=*in_buffer;

      if(buf->available()<8){
     
        info->flag|=HG_UPSTREAM_ADD_READ|HG_UPSTREAM_ADD_TIME|HG_UPSTREAM_TO_CLOSE;
        info->msec=2000;

        return HG_AGAIN;
      }

      FCGI_EndRequestBody *p=(FCGI_EndRequestBody*)buf->cur;

      ctx->app_status=(p->appStatusB3<<24)|(p->appStatusB2<<16)|(p->appStatusB1<<8)|(p->appStatusB0);
      ctx->protocol_status=p->protocolStatus;
     
      buf->cur=buf->cur+8;

      return HG_OK;
}




int hg_http_fastcgi_parse(cris_buf_t **in_buffer,void*data,hg_upstream_info_t *info){
 
     hg_http_fastcgi_ctx_t *ctx=(hg_http_fastcgi_ctx_t*)data;

     int state=ctx->state;

     int rc=HG_ERROR;

     bool again=true;

     while(again){

          switch(state){
     
               case HG_FCGI_PARSE_HEADER:
     
                    if((rc=hg_fastcgi_parse_header(in_buffer,ctx,info))==HG_OK){
		    
		        if(ctx->type==FCGI_STDOUT){
	                  
			   if(ctx->content_length!=0)
	 		     state=HG_FCGI_PARSE_REAL_BODY;
		           //等于0的情况就是继续解析头部，不用改变状态

			}else if(ctx->type==FCGI_END_REQUEST){
                            
			    state=HG_FCGI_PARSE_PROTOCOL_END;
			
			}else
			    rc=HG_ERROR;
		    }

                    break;

               case HG_FCGI_PARSE_REAL_BODY:

                    if((rc=hg_fastcgi_parse_real_body(in_buffer,ctx,info))==HG_OK){
		    
		       state=HG_FCGI_PARSE_HEADER;
		    
		    }

                    break;
                        
               case HG_FCGI_PARSE_PROTOCOL_END:

                    if((rc=hg_fastcgi_parse_protocol_end(in_buffer,ctx,info))==HG_OK){
		    
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






int hg_http_fastcgi_post_parse_body(cris_buf_t **in_buffer,void*data,hg_upstream_info_t *info){

       return HG_OK;
}



int hg_http_fastcgi_post_upstream(void*data,int rc){

       hg_http_fastcgi_ctx_t *ctx=(hg_http_fastcgi_ctx_t *)data;

       if(rc==HG_OK){
      
          if(ctx->forward_upstream)//在转发上游请求后才可以跳过响应
            ctx->http_request->skip_response=true;
            
	    ctx->access_upstream=true;

       }
       
       if(ctx->listen_origin)
           del_conn(ctx->http_request->conn);



       return HG_OK;
}


#endif


