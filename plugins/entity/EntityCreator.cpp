#include "EntityCreator.h"

#include "itextstream.h"
#include "iundo.h"
#include "imap.h"
#include "iscenegraph.h"
#include "inamespace.h"
#include "ifilter.h"
#include "ipreferencesystem.h"

#include "entitylib.h"

#include <boost/algorithm/string/replace.hpp>
#include <iostream>

#include "light/LightNode.h"
#include "doom3group/Doom3GroupNode.h"
#include "speaker/SpeakerNode.h"
#include "generic/GenericEntityNode.h"
#include "eclassmodel/EclassModelNode.h"
#include "Doom3Entity.h"

namespace entity {
	
	namespace {
		inline NamespacedPtr Node_getNamespaced(scene::INodePtr node) {
			return boost::dynamic_pointer_cast<Namespaced>(node);
		}
	}

scene::INodePtr Doom3EntityCreator::getEntityForEClass(IEntityClassPtr eclass) {
	
	// Null entityclass check
	if (!eclass) {
		throw std::runtime_error(
			"Doom3EntityCreator::getEntityForEClass(): "
			"cannot create entity for NULL entityclass."
		); 
	}
	
	// Otherwise create the correct entity subclass based on the entity class
	// parameters.
	scene::INodePtr node;

	if (eclass->isLight()) {
		node = scene::INodePtr(new LightNode(eclass));
	}
	else if (!eclass->isFixedSize()) {
		// Variable size entity
		node = scene::INodePtr(new entity::Doom3GroupNode(eclass));
	}
	else if (!eclass->getAttribute("model").value.empty()) {
		// Fixed size, has model path
		node = scene::INodePtr(new EclassModelNode(eclass));
	}
	else if (eclass->getName() == "speaker") {
		node = scene::INodePtr(new SpeakerNode(eclass));
	}
	else {
		// Fixed size, no model path
		node = scene::INodePtr(new GenericEntityNode(eclass));
	}

	node->setSelf(node);
	return node;
}

scene::INodePtr Doom3EntityCreator::createEntity(IEntityClassPtr eclass) {
	scene::INodePtr node = getEntityForEClass(eclass);
	Entity* entity = Node_getEntity(node);
	assert(entity != NULL);

	entity->setKeyValue("classname", eclass->getName());
	
	// If this is not a worldspawn or unrecognised entity, generate a unique
	// name for it
	if (eclass->getName().size() > 0 && 
		eclass->getName() != "worldspawn" && 
		eclass->getName() != "UNKNOWN_CLASS")
	{
		/* Clean up the name of the entity that is about the created
		 * so that nothing bad can happen (for example, the colon character 
		 * seems to be causing problems in Doom 3 Scripting)
		 */
		std::string entityName = 
			boost::algorithm::replace_all_copy(eclass->getName(), ":", "_") + "_1";

		entity->setKeyValue("name", entityName);
	}

	// Check for auto-setting key values
	EntityClassAttributeList list = eclass->getAttributeList("editor_setKeyValue");

	if (!list.empty()) {
		for (EntityClassAttributeList::const_iterator i = list.begin(); i != list.end(); ++i) {
			// Cut off the "editor_setKeyValueN " string from the key to get the spawnarg name
			entity->setKeyValue(i->name.substr(i->name.find_first_of(' ') + 1, 18), i->value);
		}
	}

	return node;
}

void Doom3EntityCreator::setKeyValueChangedFunc(KeyValueChangedFunc func) {
	Doom3Entity::setKeyValueChangedFunc(func);
}

/* Connect two entities using a "target" key.
 */
void Doom3EntityCreator::connectEntities(const scene::INodePtr& source, const scene::INodePtr& target) {
	// Obtain both entities
	Entity* e1 = Node_getEntity(source);
	Entity* e2 = Node_getEntity(target);

	// Check entities are valid
	if (e1 == NULL || e2 == NULL) {
		globalErrorStream() << "entityConnectSelected: both of the selected instances must be an entity\n";
		return;
	}

	// Check entities are distinct
	if (e1 == e2) {
		globalErrorStream() << "entityConnectSelected: the selected instances must not both be from the same entity\n";
		return;
	}

	// Start the scoped undo session
	UndoableCommand undo("entityConnectSelected");

	// Find the first unused target key on the source entity
	for (int i = 0; i < 1024; ++i) {

		// Construct candidate key by appending number to "target"
		std::string targetKey = (boost::format("target%i") % i).str();

		// If the source entity does not have this key, add it and finish,
		// otherwise continue looping
		if (e1->getKeyValue(targetKey).empty()) {
			e1->setKeyValue(targetKey,
			                e2->getKeyValue("name"));
			break;
		}
	}

	// Redraw the scene
	SceneChangeNotify();
}

// RegisterableModule implementation
const std::string& Doom3EntityCreator::getName() const {
	static std::string _name(MODULE_ENTITYCREATOR);
	return _name;
}

const StringSet& Doom3EntityCreator::getDependencies() const {
	static StringSet _dependencies;

	if (_dependencies.empty()) {
		_dependencies.insert(MODULE_XMLREGISTRY);
		_dependencies.insert(MODULE_MAP);
		_dependencies.insert(MODULE_SCENEGRAPH);
		_dependencies.insert(MODULE_SHADERCACHE);
		_dependencies.insert(MODULE_UNDOSYSTEM);
	}

	return _dependencies;
}

void Doom3EntityCreator::initialiseModule(const ApplicationContext& ctx) {
	globalOutputStream() << "Doom3EntityCreator::initialiseModule called.\n";
	
	constructStatic();
}

void Doom3EntityCreator::shutdownModule() {
	globalOutputStream() << "Doom3EntityCreator::shutdownModule called.\n";
	
	destroyStatic();
}

} // namespace entity
