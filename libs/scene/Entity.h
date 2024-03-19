#pragma once

#include "scene/AttachmentData.h"
#include "scene/EntityKeyValue.h"

#include <vector>
#include <memory>

class IUndoSystem;

/**
 * The Entity is the main building block of a map, and the uppermost layer in the scenegraph
 * under the root node. Each entity contains a arbitrary dictionary of strings ("properties"
 * or "spawnargs") containing information about this entity which is used by the game engine
 * to modify its behaviour, and may additionally contain child primitives (brushes and
 * patches) depending on its type.
 *
 * At the minimum, each Entity must contain three properties: "name" which contains a
 * map-unique string identifier, "classname" which identifies the entity class to the game,
 * and "origin" which stores the location of the entity in 3-dimensional world space.
 *
 * A valid <b>Id Tech 4</b> map must contain at least one entity: the "worldspawn" which is
 * the parent of all map geometry primitives.
 *
 * greebo: Note that keys are treated case-insensitively in Doom 3, so the Entity class will
 * return the same result for "MYKeY" as for "mykey".
 */
class Entity
{
public:

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

        /// Notification that a new key value has been inserted on the entity.
        virtual void onKeyInsert(const std::string& key, EntityKeyValue& value) {}

        /// Notification that a key value has changed on the entity.
        virtual void onKeyChange(const std::string& key, const std::string& val) {}

        /// Notification that a key value has been removed from the entity.
        virtual void onKeyErase(const std::string& key, EntityKeyValue& value) {}
    };

private:
    IEntityClassPtr _eclass;

	typedef std::shared_ptr<EntityKeyValue> KeyValuePtr;

	// A key value pair using a dynamically allocated value
	typedef std::pair<std::string, KeyValuePtr> KeyValuePair;

	// The unsorted list of KeyValue pairs
	typedef std::vector<KeyValuePair> KeyValues;
	KeyValues _keyValues;

	typedef std::set<Observer*> Observers;
	Observers _observers;

	undo::ObservedUndoable<KeyValues> _undo;

	bool _observerMutex;

	bool _isContainer;

    // Store attachment information
    entity::AttachmentData _attachments;

public:
    // A container with key => value pairs
    typedef std::vector< std::pair<std::string, std::string> > KeyValuePairs;

    // Function typedef to visit actual EntityKeyValue objects, not just the string values
    typedef std::function<void(const std::string& key, EntityKeyValue& value)> EntityKeyValueVisitFunctor;

    /// Functor to receive keys and values as strings
    using KeyValueVisitFunc = std::function<void(const std::string&, const std::string&)>;

	// Constructor, pass the according entity class
	Entity(const IEntityClassPtr& eclass);

	// Copy constructor
	Entity(const Entity& other);

	void importState(const KeyValues& keyValues);

    /// Attach an Entity::Observer to this Entity.
    void attachObserver(Observer* observer);

    /// Detach an Entity::Observer from this Entity.
    void detachObserver(Observer* observer);

    void connectUndoSystem(IUndoSystem& undoSystem);
    void disconnectUndoSystem(IUndoSystem& undoSystem);

    /// Return the entity class object for this entity.
	IEntityClassPtr getEntityClass() const;

    /**
     * \brief Enumerate all keys and values on this entity, optionally including
     * inherited spawnargs.
     *
     * \param func
     * Functor to receive each key and its associated value.
     *
     * \param includeInherited
     * true if the functor should be invoked for each inherited spawnarg (from
     * the entity class), false if only explicit spawnargs on this particular
     * entity should be visited.
     */
    void forEachKeyValue(KeyValueVisitFunc func, bool includeInherited = false) const;

    void forEachEntityKeyValue(const EntityKeyValueVisitFunctor& visitor);

    /** Set a key value on this entity. Setting the value to "" will
     * remove the key.
     *
     * @param key
     * The key to set.
     *
     * @param value
     * Value to give the key, or the empty string to remove the key.
     */
	void setKeyValue(const std::string& key, const std::string& value);

    /* Retrieve a key value from the entity.
     *
     * @param key
     * The key to retrieve.
     *
     * @returns
     * The current value for this key, or the empty string if it does not
     * exist.
     */
	std::string getKeyValue(const std::string& key) const;

    /**
     * \brief Return the list of keyvalues matching the given prefix.
     *
     * This method performs a search for all spawnargs whose key matches the
     * given prefix, with a suffix consisting of zero or more arbitrary
     * characters. For example, if "target" were specified as the prefix, the
     * list would include "target", "target0", "target127" etc.
     *
     * This operation may not have high performance, due to the need to scan
     * for matching names, therefore should not be used in performance-critical
     * code.
     *
     * @param prefix
     * The prefix to search for, interpreted case-insensitively.
     *
     * @return
     * A list of KeyValue pairs matching the provided prefix. This list will be
     * empty if there were no matches.
     */
    KeyValuePairs getKeyValuePairs(const std::string& prefix) const
    {
        KeyValuePairs list;

        forEachKeyValue([&](const std::string& k, const std::string& v) {
            if (string::istarts_with(k, prefix))
                list.push_back(std::make_pair(k, v));
        });

        return list;
    }

    /**
     * greebo: Checks whether the given key is inherited or not.
     *
     * @returns: TRUE if the value is inherited,
     *           FALSE when it is not or when the key doesn't exist at all.
     */
	bool isInherited(const std::string& key) const;

	/**
	 * Returns true if this entity is the worldspawn, which can be game-specific,
	 * but is usually true if this entity's classname equals "worldspawn"
	 */
	bool isWorldspawn() const;
	bool isContainer() const;
	void setIsContainer(bool isContainer);

    /** greebo: Returns true if the entity is a model. For Doom3, this is
     *          usually true when the classname == "func_static" and
     *          the non-empty spawnarg "model" != "name".
     */
	bool isModel() const;

	// Returns the actual pointer to a KeyValue (or NULL if not found),
	// not just the string like getKeyValue() does.
	// Only returns non-NULL for non-inherited keyvalues.
	EntityKeyValuePtr getEntityKeyValue(const std::string& key);

	/**
	 * Returns true if this entity is of type or inherits from the
	 * given entity class name. className is treated case-sensitively.
	 */
	bool isOfType(const std::string& className);

    /* ENTITY ATTACHMENTS */

    /// A functor which can receive Attachment objects
    using AttachmentFunc = std::function<void(const EntityAttachment&)>;

    /**
     * \brief Iterate over attached entities, if any.
     *
     * Each entity can define one or more attached entities, which should
     * appear at specific offsets relative to the parent entity. Such attached
     * entities are for visualisation only, and should not be saved into the
     * map as genuine map entities.
     *
     * \param func
     * Functor to receive attachment information.
     */
    void forEachAttachment(AttachmentFunc func) const;

private:

    // Parse attachment information from def_attach and related keys (which are
    // most likely on the entity class, not the entity itself)
    void parseAttachments();

    // Notification functions
	void notifyInsert(const std::string& key, EntityKeyValue& value);
    void notifyChange(const std::string& k, const std::string& v);
	void notifyErase(const std::string& key, EntityKeyValue& value);

	void insert(const std::string& key, const KeyValuePtr& keyValue);
	void insert(const std::string& key, const std::string& value);

	void erase(const KeyValues::iterator& i);
	void erase(const std::string& key);

	KeyValues::iterator find(const std::string& key);
	KeyValues::const_iterator find(const std::string& key) const;
};
