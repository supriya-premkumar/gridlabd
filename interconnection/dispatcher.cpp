// $Id$

#include "dispatcher.h"

EXPORT_IMPLEMENT(dispatcher)
EXPORT_SYNC(dispatcher)
IMPLEMENT_ISA(dispatcher)

dispatcher::dispatcher(MODULE *module)
{
	if (oclass==NULL)
	{
		// register the class definition
		oclass = gl_register_class(module,"dispatcher",sizeof(dispatcher),PC_BOTTOMUP);

		if (oclass==NULL)
			exception("unable to register class");

		// publish the class properties
	if (gl_publish_variable(oclass,
			PT_double,"supply_ramp[MW/s]",get_supply_ramp_offset(),PT_DESCRIPTION,"supply ramp rate",
			PT_double,"demand_ramp[MW/s]",get_demand_ramp_offset(),PT_DESCRIPTION,"demand ramp rate",
		NULL) < 1)
		exception("unable to publish dispatcher properties");
	}
}

int dispatcher::create()
{
	return 1;
}

int dispatcher::init(OBJECT *parent)
{
	return 1;
}

TIMESTAMP dispatcher::presync(TIMESTAMP t1)
{
	return TS_NEVER;
}

TIMESTAMP dispatcher::sync(TIMESTAMP t1)
{
	return TS_NEVER;
}

TIMESTAMP dispatcher::postsync(TIMESTAMP t1)
{
	return TS_NEVER;
}

