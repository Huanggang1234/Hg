#ifndef HG_LOG_MODULE_CPP
#define HG_LOG_MODULE_CPP
#include"../include/hg_log_module.h"
#include"../include/hg.h"
#include"../include/hg_conf_parse.h"
#include"../include/hg_define.h"
#include"../include/hg_epoll_module.h"
#include"../base/cris_buf.h"
#include"../base/include/cris_str.h"
#include"../include/hg_http_module.h"
#include"../include/hg_http_core_module.h"
#include"../include/hg_http_core_str_base.h"
#include<unistd.h>
#include<fcntl.h>
#include<ctime>
#include<arpa/inet.h>


static int hg_log_set_level(hg_module_t *module,hg_cycle_t *cycle,cris_conf_t *conf);
static int hg_log_set_flush_time(hg_module_t *module,hg_cycle_t *cycle,cris_conf_t *conf);
static int hg_log_set_access_size(hg_module_t *module,hg_cycle_t *cycle,cris_conf_t *conf);
static int hg_log_set_error_size(hg_module_t *module,hg_cycle_t *cycle,cris_conf_t *conf);
static int hg_log_set_access_file_size(hg_module_t *module,hg_cycle_t *cycle,cris_conf_t *conf);
static int hg_log_set_error_file_size(hg_module_t *module,hg_cycle_t *cycle,cris_conf_t *conf);


static int hg_log_init(hg_module_t *module,hg_cycle_t *cycle);
static int hg_log_process(hg_module_t *module,hg_cycle_t *cycle);

static int hg_log_flush_handler(hg_event_t *ev);
static inline  int hg_do_log(int fd,char*start,char*end);

static int hg_http_log_handler(cris_http_request_t *r);


static std::vector<hg_command_t> log_commands={
      {
         std::string("log_level"),
         HG_CMD_MAIN,
         &hg_log_set_level
      },
      {
         std::string("log_flush_time"),
         HG_CMD_MAIN,
	 &hg_log_set_flush_time
      },
      {
         std::string("log_access_size"),
	 HG_CMD_MAIN,
	 &hg_log_set_access_size
      },
      {
         std::string("log_error_size"),
         HG_CMD_MAIN,
	 &hg_log_set_error_size
      }
};

static int log_level=HG_LOG_ERROR;//error日志记录的等级
static unsigned int access_log_size=512;//以字节为单位的缓冲大小
static unsigned long long access_log_file_size=1024*1024*50;
static unsigned int error_log_size=512;//以字节为单位缓冲大小
static unsigned long long error_log_file_size=1024*1024*50;
static unsigned int flush_time=1000;//以毫秒为单位缓冲刷新时间

static hg_log_t  log_ctx;
static std::string log_path="./";

static hg_event_t  log_event_timer;//用于定时刷新缓冲

hg_module_t hg_log_module={
       HG_LOG_MODULE,
       0,
       0,
       NULL,
       &log_commands,
       NULL,
       NULL,
       &hg_log_init,
       &hg_log_process
};


static std::string s1="error";
static std::string s2="warning";
static std::string s3="notice";

const char *ss1="error";
const char *ss2="warning";
const char *ss3="notice";


static long long convert(cris_str_t unit){

     using std::string;

     if(unit==string("k")||unit==string("K")||unit==string("KB")||unit==string("kb"))
         return 1024;

     if(unit==string("m")||unit==string("M")||unit==string("MB")||unit==string("mb"))
         return 1024*1024;

     if(unit==string("g")||unit==string("G")||unit==string("GB")||unit==string("gb"))
         return 1024*1024*1024;

     if(unit==string("b")||unit==string("B"))
         return 1;


     if(unit==string("s")||unit==string("S"))
         return 1000;

     if(unit==string("m")||unit==string("M")||unit==string("min")||unit==string("MIN"))
         return 1000*60;

     if(unit==string("h")||unit==string("H"))
         return 1000*60*60;

     if(unit==string("ms")||unit==string("MS"))
         return 1;
     
     return HG_ERROR;
}




int hg_log_set_level(hg_module_t *module,hg_cycle_t *cycle,cris_conf_t *conf){

       if(conf->avgs.size()!=1)
           return HG_ERROR;

       cris_str_t lev=conf->avgs.front();

       if(lev==s1){
          log_level=HG_LOG_ERROR;
       }
       if(lev==s2){
          log_level=HG_LOG_WARNING;
       }
       if(lev==s3){
          log_level=HG_LOG_NOTICE;
       }

       printf("log level %d\n",log_level);

       return HG_OK;
}


