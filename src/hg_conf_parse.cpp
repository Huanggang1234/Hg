#ifndef HG_CONF_PARSE_CPP
#define HG_CONF_PARSE_CPP
#include"../include/hg_conf_parse.h"
#include<unistd.h>
#include<fcntl.h>
#include<cstring>
#include<cstdlib>
 



char* readcmdt(char *start,char *end){

      while(start<end&&(*start)!='\n'){
          start++;
      }
      return start<end?start:end;
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
                      if(ch==' '||ch=='\n'||ch=='\t'||ch=='\r'){
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
                        
                      }else if(ch==';'){

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
                          
                         if(ch==' '||ch=='\n'||ch=='\t'||ch=='\\'||ch=='\r')
                            continue;
                        
                         if(ch==';'){
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

                      if(ch==' '||ch=='\t'||ch=='\n'||ch=='\r'){

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


                      }else if(ch==';'){                   

                         avg.len=cur-avg.str;
                         avgs.push_back(avg);
                         e=++cur;
                         
                         goto analyse_succeed;

                      }
                    
                    break;	    

          }

    }

    return NULL;

    analyse_error:
           
        return e;         

    analyse_succeed:

         conf->name=name;
         conf->avgs.swap(avgs);
         return e;

}


#endif
