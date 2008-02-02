#ifndef SPECIFIERPANEL_H_
#define SPECIFIERPANEL_H_

#include <boost/shared_ptr.hpp>
#include <gtk/gtkwidget.h>

namespace objectives
{

namespace ce
{

class SpecifierPanel;

/**
 * Shared pointer typedef for SpecifierPanel.
 */
typedef boost::shared_ptr<SpecifierPanel> SpecifierPanelPtr;

/**
 * Compound widget for editing a single Specifier type.
 * 
 * A SpecifierPanel is a horizontal box containing widget(s) that are intended
 * to edit a single type of specifier. For example, a SpecifierPanel for
 * SPEC_NONE would contain no editable widgets, whereas the SpecifierPanel for
 * SPEC_CLASSNAME might provide a text entry box and a browse button to select
 * an entity class from the entity class tree.
 * 
 * SpecifierPanel subclasses are created through the virtual constructor idiom,
 * based on the Specifier type that needs to be edited. They accept and return
 * a std::string containing the specifier value to be used.
 * 
 * All SpecifierPanel subclasses are responsible for destroying their GTK
 * widgets in their own destructors.
 */
class SpecifierPanel
{
public:
	
	/**
	 * Create a SpecifierPanel subclass of the same type as this one.
	 * 
	 * This method is required for the virtual constructor idiom, and is not
	 * used by other code.
	 * 
	 * @return
	 * A SpecifierPanel subclass of the same type as this one.
	 */
	virtual SpecifierPanelPtr clone() const = 0;
	
	/**
	 * Obtain the master GtkWidget for packing into the parent window.
	 * 
	 * @return
	 * A GtkWidget containing all SpecifierPanel widgets. This widget will have
	 * been made visible with <b>gtk_widget_show_all()</b>.
	 */
	virtual GtkWidget* getWidget() const = 0;
};

}

}

#endif /*SPECIFIERPANEL_H_*/
