#include "9cc.h"

VarList *locals;

// すでに出現した変数があるか、nameを用いて検索する
static Var *find_var(Token *tok) {
  for (VarList *vl=locals; vl; vl=vl->next) {
    Var *var = vl->var;
    if (strlen(var->name) == tok->len && !memcmp(tok->str, var->name, tok->len))
      return var;
  }
  return NULL; 
}

// ノード生成における共通部分
static Node *new_node(NodeKind kind, Token *tok) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->tok = tok;
  return node;
}

// ノードを生成する（lhsもrhdも非終端記号）
static Node *new_node_binary(NodeKind kind, Node *lhs, Node *rhs, Token *tok) {
  Node *node = new_node(kind, tok);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

// 数字のノード（終端記号のため、このノードの次はない）
static Node *new_node_num(int val, Token *tok) {
  Node *node = new_node(ND_NUM, tok);
  node->val = val;
  return node;
}

static Node *new_node_unary(NodeKind kind, Node *expr, Token *tok) {
  Node *node = new_node(kind, tok);
  node->lhs = expr;
  return node;
}

// 変数を表すノード
static Node *new_node_var(Var *var, Token *tok) {
  Node *node = new_node(ND_VAR, tok);
  node->var = var;
  return node;
}

// nameの変数をスタックへpushする
// localsにある変数をnextに入れて、*nameを新しいvarのnameへ格納（先入れ先だしを表現）
static Var *push_var(char *name) {
  Var *var = calloc(1, sizeof(Var));
  var->name = name;
  VarList *vl = calloc(1, sizeof(VarList));
  vl->var = var;
  vl->next = locals;
  locals = vl;
  return var;
}

// ()の中の引数をスタックへpushする
static VarList *read_func_params(void) {
  if (consume(")"))
    return NULL;

  VarList *head = calloc(1, sizeof(VarList));
  head->var = push_var(expect_ident());
  VarList *cur = head;

  while (!consume(")")) {
    expect(",");
    cur->next = calloc(1, sizeof(VarList));
    cur->next->var = push_var(expect_ident());
    cur = cur->next;
  }
  return head;
}

static Function *function(void);
static Node *stmt(void);
static Node *expr(void);
static Node *assign(void);
static Node *equality(void);
static Node *relational(void);
static Node *add(void);
static Node *mul(void);
static Node *unary(void);
static Node *primary(void);

// program = function*
Function *program(void) {
  Function head;
  head.next = NULL;
  Function *cur = &head;

  // 終了文字が出るまで
  while (!at_eof()) {
    cur->next = function();
    cur = cur->next;
  }
  return head.next;
}

// function = ident "(" params? ")" "{" stmt* "}"
// params   = ident ("," ident)*
Function *function(void) {
  locals = NULL;

  Function *fn = calloc(1, sizeof(Function));
  fn->name = expect_ident();
  expect("(");
  fn->params = read_func_params();
  expect("{");

  Node head;
  head.next = NULL;
  Node *cur = &head;
  while (!consume("}")) {
    cur->next = stmt();
    cur = cur->next;
  }

  fn->node = head.next;
  fn->locals = locals;
  return fn;
}

// stmt = "return" expr ";"
//        | "if" "(" expr ")" stmt ("else" stmt)?
//        | "while" "(" expr ")" stmt
//        | "for" "(" expr? ";" expr? ";" expr? ")" stmt
//        | "{" stmt* "}"
//        | expr ";"
static Node *stmt(void) {
  Token *tok;
  if (tok = consume("return")) {
    Node *node = new_node_unary(ND_RETURN, expr(), tok);
    expect(";");
    return node;
  }
  if (tok = consume("if")) {
    Node *node = new_node(ND_IF, tok);
    expect("(");
    node->cond = expr();
    expect(")");
    node->then = stmt();
    if (consume("else"))
      node->els = stmt();
    return node;
  }
  if (tok = consume("while")) {
    Node *node = new_node(ND_WHILE, tok);
    expect("(");
    node->cond = expr();
    expect(")");
    node->then = stmt();
    return node;
  }
  if (tok = consume("for")) {
    Node *node = new_node(ND_FOR, tok);
    expect("(");
    // カウンタ変数
    if (!consume(";")) {
      node->init = new_node_unary(ND_EXPR_STMT, expr(), tok);
      expect(";");
    }
    // 条件
    if (!consume(";")) {
      node->cond = expr();
      expect(";");
    }
    // インクリメント
    if (!consume(")")) {
      node->inc = new_node_unary(ND_EXPR_STMT, expr(), tok);
      expect(")");
    }
    node->then = stmt();
    return node;
  }

  // ブロックを確認
  if (tok = consume("{")) {
    Node head;
    head.next = NULL;
    Node *cur = &head;

    while (!consume("}")) {
      cur->next = stmt();
      cur = cur->next;
    }

    Node *node = new_node(ND_BLOCK, tok);
    node->body = head.next;
    return node;
  }
  Node *node = new_node_unary(ND_EXPR_STMT, expr(), tok);
  expect(";");
  return node;
}

// expr = assign
static Node *expr(void) {
  return assign();
}

// assign = equality ("=" assign)?
static Node *assign(void) {
  Node *node = equality();
  Token *tok;

  if (tok = consume("="))
    node = new_node_binary(ND_ASSIGN, node, assign(), tok);
  return node;
}

// equality = relational ("==" relational | "!=" relational)*
static Node *equality(void) {
  Node *node = relational();
  Token *tok;

  for (;;) {
    if (tok = consume("=="))
      node = new_node_binary(ND_EQ, node, relational(), tok);
    else if (tok = consume("!="))
      node = new_node_binary(ND_NE, node, relational(), tok);
    else
      return node;
  }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static Node *relational(void) {
  Node *node = add();
  Token *tok;

  for (;;) {
    if (tok = consume("<"))
      node = new_node_binary(ND_LT, node , add(), tok);
    else if (tok = consume("<="))
      node = new_node_binary(ND_LE, node, add(), tok);
    else if (tok = consume(">"))
      node = new_node_binary(ND_LT, add(), node, tok);
    else if (tok = consume(">="))
      node = new_node_binary(ND_LE, add(), node, tok);
    else 
      return node;
  }
}

// add = mul ("+" mul | "-" mul)*
static Node *add(void) {
  Node *node = mul();
  Token *tok;

  for (;;) {
    if (tok = consume("+"))
      node = new_node_binary(ND_ADD, node, mul(), tok);
    else if (tok = consume("-"))
      node = new_node_binary(ND_SUB, node, mul(), tok);
    else
      return node;
  }
}

// mul = unary ("*" unary | "/" unary)*
static Node *mul(void) {
  Node *node = unary();
  Token *tok;

  for (;;) {
    if (tok = consume("*"))
      node = new_node_binary(ND_MUL, node, unary(), tok);
    else if (tok = consume("/"))
      node = new_node_binary(ND_DIV, node, unary(), tok);
    else
      return node;
  }
}

// unary = ("+" | "-" | "*" | "&")? unary
//        | primary
static Node *unary(void) {
  Token *tok;
  if (tok = consume("+"))
    return unary();
  if (tok = consume("-"))
    // 負の数の場合は、左辺に0を入れて0-xとして表現
    return new_node_binary(ND_SUB, new_node_num(0, tok), unary(), tok);
  if (tok = consume("&"))
    return new_node_unary(ND_ADDR, unary(), tok);
  if (tok = consume("*"))
    return new_node_unary(ND_DEREF, unary(), tok);
  return primary();
}

// func_args = "(" (assign ("," assign)*)? ")"
static Node *func_args(void) {
  if (consume(")"))
    return NULL;

  Node *head = assign();
  Node *cur = head;
  while(consume(",")) {
    cur->next = assign();
    cur = cur->next;
  }
  expect(")");
  return head;
}


// primary = num | "(" expr ")" | indent func-args?
static Node *primary(void) {
  // 次のトークンが"("なら、"(" expr ")"のはず
  if (consume("(")) {
    Node *node = expr();
    expect(")");
    return node;
  }

  Token *tok;
  if (tok = consume_ident()) {
    if (consume("(")) {
      Node *node = new_node(ND_FUNCALL, tok);
      node->funcname = my_strndup(tok->str, tok->len);
      node->args = func_args();
      return node;
    }
    Var *var = find_var(tok);
    if (!var)
      var = push_var(my_strndup(tok->str, tok->len));
    return new_node_var(var, tok);
  }

  // そうでなければ数値
  tok = token;
  if (tok->kind != TK_NUM)
    error_tok(tok, "expected expression");
  return new_node_num(expect_number(), tok);
}
