#ifndef  CRIS_BUF_H
#define  CRIS_BUF_H
#include"cris_memery_pool.h"
#include<cstring>
#include<stdio.h>
struct cris_buf_t{
    char         *start;//缓冲的开始
    char          *last;
    char           *end;//缓冲的结尾字节
    char           *cur;//供外部使用的自由指针
    int        capacity;
    int             res;
    int            used;
    cris_mpool_t  *pool;

    cris_buf_t():start(NULL),last(NULL),end(NULL),cur(NULL),
capacity(0),res(0),used(0),pool(NULL){}

    cris_buf_t(cris_mpool_t *mp,int size){
        pool=mp;
        cur=last=start=(char*)pool->qlloc(size);
        capacity=size;
        res=size;
        end=start+capacity;
        used=0;
    }

    cris_buf_t(cris_mpool_t *mp,char *s,int length):start(s),end(s+length),res(0),capacity(length),used(length),pool(mp) 
    {
    
    	last=end;
	cur=start;
    
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

    int appendfixed(char *data,int len){
        
	int cnt=res<=len?res:len;

        memcpy(last,data,cnt);

        last=last+cnt;
        res-=cnt;
	used+=cnt;

	return cnt;

    }

    void reuse(){
    
         char *c=cur;
	 char *l=last;
         char *s=start;
         int  len=l-c;

         while(c<l){//如果有还未使用的内容，复制到起始位置
	 
	    *s= *c;
	    s++;
	    c++;
	 }
    
         cur=start;
         last=start+(len<0?0:len);
         res=end-last;
	 used=last-start;
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

    void take(int num){
         cur+=num;
	 if(cur>=last)
	   cur=last=start;
         
	 used=last-start;
	 res=capacity-used;
    }
     
    void extend(int num){
         last+=num;
         used+=num;
	 res-=num;
    }

    int  surplus(){
         return end-last;
    }

    double blank(){         
	  return (cur-start)*1.0/(end-start);
    }

    void  check(){
    
          if(cur>=last){
	     cur=last=start;
	     used=0;
	     res=capacity;
	  }
    }
};


struct cris_chain_t{
       cris_buf_t *b=NULL;
       cris_chain_t *next=NULL;
       bool  last_node=false;
};


#endif






