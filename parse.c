#include "9cc.h"

Var *locals;

// すでに出現した変数があるか、nameを用いて検索する
static Var *find_var(Token *tok) {
  for (Var *var = locals; var; var = var->next)
    if (strlen(var->name) == tok->len && !memcmp(tok->str, var->name, tok->len))
      return var;
  return NULL; 
}

// ノード生成における共通部分
static Node *new_node(NodeKind kind) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  return node;
}

// ノードを生成する（lhsもrhdも非終端記号）
static Node *new_node_binary(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = new_node(kind);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

// 数字のノード（終端記号のため、このノードの次はない）
static Node *new_node_num(int val) {
  Node *node = new_node(ND_NUM);
  node->val = val;
  return node;
}

static Node *new_node_unary(NodeKind kind, Node *expr) {
  Node *node = new_node(kind);
  node->lhs = expr;
  return node;
}

// 変数を表すノード
static Node *new_node_var(Var *var) {
  Node *node = new_node(ND_VAR);
  node->var = var;
  return node;
}

// nameの変数をスタックへpushする
// localsにある変数をnextに入れて、*nameを新しいvarのnameへ格納（先入れ先だしを表現）
static Var *push_var(char *name) {
  Var *var = calloc(1, sizeof(Var));
  var->next = locals;
  var->name = name;
  locals = var;
  return var;
}

static Node *stmt(void);
static Node *expr(void);
static Node *assign(void);
static Node *equality(void);
static Node *relational(void);
static Node *add(void);
static Node *mul(void);
static Node *unary(void);
static Node *primary(void);

// program = stmt*
Program *program(void) {
  locals = NULL;

  Node head = {};
  Node *cur = &head;

  // 終了文字が出るまで
  while (!at_eof()) {
    cur->next = stmt();
    cur = cur->next;
  }

  Program *prog = calloc(1, sizeof(Program));
  prog->node = head.next;
  prog->locals = locals;
  return prog;
}

// stmt = "return" expr ";"
//        | "if" "(" expr ")" stmt ("else" stmt)?
//        | "while" "(" expr ")" stmt
//        | "for" "(" expr? ";" expr? ";" expr? ")" stmt
//        | "{" stmt* "}"
//        | expr ";"
static Node *stmt(void) {
  if (consume("return")) {
    Node *node = new_node_unary(ND_RETURN, expr());
    expect(";");
    return node;
  }
  if (consume("if")) {
    Node *node = new_node(ND_IF);
    expect("(");
    node->cond = expr();
    expect(")");
    node->then = stmt();
    if (consume("else"))
      node->els = stmt();
    return node;
  }
  if (consume("while")) {
    Node *node = new_node(ND_WHILE);
    expect("(");
    node->cond = expr();
    expect(")");
    node->then = stmt();
    return node;
  }
  if (consume("for")) {
    Node *node = new_node(ND_FOR);
    expect("(");
    // カウンタ変数
    if (!consume(";")) {
      node->init = new_node_unary(ND_EXPR_STMT, expr());
      expect(";");
    }
    // 条件
    if (!consume(";")) {
      node->cond = expr();
      expect(";");
    }
    // インクリメント
    if (!consume(")")) {
      node->inc = new_node_unary(ND_EXPR_STMT, expr());
      expect(")");
    }
    node->then = stmt();
    return node;
  }

  // ブロックを確認
  if (consume("{")) {
    Node head;
    head.next = NULL;
    Node *cur = &head;

    while (!consume("}")) {
      cur->next = stmt();
      cur = cur->next;
    }

    Node *node = new_node(ND_BLOCK);
    node->body = head.next;
    return node;
  }
  Node *node = new_node_unary(ND_EXPR_STMT, expr());
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

  if (consume("="))
    node = new_node_binary(ND_ASSIGN, node, assign());
  return node;
}

// equality = relational ("==" relational | "!=" relational)*
static Node *equality(void) {
  Node *node = relational();

  for (;;) {
    if (consume("=="))
      node = new_node_binary(ND_EQ, node, relational());
    else if (consume("!="))
      node = new_node_binary(ND_NE, node, relational());
    else
      return node;
  }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static Node *relational(void) {
  Node *node = add();

  for (;;) {
    if (consume("<"))
      node = new_node_binary(ND_LT, node , add());
    else if (consume("<="))
      node = new_node_binary(ND_LE, node, add());
    else if (consume(">"))
      node = new_node_binary(ND_LT, add(), node);
    else if (consume(">="))
      node = new_node_binary(ND_LE, add(), node);
    else 
      return node;
  }
}

// add = mul ("+" mul | "-" mul)*
static Node *add(void) {
  Node *node = mul();

  for (;;) {
    if (consume("+"))
      node = new_node_binary(ND_ADD, node, mul());
    else if (consume("-"))
      node = new_node_binary(ND_SUB, node, mul());
    else
      return node;
  }
}

// mul = unary ("*" unary | "/" unary)*
static Node *mul(void) {
  Node *node = unary();

  for (;;) {
    if (consume("*"))
      node = new_node_binary(ND_MUL, node, unary());
    else if (consume("/"))
      node = new_node_binary(ND_DIV, node, unary());
    else
      return node;
  }
}

// unary = ("+" | "-")? unary | primary
static Node *unary(void) {
  if (consume("+"))
    return unary();
  if (consume("-"))
    // 負の数の場合は、左辺に0を入れて0-xとして表現
    return new_node_binary(ND_SUB, new_node_num(0), unary());
  return primary();
}

// primary = num | "(" expr ")" | indent
static Node *primary(void) {
  // 次のトークンが"("なら、"(" expr ")"のはず
  if (consume("(")) {
    Node *node = expr();
    expect(")");
    return node;
  }

  Token *tok = consume_ident();
  if (tok) {
    Var *var = find_var(tok);
    if (!var)
      var = push_var(my_strndup(tok->str, tok->len));
    return new_node_var(var);
  }

  // そうでなければ数値
  return new_node_num(expect_number());
}
