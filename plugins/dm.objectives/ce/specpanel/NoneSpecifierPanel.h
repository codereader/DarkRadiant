#ifndef NONESPECIFIERPANEL_H_
#define NONESPECIFIERPANEL_H_

#include "SpecifierPanel.h"
#include "SpecifierPanelFactory.h"

namespace objectives
{

namespace ce
{

/**
 * SpecifierPanel subclass for SPEC_NONE.
 * 
 * This class does not provide any editable widgets, but displays a simple
 * text message indicating that the SPEC_NONE Specifier type does not take a
 * value.
 */
class NoneSpecifierPanel
: public SpecifierPanel
{
	// Map registration
	static struct RegHelper {
		RegHelper() { 
			SpecifierPanelFactory::registerType(
				Specifier::SPEC_NONE(),
				SpecifierPanelPtr(new NoneSpecifierPanel())
			);
		}
	} _regHelper;
	
	// Main widget
	GtkWidget* _widget;
	
public:
	
	/**
	 * Construct a NoneSpecifierPanel.
	 */
	NoneSpecifierPanel();
	
	/* SpecifierPanel implementation */
	
	GtkWidget* getWidget() const { return _widget; }
	
	SpecifierPanelPtr clone() const {
		return SpecifierPanelPtr(new NoneSpecifierPanel());
	}
};

}

}

#endif /*NONESPECIFIERPANEL_H_*/
