#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Tokenizer 
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
};

// 入力プログラム
char *user_input;

// 現在着目しているトークン
Token *token;

// エラーを報告する関数
void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// エラー箇所を報告する
void error_at(char *loc, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  int pos = loc - user_input;
  fprintf(stderr, "%s\n", user_input);
  fprintf(stderr, "%*s", pos, ""); // print pos spaces.
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// 次のトークンが期待する記号の時は、トークンを1つ進めてtrueを返す。
// それ以外はfalseを返す
bool consume(char op) {
  // kindがTK_RESERVEDじゃない場合 or 一番最初の文字じゃない場合。
  // 「+5-4]みたいな式の場合「-」のみ？
  if (token->kind != TK_RESERVED || token->str[0] != op)
    return false;
  token = token->next;
  return true;
}

// 次のトークンが期待する記号の時は、トークンを1つ読み進める。
// それ以外の場合はエラーを報告する。
void expect(char op) {
  if (token->kind != TK_RESERVED || token->str[0] != op)
    error_at(token->str, "expected '%c'", op);
  token = token->next;
}

// 次のトークンが数値の時は、トークンを1つ読み進めてその数値を返す。
// それ以外の場合はエラーを報告する。
int expect_number() {
  if (token->kind != TK_NUM) {
    error_at(token->str, "expected a number");
  }
  int val = token->val;
  token = token->next;
  return val;
}

// EOFかどうか
bool at_eof() {
  return token->kind == TK_EOF;
}

// 新しいトークンを作成してcurに繋げる
Token *new_token(TokenKind kind, Token *cur, char *str) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  cur->next = tok;
  return tok;
}

// 入力文字列（user_input）をトークナイズして、新しいトークンを返却する
Token *tokenize() {
  char *p = user_input;
  // 最初のトークンを初期化
  Token head;
  head.next = NULL;
  Token *cur = &head;

  while (*p) {
    // 空白の場合読み飛ばす
    if (isspace(*p)) {
      p++;
      continue;
    }

    // 演算子か記号の場合
    if (strchr("+-*/()", *p)) {
      cur = new_token(TK_RESERVED, cur, p++);
      continue;
    }

    // 数値の場合
    if (isdigit(*p)) {
      cur = new_token(TK_NUM, cur, p);
      cur->val = strtol(p, &p, 10);
      continue;
    }

    error_at(p, "invalid token");
  }

  new_token(TK_EOF, cur, p);
  return head.next;
}


/**
 * Parser 
 */

// 抽象構文木のノードの種類
typedef enum {
  ND_ADD, // +
  ND_SUB, // -
  ND_MUL, // *
  ND_DIV, // /
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

Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

Node *new_node_num(int val) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_NUM;
  node->val = val;
  return node;
}

Node *expr();
Node *mul();
Node *unary();
Node *primary();

// expr = mul ("+" mul | "-" mul)*
Node *expr() {
  Node *node = mul();

  for (;;) {
    if (consume('+'))
      node = new_node(ND_ADD, node, mul());
    else if (consume('-'))
      node = new_node(ND_SUB, node, mul());
    else
      return node;
  }
}

// mul = unary ("*" unary | "/" unary)*
Node *mul() {
  Node *node = unary();

  for (;;) {
    if (consume('*'))
      node = new_node(ND_MUL, node, unary());
    else if (consume('/'))
      node = new_node(ND_DIV, node, unary());
    else
      return node;
  }
}

// unary = ("+" | "-")? unary | primary
Node *unary() {
  if (consume('+'))
    return unary();
  if (consume('-'))
    // 負の数の場合は、左辺に0を入れて0-xとして表現
    return new_node(ND_SUB, new_node_num(0), unary());
  return primary();
}

// primary = num | "(" expr ")"
Node *primary() {
  // 次のトークンが"("なら、"(" expr ")"のはず
  if (consume('(')) {
    Node *node = expr();
    expect(')');
    return node;
  }

  // そうでなければ数値
  return new_node_num(expect_number());
}


/**
 * Code generator 
 */

void gen(Node *node) {
  if (node->kind == ND_NUM) {
    printf("  push %d\n", node->val);
    return;
  }

  gen(node->lhs); // 左辺に対してgen()を再帰呼出
  gen(node->rhs); // 右辺に対してgen()を再帰呼出

  printf("  pop rdi\n"); // スタックの先頭をrdiへpop（内部ではその後rspが保持するアドレスを変更）
  printf("  pop rax\n"); // スタックの先頭をrazへpop

  switch (node->kind) {
  case ND_ADD:
    printf("  add rax, rdi\n");
    break;
  case ND_SUB:
    printf("  sub rax, rdi\n");
    break;
  case ND_MUL:
    printf("  imul rax, rdi\n");
    break;
  case ND_DIV:
    printf("  cqo\n");
    printf("  idiv rdi\n");
    break;
  }

	printf("  push rax\n");
}


/**
 * 与えられた式を、実行するアセンブリを生成する
 * 
 * argc: コマンドライン引数の個数
 * **argv: argvはコマンドライン引数を格納した配列
 *          **argvなので配列のポインタ変数のポインタ？
 */
int main(int argc, char **argv) {
  if (argc != 2) {
    error("%s: invalid number of arguments", argv[0]);
    return 1;
  }

  // トークナイズ実行
  user_input = argv[1];
  token = tokenize();
  Node *node = expr();

  // アセンブリの前半部分を出力
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");

  // 抽象構文木を下りながらコード生成
  gen(node);

  // スタックの最後に式全体の値が残っているので
  // それをRAXにロードして関数からの返却値とする
  printf("  pop rax\n");
  printf("  ret\n");
  return 0;
}