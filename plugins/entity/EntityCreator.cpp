#include "EntityCreator.h"

#include "itextstream.h"
#include "iundo.h"
#include "iscenegraph.h"
#include "namespace.h"
#include "string/pooledstring.h"

#include "entitylib.h"

#include <boost/algorithm/string/replace.hpp>
#include <iostream>

#include "light/LightNode.h"
#include "doom3group/Doom3GroupNode.h"
#include "generic/GenericEntityNode.h"
#include "eclassmodel/EclassModelNode.h"
#include "Doom3Entity.h"

namespace entity {
	
	namespace {
		inline Namespaced* Node_getNamespaced(scene::Node& node) {
			return dynamic_cast<Namespaced*>(&node);
		}
		
		void Entity_setName(Entity& entity, const std::string& name) {
			entity.setKeyValue("name", name);
		}
		typedef ReferenceCaller1<Entity, const std::string&, Entity_setName> EntitySetNameCaller;
	}

scene::Node& Doom3EntityCreator::getEntityForEClass(IEntityClassPtr eclass) {
	if (eclass->isLight()) {
		return (new LightNode(eclass))->node();
	}
	else if (!eclass->isFixedSize()) {
		// Variable size entity
		return (new entity::Doom3GroupNode(eclass))->node();
	}
	else if (eclass->getModelPath().size() > 0) {
		// Fixed size, has model path
		return (new EclassModelNode(eclass))->node();
	}
	else {
		// Fixed size, no model path
		return (new GenericEntityNode(eclass))->node();
	}
}

scene::Node& Doom3EntityCreator::createEntity(IEntityClassPtr eclass) {
	scene::Node& node = getEntityForEClass(eclass);
	Node_getEntity(node)->setKeyValue("classname", eclass->getName());
	
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

		GlobalNamespace().makeUnique(
			entityName.c_str(), 
			EntitySetNameCaller(*Node_getEntity(node))
		);
	}

	// Move the new entity into the global namespace
	Namespaced* namespaced = Node_getNamespaced(node);
	if (namespaced != NULL) {
		namespaced->setNamespace(GlobalNamespace());
	}

	return node;

}

void Doom3EntityCreator::setKeyValueChangedFunc(KeyValueChangedFunc func) {
	Doom3Entity::setKeyValueChangedFunc(func);
}

void Doom3EntityCreator::setCounter(Counter* counter) {
	Doom3Entity::setCounter(counter);
}

/* Connect two entities using a "target" key.
 */
void Doom3EntityCreator::connectEntities(const scene::Path& path,
                     const scene::Path& targetPath) {
	// Obtain both entities
	Entity* e1 = Node_getEntity(path.top());
	Entity* e2 = Node_getEntity(targetPath.top());

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

void Doom3EntityCreator::printStatistics() const {
	StringPool_analyse(Doom3Entity::getPool());
}

} // namespace entity
