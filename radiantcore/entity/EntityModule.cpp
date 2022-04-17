#include "EntityModule.h"

#include "itextstream.h"
#include "icommandsystem.h"
#include "imap.h"
#include "iselection.h"
#include "i18n.h"

#include "entitylib.h"
#include "gamelib.h"
#include "selectionlib.h"

#include "string/replace.h"

#include "SpawnArgs.h"

#include "light/LightNode.h"
#include "doom3group/StaticGeometryNode.h"
#include "speaker/SpeakerNode.h"
#include "generic/GenericEntityNode.h"
#include "eclassmodel/EclassModelNode.h"
#include "target/TargetManager.h"
#include "module/StaticModule.h"
#include "EntitySettings.h"
#include "selection/algorithm/General.h"
#include "selection/algorithm/Group.h"
#include "selection/algorithm/Entity.h"
#include "selection/algorithm/Shader.h"
#include "command/ExecutionFailure.h"
#include "eclass.h"
#include "algorithm/Speaker.h"

namespace entity
{

// Default radius of lights is 320 (Q4) rather than 300 (D3)
// since the grid is usually a multiple of 8.
const float DEFAULT_LIGHT_RADIUS = 320;

// Function to return an AABB based on the current workzone AABB (retrieved
// from the currently selected brushes), or to use the default light radius
// if the workzone AABB is not valid or none is available.
AABB Doom3Light_getBounds(AABB aabb)
{
    // If the extents are 0 or invalid (-1), replace with the default radius
    for (int i = 0; i < 3; i++)
    {
        if (aabb.extents[i] <= 0)
        {
            aabb.extents[i] = DEFAULT_LIGHT_RADIUS;
        }
    }

    return aabb;
}

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
		node = StaticGeometryNode::Create(eclass);
	}
	else if (!eclass->getAttributeValue("model").empty())
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

