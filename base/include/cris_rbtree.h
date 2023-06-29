#ifndef CRIS_RBTREE
#define CRIS_RBTREE
#include<cstddef>
#include<cstdio>
#define R  0
#define B  1

typedef int (*cris_decite_pt)(void*,void*);
typedef void (*cris_print_pt)(void*);

struct cris_rbtree_node_t{
   cris_rbtree_node_t *parent=NULL;
   cris_rbtree_node_t *left=NULL;
   cris_rbtree_node_t *right=NULL;
   short color=R;
   void  *data=NULL;
};



struct cris_rbtree_t{

   cris_rbtree_node_t *root=NULL;
   void *first=NULL;
   cris_decite_pt      decite=NULL;
   int  size=0;

void rotationL(cris_rbtree_node_t *node);
void rotationR(cris_rbtree_node_t *node);
void rotationLR(cris_rbtree_node_t *node);
void rotationRL(cris_rbtree_node_t *node);

void balanceR(cris_rbtree_node_t *node);
void balanceB(cris_rbtree_node_t *node);

cris_rbtree_node_t* _find(void *data);
void  rb_free(cris_rbtree_node_t *node);

void  insert(void *data);
void  erase(void *data);

void  print(cris_rbtree_node_t *r,cris_print_pt pt);

cris_rbtree_t(cris_decite_pt dc):decite(dc){}
~cris_rbtree_t();

void* find(void*data){
     cris_rbtree_node_t *n=_find(data);
     return n==NULL?n->data:NULL;
}

};



void cris_rbtree_t::print(cris_rbtree_node_t *r,cris_print_pt pt){
 
     if(r==NULL)
       return;
     print(r->left,pt);
     pt(r->data);
     print(r->right,pt);
     return;
}

void cris_rbtree_t::rb_free(cris_rbtree_node_t *node){

     if(node==NULL)
        return;
     rb_free(node->left);
     rb_free(node->right);
     delete node;
     return;
}


cris_rbtree_t::~cris_rbtree_t(){

    rb_free(root);
    return;
}


//实际上不会对传入节点进行操作，更不会改变其子树黑高，默认传入节点为黑
void cris_rbtree_t::balanceB(cris_rbtree_node_t *node){
 
     if(node==root)
       return;

     cris_rbtree_node_t *P=node->parent;
     cris_rbtree_node_t *X=node==P->left?P->right:P->left;//兄弟节点必不为空

     if(P->color==R){
     //在红父的情况下，兄弟一定是黑色的
          if(node==P->left){

               if(X->left==NULL||X->left->color==B){
 
                    rotationL(P);

               }else{//兄弟左节点为红色

                    rotationRL(X);

                    P->color=B;

                    balanceR(P->parent);//实际上是旋转前的，兄弟的左节点,处理红冲突

               }
 
          }else{

               if(X->right==NULL||X->right->color==B){

                    rotationR(P);

               }else{

                    rotationLR(X);

                    P->color=B;

                    balanceR(P->parent);
               }

          }


     }else{
     //黑父，需要讨论兄弟的颜色

          if(X->color==R){

               if(node==P->left){

                   rotationL(P);
                   P->color=R;
                   X->color=B;
                   balanceB(node);//一定会转化为红父，黑兄的情况

               }else{

                   rotationR(P);
                   P->color=R;
                   X->color=B;
                   balanceB(node);
               }


          }else{

               if(node==P->left){

                   if(X->left==NULL||X->left->color==B){

                       rotationL(P);
                       P->color=R;
                       balanceB(X);

                   }else{
                       //该情况下，必定终结调整
                       rotationRL(X);
                       X->parent->color=B;//原其左儿子    
                   }

               }else{

                   if(X->right==NULL||X->right->color==B){

                       rotationR(P);
                       P->color=R;
                       balanceB(X);

                   }else{

                       rotationLR(X);
                       X->parent->color=B;
                   }
               }

        }

    }

}


void cris_rbtree_t::erase(void *data){

     cris_rbtree_node_t *del=_find(data);
     cris_rbtree_node_t *src=del;
     if(del==NULL)
       return;
  
     size--;

     if(del->left!=NULL&&del->right!=NULL){//双子均在，则找替身

         cris_rbtree_node_t *realdel=del->right;
 
         while(realdel->left!=NULL)
              realdel=realdel->left;
     
         del->data=realdel->data;
         del=realdel;
     }

     //替身一定是叶子节点或者单子节点

     if(decite(data,first)==0){//该操作不会与找替身重合，且是找后继的必经操作，可以降低找first的平均树深

         if(src->right!=NULL){
             
             cris_rbtree_node_t *tmp=src->right;
             while(tmp->left!=NULL) 
                tmp=tmp->left;
             first=tmp->data;

         }else if(src->parent!=NULL){
              
             first=src->parent->data;

         }else{
        
             first=NULL;//该节点必定为根              

         }
     }

     if(del->color==R){

          cris_rbtree_node_t *P=del->parent;//红节点，父一定不为空
          cris_rbtree_node_t *C=del->left==NULL?del->right:del->left;//包含没有孩子的情况

          if(C!=NULL)
             C->parent=P;

          if(del==P->left)
              P->left=C;
          else
              P->right=C;

          delete del;

          return;

     }else{

          balanceB(del);
     
          cris_rbtree_node_t *C=del->left==NULL?del->right:del->left;    
          cris_rbtree_node_t *P=del->parent;

         if(C!=NULL)
           C->parent=P;

         if(P==NULL){
            if(C!=NULL)
              C->color=B;
            root=C;
         }else{

            if(del==P->left)
              P->left=C;
            else
              P->right=C;
         }

         delete del;
    }
}



