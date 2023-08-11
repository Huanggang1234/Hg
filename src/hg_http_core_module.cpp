#ifndef HG_HTTP_CORE_MODULE_CPP
#define HG_HTTP_CORE_MODULE_CPP
#include"../include/hg_http_module.h"
#include"../include/hg_http_core_module.h"
#include"../include/hg_epoll_module.h"
#include"../include/hg_upstream_module.h"
#include"../include/hg.h"
#include"../include/hg_define.h"
#include"../include/hg_conf_parse.h"
#include"../include/hg_http_core_str_base.h"
#include<vector>
#include<string>
#include<sys/sendfile.h>


void*  hg_http_core_create_main_conf(hg_cycle_t *cycle);
void*  hg_http_core_create_srv_conf(hg_cycle_t *cycle);
void*  hg_http_core_create_loc_conf(hg_cycle_t *cycle);

int    hg_http_core_init_main_conf(hg_module_t *module,hg_cycle_t *cycle,hg_http_conf_t *conf);//全局配置，该函数只会被调用一次
int    hg_http_core_init_srv_conf(hg_module_t *module,hg_cycle_t *cycle,hg_http_conf_t *conf);//conf为同级配置
int    hg_http_core_init_loc_conf(hg_module_t *module,hg_cycle_t *cycle,hg_http_conf_t *conf);//conf为同级配置

int    hg_http_core_init_process(hg_module_t *module,hg_cycle_t *cycle);

//在解析完http块内所有内容，并对所有配置进行初始化后调用
int    hg_http_core_postconfiguration(hg_module_t *module,hg_cycle_t *cycle,hg_http_conf_t *conf);


int   hg_http_core_set_server(hg_module_t *module,hg_cycle_t *cycle,cris_conf_t *conf);
int   hg_http_core_set_location(hg_module_t *module,hg_cycle_t *cycle,cris_conf_t *conf);

int   hg_http_core_set_static(hg_module_t *module,hg_cycle_t *cycle,cris_conf_t *conf);
int   hg_http_core_set_filetype(hg_module_t *module,hg_cycle_t *cycle,cris_conf_t *conf);


int   hg_http_merge_location(hg_cycle_t *cycle,hg_http_conf_t *p_conf,hg_http_core_loc_conf_t *loc_conf);//递归合并location配置项

//int   hg_http_core_merge_server(void *p_conf,void *c_conf);//核心模块合并Server级别的配置项
//int   hg_http_core_merge_location(void *p_conf,void *c_conf);//核心模块合并location级别的配置项


int   hg_init_locations(hg_http_core_loc_conf_t *loc);//递归对location进行排序并融合同url的前缀匹配和精准匹配

hg_http_loc_tree_node_t* hg_init_locations_tree(hg_cycle_t *cycle,hg_http_loc_que_node_t *que);//递归构建前缀树


hg_http_conf_t*   hg_http_find_location_conf(cris_str_t &url,int off,hg_http_loc_tree_node_t *tree);//对指定url查找匹配的配置



int hg_http_init_request(hg_event_t *ev);//作为连接第一次出现可读事件的回调函数
int hg_http_init_request_handler(hg_event_t *ev);

int hg_http_free_request(cris_http_request_t *r);

int hg_http_core_run_phases(cris_http_request_t *r);
int hg_http_core_run_phases_handler(hg_event_t *ev);


int hg_http_connection_timeout_handler(hg_event_t *ev);//长连接的超时处理回调


int  hg_http_core_common_phase(cris_http_request_t *r,hg_http_handler_t *ph);
int  hg_http_core_rewrite_phase(cris_http_request_t *r,hg_http_handler_t *ph);
int  hg_http_core_access_phase(cris_http_request_t *r,hg_http_handler_t *ph);
int  hg_http_core_content_phase(cris_http_request_t *r,hg_http_handler_t *ph);


int  hg_http_core_find_config_phase(cris_http_request_t *r,hg_http_handler_t *ph);//专
int  hg_http_core_post_rewrite_phase(cris_http_request_t *r,hg_http_handler_t *ph);//专，检测死循环
int  hg_http_core_post_access_phase(cris_http_request_t *r,hg_http_handler_t *ph);
int  hg_http_core_try_files_phase(cris_http_request_t *r,hg_http_handler_t *ph);//专
int  hg_http_core_response_phase(cris_http_request_t *r,hg_http_handler_t *ph);//专，响应阶段负责发送一切响应


/************头部处理函数******************/
int hg_http_core_set_content_length(cris_http_request_t *r);



/*接口函数*/

int hg_http_block_read_handler(hg_event_t *ev);
int hg_http_block_write_handler(hg_event_t *ev);


int hg_http_read_body(cris_http_request_t *r,int (*callback)(cris_http_request_t *));
int hg_http_read_body_handler(hg_event_t *ev);

int hg_http_discard_body(cris_http_request_t *r);
int hg_http_discard_body_handler(hg_event_t *ev);

/*************************************************************************/

int hg_http_null_content_handler(cris_http_request_t *r);
int hg_http_special_response_process(cris_http_request_t *r,int rc);

cris_buf_t*  hg_http_tile_response(cris_http_request_t *r);




//http核心模块的上下文
hg_http_module_t  hg_http_core_ctx={
       NULL,
       &hg_http_core_postconfiguration,
       &hg_http_core_create_main_conf,
       &hg_http_core_init_main_conf,
       &hg_http_core_create_srv_conf,
       &hg_http_core_init_srv_conf,
       &hg_http_core_create_loc_conf,
       &hg_http_core_init_loc_conf,
       NULL,
       NULL,
       NULL
};

//http核心模块解析的配置
std::vector<hg_command_t>  http_core_commands={
   {
     std::string("server"),
     0,
     &hg_http_core_set_server  
   },
   { 
     std::string("location"),
     0,
     &hg_http_core_set_location
   },
   {
     std::string("static"),
     0,
     &hg_http_core_set_static
   },
   {
     std::string("filetype"),
     0,
     &hg_http_core_set_filetype
   }
};

hg_module_t  hg_http_core_module={
       HG_HTTP_MODULE,
       0,
       0,
       &hg_http_core_ctx,
       &http_core_commands,
       NULL,
       NULL,
       NULL,
       &hg_http_core_init_process
};

hg_http_core_main_conf_t  http_core_main_conf;//为了其它http模块添加处理方法

std::unordered_map<cris_str_t,hg_http_core_srv_conf_t*,hg_str_hasher>   server_dic;

//长连接的超时处理回调
int hg_http_connection_timeout_handler(hg_event_t *ev){

     hg_return_connection(((cris_http_request_t*)ev->data)->conn);
     return HG_OK;
}


int hg_http_null_content_handler(cris_http_request_t *r){
     return HG_DECLINED;
}



/*int hg_http_response(cris_http_request_t *r){

    int state=r->response_state;

    switch(state){
    
         case HG_RESPONSE_INITIAL:


	 case HG_RESPONSE_SEND_HEADER:


	 case HG_RESPONSE_SEND_BODY:


	 case HG_RESPONSE_FORWARD:


         case HG_RESPONSE_END:
    
    }
}
*/

int hg_http_add_asyn_event(cris_http_request_t*r,int type){

    cris_mpool_t *pool=r->pool;

    hg_http_asyn_event_t *asyn=new (pool->qlloc(sizeof(hg_http_asyn_event_t)))hg_http_asyn_event_t();
    asyn->type=type;
    asyn->next=r->asyn_event;
    r->asyn_event=asyn;
    return HG_OK;
}


