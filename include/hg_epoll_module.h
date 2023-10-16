#ifndef HG_EPOLL_MODULE_H
#define HG_EPOLL_MODULE_H
#include"../base/cris_memery_pool.h"
#include"../base/cris_buf.h"
#include<sys/socket.h>
#include<sys/epoll.h>
#include<sys/time.h>
#include<fcntl.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<map>

struct hg_epoll_ctx_t;
struct hg_epoll_conf_t;
struct hg_event_t;
struct hg_connection_t;
struct hg_listen_t;
struct hg_time_event_t;

struct hg_module_t;
struct hg_cycle_t;

void* hg_epoll_create_conf(hg_cycle_t *cycle);

int  hg_epoll_init_conf(hg_module_t *module,hg_cycle_t *cycle);
int  hg_epoll_init_module(hg_module_t *module,hg_cycle_t *cycle);
int  hg_epoll_init_process(hg_module_t *module,hg_cycle_t *cycle);

hg_connection_t* hg_get_connection();//留给其他模块使用的接口
void hg_return_connection(hg_connection_t *conn);//返还不使用的连接

int  add_conn(hg_connection_t *conn);
int  del_conn(hg_connection_t *conn);

int  add_read(hg_connection_t *conn);
int  add_write(hg_connection_t *conn);

int  hg_close_connection(hg_connection_t *conn);

int  hg_add_timeout(hg_event_t *ev,int msec);//对事件添加定时器
int  hg_del_timeout(hg_event_t *ev);//删除对事件的定时


extern char extra_buf[65536];//可以配合hg_recv_chain函数使用；
int  hg_real_recv(hg_connection_t *conn,int& cnt);//不会自动扩充缓冲区，剩余数据保存在extra_buf当中
int  hg_recv_chain(hg_connection_t *conn,int& cnt);//引用参数返回实际的写入连接缓存的字符数，
//返回值为总共在套接字中提取的字符，剩余字符存放在extra_buf中，用户自行存取
int  hg_recv_discard(hg_connection_t *conn);//从套接字中读出数据并抛弃
int  hg_recv(hg_connection_t *conn);//接收套接字上的信息
int  hg_recv_fixed(hg_connection_t *conn);

int  hg_send(hg_connection_t *conn);//发送信息，只要缓冲availble
int hg_send_file(int sock,int fd,off_t *off,unsigned long length );


int  hg_set_address(hg_connection_t *conn,const char *ip,unsigned short port);
int  hg_set_sock(hg_connection_t *conn);
int  hg_connect(hg_connection_t *conn);

int  hg_epoll_process_events(unsigned int flag);//留给上层的事件处理接口

extern hg_module_t hg_epoll_module;//源文件中EPOLL模块的上下文
extern hg_epoll_ctx_t hg_epoll_ctx;

struct hg_epoll_conf_t{

    int event_num=100;//一次获取事件的最大值
    int wait_time=100;//一次等待事件最长阻塞的时间 
    int connections_size=100;//连接池的大小      
    bool accept_lock=false;
};


typedef  int (*hg_event_handler_pt)(hg_event_t *ev);

//struct hg_time_node_t;

struct hg_event_t{
     void* data=NULL;

     hg_event_handler_pt handler=NULL;
     
     bool     timeout=false;
     unsigned long long instence=0;
     unsigned long long timer_id=0;//超时事件ID
     bool     in_time=false;//是否添加超时

     //以下为时间事件处理的成员
     unsigned long long id=0;
     unsigned long long msec=0;     
     hg_event_handler_pt time_handler=NULL;

     hg_event_t *next=NULL;
};//



struct hg_connection_t{

     int fd=0;//套接字接口

     int dex;//在连接池中的序号

     struct sockaddr_in sockaddr;
     socklen_t  socklen=0;           

     unsigned long long instence=0;

     void* data=NULL;
     
     hg_event_t *read=NULL;
     hg_event_t *write=NULL;

     cris_mpool_t *pool=NULL;

     cris_buf_t *in_buffer=NULL;
     cris_buf_t *out_buffer=NULL;

     hg_connection_t *next=NULL;//仅用于连接池

     int  recv=0;//该连接上已经收到的字节数
     int  send=0;//该连接上已经发送的字节数 

     bool on=false;//连接是否存在
     bool in_read=false;//是否添加读事件
     bool in_write=false;//是否添加写事件
     bool in_epoll=false;

};


struct hg_listen_t{
     
   int fd=0;
   struct sockaddr_in sockaddr;
   socklen_t    socklen=0;

   int backlog=30;//套接字监听队列的长度

   short port=80;//监听的端口号


   hg_connection_t *conn=NULL;//事件模块必须依托的连接

   hg_event_handler_pt  connect_handler=NULL;//当有新连接时的回调
   hg_event_handler_pt  read_handler=NULL;//置给新连接的回调方法
   hg_event_handler_pt  write_handler=NULL;

   hg_listen_t *next=NULL;//下一个对象

};

typedef int (*hg_time_handler_pt)(void*);


struct cris_rbtree_t;

struct hg_epoll_ctx_t{

   int epfd=0;
   struct epoll_event *event=NULL;//用于接收事件的数组 
   int max_event_num=0;
   int wait_time=0;
   bool held_accept_mutex=false;//持锁标志
   int    max_connections_num=0;//最大连接数量
   int    cur_connections_num=0;//当前连接数量
 
   cris_rbtree_t  *timetree=NULL;
   unsigned long long id=0;//
   unsigned long long cur_msec=0;//当前时间
   timeval  tv;
};



/*
struct hg_time_node_t{
   unsigned long long ID;
   unsigned long long msec;
   hg_event_t *ev=NULL;       
   friend bool operator<(const hg_time_node_t& n1,const hg_time_node_t& n2){
         return   n1.msec^n2.msec?n1.msec<n2.msec:n1.ID<n2.ID;
   }

};//超时事件的结点
*/









#endif




















