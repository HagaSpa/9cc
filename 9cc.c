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

  user_input = argv[1];        // 入力値をグローバル変数へ格納
  token = tokenize();          // トークナイズを実行
  Function *prog = program();  // 構文解析を実行（パースを実行）

  for (Function *fn=prog; fn; fn=fn->next) {
    // nodeを全て回してvariablesの分だけoffsetを生成し、stack_sizeへ格納する
    int offset = 0;
    for (Var *var=prog->locals; var; var=var->next) {
      // 変数１つにつき8バイト割り当てるとする
      offset += 8;
      var->offset = offset;
    }
    fn->stack_size = offset;
  }

  // アセンブリ生成
  codegen(prog);
  return 0;
}