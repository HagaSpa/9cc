#include "9cc.h"

// 9cc.hに宣言されたグローバル変数をここで定義
char *user_input;
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

// str.strndupと同様の振る舞いをするメソッド
char *my_strndup(char *p, int len) {
  char *buf = malloc(len + 1);
  strncpy(buf, p, len);
  buf[len] = '\0';
  return buf;
}

// 次のトークンが期待する記号の時は、トークンを1つ進めてtrueを返す。
// それ以外はfalseを返す
bool consume(char *op) {
  if (token->kind != TK_RESERVED || strlen(op) != token->len || 
        memcmp(token->str, op, token->len))
      return false;
  token = token->next;
  return true;
}

// TK_IDENTを確認する
Token *consume_ident() {
  if (token->kind != TK_IDENT)
    return NULL;
  Token *t = token;
  token = token->next;
  return t;
}

// 次のトークンが期待する記号の時は、トークンを1つ読み進める。
// それ以外の場合はエラーを報告する。
void expect(char *op) {
  if (token->kind != TK_RESERVED || strlen(op) != token->len || 
        memcmp(token->str, op, token->len))
    error_at(token->str, "expected \"%s\"", op);
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
static Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  tok->len = len;
  cur->next = tok;
  return tok;
}

// pとqが同じ文字列ならTrue, 違ければFalse
static bool startswith(char *p, char *q) {
  return memcmp(p, q, strlen(q)) == 0;
}

// アルファベットならTrue
static bool is_alpha(char c) {
  return ('a' <= c && c <= 'z') | ('A' <= c && c <='z') | c == '_';
}

// アルファベットか数字
static bool is_alnum(char c) {
  return is_alpha(c) | ('0' <= c && c <= '9');
}

static char *starts_with_reserved(char *p) {
  // キーワード
  static char *kw[] = {"return", "if", "else", "while"};

  for (int i = 0; i < sizeof(kw) / sizeof(*kw); i++) {
    int len = strlen(kw[i]);
    if (startswith(p, kw[i]) && !is_alnum(p[len]))
      return kw[i];
  }

  // 2文字の演算子
  static char *ops[] = {"==", "!=", "<=", ">="};

  for (int i = 0; i < sizeof(ops) / sizeof(*ops); i++)
    if (startswith(p, ops[i]))
      return ops[i];

  return NULL;
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

    // キーワード or 2文字の演算子かどうか判定
    char *kw = starts_with_reserved(p);
    if (kw) {
      int len = strlen(kw);
      cur = new_token(TK_RESERVED, cur, p, len);
      p += len;
      continue;
    }

    // 1文字の区切り文字の場合
    if (ispunct(*p)) {
      cur = new_token(TK_RESERVED, cur, p++, 1);
      continue;
    }

    // 1文字の変数
    if (is_alpha(*p)) {
      char *q = p++;
      while (is_alnum(*p))
        p++;
      cur = new_token(TK_IDENT, cur, q, p - q);
      continue;
    }

    // 数値の場合
    if (isdigit(*p)) {
      cur = new_token(TK_NUM, cur, p, 0);
      char *q = p;
      cur->val = strtol(p, &p, 10);
      cur->len = p - q;
      continue;
    }

    error_at(p, "invalid token");
  }

  new_token(TK_EOF, cur, p , 0);
  return head.next;
}