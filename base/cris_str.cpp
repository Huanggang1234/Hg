#ifndef CRIS_STR_CPP
#define CRIS_STR_CPP

#include"include/cris_str.h"


bool is_prefix(cris_str_t&s1,cris_str_t&s2){

     if(s1.len>s2.len)
        return false; 
     int l=s1.len;
     for(int i=0;i<l;i++)
       if(s1.str[i]!=s2.str[i])
            return false;
     return true;
}



cris_str_t::cris_str_t(const cris_str_t& s){
    str=s.str;
    len=s.len; 
}

cris_str_t& cris_str_t::operator=(const cris_str_t&s){
    str=s.str;
    len=s.len;
    return *this;
}


void  cris_str_print(cris_str_t *str){

      int n=str->len;
      char *p=str->str;
      while(n--){
         putchar(*p);
         p++;
      }
}

void cris_str_copy(cris_str_t *s1,const char *s2){

      char *s=s1->str;
      char *e=s1->str+s1->len;

      while(s<e){
         *s=*s2;
  	 s++;
	 s2++;
      }
}


#endif