int hg_do_asyn_event(cris_http_request_t *r){

     printf("hg_do_asyn_event\n");

     hg_http_asyn_event_t *asyn=r->asyn_event;


     while(asyn!=NULL){
     
         r->count++;

	 hg_upstream_t *upstream;
         
         switch(asyn->type){
	 
            case HG_ASYN_DISCARD_BODY:	 
 	 
	            
	         break;
            case HG_ASYN_RECIVE_BODY:

 
                 break;
	    case HG_ASYN_UPSTREAM:

                 printf("do asyn upstream\n");
                 upstream=r->upstream;
		 r->upstream=upstream->next;
                 hg_upstream_activate((void*)upstream);

                 break;
            defaulte:
	         r->count--;
	         break;
	 }

	 asyn=asyn->next;
     
     }

     r->asyn_event=NULL;//清空异步事件

     return HG_OK;
}

//异步事件必须实现回调函数，且必须通过该函数间接的调用·其回调
int hg_harvest_asyn_event(cris_http_request_t*r,int type,int(*callback)(void *,int),void*data,int rc){
        
	printf("harvest asyn event %d\n",rc);

        callback(data,rc);
	r->count--;   
	if(r->count==1){
	
	     hg_http_core_run_phases(r);
	
	}else if(r->count==0){
             r->count++;
             hg_http_free_request(r);	
	}
        return HG_OK;
}


int hg_http_add_spacial_request_handler(hg_http_request_pt handler,hg_http_conf_t *conf){

    hg_http_core_loc_conf_t *loc_conf=hg_get_loc_conf(hg_http_core_loc_conf_t,(&hg_http_core_module),conf);
    loc_conf->content_handler=handler;
    return HG_OK;
}

//在启动阶段通过该接口添加处理函数函数到流水线引擎
int hg_http_add_request_handler(hg_http_request_pt handler,int phase){

/*

#define   HG_HTTP_POST_READ_PHASE                   0

#define   HG_HTTP_SERVER_REWRITE_PHASE              1

#define   HG_HTTP_FIND_CONFIG_PHASE                 2

#define   HG_HTTP_REWRITE_PHASE                     3

#define   HG_HTTP_POST_REWRITE_PHASE                4

#define   HG_HTTP_PREACCESS_PHASE                   5

#define   HG_HTTP_ACCESS_PHASE                      6

#define   HG_HTTP_POST_ACCESS_PHASE                 7

#define   HG_HTTP_TRY_FILES_PHASE                   8

#define   HG_HTTP_CONTENT_PHASE                     9

#define   HG_HTTP_RESPONSE_PHASE                   10

#define   HG_HTTP_LOG_PHASE                        11

#define   HG_HTTP_PHASE_NUM                        12


*/

   if(phase<HG_HTTP_POST_READ_PHASE ||
      phase>HG_HTTP_LOG_PHASE ||
      phase==HG_HTTP_FIND_CONFIG_PHASE ||
      phase==HG_HTTP_POST_REWRITE_PHASE  ||
      phase==HG_HTTP_POST_ACCESS_PHASE ||
      phase==HG_HTTP_TRY_FILES_PHASE ||
      phase==HG_HTTP_RESPONSE_PHASE
      )
         return HG_ERROR;
    /*以上阶段禁止添加处理函数*/

    http_core_main_conf.phases[phase].push_back(handler);

    return HG_OK;

}


//对请求的读写分别调用该函数，读是指读包体或者丢弃包体，写是指常规流水线的处理，包括对子请求的处理
int hg_http_free_request(cris_http_request_t *r){

    r->count--;

    if(r->parent!=NULL){//子请求

       return HG_OK;
    } 

    if(r->count>0)//请求还在处理当中
       return HG_OK;

    cris_http_header_t *alive=r->headers_in.connection;

    //客户端具有长连接请求
    if(alive!=NULL&&alive->content.len==10&&memcmp("keep-alive",alive->content.str,10)){
    
       
        //这一步不仅添加了读事件，也删除了写事件   
        if(!r->conn->in_read||r->conn->in_write)
	    add_read(r->conn);
         
        hg_connection_t *conn=r->conn;

        conn->in_buffer->reuse();

        memset(r,0,sizeof(cris_http_request_t));
       
        conn->out_buffer=NULL;


	if(conn->in_buffer->available()<=0){
	
	     conn->read->time_handler=&hg_http_connection_timeout_handler;
	     hg_add_timeout(conn->read,120000);
             conn->read->handler=&hg_http_init_request_handler;
	     return HG_OK;

	} 

        int rc=hg_http_request_parse(r,conn->in_buffer);

        if(rc==HG_OK){
        
             r->server=server_dic[r->headers_in.host->content];
     
             conn->read->handler=NULL;//不处理读事件但是仍然监听
             hg_http_core_run_phases(r);//

        }else if(rc==HG_AGAIN){
             conn->read->handler=&hg_http_init_request_handler;
             hg_add_timeout(conn->read,2000);

        }
    }

    hg_return_connection(r->conn);

    return HG_OK;

}


int hg_http_core_run_phases_handler(hg_event_t *ev){

    cris_http_request_t *r=(cris_http_request_t*)ev->data;

    int rc;
    hg_http_handler_t *handlers=http_core_main_conf.phase_engine.handlers;
   
    while(handlers[r->phase_handler].checker!=NULL){
 
        rc=r->phase_handler;
        rc=handlers[rc].checker(r,&handlers[rc]);
        if(rc==HG_OK)
          return HG_AGAIN;

        else if(rc==HG_ERROR)
          hg_return_connection(r->conn);
        
        if(r->asyn_event!=NULL){
	
	     del_conn(r->conn);
	     r->conn->write->handler=NULL;
	     hg_do_asyn_event(r);
	     return HG_AGAIN;
	}
    }
    //处理完毕结束请求
    r->conn->write->handler=NULL;

    hg_http_free_request(r);//结束请求    

    return HG_OK;
}

int hg_http_core_run_phases(cris_http_request_t *r){

   int rc;
   hg_http_handler_t *handlers=http_core_main_conf.phase_engine.handlers;
   
   while(handlers[r->phase_handler].checker!=NULL){
 
       rc=r->phase_handler;
       rc=handlers[rc].checker(r,&handlers[rc]);
       if(rc==HG_OK){
         r->conn->write->data=(void*)r;
         r->conn->write->handler=&hg_http_core_run_phases_handler;
         add_conn(r->conn);//将写事件加入监听
         return HG_AGAIN;

       }else if(rc==HG_ERROR)
         hg_return_connection(r->conn);

       if(r->asyn_event!=NULL){	

           del_conn(r->conn);
	   r->conn->write->handler=NULL;
	   hg_do_asyn_event(r);
	   return HG_AGAIN;
       }
   }

   hg_http_free_request(r);

   return HG_OK;
}

int hg_http_special_response_process(cris_http_request_t *r,int rc){

    r->response.access_code=rc;
    r->access_code=rc;

    return HG_OK;
}

//该函数只会在发送阶段的checker函数中调用一次

cris_buf_t* hg_http_tile_response(cris_http_request_t *r){

      cris_mpool_t *pool=r->pool;
      cris_buf_t *buf=NULL;
      hg_http_response_t &rp=r->response;

      if(r->access_code==0)//没有处理该内容
         r->access_code=HG_HTTP_NOT_FOUND;

      int cnt=0;     

      str_info s=hg_rp_line[r->access_code];

      cnt+=s.len;

      if(rp.content_type!=0)
         cnt+=12+4+30;//预留字节
      if(rp.content_length!=0)
         cnt+=14+4+32;//预留字节
   //   if(rp.server!=NULL)
         cnt+=18;

      cris_http_header_t *h=rp.headers;

      while(h){
         cnt+=h->name.len+2;
         cnt+=h->content.len+2;      
         h=h->next; 
      }
      cnt+=2;

      void*p=pool->qlloc(sizeof(cris_buf_t));buf=new (p)cris_buf_t(pool,cnt);
     
      buf->append(s.str,s.len);   
 
      if(rp.content_type!=0){
            s=hg_types[rp.content_type];         
            buf->append(s.str,s.len);
      }

      if(rp.content_length!=0){
            buf->append("Content-Length: ",16);
            std::string num=std::to_string(rp.content_length);     
            buf->append(num.c_str(),num.length());
            buf->append("\r\n",2);
      }
 
 //     if(rp.server!=NULL){
         buf->append("Server: Hgserver\r\n",18);
 //     }
 
      h=rp.headers;
 
      while(h){      
         buf->append(h->name.str,h->name.len);
         buf->append(": ",2);
         buf->append(h->content.str,h->content.len);
         buf->append("\r\n",2);
         h=h->next;
      }
 
      buf->append("\r\n",2);

      return buf;
}



