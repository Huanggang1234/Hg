#ifndef CRIS_MEMERY_POOL_H
#define CRIS_MEMERY_POOL_H
#include<list>
#include<functional>
#include<cstdio>
#include<cstring>
#define  MEMERY_POOL_SIZE    4096

struct mry_chunk_t{
   void*start;
   void*cur;
   int capacity;
   int res;
public:
   mry_chunk_t(){
      start=cur=malloc(MEMERY_POOL_SIZE*sizeof(char));
      capacity=res=MEMERY_POOL_SIZE;
   }
         
   ~mry_chunk_t(){
      free(start);
      //printf("释放\n");
   }

   void* alloc(int size){
      void* p=cur;
      cur=(void*)((char*)cur+size);
      res-=size;
      return p;      
   }

};

class  cris_mpool_t{
private:
   mry_chunk_t* cur;
   std::list<mry_chunk_t*> chunks;
   std::list<std::function<void()>> cleaners;
   std::list<void*>  larges;
public:
   cris_mpool_t(){
      cur=new mry_chunk_t();
      chunks.push_back(cur);  
   }

   ~cris_mpool_t(){
       for(std::list<std::function<void()>>::iterator it=cleaners.begin();it!=cleaners.end();it++){       
             (*it)();
       }

       for(std::list<void*>::iterator it=larges.begin();it!=larges.end();it++){
            //printf("删除大块\n");
            free(*it);
       }

       for(std::list<mry_chunk_t*>::iterator it=chunks.begin();it!=chunks.end();it++){
             //printf("删除chunk\n");
             delete *it;
       }
   }
   
   void  add_cleaner(std::function<void()> fc){
       cleaners.push_back(fc);
   }

   void* alloc_block(int large_size){
      void *p=calloc(large_size,sizeof(char));
      larges.push_back(p);
      return p;
   }

   void  free_block(void* p){
      for(std::list<void*>::iterator it=larges.begin();it!=larges.end();){
            if(*it==p){
               free(p);
               larges.erase(it);
               return ;
            }else
              ++it ;
      }
   }

   void* alloc(int size){

       void* p=NULL;

       if(cur->res>=size){
          p=cur->alloc(size);
          memset(p,0,size*sizeof(char));      
          return p;
       }

       cur=new mry_chunk_t();
       chunks.push_back(cur);

       if(cur->res>=size){
          p=cur->alloc(size);
          memset(p,0,size*sizeof(char));
          return p;
       }

       return alloc_block(size);          
   }

   void* qlloc(int size){

       if(cur->res>=size)
          return cur->alloc(size);

       cur=new mry_chunk_t();
       chunks.push_back(cur);
       
       if(cur->res>=size)
          return cur->alloc(size);

       void* p=malloc(size*sizeof(char));
       larges.push_back(p);
       return p;
   }

};
#endif




















