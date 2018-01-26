#ifndef SAT_SOLVER_CLAUSEDB_H
#define SAT_SOLVER_CLAUSEDB_H

#include "Types.h"

#include <functional>

namespace sat {

struct ClauseDB {
	Literal *ptr, *w;

#ifdef NS_CLAUSE_LEARNING
	Literal *lptr, *lw;
#endif

	size_t capacity;
	ClauseIndex size;

	ClauseDB();
	~ClauseDB();
	ClauseDB(const ClauseDB&) = delete;
	ClauseDB& operator=(const ClauseDB&) = delete;

	void append(Clause clause, size_t len);
	void close();
	void gc();

	void visitLits(std::function<void(Literal)> f) const;
};

} // end of sat namespace

#endif