int hg_http_discard_body_handler(hg_event_t *ev){

      cris_http_request_t *r=(cris_http_request_t*)ev->data;
      hg_connection_t  *conn=r->conn;

      int cnt=hg_recv_discard(conn);

      if(cnt<=0)
          hg_return_connection(conn);

      r->recv_body+=cnt;

      if(r->recv_body>=r->content_length){
 
          conn->read->handler=NULL;
          hg_http_free_request(r);//抛包是一个异步行为，可以直接结束请求
          return HG_OK;
      }
      return HG_AGAIN;
}


int hg_http_discard_body(cris_http_request_t *r){

    hg_connection_t *conn=r->conn;
    cris_buf_t      *buffer=conn->in_buffer; 
   
    r->recv_body=buffer->available();

    if(r->recv_body>=r->content_length){
         buffer->cur+=r->content_length;
         return HG_OK;
    }else{
         buffer->cur+=buffer->available();
    }

    
    r->count++;
    conn->read->handler=&hg_http_discard_body_handler;

    return HG_AGAIN;

};

/*
int hg_http_read_body_handler(hg_event_t *ev){

    cris_http_request_t *r=(cris_http_request_t*)ev->data;
    hg_connection_t *conn=r->conn;
    hg_http_core_loc_conf_t *loc_conf=hg_get_loc_conf(hg_http_core_loc_conf_t,(&hg_http_core_module),(r->loc_conf));   
    
    cris_buf_t *buf=conn->in_buffer;
    conn->in_buffer=r->body.temp;

    if(loc_conf->body_in_file){
    
    
    }else{
    
        int rc=0,cnt=0;    
              
        rc=hg_recv_chain(conn,cnt);	

	if(rc==HG_ERROR){
	 
	    hg_return_connection(conn);
	    return HG_ERROR;
	
	}
    
        r->recv_body=r->recv_body+cnt;

        if(r->recv_body>=r->content_length){

              buf->reuse();       
	      buf->append(extra_buf,rc-cnt);
              conn->in_buffer=buf;
	      conn->read->handler=NULL;
              (r->body.callback)(r);
	      hg_http_free_request(r);
	      return HG_OK;
        }

    }

    return HG_AGAIN;
}


int hg_http_read_body(cris_http_request_t *r,int (*callback)(cris_http_request_t *)){

    hg_connection_t *conn=r->conn;    
    hg_http_core_loc_conf_t *loc_conf=hg_get_loc_conf(hg_http_core_loc_conf_t,(&hg_http_core_module),(r->loc_conf));

    r->recv_body=conn->in_buffer->available();

    hg_http_body_t *body=&(r->body);
 
    cris_buf_t *conn_buf=conn->in_buffer;

    if(loc_conf->body_in_file){//将包体保存在文件当中
     
    
    }else{//在内存中处理包体
	  
	  if(r->recv_body>=r->content_length){
	  
                r->recv_body=r->content_length;

		body->temp=new (r->pool->qlloc(sizeof(cris_buf_t)))cris_buf_t(r->pool,conn_buf->cur,r->content_length);

		conn_buf->cur=conn_buf->cur+r->content_length;
		conn_buf->res=conn_buf->res-r->content_length;
		conn_buf->used=conn_buf->used+r->content_length;

		if(callback!=NULL)
		   return callback(r);
	        return HG_OK;

	  }else{
	  
                body->temp=new (r->pool->qlloc(sizeof(cris_buf_t)))cris_buf_t(r->pool,conn_buf->cur,r->recv_body);               
                
		conn_buf->cur=conn_buf->cur+r->recv_body;
		conn_buf->res=conn_buf->res-r->recv_body;
		conn_buf->used=conn_buf->used+r->recv_body;
		body->callback=callback;
	        body->temp->prealloc(r->content_length-r->recv_body,1);//预分配
                
		r->count++;
		conn->read->handler=&hg_http_read_body_handler;
	        return HG_AGAIN;
	  }
    
    }

    return HG_OK;

}
*/

int  hg_http_block_write_handler(hg_event_t *ev){

     return HG_OK;
}

int  hg_http_block_read_handler(hg_event_t *ev){

     return HG_OK;    
}


int  hg_http_core_init_process(hg_module_t *module,hg_cycle_t *cycle){

     hg_http_conf_t *conf=(hg_http_conf_t*)hg_get_module_conf(module,cycle);

     hg_http_core_main_conf_t *main_conf=hg_get_main_conf(hg_http_core_main_conf_t,module,conf);

     cris_mpool_t *pool=cycle->pool;

     hg_http_phase_engine_t &engine=main_conf->phase_engine;

     std::vector<hg_http_request_pt> *p=main_conf->phases;

     //添加占位函数，防止跳过内容处理阶段，而忽略掉特殊处理函数
     hg_http_add_request_handler(&hg_http_null_content_handler,HG_HTTP_CONTENT_PHASE);

     int last_content_index=0;
     int last_access_index=0;
     int response_index=0;

     int cnt=0;

     cnt+=p[0].size();
     cnt+=p[1].size();
     cnt+=1;
     cnt+=p[3].size();
     cnt+=1;
     cnt+=p[5].size();
     cnt+=p[6].size();
     last_access_index=cnt-1;//最后一个权限处理方法的下标
     cnt+=1;
     cnt+=1;        
     cnt+=p[9].size();
     last_content_index=cnt-1;//最后一个内容处理方法的下标

     cnt+=1;//结束子请求或则发送请求阶段
     response_index=cnt-1;

     cnt+=p[11].size();
     cnt+=1;//添加一个空处理表示请求处理完毕

     engine.handlers=(hg_http_handler_t*)pool->alloc(sizeof(hg_http_handler_t)*cnt);
          
     hg_http_check_pt checker=NULL;

     int index=0;
 
     for(int i=0;i<HG_HTTP_PHASE_NUM;i++){

         if(i==HG_HTTP_POST_READ_PHASE)
             checker=&hg_http_core_common_phase;

         if(i==HG_HTTP_SERVER_REWRITE_PHASE)
             checker=&hg_http_core_rewrite_phase;

         if(i==HG_HTTP_FIND_CONFIG_PHASE){
             engine.handlers[index].checker=&hg_http_core_find_config_phase;
             engine.handlers[index++].handler=NULL;
             continue;
         }

         if(i==HG_HTTP_REWRITE_PHASE)
              checker=&hg_http_core_rewrite_phase;

         if(i==HG_HTTP_POST_REWRITE_PHASE){
              engine.handlers[index].checker=&hg_http_core_post_rewrite_phase;
              engine.handlers[index++].handler=NULL;
              continue;
         }
 
         if(i==HG_HTTP_PREACCESS_PHASE)
               checker=&hg_http_core_common_phase;

         if(i==HG_HTTP_ACCESS_PHASE)
               checker=&hg_http_core_access_phase;

         if(i==HG_HTTP_POST_ACCESS_PHASE){
              engine.handlers[index].checker=&hg_http_core_post_access_phase;
              engine.handlers[index++].handler=NULL;
              continue;
         }

         if(i==HG_HTTP_TRY_FILES_PHASE){
              engine.handlers[index].checker=&hg_http_core_try_files_phase;
              engine.handlers[index++].handler=NULL;
              continue;
         }

         if(i==HG_HTTP_CONTENT_PHASE)
                checker=&hg_http_core_content_phase;

         if(i==HG_HTTP_RESPONSE_PHASE){
              engine.handlers[index].checker=&hg_http_core_response_phase;
              engine.handlers[index++].handler=NULL;
              continue;
         }

         if(i==HG_HTTP_LOG_PHASE)
                checker=&hg_http_core_common_phase;
     
         for(hg_http_request_pt pt:p[i]){

            engine.handlers[index].handler=pt;
            engine.handlers[index].checker=checker;
            index++;
         }

     }

     int next=cnt;
     checker=NULL;

     for(int i=cnt-1;i>=0;i--){

        engine.handlers[i].last_access_index=last_access_index;
        engine.handlers[i].last_content_index=last_content_index;
        engine.handlers[i].response_index=response_index;

        if(engine.handlers[i].checker!=checker){
               next=i+1;
               engine.handlers[i].next=next;
               checker=engine.handlers[i].checker;
        }else{
               engine.handlers[i].next=next;
        }
     } 

     engine.handlers[cnt-1].checker=NULL;//最后一个为空处理
     return HG_OK;

}




