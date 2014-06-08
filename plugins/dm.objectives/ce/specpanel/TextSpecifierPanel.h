#pragma once

#include "SpecifierPanel.h"
#include "SpecifierPanelFactory.h"
#include <wx/event.h>

class wxTextCtrl;

namespace objectives
{

class Component;

namespace ce
{

/**
 * SpecifierPanel intermediate class for all SpecifierPanels which wish to
 * provide a simple text entry box for editing their value.
 *
 * This class does not register itself directly in the SpecifierPanelFactory,
 * but should be used as a parent class for individual type-based
 * SpecifierPanels which register themselves correctly for a given SPEC_* type.
 */
class TextSpecifierPanel :
	public SpecifierPanel,
	public wxEvtHandler
{
protected:
	wxTextCtrl* _entry;

	// The change callback, invoked when the text changes
	std::function<void()> _valueChanged;

	TextSpecifierPanel();

public:
	TextSpecifierPanel(wxWindow* parent);

	virtual ~TextSpecifierPanel();

	/* SpecifierPanel implementation */
	virtual SpecifierPanelPtr create(wxWindow* parent) const
	{
		return SpecifierPanelPtr(new TextSpecifierPanel(parent));
	}

	void setChangedCallback(const std::function<void()>& callback)
	{
		_valueChanged = callback;
	}

	virtual wxWindow* getWidget();
    void setValue(const std::string& value);
    std::string getValue() const;

private:
	void onEntryChanged(wxCommandEvent& ev);
};

}

}
