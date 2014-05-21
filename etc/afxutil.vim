" FILE: afxutil.vim
" AUTHOR:  yuratomo
" Last Modified: 2013/10/25
" Version: 0.1.0

if exists('g:afxutils_loaded') && g:afxutils_loaded == 1
  finish
endif

command! -nargs=0 Gal  edit `afxams Extract $L`
command! -nargs=0 Gar  edit `afxams Extract $R`
command! -nargs=0 Gap  edit `afxams Extract $P\F`
command! -nargs=0 Sal  exe 'silent !start /MIN afxams Exec &EXCD -L"' . escape(expand('%:p'),'\') . '"'
command! -nargs=0 Sar  exe 'silent !start /MIN afxams Exec &EXCD -R"' . escape(expand('%:p'),'\') . '"'
command! -nargs=0 Sap  exe 'silent !start /MIN afxams Exec &EXCD -P"' . escape(expand('%:p'),'\') . '"'

let g:afxutils_loaded = 1
