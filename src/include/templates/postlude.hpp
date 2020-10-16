#pragma once

#include <string>

namespace yalr::codegen {

using namespace std::string_literals;

const std::string postlude_template =
R"DELIM(

namespace <%namespace%> {
/***** verbatim namespace.bottom start ********/
## for v in verbatim.namespace_bottom
<% v %>
## endfor
/***** verbatim namespace.bottom end ********/

} // namespace <%namespace%>

/***** verbatim file.bottom start ********/
## for v in verbatim.file_bottom
<% v %>
## endfor
/***** verbatim file.bottom end ********/

)DELIM"s;



} // namespace yalr::codegen
