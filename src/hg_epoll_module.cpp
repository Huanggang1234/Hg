#ifndef HG_EPOLL_MODULE_CPP
#define HG_EPOLL_MODULE_CPP
#include"../include/hg_epoll_module.h"
#include"../include/hg.h"
#include"../include/hg_define.h"
#include"../include/hg_conf_parse.h"
#include"../include/hg_control_module.h"
#include"../base/include/cris_rbtree.h"
#include<cstring>
#include<cstdlib>
#include<vector>
#include<unistd.h>
#include<sys/uio.h>
#include<stdint.h>
#include<errno.h>
#include<sys/sendfile.h>

int hg_epoll_process_events(unsigned int flag);
inline hg_connection_t* hg_get_connection(hg_cycle_t *cycle);
void* hg_epoll_create_conf(hg_cycle_t *cycle);
int hg_epoll_init_conf(hg_module_t *module,hg_cycle_t *cycle);
int hg_epoll_init_module(hg_module_t *module,hg_cycle_t *cycle);
int hg_epoll_init_process(hg_module_t *module,hg_cycle_t *cycle);


int hg_epoll_set_event_num(hg_module_t *module,hg_cycle_t *cycle,cris_conf_t *conf);
int hg_epoll_set_wait_time(hg_module_t *module,hg_cycle_t *cycle,cris_conf_t *conf);
int hg_epoll_set_connections_size(hg_module_t *module,hg_cycle_t *cycle,cris_conf_t *conf);
int hg_epoll_set_accept_lock(hg_module_t *module,hg_cycle_t *cycle,cris_conf_t *conf);

int hg_epoll_timeout_handler(hg_event_t *ev);
int hg_epoll_accept_handler(hg_event_t *ev);//默认accept函数
int hg_epoll_get_accept_mutex();
int hg_epoll_enable_listens();
int hg_epoll_disable_listens();

int hg_time_cmp(void*,void*);

hg_cycle_t *epoll_cycle=NULL;//保留指针为了给其他模块提供服务

char extra_buf[65536];//接收信息时使用

hg_event_t  *wait_mutex=NULL;//用于抢锁计时的时间事件

std::vector<hg_command_t> epoll_commands={
  {
    std::string("event_num"),
    HG_CMD_MAIN,
    &hg_epoll_set_event_num
  },
  {
    std::string("wait_time"),
    HG_CMD_MAIN,
    &hg_epoll_set_wait_time
  },
  {
    std::string("connections_size"),
    HG_CMD_MAIN,
    &hg_epoll_set_connections_size
  },
  {
    std::string("accept_lock"),
    HG_CMD_MAIN,
    &hg_epoll_set_accept_lock
  }
};


hg_epoll_ctx_t  hg_epoll_ctx;

hg_epoll_conf_t hg_epoll_conf;

hg_module_t  hg_epoll_module={
   HG_EPOLL_MODULE,
   0,
   0,
   &hg_epoll_ctx,
   &epoll_commands,
   &hg_epoll_create_conf,
   &hg_epoll_init_conf,
   &hg_epoll_init_module,
   &hg_epoll_init_process   
};


int hg_connect(hg_connection_t *conn){
      
     int rc=connect(conn->fd,(sockaddr*)&conn->sockaddr,sizeof(struct sockaddr_in));
    
     if(rc==0){
 
        return HG_OK;
     
     }else{
 
        if(errno!=EINPROGRESS)
	  return HG_ERROR;

        return HG_AGAIN;
     }

     return HG_OK;
}


int hg_set_sock(hg_connection_t *conn){

    conn->fd=socket(PF_INET,SOCK_STREAM,0);
    if(conn->fd<=0)
      return HG_ERROR;

    int flag=fcntl(conn->fd,F_GETFL);
    fcntl(conn->fd,F_SETFL,flag|O_NONBLOCK);

    return HG_OK;
}

int hg_set_address(hg_connection_t *conn,const char *ip,unsigned short port){
    
    struct sockaddr_in & addr=conn->sockaddr;
    memset(&addr,0,sizeof(addr));
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=inet_addr(ip);
    addr.sin_port=htons(port);
    return HG_OK;
}



