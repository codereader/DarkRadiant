#ifndef DEFERREDADJUSTMENT_H_
#define DEFERREDADJUSTMENT_H_

#include <glib/gtypes.h>
typedef struct _GtkAdjustment GtkAdjustment;

namespace gtkutil {

class DeferredAdjustment
{
	gdouble m_value;
	guint m_handler;
	typedef void (*ValueChangedFunction)(void* data, gdouble value);
	ValueChangedFunction m_function;
	void* m_data;

public:
	DeferredAdjustment(ValueChangedFunction function, void* data);
	
	void flush();
	void value_changed(gdouble value);

	static void adjustment_value_changed(GtkAdjustment *adjustment, DeferredAdjustment* self);

private:
	static gboolean deferred_value_changed(gpointer data);
};

} // namespace gtkutil

#endif /*DEFERREDADJUSTMENT_H_*/
