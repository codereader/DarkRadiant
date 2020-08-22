#pragma once

#include "inode.h"
#include "ipath.h"
#include "imodule.h"
#include "irender.h"
#include "inameobserver.h"
#include <functional>

class IEntityClass;
typedef std::shared_ptr<IEntityClass> IEntityClassPtr;
typedef std::shared_ptr<const IEntityClass> IEntityClassConstPtr;

// Observes a single entity key value and gets notified on change
class KeyObserver
{
public:
    virtual ~KeyObserver() {}

    /**
     * This event gets called when the observed keyvalue changes.
     * The new value is passed as argument, which can be an empty string.
     */
    virtual void onKeyValueChanged(const std::string& newValue) = 0;
};

class EntityKeyValue :
    public NameObserver
{
public:
    virtual ~EntityKeyValue() {}
    /** greebo: Retrieves the actual value of this key
     */
    virtual const std::string& get() const = 0;

    /** greebo: Sets the value of this key
     */
    virtual void assign(const std::string& other) = 0;

    /** greebo: Attaches/detaches a callback to get notified about
     *          the key change.
     */
    virtual void attach(KeyObserver& observer) = 0;
    virtual void detach(KeyObserver& observer) = 0;
};
typedef std::shared_ptr<EntityKeyValue> EntityKeyValuePtr;

/**
 * Interface for a map entity. The Entity is the main building block of a
 * map, and the uppermost layer in the scenegraph under the root node. Each
 * entity contains a arbitrary dictionary of strings ("properties" or
 * "spawnargs") containing information about this entity which is used by the
 * game engine to modify its behaviour, and may additionally contain child
 * primitives (brushes and patches) depending on its type.
 *
 * At the minimum, each Entity must contain three properties: "name" which
 * contains a map-unique string identifier, "classname" which identifies the
 * entity class to the game, and "origin" which stores the location of the
 * entity in 3-dimensional world space.
 *
 * A valid <b>Id Tech 4</b> map must contain at least one entity: the
 * "worldspawn" which is the parent of all map geometry primitives.
 *
 * greebo: Note that keys are treated case-insensitively in Doom 3, so
 * the Entity class will return the same result for "MYKeY" as for "mykey".
 */
class Entity
{
public:
    // A container with key => value pairs
    typedef std::vector< std::pair<std::string, std::string> > KeyValuePairs;

    /**
     * \brief
     * Abstract base class for entity observers.
     *
     * An entity observer receives notifications when keyvalues are inserted or
     * deleted on the entity it is observing.
     */
    class Observer
    {
    public:
        virtual ~Observer() {}

        /**
         * \brief
         * Notification that a new key value has been inserted on the entity.
         */
        virtual void onKeyInsert(const std::string& key, EntityKeyValue& value)
        { }

        /**
         * \brief
         * Notification that a key value has changed on the entity.
         */
        virtual void onKeyChange(const std::string& key, const std::string& val)
        { }

        /**
         * \brief
         * Notification that a key value has been removed from the entity.
         */
        virtual void onKeyErase(const std::string& key, EntityKeyValue& value)
        { }
    };

    // Function typedef to visit keyvalues
    typedef std::function<void(const std::string& key, const std::string& value)> KeyValueVisitFunctor;

    // Function typedef to visit actual EntityKeyValue objects, not just the string values
    typedef std::function<void(const std::string& key, EntityKeyValue& value)> EntityKeyValueVisitFunctor;

    virtual ~Entity() {}

    /**
     * Return the entity class object for this entity.
     */
    virtual IEntityClassPtr getEntityClass() const = 0;

    /**
     * Enumerate key values on this entity using a function object taking
     * key and value as string arguments.
     */
    virtual void forEachKeyValue(const KeyValueVisitFunctor& visitor) const = 0;

    // Similar to above, visiting the EntityKeyValue objects itself, not just the string value.
    virtual void forEachEntityKeyValue(const EntityKeyValueVisitFunctor& visitor) = 0;

    /** Set a key value on this entity. Setting the value to "" will
     * remove the key.
     *
     * @param key
     * The key to set.
     *
     * @param value
     * Value to give the key, or the empty string to remove the key.
     */
    virtual void setKeyValue(const std::string& key,
                             const std::string& value) = 0;

    /* Retrieve a key value from the entity.
     *
     * @param key
     * The key to retrieve.
     *
     * @returns
     * The current value for this key, or the empty string if it does not
     * exist.
     */
    virtual std::string getKeyValue(const std::string& key) const = 0;

    /**
     * greebo: Checks whether the given key is inherited or not.
     *
     * @returns: TRUE if the value is inherited,
     *           FALSE when it is not or when the key doesn't exist at all.
     */
    virtual bool isInherited(const std::string& key) const = 0;

