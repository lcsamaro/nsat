#ifndef SAT_SOLVER_TYPES_H
#define SAT_SOLVER_TYPES_H

namespace sat {

#define NS_CLAUSE_LEARNING
#ifdef NS_CLAUSE_LEARNING
# define NS_MAX_LEARNT                2048
# define NS_MAX_LEARNT_FACTOR          0.2

# define NS_MAX_LEARNT_PER_LIT          16 /* watchlist allocation */
//# define NS_MAX_LEARNT_PER_LIT_FACTOR    2 /* watchlist allocation */
#endif

//#define EXP_BCLAUSE_HEUR

typedef unsigned int Index;
typedef double Real;

/* Variable */
typedef Index VarIndex;
typedef unsigned char VarAssignment;

const VarAssignment ASSIGNMENT_UNASSIGNED     = 2;
const VarAssignment ASSIGNMENT_ASSIGNED_FALSE = 0;
const VarAssignment ASSIGNMENT_ASSIGNED_TRUE  = 1;

/* Literal

   layout
   +------------------------------------------------------------------------------+
   |                                    32b                                       |
   +-----+---------+--------------+-----------------+--------------+--------------+
   | 27b |   1b    |     1b       |       1b        |      1b      |      1b      |
   +-----+---------+--------------+-----------------+--------------+--------------+
   | idx | negated | begin_clause | end_clause_list | watch_flag_a | watch_flag_b |
   +-----+---------+--------------+-----------------+--------------+--------------+
   |    literal    |                                |           skip_sz           |
   +---------------+--------------------------------+-----------------------------+


   begin clause                                                                  begin next clause
   v                                                                             v
   +---------------------+---------------------+--- ... ---+---------------------+
   |  literal            |  literal            |  literal  |  literal            |
   +--------------+------+--------------+------+--- ... ---+--------------+------+
   | begin_clause | skip |              | skip |	   |              | skip |
   |              |  11  |              |  10  |	   |              |  10  |
   +--------------+------+--------------+------+--- ... ---+--------------+------+
   ^                     ^
   watch a ptr           watch b ptr

*/

typedef Index LiteralIndex;
typedef Index Literal;

const Literal LITERAL_NIL                  =    0;

const Literal LITERAL_VAR_SHIFT            =    5;
const Literal LITERAL_IDX_SHIFT            =    4;
const Literal LITERAL_NEGATED_FLAG         = 0x10;

const Literal LITERAL_WATCH_SKIP_FLAG_B    = 0x01;
const Literal LITERAL_WATCH_SKIP_FLAG_A    = 0x02;
const Literal LITERAL_WATCH_SKIP_MASK      = (LITERAL_WATCH_SKIP_FLAG_A | LITERAL_WATCH_SKIP_FLAG_B);
const Literal LITERAL_WATCH_BACKSKIP_MASK  = LITERAL_WATCH_SKIP_FLAG_B;

const Literal LITERAL_BEGIN_CLAUSE_FLAG    = 0x08;
const Literal LITERAL_BEGIN_CLAUSE_MARK    = LITERAL_BEGIN_CLAUSE_FLAG | LITERAL_WATCH_SKIP_MASK;
const Literal LITERAL_END_CLAUSE_LIST_MARK = (0x04 | LITERAL_BEGIN_CLAUSE_MARK); /* assigns begin flag too */

inline Literal lit_new(VarIndex v, bool negated) {
	return (v << LITERAL_VAR_SHIFT) |
		(negated ? LITERAL_NEGATED_FLAG : 0) |
		LITERAL_WATCH_SKIP_FLAG_A;
}
inline Index lit_neg(Literal l) { return l & LITERAL_NEGATED_FLAG; }
inline VarIndex lit_var(Literal l) { return l >> LITERAL_VAR_SHIFT; }
inline VarIndex lit_index(Literal l) { return l >> LITERAL_IDX_SHIFT; }
inline Literal lit_flip(Literal l) { return (l ^ LITERAL_NEGATED_FLAG); }

typedef Literal* Clause;
typedef LiteralIndex ClauseIndex;

} // end of sat namespace

#endif // SAT_SOLVER_NS_H
