#pragma once

#include <gtkmm/adjustment.h>
#include <boost/function.hpp>

#include <wx/scrolbar.h>
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

#if 0

namespace wxutil
{

class DeferredScrollbar :
	public wxScrollBar
{
public:
	typedef boost::function<void(int)> ValueChangedFunction;

	enum Orientation
	{
		Horizontal,
		Vertical,
	};

private:
	int _cachedValue;
	ValueChangedFunction _function;

	bool _idleRequestPending;

public:
	DeferredScrollbar(wxWindow* parent, Orientation dir, const ValueChangedFunction& function, 
					  int value, int upper, int pageSize = 1) :
		wxScrollBar(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, dir == Horizontal ? wxSB_HORIZONTAL : wxSB_VERTICAL),
		_function(function),
		_idleRequestPending(false)
	{
		SetRange(upper);
		SetScrollbar(value, 1, upper, pageSize);

		Connect(wxEVT_SCROLL_THUMBTRACK, wxScrollEventHandler(DeferredScrollbar::onScrollPositionChanged), NULL, this);
		Connect(wxEVT_IDLE, wxIdleEventHandler(DeferredScrollbar::onIdle), NULL, this);
	}

	void flush()
	{
		if (_idleRequestPending)
		{
			_idleRequestPending = false;
			_function(_cachedValue);
		}
	}

protected:
	void onIdle(wxIdleEvent& ev)
	{
		if (_idleRequestPending)
		{
			_idleRequestPending = false;
			_function(_cachedValue);
		}
	}

	// wx signal handler
	void onScrollPositionChanged(wxScrollEvent& ev)
	{
		_cachedValue = ev.GetPosition();
		_function(_cachedValue);
	}
};

} // namespace wxutil

#endif
