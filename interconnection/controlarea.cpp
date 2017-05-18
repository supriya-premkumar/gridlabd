// $Id: controlarea.cpp 5185 2015-06-23 00:13:36Z dchassin $

#include "transactive.h"
#include <list>

EXPORT_IMPLEMENT(controlarea)
EXPORT_SYNC(controlarea)
EXPORT_PRECOMMIT(controlarea)
EXPORT_NOTIFY_PROP(controlarea,update)

static TIMESTAMP ace_sample_time = 4;
set controlarea::default_on_schedule_failure = SFH_NONE;

controlarea::controlarea(MODULE *module)
{
	oclass = gld_class::create(module,"controlarea",sizeof(controlarea),PC_PRETOPDOWN|PC_BOTTOMUP|PC_AUTOLOCK);
	oclass->trl = TRL_CONCEPT;
	defaults = this;
	if ( gl_publish_variable(oclass,
		PT_int64,"update",get_update_offset(),PT_HAS_NOTIFY_OVERRIDE,PT_DESCRIPTION,"incoming update handler",
		PT_double,"inertia[MJ]",get_inertia_offset(),PT_DESCRIPTION,"moment of inertia",
		PT_double,"capacity[MW]",get_capacity_offset(),PT_DESCRIPTION,"total rated generation capacity",
		PT_double,"supply[MW]",get_supply_offset(),PT_DESCRIPTION,"total generation supply",
		PT_double,"demand[MW]",get_demand_offset(),PT_DESCRIPTION,"total load demand",
		PT_double,"schedule[MW]",get_schedule_offset(),PT_DESCRIPTION,"scheduled intertie exchange",
		PT_double,"actual[MW]",get_actual_offset(),PT_DESCRIPTION,"actual intertie exchange",
		PT_double,"ace[MW]",get_ace_offset(),PT_DESCRIPTION,"area control error",
		PT_double_array,"ace_filter",get_ace_filter_offset(),PT_DESCRIPTION,"area control filter coefficients (0=P 1=I)",
		PT_double,"bias[MW/Hz]",get_bias_offset(),PT_DESCRIPTION,"frequency control bias",
		PT_double,"losses[MW]",get_losses_offset(),PT_DESCRIPTION,"line losses (internal+export)",
		PT_double,"internal_losses[pu]",get_internal_losses_offset(),PT_DESCRIPTION,"internal line losses per unit load",
		PT_double,"imbalance[MW]",get_imbalance_offset(),PT_DESCRIPTION,"area power imbalance",
		PT_double,"forecast[MWh]",get_forecast_offset(),PT_DESCRIPTION,"hour ahead energy forecast",
		PT_double,"supply_marginal_price[$/MW^2*h]",get_s_offset(),PT_DESCRIPTION,"slope of supply price curve",
		PT_double,"supply_minimum_price[$/MWh]",get_pmin_offset(),PT_DESCRIPTION,"minimum supply price (price floor)",
		PT_double,"supply_minimum_quantity[MW]",get_qw_offset(),PT_DESCRIPTION,"supply quantity at minimum price (e.g., renewables)",
		PT_double,"supply_marginal_quantity[MW]",get_qg_offset(),PT_DESCRIPTION,"additional supply quantity at maximum price (e.g., dispatchable)",
		PT_double,"demand_marginal_price[$/MW^2*h]",get_d_offset(),PT_DESCRIPTION,"slope of demand price curve (marginal price of supply)",
		PT_double,"demand_maximum_price[$/MWh]",get_pmax_offset(),PT_DESCRIPTION,"maximum demand price (price cap)",
		PT_double,"demand_maximum_quantity[MW]",get_qu_offset(),PT_DESCRIPTION,"demand quantity at maximum price (marginal price of demand)",
		PT_double,"demand_marginal_quantity[MW]",get_qu_offset(),PT_DESCRIPTION,"additional demand quantity at maximum price (e.g., curtailable load)",
		PT_enumeration,"status",get_status_offset(),PT_DESCRIPTION,"status flag",
			PT_KEYWORD,"OK",(enumeration)CAS_OK,
			PT_KEYWORD,"OVERCAPACITY",(enumeration)CAS_OVERCAPACITY,
			PT_KEYWORD,"CONSTRAINED",(enumeration)CAS_CONSTRAINED,
			PT_KEYWORD,"ISLAND",(enumeration)CAS_ISLAND,
			PT_KEYWORD,"UNSCHEDULED",(enumeration)CAS_UNSCHEDULED,
			PT_KEYWORD,"BLACKOUT",(enumeration)CAS_BLACKOUT,
		PT_set,"on_schedule_failure",get_on_schedule_failure_offset(),PT_DESCRIPTION,"flags for handling scheduling failures",
			PT_KEYWORD,"NONE", (set)SFH_NONE,
			PT_KEYWORD,"IGNORE",(set)SFH_IGNORE,
			PT_KEYWORD,"DUMP",(set)SFH_DUMP,
			PT_double,"s[$/MW^2*h]",get_s_offset(),PT_DESCRIPTION,"alias for supply_marginal_price",
			PT_double,"pmin[$/MWh]",get_pmin_offset(),PT_DESCRIPTION,"alias for supply_minimum_price",
			PT_double,"qw[MW]",get_qw_offset(),PT_DESCRIPTION,"alias for supply_minimum_quantity",
			PT_double,"qg[MW]",get_qg_offset(),PT_DESCRIPTION,"alias for supply_marginal_quantity",
			PT_double,"d[$/MW^2*h]",get_d_offset(),PT_DESCRIPTION,"alias for demand_marginal_price",
			PT_double,"pmax[$/MWh]",get_pmax_offset(),PT_DESCRIPTION,"alias for demand_maximum_price",
			PT_double,"qu[MW]",get_qu_offset(),PT_DESCRIPTION,"alias for demand_maximum_quantity",
			PT_double,"qr[MW]",get_qr_offset(),PT_DESCRIPTION,"alias for demand_marginal_quantity",
			PT_double,"dq[MW]",get_dq_offset(),PT_DESCRIPTION,"scheduled export",
			PT_double,"qd[MW]",get_qd_offset(),PT_DESCRIPTION,"scheduled demand quantity",
			PT_double,"qs[MW]",get_qs_offset(),PT_DESCRIPTION,"scheduled supply quantity",
			PT_double,"dp[$/MWh]",get_dp_offset(),PT_DESCRIPTION,"scheduled subsidy",
			PT_double,"pd[$/MWh]",get_pd_offset(),PT_DESCRIPTION,"scheduled demand price",
			PT_double,"ps[$/MWh]",get_ps_offset(),PT_DESCRIPTION,"scheduled supply price",
		NULL)<1 )
		exception("unable to publish controlarea properties");
	gl_global_create("interconnection::on_schedule_failure",
			PT_set, &default_on_schedule_failure,
			PT_KEYWORD,"NONE", (set)SFH_NONE,
			PT_KEYWORD,"IGNORE",(set)SFH_IGNORE,
			PT_KEYWORD,"DUMP",(set)SFH_DUMP,
			NULL);
	memset(defaults,0,sizeof(controlarea));
}

