#ifndef HG_HTTP_MODULE_CPP
#define HG_HTTP_MODULE_CPP
#include"../include/hg_http_module.h"
#include"../include/hg_epoll_module.h"
#include"../include/hg_define.h"
#include"../include/hg_conf_parse.h"


void* hg_http_create_conf(hg_cycle_t *cycle);

int   hg_http_block(hg_module_t *module,hg_cycle_t *cycle,cris_conf_t *conf);
int   hg_http_init_module(hg_module_t *module,hg_cycle_t *cycle);
//int   hg_http_init_module(hg_module_t *m,hg_cycle_t *cycle);

int num_http_modules=0;//模块的数量

std::vector<hg_command_t> http_commands={
    {
       std::string("http"),
       0,
       &hg_http_block
    }
};

hg_http_module_t hg_http_ctx={
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL
};


hg_module_t hg_http_module={
     HG_HTTP_MODULE,
     0,
     0,
     &hg_http_ctx,
     &http_commands,
     &hg_http_create_conf,
     NULL,
     &hg_http_init_module,
     NULL
};


int   hg_http_init_module(hg_module_t *module,hg_cycle_t *cycle){


     std::vector<hg_module_t*> *modules=cycle->modules;
     
     hg_http_conf_t *conf=(hg_http_conf_t*)hg_get_module_conf(module,cycle);

     for(hg_module_t *m:*modules){

          if(m->type!=HG_HTTP_MODULE)
               continue;
          hg_http_module_t *http_m=(hg_http_module_t*)m->ctx;
           
          if(http_m->postconfiguration)
               http_m->postconfiguration(m,cycle,conf);

     }

     return HG_OK;
}

void* hg_http_create_conf(hg_cycle_t *cycle){

    cris_mpool_t *pool=cycle->pool;
    hg_http_conf_t *conf=(hg_http_conf_t*)pool->alloc(sizeof(hg_http_conf_t));

    return conf;
}


int   hg_http_block(hg_module_t *module,hg_cycle_t *cycle,cris_conf_t *conf){

      if(conf->avgs.size()!=1)
         return HG_ERROR;


      cris_mpool_t *pool=cycle->pool;
      std::vector<hg_module_t*> *modules=cycle->modules;
      
      hg_http_conf_t *http_conf=(hg_http_conf_t*)hg_get_module_conf(module,cycle);

      cris_str_t  block=conf->avgs.front();

      char *pre=block.str;
      char *end=pre+block.len; 
      cris_conf_t  conf_tmp;

      //为全体http模块设置http模块内部的序号
      num_http_modules=0;
      for(hg_module_t *mm: *modules){

         if(mm->type!=HG_HTTP_MODULE)
            continue;

         mm->ctx_index=num_http_modules++;
         mm->index=module->index;//以便其他模块对顶级配置的快速访问

      } 

      http_conf->main_conf=(void**)pool->alloc(num_http_modules*sizeof(void*));
      http_conf->srv_conf=(void**)pool->alloc(num_http_modules*sizeof(void*));       
      http_conf->loc_conf=(void**)pool->alloc(num_http_modules*sizeof(void*));


      for(hg_module_t *m:*modules){

          if(m->type!=HG_HTTP_MODULE)
             continue;

          hg_http_module_t *http_m=(hg_http_module_t*)m->ctx;

          if(http_m->create_main_conf)
             http_conf->main_conf[m->ctx_index]=(void*)http_m->create_main_conf(cycle);

          if(http_m->create_srv_conf)
             http_conf->srv_conf[m->ctx_index]=(void*)http_m->create_srv_conf(cycle);

          if(http_m->create_loc_conf)
             http_conf->loc_conf[m->ctx_index]=(void*)http_m->create_loc_conf(cycle);

          if(http_m->preconfiguration)
             http_m->preconfiguration(cycle);

      }


      while((pre=cris_take_one_conf(pre,end,&conf_tmp))!=NULL){

            bool found=false;             
 
            for(hg_module_t *m:*modules){

                if(m->type!=HG_HTTP_MODULE)
                   continue;
                
                std::vector<hg_command_t> *commands=m->commands; 
     
                for(hg_command_t &cd:*commands){

                    if(!(cd.conf_name==conf_tmp.name))
                       continue;

                    if(!(cd.info&HG_CMD_HTTP)){
		       printf("hg_http_block():error: %s 不能出现在http域中\n",cd.conf_name.c_str());
		       return HG_ERROR;
		    }

         
                    found=true;
 
                    ((hg_http_module_t*)m->ctx)->ptr=http_conf;

                    if(cd.conf_handler(m,cycle,&conf_tmp)!=HG_OK){
		       printf("hg_http_block():error: %s 解析错误\n",cd.conf_name.c_str());
		       return HG_ERROR;
		    }
                                     
                    break;
                    
                }
 
                if(found)
                   break;
 
            }

	    if(!found){
        	conf_tmp.name.str[conf_tmp.name.len]='\0';
		printf("无法识别的配置: %s\n",conf_tmp.name.str);   
		return HG_ERROR;
	    }
      }

      for(hg_module_t *m:*modules){

          if(m->type!=HG_HTTP_MODULE)
               continue;
          hg_http_module_t *http_m=(hg_http_module_t*)m->ctx;
           
          if(http_m->init_main_conf)
               http_m->init_main_conf(m,cycle,http_conf);

      }


      return HG_OK;
}







#endif







