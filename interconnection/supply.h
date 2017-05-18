// interconnection/supply.h
// Copyright (c) 2017, Stanford University
// dchassin@slac.stanford.edu
//
// A supply curve is a series of line segments extending from the price
// floor at quantity zero to the price cap at the maximum total quantity
// possible.  The function to add components is 'add_components' and the
// function to obtain the segments is 'get_segments'.
//
// Supply curves are constructed from a series of supply components,
// each of which has a quantity, a fixed price and a variable price.
// The component's contribution to the supply curve can be thought of as
// a ray extending from no quantity '0' at the fixed price 'f' to the
// quantity 'q' at the variable price 'f+v', where 'v' is positive.
//
// A supply curve is constructed by superimposing all the quantities
// over their respective price ranges. For example, the components
//
//   A = [10,100,10] and B = [30,95,20]
//
// would give the supply curve (assuming Pmax=1000 and Pmin=0)
//
//   Q = [  0,  0, 7.5, 32.5,  40,   40]
//   P = [  0, 95, 100,  110, 115, 1000]
//
// The supply curve includes additional information that can be useful
// to analysis and processing of markets.  This includes
//
// Slopes (S)
//
//   These values correspond to the slopes of each line segments,
//   including Inf values where appropriate. Note that there are 1 fewer
//   slopes than entries in the Q and P arrays because the slopes are
//   obtained using the computation
//
//     S = diff(P)./diff(Q);
//
//   In the above example, the slopes obtained are
//
//     S = [Inf, 0.6667, 0.4000, 0.6667, Inf]
//
// Contributions (R)
//
//   These values correspond to the fractional contribution of
//   components to each segment. Each component is listed in the order
//   that is was submitted to 'add_components'.  Components are in rows
//   and segments are in columns.
//
//   In the above example, the contributions are
//
//     R = [ 0, 1.0000, 0.2500,      0, 0 ;
//           0,      0, 0.7500, 1.0000, 0 ]
//
// Component Index (I)
//
//    The indexes of components contributing to each segments. This is
//    obtained by finding all the columns of R that are non-zero for
//    each component.
//
//    In the above example, the component indexes to segments are
//
//      I{1} = [2,3]
//      I{2} = [3,4]
//
// Segment index (J)
//
//    The indexes of segments contributed by each components.  This is
//    obtained by finding all the rows of R that are non-zero for each
//    segment.
//
//    In the above example, the segments indexes to components are
//
//      J{1} = []
//      J{2} = [1]
//      J{3} = [1,2]
//      J{4} = [2]
//      J{5} = []
//
// Bugs:
//
// 1) If any component goes outside range of [Pmin,Pmax] the curve may
//    not be constructed properly. A warning will be emitted by any
//    'add_component' calls that goes outside the allowed price range.
//

#ifndef _SUPPLY_H
#define _SUPPLY_H

#include "market.h"

typedef struct s_component {
	double q; // quantity
	double f; // fixed price
	double v; // variable price
} COMPONENT;

class supply {
private:
	double Pmin; // minimum price
	double Pmax; // maximum price
	int Pres; // price resolution
	int Qres; // quantity resolution
	string Punit; // price units
	string Qunit; // quantity units
public:
	inline double get_minumum_price(void) const { return Pmin; };
	inline double get_maximum_price(void) const { return Pmax; };
	inline int get_price_precision(void) const { return Pres; };
	inline int get_quantity_precision(void) const { return Qres; };
	inline double get_price_resolution(void) const { return pow(10,(double)Pres); };
	inline double get_quantity_resolution(void) const { return pow(10,(double)Qres); };
	inline string get_price_unit(void) const { return Punit; };
	inline string get_quantity_unit(void) const { return Qunit;};
public:
	inline double set_minimum_price(double p) { double _t=Pmin; Pmin=p; return _t; };
	inline double set_maximum_price(double p) { double _t=Pmax; Pmax=p; return _t; };
	inline int set_price_precision(int p) { int _t=Pres; Pres=p; return _t; };
	inline int set_quantity_precision(int p) { int _t=Qres; Qres=p; return _t; };
	inline string set_price_unit(string &p) { string _t=Punit; Punit=p; return _t; };
	inline string set_quantity_unit(string &p) { string _t=Qunit; Qunit=p; return _t; };
private:
	size_t N; // number of components
	mat C; // components list (q,f,v)
	vec R; // component contribution factors
	vec P; // price inflection points
	vec dP; // incremental prices of segments
	vec Q;
	vec dQ;
	vec S;
	list<vec> I;
	list<vec> J;
	double Qmax;
	size_t Nlast;
public:
	supply(void);