int controlarea::create(void)
{
	memcpy(this,defaults,sizeof(*this));
	ace_filter.set_name("ace_filter");
	set_ace_filter("0.1 0.9");
	forecast = NaN;
	on_schedule_failure = default_on_schedule_failure;
	return 1;
}

int controlarea::init(OBJECT *parent)
{
	// check that there is at least one intertie
	if ( n_intertie==0 )
	{
		if ( init_count++==0 )
			return OI_WAIT; // defer
		else if ( init_count>10 )
			warning("control area with no intertie");
		/* TROUBLESHOOT
		   TODO
		*/
	}

	// check that parent is an interconnection
	if ( parent==NULL ) exception("parent must be defined");
	system = get_object(parent);
	if ( !system->isa("interconnection") ) exception("parent must be an interconnection");

	// collect properties
	update_system = gld_property(system,"update");
	frequency = gld_property(system,"frequency");

	// register control area with interconnection
	update_system.notify(TM_REGISTER_CONTROLAREA,my());

	// update system info with SWING message
	update_system.notify(TM_AREA_STATUS,inertia,capacity,supply,demand,losses);

	// check for undefined bias
	if ( bias==0 )
		bias = capacity/nominal_frequency/droop_control + demand ;

	// check ace filter
	if ( ace_filter.get_rows()!=1 && ace_filter.get_cols()!=2 )
		exception("ace_filter must be 2 PI gains (e.g., [P,I]=\"0.9 0.1\")");

	last_update = gl_globalclock;

	// default supply and demand curves
	if ( s==0 ) s=1e12;
	if ( d==0 ) d=-1e12;

	// default forecast
	if ( isnan(forecast) )
	{
		forecast = supply - demand - losses;
	}
	A = Ainv = mat(4,4,fill::zeros);
	x = b = vec(4,fill::zeros);
	return OI_DONE;
}

