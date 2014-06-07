#pragma once

#include "SpecifierPanel.h"
#include "SpecifierPanelFactory.h"

class wxTextCtrl;

namespace objectives
{

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
	public SpecifierPanel
{
protected:
	wxTextCtrl* _entry;

	TextSpecifierPanel();

public:
	TextSpecifierPanel(wxWindow* parent);

	/* SpecifierPanel implementation */
	virtual SpecifierPanelPtr create(wxWindow* parent) const
	{
		return SpecifierPanelPtr(new TextSpecifierPanel(parent));
	}

	virtual wxWindow* getWidget();
    void setValue(const std::string& value);
    std::string getValue() const;
};

}

}