    // Reset the supply curve to an empty curve
	void reset(void);

    // Check the supply curve for erroneous results
	//
	// Calls errfn, throws exception (if errfn==NULL), or returns false when check fails.
	//
	void check(void) const;
	void check(ostream &cout) const;
	bool check(string &errmsg) const;

    // Regenerate segment data
    //
	// Input:
    //   force=true forces complete refresh
    //
    // Note: the curve is ordered from lowest price to highest.
	//
	void refresh(bool force=false);

    // Update the slopes and index tables
    //
    // This function updates the internal structure of the supply
    // curve given the price and quantity differentials, 'dP' and
    // 'dQ'. Any 0/0 values of dP/dQ is removed from the curve.
    // Updated values include the slope 'S', the prices 'P',
    // quantities 'Q', their differentials 'dP' and 'dQ
    // respectively, the slopes 'S', the contribution factors 'R',
    // the maximum quantity 'Qmax', the segment and components
    // indexes, 'I' and 'J'. When the curve is up to date, the
    // values of 'N' and 'Nlast' are equal.
    //
	void update(void);

    // Add a component to the supply curve
    //
    // Inputs:
    //   q = quantities (MW)
    //   f = fixed price component ($/MW.h)
    //   v = variable price component ($/MW.h)
    //   h = surplus inclusion factor (default 1.0)
    //   c = [q,f,v,s] component block data
    //
    // Outputs
    //   n = component id
    //
    // The supply component is added starting at the price f, with
    // the quantity q increasing until the price f+v is reached.
    // Note that the value of v must be positive.  If you only have
    // the slope s of the component, then you should use the call
    //
    //   d = supply_curve.add_components(q,f,q.*s)
	size_t add_components(vec q, vec f, vec v, vec h);
	size_t add_components(mat c);

	// Get components that clear at value 'v'
	//
	//   [c,n,r,s] = supply_curve.get_components_in(v,t,z)
	//
	// Inputs:
	//   v   value (optional, default is quantity 0)
	//   t   type (optional, default is price)
	//   z   set (optional, default is 'in');
	//   lim flow limit (optional, default is no limit)
	//
	// Outputs:
	//   c   components by rows [q,f,v] (see add_components)
	//   n   index into components of 'd' (see C)
	//   r   fractions of components used
	//   s   slopes of components
	//
	// This function returns the portion of the supply curve that
	// falls in or out at the specific price or quantity specified
	// by the value 'v'. The interpretation of 'v' is determined by
	// the value 't', which may be either 'price' (default) or
	// 'quantity'.  The portion of the curve that is returned is by
	// default 'in', unless 'z' is specified as 'out'.
	//
	// If the flow limit 'lim' is specified, the supply curve will
	// be truncated so that the total quantity does not exceed the
	// flow limit.
	mat get_components(double v, bool is_price=true, bool is_inside=true, double limit=0) const;

    // Get the segments of the curve
    //
    //   [p,q,r,s] = supply_curve.get_segments(n)
    //
    // Inputs:
    //   n   (optional) index of segments desired
    //
    // Outputs:
    //   p   price points
    //   q   quantity points
    //   r   component contribution factors of each segment
    //   s   slopes of each segments
    //
    // This function return the segments data for the supply curve.
    // If 'n' is not specified all segments are returned.
	mat get_segments(size_t n=-1) const;

