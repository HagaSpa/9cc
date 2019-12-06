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
bool consume(char *op);
void expect(char *op);
int expect_number(void);
bool at_eof(void);
Token *tokenize(void);

// 入力プログラム
extern char *user_input;

// 現在着目しているトークン
extern Token *token;


/**
 * parse.c 
 */

// 抽象構文木のノードの種類
typedef enum {
  ND_ADD, // +
  ND_SUB, // -
  ND_MUL, // *
  ND_DIV, // /
  ND_EQ,  // ==
  ND_NE,  // !=
  ND_LT,  // <
  ND_LE,  // <=
  ND_NUM, // 整数
} NodeKind;

typedef struct Node Node;

// 抽象構文木のノードの型
struct Node{
  NodeKind kind; // ノードの型
  Node *lhs;     // 左辺（left-hand side）
  Node *rhs;     // 右辺（right-hand side）
  int val;       // kindがND_NUMの場合のみ使う
};

Node *expr(void);


/**
 * codegen.c
 */

 void codegen(Node *node);