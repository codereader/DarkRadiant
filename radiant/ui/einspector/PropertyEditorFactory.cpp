#include "PropertyEditorFactory.h"

#include "Vector3PropertyEditor.h"
#include "BooleanPropertyEditor.h"
#include "EntityPropertyEditor.h"
#include "ColourPropertyEditor.h"
#include "TexturePropertyEditor.h"
#include "SkinPropertyEditor.h"
#include "SoundPropertyEditor.h"

#include "gtkutil/image.h"

namespace ui
{

// Initialisation
PropertyEditorFactory::PropertyEditorMap PropertyEditorFactory::_peMap;

// Register the classes
void PropertyEditorFactory::registerClasses() {
    _peMap["vector3"] = new Vector3PropertyEditor();
    _peMap["boolean"] = new BooleanPropertyEditor();
    _peMap["entity"] = new EntityPropertyEditor();
	_peMap["colour"] = new ColourPropertyEditor();
	_peMap["texture"] = new TexturePropertyEditor();
	_peMap["skin"] = new SkinPropertyEditor();
	_peMap["sound"] = new SoundPropertyEditor();
}

// Create a PropertyEditor from the given name.

PropertyEditor* PropertyEditorFactory::create(const std::string& className,
											  Entity* entity,
											  const std::string& key,
											  const std::string& options) 
{
    // Register the PropertyEditors if the map is empty
    if (_peMap.empty()) {
        registerClasses();
    }

	PropertyEditorMap::iterator iter(_peMap.find(className));

	if (iter == _peMap.end()) {
		return NULL;
	} else {
        PropertyEditor* pe = iter->second->createNew(entity, key, options);
        pe->refresh();
		return pe;
	}
}

// Return a GdkPixbuf containing the icon for the given property type

GdkPixbuf* PropertyEditorFactory::getPixbufFor(std::string type) {
	std::string iconName(std::string("icon_") + type + ".png");
	return gtkutil::getLocalPixbuf(iconName);	
}

}
