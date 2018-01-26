#ifndef SAT_SOLVER_NS_H
#define SAT_SOLVER_NS_H

#include "ClauseDB.h"
#include "WatchList.h"
#include "selection/Heuristic.h"

#include <cstdint>
#include <cstring>
#include <stack>
#include <vector>

namespace sat {

enum Result {
	RESULT_SAT = 1,
	RESULT_UNSAT = 0,
	RESULT_UNKNOWN = -1
};

class NS {
	ClauseDB clauses;
	WatchList watch;
	selection::Heuristic& heur;

	VarAssignment *assignment;          /* variable assignment              */
	std::vector<Literal> trail;         /* assignment stack                 */
	std::vector<LiteralIndex> trail_lim; /* separator indices for decision   */
	                                    /* levels in trail                  */
	VarIndex noVars;                    /* number of vars                   */

#ifdef NS_CLAUSE_LEARNING
	std::vector<Literal> implicationGraph; /* v[idx(A)] = B: B -> A         */
	void learn(Literal conflict, Clause clause);
#endif

	void newAssumption();
	void newImplication(Literal l);
	void bt();                          /* backtrack last decision          */

	bool simplify();                    /* top-level, false if UNSAT        */
	bool bcp(Literal unit);             /* true if conflict                 */
	bool preSolve();                    /* setup solver state               */

	bool addClause(Literal l);

	void verifyResult();

public:
	NS(selection::Heuristic& heur);
	~NS();
	NS(const NS&) = delete;
	NS& operator=(const NS&) = delete;

	void setVars(VarIndex n);
	bool addClause(Clause clause, size_t len);
	Result solve();					   /* true if SAT                       */
};

// file readers
void addCNF(NS& ns, const char* fn);   /* adds DIMACS CNF file to DB        */

} // end of sat namespace

#endif // SAT_SOLVER_NS_H