int controlarea::isa(const char *type)
{
	return strcmp(type,"controlarea")==0;
}

void controlarea::add_generator(OBJECT *obj)
{
	gld_object *unit = get_object(obj);
	if ( !unit->isa("generator") )
	{
		error("add_generator(OBJECT *obj='%s'): object is not a generator", unit->get_name());
	}
	else
	{
		if ( verbose_options&VO_CONTROLAREA )
			fprintf(stderr,"CONTROLAREA    : '%s' adding generator '%s' \n",get_name(), unit->get_name());
		generator_list = add_object(generator_list,obj);
	}
}

void controlarea::add_load(OBJECT *obj)
{
	gld_object *unit = get_object(obj);
	if ( !unit->isa("load") )
	{
		error("add_load(OBJECT *obj='%s'): object is not a load", unit->get_name());
	}
	else
	{
		if ( verbose_options&VO_CONTROLAREA )
			fprintf(stderr,"CONTROLAREA    : '%s' adding load '%s' \n",get_name(), obj->name);
		load_list = add_object(load_list,obj);
	}
}

int controlarea::precommit(TIMESTAMP t1)
{
	// rebuild schedule hourly
	if ( gl_globalclock % 3600 == 0 && get_schedule_source()==SM_LOCAL )
	{
		try {
			SCHEDULESOLUTION result = update_local_schedule();
			if ( get_on_schedule_failure(SFH_IGNORE)==SFH_IGNORE )
				return true;
			else
				return result > SS_NONE;
		}
		catch (std::exception& e)
		{
			cerr << "scheduling exception: " << e.what() << "\n";
			return false;
		}
	}
	else if ( get_schedule_source()==SM_CENTRAL && gl_globalclock % (int32)central_scheduler->get_interval() )
	{
		if ( !update_central_schedule() )
		{
			error("unable to update central schedule");
			return false;
		}
	}
	else
		return true;
}

