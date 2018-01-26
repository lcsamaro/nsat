#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#ifdef _DEBUG
#define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif

#define DBG_SOLVER_TREE
#define DBG_MEM_DISP
//#define DBG_VERIFY_RESULT
//#define DBG_CLAUSEDB

#include "NS.h"

#include "../../3rd/fmt/format.h"

#include <algorithm>
#include <cstring>
#include <chrono>    /* DBG */
#include <iostream>  /* DBG */
#include <fstream>
#include <stack>
#include <vector>

namespace sat {

NS::NS(selection::Heuristic& heur) :
	assignment(nullptr),
	noVars(0),
	heur(heur) {}

NS::~NS() {
	delete[] assignment;
}

bool NS::simplify() {
	clauses.close();

	// TODO: remove satisfied clauses
	// TODO: pure literal removal, only here

	// TODO: cyclic implications removal
	// TODO: subsumption
	for (VarIndex i = 1; i < noVars+1; i++) {
//		if (assignment[i] != ASSIGNMENT_UNASSIGNED) {
//			bcp(lit_new(i,
//				assignment[i] == ASSIGNMENT_ASSIGNED_TRUE ? false : true));
//		}
	}

	return false;
}

#ifdef NS_CLAUSE_LEARNING
void NS::learn(Literal conflict, Clause clause) {
	fmt::print("\n");



	fmt::print(" - learnt: ( ");
	for (auto tl : trail_lim) {
		auto l = trail[tl];
		l = lit_flip(l);
		fmt::print("{}{} ", (lit_neg(l) ? "~" : ""), lit_var(l));
	}
	fmt::print(")\n");


}
#endif

void NS::newAssumption() {
	trail_lim.push_back(trail.size());
}

void NS::newImplication(Literal l) { // l is unit
	trail.push_back(l);
	assignment[lit_var(l)] = lit_neg(l) ?
		ASSIGNMENT_ASSIGNED_FALSE :
		ASSIGNMENT_ASSIGNED_TRUE;

	heur.propagation(l);

	//std::cout << " assign " << lit_var(l) << " as " << (int)assignment[lit_var(l)] << '\n';
}

void NS::bt() {
	while (trail.size()-1 > trail_lim.back()) {
		assignment[lit_var(trail.back())] = ASSIGNMENT_UNASSIGNED;
		heur.backtrack(trail.back());
		trail.pop_back();
	}
	assignment[lit_var(trail.back())] = ASSIGNMENT_UNASSIGNED;
	heur.backtrack(trail.back());
	trail.pop_back();
	trail_lim.pop_back();

}

bool NS::bcp(Literal unit) {
	unsigned char a;
	newAssumption();

//	std::cout << "bcp> " << (lit_neg(unit) ? '~' : ' ') << lit_var(unit) << '\n';

#ifdef NS_CLAUSE_LEARNING
	implicationGraph[lit_index(lit_flip(unit))] = LITERAL_NIL; // unit was not implied by anyone
#endif

	std::stack<Literal> implications;
	newImplication(unit);
	implications.push(unit);

#ifdef _DEBUG
	auto dbg_print_clause = [] (Clause clause) {
		while (!((*clause) & LITERAL_BEGIN_CLAUSE_FLAG)) {
			clause--;
		}
		std::cout << "(";
		clause += ((*clause)&LITERAL_WATCH_BACKSKIP_MASK);
		while (!(*clause & LITERAL_BEGIN_CLAUSE_FLAG)) {
			std::cout << " " << (lit_neg(*clause) ? '~' : ' ') << lit_var(*clause) << " ";
			clause++;
		}
		std::cout << ")\n";
	};
#endif

	while (!implications.empty()) {
		
		auto flit = lit_flip(implications.top());
		implications.pop();

		auto &watchlist = watch[flit];
		for (int j = ((int)watchlist.size())-1; j >= 0; j--) { /* for each watched clause */
			auto wlit = watchlist[j]; // watched lit ptr, offseted by 1

			auto clause = wlit + ((*wlit)&LITERAL_WATCH_SKIP_MASK); // adjust clause to being after watched lit b
			wlit++; // adjust watched lit

			// update the watched clause
			while (!((*clause) & LITERAL_BEGIN_CLAUSE_FLAG)) {

				auto lit = *clause;
				auto var = lit_var(lit);
				auto neg = lit_neg(lit);
				
				if ((assignment[var] == ASSIGNMENT_UNASSIGNED) ||
					(assignment[var] == (neg ? ASSIGNMENT_ASSIGNED_FALSE : ASSIGNMENT_ASSIGNED_TRUE))) { // update watch

#ifdef _DEBUG
					watch.verify();
#endif
					*clause = *wlit; *wlit = lit; // swap clause watch
					watchlist.remove(j); // remove from current watchlist
					watch[lit].append(wlit-1); // insert new watch into new lit watchlist
#ifdef _DEBUG
					watch.verify();
#endif

					goto found;
				}
				clause++;
			}
			
			// not found, we have a unit clause, propagate the other literal
			Literal implied_lit;
			if (*(watchlist[j]) & LITERAL_BEGIN_CLAUSE_FLAG) { // watch_a, propagate b
				implied_lit = *(wlit+1); // implied_lit is b
			} else { // watch_b, propagate a
				implied_lit = *(wlit-1); // implied_lit is a
			}

			//std::cout << (lit_neg(*wlit) ? '~' : ' ') << lit_var(*wlit) <<
			//	" = F -> " << (lit_neg(implied_lit) ? '~' : ' ') << lit_var(implied_lit) << " = T\n";
			//
			//if (lit_var(*wlit) == lit_var(flit)) {
			//	if (lit_neg(*wlit) != lit_neg(flit)) {
			//		std::cout << "\nbelongs to: \t"; dbg_print_clause(watchlist[j] + ((*watchlist[j])&LITERAL_WATCH_SKIP_MASK));
			//		std::cout << "invalid implication \n";
			//	}
			//} else {
			//	std::cout << "\nbelongs to: \t"; dbg_print_clause(watchlist[j] + ((*watchlist[j])&LITERAL_WATCH_SKIP_MASK));
			//	std::cout << "invalid implication 2\n";
			//}

#ifdef DBG_SOLVER_TREE
			//std::cout << (lit_neg(implied_lit) ? '~' : ' ') << lit_var(implied_lit) << " ";
#endif

			a = lit_neg(implied_lit) ?
				ASSIGNMENT_ASSIGNED_FALSE : ASSIGNMENT_ASSIGNED_TRUE;
			if (a != assignment[lit_var(implied_lit)] &&
				assignment[lit_var(implied_lit)] != ASSIGNMENT_UNASSIGNED) {
				/* conflict */

#ifdef DBG_SOLVER_TREE
				std::cout << "  conflict " << (lit_neg(implied_lit) ? '~' : ' ') << lit_var(implied_lit) << " ";
#endif

#ifdef NS_CLAUSE_LEARNING
				learn(implied_lit,
					watchlist[j] + ((*watchlist[j])&LITERAL_WATCH_SKIP_MASK));
#endif
				heur.conflict(&trail[trail_lim.back()], trail.size()-trail_lim.back());

				return true;
			}

			if (assignment[lit_var(implied_lit)] == ASSIGNMENT_UNASSIGNED) {

#ifdef DBG_SOLVER_TREE
				std::cout << (lit_neg(implied_lit) ? '~' : ' ') << lit_var(implied_lit) << " ";
#endif


#ifdef NS_CLAUSE_LEARNING
				implicationGraph[lit_index(lit_flip(implied_lit))] = *wlit;
#endif

				/*std::cout << (lit_neg(*wlit) ? ' ' : '~') << lit_var(*wlit) <<
					" -> " << (lit_neg(implied_lit) ? '~' : ' ') << lit_var(implied_lit) << " = T\n";*/

				newImplication(implied_lit);
				implications.push(implied_lit);
//std::cout << "pushing " << (lit_neg(implied_lit) ? '~' : ' ') << lit_var(implied_lit) << "\n";

			}
			
			continue;

found: ;

#ifdef EXP_BCLAUSE_HEUR

			clause = wlit + ((*wlit)&LITERAL_WATCH_SKIP_MASK);

			while (!((*clause) & LITERAL_BEGIN_CLAUSE_FLAG)) {
				auto lit = *clause;
				auto var = lit_var(lit);
				auto neg = lit_neg(lit);
				if ((assignment[var] == ASSIGNMENT_UNASSIGNED) ||
					(assignment[var] == (neg ? ASSIGNMENT_ASSIGNED_FALSE : ASSIGNMENT_ASSIGNED_TRUE))) { // update watch
					goto not_bclause;
				}
				clause++;
			}

			// new binary clause
			auto bclause = wlit + ((*wlit)&LITERAL_WATCH_BACKSKIP_MASK);
			heur.bclause(*bclause, bclause[1]);


not_bclause: ;
#endif
		}
	}

	return false;
}

bool NS::preSolve() {
	clauses.close();

	auto noLits = (noVars+1)*2;

#ifdef DBG_CLAUSEDB
	std::cout << "***** dbg *****\n";

	std::cout << " clauses";
	auto c = clauses.ptr;
	while (*c != LITERAL_END_CLAUSE_LIST_MARK) {
		if (*c & LITERAL_BEGIN_CLAUSE_FLAG) {
			std::cout << "\n. ";
			c++;
			continue;
		}
		std::cout << (lit_neg(*c) ? '-' : ' ') << lit_var(*c) << '\t';
		c++;
	}
	std::cout << '\n';
	std::cout << "***************\n";
#endif

#ifdef NS_CLAUSE_LEARNING
	implicationGraph.resize(noLits);
#endif

	watch.setup(clauses, assignment, noLits);

	// so we maintain the invariant
	heur.initialize(clauses, noVars);
	simplify();
	for (int i = 1; i < noVars+1; i++) {
		if (assignment[i] != ASSIGNMENT_UNASSIGNED) {
			if (bcp(lit_new(i, assignment[i] == ASSIGNMENT_ASSIGNED_TRUE ? false : true))) {
				return true; // conflict
			}
		}
	}

#ifdef DBG_WATCHLIST
	watch.dbg();
#endif

#ifdef DBG_MEM_DISP
	std::cout << "***** memory *****\n";
	std::cout << "clauses    " << (clauses.capacity * sizeof(Literal))/(1<<10) << "KB\n";
	std::cout << "watch      " << (watch.capacity * sizeof(Clause))/(1<<10) << "KB\n";
	std::cout << "assignment " << (noVars * sizeof(VarAssignment))/(1<<10) << "KB\n";
	std::cout << "prop       " << (noVars * sizeof(VarAssignment))/(1<<10) << "KB\n";
	std::cout << "******************\n";
#endif



	return false;
}

void NS::setVars(VarIndex n) {
	noVars = n;
	delete[] assignment;
	assignment = new VarAssignment[noVars+1];
	for (VarIndex i = 0; i < noVars+1; i++)
		assignment[i] = ASSIGNMENT_UNASSIGNED;
}

bool NS::addClause(Literal l) {
	auto a = lit_neg(l) ?
		ASSIGNMENT_ASSIGNED_FALSE : ASSIGNMENT_ASSIGNED_TRUE;
	auto v = lit_var(l);
	if (assignment[v] == ASSIGNMENT_UNASSIGNED) assignment[v] = a;
	else if (assignment[v] != a) return true; // conflict
	return false;
}

bool NS::addClause(Clause clause, size_t len) {
	if (!len) return false;
	if (len == 1) return addClause(*clause);
	clauses.append(clause, len);
	return false;
}

Result NS::solve() {
	if (preSolve()) return RESULT_UNSAT;

	const int TRIED_FALSE = 0x01, TRIED_TRUE  = 0x02, TRY_FIRST_FALSE = 0x04;
	struct Trace { VarIndex var; int tried; };
	std::vector<Trace> state;
	state.resize(noVars+1); // +1 not necessary?
	
	VarIndex depth = 0;

#ifdef DBG_SOLVER_TREE
	auto printIndent = [&] () {
		fmt::print("{:2} .  ", depth);

		for (auto i = depth; i > 0; i--) std::cout << "| ";
	};
#endif

	auto selection = heur.next();
	if (selection == LITERAL_NIL) {
#ifdef DBG_VERIFY_RESULT
		verifyResult();
#endif
		return RESULT_SAT; // SAT
	}
	state[depth].var = lit_var(selection);
	state[depth].tried = lit_neg(selection) ? TRY_FIRST_FALSE : 0;
	while (true) {
		if ((state[depth].tried & 0x03) == (TRIED_FALSE | TRIED_TRUE)) { /* both tried */
			if (depth) {
				depth--;
				bt();
				continue;
			} else {
				break;
			}
		}

		bool a = true;
		switch (state[depth].tried & 0x03) {
		case 0:
			if (state[depth].tried & TRY_FIRST_FALSE) {
				state[depth].tried |= TRIED_FALSE;
				a = true;
			} else {
				state[depth].tried |= TRIED_TRUE;
				a = false;
			}
			break;
		case TRIED_FALSE:
			state[depth].tried |= TRIED_TRUE;
			a = false;
			break;
		case TRIED_TRUE:
			state[depth].tried |= TRIED_FALSE;
			a = true;
			break;
		}

#ifdef DBG_SOLVER_TREE
		printIndent();
		fmt::print("{:>2}={} ({}) -> ", state[depth].var,
			(a ? "F" : "T"), trail.size());
#endif

		auto lit = lit_new(state[depth].var, a);
		if (!bcp(lit)) { // ok
			depth++;
			selection = heur.next();
			if (selection == LITERAL_NIL) {
#ifdef DBG_VERIFY_RESULT
				verifyResult();
#endif
				return RESULT_SAT; // SAT
			}
			state[depth].var = lit_var(selection);
			state[depth].tried = lit_neg(selection) ? TRY_FIRST_FALSE : 0;
		} else {
			bt();
		}

#ifdef DBG_SOLVER_TREE
		fmt::print("\n");
#endif
	}
	return RESULT_UNSAT; // UNSAT
}

void NS::verifyResult() {
	std::cout << "\nverifying result... ";
	auto c = clauses.ptr;
	while (*c != LITERAL_END_CLAUSE_LIST_MARK) {
		if (*c & LITERAL_BEGIN_CLAUSE_FLAG) {
			c++;
			while (!(*c & LITERAL_BEGIN_CLAUSE_FLAG)) {

				if (assignment[lit_var(*c)] == ASSIGNMENT_UNASSIGNED) {
					c++;
					continue;
				}

				if ((lit_neg(*c) ? ASSIGNMENT_ASSIGNED_FALSE : ASSIGNMENT_ASSIGNED_TRUE) ==
					assignment[lit_var(*c)]) {
					goto OK;
				}
				c++;
			}
			std::cout << "INVALID\n";
			return;
OK:;
		}
		c++;
	}
	std::cout << "OK\n";
}


// file readers
void addCNF(NS& ns, const char* fn) {
	std::chrono::time_point<std::chrono::system_clock> ts, te;
    ts = std::chrono::system_clock::now();

	std::ifstream f(fn);

	while (f.good() && f.peek() == 'c')
		f.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	if (!f.good() || f.peek() != 'p') throw std::runtime_error("invalid format");
	f.ignore(std::numeric_limits<std::streamsize>::max(), ' ');
	f.ignore(std::numeric_limits<std::streamsize>::max(), ' ');

	unsigned int nvars, nclauses;
	f >> nvars >> nclauses;

	auto clause = new Literal[nvars+1]; // max clause sz
	ns.setVars(nvars);
	for (unsigned int i = 0; i < nclauses; i++) {
		auto clauseWrt = clause;
		while (true) {
			int var; f >> var;
			if (!var) break;
			if (var > 0) *clauseWrt++ = lit_new(var, false); // lit
			else *clauseWrt++ = lit_new(-var, true); // ~lit
		}
		ns.addClause(clause, clauseWrt-clause);
	}

    te = std::chrono::system_clock::now();
	std::cout << "in " << fn;
	std::cout << "  vars " << nvars;
	std::cout << "  clauses " << nclauses;
	std::chrono::duration<double> elapsed_seconds = te-ts;
    std::cout << "  et " << elapsed_seconds.count() << "s\n";
	
	delete[] clause;
}

} // end of sat namespace
