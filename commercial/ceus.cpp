/*
 * ceus.cpp
 *
 *
 *  Created on: Apr 1, 2017
 *      Author: dchassin@slac.stanford.edu
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <complex.h>

#include "ceus.h"

EXPORT_CREATE(ceus);
EXPORT_INIT(ceus);
EXPORT_PRECOMMIT(ceus);

CLASS *ceus::oclass = NULL;
ceus *ceus::defaults = NULL;

ceus::ceus(MODULE *module)
{
	if (oclass==NULL)
	{
		// register to receive notice for first top down. bottom up, and second top down synchronizations
		oclass = gld_class::create(module,"ceus",sizeof(ceus),PC_AUTOLOCK|PC_OBSERVER);
		if (oclass==NULL)
			throw "unable to register class ceus";
		else
			oclass->trl = TRL_PROVEN;

		defaults = this;
		if (gl_publish_variable(oclass,
				PT_object,"weather",get_weather_offset(),PT_DESCRIPTION,"reference to weather data",
				PT_char1024,"datafile",get_datafile_offset(),PT_DESCRIPTION,"data file pathname",
				PT_double,"small_office[ksf]",get_small_office_offset(),PT_DESCRIPTION,"small office floor area",
				PT_double,"large_office[ksf]",get_large_office_offset(),PT_DESCRIPTION,"large office floor area",
				PT_double,"retail[ksf]",get_retail_offset(),PT_DESCRIPTION,"retail floor area",
				PT_double,"lodging[ksf]",get_lodging_offset(),PT_DESCRIPTION,"lodging floor area",
				PT_double,"grocery[ksf]",get_grocery_offset(),PT_DESCRIPTION,"grocery floor area",
				PT_double,"restaurant[ksf]",get_restaurant_offset(),PT_DESCRIPTION,"restaurant floor area",
				PT_double,"school[ksf]",get_school_offset(),PT_DESCRIPTION,"school floor area",
				PT_double,"health[ksf]",get_health_offset(),PT_DESCRIPTION,"health floor area",
				PT_double,"temperature_sensitivity[kW/degF]",get_temperature_sensitivity_offset(),PT_DESCRIPTION,"temperature sensitivity",
				PT_char32,"temperature_source",get_temperature_source_offset(),PT_DESCRIPTION,"temperature source",
				PT_double,"price_sensitivity[kW*MWh/$]",get_price_sensitivity_offset(),PT_DESCRIPTION,"price sensitivity",
				PT_char32,"price_source",get_price_source_offset(),PT_DESCRIPTION,"price source",
				PT_double,"frequency_sensitivity[kW/Hz]",get_frequency_sensitivity_offset(),PT_DESCRIPTION,"frequency sensitivity",
				PT_char32,"frequency_source",get_frequency_source_offset(),PT_DESCRIPTION,"frequency source",
				NULL)<1)
		{
			char msg[256];
			sprintf(msg, "unable to publish properties in %s",__FILE__);
			throw msg;
		}
		memset(this,0,sizeof(ceus));
	}
}

int ceus::create(void)
{
	memcpy(this,defaults,sizeof(*this));

	return 1; /* return 1 on success, 0 on failure */
}

int ceus::init(OBJECT *parent)
{
	if ( weather==NULL )
	{
		FINDLIST *list = gl_find_objects(FL_NEW,FT_CLASS,SAME,"climate",FT_END);
		if ( list==NULL || list->hit_count==0 )
		{
			gl_error("unable to find any climate objects");
			return 0;
		}
		weather = gl_find_next(list,NULL);
		if ( gl_find_next(list,weather)!=NULL )
			gl_warning("more than one climate object found, using first as default");
	}
	if ( strcmp(weather->oclass->name,"climate")!=0 )
	{
		gl_error("weather object '%s' is not of class 'climate'",gl_name(weather,NULL,NULL));
		return 0;
	}
	gl_debug("weather object '%s' is ok",gl_name(weather,NULL,NULL));
#define DEFAULT_CEUSDATAFILE "ceus.csv"
	if ( strcmp(datafile,"")==0 && gl_findfile(DEFAULT_CEUSDATAFILE,NULL,R_OK,datafile,sizeof(datafile))==NULL )
	{
		gl_error("unable to find default ceus datafile '%s'",DEFAULT_CEUSDATAFILE);
		return 0;
	}
	if ( ! load_datafile(datafile) )
	{
		gl_error("unable to load ceus datafile '%s'", (const char *)datafile);
		return 0;
	}
	return 1;
}

