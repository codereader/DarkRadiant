#ifndef TEXTSPECIFIERPANEL_H_
#define TEXTSPECIFIERPANEL_H_

#include "SpecifierPanel.h"
#include "SpecifierPanelFactory.h"
#include <gtkmm/entry.h>

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
	public SpecifierPanel,
	protected Gtk::Entry
{
public:
	/* SpecifierPanel implementation */
	SpecifierPanelPtr clone() const
	{
		return SpecifierPanelPtr(new TextSpecifierPanel());
	}

	virtual Gtk::Widget* getWidget();
    void setValue(const std::string& value);
    std::string getValue() const;
};

}

}

#endif /*TEXTSPECIFIERPANEL_H_*/