int hg_epoll_timeout_handler(hg_event_t *ev){
 
    hg_connection_t *conn=(hg_connection_t*)ev->data; 
    hg_return_connection(conn);

    return HG_OK;
}

int hg_epoll_accept_handler(hg_event_t *ev){

    hg_listen_t *ls=(hg_listen_t*)ev->data;  

    int clnt_fd=0;
    int serv_fd=ls->fd;

    struct sockaddr_in sockaddr;
    socklen_t     socklen=0;

    while((clnt_fd=accept(serv_fd,(struct sockaddr*)&sockaddr,&socklen))>0){


          printf("pid %d\n",getpid());

          int flag=fcntl(clnt_fd,F_GETFL);
          flag|=O_NONBLOCK;
          fcntl(clnt_fd,F_SETFL,flag);
 
          hg_connection_t *conn=hg_get_connection();       

          if(conn==NULL)
            close(clnt_fd);

          conn->fd=clnt_fd;
          conn->on=true;

          conn->read->handler=ls->read_handler; 
          conn->read->data=conn;
          conn->read->time_handler=&hg_epoll_timeout_handler;

          conn->sockaddr=sockaddr;
          conn->socklen=socklen;

/*
          printf("\nip:%x   ntohl:%x ",sockaddr.sin_addr.s_addr,ntohl(sockaddr.sin_addr.s_addr));
          printf("%s   %s\n",inet_ntoa(sockaddr.sin_addr),inet_ntoa({ntohl(sockaddr.sin_addr.s_addr)}));
*/
          add_read(conn);
          //这个时间不能太短，特别对比与epoll的等待时间
          hg_add_timeout(conn->read,3000);//对连接的读事件添加定时器
    }
    return HG_OK;
}

int hg_epoll_set_accept_lock(hg_module_t *module,hg_cycle_t *cycle,cris_conf_t *conf){

    if(conf->avgs.size()==0||conf->avgs.size()>1)
       return HG_ERROR;

    hg_epoll_conf_t *epoll_conf=(hg_epoll_conf_t*)hg_get_module_conf(module,cycle);

    cris_str_t arg=conf->avgs.front();

    if(arg==std::string("on"))
       epoll_conf->accept_lock=true;
 
    return HG_OK;
}

int hg_epoll_set_event_num(hg_module_t *module,hg_cycle_t *cycle,cris_conf_t *conf){

    if(conf->avgs.size()>1)
       return HG_ERROR;
 
    hg_epoll_conf_t *epoll_conf=(hg_epoll_conf_t*)hg_get_module_conf(module,cycle);

    int size=atoi(conf->avgs.front().str);

    if(size<=20||size>=10000){
        printf("请设置合理的事件接收大小\n");
        return HG_ERROR;
    }

    epoll_conf->event_num=size;

    return HG_OK;


}


int hg_epoll_set_wait_time(hg_module_t *module,hg_cycle_t *cycle,cris_conf_t *conf){

    if(conf->avgs.size()>1)
       return HG_ERROR;
 
    hg_epoll_conf_t *epoll_conf=(hg_epoll_conf_t*)hg_get_module_conf(module,cycle);

    int t=atoi(conf->avgs.front().str);

    if(t>=600000){
        printf("请设置合理的等待时间\n");
        return HG_ERROR;
    }

    epoll_conf->wait_time=t;

    return HG_OK;

}

int hg_epoll_set_connections_size(hg_module_t *module,hg_cycle_t *cycle,cris_conf_t *conf){

    if(conf->avgs.size()>1)
       return HG_ERROR;
 
    hg_epoll_conf_t *epoll_conf=(hg_epoll_conf_t*)hg_get_module_conf(module,cycle);

    int size=atoi(conf->avgs.front().str);

    if(size<=20||size>=5000000){
        printf("请设置合理的连接池大小\n");
        return HG_ERROR;
    }

    epoll_conf->connections_size=size;

    return HG_OK;
}

