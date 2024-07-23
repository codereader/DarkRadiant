#pragma once

#include "inode.h"
#include "imodule.h"
#include "irender.h"
#include "inameobserver.h"
#include "iscenegraph.h"
#include "itransformnode.h"
#include <functional>

#include "scene/scene_fwd.h"
#include "string/predicate.h"

class IEntityClass;
typedef std::shared_ptr<IEntityClass> IEntityClassPtr;
typedef std::shared_ptr<const IEntityClass> IEntityClassConstPtr;

// Observes a single entity key value and gets notified on change
class KeyObserver: public sigc::trackable
{
public:
    using Ptr = std::shared_ptr<KeyObserver>;

    virtual ~KeyObserver() {}

    /**
     * This event gets called when the observed keyvalue changes.
     * The new value is passed as argument, which can be an empty string.
     */
    virtual void onKeyValueChanged(const std::string& newValue) = 0;
};

/// Callback for an entity key value change
using KeyObserverFunc = sigc::slot<void(const std::string&)>;

inline bool Node_isEntity(const scene::INodePtr& node)
{
    //assert(!((std::dynamic_pointer_cast<EntityNode>(node) != nullptr) ^ (node->getNodeType() == scene::INode::Type::Entity)));
    return node->getNodeType() == scene::INode::Type::Entity;
}

// Represents a set of selected entities
class IEntitySelection
{
public:
    virtual ~IEntitySelection() {}

    // True if there's no selected entity available
    virtual bool empty() const = 0;

    // Returns the number of selected entities
    virtual std::size_t size() const = 0;

    // Iterates over each selected entity node, invoking the given functor
    virtual void foreachEntity(const std::function<void(const EntityNodePtr&)>& functor) = 0;

    // Returns the key value shared by all entities in this set, or an empty string
    // if there is no such value.
    virtual std::string getSharedKeyValue(const std::string& key, bool includeInherited) = 0;
};

/**
* greebo: This is an abstract representation of a target.
* In Doom3 maps, a Target can be any entity node, that's
* why this object encapsulates a reference to an actual
* scene::INode.
*
* Note: Such a Target object can be empty. That's the case for
* entities referring to non-existing entities in their
* "target" spawnarg.
*
* All ITargetableObjects are owned by the TargetManager class.
*/
class ITargetableObject
{
public:
    virtual ~ITargetableObject() {}

    // Returns the scene node behind this target. If the named target
    // cannot be resolved in the current scene, an empty pointer is returned.
    virtual const scene::INode* getNode() const = 0;

    // Use this method to check whether the node can be resolved
    virtual bool isEmpty() const = 0;
};
typedef std::shared_ptr<ITargetableObject> ITargetableObjectPtr;

/**
* greebo: The TargetManager keeps track of all ITargetableObjects
* in the current scene/map. A TargetManager instance is owned
* by the RootNode. TargetManager instances can be acquired through
* the EntityCreator interface.
*
* Clients acquire a named ITargetableObjectPtr by calling getTarget(). This
* always succeeds - if the named ITargetableObject is not found,
* a new, empty one is created.
*
*       ITargetableObject object (can be empty)
*                   ________
*                  /        \
* Entity           |        |
* TargetKey ----->>|    -------->> holds scene::INodePtr (==NULL, if empty)
*                  |        |
*                  \________/
*/
class ITargetManager
{
public:
    /**
     * Returns the Target with the given name.
     * This never returns NULL, an ITargetableObject is created if it doesn't exist yet.
     */
    virtual ITargetableObjectPtr getTarget(const std::string& name) = 0;

    /**
     * greebo: Associates the named Target with the given scene::INode.
     * The Target will be created if it doesn't exist yet.
     */
    virtual void associateTarget(const std::string& name, const scene::INode& node) = 0;

    // Will be called by a TargetableNode to notify about visibility changes
    virtual void onTargetVisibilityChanged(const std::string& name, const scene::INode& node) = 0;

