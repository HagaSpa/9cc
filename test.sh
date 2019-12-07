#!/bin/bash
assert() {
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

assert 0 '0;'
assert 42 '42;'
assert 21 '5+20-4;'
assert 41 ' 12 + 34 - 5 ;'
assert 47 '5+6*7;'
assert 15 '5*(9-6);'
assert 4 '(3+5)/2;'
assert 10 '-10+20;'
assert 10 '- -10;'
assert 10 '- - +10;'
assert 10 '- - - -10;'

assert 0 '0==1;'
assert 1 '42==42;'
assert 1 '0!=1;'
assert 0 '42!=42;'

assert 1 '0<1;'
assert 0 '1<1;'
assert 0 '2<1;'
assert 1 '0<=1;'
assert 1 '1<=1;'
assert 0 '2<=1;'

assert 1 '1>0;'
assert 0 '1>1;'
assert 0 '1>2;'
assert 1 '1>=0;'
assert 1 '1>=1;'
assert 0 '1>=2;'

assert 3 '1; 2; 3;'

echo OK