int hg_epoll_enable_listens(){

     hg_listen_t *ls=epoll_cycle->listens;

     while(ls!=NULL){
         
         if(add_read(ls->conn)==HG_ERROR)
               goto do_error;
         ls=ls->next;
     }

     return HG_OK;

do_error:

     hg_listen_t *x=epoll_cycle->listens;

     while(x!=ls){
        del_conn(x->conn);
        x=x->next;
     }
     return HG_ERROR;
}


int hg_epoll_disable_listens(){


     hg_listen_t *ls=epoll_cycle->listens;

     while(ls!=NULL){
         
         del_conn(ls->conn);
         ls=ls->next;
     }

     return HG_OK;

}

int hg_epoll_get_accept_mutex(){

    if(pthread_mutex_trylock(accept_mutex)!=EBUSY){//获得锁

//         printf("进程%d 抢锁成功\n",(int)getpid());
 
          if(hg_epoll_ctx.held_accept_mutex)//之前已经持锁
                 return HG_OK;

          if(hg_epoll_enable_listens()==HG_ERROR){
              pthread_mutex_unlock(accept_mutex);
              return HG_OK;
          }
          
          hg_epoll_ctx.held_accept_mutex=true;
          return HG_OK;
    }

//    printf("进程%d 抢锁失败\n",(int)getpid());

    //没有抢到锁
    if(hg_epoll_ctx.held_accept_mutex){//但之前持过锁

       hg_epoll_disable_listens();       

       hg_epoll_ctx.held_accept_mutex=false;
    }

    return HG_AGAIN;

}

//该函数直接使用本文件中epoll上下文的静态变量，不需要任何参数
int hg_epoll_process_events(unsigned int flag){

    int epfd=hg_epoll_ctx.epfd;

    int max_event_num=hg_epoll_ctx.max_event_num;

    int wait_time=hg_epoll_ctx.wait_time;

    epoll_event *events=hg_epoll_ctx.event;       
    
    int num=0;
    uintptr_t instence=0;

    hg_event_t *to_do=NULL;


   if(hg_epoll_conf.accept_lock&&(!wait_mutex->in_time)&&hg_epoll_ctx.cur_connections_num<=((hg_epoll_ctx.max_connections_num*7)>>3)){
        if(hg_epoll_get_accept_mutex()==HG_AGAIN)
            hg_add_timeout(wait_mutex,400);
   }

    gettimeofday(&hg_epoll_ctx.tv,0);
    hg_epoll_ctx.cur_msec=hg_epoll_ctx.tv.tv_sec*1000+hg_epoll_ctx.tv.tv_usec/1000;

    unsigned long long msec=hg_epoll_ctx.cur_msec;
   
    cris_rbtree_t *timetree=hg_epoll_ctx.timetree;

    while(timetree->first!=NULL&&((hg_event_t*)timetree->first)->msec<=msec){ 
    
           to_do=(hg_event_t*)timetree->first;

           timetree->erase((void*)to_do);

           to_do->in_time=false;
 
           to_do->timeout=true;

           if(to_do->time_handler!=NULL)
               to_do->time_handler(to_do);

    } 

   num=epoll_wait(epfd,events,max_event_num,wait_time);

   hg_epoll_ctx.cur_msec+=wait_time;

   if(!hg_epoll_ctx.held_accept_mutex){
  
       for(int i=0;i<num;i++){

          hg_connection_t *c=(hg_connection_t*)events[i].data.ptr;        
        
          instence=((uintptr_t)c)&1;

          c=(hg_connection_t*)(((uintptr_t)c)&((uintptr_t)~1));

          int revents=events[i].events;

          if(revents&EPOLLIN){           
               to_do=c->read;
              if(to_do->handler&&instence==to_do->instence)
               to_do->handler(to_do); 
          } 

          if(revents&EPOLLOUT){
     
                to_do=c->write;
              if(to_do->handler&&instence==to_do->instence)
                to_do->handler(to_do);            
          }

       }
    }else{

         hg_event_t head;
         hg_event_t *cur=NULL;        
         to_do=&head;

         for(int i=0;i<num;i++){

          hg_connection_t *c=(hg_connection_t*)events[i].data.ptr;        
        
          instence=((uintptr_t)c)&1;

          c=(hg_connection_t*)(((uintptr_t)c)&((uintptr_t)~1));

          int revents=events[i].events;

          if(revents&EPOLLIN){           
               cur=c->read;
              if(cur->handler&&instence==cur->instence){
                  to_do->next=cur;
                  to_do=cur;
              }
          } 

          if(revents&EPOLLOUT){
                cur=c->write;
              if(cur->handler&&instence==cur->instence){
                  to_do->next=cur;
                  to_do=cur;
              }        
          }

         }

         pthread_mutex_unlock(accept_mutex);

         to_do->next=NULL;
         to_do=head.next;//指向第一个事件;
          
         while(to_do!=NULL){
            to_do->handler(to_do);
            to_do=to_do->next;
         }

    }

 
    return HG_OK;

}




