#!/bin/bash
try() {
  expected="$1"
  input="$2"
  
  # 2つめの引数を./9ccバイナリに渡して実行し、その結果（=アセンブリ）をtmp.sへ書き込んでいる
  ./9cc "$input" > tmp.s
  # tmp.sをtmpバイナリへアセンブル。アセンブリ➡️機械語
  gcc -o tmp tmp.s
  ./tmp
  # shは$?で直前のコードの終了コードを取得できる（ここでは./tmpの終了コード）
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

try 0 0
try 42 42
try 21 "5+20-4"
try 41 " 12 + 34 - 5 "

echo OK