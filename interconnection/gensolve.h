// gensolve.h
// dchassin@slac.stanford.edu
//

#define ARMA_DONT_PRINT_ERRORS

#include <armadillo>
#include <string>
#include <list>

using namespace arma;
using namespace std;

// some utilities arma should have but doesn't
inline mat join_quad(mat A, mat B, mat C, mat D)
{
	return join_vert( join_horiz(A,B), join_horiz(C,D));
}
inline string to_string(mat x)
{
	string str;
	str = "[";
	for ( mat::iterator i = x.begin() ; i != x.end() ; i++ )
	{
		str += " " + std::to_string(*i);
	}
	str += "]";
	return str;
}
inline void print (const char *msg, list<string> &x)
{
	cout << msg << ": " << "[";
	for ( list<string>::iterator i = x.begin() ; i != x.end() ; i++ )
	{
		cout << *i;
		if ( i!=x.end() )
			cout << ", ";
	}
	cout << "]\n";
}
class gensolve_error : exception {
	string msg;
public:
	explicit gensolve_error(const string &txt) { msg = txt; throw *this; };
	virtual ~gensolve_error(void) throw() {};
	const char *what(void) const throw() { return msg.c_str(); };
};

class gensolve {
private:
	size_t na; // number of equalities
	size_t nc; // number of inequalities
	mat A, S, L, C, Ac, invAc; // full A matrix, system S, lambdas L, constraints C, constrained A, inverse of constrained A
	vec b, bc; // full b vector, constrained B vector
	vec x; // full x solution
	uvec c; // active constraints
	list<string> n; // variable names
	unsigned int flags;
public:
	typedef enum {
		NONE = 0x00,
		VERBOSE = 0x01,
		DUMP = 0x02,
	} FLAGS;
public:
	gensolve(FLAGS f=NONE);
public:
	inline mat &get_A(void) { return A; };
	inline mat &get_Ac(void) { return Ac; };
	inline mat &get_x(void) { return x; };
public:
	uvec add_equality(size_t nvars, ...);
	uvec add_constraint(size_t nvars, ...);
	void clear();
	vec solve(const vec &b);
	vec solve(double b, ...);
public:
	inline void error(const char *msg) { throw gensolve_error(msg); };
public:
	inline void set_verbose (void) { flags |= VERBOSE; };
	inline void clear_verbose (void) { flags &= ~VERBOSE; };
	inline bool is_verbose(void) { return flags&VERBOSE; };
public:
	void dump(const char *msg,...);
};
