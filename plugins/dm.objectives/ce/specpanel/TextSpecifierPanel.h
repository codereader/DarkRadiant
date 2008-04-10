#ifndef TEXTSPECIFIERPANEL_H_
#define TEXTSPECIFIERPANEL_H_

#include "SpecifierPanel.h"
#include "SpecifierPanelFactory.h"

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
class TextSpecifierPanel
: public SpecifierPanel
{
	// Main widget
	GtkWidget* _widget;
	
public:
	
	/**
	 * Construct a TextSpecifierPanel.
	 */
	TextSpecifierPanel();
	
	/**
	 * Destroy this TextSpecifierPanel including all widgets.
	 */
	~TextSpecifierPanel();
	
	/* SpecifierPanel implementation */
	
	GtkWidget* getWidget() const;
	
	SpecifierPanelPtr clone() const {
		return SpecifierPanelPtr(new TextSpecifierPanel());
	}
};

}

}

#endif /*TEXTSPECIFIERPANEL_H_*/
