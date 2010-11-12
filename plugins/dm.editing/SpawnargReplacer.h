#ifndef _SPAWNARG_REPLACER_H_
#define _SPAWNARG_REPLACER_H_

#include "inode.h"
#include "entitylib.h"

class SpawnargReplacer :
	public scene::NodeVisitor,
	public Entity::Visitor
{
	std::string _oldVal;
	std::string _newVal;

	std::size_t _modelCount;
	std::size_t _otherCount;
	std::size_t _eclassCount;

	// A list of keys to be replaced
	typedef std::vector<std::string> KeyList;
	typedef std::map<scene::INodePtr, KeyList> EntityKeyMap;
	EntityKeyMap _entityMap;

	KeyList _curKeys;

public:
	SpawnargReplacer(const std::string& oldVal, const std::string& newVal) :
		_oldVal(oldVal),
		_newVal(newVal),
		_modelCount(0),
		_otherCount(0),
		_eclassCount(0)
	{}

	bool pre(const scene::INodePtr& node)
	{
		Entity* ent = Node_getEntity(node);

		if (ent != NULL)
		{
			_curKeys.clear();

			// Traverse the entity's spawnargs to check for keys to be replaced
			ent->forEachKeyValue(*this);

			// Save the result of the spawnarg search
			if (!_curKeys.empty())
			{
				_entityMap[node] = _curKeys;
			}

			_curKeys.clear();
		}

		return false;
	}

	// Entity::Visitor
	void visit(const std::string& key, const std::string& value)
	{
		if (value == _oldVal)
		{
			// Matching value, remember this key
			_curKeys.push_back(key);
		}
	}

	void processEntities()
	{
		for (EntityKeyMap::const_iterator e = _entityMap.begin();
			 e != _entityMap.end(); ++e)
		{
			const scene::INodePtr& ent = e->first;
			const KeyList& keys = e->second;

			for (KeyList::const_iterator i = keys.begin();
				 i != keys.end(); ++i)
			{
				// We have a match, check which key is affected
				if (*i == "classname")
				{
					// Classname change
					changeEntityClassname(ent, _newVal);
					_eclassCount++;
				}
				else
				{
					Entity* entity = Node_getEntity(ent);
					assert(entity != NULL);

					entity->setKeyValue(*i, _newVal);

					if (*i == "model")
					{
						_modelCount++;
					}
					else
					{
						_otherCount++;
					}
				}
			}
		}

		_entityMap.clear();
	}

	std::size_t getModelCount() const
	{
		return _modelCount;
	}

	std::size_t getOtherCount() const
	{
		return _otherCount;
	}

	std::size_t getEclassCount() const
	{
		return _eclassCount;
	}
};

#endif /* _SPAWNARG_REPLACER_H_ */
