#include "PropertyEditorFactory.h"

namespace ui
{

// Initialisation

PropertyEditorFactory::PropertyEditorMap PropertyEditorFactory::_peMap;

// Create a PropertyEditor from the given name.

PropertyEditor* PropertyEditorFactory::create(const std::string& className, Entity* entity, const std::string& key) {
	PropertyEditorMap::iterator iter(_peMap.find(className));
	if (iter == _peMap.end()) {
		std::cout << "PropertyEditorFactory: unable to find PropertyEditor instance"
							<< " for type \"" << className << "\"." << std::endl;
		return NULL;
	} else {
		return iter->second->createNew(entity, key);
	}
}


}
