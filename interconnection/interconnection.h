// $Id: interconnection.h 5181 2015-06-18 23:56:26Z dchassin $

#ifndef _INTERCONNECTION_H
#define _INTERCONNECTION_H

#include "solver.h"
#include "controlarea.h"
#include "intertie.h"
class scheduler;

typedef enum {
	ICS_OK=KC_GREEN,
	ICS_OVERCAPACITY=KC_RED,
	ICS_OVERFREQUENCY=KC_ORANGE,
	ICS_UNDERFREQUENCY=KC_BLUE,
	ICS_BLACKOUT=KC_BLACK,
} INTERCONNECTIONSTATUS;

typedef enum {
	IC_BALANCED = 0, ///< initially balanced system (find balance)
	IC_STEADY = 1, ///< initially steady system (check balance)
	IC_TRANSIENT = 2, ///< initially unbalanced system (balance ignored)
} INITIALCONDITION;

class solver;
class interconnection : public gld_object {
public:
	static double frequency_resolution;
public:
	// public data
	GL_ATOMIC(int64,update);
	GL_ATOMIC(double,frequency);
	GL_ATOMIC(double,inertia);
	GL_ATOMIC(double,capacity);
	GL_ATOMIC(double,damping);
	GL_ATOMIC(double,supply);
	GL_ATOMIC(double,demand);
	GL_ATOMIC(double,losses);
	GL_ATOMIC(double,imbalance);
	GL_ATOMIC(enumeration,initialize);
	GL_ATOMIC(enumeration,status);
	GL_ATOMIC(object,initial_balancing_unit);
private:
	// private data
	double f0; ///< initial frequency on this pass
	double fr; ///< relative frequency difference
	size_t n_controlarea; ///< number of control areas
	OBJECTLIST *controlarea_list; ///< list of controlareas
	controlarea **area;
	size_t n_intertie; ///< number of interties
	OBJECTLIST *intertie_list; ///< list of interties
	intertie **line;
	solver *engine; ///< solver engine
	TIMESTAMP last_solution_time;
	scheduler *central_scheduler;
public:
	inline size_t get_nareas() { return n_controlarea; };
	inline double get_nlines() { return n_intertie; };
	inline controlarea* get_area(size_t n) { return area[n]; };
	inline intertie* get_line(size_t n) { return line[n]; };
public:
	void set_scheduler(scheduler*p) { central_scheduler = p; };
	scheduler* get_schedular(void) { return central_scheduler; };
public:
	DECL_IMPLEMENT(interconnection);
	DECL_PRECOMMIT;
	DECL_SYNC;
	DECL_COMMIT;
	DECL_NOTIFY(update);
	int kmldump(int (*stream)(const char*, ...));
public:
	void solve_powerflow(double dt=0);
	int init_balanced();
	int init_steady();
	int init_transient();
};

#endif // INTERCONNECTION_H