int  hg_http_core_response_phase(cris_http_request_t *r,hg_http_handler_t *ph){
     
     printf("hg_http_core_response_phase\n");

     if(!r->responsing){

         if(r->skip_response){

	    printf("skip response\n");

	    goto next;
         }

         r->responsing=true;

         hg_connection_t *conn=r->conn;        
         hg_http_response_t &rp=r->response;

         conn->out_buffer=hg_http_tile_response(r);//展开首部

             hg_send(conn);
    
         if(conn->out_buffer->available()==0){//发送完首部部分

            rp.send_head=true;

            if(rp.has_body){

               if(rp.body!=NULL){//缓冲式包体
                                           
                  conn->out_buffer=rp.body;

                  hg_send(conn);

                  if(rp.body->available()==0){
                     goto next;
                  } else {
                     return HG_OK;    
		  }

               }else{
		    
                  sendfile(conn->fd,rp.fd,&rp.body_send,rp.content_length-rp.body_send);

                  if(rp.body_send<rp.content_length){
                     return HG_OK;
                  }else{
                     close(rp.fd);
                     goto next;
                  }
               }
    

           }else{//没有包体转入下一阶段
                   goto next;
  
           }      


       }else
          return HG_OK;//请求再次调度

   }else{

         hg_connection_t *conn=r->conn;        
         hg_http_response_t &rp=r->response;

 
         if(rp.send_head){//头部发送完成
 
                 
              if(rp.body==NULL){

                  sendfile(conn->fd,rp.fd,&rp.body_send,rp.content_length-rp.body_send);
                 
                  if(rp.body_send<rp.content_length){
         //             printf("未完成 length:%d\n",rp.body_send);
                     return HG_OK;
                  }else{
                     close(rp.fd);
                     goto next;
                  }

              }else{      

                  hg_send(conn);
                 
                  if(conn->out_buffer->available()==0)
                       goto next;
                  else
                       return HG_OK;

              }



         }else{
       
              hg_send(conn);
 
              if(conn->out_buffer->available()==0){//头部发送完成
                
                   rp.send_head=true;

                   if(rp.has_body){

                       if(rp.body!=NULL){
                                           
                          conn->out_buffer=rp.body;

                          hg_send(conn);
                                 
                          if(rp.body->available()==0)
                             goto next;
                          else 
                             return HG_OK;         

                       }else{

                          sendfile(conn->fd,rp.fd,&rp.body_send,rp.content_length-rp.body_send);
                        
                          if(rp.body_send<rp.content_length){
                            //  printf("未完成 length:%d\n",rp.body_send);
                             return HG_OK;
                          }else{
                             close(rp.fd);
                             goto next;
                          }
                       }
    

                  }else{
                      goto next;
  
                  }      

             }else
                 return HG_OK;
        }

  }

/*err://通常这个错误是对方断开连接，直接返还连接，不再处理

     if(r->response.fd!=0)
        close(r->response.fd);

     return HG_ERROR;//交给core_run_phase进行关闭操作
*/
next://进入下一阶段
     r->phase_handler++;
     return HG_AGAIN;
}




int  hg_http_core_try_files_phase(cris_http_request_t *r,hg_http_handler_t *ph){


     hg_http_core_loc_conf_t *loc=hg_get_loc_conf(hg_http_core_loc_conf_t,(&hg_http_core_module),r->loc_conf);

   //  cris_str_print(&(loc->name));

   //  printf("ty=%d\n",loc->filetype);

     if(loc->static_file){//当前文件为静态文件直接发送

        r->url.str[r->url.len]='\0';
        
        hg_http_response_t &rp=r->response;

        r->access_code=HG_HTTP_OK;

        rp.fd=open(r->url.str+1,O_RDONLY);
        rp.has_body=rp.fd<0?false:true;

        if(rp.fd>0){

           rp.content_length=lseek(rp.fd,0,SEEK_END);
           lseek(rp.fd,0,SEEK_SET);
        }else{
             
           hg_http_special_response_process(r,HG_HTTP_NOT_FOUND);

           r->phase_handler=ph->response_index;

           return HG_AGAIN;
        }

        rp.content_type=loc->filetype;

        rp.body=NULL;

        r->phase_handler=ph->response_index;//直接跳转到响应阶段
        return HG_AGAIN;
     }


     r->phase_handler++;//继续常规处理

     return HG_AGAIN;
}



int  hg_http_core_post_access_phase(cris_http_request_t *r,hg_http_handler_t *ph){

     r->phase_handler++;

     return HG_AGAIN;      
}


int  hg_http_core_post_rewrite_phase(cris_http_request_t *r,hg_http_handler_t *ph){

      if(r->count_rewrite>10){
         
         hg_http_special_response_process(r,HG_HTTP_INTERNAL_SERVER_ERROR);     

         r->phase_handler=ph->response_index;//发送错误响应

         return HG_AGAIN;
      }
 
      r->phase_handler++;

      return HG_AGAIN;     

}


int  hg_http_core_find_config_phase(cris_http_request_t *r,hg_http_handler_t *ph){

     hg_http_core_srv_conf_t *server=r->server;

     r->loc_conf=hg_http_find_location_conf(r->url,0,server->loc_tree);

     if(r->loc_conf==NULL){

          hg_http_special_response_process(r,HG_HTTP_FORBIDDEN);
     
          r->phase_handler=ph->response_index;

          return HG_AGAIN;

     }
    
     hg_http_core_loc_conf_t *loc_conf=hg_get_loc_conf(hg_http_core_loc_conf_t,(&hg_http_core_module),(r->loc_conf));

     printf("find config handler %p\n",loc_conf->content_handler);

     r->content_handler=loc_conf->content_handler;

     r->phase_handler=ph->next;

     return HG_AGAIN;     

}



int  hg_http_core_content_phase(cris_http_request_t *r,hg_http_handler_t *ph){

//     printf("hg_http_core_content_phase\n");

     if(r->content_handler!=NULL){

         int rc=r->content_handler(r);

         if(rc==HG_OK){
               
             r->phase_handler=ph->response_index;
             return HG_AGAIN;  

         }

         if(rc==HG_AGAIN){        

             return HG_OK;
         
         }

         hg_http_special_response_process(r,HG_HTTP_INTERNAL_SERVER_ERROR);//返回其他值，服务器内部错误

         r->phase_handler=ph->response_index;
         
         return HG_AGAIN;

     }

       
     int rc=ph->handler(r);

     if(rc==HG_DECLINED){

           r->phase_handler++;
           return HG_AGAIN;

     }

     if(rc==HG_OK){
               
           r->phase_handler=ph->response_index;
           return HG_AGAIN;  

     }

     if(rc==HG_AGAIN){        

           return HG_OK;
         
     }

     if(rc==HG_HTTP_DISCONNECT){

           return HG_ERROR;

     }

     hg_http_special_response_process(r,HG_HTTP_INTERNAL_SERVER_ERROR);//返回其他值，服务器内部错误

     r->phase_handler=ph->response_index;

     return HG_AGAIN; 
}


