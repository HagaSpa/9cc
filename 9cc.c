#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// tokenの種類
// enum型にTokenKindという名前でエイリアスしている
typedef enum {
  TK_RESERVED, // 記号
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

    // 記号の場合
    if (*p == '+' || *p == '-') {
      cur = new_token(TK_RESERVED, cur, p++);
      continue;
    }

    // 数値の場合
    if (isdigit(*p)) {
      cur = new_token(TK_NUM, cur, p);
      cur->val = strtol(p, &p, 10);
      continue;
    }

    error_at(p, "expected a number");
  }

  new_token(TK_EOF, cur, p);
  return head.next;
}


/**
 * 与えられた加減算のみの式を、実行するアセンブリを生成する
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

  // アセンブリの前半部分を出力
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");


  // 式の最初は数字じゃなくてはならないので、それをチェック
  // 最初のmov命令を実行
  printf("  mov rax, %d\n", expect_number());

  // `+ <数>`あるいは`- <数>`というトークンの並びを消費しつつ、アセンブリ出力
  while (!at_eof()) {
    if (consume('+')) {
      printf("  add rax, %d\n", expect_number());
      continue;
    }

    expect('-');
    printf("  sub rax, %d\n", expect_number());
  }

  printf("  ret\n");
  return 0;
}