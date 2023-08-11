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
    if(hg_set_address(conn,upstream->host->str,upstream->port)==HG_ERROR)
      return HG_ERROR;
    
    int rc=hg_connect(conn);
    return rc;
}

int hg_upstream_parse(hg_upstream_t *upstream,hg_upstream_info_t *info){

    int rc=HG_ERROR;
 
    if(upstream->parse_state==HG_UPSTREAM_PARSE_HEADER){
    
        if((rc=(upstream->hg_upstream_parse_header)(&(upstream->conn->in_buffer),upstream->data,info))==HG_OK){
	
	    upstream->parse_state=HG_UPSTREAM_PARSE_BODY;

	}   
    }

    if(upstream->parse_state==HG_UPSTREAM_PARSE_BODY){
    
        if((rc=(upstream->hg_upstream_parse_body)(&(upstream->conn->in_buffer),upstream->data,info))==HG_OK){
	
            upstream->parse_state=HG_UPSTREAM_PARSE_HEADER;

	} 
    }

    return rc;
}



int hg_upstream_handler(hg_event_t *ev){

    hg_upstream_t *upstream=(hg_upstream_t *)ev->data;

    if(upstream->conn->write->in_time){
    
 //       printf("取出定时\n");
        hg_del_timeout(upstream->conn->write);
    }

    return hg_upstream_activate((void*)upstream);

}


int hg_upstream_timeout_handler(hg_event_t *ev){

      hg_upstream_t *upstream=(hg_upstream_t *)ev->data;     

      hg_harvest_asyn_event(upstream->r,HG_ASYN_UPSTREAM,upstream->hg_upstream_post_upstream,upstream->data,HG_ERROR);

      upstream->conn->pool=NULL;

      hg_return_connection(upstream->conn); 

      return HG_OK;

}


int hg_upstream_connect_handler(hg_event_t *ev){

      hg_upstream_t *upstream=(hg_upstream_t *)ev->data;     

      if(upstream->connect)
          return HG_OK;

      hg_harvest_asyn_event(upstream->r,HG_ASYN_UPSTREAM,upstream->hg_upstream_post_upstream,upstream->data,HG_ERROR);

      upstream->conn->pool=NULL;

      hg_return_connection(upstream->conn); 

      return HG_OK;
}

//用来直接结束对上游的处理
int hg_upstream_finish(hg_upstream_t *upstream,int rc){

      hg_harvest_asyn_event(upstream->r,HG_ASYN_UPSTREAM,upstream->hg_upstream_post_upstream,upstream->data,rc);

      upstream->conn->pool=NULL;

      hg_return_connection(upstream->conn); 

      return HG_OK;    
}

//删除上游对象中可能存在的定时事件
int hg_upstream_del_timeout(hg_upstream_t *upstream){

    hg_event_t *ev=upstream->conn->write;

    if(ev->in_time)
      hg_del_timeout(ev);

    return HG_OK;
}

int hg_upstream_add_timeout(hg_upstream_t *upstream,unsigned int flag,unsigned long long msec){

              hg_connection_t *conn=upstream->conn;

             if(flag&HG_UPSTREAM_ADD_TIME){

	         hg_add_timeout(conn->write,msec);

                 conn->write->data=(void*)upstream;

                 if(flag&HG_UPSTREAM_TO_AGAIN){
		 
		      conn->write->time_handler=&hg_upstream_handler;

		 }

		 if(flag&HG_UPSTREAM_TO_CLOSE){
		 
		      conn->write->time_handler=&hg_upstream_timeout_handler;
		 }
              }

     return HG_OK;
}