    // Will be called by a TargetableNode to notify about a position change
    virtual void onTargetPositionChanged(const std::string& name, const scene::INode& node) = 0;

    /**
     * greebo: Disassociates the Target from the given name. The node
     * must also be passed to allow the manager to check the request.
     * Otherwise it would be possible for cloned nodes to dissociate
     * the target from their source node.
     */
    virtual void clearTarget(const std::string& name, const scene::INode& node) = 0;
};
typedef std::shared_ptr<ITargetManager> ITargetManagerPtr;

enum class LightEditVertexType : std::size_t
{
    StartEndDeselected,
    StartEndSelected,
    Inactive,
    Deselected,
    Selected,
    NumberOfVertexTypes,
};

constexpr const char* const RKEY_SHOW_ENTITY_NAMES("user/ui/xyview/showEntityNames");
constexpr const char* const RKEY_SHOW_ALL_SPEAKER_RADII = "user/ui/showAllSpeakerRadii";
constexpr const char* const RKEY_SHOW_ALL_LIGHT_RADII = "user/ui/showAllLightRadii";
constexpr const char* const RKEY_DRAG_RESIZE_SYMMETRICALLY = "user/ui/dragResizeEntitiesSymmetrically";
constexpr const char* const RKEY_ALWAYS_SHOW_LIGHT_VERTICES = "user/ui/alwaysShowLightVertices";
constexpr const char* const RKEY_FREE_OBJECT_ROTATION = "user/ui/rotateObjectsIndependently";
constexpr const char* const RKEY_SHOW_ENTITY_ANGLES = "user/ui/xyview/showEntityAngles";

/**
 * Global entity settings affecting appearance, render options, etc.
 */
class IEntitySettings
{
public:
    virtual ~IEntitySettings() {}

    virtual const Vector3& getLightVertexColour(LightEditVertexType type) const = 0;
    virtual void setLightVertexColour(LightEditVertexType type, const Vector3& value) = 0;

    virtual bool getRenderEntityNames() const = 0;
    virtual void setRenderEntityNames(bool value) = 0;

    virtual bool getShowAllSpeakerRadii() const = 0;
    virtual void setShowAllSpeakerRadii(bool value) = 0;

    virtual bool getShowAllLightRadii() const = 0;
    virtual void setShowAllLightRadii(bool value) = 0;

    virtual bool getDragResizeEntitiesSymmetrically() const = 0;
    virtual void setDragResizeEntitiesSymmetrically(bool value) = 0;

    virtual bool getAlwaysShowLightVertices() const = 0;
    virtual void setAlwaysShowLightVertices(bool value) = 0;

    virtual bool getFreeObjectRotation() const = 0;
    virtual void setFreeObjectRotation(bool value) = 0;

    virtual bool getShowEntityAngles() const = 0;
    virtual void setShowEntityAngles(bool value) = 0;

    virtual sigc::signal<void>& signal_settingsChanged() = 0;
};

constexpr const char* const MODULE_ENTITY("EntityModule");

/**
 * \brief
 * Interface for the entity module.
 */
class IEntityModule :
    public RegisterableModule
{
public:
    virtual ~IEntityModule() {}

    /// Create an entity node with the given entity class.
    virtual EntityNodePtr createEntity(const IEntityClassPtr& eclass) = 0;

    // Constructs a new targetmanager instance (used by root nodes)
    virtual ITargetManagerPtr createTargetManager() = 0;

    // Access to the settings manager
    virtual IEntitySettings& getSettings() = 0;

    /**
     * Create an instance of the given entity at the given position, and return
     * the Node containing the new entity.
     *
     * @returns: the scene::EntityNodePtr referring to the new entity.
     * @throws: cmd::ExecutionFailure if anything goes wrong or the selection is not suitable.
     */
    virtual EntityNodePtr createEntityFromSelection(const std::string& name, const Vector3& origin) = 0;
};

inline IEntityModule& GlobalEntityModule()
{
    static module::InstanceReference<IEntityModule> _reference(MODULE_ENTITY);
    return _reference;
}