inline hg_connection_t* hg_get_connection(){
 
      hg_connection_t *conn=epoll_cycle->free_connections;
      if(conn){
        epoll_cycle->free_connections=conn->next;
             
        hg_epoll_ctx.cur_connections_num++;//统计连接使用情况
      }
      return conn; 
}


void* hg_epoll_create_conf(hg_cycle_t *cycle){
 
    hg_epoll_conf.event_num=100;
    hg_epoll_conf.wait_time=100;
    hg_epoll_conf.connections_size=300;

    return (void*)&hg_epoll_conf;
}


int hg_epoll_init_conf(hg_module_t *module,hg_cycle_t *cycle){

     return HG_OK;
}


int hg_epoll_init_module(hg_module_t *module,hg_cycle_t *cycle){

    int index=module->index; 
    epoll_cycle=cycle;//保留cycle的指针;
  
    cris_mpool_t *pool =cycle->pool;

    hg_epoll_conf_t *epconf=(hg_epoll_conf_t*)(cycle->conf_ctx[index]);
    
    hg_epoll_ctx_t *ctx=(hg_epoll_ctx_t*)(module->ctx);

    int size=0;

    void *p;

    /*设置连接池*/
         
    size=epconf->connections_size;
 
    p=(hg_connection_t*)pool->alloc(size*sizeof(hg_connection_t)),cycle->connections=new (p)hg_connection_t[size];

    p=(hg_event_t*)pool->alloc(size*sizeof(hg_event_t)),cycle->reads=new (p)hg_event_t[size];

    p=(hg_event_t*)pool->alloc(size*sizeof(hg_event_t)),cycle->writes=new (p)hg_event_t[size];

    hg_connection_t *conn=NULL;
    
    for(int i=0;i<size;i++){
        conn=&cycle->connections[i];
        conn->dex=i; 
        conn->read=&cycle->reads[i];//设置读事件
        conn->write=&cycle->writes[i];//设置写事件
        
        conn->next=i+1<size?&cycle->connections[i+1]:NULL;

    }
     
    cycle->free_connections=&cycle->connections[0];//将空闲连接链表指向第一个连接
 

    /*分配事件数组*/
    size=epconf->event_num;

    ctx->max_event_num=size; 
    ctx->event=(epoll_event*)pool->alloc(size*sizeof(epoll_event));   

    /*设置等待时间*/
      
    ctx->wait_time=epconf->wait_time;

    ctx->max_connections_num=size;
    ctx->cur_connections_num=0;

    ctx->held_accept_mutex=false;

    /*打开epoll描述符*/
    ctx->epfd=epoll_create(epconf->connections_size);

    p=pool->alloc(sizeof(cris_rbtree_t)),ctx->timetree=new (p)cris_rbtree_t(&hg_time_cmp);
    ctx->id=0;
    ctx->cur_msec=0;    

    return HG_OK;

}

