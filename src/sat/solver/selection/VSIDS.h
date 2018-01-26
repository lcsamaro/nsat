#ifndef SAT_SOLVER_SELECTION_VSIDS_H
#define SAT_SOLVER_SELECTION_VSIDS_H

#include "Heuristic.h"

#include "../../../3rd/fmt/format.h"

#include <set>
#include <vector>

namespace sat {
namespace selection {

class VSIDS : public Heuristic {
	struct Choice {
		Real falseWeight;
		Real trueWeight;

		Choice() : falseWeight((Real)0), trueWeight((Real)0) {}

		Real f() {
			return std::max(falseWeight, trueWeight);
		}
	};

	std::set<VarIndex> available;
	std::vector<Choice> choices;

	Real bump;

	VarIndex best;

#define MAX_WEIGHT ((Real)1e40)
#define WEIGHT_NORM ((Real)1e-40)

	void normalize() {
		for (auto& c : choices) {
			c.falseWeight *= WEIGHT_NORM;
			c.trueWeight *= WEIGHT_NORM;
		}
		bump *= WEIGHT_NORM;
	}

	void bumpActivity(Real& activity) {
		//if ((activity += bump) > MAX_WEIGHT) normalize();
		activity += bump;
	}

public:
	void initialize(const ClauseDB& db, VarIndex noVars) {
		bump = (Real)1;
		best = 1;
		choices.resize(noVars+1);

		for (VarIndex i = 1; i <= noVars; i++)
			available.insert(i);

		db.visitLits([&] (Literal l) {
			if (lit_neg(l)) {
				bumpActivity(choices[lit_var(l)].falseWeight);
			} else {
				bumpActivity(choices[lit_var(l)].trueWeight);
			}
		});
	}

	void conflict(Literal *involved, VarIndex len) {
		bump *= 1.02;
		while (len > 0) {
			auto l = involved[--len];
			if (lit_neg(l)) {
				bumpActivity(choices[lit_var(l)].falseWeight);
			} else {
				bumpActivity(choices[lit_var(l)].trueWeight);
			}
		}
		if (bump > MAX_WEIGHT) normalize();
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

#ifdef EXP_BCLAUSE_HEUR
	void bclause(Literal a, Literal b) {
		if (lit_neg(a)) bumpActivity(choices[lit_var(a)].falseWeight);
		else bumpActivity(choices[lit_var(a)].trueWeight);

		if (lit_neg(b)) bumpActivity(choices[lit_var(b)].falseWeight);
		else bumpActivity(choices[lit_var(b)].trueWeight);
	}
#endif

	Literal next() {
		if (!available.empty()) {
			VarIndex best = 0;
			for (auto a : available) {
				if (choices[a].f() > choices[best].f() || !best) {
					best = a;
				}
			}
			auto v = choices[best];

			//fmt::print("{:>3}/{:>3} {}       \r", choices.size()-available.size(), choices.size(), choices[best].f());

			return lit_new(best, v.falseWeight > v.trueWeight ? true : false); // #~L > #L, propagate ~L
			//return lit_new(best, v.falseWeight > v.trueWeight ? false : true); // #~L > #L, propagate L
		}
		return LITERAL_NIL;
	}
};

} // end of selection namespace
} // end of sat namespace

#endif // SAT_SOLVER_SELECTION_VSIDS_H
