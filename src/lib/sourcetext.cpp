#include "sourcetext.hpp"

namespace yalr {

    line_info_t text_source::offset_line_info(text_offset_t offset) {
        analyze_lines();
        
        // start and end define a half open range [start, end)
        // With end being one past the end of the range we want to search.
        decltype(lines)::size_type start = 0;
        std::vector<line_offset_data>::size_type end   = lines.size();

        while(start < end) {
            //
            // Compute our midpoint. assumes integer math.
            //
            auto index = start + (end - start)/2;

            //std::cerr << "s=" << start << " i=" << index << " e=" << end << '\n';
            if (lines[index].line_start == offset) {
                // exact match - return it.
                return lines[index].build_line_info(offset);

            } else if (lines[index].line_start > offset) {
                // we passed it. So everything north is also out of scope.
                // reduce our range to the items before this.
                if (index == end) {
                    // Make sure we shrink our range by at least one each time.
                    end -= 1;
                } else {
                    end = index;
                }

            } else {
                // Less than. Need to move forward more.
                // Strip off everything "before" index.
                // Note that this keeps `index` as a part of the range.
                //
                // However, if index is the same as start, then
                // we just need to quit. The only way we could get that situation
                // is if our range is only 1 item long (start = 4, end = 5).
                // The index math will return start in that case.
                //
                if (index == start) { break; }

                start = index;
            }
        }

        // ran out of places to look. Return whatever we have.
        return lines[start].build_line_info(offset);
    }

    void text_source::analyze_lines() {

        if (analyzed) { return; }

        int offset = 0;

        //
        // line numbers start at 1.
        // but we increment before the first use so
        // initialize to 0.
        //
        int last_line_num = 0;

        //
        // was the last character seen a newline?
        // initalize to true so that the start of the 
        // file counts as a "newline"
        //
        bool saw_line_end = true;

        for(; (unsigned)offset < content.size(); ++offset) {
            if (saw_line_end) {
                lines.emplace_back(++last_line_num, offset);
                saw_line_end = false;
            }
            if (content[offset] == '\n') {
                saw_line_end = true;
                lines[last_line_num-1].line_end = offset;
            }
        }

        //
        // If the last character of the file was not a newline,
        // then we need to update the last line with the ending offset.
        //
        if (not saw_line_end) {
            lines[last_line_num-1].line_end = offset;
        }

        analyzed = true;

        /*
        for (const auto &i : lines) {
            std::cout << i.line_num << " " << i.line_start << " - " << i.line_end << "\n";
        }
        */
    }

    std::ostream &operator<<(std::ostream &s, const text_fragment & tf) {
        s << tf.text;
        return s;
    }

} // namespace yalr
