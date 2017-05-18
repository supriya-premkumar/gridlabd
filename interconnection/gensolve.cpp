// gensolve.cpp
// dchassin@slac.stanford.edu
//

#include "gensolve.h"

gensolve::gensolve(FLAGS f)
{
	flags = f;
	clear();
}

void gensolve::clear(void)
{
	na = nc = 0;
	A.clear();
	S.clear();
	L.clear();
	C.clear();
	Ac.clear();
	invAc.clear();
	b.clear();
	bc.clear();
	x.clear();
	c.clear();
	n.clear();
	dump("gensolve::clear()");
}

uvec gensolve::add_equality(size_t nvars, ...)
{
	uvec new_vars;
	mat Sn(1, S.n_cols, fill::zeros); // new S row
	va_list ptr;
	va_start(ptr,nvars);
	string terms;
	for ( size_t id = 1 ; id<=nvars ; id++ )
	{
		const char *m = va_arg(ptr,const char *); // get variable name
		double a = va_arg(ptr,double); // get coefficient
		terms += ", m" + to_string(id) + "='" + m + "', a" + to_string(id) + "=" + to_string(a);

		// find variable
		size_t i = 0;
		list<string>::iterator p = n.begin();
		while ( p != n.end() && *p != m ) { i++; p++; } // p <- find name m in list n
		if ( p == n.end() ) // not found
		{
			// add a new variable
			new_vars << S.n_rows;
			S = join_horiz(S,colvec(S.n_rows,1,fill::zeros)); // extend S with zeros for new variable
			Sn << a; // add coefficient to Sn
			n.push_back(string(m)); // record new variable name
		}
		else // found
		{
			// use existing variable
			Sn[i] = a; // set element in Sn
			// TODO: identify existing constraints
		}
	}
	va_end(ptr);
	if ( !new_vars.is_empty() )
	{
		dump("gensolve::add_equality(nvars=%d%s)",nvars,terms.c_str());
		S = join_vert(S,Sn); // add new equality to S
		L << 0; // add zero element to L
		A = join_quad(S,L,C,mat(C.n_rows,L.n_cols,fill::zeros)); // rebuild A
	}
	return new_vars;
}

uvec gensolve::add_constraint(size_t nvars, ...)
{
	uvec new_vars;
	rowvec Cn(A.n_cols, fill::zeros); // new row in C
	colvec Ln(A.n_rows+1, fill::zeros); // new column in L
	va_list ptr;
	va_start(ptr,nvars);
	string terms;
	for ( size_t id = 0 ; id<nvars ; id++ )
	{
		const char *m = va_arg(ptr,const char *); // get variable name
		double a = va_arg(ptr,double); // get coefficient
		terms += ", m" + to_string(id) + "='" + m + "', a" + to_string(id) + "=" + to_string(a);
		size_t i = 0;
		list<string>::iterator p = n.begin();
		while ( p != n.end() && *p != m ) { i++; p++; } // p <- find name m in list n
		if ( p == n.end() ) // not found
			error("constraint variable not found");
		else // found
		{
			new_vars << C.n_rows;
			Cn[i] = a; // add constraint coefficient to C
			for ( size_t j = 0 ; j < S.n_rows ; j++ ) // check rows of S for presence of constrained variable
			{
				for ( size_t k = 0 ; k < S.n_cols ; k++ ) // check columns of S for presence of constrained variable
				{
					if ( S(j,k)!=0 ) // if variable is used
					{
						Ln[j] = S(j,k)*a; // add constraint to L
					}
				}
			}
		}
	}
	va_end(ptr);
	C = join_vert(C,Cn); // add constraint row to C
	L = join_horiz(L,Ln); // add constraint column to L
	A = join_quad(S,L,C,mat(C.n_rows,L.n_cols,fill::zeros)); // rebuild A
	Ac.clear();
	invAc.clear();
	dump("gensolve::add_constraint(nvars=%d%s)",nvars,terms.c_str());
	return new_vars;
}
vec gensolve::solve(const vec &y)
{
	b = y;
	mat invA = inv(A);
	x = invA * b;
	dump("gensolve::solve(b='%s')",to_string(y).c_str());
	return x.head_rows(na);
}

vec gensolve::solve(double a, ...)
{
	mat y;
	va_list ptr;
	va_start(ptr,a);
	for ( int i = 0 ; i < A.n_rows ; i++ )
	{
		b << a;
		a = va_arg(ptr,double);
	}
	va_end(ptr);
	return solve(y);
}

void gensolve::dump(const char *msg, ...)
{
	if ( flags&DUMP )
	{
		char buffer[1024];
		va_list ptr;
		va_start(ptr,msg);
		vsprintf(buffer,msg,ptr);
		va_end(ptr);
		cout << buffer << ":\n";
		print("n",n);
		A.print("A");
		b.print("b");
		x.print("x");
		c.print("c");
	}

}
