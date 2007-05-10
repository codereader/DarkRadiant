#include "DeferredAdjustment.h"

#include <glib/gmain.h>
#include <gtk/gtkadjustment.h>

namespace gtkutil {

DeferredAdjustment::DeferredAdjustment(ValueChangedFunction function, void* data) : 
	m_value(0), 
	m_handler(0), 
	m_function(function), 
	m_data(data)
{}

void DeferredAdjustment::flush() {
	if (m_handler != 0) {
		g_source_remove(m_handler);
		deferred_value_changed(this);
	}
}

void DeferredAdjustment::value_changed(gdouble value) {
	m_value = value;
	if (m_handler == 0) {
		m_handler = g_idle_add(deferred_value_changed, this);
	}
}

void DeferredAdjustment::adjustment_value_changed(GtkAdjustment *adjustment, DeferredAdjustment* self) {
	self->value_changed(adjustment->value);
}

gboolean DeferredAdjustment::deferred_value_changed(gpointer data) {
	DeferredAdjustment* self = reinterpret_cast<DeferredAdjustment*>(data);
	
	self->m_function(self->m_data, self->m_value);
	self->m_handler = 0;
	self->m_value = 0;

	return FALSE;
}

} // namespace gtkutil
