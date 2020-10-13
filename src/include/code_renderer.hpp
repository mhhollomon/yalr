#pragma once

#include <ostream>
#include "inja.hpp"

namespace yalr {
    class code_renderer {
        std::ostream & stream_ ;
        inja::Environment env_;


      public :
        code_renderer(std::ostream & strm) : stream_(strm) {
            env_.set_expression("<%", "%>");
        }

        void render(const std::string &tmpl, nlohmann::json data) {
            auto t = env_.parse(tmpl);
            env_.render_to(stream_, t, data);
        }

        std::ostream & stream() {
            return stream_;
        }

    };

} // namespace yalr
