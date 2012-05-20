#pragma once

#include <gtkmm/adjustment.h>
#include <boost/function.hpp>

#include "event/SingleIdleCallback.h"

namespace gtkutil
{

class DeferredAdjustment :
	public Gtk::Adjustment,
	protected SingleIdleCallback
{
public:
	typedef boost::function<void(double)> ValueChangedFunction;

private:
	double _value;
	ValueChangedFunction _function;

public:
	DeferredAdjustment(const ValueChangedFunction& function, 
					   double value, double lower, double upper, double step_increment = 1.0,
					   double page_increment = 10.0, double page_size = 0.0) :
		Gtk::Adjustment(value, lower, upper, step_increment, page_increment, page_size),
		_function(function)
	{}

	void flush()
	{
		flushIdleCallback();
	}

protected:
	void onGtkIdle()
	{
		_function(_value);
	}

	// gtkmm signal handler
	void on_value_changed()
	{
		_value = get_value();
		requestIdleCallback();
	}
};

} // namespace gtkutil