bool controlarea::update_central_schedule(void)
{
	return false;
}
SCHEDULESOLUTION controlarea::update_local_schedule(void)
{
	// reset schedule
	s = d = pmax = pmin = qu = qr = qw = qg = dp = dq = pd = qd = ps = qs = 0;

	// get the generation resources for the coming hour
	OBJECTLIST *item;
	generator *marginal_supply = NULL;
	for ( item = generator_list ; item != NULL ; item = item->next )
	{
		generator *supply = (generator*)get_object(item->obj);
		debug("reading schedule from generator '%s'", supply->get_name());
		if ( supply->get_marginal_price() == 0.0 ) // unresponsive generation
		{
			debug("'%s' is unresponsive baseload unit or renewable", supply->get_name());
			if ( qw == 0 || supply->get_fixed_price() < pmin ) // first one or lower base price than previous ones
			{
				pmin = supply->get_fixed_price(); // fixed price become the base price
				debug("'%s' is first unit; pmin=%s", supply->get_name(), (const char*)get_pmin_string());
			}
			qw += supply->get_capacity(); // add unit to unresponsive generation
			debug("qw = %s", (const char*)get_qw_string());
		}
		else if ( supply->get_marginal_price() > 0 ) // responsive generation
		{
			debug("'%s' is responsive/dispatchable supply", supply->get_name());

			// if unit has higher marginal price than previous units
			if ( supply->get_marginal_price() > s )
			{
				s = supply->get_marginal_price(); // unit becomes marginal unit
				marginal_supply = supply;
				qw += (pmax-pmin)/s; // add previous unit to base capacity
				qg = supply->get_capacity();
				pmax = pmin + s*qg; // update maximum price allowed by unit capacity
				debug("'%s' displaces existing supply as marginal supply; s=%s; qw=%s, qg=%s, pmax=%s",
						supply->get_name(), (const char*)get_s_string(), (const char*)get_qw_string(), (const char*)get_qg_string(), (const char*)get_pmax_string());
			}
			else // unit is lower marginal price than previous unit
			{
				qw += supply->get_capacity(); // add previous unit to base capacity
				debug("qw = %s", (const char*)get_qw_string());
			}
		}
		else
			warning("generator '%s' has negative marginal price %s", supply->get_name(), (const char*)supply->get_marginal_price_string());
	}

	// get load resources for the coming hour
	for ( item = load_list ; item != NULL ; item = item->next )
	{
		load *demand = (load*)get_object(item->obj);
		debug("reading schedule from load '%s'", demand->get_name());
		if ( demand->get_marginal_price() == 0.0 ) // unresponsive load
		{
			if ( qu == 0 || demand->get_fixed_price() > pmax ) // first one or higher base price than previous ones
			{
				pmax = demand->get_fixed_price(); // fixed price becomes the base price
				debug("'%s' is first demand unit; pmin=%s", demand->get_name(), (const char*)get_pmax_string());
			}
			qu += demand->get_capacity(); // add unit to unresponsive load
			debug("qu = %s", (const char*)get_qu_string());
		}
		else if ( demand->get_marginal_price() < 0 ) // responsive load
		{
			debug("'%s' is responsive/dispatchable demand", demand->get_name());

			// if unit has lower marginal price than previous units
			if ( demand ->get_marginal_price() < d )
			{
				d = demand->get_marginal_price(); // unit becomes marginal units
				qu += (pmin-pmax)/d; // add previous unit to base capacity
				qr = demand->get_capacity();
				pmin = pmax + d*demand->get_capacity();
				debug("'%s' displaces existing demand as marginal demand; d=%s; qu=%s, qr=%s, pmax=%s",
						demand->get_name(), (const char*)get_d_string(), (const char*)get_qu_string(), (const char*)get_qr_string(), (const char*)get_pmin_string());
			}
			else
			{
				qu += demand->get_capacity();
				debug("qu = %s", (const char*)get_qu_string());
			}
		}
		else
			warning("load '%s' has positive marginal price %.3f", demand->get_name(), demand->get_marginal_price());
	}

	// check resources
	if ( pmax == 0 && pmin == 0 )
	{
		warning("both pmin and pmax are zero; defaulting to price range 0-1000");
		pmax = 1000;
	}
	if ( s == 0 )
	{
		warning("s is not positive; defaulting to a nearly vertical curve");
		s = 1e6;
	}
	if ( d == 0 )
	{
		warning("d is not negative; defaulting to a nearly vertical curve");
		d = -1e6;
	}
	if ( pmax <= pmin )
	{
		error("pmax=%s is not greater than pmin=%s", (const char*)get_pmax_string(), (const char*)get_pmin_string());
		return SS_ERROR;
	}

	vec x, bc, f;
	uvec c, m;
	uvec i, n;
	mat Ac;
	if ( qw+qg >= 0.0 || qu+qr >= 0 ) // there is nonzero supply or demand in this area
	try {
		// build the controlarea schedule solver
		A	<< -d	<<  1	<<  0	<<  0 	<< -d	<<  1	<<  0	<<  0 	<< endr	// qd
			<<  0	<<  0	<< -s	<<  1 	<<  0	<<  0	<< -s	<< -1 	<< endr	// pd
			<< -1	<<  0	<<  1	<<  0 	<<  0	<<  0	<<  0	<<  0 	<< endr	// qs
			<<  0	<< -1	<<  0	<<  1	<<  0	<<  0	<<  0	<<  0	<< endr	// ps
			<<  1	<<  0	<<  0	<<  0	<<  0	<<  0	<<  0	<<  0	<< endr	// l1
			<<  0	<<  1	<<  0	<<  0	<<  0	<<  0	<<  0	<<  0	<< endr	// l2
			<<  0	<<  0	<<  1	<<  0	<<  0	<<  0	<<  0	<<  0	<< endr // l3
			<<  0	<<  0	<<  0	<<  1	<<  0	<<  0	<<  0	<<  0	<< endr // l4
			;
		// this is a simplified clearing that cannot handle price differences
		b << pmax-d*qu
		  << pmin-s*qw
		  << dq
		  << dp
		  << qu+qr
		  << pmax
		  << qw+qg
		  << pmin;
		int maxiter = 10;
		while ( x.is_empty() || m.n_elem!=c.n_elem || any(m!=c) )
		{
			if ( maxiter-- < 0 )
			{
				error("maximum number of iterations reached");
				x.clear();
				break;
			}
			c = m;
			(c-3).t().print("active constraints:");
			i = join_cols(regspace<uvec>(0,3),c);
			Ac = A.submat(i,i);
			bc = b.rows(i);
			//mat Ainv = inv(Ac);
			//x = Ainv * bc;
			x = solve(Ac,bc,solve_opts::fast);
			f << 1 << 1 << 1 << -1;
			m = find( f % x.head_rows(4) >= f % bc.head_rows(4) ) + 4;
			if ( !c.is_empty() )
			{
				n = c.elem( find(x.rows(4,x.n_elem-1)<0) );
				if ( !m.is_empty() && !n.is_empty() )
					m = setdiff(m,n);
			}
		}
		if ( verbose_options&VO_CONTROLAREA )
		{
			fprintf(stderr,"CONTROLAREA    : %s hourly update \n",get_name());
			if ( x.n_elem >= 4 )
				fprintf(stderr,"CONTROLAREA    : qd=%.3f, pd=%.3f, qs=%.3f, ps=%.3f\n",x[0],x[1],x[2],x[3]);
			else
				fprintf(stderr,"CONTROLAREA    : no solution (keeping old values)\n");
		}
	}
	catch (std::exception &e)
	{
		error("schedule_update failure: %s",e.what());
		verbose("d=%s, qu=%s, qr=%s, pmax=%s",(const char*)get_d_string(), (const char*)get_qu_string(), (const char*)get_qr_string(), (const char*)get_pmax_string());
		verbose("s=%s, qw=%s, qg=%s, pmin=%s",(const char*)get_s_string(), (const char*)get_qw_string(), (const char*)get_qg_string(), (const char*)get_pmin_string());
		verbose("dq=%s, dp=%s", (const char*)get_dq_string(), (const char*)get_dp_string());
		if ( get_on_schedule_failure(SFH_DUMP) )
		{
			i.print("i");
			A.print("A");
			b.print("b");
			x.print("x");
			m.print("m");
			n.print("n");
		}
		x.clear();
	}

	SCHEDULESOLUTION result = SS_NONE;

	// fallback method when no solution is found
	if ( x.is_empty() || x.n_elem < 4 )
	{
		if ( qu > qw+qg ) // too much unresponsive demand
		{
			verbose("solution fallback to high demand condition");
			qd = qw + qg;
			pd = pmax;
			qs = qd + dq;
			ps = pd + dp;
			result = SS_CONSTRAINED;
		}
		else if ( qu+qr < qg ) // too little total demand
		{
			verbose("solution fallback to low demand condition");
			qs = qu + qr;
			ps = pmin;
			qd = qs - dq;
			pd = ps - dp;
			result = SS_CONSTRAINED;
		}
		else // meet unresponsive demand and half of responsive
		{
			verbose("solution fallback to demand response condition");
			qd = qu + qr/2;
			pd = (pmax+pmin-dp)/2;
			qs = qd + dq;
			ps = pd + dp;
			result = SS_UNCONSTRAINED;
		}
	}
	else {
		verbose("solution is valid");
		qd = x[0];
		pd = x[1];
		qs = x[2];
		ps = x[3];
		result = x.n_elem == 4 ? SS_UNCONSTRAINED : SS_CONSTRAINED;
	}
	schedule = qs - qd;

	verbose("supply %s @ %s, demand is %s @ %s with %s %s",
			(const char*)get_qs_string(), (const char*)get_ps_string(),
			(const char*)get_qd_string(), (const char*)get_pd_string(),
			(const char*)get_schedule_string(), schedule<0?"imports":"exports");

	return result;
}

