#include "PropertyEditorFactory.h"

#include "gtkutil/image.h"

#include "config.h"

namespace ui
{

// Initialisation

PropertyEditorFactory::PropertyEditorMap PropertyEditorFactory::_peMap;

// Create a PropertyEditor from the given name.

PropertyEditor* PropertyEditorFactory::create(const std::string& className, Entity* entity, const std::string& key) {

#ifdef ALL_PROPERTY_EDITORS_ARE_TEXT
	PropertyEditorMap::iterator iter(_peMap.find("text"));
#else
	PropertyEditorMap::iterator iter(_peMap.find(className));
#endif

	if (iter == _peMap.end()) {
		std::cout << "PropertyEditorFactory: unable to find PropertyEditor instance"
							<< " for type \"" << className << "\"." << std::endl;
		return NULL;
	} else {
		return iter->second->createNew(entity, key);
	}
}

// Return a GdkPixbuf containing the icon for the given property type

GdkPixbuf* PropertyEditorFactory::getPixbufFor(std::string type) {
	std::string iconName(std::string("icon_") + type + ".png");
	return gtkutil::getLocalPixbuf(iconName);	
}

}