int hg_epoll_init_process(hg_module_t *module,hg_cycle_t *cycle){

    hg_epoll_ctx_t *ctx=(hg_epoll_ctx_t*)module->ctx;
    hg_listen_t  *ls=cycle->listens;
    int epfd=ctx->epfd;

    while(ls!=NULL){

        /*初始化该监听对象*/
        int fd=socket(PF_INET,SOCK_STREAM,0);

        int flag=fcntl(fd,F_GETFL);
        flag|=O_NONBLOCK;
        fcntl(fd,F_SETFL,flag);
       
        ls->fd=fd;

        struct sockaddr_in *addr=&ls->sockaddr;
   
        memset(addr,0,sizeof(addr));
        addr->sin_family=AF_INET;
        addr->sin_addr.s_addr=htonl(INADDR_ANY);
        addr->sin_port=htons(ls->port);

        bind(fd,(sockaddr*)addr,sizeof(*addr));

        listen(fd,ls->backlog); 

        /*把该对象加入epoll监听*/

        hg_connection_t *conn=hg_get_connection();

        if(conn==NULL)
          return HG_ERROR;

        ls->conn=conn;

        conn->data=ls;

        if(ls->connect_handler!=NULL)
          conn->read->handler=ls->connect_handler;
        else
          conn->read->handler=&hg_epoll_accept_handler;

        conn->read->data=ls;
        conn->fd=ls->fd;
     
        if(!hg_epoll_conf.accept_lock)
            add_read(conn);

        ls=ls->next;
    }

    //该连接用于抢锁计时
   if(hg_epoll_conf.accept_lock){
    
         hg_connection_t *conn=hg_get_connection();

         if(conn==NULL) 
             return HG_ERROR;
          
         conn->write->data=(void*)conn;
         wait_mutex=conn->write; 
    }

    return HG_OK;
}

void hg_return_connection(hg_connection_t *conn){
      
      if(conn->pool!=NULL){
         delete conn->pool;//销毁内存池 
         conn->pool=NULL;
      }
      if(conn->in_epoll)
         del_conn(conn);//将连接从epoll中删除
      if(conn->read->in_time)
         hg_del_timeout(conn->read);
      if(conn->write->in_time)
         hg_del_timeout(conn->write);

      if(conn->on)
         close(conn->fd);//关闭连接
      conn->on=false;
      conn->fd=0;
      conn->data=NULL;
      conn->read->data=NULL;
      conn->read->handler=NULL;
      conn->read->time_handler=NULL;
      conn->read->timeout=false; 
      conn->write->data=NULL;
      conn->write->handler=NULL;
      conn->write->time_handler=NULL;  
      conn->write->timeout=false;

      int s=conn->instence;
      conn->instence=s^1;
      conn->read->instence=s^1;
      conn->write->instence=s^1;

      conn->in_buffer=NULL;
      conn->out_buffer=NULL;
    
      conn->recv=0;      
      conn->send=0;    

      conn->in_read=false;
      conn->in_write=false;
      conn->in_epoll=false;

      conn->next=epoll_cycle->free_connections;
      epoll_cycle->free_connections=conn;
      hg_epoll_ctx.cur_connections_num--;

}

int add_conn(hg_connection_t *conn){    
    epoll_event  event;

    event.data.ptr=(void*)(((uintptr_t)conn)|(conn->instence));
    event.events=EPOLLIN|EPOLLOUT;

    if(conn->in_epoll){
       if(epoll_ctl(hg_epoll_ctx.epfd,EPOLL_CTL_MOD,conn->fd,&event)<0)
            return HG_ERROR;
    }else{
       if(epoll_ctl(hg_epoll_ctx.epfd,EPOLL_CTL_ADD,conn->fd,&event)<0)
            return HG_ERROR;
    }

    conn->in_read=true;
    conn->in_write=true;
    conn->in_epoll=true;

    return HG_OK;
}


int del_conn(hg_connection_t *conn){
   if(conn->in_epoll){
      if(epoll_ctl(hg_epoll_ctx.epfd,EPOLL_CTL_DEL,conn->fd,NULL)<0)
           return HG_ERROR;
   }
 
   conn->in_epoll=false;
   conn->in_read=false;
   conn->in_write=false;
   
   return  HG_OK;
}

