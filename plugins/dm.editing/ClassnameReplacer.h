#ifndef _CLASSNAME_REPLACER_H_
#define _CLASSNAME_REPLACER_H_

#include "inode.h"
#include "entitylib.h"

class ClassnameReplacer :
	public scene::NodeVisitor
{
	std::string _oldClass;
	std::string _newClass;

	std::size_t _eclassCount;

	// A list of keys to be replaced
	typedef std::vector<scene::INodePtr> EntityList;
	EntityList _entities;
	
public:
	ClassnameReplacer(const std::string& oldClass, const std::string& newClass) :
		_oldClass(oldClass),
		_newClass(newClass),
		_eclassCount(0)
	{}

	bool pre(const scene::INodePtr& node)
	{
		Entity* ent = Node_getEntity(node);

		if (ent != NULL) 
		{
			if (ent->getKeyValue("classname") == _oldClass)
			{
				_entities.push_back(node);
			}
		}

		return false;
	}

	void processEntities()
	{
		for (EntityList::const_iterator e = _entities.begin();
			 e != _entities.end(); ++e)
		{
			changeEntityClassname(*e, _newClass);
			_eclassCount++;
		}

		_entities.clear();
	}

	std::size_t getEclassCount() const
	{
		return _eclassCount;
	}
};

#endif /* _CLASSNAME_REPLACER_H_ */
