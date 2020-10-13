#include "codegen.hpp"

#include "code_renderer.hpp"
#include "templates/main_func_template.hpp"


namespace yalr {

std::unique_ptr<algo::gen_results> 
generate_code(
    std::shared_ptr<yalr::algo::parser_generator> gen, const analyzer_tree &tree, std::ostream &strm) {

    auto cr = code_renderer(strm);
    auto retval = gen->generate_code(cr);
    
    if (tree.options.code_main.get()) {
        cr.stream() << yalr::codegen::gen_main_code;
    }

    return retval;
}

} // namespace yalr
