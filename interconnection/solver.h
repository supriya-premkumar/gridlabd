// $Id: solver.h 5070 2015-01-30 23:39:12Z dchassin $

#ifndef _SOLVER_H
#define _SOLVER_H

#define ARMA_DONT_PRINT_ERRORS
#include <armadillo>
#include "transactive.h"

using namespace arma;

class controlarea;
class intertie;

class solver {
private:
	enum {
		SS_UNUSED, ///< not used
		SS_INIT, ///< created but setup not complete
		SS_READY, ///< setup completed ok
		SS_ERROR, ///< setup failed
	} status;
	size_t max_nodes;
	size_t max_lines;
	size_t n_nodes;
	size_t n_lines;
	size_t n_constraints;
	controlarea **node;
	intertie **line;
	intertie **constraint;
	vec x,b;
	mat A;
public:
	solver();
	void create(size_t nodes,size_t lines=0);
	void add_node(OBJECT *p);
	void add_line(OBJECT *p);
	inline bool is_used(void) { return status!=SS_UNUSED; };
	inline bool is_ready(void) { return status==SS_READY; };
	bool setup(void);
	bool solve(double dt);
	void update_b(void);
	bool update_x(void);
	const vec &get_x(void) const { return x; };
	const vec &get_b(void) const { return b; };
	const mat &get_A(void) const { return A; };
private:
	bool solve_constrained(double dt);
	bool solve_unconstrained(double dt);
	bool test_unconstrained(size_t n=3);
public:
	static char1024 solver_check;
};

#endif // _SOLVER_H
