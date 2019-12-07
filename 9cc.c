#include "9cc.h"

/**
 * アセンブリを生成する
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

  user_input = argv[1];     // 入力値をグローバル変数へ格納
  token = tokenize();       // トークナイズを実行
  Node *node = program();   // 構文解析を実行

  // アセンブリ生成
  codegen(node);
  return 0;
}