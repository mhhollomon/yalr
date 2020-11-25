#if ! defined(YALR_SOURCETEXT_HPP_)
#define YALR_SOURCETEXT_HPP_

#include <iostream>
#include <string>
#include <string_view>
#include <memory>
#include <vector>
#include <optional>

namespace yalr {

    // don't want to wrap this 
    // because we'll do math on it.
    using text_offset_t = int;


    //
    // Simple struct to pass info
    // back to the user. Easier than a 4-tuple
    //
    struct line_info_t {
        int line_num;
        int col_num;
        text_offset_t line_start;
        text_offset_t line_end;
    };

    //***********************************************************************
    // text_source
    //
    // Represent a source of text. Contains the entire
    // text. The public view contains a string_view, but
    // we also hold the std::string so that we are sure
    // the string_views don't become dangling.
    //
    struct text_source {
        std::string name;
        std::string_view content;

        //
        // Compute the line and column number of the
        // offset. Returns "pointers" to the start
        // and end of the line.
        //
        line_info_t offset_line_info(text_offset_t offset);

        //
        // Note - you are giving up ownership of the string
        //
        text_source(std::string n, std::string&& c) :
                    name(n), content_data(std::move(c)) {

            content = content_data;
            auto buffer_size = int(content_data.size()/30);
            if (buffer_size < 500) buffer_size = 500;
            lines.reserve(buffer_size);
        }

      private:
        //
        // Used to store info on where lines begin
        // in the source.
        // XXX - Maybe get rid of this and use line_info_t directly?
        // overhead would only be an int per line.
        //
        struct line_offset_data {
            int line_num;
            text_offset_t line_start;
            text_offset_t line_end;
            line_offset_data( int l, text_offset_t s, text_offset_t e=0) :
                line_num(l), line_start(s), line_end(e) {}
            line_info_t build_line_info(text_offset_t offset) {
                return {line_num, offset-line_start+1, line_start, line_end};
            }
        };

        //
        // The ACTUAL content.
        //
        std::string content_data;
        std::vector<line_offset_data> lines;
        bool analyzed = false;
        void analyze_lines();
    };

    //***********************************************************************
    // text_location
    //
    // A location inside a text_source. Holds a shared_ptr to the text_source.
    //
    struct text_location {
        text_offset_t offset;
        std::shared_ptr<text_source> source;

        line_info_t line_info() const {
            return source->offset_line_info(offset);
        }
    };

    //***********************************************************************
    // text_fragment
    //
    // A piece of text found at a certain location.
    //
    struct text_fragment {
        std::string_view text;
        text_location location;
        friend std::ostream &operator<<(std::ostream &s, const text_fragment & tf);
    };

    using optional_text_fragment = std::optional<text_fragment>;

} // namespace


#endif
