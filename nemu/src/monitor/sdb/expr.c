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

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include "sdb.h"
#include "../../nemu/include/memory/paddr.h"

enum {
  TK_NOTYPE = 256, TK_EQ,TK_LPAREN,TK_RPAREN,TK_PLUS,TK_MINUS,TK_MULTIPLY,TK_DIVIDE,TK_NUMBER,TK_NEGATIVE,TK_NEQ,TK_AND,TK_REG,TK_DEREF

  /* TODO: Add more token types */

};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"0x[0-9a-zA-Z]+",TK_NUMBER}, //16进制数
  {"([0-9]+\\.[0-9]*|[0-9]+)",TK_NUMBER}, //number
  {"\\+", TK_PLUS},         // plus
  {"-", TK_MINUS},         // minus
  {"\\*", TK_MULTIPLY},         //multipy
  {"\\/", TK_DIVIDE},         // divide
  {"\\(",TK_LPAREN},    //left括号
  {"\\)",TK_RPAREN},    //right括号
  {"==", TK_EQ},        // equal
  {"!=", TK_NEQ},        //不等于
  {"&&",TK_AND},       //逻辑与
  {"\\$[a-zA-Z0-9]+", TK_REG},        //寄存器访问
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[6000] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;
  //tokens[]={0,'\0'};

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
          //K_EQ,TK_LPAREN,TK_RPAREN,TK_PLUS,TK_MINUS,TK_MULTIPLY,TK_DIVIDE,TK_NUMBER
          case TK_NUMBER:{
            tokens[nr_token].type=TK_NUMBER;
            strncpy(tokens[nr_token].str,substr_start,substr_len);
            tokens[nr_token].str[substr_len]='\0';
          };break;
          case TK_REG:{
            tokens[nr_token].type=TK_REG;
            strncpy(tokens[nr_token].str,substr_start+1,substr_len-1);
            tokens[nr_token].str[substr_len-1]='\0';
          };break;
            
          case TK_PLUS: tokens[nr_token].type=TK_PLUS;break;
          case TK_MINUS: 
            if (nr_token==0||tokens[nr_token-1].type==TK_LPAREN||tokens[nr_token-1].type==TK_PLUS||tokens[nr_token-1].type==TK_MINUS||tokens[nr_token-1].type==TK_MULTIPLY||tokens[nr_token-1].type==TK_DIVIDE)
            {
              tokens[nr_token].type=TK_NEGATIVE;
            }else{
              tokens[nr_token].type=TK_MINUS;
            }
            break;
          case TK_MULTIPLY: 
          if (nr_token==0||tokens[nr_token-1].type==TK_LPAREN||tokens[nr_token-1].type==TK_PLUS||tokens[nr_token-1].type==TK_MINUS||tokens[nr_token-1].type==TK_MULTIPLY||tokens[nr_token-1].type==TK_DIVIDE)
            {
              tokens[nr_token].type=TK_DEREF;
            }else{
              tokens[nr_token].type=TK_MULTIPLY;
            }
          break;
          case TK_DIVIDE: tokens[nr_token].type=TK_DIVIDE;break;
          case TK_LPAREN: tokens[nr_token].type=TK_LPAREN;break;
          case TK_RPAREN: tokens[nr_token].type=TK_RPAREN;break;
          case TK_EQ: tokens[nr_token].type=TK_EQ;break;
          case TK_NEQ: tokens[nr_token].type=TK_NEQ;break;
          case TK_AND: tokens[nr_token].type=TK_AND;break;
          default:break;
        }
        nr_token++;
        break;
      }

    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
    
    
  }
  //打印tokens里面识别的标识符
  /*
  for (int i = 0; i < nr_token; i++)
  {
    printf("token:%d,data:%s\n",tokens[i].type,tokens[i].str);
  }
*/
  return true;
}

//check_parentheses检查括号是否匹配

static bool check_parentheses(int p,int q){
  if(tokens[p].type!=TK_LPAREN||tokens[q].type!=TK_RPAREN){
    return false;
  }
  int balance=0; //用于检查括号是否匹配
  for(int i=p;i<=q;i++){
    if(tokens[i].type==TK_LPAREN){
      balance++;
    }else if(tokens[i].type==TK_RPAREN){
      balance--;
      if(balance<0){
        return false;
      }
    }
  }
  return balance==0; //括号匹配
}


