#include "EntityCreator.h"
#include "EntitySettings.h"

#include "itextstream.h"
#include "iundo.h"
#include "imap.h"
#include "iscenegraph.h"
#include "ieventmanager.h"
#include "inamespace.h"
#include "ifilter.h"
#include "ipreferencesystem.h"

#include "entitylib.h"
#include "gamelib.h"

#include <boost/algorithm/string/replace.hpp>
#include <iostream>

#include "i18n.h"
#include "target/RenderableTargetInstances.h"
#include "Doom3Entity.h"

#include "light/LightNode.h"
#include "doom3group/Doom3GroupNode.h"
#include "speaker/SpeakerNode.h"
#include "generic/GenericEntityNode.h"
#include "eclassmodel/EclassModelNode.h"

namespace entity
{

IEntityNodePtr createNodeForEntity(const IEntityClassPtr& eclass)
{
	// Null entityclass check
	if (!eclass)
	{
		throw std::runtime_error(
			_("createNodeForEntity(): "
			  "cannot create entity for NULL entityclass.")
		);
	}

	// Otherwise create the correct entity subclass based on the entity class
	// parameters.
	EntityNodePtr node;

	if (eclass->isLight())
	{
		node = LightNode::Create(eclass);
	}
	else if (!eclass->isFixedSize())
	{
		// Variable size entity
		node = Doom3GroupNode::Create(eclass);
	}
	else if (!eclass->getAttribute("model").getValue().empty())
	{
		// Fixed size, has model path
		node = EclassModelNode::Create(eclass);
	}
	else if (eclass->getName() == "speaker")
	{
		node = SpeakerNode::create(eclass);
	}
	else
	{
		// Fixed size, no model path
		node = GenericEntityNode::Create(eclass);
	}

	return node;
}

IEntityNodePtr Doom3EntityCreator::createEntity(const IEntityClassPtr& eclass)
{
	IEntityNodePtr node = createNodeForEntity(eclass);

	// All entities are created in the active layer by default
	node->moveToLayer(GlobalLayerSystem().getActiveLayer());

	node->getEntity().setKeyValue("classname", eclass->getName());

	// If this is not a worldspawn or unrecognised entity, generate a unique
	// name for it
	const std::string& eclassName = eclass->getName();

	if (!eclassName.empty() &&
		eclassName != "worldspawn" &&
		eclassName != "UNKNOWN_CLASS")
	{
		/* Clean up the name of the entity that is about the created
		 * so that nothing bad can happen (for example, the colon character
		 * seems to be causing problems in Doom 3 Scripting)
		 */
		std::string entityName =
			boost::algorithm::replace_all_copy(eclassName, ":", "_") + "_1";

		node->getEntity().setKeyValue("name", entityName);
	}

	return node;
}

/* Connect two entities using a "target" key.
 */
void Doom3EntityCreator::connectEntities(const scene::INodePtr& source,
                                         const scene::INodePtr& target)
{
	// Obtain both entities
	Entity* e1 = Node_getEntity(source);
	Entity* e2 = Node_getEntity(target);

	// Check entities are valid
	if (e1 == NULL || e2 == NULL) {
		rError() << "entityConnectSelected: both of the selected instances must be an entity\n";
		return;
	}

	// Check entities are distinct
	if (e1 == e2) {
		rError() << "entityConnectSelected: the selected instances must not both be from the same entity\n";
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
		_dependencies.insert(MODULE_RENDERSYSTEM);
		_dependencies.insert(MODULE_UNDOSYSTEM);
	}

	return _dependencies;
}

void Doom3EntityCreator::initialiseModule(const ApplicationContext& ctx)
{
	rMessage() << "Doom3EntityCreator::initialiseModule called." << std::endl;

	LightShader::m_defaultShader = game::current::getValue<std::string>("/defaults/lightShader");

	GlobalRenderSystem().attachRenderable(RenderableTargetInstances::Instance());

	GlobalEventManager().addRegistryToggle("ToggleShowAllLightRadii", RKEY_SHOW_ALL_LIGHT_RADII);
	GlobalEventManager().addRegistryToggle("ToggleShowAllSpeakerRadii", RKEY_SHOW_ALL_SPEAKER_RADII);
	GlobalEventManager().addRegistryToggle("ToggleDragResizeEntitiesSymmetrically", RKEY_DRAG_RESIZE_SYMMETRICALLY);
}

void Doom3EntityCreator::shutdownModule()
{
	rMessage() << "Doom3EntityCreator::shutdownModule called." << std::endl;

	GlobalRenderSystem().detachRenderable(RenderableTargetInstances::Instance());

	// Destroy the settings instance
	EntitySettings::destroy();
}

} // namespace entity