int  hg_http_core_access_phase(cris_http_request_t *r,hg_http_handler_t *ph){

     if(r->parent!=NULL){//子请求跳过权限判定阶段
        r->phase_handler=ph->next;
        return HG_AGAIN;
     }   
 
     int rc=ph->handler(r);

     if(rc==HG_AGAIN)
          return HG_OK;
 

     hg_http_core_loc_conf_t *loc=hg_get_loc_conf(hg_http_core_loc_conf_t,(&hg_http_core_module),r->loc_conf);

     if(loc->satisfy==ANY){

         if(rc==HG_HTTP_FORBIDDEN){
       
             r->phase_handler++;

             if(r->phase_handler>ph->last_access_index){

                hg_http_special_response_process(r,rc);//这里是传入的是禁止码

                r->phase_handler=ph->response_index;
 
                return HG_AGAIN;

             }
             return HG_AGAIN;  
         }

         if(rc==HG_OK){

             r->phase_handler=ph->next;
             return HG_AGAIN;
         }

         hg_http_special_response_process(r,HG_HTTP_INTERNAL_SERVER_ERROR);//返回其他值，服务器内部错误

         r->phase_handler=ph->response_index;
         
         return HG_AGAIN;

     }else{//==ALL

         if(rc==HG_OK){
            r->phase_handler++;
            return HG_AGAIN;           
         }
        
         hg_http_special_response_process(r,HG_HTTP_FORBIDDEN);//除了OK，均构造禁止码

         r->phase_handler=ph->response_index;
      
         return HG_AGAIN; 

     }
    
     return HG_AGAIN;
}

int  hg_http_core_rewrite_phase(cris_http_request_t *r,hg_http_handler_t *ph){

     int rc=ph->handler(r);

     if(rc==HG_DECLINED){
         r->phase_handler++;
         return HG_AGAIN;
     }

     if(rc==HG_OK){

         r->phase_handler=ph->next;
         return HG_AGAIN;
     }   

     hg_http_special_response_process(r,HG_HTTP_INTERNAL_SERVER_ERROR);

     r->phase_handler=ph->response_index;

     return HG_AGAIN;
}

int  hg_http_core_common_phase(cris_http_request_t *r,hg_http_handler_t *ph){

     int rc=ph->handler(r);

     if(rc==HG_OK){
         r->phase_handler=ph->next;
         return HG_AGAIN;
     }

     if(rc==HG_DECLINED){
         r->phase_handler++;
         return HG_AGAIN;
     }

     if(rc==HG_AGAIN)
         return HG_OK;

     hg_http_special_response_process(r,HG_HTTP_INTERNAL_SERVER_ERROR);

     r->phase_handler=ph->response_index;

     return HG_AGAIN;
}



void test_tree(hg_http_loc_tree_node_t *tree){
     
     if(tree==NULL)
       return;

     test_tree(tree->left);

     printf("----------------\n");
     cris_str_print(&tree->url);
     int a=tree->exact==NULL?0:1;
     int b=tree->inclusive==NULL?0:1;
     int c=tree->limit?1:0;

     int d=0;

     hg_http_loc_que_node_t *r=tree->regexes;

     while(r){
       d++;
       r=r->next;
     }


     printf("  精准:%d  前缀:%d  限制:%d   正则:%d\n------------------\n",a,b,c,d);

     test_tree(tree->c_tree);
     test_tree(tree->right);
}


//第二个参数为已经匹配的长度
hg_http_conf_t*   hg_http_find_location_conf(cris_str_t &url,int off,hg_http_loc_tree_node_t *tree){

         if(tree==NULL)
            return NULL;

         if(url.len<tree->url.len)//如果url长度小于该配置，则不会以该配置为前缀或相等，只能在左子树查找
            return hg_http_find_location_conf(url,off,tree->left);

         int m=0;
         cris_str_t &r_url=tree->url;
         char *p1=url.str;
         char *p2=r_url.str;

         int len=r_url.len;

         hg_http_conf_t   *rc=NULL; 

         for(int i=off;i<len&&m==0;i++)
             m=p1[i]-p2[i];   
  
         if(m==0){

             if(r_url.len<url.len){//为url的前缀

                 if(tree->limit)
                    return tree->inclusive->ctx;

                 rc=hg_http_find_location_conf(url,r_url.len,tree->c_tree);//在子树中继续查找

                 rc=rc==NULL?tree->inclusive->ctx:rc;       

             }else{//相等的情况，且只能为相等的情况

                 rc=tree->exact!=NULL?tree->exact->ctx:tree->inclusive->ctx;//如果该节点有精准匹配，优先精准匹配

             }


         }else{

              rc=hg_http_find_location_conf(url,off,m>0?tree->right:tree->left);

         }

         return rc;

}


hg_http_loc_tree_node_t* hg_init_locations_tree(hg_cycle_t *cycle,hg_http_loc_que_node_t *que){
   
 //  printf("hg_init_locations_tree\n");
 
   if(que==NULL)
     return NULL;

   hg_http_loc_que_node_t *head=que;
   hg_http_loc_que_node_t *mid=que;
   hg_http_loc_que_node_t *tail=que->next;

   while(tail!=NULL&&tail->next!=NULL){
       mid=mid->next;
       tail=tail->next->next;
   }

   if(mid->pre!=NULL)
      mid->pre->next=NULL;
   if(mid->next!=NULL)
      mid->next->pre=NULL;
   
   mid->pre=NULL;
  // mid->next=NULL;在下面处理

   hg_http_loc_tree_node_t *node=(hg_http_loc_tree_node_t*)cycle->pool->alloc(sizeof(hg_http_loc_tree_node_t));

   node->url=mid->name;
   node->exact=mid->exact;
   node->inclusive=mid->inclusive;
   node->regexes=mid->regexes;

   //处理一个或者两个节点的情况
   if(mid!=head)
     node->left=hg_init_locations_tree(cycle,head); 
   else
     node->left=NULL;

   if(mid->next!=NULL)
     node->right=hg_init_locations_tree(cycle,mid->next);
   else
     node->right=NULL;

   mid->next=NULL;


   hg_http_core_loc_conf_t *loc=mid->inclusive;//精准匹配不含子配置

//该情况只可能为该配置项为精准匹配
   if(loc==NULL){//没有子配置了，直接返回
      node->limit=false;
      node->c_tree=NULL;
      return node;
   }

   node->limit=loc->limit_prefix;//是否为有限制的前缀匹配
   node->c_tree=hg_init_locations_tree(cycle,loc->locations);
  

   return node;
}


