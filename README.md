afxtools
========

Description
-----------
あふwのツール群です。

Detail
------

### 1. afxfind ###
あふwで検索を行うためのツール。

### 2. afxflist ###
あふwで検索を行うためのafxfindの出力結果を、
あふwのファイル窓に表示するためのプラグイン。

### 3. afxbkmk ###
あふw用のぶっくまーく機能。

### 4. afxckw ###
あふwのメッセージ窓にckwを表示するための機能。
ckw 0.9.0-d2 (afxwer's edition 5)が別途必要です。

### 5. afxclock ###
あふwのファイル窓上部のステータス領域に時計を表示する機能。

### 6. afxdmask ###
あふw用の動的マスク機能

### 7. afxexec ###
実行結果をあふwのメッセージ窓に表示する機能。

### 8. afxfazzy ###
あふwのメニューを統合して曖昧検索を可能にするツール。

### 9. afxmake ###
あふwで各種プログラムをメイクするためのツール。

### 10. afxpopd / afxpushd ###
現在の左右窓のカレントディレクトリを記憶(pushd)し、
任意のタイミングで再現する(popd)するツール。

### 11. afxswitch ###
あふwのウィンドウと同サイズ・位置でアプリケーションを起動して、
あふwを非表示にする。
あふwが他のアプリにスイッチするような動作をするツール。

### 12. afxtail ###
あふwでtail -f のような動作をするツール。
メッセージ窓にファイルに追加された行が表示される。

### 13. afxvim ###
あふwの中にgvimを起動する。

### 14. afxyzzy ###
あふwの中にxyzzyを起動する
xyzzy(afxwer's edition)が別途必要です。

### 15. afxandroid ###
android開発支援ツール for あふw

### 16. afxbrws ###
HTML等のIEコンポーネントを使ったあふwの中でブラウザを起動するツール。

### 17. afxtask ###
あふwのメッセージ窓にタスク一覧を表示します。
削除することによってプロセスを終了することが可能です。

### 18. afxdrive ###
あふwのメッセージ窓にドライブ一覧と特殊フォルダを表示します。

### 19. afxnetwork ###
あふwのメッセージ窓にネットワーク一覧と特殊フォルダを表示します。
ワークグループ一覧、コンピューター一覧、共有フォルダ一覧から選択可能。

### 20. afxdrive ###
あふwのメッセージ窓にフォルダ履歴表示します。

### 21. afxview ###
実行結果をあふwのビュワーで表示する機能。

### 22. afxgit ###
あふwでGIT操作用のメニューとプラグイン。

### 23. afxams ###
あふwのオートメーションサーバーの機能をコマンドラインから呼び出すことを
可能にするツール。

### 24. afx2ch ###
あふwで2chを見るためのツール。

### 25. afxsvn (NEW!!) ###
あふwでSVN操作用のメニューとプラグイン。

History
-------

* 1.5.4 u338stevenさん
  - afxfazzyのMigemo対応
  - 直接実行機能強化

* 1.5.3 yuratomo
  - sal.bat/sar.bat/sap.batを追加

* 1.5.2 yuratomo
  - コンパイラをVisual Studio 2013 (VC12)に変更
  - ワーニング除去
  - Windows8でプラグインが固まる問題の対応
  - afxtask 0.1.2
    (u338steven氏)x64 上でタスクをファイル窓に表示できなかったのを修正
    (u338steven氏)表示されるプロセスの作成時刻をローカル時刻に修正
  - afxdrive 0.1.2
    ドライブ名に")"が含まれると正しく開けない問題の修正

* 1.5.1 yuratomo
  - afxdrive 0.1.1
    FDD対策

* 1.5.0 yuratomo
  - afxsvn 0.1.0
    新規追加

* 1.4.1 yuratomo
  - afxfind 1.0.3
    afxfind.iniのresult の絶対パス判定ミス修正
  - afxgit 0.2.0
    statusモードを追加。変更のあるファイルやINDEXに追加されていないファイルを
    ファイル窓に表示する。
    branchモードの表示変更。
  - afx2ch追加

* 1.4.0 yuratomo
  - afxams 0.1.0
    新規追加
  - galとgar関連ファイル
      - /bin/gar.bat
      - /bin/gal.bat
      - /bin/gap.bat
      - /etc/.bashrc
      - /etc/.nya/*

* 1.3.0 yuratomo
  - afxview 0.1.0
    新規追加
  - afxgit 0.1.0
    新規追加
  - afxflist 1.0.2
    マッチしたフォルダに対してJUMPできるようにした
    ※ただしファイルとして表示するのでフォルダのアイコンは表示さ
    れません。(プラグインの仕様上、フォルダクリック時の挙動は変えられないため)

* 1.2.0 yuratomo
  - afxflist 1.0.1
    けんさく後ENTERでファイルを開くのではなく、検索位置移動するようにした。
    1.58以下は未サポート
  - afxtask 0.1.1
    ビュワーで表示した際に使用モジュール一覧を表示するようにした
  - afxdrive 0.1.0 
    新規追加(どらいぶプラグイン)
  - afxnetwork 0.1.0 
    新規追加(ねっとわーくプラグイン)
  - afxhistory 0.1.0 
    新規追加(りれきプラグイン)

* 1.1.1 yuratomo
  - afxbkmk関連のメニューファイル修正
  - afxbrwsのR6034問題対応

* 1.1.0 yuratomo
  - afxtask追加
  - AFXW.DEF.sampleとAFXW.ini.sampleを追加

* 1.0.0 yuratomo
  - Initial Upload
