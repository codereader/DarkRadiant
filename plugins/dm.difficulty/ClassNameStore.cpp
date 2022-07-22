#include "ClassNameStore.h"

namespace ui {

ClassNameStore::ClassNameStore()
{
	populateListStore();
}

const wxArrayString& ClassNameStore::getStringList() const
{
	return _classNames;
}

void ClassNameStore::destroy()
{
	InstancePtr().reset();
}

ClassNameStore& ClassNameStore::Instance()
{
	if (InstancePtr() == NULL)
	{
		InstancePtr().reset(new ClassNameStore);
	}

	return *InstancePtr();
}

ClassNameStorePtr& ClassNameStore::InstancePtr()
{
	static ClassNameStorePtr _instancePtr;
	return _instancePtr;
}

// EntityClassVisitor implementation
void ClassNameStore::visit(const IEntityClassPtr& eclass)
{
	_classNames.Add(eclass->getDeclName());
}

void ClassNameStore::populateListStore()
{
	// Visit each entity class using <this> as visitor
	GlobalEntityClassManager().forEachEntityClass(*this);
}

} // namespace ui
