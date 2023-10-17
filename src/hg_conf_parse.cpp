#ifndef HG_CONF_PARSE_CPP
#define HG_CONF_PARSE_CPP
#include"../include/hg_conf_parse.h"
#include"../base/cris_memery_pool.h"
#include"../include/hg_define.h"
#include<unistd.h>
#include<fcntl.h>
#include<cstring>
#include<cstdlib>
 


long long unit_convert(cris_str_t unit){

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



static char* readcmdt(char *start,char *end){

      while(start<end&&(*start)!='\n'){
          start++;
      }
      return start<end?start:end;
}

cris_str_t cris_filter_convert(cris_str_t token,cris_mpool_t *pool){

       char *cur=token.str;
       char *end=token.str+token.len;

       bool notconvert=true;

       while(cur<end&&*cur!='\\')
           cur++;
       
       if(cur==end)
          return token;

       
       cris_str_t new_str;
       new_str.str=(char*)pool->qlloc(sizeof(char)*token.len);

       char *cur2=new_str.str;

       cur=token.str;
       
       while(cur<end){
       
            char ch=*cur;
	    bool notconvert=true;
       
            if(ch=='\\'){

	        do{
		   cur++;		
		   if(cur>=end)
		     break;
		   
		   ch=*cur;

		   switch(ch){
		   
	              case 'n':
		              notconvert=false;
                              ch='\n';
                              break;
		      case 't':
		              notconvert=false;
			      ch='\t';
                              break;
		      case 'r':
		              notconvert=false;
                              ch='\r';
			      break;
                      case ';':
		              notconvert=false;
			      ch=';';
			      break;
	              case 's':
		              notconvert=false;
			      ch=' ';
			      break;
		      case '\\':
		              notconvert=false;
			      ch='\\';
			      break;
		   }
		
		}while(notconvert&&(ch==' '||ch=='\n'||ch=='\t'||ch=='\r'));
	   }

	   *cur2++=ch;
	    cur++;
    }

    new_str.len=cur2-new_str.str;
    
    return new_str;
}


//解析成功，将装填conf，并返回最后一个被解析字符后的指针，解析失败返回NULL
char*  cris_take_one_conf(char *start_,char *end,cris_conf_t *conf){

     char *cur=start_;
     //char *end=end;

     cris_conf_state state=start;

     cris_str_t name;     

     cris_str_t avg;

     std::list<cris_str_t> avgs;

     int  num_brackets=0;//解析中大括号数量

     char *e=NULL;

     for(;cur<end;cur++){

           char ch= *cur;
           bool notconvert=true;//是否不是转义字符
           
           if(ch=='\\'){

                cur++;
		if(cur>=end)
		 goto analyse_notfinish;
                
		while(cur<end&&(*cur=='\n'||*cur=='\r'||*cur=='\t'||*cur==' '))
		      cur++;
                
		if(cur>=end)
		 goto analyse_notfinish;
                
		ch=*cur;

		if(ch==';')
		  notconvert=false;
	   }

           switch(state){

               case start:
                      if((ch>='a'&&ch<='z')||(ch>='A'&&ch<='Z')){
                         name.str=cur;
                         state=naming;//状态转移
                      }else if(ch=='#'){
		         cur=readcmdt(cur,end);
		      }
                    break;                    

               case naming: 
                      if((ch==' '||ch=='\n'||ch=='\t'||ch=='\r')&&notconvert){
                          name.len=cur-name.str;                          
                          state=blank_avg;                         
                      }else if(ch=='{'){
                          name.len=cur-name.str;
                          cur++;

                          if(cur>=end)
                            goto analyse_error;                          

                          avg.str=cur;
                          num_brackets=0;
                          while(cur<end&&(*cur!='}'||num_brackets>0)){
                              if(*cur=='{')
                                 num_brackets++;   
                              else if(*cur=='}')
                                 num_brackets--;
                              cur++;
                          }
                           
                          if(cur>=end)
                             goto analyse_error;                    
        
                          avg.len=cur-avg.str;
 
                          avgs.push_back(avg);        

                          e=++cur;    

                          goto analyse_succeed;           
                        
                      }else if(ch==';'&&notconvert){

                           name.len=cur-name.str;
                           e=++cur;
                           goto analyse_succeed;

                      }else if(ch=='#'){
		           cur=readcmdt(cur,end);
		      }

                    break;

               case blank_avg:

	                 if(ch=='#'){
			    cur=readcmdt(cur,end);
			    continue;
			 }
                          
                         if((ch==' '||ch=='\n'||ch=='\t'||ch=='\\'||ch=='\r')&&notconvert)
                            continue;
                        
                         if(ch==';'&&notconvert){
                              e=++cur;
                              goto analyse_succeed;
                          }


                         if(ch=='{'){
                            cur++;
                            
                            if(cur>=end)
                              goto analyse_error;                            

                            avg.str=cur;
                            num_brackets=0;
                            while(cur<end&&(*cur!='}'||num_brackets>0)){
                              if(*cur=='{')
                                 num_brackets++;   
                              else if(*cur=='}')
                                 num_brackets--;
                              cur++;
                            }
                           
                            if(cur>=end)
                               goto analyse_error;

                            avg.len=cur-avg.str;
 
                            avgs.push_back(avg);
                            
                            e=++cur;
                                    
                            goto analyse_succeed;   

                         }else{
                           avg.str=cur;
                           state=avging;
                         }                         

                    break;

               case avging:

                      if(ch=='#'){
		        cur=readcmdt(cur,end);
			continue;
		      }

                      if((ch==' '||ch=='\t'||ch=='\n'||ch=='\r')&&notconvert){

                         avg.len=cur-avg.str;
                         avgs.push_back(avg);
                         state=blank_avg;                         

                      }else if(ch=='{'){

                         avg.len=cur-avg.str;
                         avgs.push_back(avg);
                         
                         cur++;

                         if(cur>=end)
                            goto analyse_error;

                         avg.str=cur;
                         num_brackets=0;
                         while(cur<end&&(*cur!='}'||num_brackets>0)){
                            if(*cur=='{')
                               num_brackets++;   
                            else if(*cur=='}')
                               num_brackets--;
                             cur++;
                          }
                        
                         if(cur>=end)
                             goto analyse_error;

                         avg.len=cur-avg.str;
                         avgs.push_back(avg);
                          
                         e=++cur;

                         goto analyse_succeed;


                      }else if(ch==';'&&notconvert){                   

                         avg.len=cur-avg.str;
                         avgs.push_back(avg);
                         e=++cur;
                         
                         goto analyse_succeed;

                      }
                    
                    break;	    

          }

    }

    analyse_notfinish:

        return NULL;

    analyse_error:
           
        return e;         

    analyse_succeed:

         conf->name=name;
         conf->avgs.swap(avgs);
         return e;

}


#endif