//精准匹配没有子loc
int   hg_init_locations(hg_http_core_loc_conf_t *loc){

 //    printf("hg_init_locations\n");

      hg_http_loc_que_node_t *que=loc->locations;
      hg_http_loc_que_node_t *reg=loc->regexes;
      //以上两个配置是同级别的
      if(que==NULL)
         return HG_OK;          
      
      hg_http_loc_que_node_t head;
      head.next=que;
      que->pre=&head;
      int n=0;
 
      while(que){
         n++;
         if(!que->set){
             que->regexes=reg;
             que->set=true;
         }
         que=que->next;
      }//算出链表数量,设置没有设置正则队列的结点

      for(int i=1;i<n;i++){

          que=head.next;

          while(que->next!=NULL){

               hg_http_loc_que_node_t *nxtmp=que->next;
               hg_http_loc_que_node_t *pre=que->pre;
               hg_http_loc_que_node_t *next=nxtmp->next;

               if(*nxtmp<*que){

                  que->pre=nxtmp;
                  nxtmp->next=que;

                  que->next=next;
                  nxtmp->pre=pre;

                  pre->next=nxtmp;
                  if(next!=NULL)
                     next->pre=que;
                  //以上已经进行que的更新操作
               }else
                  que=que->next;
         }
      }
      //以上对链表进行冒泡排序
      hg_http_loc_que_node_t *lq=head.next;
      hg_http_loc_que_node_t *x;

    //  printf("融合前\n");
     //对相同url的精准匹配节点和前缀匹配节点进行融合
      while(lq->next!=NULL){

           x=lq->next;
           //lq不是一个精准匹配节点，或者说前后的url不相等不需要被融合
           if(lq->exact==NULL||!(lq->name==x->name)){
                lq=x;
                continue;
           }
           //将lq节点从链表中删除,如果x也是exact，那只有被覆盖掉，说明配置重复了
           //因为精准匹配没有子配置，它们的子配置只有x的子配置队列，如果x也是精准匹配，则这个节点没有子配置
           x->exact=lq->exact;
           x->pre=lq->pre;
           lq->pre->next=x;           
           lq=x;

      }      
   //   printf("融合后\n");
      lq=head.next;

      while(lq!=NULL&&lq->next!=NULL){

           x=lq;
            
           //如果lq是一个精准匹配节点，则其没有子配置，也不能将同级以它为前缀的配置，放到它的子配置中
           if(lq->inclusive==NULL){
              lq=lq->next;
              continue;
           }

           //找到以lq的url为前缀的最后一个结点
           while(x->next!=NULL&&is_prefix(lq->name,x->next->name))
              x=x->next;

           if(lq==x){//没有可以放入到子队列中的结点
               lq=lq->next;
               continue;
           }

           hg_http_core_loc_conf_t *p=lq->inclusive;

           hg_http_loc_que_node_t *nx=x->next;
                                       
           que=lq->next;
           que->pre=NULL;
          
           if(p->locations!=NULL)                      
              p->locations->pre=x;

           x->next=p->locations;
           p->locations=que;
           
           if(nx!=NULL)
              nx->pre=lq;
           lq->next=nx;

           lq=lq->next;

      }

     // printf("放入子配置后\n");


      lq=head.next;
      lq->pre=NULL;//从head解绑，head只是一个辅助变量没有实际意义
      loc->locations=lq;//把新队列重新挂载到原配置上

      while(lq!=NULL){
          if(lq->inclusive!=NULL)//说明该节点为只是一个精准匹配，没有子队列
            hg_init_locations(lq->inclusive);//进行递归处理        
          lq=lq->next;

      }

     // printf("hg_init_locations end\n");

      return HG_OK;
}

//在http模块的init_module函数中被调用
int   hg_http_core_postconfiguration(hg_module_t *module,hg_cycle_t *cycle,hg_http_conf_t *conf){

 //     printf("hg_http_core_post_configuration\n");

      hg_http_core_main_conf_t *main_conf=hg_get_main_conf(hg_http_core_main_conf_t,module,conf);

      std::vector<hg_http_core_srv_conf_t*> &servers=main_conf->server;

      cris_mpool_t *pool=cycle->pool;

      for(hg_http_core_srv_conf_t *srv_conf:servers){

          hg_http_conf_t *conf_ctx=srv_conf->ctx;

          //配置的合并在init函数里已经完成，这里直接进行排序和建树

          hg_http_core_loc_conf_t *loc=hg_get_loc_conf(hg_http_core_loc_conf_t,module,conf_ctx);//找到与server同级别的loc

          hg_init_locations(loc);          

          srv_conf->loc_tree=hg_init_locations_tree(cycle,loc->locations);//构造前缀树

          server_dic[srv_conf->server_name]=srv_conf;//将主机名称插入字典

          cris_str_print(&(srv_conf->server_name));

          test_tree(srv_conf->loc_tree);

          void *p=pool->qlloc(sizeof(hg_listen_t));

          hg_listen_t *ls=new (p)hg_listen_t();
      
          ls->read_handler=&hg_http_init_request;   

          hg_add_listen(ls,cycle);
 

      } 

      
      /***********在解析完http请求后，添加处理头部的函数****************/
      hg_http_add_request_handler(&hg_http_core_set_content_length,HG_HTTP_POST_READ_PHASE);


      return HG_OK;
}



int   hg_http_merge_location(hg_cycle_t *cycle,hg_http_conf_t *p_conf,hg_http_core_loc_conf_t *loc_conf){

 //     printf("hg_http_merge_location\n");

      std::vector<hg_module_t*> *modules=cycle->modules;
      
      hg_http_loc_que_node_t *que=loc_conf->locations;
      //对非正则匹配的子队列进行融合
      while(que!=NULL){

          hg_http_core_loc_conf_t *loc=que->exact!=NULL?que->exact:que->inclusive;

          hg_http_conf_t *c_conf=loc->ctx;//子配置

          for(hg_module_t *m:*modules){

              if(m->type!=HG_HTTP_MODULE)
                   continue;
              
              hg_http_module_t *http_m=(hg_http_module_t*)m->ctx;
        
              if(http_m->merge_loc_conf)
                 http_m->merge_loc_conf(hg_get_loc_conf(void,m,p_conf),hg_get_loc_conf(void,m,c_conf));

          }

          hg_http_merge_location(cycle,c_conf,loc);
    
          que=que->next;
 
      }
       
      que=loc_conf->regexes;
      //对正则的队列进行融合
      while(que!=NULL){

          hg_http_core_loc_conf_t *loc=que->exact!=NULL?que->exact:que->inclusive;//实际上一直等于第二个参数

          hg_http_conf_t *c_conf=loc->ctx;//子配置

          for(hg_module_t *m:*modules){

              if(m->type!=HG_HTTP_MODULE)
                   continue;
              
              hg_http_module_t *http_m=(hg_http_module_t*)m->ctx;
        
              if(http_m->merge_loc_conf)
                 http_m->merge_loc_conf(hg_get_loc_conf(void,m,p_conf),hg_get_loc_conf(void,m,c_conf));

          }

          hg_http_merge_location(cycle,c_conf,loc);

          que=que->next;
     
      }

      return HG_OK;
}


//该函数主要用于合并不同级别的配置
int  hg_http_core_init_main_conf(hg_module_t *module,hg_cycle_t *cycle,hg_http_conf_t *conf){

 //    printf("hg_http_core_init_main_conf\n");

     hg_http_core_main_conf_t *core_main_conf=hg_get_main_conf(hg_http_core_main_conf_t,module,conf);

     std::vector<hg_module_t*>  *modules=cycle->modules;

     std::vector<hg_http_core_srv_conf_t*> &servers=core_main_conf->server;

     cris_mpool_t *pool=cycle->pool;      

 
     for(hg_http_core_srv_conf_t *serv:servers){

         hg_http_conf_t *srv_conf=serv->ctx;

         for(hg_module_t *m:*modules){

           if(m->type!=HG_HTTP_MODULE)
               continue;
           hg_http_module_t *http_m=(hg_http_module_t*)m->ctx;

           if(http_m->merge_srv_conf)
              http_m->merge_srv_conf(hg_get_srv_conf(void,m,conf),hg_get_srv_conf(void,m,srv_conf));
          
           if(http_m->merge_loc_conf)
              http_m->merge_loc_conf(hg_get_loc_conf(void,m,conf),hg_get_loc_conf(void,m,srv_conf));

         }                  
      
         hg_http_core_loc_conf_t *loc=hg_get_loc_conf(hg_http_core_loc_conf_t,module,srv_conf);//与Servert同级别的loction结构体
         
         hg_http_merge_location(cycle,srv_conf,loc);      

     }

     return HG_OK;
}