TIMESTAMP controlarea::presync(TIMESTAMP t1)
{
	// reset accumulators
	inertia = 0;
	capacity = 0;
	supply = 0;
	demand = 0;
	actual = 0;



	// update only on ACE time
	return (gl_globalclock/ace_sample_time+1)*ace_sample_time;
}

TIMESTAMP controlarea::sync(TIMESTAMP t1)
{
	// update actual
	actual = 0;
	double export_losses = 0;
	double freq_error = 0;
	if ( verbose_options&VO_CONTROLAREA )
	{
		gld_clock ts(t1);
		fprintf(stderr,"CONTROLAREA    : %s at %s: dt=%d s\n",get_name(),(const char*)ts.get_string(),t1-last_update);
		last_update = t1;
		fprintf(stderr,"CONTROLAREA    :   supply.............. %8.3f MW\n",supply);
		fprintf(stderr,"CONTROLAREA    :   demand.............. %8.3f MW\n",demand);
	}
	for ( OBJECTLIST *line=intertie_list ; line!=NULL ; line=line->next )
	{
		intertie *tie = OBJECTDATA(line->obj,intertie);

		// primary flow
		double flow = (tie->get_from()==my()?1:-1) * tie->get_flow();
		actual += flow;
		
		// add losses only on outbound schedules
		if ( flow>0 )
		{
			// compute loss factor
			double k = tie->get_loss(); // 1.0/(1.0-tie->get_loss()) - 1.0;

			if ( verbose_options&VO_CONTROLAREA )
			{
				double limit = tie->get_capacity();
				if ( flow>0 )
					fprintf(stdout,"CONTROLAREA    :   flow................ %8.3f %+8.3f %s (%s->%s)\n",
						flow, flow*k, limit>0&&abs(flow)>limit ? "!!!":"   ", tie->get_from()->name, tie->get_to()->name); 
				else
					fprintf(stdout,"CONTROLAREA    :   flow................ %8.3f %+8.3f %s (%s->%s) \n", 
						flow, flow*k, limit>0&&abs(flow)>limit ? "!!!":"   ",tie->get_to()->name, tie->get_from()->name); 
			}
			export_losses += flow*k;
		}
		else
		{
			if ( verbose_options&VO_CONTROLAREA )
			{
				double limit = tie->get_capacity();
				if ( flow>0 )
					fprintf(stdout,"CONTROLAREA    :   flow................ %8.3f          %s (%s->%s)\n",
						flow, limit>0&&abs(flow)>limit ? "!!!":"   ",tie->get_from()->name, tie->get_to()->name); 
				else
					fprintf(stdout,"CONTROLAREA    :   flow................ %8.3f          %s (%s->%s)\n",
						flow, limit>0&&abs(flow)>limit ? "!!!":"   ",tie->get_to()->name, tie->get_from()->name); 
			}
		}
	}

	// finalize imbalance and total losses
	losses = export_losses + demand*internal_losses;
	imbalance = supply-demand-losses-actual;

	// update system info with SWING message
	update_system.notify(TM_AREA_STATUS,inertia,capacity,supply,demand,losses);

	// update controlarea status flag
	if ( supply>capacity*1.01 ) status = CAS_OVERCAPACITY;
	else if ( supply>capacity*0.99 ) status = CAS_CONSTRAINED;
	else if ( schedule<capacity*0.001 ) status = CAS_UNSCHEDULED;
	else if ( actual<capacity*0.001 ) status= CAS_ISLAND;
	else if ( demand<capacity*0.001 && supply<capacity*0.001 && fabs(schedule)>capacity*0.001 ) status = CAS_BLACKOUT;
	else status = CAS_OK;

	// calculate ACE only every 4 seconds
	if ( gl_globalclock%ace_sample_time==0 )
	{
		freq_error = bias * ( nominal_frequency - frequency.get_double() );
		if ( true ) // actual!=0 ) // && schedule!=0 )
		{
			ace = ace_filter[0][1]*ace + ace_filter[0][0]*((actual + export_losses - schedule) + freq_error);
			if ( !isfinite(ace) )
			{
				gl_warning("ACE for node %s is not a finite number, using 0.0 instead", get_name());
			/* TROUBLESHOOT
			   TODO
			*/
				ace = 0.0;
			}
		}
	}

	if ( verbose_options&VO_CONTROLAREA )
	{
		fprintf(stdout,"CONTROLAREA    :   actual.............. %8.3f MW\n",actual);
		fprintf(stdout,"CONTROLAREA    :   schedule............ %8.3f MW\n",schedule);
		fprintf(stdout,"CONTROLAREA    :   losses.............. %8.3f MW\n",losses);
		fprintf(stdout,"CONTROLAREA    :   frequency error..... %8.3f MW\n",freq_error);
		fprintf(stdout,"CONTROLAREA    :   total ace........... %8.3f MW\n",ace);
	}

	return (gl_globalclock/ace_sample_time+1)*ace_sample_time;
}

