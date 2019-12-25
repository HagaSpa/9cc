#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * tokenize.c
 */

// tokenの種類
// enum型にTokenKindという名前でエイリアスしている
typedef enum {
  TK_RESERVED, // 演算子か記号
  TK_IDENT,    // 識別子
  TK_NUM,      // 数値
  TK_EOF,      // 入力の終わりを表すトークン
} TokenKind;

typedef struct Token Token;
// トークン型
struct Token {
  TokenKind kind; // トークンの型
  Token *next;    // 次の入力トークン
  int val;        // kindがTK_NUMの場合、その数値
  char *str;      // トークン文字列
  int len;        // トークンの長さ
};

void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
void error_tok(Token *tok, char *fmt, ...);
Token *consume(char *op);
char *my_strndup(char *p, int len);
Token *consume_ident();
void expect(char *op);
int expect_number(void);
char *expect_ident(void);
bool at_eof(void);
Token *tokenize(void);

// 入力プログラム
extern char *user_input;

// 現在着目しているトークン
extern Token *token;


/**
 * parse.c 
 */

// Local Variable
typedef struct Var Var;
struct Var {
  char *name; // 変数名
  int offset; // RBPからのオフセット
};

typedef struct VarList VarList;
struct VarList {
  VarList *next;
  Var *var;
};

// 抽象構文木のノードの種類
typedef enum {
  ND_ADD,       // +
  ND_SUB,       // -
  ND_MUL,       // *
  ND_DIV,       // /
  ND_EQ,        // ==
  ND_NE,        // !=
  ND_LT,        // <
  ND_LE,        // <=
  ND_ASSIGN,    // =
  ND_ADDR,      // 単項式での"&"
  ND_DEREF,     // 単項式での"*"
  ND_VAR,       // Variable
  ND_EXPR_STMT, // 式ステートメント
  ND_RETURN,    // "return"
  ND_IF,        // "if"
  ND_WHILE,     // "while"
  ND_FOR,       // "for"
  ND_BLOCK,     // 複文（compound statement） {...}
  ND_FUNCALL,   // 関数呼び出し
  ND_NUM,       // 整数
} NodeKind;

typedef struct Node Node;

// 抽象構文木のノードの型
struct Node{
  NodeKind kind; // ノードの型
  Node *next;    // 次のノード
  Node *lhs;     // 左辺（left-hand side）
  Node *rhs;     // 右辺（right-hand side）
  Token *tok;    // repesentative token

  // if or while or forの場合使用するノード
  Node *cond;   // 条件
  Node *then;   // trueの時 
  Node *els;    // falseの時
  Node *init;   // forのカウンタ変数
  Node *inc;    // forのインクリメント変数
  Node *body;   // 複文の場合に使う {...}

  // 関数を呼び出している場合に使う
  char *funcname;
  Node *args;

  Var *var;      // kindがND_VARの場合のみ使う
  int val;       // kindがND_NUMの場合のみ使う
};

typedef struct Function Function;
struct Function {
  Function *next;
  char *name;
  VarList *params;
  Node *node;
  VarList *locals;
  int stack_size;
};

Function *program(void);

/**
 * codegen.c
 */

 void codegen(Function *prog);