void  cris_rbtree_t::insert(void *data){

      cris_rbtree_node_t *it=new cris_rbtree_node_t();
      it->data=data;
      size++; 

      if(root==NULL){
         root=it;
         first=it->data;
         it->color=B;
         return;        
      }
 
      cris_rbtree_node_t *parent=NULL;
      cris_rbtree_node_t *cur=root;
      int     p=-1;

      do{

          p=decite(data,cur->data);

          if(p==0)//重复插入
            return;

          parent=cur;
          
          if(p<0)
            cur=cur->left;
          else
            cur=cur->right;
      }while(cur!=NULL);

      if(p<0){

        parent->left=it;
        if(decite(parent->data,first)==0)
           first=it->data;

      }else
        parent->right=it;

      it->parent=parent;
     
      balanceR(it);
}

cris_rbtree_node_t* cris_rbtree_t::_find(void *data){

      cris_rbtree_node_t *cur=root;
   
      int p;

      while(cur!=NULL){
        
           p=decite(data,cur->data);

           if(p<0)
             cur=cur->left;
           else if(p==0)
             break;
           else
             cur=cur->right;
      }
      return cur;
}



void cris_rbtree_t::rotationL(cris_rbtree_node_t *node){
      
     cris_rbtree_node_t *X=node->right;
     cris_rbtree_node_t *XL=X->left;
     cris_rbtree_node_t *P=node->parent;

     node->right=XL;
     X->left=node;
     node->parent=X;

     if(XL!=NULL)
       XL->parent=node;


     X->parent=P;

     if(P==NULL){
        root=X;
     }else{

        if(node==P->left)
            P->left=X;
        else
            P->right=X;
     }

}

void cris_rbtree_t::rotationR(cris_rbtree_node_t *node){

    cris_rbtree_node_t *L=node->left;
    cris_rbtree_node_t *LR=L->right;
    cris_rbtree_node_t *P=node->parent;

    node->left=LR;
    L->right=node;
    node->parent=L;

    if(LR!=NULL)
      LR->parent=node;

    L->parent=P;

    if(P==NULL){
      root=L;
    }else{
 
      if(node==P->left)
         P->left=L;
      else
         P->right=L;
    }

}


void cris_rbtree_t::rotationLR(cris_rbtree_node_t *node){

     cris_rbtree_node_t *P=node->parent;
     rotationL(node);
     rotationR(P);
}

void cris_rbtree_t::rotationRL(cris_rbtree_node_t *node){

     cris_rbtree_node_t *P=node->parent;
     rotationR(node);
     rotationL(P);
}

//调整不会影响传入节点的子节点的黑高或者产生红冲突
void cris_rbtree_t::balanceR(cris_rbtree_node_t *node){

     if(node==root){
         node->color=B;
         return;
     }

     cris_rbtree_node_t *P=node->parent;

     if(P->color==B)
        return;

     cris_rbtree_node_t *GP=P->parent;
     cris_rbtree_node_t *U=P==GP->left?GP->right:GP->left;

     if(U==NULL){//叔叔节点不存在

         if(P==GP->left){

             if(node==P->left){
                  rotationR(GP);
                  GP->color=R;
                  P->color=B;
             }else{
                  rotationLR(P);
                  GP->color=R;
                  P->color=R;
                  node->color=B;
             }

         }else{

             if(node==P->right){
                  rotationL(GP);
                  GP->color=R;
                  P->color=B;
             }else{
                  rotationRL(P);
                  GP->color=R;
                  P->color=R;
                  node->color=B;
             }

         }

     }else{

        if(P==GP->left){

            if(U->color==B){

                 if(node==P->left){

                     rotationR(GP);
                     GP->color=R;
                     P->color=B;
                 }else{

                     rotationLR(P);
                     GP->color=R;
                     node->color=B;
                 }

            }else{//这一步中只染色即可，没有旋转，所以不考虑分节点的左右

                 GP->color=R;
                 P->color=B;
                 U->color=B;
                 balanceR(GP);                     
            }


        }else{
 
            if(U->color==B){

                 if(node==P->right){

                    rotationL(GP);
                    GP->color=R;
                    P->color=B;
                 }else{

                    rotationRL(P);
                    GP->color=R;
                    node->color=B;
                 }


            }else{

                 GP->color=R;
                 P->color=B;
                 U->color=B;
                 balanceR(GP);
            }

        }

     }

}



#endif


















