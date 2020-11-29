#include "regex_run.hpp"

#include <vector>
#include <iostream>
#include <ostream>

struct runner {
    vm_program program_;
    bool debug_ = false;

    runner(vm_program p) : program_(p) {}

    struct thread_state {
        int pc;
        int length;

        thread_state(int p, int l) : pc(p), length(l) {}
        thread_state() = default;
    };

    struct thread_set {
        std::vector<thread_state> threads;

        bool empty() {
            return threads.empty();
        }

        void clear() {
            threads.clear();
        }

        void add_thread(int pc, int length) {
            for (auto const & x : threads) {
                if (pc == x.pc) {
                    return;
                }
            }
            threads.emplace_back(pc, length);
        }

        void print(std::ostream &strm) {
            strm << "thread set = ";
            if (empty()) {
                strm << "(empty)\n";
                return;
            }
            strm << "\n";
            for (auto const & x : threads) {
                strm << "      " << x.pc << " " << x.length << "\n";
            }
        }
    };

    void add_thread(int pc, int length, thread_set *threads) {
        auto const & current_instr = program_[pc];
        switch (current_instr.opcode) {
            case vm_opcode_t::jump :
                add_thread(current_instr.op[0], length, threads);
                break;
            case vm_opcode_t::noop :
                add_thread(pc+1, length, threads);
                break;
            case vm_opcode_t::split :
                add_thread(current_instr.op[0], length, threads);
                add_thread(current_instr.op[1], length, threads);
                break;
            default :
                threads->add_thread(pc, length);
                break;
        }
    }

    int match_length = -1;

    void check_for_match(thread_set *threads) {
        for (auto const &x : threads->threads) {
            if (program_.at(x.pc).opcode == vm_opcode_t::match) {
                if (x.length > match_length) {
                    match_length = x.length;
                }
            }
        }
    }

    run_results match(std::string_view input) {

        match_length = -1;
        thread_set *current_state = new thread_set();
        thread_set *next_state = new thread_set();
        thread_set *swap_temp;

        add_thread(0, 0, current_state);

        auto current_char_ptr = input.begin();

        /** Main loop
         *
         * Only advance the character pointer once per trip around
         */
        while(true) {
            next_state->clear();
            if (debug_) std::cout << "match_length = " << match_length << "\n";
            if (debug_) current_state->print(std::cout);
            if (debug_) std::cout << "next Char = " << *current_char_ptr << "\n";
            /** Thread loop
             * process one thread for the current character each iteration
             */
            for (auto const & state : current_state->threads) {
                bool stop_the_loop = false;

                int pc = state.pc;
                auto const & current_instr = program_[pc];

                switch(current_instr.opcode) {
                    case vm_opcode_t::literal :
                        if (*current_char_ptr == static_cast<char>(current_instr.op[0])) {
                            add_thread(pc+1, state.length+1, next_state);
                        }
                        break;
                    case vm_opcode_t::range :
                        if (*current_char_ptr >= static_cast<char>(current_instr.op[0]) and
                            *current_char_ptr <= static_cast<char>(current_instr.op[1])) {
                            add_thread(pc+1, state.length+1, next_state);
                        }
                        break;
                    case vm_opcode_t::match :
                        if (state.length > match_length) {
                            match_length = state.length;
                        }
                        // lower priority threads get the shaft
                        stop_the_loop = true;
                        break;
                    case vm_opcode_t::jump :
                    case vm_opcode_t::split :
                    case vm_opcode_t::noop :
                        std::cerr << "saw an epsilon instruction - why didn't add_thread absorb it?\n";
                        return {false, 0};
                        break;
                    default :
                        std::cerr << "unkown instruction at pc = " << state.pc << "\n";
                        return {false, 0};
                }

                if (stop_the_loop) {
                    break;
                }

            } // threads

            if (next_state->empty()) {
                break;
            }

            ++current_char_ptr;
            if (current_char_ptr == input.end()) {
                break;
           }
            swap_temp = current_state;
            current_state= next_state;
            next_state = swap_temp;
        }


        if (debug_) next_state->print(std::cout);
        check_for_match(next_state);
        if (debug_) std::cout << "match_length = " << match_length << "\n";


        if (match_length > -1) {
            return {true, match_length};
        } else {
            return {false, 0};
        }
    }

    
};

run_results regex_runner::match(std::string_view txt) const {
    runner r(program_);

    return r.match(txt);
}
