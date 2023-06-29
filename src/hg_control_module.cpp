#ifndef HG_CONTROL_MODULE_CPP
#define HG_CONTROL_MODULE_CPP
#include"../include/hg.h"
#include"../include/hg_define.h"
#include"../include/hg_control_module.h"
#include"../include/hg_conf_parse.h"
#include<signal.h>
#include<cstdlib>
#include<cstdio>
#include<sys/wait.h>
#include"../include/hg_epoll_module.h"
#include<unistd.h>
#include<sys/mman.h>
#include<sys/resource.h>
#include<set>

int  hg_control_set_worker(hg_module_t *module,hg_cycle_t *cycle,cris_conf_t *conf);
void*  hg_control_create_conf(hg_cycle_t *cycle);

void hg_control_close_worker();


bool  hg_control_module_term=false;
int   hg_control_module_worker_num=0;

std::set<pid_t>    hg_control_pids;

void  hg_control_sig_alrm(int sig);
void  hg_control_sig_term(int sig);
void  hg_control_sig_chld(int sig);
void  hg_control_sig_int(int sig);

std::vector<hg_command_t>  control_commands={
    {
      std::string("worker"),
      0,
      &hg_control_set_worker
    }
};



hg_module_t  hg_control_module={
      HG_CONTROL_MODULE,
      0,
      0,
      NULL,
      &control_commands,
      &hg_control_create_conf,
      NULL,
      NULL,
      NULL
};


hg_control_conf_t  hg_control_conf;
/*****************/
struct timeval  control_time;
unsigned int control_flag=0;
void *shared_s=NULL;
void *shared_c=NULL;
int shared_length=0;
pthread_mutex_t *accept_mutex=NULL;
pthread_mutexattr_t *accept_mutex_attr=NULL;

/****************/


void*  hg_control_create_conf(hg_cycle_t *cycle){
 
       shared_length=sizeof(pthread_mutex_t)+sizeof(pthread_mutexattr_t)+512;

       shared_s=shared_c=mmap(NULL,shared_length,PROT_READ|PROT_WRITE,MAP_ANON|MAP_SHARED,-1,0);

       accept_mutex=(pthread_mutex_t*)shared_c;

       shared_c=shared_c+sizeof(pthread_mutex_t);

       accept_mutex_attr=(pthread_mutexattr_t*)shared_c;

       pthread_mutexattr_init(accept_mutex_attr);
       pthread_mutexattr_setpshared(accept_mutex_attr,PTHREAD_PROCESS_SHARED);
       pthread_mutex_init(accept_mutex,accept_mutex_attr);


       struct rlimit limit;
       limit.rlim_cur=RLIM_INFINITY;
       limit.rlim_max=RLIM_INFINITY;
 
       if(setrlimit(RLIMIT_NOFILE,&limit)!=0)
             printf("NOFILE提升失败\n");

       if(setrlimit(RLIMIT_AS,&limit)!=0)
             printf("AS提升失败\n");

       if(setrlimit(RLIMIT_CPU,&limit)!=0)
             printf("CPU提升失败\n");

       if(setrlimit(RLIMIT_DATA,&limit)!=0)
             printf("DATA提升失败\n");

       if(setrlimit(RLIMIT_NICE,&limit)!=0)
             printf("NICE提升失败\n");

       if(setrlimit(RLIMIT_RSS,&limit)!=0)
             printf("RSS提升失败\n");

       if(setrlimit(RLIMIT_STACK,&limit)!=0)
             printf("STACK提升失败\n");
 
       return (void*)&hg_control_conf;
}




int  hg_control_set_worker(hg_module_t *module,hg_cycle_t *cycle,cris_conf_t *conf){

     if(conf->avgs.size()==0||conf->avgs.size()>1)
             return HG_ERROR;

     cris_str_t num=conf->avgs.front();

     num.str[num.len]='\0';

     hg_control_conf_t *control_conf=(hg_control_conf_t*)hg_get_module_conf(module,cycle);

     int cnt=atoi(num.str);

     printf("worker=%d\n",cnt);

     control_conf->worker=cnt;

     return HG_OK;
}

void  hg_control_sig_alrm(int sig){
     // printf("SIGALRM\n");
      gettimeofday(&control_time,NULL);
      control_flag|=HG_EPOLL_TIME;
      alarm(2);
      return;
}

void  hg_control_sig_term(int sig){
      printf("SIGTERM\n");
      hg_control_module_term=true;
      return;
}


void  hg_control_sig_chld(int sig){

      pid_t pid;

      if((pid=wait(NULL))>0){
       hg_control_module_worker_num--;
       hg_control_pids.erase(pid);      
      }
      return;
}


void  hg_control_sig_int(int sig){
      printf("SIGINT\n");
      hg_control_module_term=true;
      return;
}


int  hg_work(){

    int cnt=hg_control_conf.worker;
    pid_t pid;
 
    pid=getpid();

    printf("master:%d\n",pid);

    for(int i=0;i<cnt;i++){

       pid=fork();

       if(pid==0){
         printf("worker:%d\n",getpid());
         goto worker;
       }
 
       hg_control_pids.insert(pid);

       hg_control_module_worker_num++;

   }

master:

      hg_master_process();

      return HG_OK; 

worker:

      hg_worker_process();
        
      return HG_OK;
}

int  hg_master_process(){

     signal(SIGCHLD,&hg_control_sig_chld); 
     signal(SIGTERM,&hg_control_sig_term);
     signal(SIGINT,&hg_control_sig_int); 

     int  worker=hg_control_conf.worker;

master_wait:
   
     while((!hg_control_module_term)&&(hg_control_module_worker_num==worker))
              pause();

     
     if(hg_control_module_term){
          for(pid_t g:hg_control_pids){
           
               kill(g,SIGTERM);

          }

          return HG_OK;
     }
    
     while(hg_control_module_worker_num<worker){

          pid_t pid;
           
          if((pid=fork())==0){

             goto do_worker;

          }else{

              hg_control_pids.insert(pid);
              hg_control_module_worker_num++;
          }
 
     }

     goto master_wait;

     return HG_OK;


do_worker:

     hg_worker_process();

     return HG_OK;
}


int  hg_worker_process(){

     sigset_t set;
     sigfillset(&set);
     sigdelset(&set,SIGTERM);
     sigdelset(&set,SIGALRM);
     if(sigprocmask(SIG_SETMASK,&set,NULL)!=0)
        printf("信号屏蔽失败\n");

     signal(SIGTERM,&hg_control_sig_term);
     signal(SIGALRM,&hg_control_sig_alrm);
   
     gettimeofday(&control_time,NULL);//初始时间
     control_flag|=HG_EPOLL_TIME;     
     alarm(2);
     while(!hg_control_module_term){

         hg_epoll_process_events(control_flag);
         control_flag=0;
     }
 
     return HG_OK;     

}









#endif







