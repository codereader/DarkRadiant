#pragma once

#include "PropertyEditor.h"

#include <gtk/gtk.h>

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
    // Main widget
    GtkWidget* _widget;

    // Eight directional buttons
    GtkWidget* _nButton;
    GtkWidget* _neButton;
    GtkWidget* _eButton;
    GtkWidget* _seButton;
    GtkWidget* _sButton;
    GtkWidget* _swButton;
    GtkWidget* _wButton;
    GtkWidget* _nwButton;

    // Key to edit
    std::string _key;

private:

    // Construct the buttons
    void constructButtons();

    /* GTK Callbacks */
    static void _onButtonClick(GtkButton* button, AnglePropertyEditor* self);

protected:

    /* gtkutil::Widget implementation */
    GtkWidget* _getWidget() const
    {
        return _widget;
    }

public:

    /**
     * \brief
     * Default constructor for the factory map.
     */
    AnglePropertyEditor()
    : _widget(NULL)
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
	// Helper method to construct an angle button
	GtkWidget* constructAngleButton(const std::string& icon, int angleValue);
};

}