    /**
     * Return the list of Key/Value pairs matching the given prefix, case ignored.
     *
     * This method performs a search for all spawnargs whose key
     * matches the given prefix, with a suffix consisting of zero or more
     * arbitrary characters. For example, if "target" were specified as the
     * prefix, the list would include "target", "target0", "target127" etc.
     *
     * This operation may not have high performance, due to the need to scan
     * for matching names, therefore should not be used in performance-critical
     * code.
     *
     * @param prefix
     * The prefix to search for, interpreted case-insensitively.
     *
     * @return
     * A list of KeyValue pairs matching the provided prefix. This
     * list will be empty if there were no matches.
     */
    virtual KeyValuePairs getKeyValuePairs(const std::string& prefix) const = 0;

    /** greebo: Returns true if the entity is a model. For Doom3, this is
     *          usually true when the classname == "func_static" and
     *          the non-empty spawnarg "model" != "name".
     */
    virtual bool isModel() const = 0;

	/**
	 * Returns true if this entity is the worldspawn, which can be game-specific,
	 * but is usually true if this entity's classname equals "worldspawn"
	 */
	virtual bool isWorldspawn() const = 0;

	virtual bool isContainer() const = 0;

    /**
     * \brief
     * Attach an Entity::Observer to this Entity.
     */
    virtual void attachObserver(Observer* observer) = 0;

    /**
     * \brief
     * Detach an Entity::Observer from this Entity.
     */
    virtual void detachObserver(Observer* observer) = 0;

	/**
	 * Returns true if this entity is of type or inherits from the 
	 * given entity class name. className is treated case-sensitively.
	 */
	virtual bool isOfType(const std::string& className) = 0;
};

/// Interface for a INode subclass that contains an Entity
class IEntityNode :
    public virtual IRenderEntity,
    public virtual scene::INode
{
public:
    virtual ~IEntityNode() {}

    /// Get a modifiable reference to the contained Entity
    virtual Entity& getEntity() = 0;

    /**
     * greebo: Tells the entity to reload the child model. This usually
     *         includes removal of the child model node and triggering
     *         a "skin changed" event.
     */
    virtual void refreshModel() = 0;
};
typedef std::shared_ptr<IEntityNode> IEntityNodePtr;

inline Entity* Node_getEntity(const scene::INodePtr& node)
{
    IEntityNodePtr entityNode = std::dynamic_pointer_cast<IEntityNode>(node);

    if (entityNode != NULL) {
        return &(entityNode->getEntity());
    }

    return NULL;
}

inline bool Node_isEntity(const scene::INodePtr& node)
{
    //assert(!((std::dynamic_pointer_cast<IEntityNode>(node) != nullptr) ^ (node->getNodeType() == scene::INode::Type::Entity)));
    return node->getNodeType() == scene::INode::Type::Entity;
}

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
    virtual ITargetableObjectPtr getTarget(const std::string name) = 0;

    /**
     * greebo: Associates the named Target with the given scene::INode.
     * The Target will be created if it doesn't exist yet.
     */
    virtual void associateTarget(const std::string& name, const scene::INode& node) = 0;

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

const char* const RKEY_SHOW_ENTITY_NAMES("user/ui/xyview/showEntityNames");
const char* const RKEY_SHOW_ALL_SPEAKER_RADII = "user/ui/showAllSpeakerRadii";
const char* const RKEY_SHOW_ALL_LIGHT_RADII = "user/ui/showAllLightRadii";
const char* const RKEY_DRAG_RESIZE_SYMMETRICALLY = "user/ui/dragResizeEntitiesSymmetrically";
const char* const RKEY_ALWAYS_SHOW_LIGHT_VERTICES = "user/ui/alwaysShowLightVertices";
const char* const RKEY_FREE_OBJECT_ROTATION = "user/ui/rotateObjectsIndependently";
const char* const RKEY_SHOW_ENTITY_ANGLES = "user/ui/xyview/showEntityAngles";

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

const char* const MODULE_ENTITY("EntityModule");

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
    virtual IEntityNodePtr createEntity(const IEntityClassPtr& eclass) = 0;

    // Constructs a new targetmanager instance (used by root nodes)
    virtual ITargetManagerPtr createTargetManager() = 0;

    // Access to the settings manager
    virtual IEntitySettings& getSettings() = 0;

    /**
     * Create an instance of the given entity at the given position, and return
     * the Node containing the new entity.
     *
     * @returns: the scene::IEntityNodePtr referring to the new entity.
     * @throws: cmd::ExecutionFailure if anything goes wrong or the selection is not suitable.
     */
    virtual IEntityNodePtr createEntityFromSelection(const std::string& name, const Vector3& origin) = 0;
};

inline IEntityModule& GlobalEntityModule()
{
    // Cache the reference locally
    static IEntityModule& _entityCreator(
        *std::static_pointer_cast<IEntityModule>(
            module::GlobalModuleRegistry().getModule(MODULE_ENTITY)
        )
    );
    return _entityCreator;
}
