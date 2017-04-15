// $Id: generator.h 5185 2015-06-23 00:13:36Z dchassin $

#ifndef _GENERATOR_H
#define _GENERATOR_H

class generator : public gld_object {
public:
	typedef enum {
		GCF_NONE = 0x00, // no control
		GCF_DROOP = 0x01, // droop-only control
		GCF_AGC = 0x02, // AGC control
	} GENERATORCONTROLFLAG;
	typedef enum {
		GS_OFFLINE, ///< generation is offline
		GS_STARTING,
		GS_STOPPING,
		GS_MINIMUM, ///< generation is running at minimum allowed
		GS_RUNNING, ///< generation is running ok
		GS_MAXIMUM, ///< generation is running at maximum allowed
		GS_OVERCAP, ///< generation is over capacity
	} GENERATORSTATUS;
public:
	GL_ATOMIC(int64,update); ///< pseudo variable for receiving messages from controlarea
	GL_ATOMIC(double,inertia); ///< total inertia of generator
	GL_ATOMIC(double,capacity); ///< rated capacity of generator
	GL_ATOMIC(double,schedule); ///< scheduled generator power
	GL_ATOMIC(double,actual); ///< actual generator power
	GL_ATOMIC(enumeration,status); /// generation status flags (GS_*)
	GL_ATOMIC(double,max_ramp); ///< maximum ramp up capability
	GL_ATOMIC(double,min_ramp); ///< maximum ramp dn capability
	GL_ATOMIC(double,min_output);
	GL_ATOMIC(double,max_output);
	GL_ATOMIC(double,control_gain); ///< generator control gain
	GL_ATOMIC(set,control_flags); ///< generation control flags (GCF_*)
	GL_ATOMIC(double,frequency_deadband); ///< frequency control deadband
	GL_STRUCT(double_array,A); ///< plant model A matrix
	GL_STRUCT(double_array,B); ///< plant model B matrix
	GL_STRUCT(double_array,C); ///< plant model C matrix
	GL_STRUCT(double_array,D); ///< plant model D matrix
	GL_STRUCT(double_array,X); ///< plant state vector X
	GL_STRUCT(double_array,dX); ///< plant state vector dX
	GL_STRUCT(double_array,Y); /// plant output vector Y
	GL_STRUCT(double_array,K); /// plant control gain K
	GL_STRUCT(double_array,Z); /// plant zeros
	GL_STRUCT(double_array,P); /// plant poles
private:
	gld_object *area;
	gld_property update_area;
	controlarea *control;
public:
	DECL_IMPLEMENT(generator);
	DECL_SYNC;
	DECL_NOTIFY(update);
	int kmldump(int (*stream)(const char*, ...));
};

#endif // _GENERATOR_H