TIMESTAMP ceus::precommit(TIMESTAMP t1)
{
	// TODO
	return TS_NEVER;
}

bool ceus::load_datafile(const char *pathname)
{
	size_t linenum=0; // line number
	FILE *fp = fopen(pathname,"rt");
	if ( fp==NULL )
	{
		gl_error("ceus datafile '%s' open failed: %s", pathname, strerror(errno));
		return false;
	}

	gl_debug("%s(%d): starting load", pathname,linenum);
	char *line; // input line
	size_t len=0; // input line len
	BUILDINGCLASS building_class = BC_UNKNOWN; // class that is being processed
	CEUSENDUSE enduse = EU_UNKNOWN; // enduse that is being processed
	while ( !feof(fp) && !ferror(fp) && (line=fgetln(fp,&len)) != NULL )
	{
#define IFTOKEN(X) else if (strncmp(line,X",",strlen(X","))==0)
		linenum++;
		if ( len<0 )
		{
			gl_error("error reading datafile, line length is negative (len=%d)", len);
			break;
		}
		if ( len>0 ) len--; // remove newline
		line[len]='\0'; // truncate data
		gl_debug("%s(%d): input len=%d; line=[%-*.*s]",pathname,linenum,len,len,len,line);
		if ( strcmp(line,"")==0 )
			continue;
		IFTOKEN("CLASS")
		{
			char name[100];
			if ( sscanf(line,"CLASS,%99[^,]",name)==1 )
			{
#define	HANDLE(X) if (strcmp(name,#X)==0 ) {building_class = X;} else
				HANDLE(SMALL_OFFICE)
				HANDLE(LARGE_OFFICE)
				HANDLE(RETAIL)
				HANDLE(LODGING)
				HANDLE(GROCERY)
				HANDLE(RESTAURANT)
				HANDLE(SCHOOL)
				HANDLE(HEALTH)
#undef HANDLE
				{
					gl_error("%s(%d): class name '%s' is not recognized",pathname,linenum,name);
					break;
				}
			}
			else
			{
				gl_error("%s(%d): unable to parse CLASS line '%s'", pathname,linenum,line);
				break;
			}
			gl_debug("%s(%d): setting building type to '%s'", pathname,linenum,name);
		}
		IFTOKEN("HEADER")
		{
			if ( strcmp(line,"HEADER,ENDUSE,DENSITY,DEMAND,ELECTRONIC,MOTORA,MOTORB,MOTORC,MOTORD,IP,IQ,PP,PQ,ZP,ZQ,PF")!=0
					&& strcmp(line,"HEADER,CEUSCODE,HEATING,COOLING,VENTILATION,HOTWATER,COOKING,REFRIGERATION,OUTDOOR_LIGHTS,INDOOR_LIGHTS,OFFICE_EQUIPMENT,MISCELLANEOUS,PROCESS,MOTORS,COMPRESSORS,TOTAL")!=0 )
			{
				gl_error("%s(%d): header is not recognized",pathname,linenum);
				break;
			}
			gl_debug("%s(%d): header is ok",pathname,linenum);
		}
		IFTOKEN("COMPOSITION")
		{
			char enduse_name[100];
			COMPOSITION lc;
			if ( sscanf(line,"COMPOSITION,%99[^,],%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g",
					enduse_name, &lc.density, &lc.demand,
					&lc.electronic, &lc.motorA, &lc.motorB, &lc.motorC, &lc.motorD,
					&lc.Z.Re(), &lc.Z.Im(), &lc.I.Re(), &lc.I.Im(), &lc.P.Re(), &lc.P.Im(), &lc.pf)!=15 )
			{
				gl_error("%s(%d): composition is missing data",pathname,linenum);
				break;
			}
#define	HANDLE(X) if (strcmp(enduse_name,#X)==0 ) {enduse = X;} else
			HANDLE(HEATING)
			HANDLE(COOLING)
			HANDLE(VENTILATION)
			HANDLE(HOTWATER)
			HANDLE(COOKING)
			HANDLE(REFRIGERATION)
			HANDLE(OUTDOOR_LIGHTS)
			HANDLE(INDOOR_LIGHTS)
			HANDLE(OFFICE_EQUIPMENT)
			HANDLE(MISCELLANEOUS)
			HANDLE(PROCESS)
			HANDLE(MOTORS)
			HANDLE(COMPRESSORS)
			HANDLE(TOTAL)
#undef HANDLE
			{
				gl_error("%s(%d): enduse '%s' is not recognized",pathname,linenum,enduse_name);
				break;
			}
			gl_debug("%s(%d): composition %s is ok", pathname,linenum,enduse_name);
		}
		IFTOKEN("NORMALIZE")
		{
			double *area = &(composition[building_class].area[enduse]);
			if ( sscanf(line,"NORMALIZE,FLOORAREA,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g",
					// TODO fix so it is automatically reading _NUM_ENDUSES %g
					area,area+1,area+2,area+3,area+4,area+5,area+6,area+7,area+8,area+9,area+10,area+11,area+12,area+13
					)!=_NUM_ENDUSES )
			{
				gl_error("%s(%d): floorarea normalization is missing data",pathname,linenum);
				break;
			}
			gl_debug("%s(%d): floorarea normalization is ok", pathname,linenum);
			// TODO
		}
		IFTOKEN("DATA")
		{
			unsigned int month,day,hour;
			double data[_NUM_ENDUSES];
			if ( sscanf(line,"DATA,%1d%1d%2d,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g",
					&month,&day,&hour,
					// TODO fix so it is automatically reading _NUM_ENDUSES %g
					data,data+1,data+2,data+3,data+4,data+5,data+6,data+7,data+8,data+9,data+10,data+11,data+12,data+13
					)!=_NUM_ENDUSES+3 )
			{
				gl_error("%s(%d): ceusdata is missing data",pathname,linenum);
				break;
			}
			if ( month<1 || month>_NUM_CEUSMONTH )
			{
				gl_error("%s(%d): ceus month %d is invalid",pathname,linenum,month);
				break;
			}
			if ( day<1 || day>_NUM_DAYTYPES )
			{
				gl_error("%s(%d): ceus daytype %d is invalid",pathname,linenum,day);
				break;
			}
			if ( hour<1 || hour>24 )
			{
				gl_error("%s(%d): ceus hour %d is invalid",pathname,linenum,hour);
				break;
			}
			gl_debug("%s(%d): month=%d, day=%d, hour=%d, data is ok", pathname,linenum,month,day,hour);
			memcpy(composition[building_class].data[--month][--day][--hour],data,sizeof(data));
		}
		else
		{
			gl_error("%s(%d): ceus datafile line '%s' is not recognized", pathname,linenum,line);
			break;
		}
	}
Done:
	if ( ferror(fp) )
	{
		gl_error("%s(%d): unable to read ceus datafile: %s", pathname, linenum, strerror(errno));
		fclose(fp);
		return false;
	}
	else
	{
		gl_verbose("%s(%d): ceus data load ok", pathname, linenum);
		fclose(fp);
		return true;
	}
}
