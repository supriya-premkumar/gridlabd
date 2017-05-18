// $Id$

#include "transactive.h"
#include <stdio.h>

EXPORT_IMPLEMENT(scheduler)
EXPORT_SYNC(scheduler)
IMPLEMENT_ISA(scheduler)
IMPLEMENT_NUL(scheduler,presync,TS_NEVER)
IMPLEMENT_NUL(scheduler,postsync,TS_NEVER)

scheduler::scheduler(MODULE *module)
{
	if (oclass==NULL)
	{
		// register the class definition
		oclass = gl_register_class(module,"scheduler",sizeof(scheduler),PC_BOTTOMUP);

		if (oclass==NULL)
			exception("unable to register class");

		// publish the class properties
	if (gl_publish_variable(oclass,
		PT_char1024,"schedule_file", get_schedule_file_offset(), PT_DESCRIPTION,"schedule source file",
		PT_double,"interval[s]",get_interval_offset(),PT_DESCRIPTION,"scheduling interval (s)",
		PT_double,"ramptime[s]",get_ramptime_offset(),PT_DESCRIPTION,"dispatcher ramp duration (s)",
		PT_enumeration,"method",get_method_offset(),PT_DESCRIPTION,"scheduling method",
			PT_KEYWORD,"NONE",(enumeration)NONE,
			PT_KEYWORD,"INTERNAL",(enumeration)INTERNAL,
			PT_KEYWORD,"EXTERNAL",(enumeration)EXTERNAL,
			PT_KEYWORD,"STATIC",(enumeration)STATIC,
		PT_set,"options",get_options_offset(),PT_DESCRIPTION,"scheduling options",
			PT_KEYWORD,"NONE",(set)SO_NONE,
			PT_KEYWORD,"NORAMP",(set)SO_NORAMP,
			PT_KEYWORD,"ALL",(set)SO_ALL,
		PT_double,"supply[MW]",get_supply_offset(),PT_DESCRIPTION,"scheduled supply (MW)",
		PT_double,"demand[MW]",get_demand_offset(),PT_DESCRIPTION,"scheduled demand (MW)",
		PT_double,"losses[MW]",get_losses_offset(),PT_DESCRIPTION,"scheduled losses (MW)",
		PT_double,"cost[$/h]",get_cost_offset(),PT_DESCRIPTION,"scheduled cost ($/h)",
		PT_double,"price[$/MWh]",get_price_offset(),PT_DESCRIPTION,"scheduled price ($/MWh)",
		PT_int16,"max_areas",get_max_areas_offset(),PT_DESCRIPTION,"maximum number of areas supported",
		NULL) < 1)
		exception("unable to publish scheduler roperties");
	}
}

int scheduler::create()
{
	interval = schedule_interval;
	ramptime = dispatch_ramptime;
	max_areas = DEFAULT_MAX_AREAS;
	return 1;
}

int scheduler::init(OBJECT *parent)
{
	// check parent
	system = OBJECTDATA(parent,interconnection);
	if ( !system->isa("interconnection") )
	{
		error("parent must be an interconnection object");
	}
	// check methods
	if ( get_method()==EXTERNAL )
	{
		error("matpower scheduling not implemented");
	}

	// check options
	if ( get_method()==NONE )
	{
		error("scheduling method must be specified");
	}
	if ( get_method()==STATIC )
	{
		char fname[1025];
		if ( gl_findfile(get_schedule_file(),NULL,4,fname,sizeof(fname)) == NULL )
		{
			error("schedule file '%s' not found", (const char*)get_schedule_file());
			return 0;
		}
		if ( !schedule_open(fname) )
		{
			error("schedule file '%s' load failed", (const char*)get_schedule_file());
			return 0;
		}
		schedule.area = (AREASCHEDULE*)malloc(sizeof(AREASCHEDULE)*max_areas);
	}
	else
	{
		error("schedule method '%s' not supported yet", (const char*)get_method_string());
		return 0;
	}
	system = OBJECTDATA(get_parent(),interconnection);
	system->set_scheduler(this);
	
	return 1;
}

TIMESTAMP scheduler::sync(TIMESTAMP t1)
{
	TIMESTAMP dt = (TIMESTAMP)interval;
	if ( t1 % dt == 0 ) // scheduler time has arrived
	{
		switch ( get_method() ) {
		case INTERNAL: schedule_internal(t1);
		case EXTERNAL: schedule_external(t1);
		default: break;
		}
	}
	return ((t1/dt)+1)*dt;
}

void scheduler::schedule_internal(TIMESTAMP t1)
{
	// TODO: implement played schedule method
	interconnection *system = (interconnection*)get_parent();
	size_t n_area = system->get_nareas();
	size_t n_line = system->get_nlines();
	if ( verbose_options&VO_SCHEDULER )
	{
		fprintf(stderr,"SCHEDULER      : scheduling interconnection '%s' at '%s'\n", system->get_name(),(const char *)gld_clock().get_string());
		fprintf(stderr,"SCHEDULER      : n_area=%d, n_lines=%d\n", n_area, n_line);
		size_t n;
		double total = 0;
		for ( n = 0 ; n < n_area ; n++ )
		{
			controlarea *area = system->get_area(n);
			double forecast = area->get_forecast();
			fprintf(stderr,"SCHEDULER      : area %2d is %-8.8s: hour ahead forecast is %8.3f MWh\n", n+1, area->get_name(), forecast);
			total += forecast;
		}
		fprintf(stderr,"SCHEDULER      : hour ahead forecast total is %8.3f MWh\n", total);
	}
}
void scheduler::schedule_external(TIMESTAMP t1)
{
	// TODO: implement matpower schedule method
}

void scheduler::schedule_static(TIMESTAMP t1)
{
	// TODO: implement static schedule method
}

bool scheduler::schedule_open(const char *file)
{
	// load static schedule
	fh = fopen(file,"r");
	if ( fh == NULL )
	{
		error("fopen() error on file '%s': %s",file,strerror(errno));
		return false;
	}
	schedule.hour = 0;
	return true;
}

bool scheduler::schedule_next(void)
{
	schedule.hour++;
	while ( !feof(fh) && fgets(buffer,sizeof(buffer),fh)!=NULL && atoi(buffer)==schedule.hour)
	{
		debug("schedule_next() reading data: [%s]", buffer);
	}
	return errno!=0 && !feof(fh);
}




