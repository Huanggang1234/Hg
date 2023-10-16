#ifndef HG_UPSTREAM_MODULE_CPP
#define HG_UPSTREAM_MODULE_CPP

#include"../include/hg_http_module.h"
#include"../include/hg_upstream_module.h"
#include"../include/hg_epoll_module.h"
#include"../include/hg_http_core_module.h"
#include"../base/cris_buf.h"
#include"../base/cris_memery_pool.h"
#include"../include/hg_define.h"

static int hg_pipe_initial(hg_pipe_t *pipe);
static hg_pipe_res_t hg_pipe_default_in_filter(cris_buf_t *raw,cris_buf_t *in,void*data);
static hg_pipe_res_t hg_pipe_default_out_filter(cris_buf_t *out,cris_buf_t *pkt,void*data);
//在请求当中添加上游请求
hg_upstream_t*  hg_add_upstream(cris_http_request_t *r,hg_upstream_conf_t *conf){

      hg_upstream_t *up= new (r->pool->qlloc(sizeof(hg_upstream_t)))hg_upstream_t();

      up->next=r->upstream;
      r->upstream=up;

      up->r=r;
      up->pool=r->pool;
      up->conf=conf!=NULL?conf:new (r->pool->qlloc(sizeof(hg_upstream_conf_t)))hg_upstream_conf_t();

      return up;
}


hg_pipe_t *hg_add_pipe(hg_upstream_t *up){

      cris_mpool_t *pool=up->r->pool;
      if(up->pipe==NULL)
          up->pipe=new (pool->qlloc(sizeof(hg_pipe_t)))hg_pipe_t();
      else{
          memset(up->pipe,0,sizeof(hg_pipe_t));
      }
          
      up->pipe->pool=pool;
      up->pipe->upstream=up;
      return up->pipe;
}



//初始化上游连接的套接字及地址，并尝试连接上游
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


//分解请求，分别处理头部和包体，函数不能为空
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


//epoll常规事件处理器，以新连接的写事件为定时器的载体，取出定时，调用hg_upstream_activate
int hg_upstream_handler(hg_event_t *ev){

    hg_upstream_t *upstream=(hg_upstream_t *)ev->data;

    if(upstream->conn->write->in_time){
    
 //       printf("取出定时\n");
        hg_del_timeout(upstream->conn->write);
    }

    return hg_upstream_activate((void*)upstream);
}

//超时处理器，直接调用回调函数，返回上游连接到连接池中
int hg_upstream_timeout_handler(hg_event_t *ev){

      hg_upstream_t *upstream=(hg_upstream_t *)ev->data;     

      hg_harvest_asyn_event(upstream->r,HG_ASYN_UPSTREAM,upstream->hg_upstream_post_upstream,upstream->data,HG_ERROR);

      upstream->conn->pool=NULL;

      hg_return_connection(upstream->conn); 

      return HG_OK;
}

//连接超时处理器，与超时处理器类似，会多一个连接判断，防止假超时
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

//对upstream添加定时器
int hg_upstream_add_timeout(hg_upstream_t *upstream,unsigned int flag,unsigned long long msec){

              hg_connection_t *conn=upstream->conn;

             if(flag&HG_UPSTREAM_ADD_TIME){

	         hg_add_timeout(conn->write,msec);

                 conn->write->data=(void*)upstream;

                 if(flag&HG_UPSTREAM_TO_AGAIN){//超时后进行处理
		 
		      conn->write->time_handler=&hg_upstream_handler;

		 }

		 if(flag&HG_UPSTREAM_TO_CLOSE){//超时后进行关闭
		 
		      conn->write->time_handler=&hg_upstream_timeout_handler;
		 }
              }

     return HG_OK;
}

