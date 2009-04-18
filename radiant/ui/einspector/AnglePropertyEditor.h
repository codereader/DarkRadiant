#pragma once

#include "PropertyEditor.h"

#include <gtk/gtkwidget.h>

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
    PropertyEditorPtr createNew(Entity* entity, 
                                const std::string& key,
                                const std::string& options)
    {
        return PropertyEditorPtr(new AnglePropertyEditor(entity, key));
    }


};

}
