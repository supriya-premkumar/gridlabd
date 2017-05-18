/// $Id$
/// @file scheduler.h

#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#define ARMA_DONT_PRINT_ERRORS
#include <armadillo>
#include <string>
#include <array>

using namespace std;

// scheduling options
#define SO_NONE 0x00 ///< no options set
#define SO_NORAMP 0x01 ///< dispatcher does not ramp schedules (ramptime ignored)
#define SO_ALL 0x01 ///< all options set

#define DEFAULT_MAX_AREAS 20

typedef struct s_demandschedule {
	double* total;
	double* response;
	double* slope;
	double* max_price;
} DEMANDSCHEDULE;
typedef struct s_supplyschedule {
	double* total;
	double* slope;
	double* coal;
	double* nuke;
	double* hydro;
	double* geo;
	double* bio;
	double* ror;
	double* solar;
	double* wind;
} SUPPLYSCHEDULE;
typedef struct s_areaschedule {
	string name;
	double* price;
	double* exports;
	DEMANDSCHEDULE demand;
	SUPPLYSCHEDULE supply;
} AREASCHEDULE;
typedef struct s_systemschedule {
	string name;
	uint32 hour;
	DATETIME datetime;
	AREASCHEDULE *area;
} SYSTEMSCHEDULE;

class scheduler : public gld_object {
	typedef enum {
		NONE = 0x00, ///< no scheduling
		INTERNAL = 0x01, ///< internal scheduler used
		EXTERNAL = 0x02, ///< external scheduler used
		STATIC = 0x03, ///< static data file used
	};
public:
	GL_ATOMIC(char1024,schedule_file); ///< schedule data file
	GL_ATOMIC(int16,max_areas); // maximum number of areas supported (default 20)
	GL_ATOMIC(enumeration,method); ///< scheduling method
	GL_BITFLAGS(set,options); ///< scheduling options
	GL_ATOMIC(double,ramptime); ///< dispatch ramp time (s)
	GL_ATOMIC(double,interval); ///< scheduling interval (s)
	GL_ATOMIC(double,supply); ///< scheduled supply (MW)
	GL_ATOMIC(double,demand); ///< scheduled demand (MW)
	GL_ATOMIC(double,losses); ///< schedule losses (MW)
	GL_ATOMIC(double,cost); ///< base supply and losses cost ($/h)
	GL_ATOMIC(double,price); ///< base demand price ($/MWh)
private:
	interconnection *system; ///< interconnection for this scheduler
	FILE *fh; // schedule data file
	char buffer[1024]; // schedule input buffer
	SYSTEMSCHEDULE schedule;
private:
	void schedule_internal(TIMESTAMP t1);
	void schedule_external(TIMESTAMP t1);
	void schedule_static(TIMESTAMP t1);
	bool schedule_open(const char *file);
	bool schedule_next(void);
public:
// required
	DECL_IMPLEMENT(scheduler);
	DECL_SYNC;
}; ///< scheduler class

#endif
