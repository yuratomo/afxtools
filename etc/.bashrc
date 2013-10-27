# ぎゃる＆ぎゃー
#  gal 左窓のパスに移動
#  gar 右窓のパスに移動
#  gap カレント窓パスに移動
#  gao 他窓パスに移動
alias gal='cd "`afxams Extract \\$L`"'
alias gar='cd "`afxams Extract \\$R`"'
alias gap='cd "`afxams Extract \\$P`"'
alias gao='cd "`afxams Extract \\$O`"'

# あふwで選択中のファイルを環境変数にいれる
alias gas='set GAS=`afxams Extract \\$MF`'
