/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include "sdb.h"

#define NR_WP 32



typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
  char expr[128];
  uint32_t value;

} WP;

WP *new_wp();
void free_wp(WP *WP);

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */
//从free_链表中返回一个空闲的监视点
WP *new_wp(){
  if (free_==NULL)
  {
    assert(0);
  }
  
  //声明暂时的变量
  WP * new_wp=free_;
  free_=free_->next;
  new_wp->next=NULL;
  return new_wp;
  
}

//将使用完的监视点返回
void free_wp(WP *WP){
  WP->next=free_;
  free_=WP;
}

//给定表达式创建空的监视点
void create_wp(char *expression){
  WP *wp=new_wp();
  if (wp==NULL)
  {
    printf("NO watchpoints available.\n");
    return;
  }
  //long int n = strlen(expression);
  snprintf(wp->expr, sizeof(wp->expr), "%s", expression);  // 使用 snprintf 复制字符串

  //计算表达式
  bool success;
 uint32_t result=expr(expression, &success);
  //打印识别的标识符
 if (!success)
 {
  printf("invalid expression:%s\n",expression);
  free_wp(wp);
 }

  wp->value=result;
  //将表达式放进head
  wp->next=head;
  head=wp;
  
}

void check_wp(){
  WP *wp=head;
  while (wp!=NULL)
  {
    //计算表达式
  bool success;
  uint32_t new;
  new=expr(wp->expr, &success);
  //打印识别的标识符
  if (!success)
   {
    printf("error expression:%s\n",wp->expr);
    wp=wp->next;
    continue;
    
  }
  if (new!=wp->value)
  {
    printf("watch %d triggered:%s\n",wp->NO,wp->expr);
    printf("old value:%u,new value:%u\n",wp->value,new);
    wp->value=new;
    extern NEMUState nemu_state;1
    nemu_state.state=NEMU_STOP;
  }
  
  wp=wp->next;

  }
  
}


void delete_wp(int NO){
  WP *prev=NULL;
  WP *curr=head;
  while (curr!=NULL)
  {
    if (curr->NO==NO)
    {
      if (prev==NULL)
      {
        head=curr->next;
      }else{
        prev->next=curr->next;
      }
      free_wp(curr);
      printf("watchpoint %d deleted\n",NO);
      return;
    }
    prev=curr;
    curr=curr->next;
    
  }
  printf("watchpoint %d not found\n",NO);
  
}

void print_wp(){
  WP *wp=head;
  if (wp==NULL)
  {
    printf("no watchpoints\n");
  }
  printf("watch:\n");
  while (wp !=NULL)
  {
    printf("%d:%s=%u\n",wp->NO,wp->expr,wp->value);
    wp=wp->next;
  }
  
}