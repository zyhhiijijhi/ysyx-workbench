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

#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"
#include "../../nemu/include/memory/paddr.h"

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();
//static bool make_token(char *e);

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  //nemu_state.state =NEMU_QUIT;
  return -1;
}

static int cmd_s(char *args) {
  int n;
  if (args!=NULL)
  {
    n=atoi(args);
  }else{
    n=1;
  }
  
  cpu_exec(n);
  return 0;
}

static int cmd_info_r(char *args) {
//打印寄存器的值
printf("start print reg\n");
 isa_reg_display();
  return 0;
}

static int cmd_x(char *args) {
  //扫描寄存器
  //printf("args:%s\n",args);
   if (args==NULL)
   {
     printf("usage :x <len> <addr>\n");
   }
   //printf("args:%s\n",args);
   int length;
   vaddr_t addr;
   if (sscanf(args,"%d 0x%x",&length,&addr)!=2)
   {
     printf("invalid argument\n");
     return 0;
   }
   for (int i = 0; i <length; i++)
   {
     uint32_t data=paddr_read(addr+i*4,4); //读取4字节
     printf("0x%08x:0x%08x\n",addr+i*4,data);
   }
   
   
    return 0;
  }



void remove_whitespace(char *str) {
  int i = 0, j = 0;
  while (str[i]) {
    if (!isspace((unsigned char)str[i])) { // 检查是否为空白字符
      str[j++] = str[i];
    }
    i++;
  }
  str[j] = '\0'; // 字符串末尾添加 null 终止符
}


void test_expr(){

  /**
 * 随机测试表达式  过程:1.打开文件读取 2.解析每行内容 3.填充表达式 4.比较结果
 */
 FILE *fp=fopen("/home/ni/ysyx-workbench/nemu/tools/gen-expr/input","r");
 if(fp==NULL){
   printf("open file failed\n");
   return ;
 }
 char buf[65536];
 int line_num=0;
 int pass=0;
 int fail=0;

 //读取内容并解析
 while (fgets(buf,65536,fp)!=NULL)
 {
   line_num++;
 
   unsigned result;
   char expr_str[65536];
   //printf("buf:%s\n",buf);
   sscanf(buf,"%u",&result);

   //提取表达式：
   char *expr_start=strchr(buf,' '); //找到第一个空的字符
   if (expr_start!=NULL)
   {
     expr_start++; //跳过空格
     strncpy(expr_str,expr_start,sizeof(expr_str)-1);
     expr_str[sizeof(expr_str)-1]='\0';
   }else{
     expr_str[0]='\0';
   }
   //printf("result:%u,expr_str:%s\n",result,expr_str);

   //计算表达式
   bool success;
   remove_whitespace(expr_str);
   int result2=expr(expr_str,&success);
   //结果对比
   if (!success)
   {
     printf("line %d:expression invalid\n",line_num);
     fail++;
   }else if ((int)result!=result2)
   {
     printf("line %d:expression result is %d,correct result is %d expression:%s\n",line_num,result2,result,buf);
     fail++;
   }else{
     pass++;
   }
   
   
 }
 //关闭文件
 fclose(fp);
 //输出测试结果
 printf("总数目是:%d,pass:%d,fail:%d\n",line_num,pass,fail);


}

//用于调试表达式求值
static int cmd_p(char *args) {
  init_regex();
 
  
  //计算p 表达式这样的形式。
  //printf("args:%s\n",args);
  //打印匹配到的token值
  //make_token("5+4*3/2-1");
  bool success;
  int result;
  //result=expr("(((71 / 1 / 1 + (((((7))))))) + 28)", &success);
  result=expr(args, &success);
  //打印识别的标识符
 if (success)
 {
    printf("result:%d\n",result);
 }else{
    printf("expression is invalid\n");
 }


  //生成随机测试表达式
  //test_expr();

  return  success;
  //return 0;
};

int cmd_w (char *args){
  if (args==NULL)
  {
    printf("usage :w <expression>\n");
    return 0;
  }
  create_wp(args);
  return 0;
}

int cmd_d (char *args){
  if (args==NULL)
  {
    printf("usage :d NO\n");
    return 0;
  }
  int no=atoi(args);
  if (no<0)
  {
    printf("invalid watchpoints no:%d\n",no);
  }
  delete_wp(no);
  return 0;
}

int cmd_info_w(char *args){
  print_wp();
  return 0;
}


static int cmd_help(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c},
  { "q", "Exit NEMU", cmd_q },

  /* TODO: Add more commands */
  {"s","step execution of the program",cmd_s},
  {"info r","step execution of the program",cmd_info_r},
  {"x","scan memory",cmd_x},
  { "p", "print varibale value", cmd_p},
  {"w","watch point",cmd_w},
  {"d","delete watch point",cmd_d},
  {"info w" ,"print watch point",cmd_info_w},
};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  //输入的命令是：p 4+3*(2-1)
  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }
    char *args =strtok(NULL," ");
    //info命令特殊，后面的参数属于命令
    if (strcmp(cmd,"info")==0)
    {
      if (strcmp(args,"r")==0)
      {
        char full_cmd[56];
        strncpy(full_cmd,cmd,sizeof(full_cmd)-1);
        strncat(full_cmd," ",1);
        strncat(full_cmd,args,1);
        //printf("cmd:%s\n",full_cmd);
        cmd=full_cmd;
      }else if (strcmp(args,"w")==0)
      {
        char full_cmd[56];
        strncpy(full_cmd,cmd,sizeof(full_cmd)-1);
        strncat(full_cmd," ",1);
        strncat(full_cmd,args,1);
        printf("cmd:%s\n",full_cmd);
        cmd=full_cmd;
      }
      
    }else if (strcmp(cmd,"x")==0)
    {
      //处理args
      char full_args[56];
      strncpy(full_args,args,sizeof(full_args)-1);
      strncat(full_args," ",1);
      args =strtok(NULL," ");
      strncat(full_args,args,sizeof(full_args)-10);
      args=full_args;
      //printf("full_args:%s\n",full_args);
    }
    

    //printf("cmd:%s\n",cmd);
    //printf("args:%s\n",args);
    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    /*if(cmd=='p'){

      char *args;
      int len=0,i=1;
      while((cmd+strlen(cmd)+i)!='\0'){
        len++;
        i++;
      }
      strcpy(args,cmd + strlen(cmd),len);

    }else{
      char *args = cmd + strlen(cmd) + 1;
    }*/
    if (args >= str_end) {
      //args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
