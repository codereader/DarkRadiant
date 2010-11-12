#ifndef SPECIFIERPANEL_H_
#define SPECIFIERPANEL_H_

#include <boost/shared_ptr.hpp>

namespace Gtk { class Widget; }

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
    virtual ~SpecifierPanel() {}

	/**
	 * Retrieve the widget which can be packed into a parent container.
	 */
	virtual Gtk::Widget* getWidget() = 0;

	/**
     * Set the value of the string which should be edited by this widget. The
     * child editing widgets will be immediately updated to reflect the new
     * value.
     */
	virtual void setValue(const std::string& val) = 0;

    /**
     * Get the current value of the string which is being edited by this widget.
     */
    virtual std::string getValue() const = 0;

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
};

}

}

#endif /*SPECIFIERPANEL_H_*/
