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
"
execute 'set makeprg=' . 'cmake\ --build\ ' . g:project_path. '/build'

"
" For PlantUML files, set the make program
" to a wrapper in the scripts directory that
" helps do the correct thing for the common.puml
" file.
"
let s:puml_make_cmd =  g:project_path . '/scripts/gendiag %'
augroup filetype_plantuml
    autocmd!
    autocmd FileType plantuml execute 'setlocal makeprg=' . s:puml_make_cmd
augroup END


"
" pvim uses seesion support (-S) to load this file. But seesion
" files are loaded very late. So there may already be buffers
" loaded that need have the makeprg set.
"
for buf in getbufinfo()
    if getbufvar(buf.bufnr, '&filetype') == "plantuml"
        call setbufvar(buf.bufnr, '&makeprg', s:puml_make_cmd)
    endif
endfor