    // Add segments from another curve
    //
    //   d = supply_curve.add_segments(e,v,t,z)
    //
    // Inputs:
    //   e   supply curve from which to take segments
    //   v   value at which to begin taking segments
    //   t   type of value, i.e., 'price' or 'quantity' (optional,
    //       default is 'price')
    //   z   portion of curve from which to take segments (optional,
    //       defaults is 'out')
    //
    // Outputs:
    //    d   new supply curve constructed from combined curves
    //
    // This function adds the segments from the curve 'e' to the
    // curve 'd' beginning at the value 'v'.  The value 't'
    // specifies whether 'v' is a 'price' or a 'quantity'.  The
    // value 'z' specifies whether the 'in' or 'out' portion of the
    // curve is to be added.
	void add_segments(supply *e, double v, bool is_price=true, bool is_outside=true);

    // Get the prices for the given quantity
    //
    //   [p,q] = supply_curve.get_price(q,t,dq)
    //
    // Inputs:
    //
    //   q   quantity at which prices is desired
    //   t   (optional) type of aggregation
    //   dq  (optional) quantity offset to apply to supply curve
    //
    // Outputs:
    //   p   price(s) found, if any
    //   q   quantity(ies) found
    //   n   fraction of marginal segment taken
    //   f   number of marginal segment
    //
    // Only a single quantity can be requested.  Valid types are any
    // unary functions which takes a list as an input and returns a
    // single value as an output, e.g., 'min', 'max', 'mean', 'std'.
	mat get_price(double q, double (*t)(vec)=NULL, double dq=0) const;

    // Get the quantities for the given price
    //
    //   [p,q] = supply_curve.get_quantity(p,t,dq)
    //
    // Inputs:
    //
    //   p   price at which quantity is desired
    //   t   (optional) type of aggregation
	//   dq  (optional) quantity offset to apply to supply curve
    //
    // Outputs:
    //   p   price(s) found, if any
    //   q   quantity(ies) found
    //   n   fraction of marginal segment taken
    //   f   number of marginal segment
    //
    // Only a single price can be requested.  Valid types are any
    // unary functions which takes a list as an input and returns a
    // single value as an output, e.g., 'min', 'max', 'mean', 'std'.
	mat get_quantity(double p, double (*t)(vec)=NULL, double dq=0) const;

    // Get curve to the right of a quantity
    //
    //   c = supply_curve.get_right(q)
    //
    // Inputs:
    //   q   quantity at which to begin curve
    //
    // Outputs:
    //   c   new supply curve
    //
    // Obtains the supply curve for all quantities greater than or
    // equal to 'q'.  All prices at 'q' are included.
	mat get_right(double q) const;

    // Get curve below a price
    //
    //   c = supply_curve.get_below(d,p)
    //
    // Input:
    //   p   price below which curve is to be obtained
    //
    // Output:
    //   c   the new supply curve
    //
    // This function produces a new supply curve using only the
    // portion of the original supply that is below or equal to the
    // price 'P'.
	mat get_below(double p) const;

    // Get curve to the left of a quantity
    //
    //   c = supply_curve.get_left(q)
    //
    // Inputs:
    //   q   quantity at which to begin curve
    //
    // Outputs:
    //   c   new supply curve
    //
    // Obtains the supply curve for all quantities less than or
    // equal to 'q'.  All prices at 'q' are included.
	mat get_left(double q) const;

    // Get curve above a price
    //
    //   c = supply_curve.get_above(p)
    //
    // Input:
    //   p   price above which curve is to be obtained
    //
    // Output:
    //   c   the new supply curve
    //
    // This function produces a new supply curve using only the
    // portion of the original supply that is above or equal to the
    // price 'P'.
	mat get_above(double p) const;

    // Compute the consumer surplus
    //
    //   s = supply_curve.surplus(p,n)
    //
    // Inputs:
    //   p   price at which surplus is to be calculated
    //   n   components to include in surplus calculation
    //
    // Output:
    //   s   surplus for each component (zero for those excluded)
    //
    // This function computes the surpluses for the components
    // indicated by 'm'.  If 'n' is not specified, all components
    // are returned.
	double surplus(size_t n) const;

	// Print the curve
	void print(string &header);
	void print(ostream &cout, string &header);
	void print_matlab(ostream &cout, string &variable);
};
#endif // _MARKET_H
