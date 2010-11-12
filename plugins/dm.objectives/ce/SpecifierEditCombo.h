#ifndef SPECIFIEREDITCOMBO_H_
#define SPECIFIEREDITCOMBO_H_

#include <gtkmm/box.h>
#include "../Specifier.h"
#include "specpanel/SpecifierPanel.h"

namespace Gtk { class ComboBox; }

namespace objectives
{

namespace ce
{

/**
 * Compound widget for changing specifier types and editing their values.
 *
 * A SpecifierEditCombo is a horizontal box containing two main elements. On the
 * left is a GtkComboBox which contains the names of a number of different
 * SpecifierType types, and on the right is a SpecifierPanel which is switched
 * depending on the selected dropdown value. The SpecifierEditCombo therefore
 * provides a means for the user to choose both a SpecifierType and its
 * associated value.
 *
 * Since some Component types accept a different subset of the SpecifierType types,
 * the SpecifierEditCombo can accept a std::set of SpecifierType types to display
 * in its dropdown list. Alternatively the entire set of Specifiers can be
 * made available.
 */
class SpecifierEditCombo :
	public Gtk::HBox
{
private:
	// Current SpecifierPanel
	SpecifierPanelPtr _specPanel;

    // Combo box containing Specifiers
	Gtk::ComboBox* _specifierCombo;

private:

    // Get the selected SpecifierType string
    std::string getSpecName() const;

    // Create the SpecifierPanel for the given specifier type
    void createSpecifierPanel(const std::string& type);

	// gtkmm callback
	void _onChange();

public:

	/**
	 * Construct a SpecifierEditCombo with a subset of SpecifierType types
	 * available.
	 *
	 * @param set
	 * A SpecifierSet containing the subset of Specifiers which should be
	 * displayed in this edit combo. The default is the complete set of
	 * specifiers.
	 */
	SpecifierEditCombo(const SpecifierTypeSet& set = SpecifierType::SET_ALL());

    /**
     * Return the current value of the Specifier (type and value).
     *
     * @return
     * A Specifier containing the type and value of the Specifier as currently
     * shown by the edit widgets.
     */
    SpecifierPtr getSpecifier() const;

    /**
     * Set the Specifier to display in the ComboBox.
     *
     * @param
     * A Specifier containing the type and value to be displayed in the edit
     * widgets.
     */
    void setSpecifier(SpecifierPtr spec);
};

}

}

#endif /*SPECIFIEREDITCOMBO_H_*/
