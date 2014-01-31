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

namespace wxutil
{

class DeferredScrollbar :
	private SingleIdleCallback
{
public:
	typedef boost::function<void(int)> ValueChangedFunction;

	enum Orientation
	{
		Horizontal,
		Vertical,
	};

private:
	wxScrollBar* _scrollbar;

	int _cachedValue;
	ValueChangedFunction _function;

public:
	DeferredScrollbar(wxWindow* parent, Orientation dir, const ValueChangedFunction& function, 
					  int value, int upper, int pageSize = 1) :
		_scrollbar(new wxScrollBar(parent, wxID_ANY, wxDefaultPosition, 
				   wxDefaultSize, dir == Horizontal ? wxSB_HORIZONTAL : wxSB_VERTICAL)),
		_function(function)
	{
		_scrollbar->SetRange(upper);
		_scrollbar->SetScrollbar(value, 1, upper, pageSize);

		_scrollbar->Connect(wxEVT_SCROLL_CHANGED, wxScrollEventHandler(DeferredScrollbar::onScrollPositionChanged), NULL, this);
	}

	wxWindow* getWidget()
	{
		return _scrollbar;
	}

	void flush()
	{
		flushIdleCallback();
	}

	void Show()
	{
		_scrollbar->Show();
	}

	void Hide()
	{
		_scrollbar->Hide();
	}

protected:
	void onIdle()
	{
		_function(_cachedValue);
	}

	// wx signal handler
	void onScrollPositionChanged(wxScrollEvent& ev)
	{
		_cachedValue = ev.GetPosition();
		requestIdleCallback();
	}
};

} // namespace wxutil
