#ifndef _NAMESPACE_MANAGER_H_
#define _NAMESPACE_MANAGER_H_

#include "ientity.h"
#include "inamespace.h"

#include <map>
#include "Doom3Entity.h"
#include "KeyValueObserver.h"
#include "NameKeyObserver.h"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/noncopyable.hpp>

namespace entity {

class NamespaceManager : 
	public Entity::Observer,
	public Namespaced,
	public boost::noncopyable
{
	INamespace* _namespace;
	
	// The attached entity
	Doom3Entity& _entity;
	
	// All the observed key values of the entity get remembered
	// This prevents having to traverse all the keyvalues again when changing namespaces
	typedef std::map<std::string, EntityKeyValue*> KeyValues;
	KeyValues _nameKeys;

	typedef std::map<EntityKeyValue*, NameKeyObserverPtr> NameKeyObserverMap;
	NameKeyObserverMap _nameKeyObservers;

	typedef std::map<EntityKeyValue*, KeyValueObserverPtr> KeyValueObserverMap;
	KeyValueObserverMap _keyValueObservers;

	// lock for this class to avoid double-updates
	bool _updateMutex;

public:
	NamespaceManager(Doom3Entity& entity);

	~NamespaceManager();

	// Gets/sets the namespace of this named object
	void setNamespace(INamespace* space);
	INamespace* getNamespace() const;

	void attachNames();
	void detachNames();

	void connectNameObservers();
	void disconnectNameObservers();

	// Returns or changes the name of this entity
	std::string getName();
	void changeName(const std::string& newName);

	/** 
	 * greebo: This gets called as soon as a new entity key/value gets added
	 * to the attached entity.
	 * 
	 * The routine saves all relevant keyvalues and attaches the 
	 * "name keys" to the Namespace.
	 * 
	 * Note: Entity::Observer implementation
	 */
	void onKeyInsert(const std::string& key, EntityKeyValue& value);
	
	/** 
	 * greebo: Gets called by the observed Entity when a value is erased from
	 * the list of spawnargs.
	 * 
	 * Note: Entity::Observer implementation
	 */
	void onKeyErase(const std::string& key, EntityKeyValue& value);

	/** 
	 * greebo: returns TRUE if the given key is recognised as "name" for the 
	 * selected game type.
	 */
	static bool keyIsName(const std::string& key);

private:
	// Freshly attaches all "names" to our namespace
	void attachNameKeys();
	void detachNameKeys();

	void attachKeyObservers();
	void detachKeyObservers();

	void attachKeyToNamespace(const std::string& key, EntityKeyValue& keyValue);
	void detachKeyFromNamespace(const std::string& key, EntityKeyValue& keyValue);

	void attachKeyObserver(const std::string& key, EntityKeyValue& keyValue);
	void detachKeyObserver(const std::string& key, EntityKeyValue& keyValue);
};

} // namespace entity

#endif /* _NAMESPACE_MANAGER_H_ */
