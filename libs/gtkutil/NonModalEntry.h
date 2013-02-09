#pragma once

#include <gtkmm/entry.h>
#include <gtkmm/window.h>
#include <boost/function.hpp>

namespace gtkutil
{

/**
 * A special entry field keeping track of focus events and editing status.
 * The attached callbacks ("apply" and "cancel") are automatically invoked 
 * where appropriate (losing focus, pressing enter or escape, etc.)
 */
class NonModalEntry :
	public Gtk::Entry
{
public:
	typedef boost::function<void()> ApplyCallback;
	typedef boost::function<void()> CancelCallback;
	typedef boost::function<void()> ChangedCallback;

private:
	bool _editing;

	ApplyCallback _apply;
	CancelCallback _cancel;
	ChangedCallback _changed;
	bool _giveUpFocusOnApplyOrCancel;

public:
	NonModalEntry(const ApplyCallback& apply, const CancelCallback& cancel, const ChangedCallback& changed = 0, bool giveUpFocusOnApplyOrCancel = true) : 
		_editing(false), 
		_apply(apply), 
		_cancel(cancel),
		_changed(changed),
		_giveUpFocusOnApplyOrCancel(giveUpFocusOnApplyOrCancel)
	{}

protected:

	// Override gtkmm event handlers
	bool on_focus_in_event(GdkEventFocus* ev)
	{
		_editing = false;

		return Gtk::Entry::on_focus_in_event(ev);
	}

	bool on_focus_out_event(GdkEventFocus* ev)
	{
		if (_editing && is_visible())
		{
			if (_apply)
			{
				_apply();
			}
		}

		_editing = false;

		return Gtk::Entry::on_focus_out_event(ev);
	}
	
	void on_changed()
	{
		_editing = true;

		if (is_visible())
		{
			if (_changed)
			{
				_changed();
			}
		}

		Gtk::Entry::on_changed();
	}

	bool on_key_press_event(GdkEventKey* ev)
	{
		if (ev->keyval == GDK_Return || ev->keyval == GDK_Escape)
		{
			if (ev->keyval == GDK_Return)
			{
				if (_apply)
				{
					_apply();
				}
			}
			else
			{
				if (_cancel)
				{
					_cancel();
				}
			}

			_editing = false;
			
			if (_giveUpFocusOnApplyOrCancel)
			{
				Gtk::Container* toplevel = get_toplevel();

				if (dynamic_cast<Gtk::Window*>(toplevel) != NULL)
				{
					static_cast<Gtk::Window*>(toplevel)->unset_focus();
				}
			}

			return true;
		}

		return Gtk::Entry::on_key_press_event(ev);
    }
};

} // namespace