int hg_log_set_flush_time(hg_module_t *module,hg_cycle_t *cycle,cris_conf_t *conf){

    if(conf->avgs.size()==0||conf->avgs.size()>2)
       return HG_ERROR;
    
    cris_str_t t=conf->avgs.front();

    t.str[t.len]='\0';

    int num=atoi(t.str);
    
    flush_time=num;

    if(conf->avgs.size()==2){
    
       conf->avgs.pop_front();
       flush_time*=convert(conf->avgs.front());

       if(flush_time<0)
         return HG_ERROR;
    }

    printf("log flush time %u\n",flush_time);

    return HG_OK;
}

int hg_log_set_access_size(hg_module_t *module,hg_cycle_t *cycle,cris_conf_t *conf){

    if(conf->avgs.size()==0||conf->avgs.size()>2)
       return HG_ERROR;
    
    cris_str_t t=conf->avgs.front();

    t.str[t.len]='\0';

    int num=atoi(t.str);
    
    access_log_size=num;

    if(conf->avgs.size()==2){
    
       conf->avgs.pop_front();
       access_log_size*=convert(conf->avgs.front());

       if(access_log_size<0)
         return HG_ERROR;
    }

    printf("access_log_size %u\n",access_log_size);

    return HG_OK;
}


int hg_log_set_error_size(hg_module_t *module,hg_cycle_t *cycle,cris_conf_t *conf){

    if(conf->avgs.size()==0||conf->avgs.size()>2)
       return HG_ERROR;
    
    cris_str_t t=conf->avgs.front();

    t.str[t.len]='\0';

    int num=atoi(t.str);
    
    error_log_size=num;

    if(conf->avgs.size()==2){
        conf->avgs.pop_front();
        error_log_size*=convert(conf->avgs.front());

       if(error_log_size<0)
          return HG_ERROR;
    }

    printf("error_log_size %u\n",error_log_size);

    return HG_OK;
}


int hg_log_set_access_file_size(hg_module_t *module,hg_cycle_t *cycle,cris_conf_t *conf){


    if(conf->avgs.size()==0||conf->avgs.size()>2)
       return HG_ERROR;
    
    cris_str_t t=conf->avgs.front();

    t.str[t.len]='\0';

    int num=atoi(t.str);
    
    access_log_file_size=num;

    if(conf->avgs.size()==2){
    
       conf->avgs.pop_front();
       access_log_file_size*=convert(conf->avgs.front());

       if(access_log_file_size<0)
         return HG_ERROR;

    }

    printf("access_log_file_size %u\n",access_log_file_size);

    return HG_OK;


}


int hg_log_set_error_file_size(hg_module_t *module,hg_cycle_t *cycle,cris_conf_t *conf){


    if(conf->avgs.size()==0||conf->avgs.size()>2)
       return HG_ERROR;
    
    cris_str_t t=conf->avgs.front();

    t.str[t.len]='\0';

    int num=atoi(t.str);
    
    error_log_file_size=num;

    if(conf->avgs.size()==2){
      
       conf->avgs.pop_front();
       error_log_file_size*=convert(conf->avgs.front());

       if(error_log_file_size<0)
         return HG_ERROR;
    }

    printf("error_log_file_size %u\n",error_log_file_size);

    return HG_OK;

}


int hg_log_process(hg_module_t *module,hg_cycle_t *cycle){

//    log_event_timer.time_handler=&hg_log_flush_handler;
//    hg_add_timeout(&log_event_timer,flush_time);
    
    //以下操作清空在启动阶段残留的缓冲，避免在worker进程中重复打印
    if(log_ctx.access_buf->available()){
      hg_do_log(log_ctx.access_log_fd,log_ctx.access_buf->cur,log_ctx.access_buf->last);
      log_ctx.access_buf->cur=log_ctx.access_buf->last=log_ctx.access_buf->start;
    }
    return HG_OK;
}