TIMESTAMP controlarea::postsync(TIMESTAMP t1)
{
	error("postsync not implemented");
		/* TROUBLESHOOT
		   TODO
		*/
	return TS_INVALID;
}

int controlarea::notify_update(const char *message)
{
	trace_message(TMT_CONTROLAREA,message);

	// ACTUAL GENERATION
	double rated, power, energy;
	if ( sscanf(message,TM_ACTUAL_GENERATION,&rated,&power,&energy)==3 )
	{
		capacity += rated;
		supply += power;
		inertia += energy;
		return 1;
	}

	// ACTUAL LOAD
	if ( sscanf(message,TM_ACTUAL_LOAD,&power,&energy)==2 )
	{
		demand += power;
		inertia += energy;
		return 1;
	}


	// REGISTER INTERTIE
	OBJECT *obj;
	if ( sscanf(message,TM_REGISTER_INTERTIE,&obj)==1 )
	{
		intertie_list = add_object(intertie_list,obj);
		gld_object *line = get_object(obj);
		if ( !line->isa("intertie") ) 
			exception("attempt by non-intertie object to register with interconnection as an intertie");
		n_intertie++;
		return 1;
	}

	error("update message '%s' is not recognized",message);
		/* TROUBLESHOOT
		   TODO
		*/
	return 0;
}

