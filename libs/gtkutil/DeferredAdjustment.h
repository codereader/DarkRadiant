#pragma once

#include <gtkmm/adjustment.h>
#include <boost/function.hpp>

namespace gtkutil
{

/// Adjustment that calls a callback function when Gtk is idle
class DeferredAdjustment : public Gtk::Adjustment
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
		_function(_value);
	}

protected:

	// gtkmm signal handler
	void on_value_changed()
	{
		_value = get_value();

        Glib::signal_idle().connect_once(
            sigc::mem_fun(this, &DeferredAdjustment::flush)
        );
	}
};

} // namespace gtkutil
