#include "gensolve.h"

int main(int nargs, char *pargs[])
{
	try {
		gensolve solver(gensolve::DUMP);
		solver.add_equality(1,"x0",2.0);
		solver.solve(3.0);
	}
	catch (exception &e)
	{
		cout << "exception caught: "<< e.what() << "\n";
		return 1;
	}
	return 0;
}