int controlarea::kmldump(int (*stream)(const char*, ...))
{
	if ( isnan(get_latitude()) || isnan(get_longitude()) ) return 0;
	stream("<Placemark>\n");
	stream("  <name>%s</name>\n", get_name());
	stream("  <description>\n<![CDATA[<TABLE BORDER=1 CELLSPACING=0 CELLPADDING=3 STYLE=\"font-size:10;\">\n");
#define TR "    <TR><TH ALIGN=LEFT>%s</TH><TD ALIGN=RIGHT>%s</TD></TR>\n"
#define HREF "    <TR><TH ALIGN=LEFT><A HREF=\"%s_%s.png\"  ONCLICK=\"window.open('%s_%s.png');\">%s</A></TH><TD ALIGN=RIGHT>%s</TD></TR>\n"
	gld_clock now(my()->clock);
	stream("<caption>%s</caption>",(const char*)now.get_string());
	stream(TR,"Status",(const char*)get_status_string());
	stream(HREF,(const char*)get_name(),"actual",(const char*)get_name(),"actual","Actual",(const char*)get_actual_string());
	stream(HREF,(const char*)get_name(),"schedule",(const char*)get_name(),"schedule","Schedule",(const char*)get_schedule_string());
	stream(HREF,(const char*)get_name(),"capacity",(const char*)get_name(),"capacity","Capacity",(const char*)get_capacity_string());
	stream(HREF,(const char*)get_name(),"supply",(const char*)get_name(),"supply","Supply",(const char*)get_supply_string());
	stream(HREF,(const char*)get_name(),"demand",(const char*)get_name(),"demand","Demand",(const char*)get_demand_string());
	stream(HREF,(const char*)get_name(),"losses",(const char*)get_name(),"losses","Losses",(const char*)get_losses_string());
	stream(HREF,(const char*)get_name(),"imbalance",(const char*)get_name(),"imbalance","Imbalance",(const char*)get_imbalance_string());
	stream(HREF,(const char*)get_name(),"inertia",(const char*)get_name(),"inertia","Inertia",(const char*)get_inertia_string());
	stream(HREF,(const char*)get_name(),"bias",(const char*)get_name(),"bias","Bias",(const char*)get_bias_string());
	stream("  </TABLE>]]></description>\n");
	stream("  <styleUrl>#%s_mark_%s</styleUrl>\n",my()->oclass->name, (const char*)get_status_string());
	stream("  <Point>\n");
	stream("    <altitudeMode>relative</altitudeMode>\n");
	stream("    <coordinates>%f,%f,100</coordinates>\n", get_longitude(), get_latitude());
	stream("  </Point>\n");
	stream("</Placemark>\n");
#ifdef SHOWUNITS
	stream("<Folder><name>%s</name>\n","Generators");
	for ( OBJECT *obj=callback->object.get_first() ; obj!=NULL ; obj=obj->next )
	{
		gld_object *item = get_object(obj);
		if ( obj->oclass->module==my()->oclass->module && obj->parent==my() && item->isa("generator") )
			OBJECTDATA(obj,generator)->kmldump(stream);
	}
	stream("%s","</Folder>\n");

	stream("<Folder><name>%s</name>\n","Loads");
	for ( OBJECT *obj=callback->object.get_first() ; obj!=NULL ; obj=obj->next )
	{
		gld_object *item = get_object(obj);
		if ( obj->oclass->module==my()->oclass->module && obj->parent==my() && item->isa("load") )
			OBJECTDATA(obj,load)->kmldump(stream);
	}
	stream("%s","</Folder>\n");
#endif
	return 1;
}