//激活对上游的处理
int hg_upstream_activate(void *data){

    hg_upstream_t *upstream=(hg_upstream_t*)data;
    hg_connection_t *conn=upstream->conn;

    int state=upstream->upstream_state;
 
    bool again=true;
    int  rc=HG_ERROR;
    

    while(again){

        hg_upstream_info_t info;

        switch(state){
    
           case HG_UPSTREAM_INITIAL:
        
                conn=upstream->conn=hg_get_connection();

                rc=hg_upstream_initial(upstream);

                if(rc==HG_AGAIN){          

                   info.flag|=HG_UPSTREAM_ADD_READ|HG_UPSTREAM_ADD_WRITE|HG_UPSTREAM_ADD_TIME|HG_UPSTREAM_NR_HANDLER;
                   info.msec=upstream->connect_timeout;

                   conn->read->handler=&hg_upstream_connect_handler;
		   conn->write->time_handler=&hg_upstream_connect_handler;

		}

                state=HG_UPSTREAM_CREATE_REQUEST;

                break;

           case HG_UPSTREAM_CREATE_REQUEST:

	        upstream->connect=true;

                if((rc=(upstream->hg_upstream_create_request)(&conn->out_buffer,upstream->data,&info))==HG_OK){

		   state=HG_UPSTREAM_SEND_REQUEST;  

		}

	        if(rc!=HG_ERROR){

	             if(hg_send(conn)==HG_ERROR)
		        rc=HG_ERROR;
		}

		break;

           case HG_UPSTREAM_SEND_REQUEST:

                if(conn->out_buffer->available()>0){

		    if(hg_send(conn)==HG_ERROR)
                        rc=HG_ERROR;
                }
		
		if(conn->out_buffer->available()==0){
		
		   state=HG_UPSTREAM_PARSE;

		   info.flag|=HG_UPSTREAM_ADD_READ|HG_UPSTREAM_ADD_TIME|HG_UPSTREAM_TO_CLOSE;
                   info.msec=10000;

		   conn->in_buffer=conn->out_buffer;
	           conn->in_buffer->reuse();
		   conn->out_buffer=NULL;
                
		   rc=HG_AGAIN;

		}else{

		   info.flag|=HG_UPSTREAM_ADD_WRITE|HG_UPSTREAM_ADD_TIME|HG_UPSTREAM_TO_CLOSE;
                   info.msec=2000;
                    
                   rc=HG_AGAIN;
		}

                break;

           case HG_UPSTREAM_PARSE:
                
	        if(hg_recv(conn)!=HG_ERROR){

                   if((rc=hg_upstream_parse(upstream,&info))==HG_OK){
		
		       state=HG_UPSTREAM_END;		

		   } 

                }else{
	
		   rc=HG_ERROR;
		}
              
                break;

	   case HG_UPSTREAM_END:
          
                hg_harvest_asyn_event(upstream->r,HG_ASYN_UPSTREAM,upstream->hg_upstream_post_upstream,upstream->data,HG_OK);   
                
		conn->pool=NULL;
	        hg_return_connection(conn);

		rc=HG_OK;
		again=false;
                break;

           default:
              return HG_ERROR;
    
        }

        if(rc==HG_AGAIN){

              again=false;

	      bool add=false;
            
              unsigned int flag=info.flag;


              if(flag&HG_UPSTREAM_ADD_TIME){

                 del_conn(conn);

	         hg_add_timeout(conn->write,info.msec);

                 conn->write->data=(void*)upstream;

                 if(flag&HG_UPSTREAM_TO_AGAIN){
		 
		      conn->write->time_handler=&hg_upstream_handler;

		 }

		 if(flag&HG_UPSTREAM_TO_CLOSE){
		 
		      conn->write->time_handler=&hg_upstream_timeout_handler;
		 }
              }

              if(flag&HG_UPSTREAM_ADD_READ){
	      
	           add=true;

	           add_read(conn);

		   conn->read->data=(void*)upstream;

                   if(!(flag&HG_UPSTREAM_NR_HANDLER)){
                       conn->read->handler=&hg_upstream_handler;
		   }

	      }

              if(flag&HG_UPSTREAM_ADD_WRITE){
	       
	          if(add)
		     add_conn(conn);
		  else
		     add_write(conn);

		   add=true;
	      
	           conn->write->data=(void*)upstream;

                  if(!(flag&HG_UPSTREAM_NW_HANDLER)){
                      conn->write->handler=&hg_upstream_handler;
	          }	
	      
	      }
      }


       if(rc==HG_ERROR)
          break;

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












