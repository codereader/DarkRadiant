#include "EClassTreeBuilder.h"

EClassTreeBuilder::EClassTreeBuilder(GtkTreeStore* targetStore) :
	_treeStore(targetStore)
{}

void EClassTreeBuilder::visit(IEntityClassPtr eclass) {
	
}