#include <stdio.h>
#include <stdlib.h>

/**
 * 与えられた加減算のみの式を、実行するアセンブリを生成する
 * 
 * argc: コマンドライン引数の個数
 * **argv: argvはコマンドライン引数を格納した配列
 *          **argvなので配列のポインタ変数のポインタ？
 */
int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "引数の個数が正しくありません\n");
    return 1;
  }

  // char型のポインタ変数
  char *p = argv[1];

  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");
  // 引数の最初の数字部分（文字列）を10進数のlong型として取得
  printf("  mov rax, %ld\n", strtol(p, &p, 10));

  while (*p) {
    if (*p == '+') {
      // ポインタを進めている
      p++; 
      // +の後は数字を想定.
      printf("  add rax, %ld\n", strtol(p, &p, 10));
      continue;
    }

    if (*p == '-') {
      p++;
      printf("  sub rax, %ld\n", strtol(p, &p, 10));
      continue;
    }

    fprintf(stderr, "予期しない文字列です: '%c'\n", *p);
    return 1;
  }

  printf("  ret\n");
  return 0;
}