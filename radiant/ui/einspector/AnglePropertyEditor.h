#pragma once

#include "PropertyEditor.h"

namespace Gtk { class Button; }

namespace ui
{

/**
 * \brief
 * Property editor for editing "angle" keys, for setting the direction an entity
 * faces.
 *
 * This property editor provides 8 direction arrow buttons to set the common
 * directions.
 */
class AnglePropertyEditor
: public PropertyEditor
{
private:
    // Eight directional buttons
	Gtk::Button* _nButton;
    Gtk::Button* _neButton;
    Gtk::Button* _eButton;
    Gtk::Button* _seButton;
    Gtk::Button* _sButton;
    Gtk::Button* _swButton;
    Gtk::Button* _wButton;
    Gtk::Button* _nwButton;

    // Key to edit
    std::string _key;

public:

    /**
     * \brief
     * Default constructor for the factory map.
     */
    AnglePropertyEditor()
    { }

    /**
     * \brief
     * Construct a AnglePropertyEditor to edit the given entity and key.
     */
    AnglePropertyEditor(Entity* entity, const std::string& key);

    /* PropertyEditor implementation */
    IPropertyEditorPtr createNew(Entity* entity,
                                const std::string& key,
                                const std::string& options)
    {
        return PropertyEditorPtr(new AnglePropertyEditor(entity, key));
    }

private:
	// Construct the buttons
    void constructButtons();

    // callback
    void _onButtonClick(int angleValue);

	// Helper method to construct an angle button
	Gtk::Button* constructAngleButton(const std::string& icon, int angleValue);
};

}
