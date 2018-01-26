#ifndef SAT_SOLVER_SELECTION_HEURISTIC_H
#define SAT_SOLVER_SELECTION_HEURISTIC_H

#include "../Types.h"
#include "../ClauseDB.h"

#include "../../../3rd/fmt/format.h"

#include <algorithm>
#include <random>
#include <set>
#include <stack>
#include <vector>

namespace sat {
namespace selection {

class Heuristic {
public:
	virtual void initialize(const ClauseDB& db, VarIndex noVars) = 0;
	virtual void conflict(Literal *involved, VarIndex len) = 0;
	virtual void assumption(const Literal lit) = 0;
	virtual void propagation(const Literal implied) = 0;
	virtual void backtrack(const Literal lit) = 0;
	virtual Literal next() = 0;

#ifdef EXP_BCLAUSE_HEUR
	virtual void bclause(Literal a, Literal b) {}
#endif
};

class Sequential : public Heuristic {
	std::set<VarIndex> available;
public:
	void initialize(const ClauseDB& db, VarIndex noVars) {
		for (VarIndex i = 1; i <= noVars; i++) {
			available.insert(i);
		}
	}

	void conflict(Literal *involved, VarIndex len) {

	}
	
	void assumption(const Literal lit) {
		available.erase(lit_var(lit));
	}

	void propagation(const Literal implied) {
		available.erase(lit_var(implied));
	}

	void backtrack(const Literal lit) {
		available.insert(lit_var(lit));
	}

	Literal next() {
		if (!available.empty()) {
			VarIndex v = *available.begin();
			return lit_new(v, false);
		}
		return LITERAL_NIL;
	}
};

class Random : public Heuristic {
	std::set<VarIndex> available; // other container?

	std::mt19937 mt;
	std::uniform_real_distribution<double> dist;

public:
	Random(std::mt19937::result_type seed) : mt(seed), dist(0.0, 1.0) {}

	void initialize(const ClauseDB& db, VarIndex noVars) {
		for (VarIndex i = 1; i <= noVars; i++)
			available.insert(i);
	}

	void conflict(Literal *involved, VarIndex len) {

	}
	
	void assumption(const Literal lit) {
		available.erase(lit_var(lit));
	}

	void propagation(const Literal implied) {
		available.erase(lit_var(implied));
	}

	void backtrack(const Literal lit) {
		available.insert(lit_var(lit));
	}

	Literal next() {
		if (!available.empty()) {
			auto n = (VarIndex)(dist(mt) * available.size());
			VarIndex v = *std::next(available.begin(), n);
			return lit_new(v, false);
		}
		return LITERAL_NIL;
	}
};

} // end of selection namespace
} // end of sat namespace

#endif // SAT_SOLVER_SELECTION_HEURISTIC_H
