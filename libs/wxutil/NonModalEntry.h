#pragma once

#include <functional>
#include <wx/textctrl.h>

namespace wxutil
{

/**
 * A special entry field keeping track of focus events and editing status.
 * The attached callbacks ("apply" and "cancel") are automatically invoked 
 * where appropriate (losing focus, pressing enter or escape, etc.)
 */
class NonModalEntry :
	public wxTextCtrl
{
public:
	typedef std::function<void()> ApplyCallback;
	typedef std::function<void()> CancelCallback;
	typedef std::function<void()> ChangedCallback;

private:
	bool _editing;

	ApplyCallback _apply;
	CancelCallback _cancel;
	ChangedCallback _changed;
	bool _giveUpFocusOnApplyOrCancel;

public:
	NonModalEntry(wxWindow* parent, const ApplyCallback& apply, const CancelCallback& cancel, 
				  const ChangedCallback& changed = 0, 
				  bool giveUpFocusOnApplyOrCancel = true) : 
		wxTextCtrl(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER),
		_editing(false), 
		_apply(apply), 
		_cancel(cancel),
		_changed(changed),
		_giveUpFocusOnApplyOrCancel(giveUpFocusOnApplyOrCancel)
	{
		Connect(wxEVT_SET_FOCUS, wxFocusEventHandler(NonModalEntry::onGetFocus), NULL, this);
		Connect(wxEVT_KILL_FOCUS, wxFocusEventHandler(NonModalEntry::onLoseFocus), NULL, this);
		Connect(wxEVT_TEXT, wxCommandEventHandler(NonModalEntry::onTextChanged), NULL, this);
		Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(NonModalEntry::onTextEntryKeyPressed), NULL, this);
	}

protected:
	void onGetFocus(wxFocusEvent& ev)
	{
		_editing = false;
		ev.Skip();
	}

	void onLoseFocus(wxFocusEvent& ev)
	{
		if (_editing && IsShown())
		{
			if (_apply)
			{
				_apply();
			}
		}

		_editing = false;

		ev.Skip();
	}
	
	void onTextChanged(wxCommandEvent& ev)
	{
		_editing = true;

		if (IsShown())
		{
			if (_changed)
			{
				_changed();
			}
		}

		ev.Skip();
	}

	void onTextEntryKeyPressed(wxKeyEvent& ev)
	{
		if (ev.GetKeyCode() == WXK_RETURN || ev.GetKeyCode() == WXK_ESCAPE)
		{
			if (ev.GetKeyCode() == WXK_RETURN)
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
				wxWindow* parent = GetParent();

				while (parent != NULL && parent->GetParent() != NULL)
				{
					parent = parent->GetParent();
				}

				if (parent != NULL)
				{
					parent->SetFocus();
				}
			}

			return;
		}

		ev.Skip();
    }
};

} // namespace
