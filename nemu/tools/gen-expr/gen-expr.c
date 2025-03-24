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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

// this should be enough
static char buf[65536] = {};
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";

static int buf_pos=0; //当前缓冲区的位置
static int expr_depth=0; //表达式深度
static int MAX_DEPTH=10;//最大的深度

static uint32_t choose(uint32_t n){
  return rand()%n;
}
//向buf添加字符
static void append_to_buf(const char *str){
  if (buf_pos+strlen(str)<sizeof(buf)-1)
  {
    strcat(buf,str);
    buf_pos+=strlen(str);
  }
  
}

//生成随机数
static void gen_num(int avoid_zero){
  char num[12];
  unsigned value=rand()%100;
  if(avoid_zero || value==0) value=1; //避免除0
  sprintf(num,"%u",value);
  append_to_buf(num);
}


//生成随机操作符
static char gen_rand_op(){
  static const char ops[]="+-*/";
  return ops[choose(4)];
}

static void gen_rand_expr() {

  if (buf_pos>60000)
  {
    return; //防止缓冲区溢出
  }
  if (expr_depth>=MAX_DEPTH){
    gen_num(0);
    return;
  }
  expr_depth++;
  switch (choose(3))
  {
  case 0: //生成随机数
    gen_num(0);
    break;
  case 1://生成一个括号表达式
  append_to_buf("(");
  gen_rand_expr();
  append_to_buf(")");
  break;
  
  default: //生成二元运算表达式
    gen_rand_expr();
    char op=gen_rand_op();
    append_to_buf(" ");
    append_to_buf(&op);
    append_to_buf(" ");
    if (op == '/')
    {
      gen_num(1);
    }else{
      gen_rand_expr();
    }
    break;
  }
  expr_depth--;
}


int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {
      buf[0]='\0';
      buf_pos=0;
      gen_rand_expr();

    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc /tmp/.code.c -o /tmp/.expr");
    if (ret != 0) continue;

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    int result;
    ret = fscanf(fp, "%d", &result);
    pclose(fp);

    printf("%u %s\n", result, buf);
  }
  return 0;
}
