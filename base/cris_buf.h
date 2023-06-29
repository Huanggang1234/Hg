#ifndef  CRIS_BUF_H
#define  CRIS_BUF_H
#include"cris_memery_pool.h"
#include<cstring>
#include<stdio.h>
struct cris_buf_t{
    char *start;//缓冲的开始
    char *last;
    char *end;//缓冲的结尾字节
    char *cur;//供外部使用的自由指针
    int capacity;
    int res;
    int used;      
    cris_mpool_t *pool;
    cris_buf_t *next;

    cris_buf_t():start(NULL),last(NULL),end(NULL),cur(NULL),
capacity(0),res(0),used(0),pool(NULL),next(NULL){}

    cris_buf_t(cris_mpool_t *mp,int size):next(NULL){
        pool=mp;
        cur=last=start=(char*)pool->qlloc(size);
        capacity=size;
        res=size;
        end=start+capacity;
        used=0;
    }

    cris_buf_t(const cris_buf_t &buf){
         start=buf.start;
         last=buf.last;
         end=buf.end;
         cur=buf.cur;
         capacity=buf.capacity;
         res=buf.res;
         pool=buf.pool;
         used=buf.used;
         next=NULL;
    }
       
//使用前需要将used,res,last归位
    int append(char* data,int len){
       if(res>=len){
       //  printf("直接调用\n");
         memcpy(last,data,len);
         used+=len;
         last+=len;
         res-=len;
         return 1;
       }
       
       while(res<len){
         capacity=capacity*2;
         //printf("cap=%d\n",capacity);
         res=capacity-used;
       } 
       char *p;
       if(pool)
          p=(char*)pool->qlloc(capacity);  
       else
          return -1;

       memcpy(p,start,used);
       cur=cur-start+p;
       start=p;
       end=p+capacity;
        
       last=start+used;
       memcpy(last,data,len);

       used+=len;

       res=capacity-used;

       last+=len;

       return 1;    
    }

    int append(char const* data,int len){
       if(res>=len){
       //  printf("直接调用\n");
         memcpy(last,data,len);
         used+=len;
         last+=len;
         res-=len;
         return 1;
       }
       
       while(res<len){
         capacity=capacity*2;
         //printf("cap=%d\n",capacity);
         res=capacity-used;
       } 
       char *p;
       if(pool)
          p=(char*)pool->qlloc(capacity);  
       else
          return -1;
       memcpy(p,start,used);
       cur=cur-start+p;
       start=p;
       end=p+capacity;
        
       last=start+used;
       memcpy(last,data,len);

       used+=len;

       res=capacity-used;

       last+=len;

       return 1;    
    }


    int available(){
       return last-cur;
    }

    int with_cur(){
       last=cur;
       used=cur-start;
       res=capacity-used;
       return 1;
    }

    void print(){
       printf("打印\n");
       char *p=cur;
       while(p<last){

         putchar(*p);
         p++;
       }
       printf("结束\n");

    }
};


struct  cris_buf_chain_t{
     
     cris_mpool_t *pool;

     cris_buf_t   *start;

     cris_buf_t   *cur;

     int num_buf;
 
     int size;

     cris_buf_chain_t(cris_mpool_t *mp):pool(mp),start(NULL),cur(NULL),num_buf(0),size(0){}
          
};




#endif






