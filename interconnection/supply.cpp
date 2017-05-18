// interconnection/supply.cpp
// Copyright (c) 2017, Stanford University
// dchassin@slac.stanford.edu

#include "market.h"

supply::supply(void)
{
	Pmin = DEFAULT_PMIN;
	Pmax = DEFAULT_PMAX;
	Pres = DEFAULT_PRES;
	Qres = DEFAULT_QRES;
	Punit = DEFAULT_PUNIT;
	Qunit = DEFAULT_QUNIT;
	N = 0;
	refresh();
}

void supply::reset(void)
{
	N = 0;
	C = vec ();
	refresh();
}

void supply::check(void) const
{
	string errmsg;
	if ( ! check(errmsg) )
		throw errmsg;
}

void supply::check(ostream &cout) const
{
	string errmsg;
	if ( ! check(errmsg) )
		cout << errmsg;
}

bool supply::check(string &errmsg) const
{
	size_t n = Q.index_min();

	// check for negative quantities
	if ( Q.at(n) < 0 )
	{
		errmsg = "negative quantity for segment " + std::to_string(n);
	}
	// check for negative slopes
}
void supply::refresh(bool force)
{
	if ( N == 0 )
	{
		R = vec ();
		P << Pmin << Pmax;
		dP << Pmax - Pmin;
		Q << 0 << 0;
		dQ << 0;
		S << datum::inf;
		I.clear();
		J.clear();
		Qmax = 0;
		Nlast = -1;
	}
	else if ( Nlast != N || force )
	{
		// find non-zero quantities ending above Pmin
		vec q = C.col(0);
		vec f = C.col(1);
		vec v = C.col(2);
		uvec m = find( ( f + v >= Pmin ) && ( q > 0 ) );
		vec Qc = q.elem(m);
		vec Pb = f.elem(m);
		vec dp0 = v.elem(m);
		vec Pe = Pb + dp0;
		vec s = dp0 / Qc;

		// initialize output starting at Pmin
		P << Pmin;
		dQ = vec ();
		dP = vec ();
		R = vec ();

		// find price points from lowest to highest
		vec Pp = join_cols(Pmin,Pb);
		Pp = join_cols(Pp,Pe);
		Pp = join_cols(Pp, Pmax);
		vec Pc = Pp.elem(find_unique(Pp,true));

		// process each price point -- this is the primary index to the out curve
		size_t n;
		for ( n = 0 ; n < Pc.n_elem ; n++ )
		{
			// get the price points
			double p0 =  ( n > 0 ) ? Pc[n-1] : Pmin; // last price or price floor
			double p1 = Pc[n]; // this price
			double dp = p1 - p0; // price difference

			// find each component that contributes only at this price
			uvec m0 = find(Pb==p1 && p1==Pe);
			double q0 = sum(Qc.elem(m0)); // horizontal quantity

			// find each component that contributes above this price
			uvec m1 = find(Pb<=p0 && p0<Pe);

			// if no segments added but price is different from last
			if ( m1.is_empty() && ( m0.is_empty() || p1 > P[P.n_elem-1] ) )
			{
				dQ = join_cols(dQ,0);
				P = join_cols(P,p1);
				R = join_cols(R,vec(N,1,fill::zeros));
			}
			// TODO: case where p0>Pcap needs a horizontal segment at Pcap from 0 to Qcap

			// add sloped segment if any found
			if ( ! m1.is_empty() )
			{
				double ts = 1 / sum(s(m1));
				vec r = vec(N,1,fill::zeros);
				r.elem(m1) = ts / s(m1);
				double q1 = dp / ts;
				dQ = join_cols(dQ,q1);
				P = join_cols(P,p1);
				R = join_cols(R,r);
			}

			// add a single horizontal segment if any found
			if ( ! m0.is_empty() )
			{
				dQ = join_cols(dQ,q0);
				P = join_cols(P,p1);
				vec r(N,1,fill::zeros);
				r.elem(m0) = Qc.elem(m0) / q0;
				R = join_cols(R,r);
			}

			// finalize output ending at price floor
			dQ = round(dQ/get_quantity_resolution())*get_quantity_resolution();
			P = round(P/get_price_resolution())*get_price_resolution();
			Q = join_cols(0.0,cumsum(dQ));
			dP = diff(P);
		}
		check();
	}
}
