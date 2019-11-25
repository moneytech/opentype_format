# OpenType フォントフォーマット

https://aznote.jakou.com/prog/opentype/

OpenType フォントのフォーマット講座の関連ソースコード。

一部のソースファイルで、FreeType ライブラリを使用します。

## Linux でのビルド方法

Makefile を使います。
コンパイラを指定する場合は、Makefile 内の `$(CC) := gcc` の "gcc" を書き換えてください。

すべてのソースをビルドする場合

	$ make

指定ソースのみをビルドする場合 (.c を除いたファイル名を指定)

	$ make 03_tables