//将srv配置体添加到main配置体的Server数组里
int  hg_http_core_init_srv_conf(hg_module_t *module,hg_cycle_t *cycle,hg_http_conf_t *conf){

 //    printf("hg_http_core_init_srv_conf\n");
 
     hg_http_core_main_conf_t *main_conf=hg_get_main_conf(hg_http_core_main_conf_t,module,conf);

     hg_http_core_srv_conf_t  *srv_conf=hg_get_srv_conf(hg_http_core_srv_conf_t,module,conf);

     main_conf->server.push_back(srv_conf);

     return HG_OK;
}


int  hg_http_core_init_loc_conf(hg_module_t *module,hg_cycle_t *cycle,hg_http_conf_t *conf){

 //    printf("hg_http_core_init_loc_conf\n");

     cris_mpool_t *pool=cycle->pool;
     
     hg_http_core_loc_conf_t *local_loc=hg_get_loc_conf(hg_http_core_loc_conf_t,module,conf);//本级配置

     if(local_loc==NULL)
        return HG_OK;
     
     hg_http_core_loc_conf_t *parent_loc=local_loc->pre;//父级配置

     hg_http_loc_que_node_t *node=(hg_http_loc_que_node_t*)pool->alloc(sizeof(hg_http_loc_que_node_t));
     node->pre=NULL;
     node->next=NULL;
     node->exact=NULL;
     node->inclusive=NULL; 
     node->set=false;
     node->name=local_loc->name;

     if(!local_loc->exact_match)
         node->inclusive=local_loc;//如果不是精准匹配，包括正则表达式
     else
         node->exact=local_loc;

     if(parent_loc->exact_match||parent_loc->use_regex)//如果父级配置是正则匹配或者是精准匹配，则不应该挂载子配置
          return HG_OK;

     if(local_loc->use_regex){//如果使用了正则
        if(parent_loc->regexes==NULL){
           parent_loc->regexes=node;
        }else{
           parent_loc->regexes->pre=node;
           node->next=parent_loc->regexes;
           parent_loc->regexes=node;
        }

     }else{
        if(parent_loc->locations==NULL){
           parent_loc->locations=node;
        }else{
           parent_loc->locations->pre=node;
           node->next=parent_loc->locations;
           parent_loc->locations=node;
        }
     } 

     return HG_OK;
}




void*  hg_http_core_create_main_conf(hg_cycle_t *cycle){
 
 //       printf("hg_http_core_create_main_conf\n");
         
        return (void*)&http_core_main_conf;
}

void*  hg_http_core_create_srv_conf(hg_cycle_t *cycle){

 //      printf("hg_http_core_create_srv_conf\n");

       void *p=cycle->pool->alloc(sizeof(hg_http_core_srv_conf_t));
       return (void*)new (p)hg_http_core_srv_conf_t();
}


void*  hg_http_core_create_loc_conf(hg_cycle_t *cycle){

 //      printf("hg_http_core_create_loc_conf\n");      

       void *p=cycle->pool->alloc(sizeof(hg_http_core_loc_conf_t));
       return (void*)new (p)hg_http_core_loc_conf_t();
}


int hg_http_core_set_server(hg_module_t *module,hg_cycle_t *cycle,cris_conf_t *conf){

 //      printf("hg_http_core_set_server\n");

      if(conf->avgs.size()!=2)
         return HG_ERROR;


      cris_mpool_t *pool=cycle->pool;
      std::vector<hg_module_t*> *modules=cycle->modules;
      
      hg_http_module_t *module_ctx=(hg_http_module_t*)module->ctx;
      
      hg_http_conf_t *parent_conf=module_ctx->ptr;//取父级配置

      hg_http_conf_t *local_conf=(hg_http_conf_t*)pool->alloc(sizeof(hg_http_conf_t));//分配本级配置

      local_conf->main_conf=parent_conf->main_conf;//直接指向父级的main配置

      local_conf->srv_conf=(void**)pool->alloc(num_http_modules*sizeof(void*));//分配新的server以及location配置
      local_conf->loc_conf=(void**)pool->alloc(num_http_modules*sizeof(void*));//


      for(hg_module_t *m:*modules){

          if(m->type!=HG_HTTP_MODULE)
             continue;

          hg_http_module_t *http_m=(hg_http_module_t*)m->ctx;

          if(http_m->create_srv_conf)
             local_conf->srv_conf[m->ctx_index]=(void*)http_m->create_srv_conf(cycle);

          if(http_m->create_loc_conf)
             local_conf->loc_conf[m->ctx_index]=(void*)http_m->create_loc_conf(cycle);

      }



      hg_http_core_srv_conf_t *srv_conf=hg_get_srv_conf(hg_http_core_srv_conf_t,module,local_conf);
      
      srv_conf->server_name=conf->avgs.front();//第一个参数为主机名

      conf->avgs.pop_front();

      srv_conf->ctx=local_conf;//设置本级配置的上下文

      cris_str_t  block=conf->avgs.front();

      char *pre=block.str;
      char *end=pre+block.len; 
      cris_conf_t  conf_tmp;


      while((pre=cris_take_one_conf(pre,end,&conf_tmp))!=NULL){

            bool found=false;             
 
            for(hg_module_t *m:*modules){

                if(m->type!=HG_HTTP_MODULE)
                   continue;
                
                std::vector<hg_command_t> *commands=m->commands; 
     
                for(hg_command_t &cd:*commands){

                    if(!(cd.conf_name==conf_tmp.name))
                       continue;
         
                    found=true;
 
                    ((hg_http_module_t*)m->ctx)->ptr=local_conf;

                    cd.conf_handler(m,cycle,&conf_tmp);
                                     
                    break;
                    
                }
 
                if(found)
                   break;
 
            }

     }


     for(hg_module_t *m:*modules){

          if(m->type!=HG_HTTP_MODULE)
               continue;
 
          hg_http_module_t *http_m=(hg_http_module_t*)m->ctx;

          if(http_m->init_srv_conf)
               http_m->init_srv_conf(m,cycle,local_conf);

     }


     return HG_OK;
}



