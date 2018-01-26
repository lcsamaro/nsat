#ifndef SAT_SOLVER_WATCHLIST_H
#define SAT_SOLVER_WATCHLIST_H

#include "Types.h"
#include "ClauseDB.h"

#include <iostream>
#include <vector>

//#define DBG_WATCHLIST

namespace sat {

//#define WATCHLIST_INTEGER_OFFSETS
#ifdef WATCHLIST_INTEGER_OFFSETS
// TODO: less memory, more perf?
typedef ClauseIndex ClausePtr;
#else
typedef Clause* ClausePtr;
#endif

struct LiteralWatchList {
	Clause *ptr, *w;

	//LiteralWatchList(const LiteralWatchList&) = delete;
	//LiteralWatchList& operator=(const LiteralWatchList&) = delete;

	LiteralWatchList(Clause *ptr) : ptr(ptr), w(ptr) {}
	void append(Clause clause) {
		*w++ = clause;
	}
	void remove(size_t i) {
		ptr[i] = *--w;
	}
	
	size_t size() {
		return w-ptr;
	}

	Clause operator [] (VarIndex i) {
		return ptr[i];
	}

	//void print() {
	//	std::cout << "#" << size() << "\t";
	//	for (size_t i = 0; i < size(); i++) {
	//
	//		auto clause = ptr[i];
	//		clause += ((*clause)&LITERAL_WATCH_BACKSKIP_MASK);
	//
	//		std::cout << "( ";
	//		while (!(*clause & LITERAL_BEGIN_CLAUSE_FLAG)) {
	//			std::cout << lit_var(*clause) << ' ';
	//			clause++;
	//		}
	//		std::cout << ") ";
	//	}
	//	std::cout << '\n';
	//}
};


struct WatchList {
	Clause *data; // TODO: use integer offsets instead of pointers?
	size_t capacity;
	std::vector<LiteralWatchList> lists;

	WatchList() : data(nullptr), capacity(0) {}
	~WatchList() { delete[] data; }

	void setup(const ClauseDB& clauses, VarAssignment *assignment, VarIndex noLits) {
		std::vector<ClauseIndex> lit_usage(noLits, 0);

		auto c = clauses.ptr;
		while (*c != LITERAL_END_CLAUSE_LIST_MARK) {
			lit_usage[lit_index(*c++)]++;
		}
		lit_usage[0] = lit_usage[1] = 0; // sentinel

#ifdef NS_CLAUSE_LEARNING
		for (int i = 2; i < noLits; i++) {
			//lit_usage[i] *= NS_MAX_LEARNT_PER_LIT_FACTOR;
			lit_usage[i] += NS_MAX_LEARNT_PER_LIT;
		}
#endif

		size_t sz = 0;
		for (VarIndex i = 0; i < noLits; i++) sz += (lit_usage[i]); // +1 was creating access violations
		sz += noLits;

		if (data) delete[] data;
		data = new Clause[sz]; capacity = sz;

		//dbg - ini
		//memset(data, 0, capacity * sizeof(Clause));
		//dbg - end

		lists.reserve(noLits);

		lists.push_back(LiteralWatchList(data));
		for (VarIndex i = 1; i < noLits; i++)
			lists.push_back(LiteralWatchList(lists[i-1].ptr + (lit_usage[i-1]))); // TODO: +1 not used without close()

#ifdef _DEBUG
		auto counter = 0;
#endif
		c = clauses.ptr;
		while (*c != LITERAL_END_CLAUSE_LIST_MARK) { // iterate clauses

			if (*c & LITERAL_BEGIN_CLAUSE_FLAG) {

#ifdef _DEBUG
				counter++;
#endif
				auto cl = c;

				// TODO: skip satisfied

				//while (!(*(++c) & LITERAL_BEGIN_CLAUSE_FLAG)) {
				//	auto lit = *c;
				//	if ((assignment[lit_var(lit)] == ASSIGNMENT_ASSIGNED_FALSE && lit_neg(lit)) || 
				//		(assignment[lit_var(lit)] == ASSIGNMENT_ASSIGNED_TRUE && !lit_neg(lit))) {
				//		// satisfied clause
				//		goto skip_clause;
				//	}
				//}

#ifdef DBG_WATCHLIST
				std::cout << (lit_neg(c[1]) ? '~' : ' ') << lit_var(c[1]) << " (" << lit_index(c[1]) << ") watching clause " << counter << " @ " << cl << '\n';
#endif
				lists[lit_index(c[1])].append(cl); // watch a points to clause delim

#ifdef DBG_WATCHLIST
				std::cout << (lit_neg(c[2]) ? '~' : ' ') << lit_var(c[2]) << " (" << lit_index(c[2]) << ") watching clause " << counter << " @ " << (cl+1) << '\n';
#endif

				lists[lit_index(c[2])].append(++cl); // watch b actually points to watched literal a

				//c = cl;
				//c+=2;
				//c = cl + 2;
			}
			c++;
//skip_clause: ;
		}

#ifdef _DEBUG
verify(); // TODO: remove when confident
#endif

	}

	LiteralWatchList& operator [] (Literal l) {
		return lists[lit_index(l)];
	}

#ifdef _DEBUG
	void verify() {
			// verify
		for (int i = 1; i < lists.size()/2; i++) {
			for (int j = 0; j < 2; j++) {
				auto lit = lit_new(i, j ? true : false);
		
				auto wl = lists[lit_index(lit)];
				for (int k = 0; k < wl.size(); k++) {


					if (lit_var(*(wl[k]+1)) == lit_var(lit) && 
						lit_neg(*(wl[k]+1)) == lit_neg(lit)) {
						//std::cout << "ok";
						goto ok;
					} else {
						std::cout << "lvar: " << lit_var(*(wl[k]+1)) << ' ' << lit_var(lit) << '\n';
						std::cout << "lneg: " << (int)lit_neg(*(wl[k]+1)) << ' ' << (int)lit_neg(lit) << '\n';
					}
				}

				if (wl.size() == 0) {
					goto ok;
				}

				std::cout << "invalid watchlist v" << lit_var(lit) << " n" << (int)lit_neg(lit) << '\n';

				//wl.print();

				ok:;

			}
		}
	}
#endif

	void dbg() {
		std::cout << "***************** wlist dbg - ini *****************\n";
		for (size_t c = 2; c < lists.size(); c++) { // skip sentinel
			auto wlist = lists[c];
			std::cout << (c&1 ? '-' : ' ') << c/2 << "\t#" << wlist.size() << " > ";
			for (size_t j = 0; j < wlist.size(); j++) { // for each watched clause
				auto clause = wlist[j];
				//std::cout << "@ " << clause << " ";
				std::cout << "(";
				//std::cout << " skip: " << ((*clause)&LITERAL_WATCH_BACKSKIP_MASK) << " > ";
				clause += ((*clause)&LITERAL_WATCH_BACKSKIP_MASK);
				while (!(*clause & LITERAL_BEGIN_CLAUSE_FLAG)) {
					std::cout << " " << (lit_neg(*clause) ? '~' : ' ') << lit_var(*clause) << " ";
					clause++;
				}
				std::cout << ") ";
			}
			std::cout << '\n';
		}
		std::cout << "***************** wlist dbg - end *****************\n";
		//std::cin.get();
	}
};

} // end of sat namespace

#endif // SAT_SOLVER_WATCHLIST_H