//eval函数，递归计算表达式的值--p 5+4*-3/2-1 4294967293
static int eval(int p, int q, bool *success) {
  if (p > q) {
      *success = false; // 表达式不合法
      return 0;
  } else if (p == q) {
      // 单个 token，必须是数字
      if (tokens[p].type == TK_NUMBER) {
          // 使用 strtol 解析数字
          char *endptr;
          long val = strtol(tokens[p].str, &endptr, 0);
          if (*endptr != '\0') {
              *success = false; // 解析失败，表达式不合法
              return 0;
          }
          return (int)val; // 返回解析后的数字
      }else if (tokens[p].type == TK_REG)
      {
        //获取寄存器的值
        word_t reg_val=isa_reg_str2val(tokens[p].str,success);
        if (!*success)
        {
          return 0;
        }
        return (int)reg_val;
        
      }else {
          *success = false; // 不是数字，表达式不合法
          return 0;
      }
  } else if (check_parentheses(p, q)) {
      // 表达式被一对匹配的括号包围，去掉括号后递归计算
      return eval(p + 1, q - 1, success);
  } else {
      // 寻找主运算符
      int op_pos = -1; // 主运算符的位置
      int balance = 0; // 用于检查括号
      int lowest_priority = 9999; // 最低优先级
      int priority;

      for (int i = p; i <= q; i++) {
          if (tokens[i].type == TK_LPAREN) {
              balance++;
          } else if (tokens[i].type == TK_RPAREN) {
              balance--;
          } else if (balance == 0) {
              // 当前 token 是运算符
              switch (tokens[i].type) {
                  case TK_PLUS:
                  case TK_MINUS:
                      priority = 1; // 加减法的优先级最低
                      break;
                  case TK_MULTIPLY:
                  case TK_DIVIDE:
                      priority = 2; // 乘除法的优先级较高
                      break;
                  case TK_NEGATIVE:
                  case TK_DEREF:
                      priority = 3; // 负号的优先级最高
                      break;
                  case TK_EQ:
                  case TK_NEQ:
                      priority = 0; //关系运算符
                      break;
                  case TK_AND:
                      priority=-1;
                      break;
                  default:
                      priority = 9999; // 不是运算符
                      break;
              }

              // 更新主运算符
              if (priority <= lowest_priority) {
                  lowest_priority = priority;
                  op_pos = i;
              }
          }
      }

      if (op_pos == -1) {
          *success = false; // 没有找到主运算符，表达式不合法
          return 0;
      }

      // 根据主运算符的类型进行处理
      if (tokens[op_pos].type == TK_NEGATIVE) {
          // 负号是单目运算符，只处理右操作数
          int val = eval(op_pos + 1, q, success);
          if (!*success) {
              return 0; // 右操作数不合法
          }
          return -val; // 返回取反后的值
      }else if (tokens[op_pos].type==TK_DEREF)  //指针解析//单目运算
      {
        int addr=eval(op_pos+1,q,success);
        if (!*success) {
          return 0; // 右操作数不合法
       }
       return (int)paddr_read(addr,4); //读取内存

      }
      
       else {
          // 双目运算符，递归计算左右操作数
          int val1 = eval(p, op_pos - 1, success);
          if (!*success) {
              return 0; // 左操作数不合法
          }

          int val2 = eval(op_pos + 1, q, success);
          if (!*success) {
              return 0; // 右操作数不合法
          }

          // 根据主运算符计算最终结果
          switch (tokens[op_pos].type) {
              case TK_PLUS:
                  return val1 + val2;
              case TK_MINUS:
                  return val1 - val2;
              case TK_MULTIPLY:
                  return val1 * val2;
              case TK_DIVIDE:
                  if (val2 == 0) {
                      *success = false; // 除零错误
                      return 0;
                  }
                  return val1 / val2;
              case TK_EQ:
                  return val1==val2;
              case TK_NEQ:
                  return val1!=val2;
              case TK_AND:
                  return val1&&val2;
              
              default:
                  *success = false; // 未知运算符
                  return 0;
          }
      }
  }
}


int expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  //TODO();
  //eval计算表达式
  *success=true;
  int result=eval(0,nr_token-1,success);

  return result;
}