int hg_http_core_set_location(hg_module_t *module,hg_cycle_t *cycle,cris_conf_t *conf){

 //     printf("hg_http_core_set_location\n");


      cris_mpool_t *pool=cycle->pool;
      std::vector<hg_module_t*> *modules=cycle->modules;
      
      hg_http_module_t *module_ctx=(hg_http_module_t*)module->ctx;
      
      hg_http_conf_t *parent_conf=module_ctx->ptr;//获取父级配置

      hg_http_conf_t *local_conf=(hg_http_conf_t*)pool->alloc(sizeof(hg_http_conf_t));
      //除了location配置，其它均指向父级配置
      local_conf->main_conf=parent_conf->main_conf;
      local_conf->srv_conf=parent_conf->srv_conf;
    
 
      local_conf->loc_conf=(void**)pool->alloc(num_http_modules*sizeof(void*));

      for(hg_module_t *m:*modules){

          if(m->type!=HG_HTTP_MODULE)
             continue;

          hg_http_module_t *http_m=(hg_http_module_t*)m->ctx;

          if(http_m->create_loc_conf)
             local_conf->loc_conf[m->ctx_index]=(void*)http_m->create_loc_conf(cycle);

      }
      //获取本次创建的hg_http_core_loc_t结构体
      hg_http_core_loc_conf_t *loc_conf=hg_get_loc_conf(hg_http_core_loc_conf_t,module,local_conf);

      //设置其父配置 
      loc_conf->pre=hg_get_loc_conf(hg_http_core_loc_conf_t,module,parent_conf);        


      //对location的参数进行设置
      if(conf->avgs.size()==2){
         //直接前缀   
         loc_conf->name=conf->avgs.front();
         conf->avgs.pop_front();

      }else if(conf->avgs.size()==3){

         if(conf->avgs.front()==std::string("="))//精准匹配
             loc_conf->exact_match=true;
         else if(conf->avgs.front()==std::string("~"))//正则匹配
             loc_conf->use_regex=true;
         else if(conf->avgs.front()==std::string("^~"))//有限制的前缀匹配
             loc_conf->limit_prefix=true;
         else
             return HG_ERROR;
         conf->avgs.pop_front();

         loc_conf->name=conf->avgs.front();//其名为第二个参数
         conf->avgs.pop_front();

      }else
         return HG_ERROR;
 
     
      loc_conf->ctx=local_conf;//指向本级配置     


      cris_str_t  block=conf->avgs.front();

      char *pre=block.str;
      char *end=pre+block.len; 
      cris_conf_t  conf_tmp;

      while((pre=cris_take_one_conf(pre,end,&conf_tmp))!=NULL){

            bool found=false;             
 
            for(hg_module_t *m:*modules){

                if(m->type!=HG_HTTP_MODULE)
                   continue;
                
                std::vector<hg_command_t> *commands=m->commands; 
     
                for(hg_command_t &cd:*commands){

                    if(!(cd.conf_name==conf_tmp.name))
                       continue;
         
                    found=true;
 
                    ((hg_http_module_t*)m->ctx)->ptr=local_conf;

                    cd.conf_handler(m,cycle,&conf_tmp);
                                     
                    break;
                    
                }
 
                if(found)
                   break;
 
            }

     }


     for(hg_module_t *m:*modules){

          if(m->type!=HG_HTTP_MODULE)
               continue;
 
          hg_http_module_t *http_m=(hg_http_module_t*)m->ctx;

          if(http_m->init_loc_conf)
               http_m->init_loc_conf(m,cycle,local_conf);

     }

     return HG_OK;
}

int  hg_http_core_set_static(hg_module_t *module,hg_cycle_t *cycle,cris_conf_t *conf){

    int num=conf->avgs.size();
    if(num==0||num>1)
      return HG_OK;
    hg_http_module_t *http_m=(hg_http_module_t*)module->ctx;
    hg_http_conf_t   *parent_conf=http_m->ptr;

    hg_http_core_loc_conf_t *loc=hg_get_loc_conf(hg_http_core_loc_conf_t,module,parent_conf);

    cris_str_t on=conf->avgs.front();

    if(on==std::string("on"))
       loc->static_file=true;

    return HG_OK;
}

int  hg_http_core_set_filetype(hg_module_t *module,hg_cycle_t *cycle,cris_conf_t *conf){

    if(conf->avgs.size()==0||conf->avgs.size()>1)
         return HG_ERROR;

    cris_str_t &set=conf->avgs.front();
    set.str[set.len]='\0';
    std::string s(set.str);

    hg_http_module_t *http_m=(hg_http_module_t*)module->ctx;
    
    hg_http_conf_t   *parent_conf=http_m->ptr;

    hg_http_core_loc_conf_t *loc=hg_get_loc_conf(hg_http_core_loc_conf_t,module,parent_conf);

    if(set_types.count(s)==0){
        printf("未识别%s\n",set.str);
        return HG_OK;
    }

    loc->filetype=set_types[s];

    return HG_OK;

}

/*剥离已经接受到的包体*/
int hg_http_peel_body(cris_http_request_t *r){

    if(r->content_length==0)
      return HG_OK;

    hg_connection_t *conn=r->conn;

    cris_buf_t *conn_buf=conn->in_buffer;

    r->recv_body=conn_buf->available();

    hg_http_body_t *body=&(r->body);

	  if(r->recv_body>=r->content_length){
	  
                body->state=HG_HTTP_BODY_COMPLETE;

                r->recv_body=r->content_length;

		body->temp=new (r->pool->qlloc(sizeof(cris_buf_t)))cris_buf_t(r->pool,conn_buf->cur,r->content_length);

		conn_buf->cur=conn_buf->cur+r->content_length;
		conn_buf->res=conn_buf->res-r->content_length;
		conn_buf->used=conn_buf->used+r->content_length;

	  }else{
	  
                body->temp=new (r->pool->qlloc(sizeof(cris_buf_t)))cris_buf_t(r->pool,conn_buf->cur,r->recv_body);               
                
		conn_buf->cur=conn_buf->cur+r->recv_body;
		conn_buf->res=conn_buf->res-r->recv_body;
		conn_buf->used=conn_buf->used+r->recv_body;
	  }

    return HG_OK;
}


int hg_http_init_request(hg_event_t *ev){

    void *p=NULL;
    int  rc=0;
    hg_connection_t *conn=(hg_connection_t*)ev->data;

    if(ev->timeout){
       hg_return_connection(conn);
       return HG_OK;  
    }else if(ev->in_time){
       hg_del_timeout(ev);
    }      

    cris_mpool_t *pool=new cris_mpool_t();//创建内存池

    conn->pool=pool;

    p=pool->qlloc(sizeof(cris_buf_t));

    conn->in_buffer=new (p)cris_buf_t(pool,2048);

    cris_http_request_t *r=(cris_http_request_t*)pool->alloc(sizeof(cris_http_request_t));
 
    r->pool=pool; 
    r->conn=conn;
    r->count=1;

    conn->data=(void*)r;
    conn->read->data=(void*)r;

    if(hg_recv(conn)<=0)//从套接字缓存中读取数据到缓存中
       goto err;      

    rc=hg_http_request_parse(r,conn->in_buffer);

    if(rc==HG_OK){
        
        r->server=server_dic[r->headers_in.host->content];
     
        conn->read->handler=NULL;//不处理读事件但是仍然监听

        hg_http_peel_body(r);//剥离已经接收到的包体

        hg_http_core_run_phases(r);//

    }else if(rc==HG_AGAIN){
        conn->read->handler=&hg_http_init_request_handler;
        hg_add_timeout(ev,2000);

    }else
        goto err;

    return HG_OK;

err:
    hg_return_connection(conn);//自动关闭连接，和退出epoll,计数器用于正常关闭，此处发生错误，进行强制关闭，不会产生资源泄露

    return HG_OK;
}

int hg_http_init_request_handler(hg_event_t *ev){
    
    cris_http_request_t *r=(cris_http_request_t*)ev->data;
    hg_connection_t     *conn=r->conn;
    int rc=0; 

    if(ev->timeout){//超时关闭连接
       hg_return_connection(conn);
       return HG_OK;
    }else if(ev->in_time){
       hg_del_timeout(ev);//删除计时
    }

    if(hg_recv(conn)<=0)
         goto err;

    rc=hg_http_request_parse(r,conn->in_buffer);

    if(rc==HG_OK){
  
        r->server=server_dic[r->headers_in.host->content];
   
        conn->read->handler=NULL;//不再处理读事件,但仍然监听

        hg_http_peel_body(r);

        hg_http_core_run_phases(r);


    }else if(rc==HG_AGAIN){
        
        hg_add_timeout(ev,3000);

    }else
        goto err;

    return HG_OK;

err:

    hg_return_connection(conn);//解析错误，关闭连接

    return HG_ERROR;
}



int hg_http_core_set_content_length(cris_http_request_t *r){

      cris_http_header_t *content_length=r->headers_in.content_length;
   
      if(content_length==NULL){
          r->content_length=0;
	  return HG_OK;
      }

      r->content_length=atoi(content_length->content.str);

      return HG_DECLINED;

}



#endif

















