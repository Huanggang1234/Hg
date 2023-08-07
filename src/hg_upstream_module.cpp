#ifndef HG_UPSTREAM_MODULE_CPP
#define HG_UPSTREAM_MODULE_CPP

#include"../include/hg_http_module.h"
#include"../include/hg_upstream_module.h"
#include"../include/hg_epoll_module.h"
#include"../include/hg_http_core_module.h"
#include"../base/cris_buf.h"
#include"../base/cris_memery_pool.h"
#include"../include/hg_define.h"



hg_upstream_t*  hg_add_upstream(cris_http_request_t *r){

      hg_upstream_t *up= new (r->pool->qlloc(sizeof(hg_upstream_t)))hg_upstream_t();

      up->next=r->upstream;
      r->upstream=up;

      up->r=r;

      return up;
}


int hg_upstream_initial(hg_upstream_t *upstream){

    hg_connection_t *conn=upstream->conn;

    if(conn==NULL)
      return HG_ERROR;

    if(hg_set_sock(conn)==HG_ERROR)
      return HG_ERROR;
    if(hg_set_address(conn,"127.0.0.1",9197)==HG_ERROR)
      return HG_ERROR;
    
    int rc=hg_connect(conn);
    return rc;
}

int hg_upstream_parse(hg_upstream_t *upstream){

    int rc;
 
    if(upstream->parse_state==HG_UPSTREAM_PARSE_HEADER){
    
        if((rc=(upstream->hg_upstream_parse_header)(&(upstream->conn->in_buffer),upstream->data))==HG_OK){
	
	    upstream->parse_state=HG_UPSTREAM_PARSE_BODY;

	}   
    }

    if(upstream->parse_state==HG_UPSTREAM_PARSE_BODY){
    
        if((rc=(upstream->hg_upstream_parse_body)(&(upstream->conn->in_buffer),upstream->data))==HG_OK){
	
            upstream->parse_state=HG_UPSTREAM_PARSE_HEADER;

	} 
            
        if(rc!=HG_ERROR&&upstream->hg_upstream_post_parse_body!=NULL)
            (upstream->hg_upstream_post_parse_body)(&(upstream->conn->in_buffer),upstream->data);
    }

    return rc;
}

int hg_upstream_handler(hg_event_t *ev){

    return hg_upstream_activate((hg_upstream_t *)ev->data);

}


int hg_upstream_activate(void *data){

    hg_upstream_t *upstream=(hg_upstream_t*)data;
    hg_connection_t *conn=upstream->conn;
    int state=upstream->upstream_state;
    bool again=true;
    int  rc=HG_AGAIN;
    int  cnt=0;

    while(again){

        switch(state){
    
           case HG_UPSTREAM_INITIAL:
        
                conn=upstream->conn=hg_get_connection();

                if((rc=hg_upstream_initial(upstream))==HG_OK){
		   state=HG_UPSTREAM_CREATE_REQUEST;
                }

                if(rc==HG_AGAIN){
                   state=HG_UPSTREAM_CREATE_REQUEST;
	           conn->write->data=(void*)upstream;
                   conn->write->handler=&hg_upstream_handler;
                   add_write(conn);
                   again=false;
		}
		if(rc==HG_ERROR)
		   again=false;

                break;

           case HG_UPSTREAM_CREATE_REQUEST:

                if((rc=(upstream->hg_upstream_create_request)(&conn->out_buffer,upstream->data))!=HG_OK)
                   again=false;
                else{
		   state=HG_UPSTREAM_SEND_REQUEST;  
		}

         	if(rc==HG_AGAIN&&!conn->in_write){

                   conn->write->data=(void*)upstream;
                   conn->write->handler=&hg_upstream_handler;
                   add_write(conn);

	        }

	        if(rc!=HG_ERROR){
	          cnt=hg_send(conn);
		  printf("send cnt=%d\n",cnt);
		}
		break;

           case HG_UPSTREAM_SEND_REQUEST:

                if(conn->out_buffer->available()>0){

		    cnt= hg_send(conn);
                  
                    printf("send cnt=%d\n",cnt);
                }
		
		if(conn->out_buffer->available()==0){
		
		   state=HG_UPSTREAM_PARSE;

		   add_read(conn);
		   conn->write->handler=NULL;
		   conn->read->data=(void*)upstream;
		   conn->read->handler=&hg_upstream_handler;

//                 cris_buf_t *buf=new (upstream->pool->qlloc(sizeof(cris_buf_t)))cris_buf_t(upstream->pool,2000);
		   conn->in_buffer=conn->out_buffer;
	           conn->in_buffer->reuse();
		   conn->out_buffer=NULL;

		}else{
		  if(!conn->in_write){
                     conn->write->data=upstream;
                     conn->write->handler=&hg_upstream_handler;
                     add_write(conn);
		  }
		}
                again=false;
		rc=HG_AGAIN;
                break;

           case HG_UPSTREAM_PARSE:
    
	        cnt=hg_recv(conn);

		printf("recv cnt=%d\n",cnt);

                if((rc=hg_upstream_parse(upstream))!=HG_OK){
		    again=false;
		}else{
		
		    state=HG_UPSTREAM_END;		
		}
 
                break;

	   case HG_UPSTREAM_END:

                if(upstream->hg_upstream_post_upstream!=NULL)                      
                   hg_harvest_asyn_event(upstream->r,HG_ASYN_UPSTREAM,upstream->hg_upstream_post_upstream,upstream->data,HG_OK);

//	           rc=(upstream->hg_upstream_post_upstream)(upstream->data,HG_OK);     
                
		conn->pool=NULL;
	        hg_return_connection(conn);
		again=false;
                break;

           default:
              return HG_ERROR;
    
        }
    }

    upstream->upstream_state=state;

    if(rc==HG_ERROR){
      hg_harvest_asyn_event(upstream->r,HG_ASYN_UPSTREAM,upstream->hg_upstream_post_upstream,upstream->data,HG_ERROR);

      conn->pool=NULL;

      hg_return_connection(upstream->conn);
    }

    return rc;
}



#endif












