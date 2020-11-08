#pragma once

#include "codegen/fsm.hpp"

#include <set>

namespace yalr::codegen {

	struct diced_trans_t {
		nfa_state_identifier_t to_state_id;
		int length;
		symbol_type_t sym_type;
		char low;
		char high;

		diced_trans_t(char l, char h, nfa_state_identifier_t id) :
			to_state_id(id), length(h-l),
			sym_type(sym_char),
			low(l), high(h) {}

		bool operator <(diced_trans_t const &r) const {
			return ( (length < r.length) or
					(length == r.length and low < r.low ) or
					(length == r.length and low == r.low and high < r.high) or
					(length == r.length and low == r.low and high == r.high and to_state_id < r.to_state_id));
		}
	};


	std::set<diced_trans_t> dice_transitions(std::set<diced_trans_t> &wood_pile);

}
