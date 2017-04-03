/*
 * ceus.h
 *
 *  Created on: Apr 1, 2017
 *      Author: dchassin@slac.stanford.edu
 */

#ifndef COMMERCIAL_CEUS_H_
#define COMMERCIAL_CEUS_H_

#include "gridlabd.h"

// =INT((StudyMonth-1)/3+1)*1000 + CEUSdaycode*100 + StudyHour+1
#define CEUSCODE(M,D,H) (((M-1)/3+1)*1000+D*100+H+1)

class ceus : public gld_object {
	typedef enum {
		SMALL_OFFICE,
		LARGE_OFFICE,
		RETAIL,
		LODGING,
		GROCERY,
		RESTAURANT,
		SCHOOL,
		HEALTH,
		_NUM_BUILDINGCLASSES,
		BC_UNKNOWN
	} BUILDINGCLASS;
	typedef enum {
		WINTER,
		SPRING,
		SUMMER,
		FALL,
		_NUM_CEUSMONTH
	} CEUSMONTH;
	typedef enum {
		WEEKEND, // weekend
		COOLPEAK, // <10 below normal temperature
		AVGPEAK, // normal temperature
		HOTPEAK, // >10 above normal tempeature
		_NUM_DAYTYPES
	};
	typedef enum {
		HEATING,
		COOLING,
		VENTILATION,
		HOTWATER,
		COOKING,
		REFRIGERATION,
		OUTDOOR_LIGHTS,
		INDOOR_LIGHTS,
		OFFICE_EQUIPMENT,
		MISCELLANEOUS,
		PROCESS,
		MOTORS,
		COMPRESSORS,
		TOTAL,
		_NUM_ENDUSES,
		EU_UNKNOWN
	} CEUSENDUSE;
	typedef struct s_composition {
		double density;
		double demand;
		double electronic;
		double motorA;
		double motorB;
		double motorC;
		double motorD;
		complex Z;
		complex I;
		complex P;
		double pf;
		complex total;
		double check; // fractional totals
		double area[_NUM_ENDUSES]; // floor areas
		double ceuscode[_NUM_CEUSMONTH][_NUM_DAYTYPES][24]; // ceuscodes
		double data[_NUM_CEUSMONTH][_NUM_DAYTYPES][24][_NUM_ENDUSES]; // shape data
	} COMPOSITION;

private:
	GL_ATOMIC(char1024,datafile); // data file
	GL_ATOMIC(object,weather); // weather referencd
	GL_ATOMIC(complex,power); // power in kW

	GL_ATOMIC(double,small_office); // floor area
	GL_ATOMIC(double,large_office); // floor area
	GL_ATOMIC(double,retail); // floor area
	GL_ATOMIC(double,lodging); // floor area
	GL_ATOMIC(double,grocery); // floor area
	GL_ATOMIC(double,restaurant); // floor area
	GL_ATOMIC(double,school); // floor area
	GL_ATOMIC(double,health); // floor area

	GL_ATOMIC(double,temperature_sensitivity);
	GL_ATOMIC(char32,temperature_source);

	GL_ATOMIC(double,price_sensitivity);
	GL_ATOMIC(object,price_source);

	GL_ATOMIC(double,frequency_sensitivity);
	GL_ATOMIC(object,frequency_source);

private:
	COMPOSITION composition[_NUM_BUILDINGCLASSES];
	gld_property temperature;
	gld_property price;
	gld_property frequency;

private:
	bool load_datafile(const char *pathname);

public:
	ceus(MODULE *);
	~ceus(void);

public:
	/* required implementations */
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP precommit(TIMESTAMP t);

public:
	static CLASS *oclass;
	static ceus *defaults;
};

#endif /* COMMERCIAL_CEUS_H_ */
