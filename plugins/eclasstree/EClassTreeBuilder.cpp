#include "EClassTreeBuilder.h"

#include "itextstream.h"
#include "iuimanager.h"
#include "EClassTree.h"

namespace ui {

	namespace {
		const char* ENTITY_ICON = "cmenu_add_entity.png";
		const std::string INHERIT_KEY("inherit");
		
		/**
		 * Visitor class to fill in column data for the tree.
		 */
		class EClassTreeVisitor : 
			public gtkutil::VFSTreePopulator::Visitor
		{
		public:
		    virtual ~EClassTreeVisitor() {}
			// Required visit function
			void visit(GtkTreeStore* store, 
					   GtkTreeIter* it, 
					   const std::string& path,
					   bool isExplicit) 
			{
				// Get the display path, everything after rightmost slash
				std::string displayPath = path.substr(path.rfind("/") + 1);
				
				// Get the icon, either folder or skin
				GdkPixbuf* pixBuf = GlobalUIManager().getLocalPixbuf(ENTITY_ICON);
				
				gtk_tree_store_set(store, it, 
								   NAME_COLUMN, displayPath.c_str(),
								   ICON_COLUMN, pixBuf,
								   -1);
			}
		};
	}

EClassTreeBuilder::EClassTreeBuilder(GtkTreeStore* targetStore) :
	_treeStore(targetStore),
	_treePopulator(_treeStore)
{
	// Travese the entity classes, this will call visit() for each eclass
	GlobalEntityClassManager().forEach(*this);
	
	// Visit the tree populator in order to fill in the column data
	EClassTreeVisitor visitor;
	_treePopulator.forEachNode(visitor);
}

void EClassTreeBuilder::visit(IEntityClassPtr eclass) {
	std::string fullPath;
	
	// Prefix mod name
	fullPath = eclass->getModName() + "/";
	
	// Prefix inheritance path (recursively)
	fullPath += getInheritancePathRecursive(eclass);
	
	// The entityDef name itself
	fullPath += eclass->getName();
	
	// Let the VFSTreePopulator do the insertion
	_treePopulator.addPath(fullPath);
}

std::string EClassTreeBuilder::getInheritancePathRecursive(const IEntityClassPtr& eclass) {
	std::string returnValue;
	
	try {
		EntityClassAttribute attribute = eclass->getAttribute(INHERIT_KEY);
		
		// Don't use empty or derived "inherit" keys
		if (!attribute.value.empty() && !attribute.inherited) {
			
			// Get the inherited eclass first and resolve the path
			IEntityClassPtr parent = GlobalEntityClassManager().findClass(
				attribute.value
			);
			
			if (parent != NULL) {
				returnValue += getInheritancePathRecursive(parent);
			}
			else {
				globalErrorStream() << "EClassTreeBuilder: Cannot resolve inheritance path for " 
									<< eclass->getName().c_str() << "\n";
			}
			
			returnValue += attribute.value + "/";
		}
	}
	catch (std::runtime_error) {
		// no inherit key
	}
	
	return returnValue;
}

} // namespace ui

