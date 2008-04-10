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
				Specifier::SPEC_NONE().getName(),
				SpecifierPanelPtr(new NoneSpecifierPanel())
			);
		}
	} _regHelper;
	
	// Main widget
	GtkWidget* _widget;
	
protected:

    /* gtkutil::EditorWidget implementation */

    virtual GtkWidget* _getWidget() const;

public:
	
	/**
	 * Construct a NoneSpecifierPanel.
	 */
	NoneSpecifierPanel();
	
	/**
	 * Destroy this NoneSpecifierPanel including all widgets.
	 */
	~NoneSpecifierPanel();
	
	/* SpecifierPanel implementation */
	
	SpecifierPanelPtr clone() const {
		return SpecifierPanelPtr(new NoneSpecifierPanel());
	}

    /* gtkutil::EditorWidget implementation */

    void setValue(const std::string&) { }
    std::string getValue() const { return ""; }
};

}

}

#endif /*NONESPECIFIERPANEL_H_*/
