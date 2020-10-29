"
" This file is used along with a simple wrapper for vim
" (pvim) that loads project specific settings.
" It makes the asumption you are working on one project per
" (g)vim session.
"
let g:project_path = expand('<sfile>:p:h')

"
" May need to make this a bit smarter.
" Maybe integrate with on of the cmake-vim
" modules.

execute 'set' 'makeprg=cmake\ --build\' g:project_path. '/build'

let s:puml_make_cmd =  g:project_path . '/scripts/gendiag %'
"
" For PlantUML files, set the make program
" to a wrapper in the scripts directory that
" helps do the correct thing for the common.puml
" file.
"
augroup filetype_plantuml
    autocmd!
    " use let with the local option so that we can get consistent
    " handling of spaces.
    autocmd FileType plantuml let &l:makeprg = s:puml_make_cmd
augroup END

let &path = '.,' . g:project_path. '/src/**,' .  g:project_path. '/test/**,,'

"execute 'set' 'path=.,' . g:project_path. '/src/lib,' . g:project_path. '/src/include,' . g:project_path. '/test,,'


"
" pvim uses session support (-S) to load this file. But seesion
" files are loaded very late. So there may already be buffers
" loaded that need have the makeprg set.
"
"for buf in getbufinfo()
"    if getbufvar(buf.bufnr, '&filetype') == "plantuml"
"        call setbufvar(buf.bufnr, '&makeprg', s:puml_make_cmd)
"    endif
"endfor

