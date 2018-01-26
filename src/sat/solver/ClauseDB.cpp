#include "ClauseDB.h"

#include <cstring>

namespace sat {

const size_t CLAUSES_INITIAL_CAPACITY = 16*1024;

ClauseDB::ClauseDB() : capacity(CLAUSES_INITIAL_CAPACITY), size(0) {
	w = ptr = new Literal[capacity];
}

ClauseDB::~ClauseDB() {
	delete[] ptr;
}

void ClauseDB::append(Clause clause, size_t len) {
	size++;

	*w++ = LITERAL_BEGIN_CLAUSE_MARK;
	while (w + len + 2 > ptr + capacity) { // +2 ?
		capacity *= 2;
		auto aux = new Literal[capacity];
		memcpy(aux, ptr, (w - ptr) * sizeof(Literal));
		auto wrt = w - ptr;
		delete[] ptr;
		ptr = aux;
		w = ptr + wrt;
	}
	memcpy(w, clause, len * sizeof(Literal));
	w += len;
}

void ClauseDB::close() {
	if (*w != LITERAL_END_CLAUSE_LIST_MARK)
		*w = LITERAL_END_CLAUSE_LIST_MARK;
}

void ClauseDB::gc() {

}

void ClauseDB::visitLits(std::function<void(Literal)> f) const {
	auto c = ptr;
	while (*c != LITERAL_END_CLAUSE_LIST_MARK) f(*c++);
}

} // end of sat namespace