/*

struct hg_upstream_info_t{
     unsigned int flag=HG_UPSTREAM_UNKOWN;
     unsigned long long  msec;
}; 

#####flag参数内容############

#define HG_UPSTREAM_UNKOWN             0
#define HG_UPSTREAM_ADD_READ           1
#define HG_UPSTREAM_ADD_WRITE          2
#define HG_UPSTREAM_ADD_TIME           4  //添加超时，没有添加处理器，即可以自定义

#define HG_UPSTREAM_NR_HANDLER         8  //表示不要安装读处理器，即自定义处理器
#define HG_UPSTREAM_NW_HANDLER        16  //表示不要安装写处理器，即自定义处理器

//超时关闭和超时继续
#define HG_UPSTREAM_TO_CLOSE          32  //添加超时关闭处理器
#define HG_UPSTREAM_TO_AGAIN          64  //添加超时再次调度处理器


*/
//激活对上游的处理
int hg_upstream_activate(void *data){

    hg_upstream_t *upstream=(hg_upstream_t*)data;
    hg_connection_t *conn=upstream->conn;
    hg_upstream_conf_t *conf=upstream->conf;

    int state=upstream->upstream_state;
 
    bool again=true;
    int  rc=HG_ERROR;
    

    while(again){

        hg_upstream_info_t info;

        switch(state){
    
           case HG_UPSTREAM_INITIAL:
        
                conn=upstream->conn=hg_get_connection();

                rc=hg_upstream_initial(upstream);//函数内会检测conn是否为空

                if(rc==HG_AGAIN){          

                   info.flag|=HG_UPSTREAM_ADD_READ|HG_UPSTREAM_ADD_WRITE|HG_UPSTREAM_ADD_TIME|HG_UPSTREAM_NR_HANDLER;
                   info.msec=conf->connect_timeout;

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

	        if(rc!=HG_ERROR&&conn->out_buffer!=NULL){

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
                   info.msec=conf->read_timeout;

                   if(conf->reused_buffer){
                        conn->in_buffer=conn->out_buffer;
	                conn->in_buffer->reuse();
                   }else{
		        conn->in_buffer=new (upstream->pool->qlloc(sizeof(cris_buf_t)))cris_buf_t(upstream->pool,conf->recv_buffer_size);
		   }

		   conn->out_buffer=NULL;
                
		   rc=HG_AGAIN;

		}else{

		   info.flag|=HG_UPSTREAM_ADD_WRITE|HG_UPSTREAM_ADD_TIME|HG_UPSTREAM_TO_CLOSE;
                   info.msec=conf->write_timeout;
                    
                   rc=HG_AGAIN;
		}

                break;

           case HG_UPSTREAM_PIPE_BACK:

                if(conf->reused_buffer){
                   conn->in_buffer=conn->out_buffer;
         	   conn->in_buffer->reuse();
                }else{
		   conn->in_buffer=new (upstream->pool->qlloc(sizeof(cris_buf_t)))cris_buf_t(upstream->pool,conf->recv_buffer_size);
		}
                conn->out_buffer=NULL;

                state=HG_UPSTREAM_PARSE;

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
       
       		conn->pool=NULL;
	        hg_return_connection(conn);

                hg_harvest_asyn_event(upstream->r,HG_ASYN_UPSTREAM,upstream->hg_upstream_post_upstream,upstream->data,HG_OK);   
            
		return HG_OK;

           default:
              return HG_ERROR;
    
        }

        if(rc==HG_AGAIN){

              again=false;

	      bool add=false;
            
              unsigned int flag=info.flag;

              if(flag&HG_UPSTREAM_USE_PIPE){
	      
	          del_conn(conn);
		  del_conn(upstream->r->conn);
	           //这里必须立即更改upstream的状态机，并退出函数，不然有可能，在hg_pipe_initial一次运行完，都可能无法及时更新状态
		   //甚至在while循环外的修改状态的语句会导致意想不到的结果
	          if(state==HG_UPSTREAM_CREATE_REQUEST){

		      upstream->upstream_state=HG_UPSTREAM_PIPE_BACK;

		      return hg_pipe_initial(upstream->pipe);

	          }else if(state==HG_UPSTREAM_PARSE){

		      upstream->upstream_state=HG_UPSTREAM_END;

		      return hg_pipe_initial(upstream->pipe);

		  } 
		  rc=HG_ERROR; 
	      }

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




/*******************************************************************/

static int hg_pipe_write_handler(hg_event_t *ev);
static int hg_pipe_read_handler(hg_event_t *ev);
static int hg_pipe_read_without_file_handler(hg_event_t *ev);
static int hg_pipe_timeout_handler(hg_event_t *ev);

static int hg_pipe_error(hg_pipe_t *pipe){

       pipe->error=true;
       del_conn(pipe->in_driver);
       del_conn(pipe->out_driver);
       if(pipe->in_driver->read->in_time)
          hg_del_timeout(pipe->in_driver->read);
       if(pipe->out_driver->write->in_time)
          hg_del_timeout(pipe->out_driver->write);

       pipe->upstream->upstream_state=HG_UPSTREAM_ERROR;
       pipe->in_driver->read->data=pipe->origin_in_data;
       pipe->in_driver->read->handler=NULL;
       pipe->in_driver->read->time_handler=NULL;
       pipe->out_driver->write->data=pipe->origin_out_data;
       pipe->out_driver->write->handler=NULL;
       pipe->out_driver->write->time_handler=NULL;
 
       
       pipe->in_driver->in_buffer=pipe->origin_in_buf;
       pipe->out_driver->out_buffer=pipe->origin_out_buf;

       if(pipe->conf->use_file_buffer)
           close(pipe->file_fd);

       return hg_upstream_activate((void*)pipe->upstream);
}


static int hg_pipe_success(hg_pipe_t *pipe){

       pipe->in_driver->read->data=pipe->origin_in_data;
       pipe->in_driver->read->handler=NULL;
       pipe->in_driver->read->time_handler=NULL;
       pipe->out_driver->write->data=pipe->origin_out_data;
       pipe->out_driver->write->handler=NULL;
       pipe->out_driver->write->time_handler=NULL;

     
       pipe->in_driver->in_buffer=pipe->origin_in_buf;
       pipe->out_driver->out_buffer=pipe->origin_out_buf;

        if(pipe->conf->use_file_buffer)
            close(pipe->file_fd);

       return hg_upstream_activate((void*)pipe->upstream);      
}


static int hg_pipe_filter_in(hg_pipe_t *pipe){

    hg_pipe_res_t res;

    res=pipe->in_filter(pipe->raw,pipe->in,pipe->in_filter_ctx);

    if(res.rc==HG_ERROR)
        return HG_ERROR;
    
    if(res.rc==HG_OK)
        pipe->cmpt_in=true;

    pipe->res_upstream=res.res;
    pipe->raw->take(res.cnt);

    if(pipe->raw->blank()>0.6)
         pipe->raw->reuse();

    if(pipe->conf->use_file_buffer){

        lseek(pipe->file_fd,0,SEEK_END);          
        if((res.cnt=write(pipe->file_fd,pipe->in->cur,pipe->in->available()))<0)
	    return HG_ERROR;

        printf("to file cnt=%d\n",res.cnt);

        pipe->file_size+=res.cnt;
	pipe->file_res_size+=res.cnt;
        pipe->in->take(res.cnt);
    }

    return HG_OK;
}



static int hg_pipe_load_out(hg_pipe_t *pipe){

    int cnt;

    lseek(pipe->file_fd,pipe->file_cur_pos,SEEK_SET);
    if((cnt=read(pipe->file_fd,pipe->out->last,pipe->out->surplus()))<0)
	 return HG_ERROR;

    printf("load %d\n",cnt);

    pipe->file_res_size-=cnt;
    pipe->file_cur_pos+=cnt;
    pipe->out->extend(cnt);

    return HG_OK;
}



static int hg_pipe_filter_out(hg_pipe_t *pipe){

    hg_pipe_res_t res;
    //在使用文件缓冲的情况下,在进入到这一步时in(也即out)缓冲一定为空
    //参考hg_pipe_do_filt函数流程
    res=pipe->out_filter(pipe->out,pipe->pkt,pipe->out_filter_ctx);

    if(res.rc==HG_ERROR)
         return HG_ERROR;
    
    if(res.rc==HG_OK)
         pipe->cmpt_out=true;

    pipe->out->take(res.cnt);
    pipe->res_downstream=res.res;//剩余的还未过滤的数据量

    return HG_OK;
}



static int hg_pipe_send(hg_pipe_t *pipe){

    int rc;
    int cnt;
    hg_connection_t *conn=pipe->out_driver;

    if(pipe->send_time<=hg_epoll_ctx.cur_msec){
    
       if((cnt=hg_send(conn))==HG_ERROR)
            return HG_ERROR;

       conn->out_buffer->check();//检查并归位cur,last

       rc=cnt/pipe->conf->limit*1000;//转换成毫秒
       
       pipe->send_time=hg_epoll_ctx.cur_msec+rc;

    }else{
    
       rc=hg_epoll_ctx.cur_msec-pipe->send_time;

    }

    if(rc>0){
    
       pipe->flag|=HG_PIPE_WAIT_WRITE;
       pipe->wait_time=rc;//更新等待时间
       printf("wait t=%d\n",rc);
    
    }
    
    return cnt;//返回发送的字节数
}



static int hg_pipe_do_filt(hg_pipe_t *pipe){


      cris_buf_t *raw=pipe->raw;
      cris_buf_t *in=pipe->in;
      cris_buf_t *out=pipe->out;
      cris_buf_t *pkt=pipe->pkt;
      int rc=0;
      int cnt=0;
      
      if(raw->available()>0&&(!pipe->canntwrite)){
sz1:
          if(hg_pipe_filter_in(pipe)==HG_ERROR)//filter_in根据情况会挂cmpt_in标志
	     return HG_ERROR;
      }
      
      pipe->canntwrite=false;

      if(pipe->conf->use_file_buffer&&pipe->file_res_size>0){
sz2:
          if(hg_pipe_load_out(pipe)==HG_ERROR)//加载out余量的数据到out
	     return HG_ERROR;

      }
      //经过hg_pipe_filter_in,如果存在没有被过滤的数据,只会出现以下两种情况*之一*
      //如果不使用文件,则out->available()一定大于0,使用文件则file_res_size一定大于0
      if(out->available()>0){
sz3:
          
          if(hg_pipe_filter_out(pipe)==HG_ERROR)
	     return HG_ERROR;
      
      }

      cnt=pkt->available();

      if(cnt>0){
      
          if((rc=hg_pipe_send(pipe))==HG_ERROR)
	     return HG_ERROR;

          printf("write %d should %d res %d\n",rc,cnt,pipe->pkt->available());

          if(pipe->flag&HG_PIPE_WAIT_WRITE)
	     return HG_OK;

          if(rc<cnt){
	     pipe->flag|=HG_PIPE_CANNT_WRITE;
	     return HG_OK;
	  }

          //这里即指代out缓冲，也指代in缓冲
          if(out->available()>0)
	      goto sz3;	      
          
	  if(pipe->file_res_size>0)
	      goto sz2;

          if(raw->available()>0)
	      goto sz1;

      }
    
      if(!pipe->cmpt_out){
        pipe->flag|=HG_PIPE_NO_RAW;
      }

      return HG_OK;
}



//该函数由hg_pipe_write_handler函数注册，只触发一次
static int hg_pipe_read_without_file_handler(hg_event_t *ev){

      int rc;

      hg_pipe_t *pipe=(hg_pipe_t*)ev->data;

      if(pipe->error)
         return HG_ERROR;

      hg_del_timeout(pipe->in_driver->read);

      if((rc=hg_recv_fixed(pipe->in_driver))==HG_ERROR)
          return hg_pipe_error(pipe);

      printf("recv cnt=%d buf->available()=%d\n",rc,pipe->in_driver->in_buffer->available());

      del_conn(pipe->in_driver);    

      printf("写驱动\n");

      return hg_pipe_write_handler(ev);
}

//该函数在initial阶段注册，直到获取所有的上游资源
static int hg_pipe_read_handler(hg_event_t *ev){

      int rc;

      hg_pipe_t *pipe=(hg_pipe_t *)ev->data;

      if(pipe->error)//不用管定时器，以及读写事件,error函数已经处理了
         return HG_ERROR;

      hg_del_timeout(pipe->in_driver->read);

      if((rc=hg_recv_fixed(pipe->in_driver))==HG_ERROR)
          return hg_pipe_error(pipe);

      pipe->flag=0;

      if((rc=hg_pipe_filter_in(pipe))==HG_ERROR)
          return hg_pipe_error(pipe);

      printf("fileter in %d\n",rc);

      if(pipe->cmpt_in)
          del_conn(pipe->in_driver);
      else
          hg_add_timeout(pipe->in_driver->read,pipe->conf->upstream_timeout);

      if(pipe->need){
          printf("驱动写\n");
          pipe->need=false;
          hg_pipe_write_handler(ev);
      }

      return HG_OK;
}



static int hg_pipe_write_handler(hg_event_t *ev){

      int rc;
      hg_pipe_t *pipe=(hg_pipe_t*)ev->data;

      if(pipe->error)
            return HG_ERROR;

      if(pipe->out_driver->write->in_time)
            hg_del_timeout(pipe->out_driver->write);

      pipe->flag=0;

      if((rc=hg_pipe_do_filt(pipe))==HG_ERROR)
         return hg_pipe_error(pipe);
      
      if(pipe->flag&HG_PIPE_WAIT_WRITE){
          
          printf("时间事件\n");

	  del_conn(pipe->out_driver);
	  pipe->out_driver->write->time_handler=&hg_pipe_write_handler;
          hg_add_timeout(pipe->out_driver->write,pipe->wait_time);     
          return HG_OK;

      }
      if(pipe->flag&HG_PIPE_CANNT_WRITE){

          printf("不能写\n");
	  pipe->canntwrite=true;
          add_write(pipe->out_driver);
	  pipe->out_driver->write->time_handler=&hg_pipe_timeout_handler;
	  hg_add_timeout(pipe->out_driver->write,pipe->conf->downstream_timeout);
          return HG_OK;
      }

      if(pipe->flag&HG_PIPE_NO_RAW){
     
          printf("无源\n");
          del_conn(pipe->out_driver);
          if(pipe->conf->use_file_buffer){     
              pipe->need=true;//告诉读处理器，需要驱动写事件
          }else{
	      add_read(pipe->in_driver);
	      hg_add_timeout(pipe->in_driver->read,pipe->conf->upstream_timeout);
	  }
	  return HG_OK;
      }
      
      if(pipe->cmpt_out){
          cris_str_print(&pipe->upstream->r->url);
          printf(" cmpt_out\n");     
          del_conn(pipe->out_driver);  
          return hg_pipe_success(pipe);
      }

      return hg_pipe_error(pipe);
}



static int hg_pipe_timeout_handler(hg_event_t *ev){
 
       return hg_pipe_error((hg_pipe_t*)ev->data);

}


static int hg_pipe_initial(hg_pipe_t *pipe){

    cris_mpool_t *pool=pipe->pool;

    pipe->origin_in_buf=pipe->in_driver->in_buffer;
    pipe->origin_out_buf=pipe->out_driver->out_buffer;


    //第三方没有设置pipe配置，则使用默认配置
    if(pipe->conf==NULL){
        pipe->conf=&pipe->upstream->conf->pipe_conf;
    }

    hg_pipe_conf_t *conf=pipe->conf;


    if(pipe->raw==NULL){
        pipe->raw=new (pool->qlloc(sizeof(cris_buf_t)))cris_buf_t(pool,conf->buffer_size);
    }
    
    pipe->in_driver->in_buffer=pipe->raw;

    if(pipe->in_filter==NULL){
        pipe->in=pipe->raw;
        pipe->in_filter_ctx=(void*)pipe;
	pipe->in_filter=&hg_pipe_default_in_filter;
    }else{
        pipe->in=new (pool->qlloc(sizeof(cris_buf_t)))cris_buf_t(pool,conf->buffer_size);
    }

    if(conf->use_file_buffer){
        pipe->out=new (pool->qlloc(sizeof(cris_buf_t)))cris_buf_t(pool,conf->buffer_size);
    }else{
        pipe->out=pipe->in;
    }

    if(pipe->out_filter==NULL){
        pipe->pkt=pipe->out;
        pipe->out_filter_ctx=(void*)pipe;
	pipe->out_filter=&hg_pipe_default_out_filter;
    }else{
        pipe->pkt=new (pool->qlloc(sizeof(cris_buf_t)))cris_buf_t(pool,conf->buffer_size);
    }

    if(conf->use_file_buffer){   
        pipe->file_fd=open("./",O_TMPFILE|O_RDWR,S_IRUSR|S_IWUSR);
    }

    pipe->origin_in_data=pipe->in_driver->read->data;
    pipe->origin_out_data=pipe->out_driver->write->data;

    pipe->in_driver->read->data=(void*)pipe;
    pipe->out_driver->write->data=(void*)pipe;

//这条语句曾经覆盖原有的buff,篡改了内存
    pipe->out_driver->out_buffer=pipe->pkt;
    
    if(conf->use_file_buffer)
         pipe->in_driver->read->handler=&hg_pipe_read_handler;
    else
         pipe->in_driver->read->handler=&hg_pipe_read_without_file_handler;


    pipe->out_driver->write->handler=&hg_pipe_write_handler;
    pipe->in_driver->read->time_handler=&hg_pipe_timeout_handler;
    pipe->out_driver->write->time_handler=NULL;

    if(conf->use_file_buffer){
         if(pipe->res_upstream>pipe->raw->available()){	
	      printf("开始驱动读\n");
	      add_read(pipe->in_driver);
	      hg_add_timeout(pipe->in_driver->read,conf->upstream_timeout);
	 }
    }

    return hg_pipe_write_handler(pipe->out_driver->write);
}


static hg_pipe_res_t hg_pipe_default_in_filter(cris_buf_t *raw,cris_buf_t *in,void*data){

    hg_pipe_t *pipe=(hg_pipe_t*)data;
    
    int res=pipe->res_upstream;
    int num=raw->available();
    
    printf("default filter cnt=%d\n",num>res?res:num);

    return  {0,num>=res?HG_OK:HG_AGAIN,res-num>=0?res-num:0};

}

static hg_pipe_res_t hg_pipe_default_out_filter(cris_buf_t *out,cris_buf_t *pkt,void*data){
 
    hg_pipe_t *pipe=(hg_pipe_t*)data;
    int rc=HG_AGAIN;
    if(pipe->cmpt_in&&pipe->file_res_size==0)
        rc=HG_OK;
    return {0,rc,0};
}


#endif
