int add_read(hg_connection_t *conn){

    if(conn->in_read)
      return HG_OK;

    epoll_event event;
    int op=EPOLL_CTL_ADD;

    event.events=EPOLLIN;
 
    if(conn->in_write||conn->in_read){
        op=EPOLL_CTL_MOD;
    }
    
    event.data.ptr=(void*)(((uintptr_t)conn)|(conn->instence));
 
    if(epoll_ctl(hg_epoll_ctx.epfd,op,conn->fd,&event)<0)
        return HG_ERROR;
 
    conn->in_read=true;   
    conn->in_epoll=true;  
    conn->in_write=false;
    return HG_OK;
}


int add_write(hg_connection_t *conn){
 
    if(conn->in_write)
       return HG_OK;

    epoll_event event;
    int op=EPOLL_CTL_ADD;

    event.events=EPOLLOUT;
 
    if(conn->in_read||conn->in_write)
        op=EPOLL_CTL_MOD;
        
    event.data.ptr=(void*)(((uintptr_t)conn)|(conn->instence));

    if(epoll_ctl(hg_epoll_ctx.epfd,op,conn->fd,&event)<0)
         return HG_ERROR;
    conn->in_write=true;   
    conn->in_epoll=true;  
    conn->in_read=false;
    return HG_OK;

}


int hg_recv(hg_connection_t *conn){

    cris_buf_t *buf=conn->in_buffer;
    int res=buf->res;
    int cnt=0;
   
    struct iovec iov[2];
    iov[0].iov_base=buf->last;
    iov[0].iov_len=res;
    iov[1].iov_base=extra_buf;
    iov[1].iov_len=65536;

    if((cnt=readv(conn->fd,iov,2))<=0){
        if(errno==EWOULDBLOCK)
	   return 0;
        return HG_ERROR;
    }

    if(cnt<=res){
       buf->last+=cnt;
       buf->res-=cnt;
       buf->used+=cnt;
    }else{
       buf->last=buf->end;
       buf->res=0;
       buf->used=buf->capacity;
       buf->append(extra_buf,cnt-res);  
    }
       
    return cnt;
}

int hg_recv_discard(hg_connection_t *conn){

    int  cnt=read(conn->fd,extra_buf,65536);

    return cnt;

}



int hg_send(hg_connection_t *conn){

   cris_buf_t *buf=conn->out_buffer;

   int num=buf->available();
 
   int cnt=0;

   if((cnt=write(conn->fd,buf->cur,num))<=0&&num!=0){

      if(errno==EWOULDBLOCK)
         return 0;
      return HG_ERROR; 
   }


   buf->cur+=cnt;   
 
   conn->send+=cnt;

   return cnt;
}

int hg_send_file(int sock,int fd,off_t *off,unsigned long length ){

     int rc=sendfile(sock,fd,off,length);

     if(rc<0){
     
       if(errno==EWOULDBLOCK)
         return 0;
       return HG_ERROR;    

     }
     
     return rc;
}


inline int hg_close_connection(hg_connection_t *conn){
      if(conn->on){
         close(conn->fd);
         conn->on=false;
      }
      return HG_OK;      
}

int hg_add_timeout(hg_event_t *ev,int msec){

      ev->id=hg_epoll_ctx.id++;
      ev->msec=hg_epoll_ctx.cur_msec+msec;

      hg_epoll_ctx.timetree->insert((void*)ev);
  
      ev->in_time=true;
      ev->timeout=false;

      return HG_OK;
}

int hg_del_timeout(hg_event_t *ev){

      hg_epoll_ctx.timetree->erase((void*)ev);
      ev->in_time=false;

      return HG_OK;
}

int hg_time_cmp(void*n1,void*n2){

    hg_event_t *ev1=(hg_event_t*)n1;
    hg_event_t *ev2=(hg_event_t*)n2;
 
    unsigned long long k=ev1->msec-ev2->msec;

    return k==0?(ev1->id-ev2->id):k;
}








#endif




