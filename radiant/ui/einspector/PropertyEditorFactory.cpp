#include "PropertyEditorFactory.h"

#include "iradiant.h"
#include "Vector3PropertyEditor.h"
#include "BooleanPropertyEditor.h"
#include "EntityPropertyEditor.h"
#include "ColourPropertyEditor.h"
#include "TexturePropertyEditor.h"
#include "SkinPropertyEditor.h"
#include "SoundPropertyEditor.h"
#include "FloatPropertyEditor.h"
#include "ModelPropertyEditor.h"
#include "ClassnamePropertyEditor.h"

namespace ui
{

// Initialisation
PropertyEditorFactory::PropertyEditorMap PropertyEditorFactory::_peMap;

// Register the classes
void PropertyEditorFactory::registerClasses() {
    _peMap["vector3"] = PropertyEditorPtr(new Vector3PropertyEditor());
    _peMap["bool"] = PropertyEditorPtr(new BooleanPropertyEditor());
    _peMap["entity"] = PropertyEditorPtr(new EntityPropertyEditor());
	_peMap["colour"] = PropertyEditorPtr(new ColourPropertyEditor());
	_peMap["texture"] = PropertyEditorPtr(new TexturePropertyEditor());
	_peMap["mat"] = PropertyEditorPtr(new TexturePropertyEditor());
	_peMap["skin"] = PropertyEditorPtr(new SkinPropertyEditor());
	_peMap["sound"] = PropertyEditorPtr(new SoundPropertyEditor());
	_peMap["float"] = PropertyEditorPtr(new FloatPropertyEditor());
	_peMap["model"] = PropertyEditorPtr(new ModelPropertyEditor());
	_peMap["classname"] = PropertyEditorPtr(new ClassnamePropertyEditor());
}

// Create a PropertyEditor from the given name.
PropertyEditorPtr PropertyEditorFactory::create(const std::string& className,
											  	Entity* entity,
											  	const std::string& key,
											  	const std::string& options) 
{
    // Register the PropertyEditors if the map is empty
    if (_peMap.empty()) {
        registerClasses();
    }

	// Search for the named property editor type
	PropertyEditorMap::iterator iter(_peMap.find(className));

	// If the type is not found, return NULL otherwise create a new instance of
	// the associated derived type.
	if (iter == _peMap.end()) {
		return PropertyEditorPtr();
	} else {
		return iter->second->createNew(entity, key, options);
	}
}

// Return a GdkPixbuf containing the icon for the given property type

GdkPixbuf* PropertyEditorFactory::getPixbufFor(const std::string& type) {
	// Sanity check
	if (type.empty()) return NULL;

	std::string iconName = "icon_" + type + ".png";
	return GlobalRadiant().getLocalPixbuf(iconName);	
}

}
