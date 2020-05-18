#include "EntityModule.h"

#include "itextstream.h"
#include "imap.h"
#include "i18n.h"

#include "entitylib.h"
#include "gamelib.h"

#include "string/replace.h"

#include "Doom3Entity.h"

#include "light/LightNode.h"
#include "doom3group/Doom3GroupNode.h"
#include "speaker/SpeakerNode.h"
#include "generic/GenericEntityNode.h"
#include "eclassmodel/EclassModelNode.h"
#include "target/TargetManager.h"
#include "module/StaticModule.h"
#include "EntitySettings.h"

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

IEntityNodePtr Doom3EntityModule::createEntity(const IEntityClassPtr& eclass)
{
	IEntityNodePtr node = createNodeForEntity(eclass);

	if (GlobalMapModule().getRoot())
	{
		// All entities are created in the active layer by default
		node->moveToLayer(GlobalMapModule().getRoot()->getLayerManager().getActiveLayer());
	}

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
		std::string entityName = string::replace_all_copy(eclassName, ":", "_") + "_1";

		node->getEntity().setKeyValue("name", entityName);
	}

	return node;
}

ITargetManagerPtr Doom3EntityModule::createTargetManager()
{
    return std::make_shared<TargetManager>();
}

IEntitySettings& Doom3EntityModule::getSettings()
{
	return *EntitySettings::InstancePtr();
}

// RegisterableModule implementation
const std::string& Doom3EntityModule::getName() const
{
	static std::string _name(MODULE_ENTITY);
	return _name;
}

const StringSet& Doom3EntityModule::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_XMLREGISTRY);
		_dependencies.insert(MODULE_MAP);
		_dependencies.insert(MODULE_GAMEMANAGER);
	}

	return _dependencies;
}

void Doom3EntityModule::initialiseModule(const ApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called." << std::endl;

	LightShader::m_defaultShader = game::current::getValue<std::string>("/defaults/lightShader");
}

void Doom3EntityModule::shutdownModule()
{
	rMessage() << getName() << "::shutdownModule called." << std::endl;

	// Destroy the settings instance
	EntitySettings::destroy();
}

// Static module instance
module::StaticModule<Doom3EntityModule> entityModule;

} // namespace entity
