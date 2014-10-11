#pragma once

#include "ieclass.h"

#include <boost/shared_ptr.hpp>
#include <wx/arrstr.h>

namespace ui
{

class ClassNameStore;
typedef boost::shared_ptr<ClassNameStore> ClassNameStorePtr;

class ClassNameStore :
	private EntityClassVisitor
{
private:
	wxArrayString _classNames;

public:
	// Constructor, traverses the eclasses and fills the GtkListStore
	ClassNameStore();

	const wxArrayString& getStringList() const;

	static ClassNameStore& Instance();

	static void destroy();

private:
	static ClassNameStorePtr& InstancePtr();

	// EntityClassVisitor implementation
	virtual void visit(const IEntityClassPtr& eclass);

	// Traverses all entities and fills the store
	void populateListStore();
};

} // namespace ui
