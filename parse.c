#include "9cc.h"

static Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

static Node *new_node_num(int val) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_NUM;
  node->val = val;
  return node;
}

static Node *stmt();
static Node *expr();
static Node *equality();
static Node *relational();
static Node *add();
static Node *mul();
static Node *unary();
static Node *primary();

// program = stmt*
Node *program() {
  Node head = {};
  Node *cur = &head;

  // 終了文字が出るまで
  while (!at_eof()) {
    cur->next = stmt();
    cur = cur->next;
  }
  return head.next;
}

// stmt = expr ";"
static Node *stmt() {
  Node *node = expr();
  expect(";");
  return node;
}

// expr = equality
static Node *expr() {
  return equality();
}

// equality = relational ("==" relational | "!=" relational)*
static Node *equality() {
  Node *node = relational();

  for (;;) {
    if (consume("=="))
      node = new_node(ND_EQ, node, relational());
    else if (consume("!="))
      node = new_node(ND_NE, node, relational());
    else
      return node;
  }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static Node *relational() {
  Node *node = add();

  for (;;) {
    if (consume("<"))
      node = new_node(ND_LT, node , add());
    else if (consume("<="))
      node = new_node(ND_LE, node, add());
    else if (consume(">"))
      node = new_node(ND_LT, add(), node);
    else if (consume(">="))
      node = new_node(ND_LE, add(), node);
    else 
      return node;
  }
}

// add = mul ("+" mul | "-" mul)*
static Node *add() {
  Node *node = mul();

  for (;;) {
    if (consume("+"))
      node = new_node(ND_ADD, node, mul());
    else if (consume("-"))
      node = new_node(ND_SUB, node, mul());
    else
      return node;
  }
}

// mul = unary ("*" unary | "/" unary)*
static Node *mul() {
  Node *node = unary();

  for (;;) {
    if (consume("*"))
      node = new_node(ND_MUL, node, unary());
    else if (consume("/"))
      node = new_node(ND_DIV, node, unary());
    else
      return node;
  }
}

// unary = ("+" | "-")? unary | primary
static Node *unary() {
  if (consume("+"))
    return unary();
  if (consume("-"))
    // 負の数の場合は、左辺に0を入れて0-xとして表現
    return new_node(ND_SUB, new_node_num(0), unary());
  return primary();
}

// primary = num | "(" expr ")"
static Node *primary() {
  // 次のトークンが"("なら、"(" expr ")"のはず
  if (consume("(")) {
    Node *node = expr();
    expect(")");
    return node;
  }

  // そうでなければ数値
  return new_node_num(expect_number());
}