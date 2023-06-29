#ifndef CRIS_STR_H
#define CRIS_STR_H
#include<cstring>
#include<cstdio>
#include<string>
struct cris_str_t{
    char* str=NULL;
    int len=0;

    friend bool operator==(const cris_str_t& s1,const cris_str_t& s2){
           if(s1.len!=s2.len)
              return false;
           return memcmp(s1.str,s2.str,s1.len)==0;      
    }

    friend bool operator==(const std::string &s1,const cris_str_t&s2){
           if(s1.length()!=s2.len)
              return false;
           return memcmp(s1.c_str(),s2.str,s2.len)==0;
    }

    bool operator==(const std::string &s){
          if(len!=s.length())
             return false;
	  return memcmp(str,s.c_str(),len)==0;
    }

    friend bool operator<(const cris_str_t&s1,const cris_str_t&s2){
           int len=s1.len<s2.len?s1.len:s2.len;
           int m;
           for(int i=0;i<len;i++){
               m=s1.str[i]-s2.str[i];
               if(m==0)
                 continue;
               return m<0;
           }
           return s1.len<s2.len;        
    }

    cris_str_t():str(NULL),len(0){}

    cris_str_t(const cris_str_t&s);

    cris_str_t(char *s):str(s){
         len=strlen(s);
    }

    cris_str_t& operator=(const cris_str_t&s);

};


struct hg_str_hasher{

    size_t operator()(const cris_str_t&s) const{
            char *p=s.str;
            int len=s.len;
            size_t hash=0;
            for(int i=0;i<len;i++)
              hash+=(p[i]-'\0'+i)*107; 

            hash*=1707;
            return hash;
    }
};


bool  is_prefix(cris_str_t&s1,cris_str_t&s2);

void  cris_str_print(cris_str_t *str);






#endif











