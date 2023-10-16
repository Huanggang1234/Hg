
#########################################



1.上游模块的使用

   （1）首先实现hg_upstream_create_request(cris_buf_t **out,void*data,hg_upstream_info_t *info) 
函数

        a.在该函数内自行创建cris_buf_t对象，在该缓冲内创建上游请求，并挂在*out上，模块会自动
发送该缓冲上的内容，直到cur>=last，该函数可以通过返回HG_AGAIN请求再次调度，返回HG_OK表示整个请求
创建完成进入下一阶段