int hg_log_init(hg_module_t *module,hg_cycle_t *cycle){

    std::string access_file_path=log_path[log_path.length()-1]=='/'?log_path+"access.log":log_path+"/access.log";
    std::string error_file_path=log_path[log_path.length()-1]=='/'?log_path+"error.log":log_path+"/error.log";

    log_ctx.access_log_fd=open(access_file_path.c_str(),O_WRONLY|O_APPEND|O_CREAT,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
    if(log_ctx.access_log_fd<0){
         printf("hg_log_init():access.log 文件创建失败,请检查路径是否存在,或是否拥有权限\n");
         return HG_ERROR;
    }

    log_ctx.error_log_fd=open(error_file_path.c_str(),O_WRONLY|O_APPEND|O_CREAT,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
    if(log_ctx.error_log_fd<0){
         printf("hg_log_init():error.log 文件创建失败,请检查路径是否存在,或者是否拥有权限\n");
	 return HG_ERROR;
    }

    log_ctx.access_buf=new (cycle->pool->alloc(sizeof(cris_buf_t)))cris_buf_t(cycle->pool,access_log_size);
    log_ctx.error_buf=new (cycle->pool->alloc(sizeof(cris_buf_t)))cris_buf_t(cycle->pool,error_log_size);


    hg_http_add_request_handler(&hg_http_log_handler,HG_HTTP_LOG_PHASE);


    return HG_OK;
}



inline int hg_do_log(int fd,char*start,char*end){
          
     int cnt= write(fd,start,end-start);
     return cnt;
}

void hg_log_printf(int level,const char *format,...){

    cris_buf_t *buf=log_ctx.access_buf;
    char  *end=buf->end;
    char  *cur=buf->cur;
    char  *last=buf->last;
    const char  *t;
    va_list li;
    va_start(li,format);

sf:
    while(last<end&&(*format)!='\0'){

       if(*format=='%'){
       
       
         t=va_arg(li,const char*);
sf2:
         while(last<end&&*t!='\0')
            *last++=*t++;

         if(last==end){
	    hg_do_log(log_ctx.error_log_fd,cur,last);
	    cur=last=buf->start;
	    goto sf2;
         }
	  
	 format++;

      }else{
         *last++=*format++;
      }
    }
    
    hg_do_log(log_ctx.access_log_fd,cur,last);

    if(last==end&&(*format)!='\0'){
         cur=last=buf->start;
         goto sf;
    }
    
    buf->cur=buf->last=buf->start;

/*
    cris_buf_t *buf=log_ctx.access_buf;
    char  *end=buf->end;
    char  *cur=buf->cur;
    char  *last=buf->last;
    const char  *t;
    va_list li;
    va_start(li,format);

sf:
    while(last<end&&(*format)!='\0'){

       if(*format=='%'){
	      
          t=va_arg(li,const char*);
sf2:
          while(last<end&&*t!='\0')
	      *last++=*t++;

          if(last==end){
	      hg_do_log(log_ctx.access_log_fd,cur,last);
	      cur=last=buf->start;
              goto sf2;
          }
         
	  format++;
           
       }else{
           *last++=*format++;
       }
    }
    
    if(last==end){
      hg_do_log(log_ctx.access_log_fd,cur,last);
      cur=last=buf->start;
      goto sf;
    }

    buf->cur=cur;
    buf->last=last;
*/
}


void hg_error_log(int level,const char *format,...){

    cris_buf_t *buf=log_ctx.error_buf;
    char  *end=buf->end;
    char  *cur=buf->cur;
    char  *last=buf->last;
    const char  *t;
    va_list li;
    va_start(li,format);

sf:
    while(last<end&&(*format)!='\0'){

       if(*format=='%'){
       
       
         t=va_arg(li,const char*);
sf2:
         while(last<end&&*t!='\0')
            *last++=*t++;

         if(last==end){
	    hg_do_log(log_ctx.error_log_fd,cur,last);
	    cur=last=buf->start;
	    goto sf2;
         }
	  
	 format++;

      }else{
         *last++=*format++;
      }
    }
    
    hg_do_log(log_ctx.error_log_fd,cur,last);

    if(last==end&&(*format)!='\0'){
         cur=last=buf->start;
         goto sf;
    }
    
    buf->cur=buf->last=buf->start;
}


int hg_log_flush_handler(hg_event_t *ev){
   
    if(log_ctx.access_buf->available()>0){
      hg_do_log(log_ctx.access_log_fd,log_ctx.access_buf->cur,log_ctx.access_buf->last);
      log_ctx.access_buf->cur=log_ctx.access_buf->last=log_ctx.access_buf->start;
    }

    hg_add_timeout(&log_event_timer,flush_time);
    return HG_OK;
}


static const char* RESPONSE_ACCESS_CODE[]={
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    "200 OK",
    "403 Forbidden",
    "404 Not Found",
    "500 Internal Server Error",
};


static int hg_http_log_handler(cris_http_request_t *r){

    static char time_str[256];

    time_t start_sec=r->start_msec/1000;

    cris_str_t url=r->url;
    cris_str_t method=r->method;
    
    int access_code=r->access_code;

    struct sockaddr_in addr=r->conn->sockaddr;

    struct tm *local=localtime(&start_sec);


    time_str[strftime(time_str,256,"%Y-%m-%d %H:%M:%S",local)]='\0';
    url.str[url.len]='\0';
    method.str[method.len]='\0';
    
    hg_log_printf(0,"[ % from: % response: % ]: method: % URL: %\n",time_str,     \
            inet_ntoa(addr.sin_addr),RESPONSE_ACCESS_CODE[access_code],method.str,url.str);

    hg_error_log(0,"[ % ]: URL: % \n",time_str,url.str);


    return HG_DECLINED;
}


#endif