IEntityNodePtr Doom3EntityModule::createEntityFromSelection(const std::string& name, const Vector3& origin)
{
    // Obtain the structure containing the selection counts
    const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();

    IEntityClassPtr entityClass = GlobalEntityClassManager().findOrInsert(name, true);

    // TODO: to be replaced by inheritance-based class detection
    bool isModel = (info.totalCount == 0 && name == "func_static");

    // Some entities are based on the size of the currently-selected primitive(s)
    bool primitivesSelected = info.brushCount > 0 || info.patchCount > 0;

    if (!(entityClass->isFixedSize() || isModel) && !primitivesSelected) {
        throw cmd::ExecutionFailure(
            fmt::format(_("Unable to create entity {0}, no brushes selected."), name)
        );
    }

    // Get the selection workzone bounds
    AABB workzone = GlobalSelectionSystem().getWorkZone().bounds;

    // Create the new node for the entity
    IEntityNodePtr node(GlobalEntityModule().createEntity(entityClass));

    GlobalSceneGraph().root()->addChildNode(node);

    // The layer list we're moving the newly created node/subgraph into
    scene::LayerList targetLayers;

    if (entityClass->isFixedSize() || (isModel && !primitivesSelected))
    {
        selection::algorithm::deleteSelection();

        ITransformablePtr transform = scene::node_cast<ITransformable>(node);

        if (transform != 0) {
            transform->setType(TRANSFORM_PRIMITIVE);
            transform->setTranslation(origin);
            transform->freezeTransform();
        }

        GlobalSelectionSystem().setSelectedAll(false);

        // Move the item to the active layer
        if (GlobalMapModule().getRoot())
        {
            targetLayers.insert(GlobalMapModule().getRoot()->getLayerManager().getActiveLayer());
        }

        Node_setSelected(node, true);
    }
    else // brush-based entity
    {
        // Add selected brushes as children of non-fixed entity
        node->getEntity().setKeyValue("model", node->getEntity().getKeyValue("name"));

        // Take the selection center as new origin
        Vector3 newOrigin = selection::algorithm::getCurrentSelectionCenter();
        node->getEntity().setKeyValue("origin", string::to_string(newOrigin));

        // If there is an "editor_material" class attribute, apply this shader
        // to all of the selected primitives before parenting them
        std::string material = node->getEntity().getEntityClass()->getAttributeValue("editor_material");

        if (!material.empty())
        {
            selection::applyShaderToSelection(material);
        }

        // If we had primitives to reparent, the new entity should inherit the layer info from them
        if (primitivesSelected)
        {
            scene::INodePtr primitive = GlobalSelectionSystem().ultimateSelected();
            targetLayers = primitive->getLayers();
        }
        // Otherwise move the item to the active layer
        else if (GlobalMapModule().getRoot())
        {
            targetLayers.insert(GlobalMapModule().getRoot()->getLayerManager().getActiveLayer());
        }

        // Parent the selected primitives to the new node
        selection::algorithm::ParentPrimitivesToEntityWalker walker(node);
        GlobalSelectionSystem().foreachSelected(walker);
        walker.reparent();

        // De-select the children and select the newly created parent entity
        GlobalSelectionSystem().setSelectedAll(false);
        Node_setSelected(node, true);
    }

    // Assign the layers - including all child nodes (#2864)
    scene::AssignNodeToLayersWalker layerWalker(targetLayers);
    node->traverse(layerWalker);

    // Set the light radius and origin

    if (entityClass->isLight() && primitivesSelected)
    {
        AABB bounds(Doom3Light_getBounds(workzone));
        node->getEntity().setKeyValue("origin",
            string::to_string(bounds.getOrigin()));
        node->getEntity().setKeyValue("light_radius",
            string::to_string(bounds.getExtents()));
    }

    // Check for auto-setting key values. TODO: use forEachAttribute
    // directly here.
    eclass::AttributeList list = eclass::getSpawnargsWithPrefix(
        *entityClass, "editor_setKeyValue"
    );

    if (!list.empty())
    {
        for (const auto& attr : list)
        {
            // Cut off the "editor_setKeyValueN " string from the key to get the spawnarg name
            std::string key = attr.getName().substr(attr.getName().find_first_of(' ') + 1);
            node->getEntity().setKeyValue(key, attr.getValue());
        }
    }

    // Return the new node
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
		_dependencies.insert(MODULE_COMMANDSYSTEM);
	}

	return _dependencies;
}

void Doom3EntityModule::initialiseModule(const IApplicationContext& ctx)
{
    rMessage() << getName() << "::initialiseModule called." << std::endl;

    LightShader::m_defaultShader = game::current::getValue<std::string>("/defaults/lightShader");

    GlobalCommandSystem().addCommand("CreateSpeaker", std::bind(&algorithm::CreateSpeaker, std::placeholders::_1),
        { cmd::ARGTYPE_STRING, cmd::ARGTYPE_VECTOR3 });

    _settingsListener = EntitySettings::InstancePtr()->signal_settingsChanged().connect(
        sigc::mem_fun(this, &Doom3EntityModule::onEntitySettingsChanged));
}

void Doom3EntityModule::shutdownModule()
{
	rMessage() << getName() << "::shutdownModule called." << std::endl;

    _settingsListener.disconnect();

	// Destroy the settings instance
	EntitySettings::destroy();
}

void Doom3EntityModule::onEntitySettingsChanged()
{
    if (!GlobalMapModule().getRoot()) return;

    // Actively notify all EntityNodes about the settings change
    GlobalMapModule().getRoot()->foreachNode([](const scene::INodePtr& node)
    {
        auto entity = std::dynamic_pointer_cast<EntityNode>(node);

        if (entity)
        {
            entity->onEntitySettingsChanged();
        }

        return true;
    });
}

// Static module instance
module::StaticModuleRegistration<Doom3EntityModule> entityModule;

} // namespace entity
