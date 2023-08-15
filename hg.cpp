#ifndef HG_CPP
#define HG_CPP
#include"include/hg.h"
#include"include/hg_epoll_module.h"
#include"include/hg_http_module.h"
#include"include/hg_http_core_module.h"
#include"include/hg_control_module.h"
#include"include/hg_define.h"
#include<vector>
#include<fstream>
#include"base/cris_buf.h"
#include"include/hg_conf_parse.h"

#include"include/modules/hg_http_fastcgi_module.h"

#endif

std::vector<hg_module_t*> modules={
    &hg_epoll_module,
    &hg_http_module,
    &hg_http_core_module,
    &hg_control_module,

    &hg_http_fastcgi_module
};

hg_cycle_t  cycle;


int hg_init_cycle(hg_cycle_t *cycle);
int hg_parse_conf(hg_cycle_t *cycle);


int hg_parse_conf(hg_cycle_t *cycle){

   void* p=NULL; 

   cris_mpool_t*pool=cycle->pool;

   std::ifstream conf_file(cycle->conf_path.c_str(),std::ios::in|std::ios::binary);

   if(!conf_file){
      printf("配置文件打开失败\n");
      return HG_ERROR;
   }

   conf_file.seekg(0,std::ios::end);
   int len=conf_file.tellg();
   conf_file.seekg(0,std::ios::beg);

   p=pool->alloc(sizeof(cris_buf_t)),cycle->conf_file=new (p)cris_buf_t(pool,len+1);  


   while(!conf_file.eof()){
       conf_file.read(cycle->conf_file->start,len);
   }//读取配置文件

   conf_file.close();

   char *pre=cycle->conf_file->start;
   char *end=(char*)(pre+len);

   cris_conf_t conf;

   while((pre=cris_take_one_conf(pre,end,&conf))!=NULL){

        bool found=false;

        for(hg_module_t *m:modules){

            
              std::vector<hg_command_t> *commands=m->commands;

              if(commands==NULL)
                 continue;
              for(hg_command_t& cd:*commands){
                  if(!(conf.name==cd.conf_name))
                      continue;

                  found=true;

                  cd.conf_handler(m,cycle,&conf);
                   
                  break;

              }

              if(found)
                 break;

       }

      if(!found){
         printf("未识别配置\n");   
         return HG_ERROR;
      }
  }

  return HG_OK;

}


int hg_init_cycle(hg_cycle_t *cycle){

   int num_modules=modules.size();
       
   cris_mpool_t *pool=new cris_mpool_t();

   cycle->pool=pool;
  
   cycle->conf_ctx=(void**)pool->alloc(num_modules*sizeof(void*));

   cycle->modules=&modules;

   for(int i=0;i<num_modules;i++){

       hg_module_t *m=modules[i];
       m->index=i;//设置模块的编号

       if(m->create_conf){
           cycle->conf_ctx[i]=m->create_conf(cycle);
       }else{
           cycle->conf_ctx[i]=NULL;
       }

   }   

   hg_parse_conf(cycle);
      
   for(int i=0;i<num_modules;i++){

       hg_module_t *m=modules[i];
       if(m->init_conf){
         m->init_conf(m,cycle);
       }
   }


   for(int i=0;i<num_modules;i++){

       hg_module_t *m=modules[i];
       if(m->init_module){
         m->init_module(m,cycle);
       }
   }

   return HG_OK;
}

int test=0;



int main(int argc,char *argv[]){

    if(argc!=2){

        printf("参数错误\n");
        return 0;
    }


    cycle.conf_path=argv[1];

    hg_init_cycle(&cycle);

//    printf("1\n");

//    printf("ls %p\n",&ls);
 

    for(hg_module_t *m:modules){

        if(m->init_process)
            m->init_process(m,&cycle);

    }
 
//    printf("2\n");


//    while(test!=1){

//       hg_epoll_process_events();

  //  }

    hg_work();


    return 0;
}











