#include "DeferredAdjustment.h"

#include <glib/gmain.h>
#include <gtk/gtkadjustment.h>

namespace gtkutil {

DeferredAdjustment::DeferredAdjustment(const ValueChangedFunction& function, 
					   double value, double lower, double upper, double step_increment,
					   double page_increment, double page_size) :
	Gtk::Adjustment(value, lower, upper, step_increment, page_increment, page_size),
	_function(function)
{}

void DeferredAdjustment::flush()
{
	flushIdleCallback();
}

void DeferredAdjustment::onGtkIdle()
{
	_function(_value);
}

void DeferredAdjustment::on_value_changed()
{
	_value = get_value();
	requestIdleCallback();
}

} // namespace gtkutil
