#include "EClassTreeBuilder.h"

#include "itextstream.h"
#include "iuimanager.h"
#include "EClassTree.h"
#include "debugging/ScopedDebugTimer.h"
#include "gtkutil/TreeModel.h"

#include <wx/artprov.h>
#include <glibmm/thread.h>

namespace ui 
{

namespace
{
	const char* ENTITY_ICON = "cmenu_add_entity.png";
	const std::string INHERIT_KEY("inherit");
}

EClassTreeBuilder::EClassTreeBuilder(const EClassTreeColumns& columns,
									 wxEvtHandler* finishedHandler) :
	_columns(columns),
	_treeStore(new wxutil::TreeModel(_columns)),
	_finishedHandler(finishedHandler),
	_treePopulator(_treeStore),
	_thread(NULL)
{
	wxBitmap icon = wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + ENTITY_ICON);
	_entityIcon.CopyFromBitmap(icon);
}

void EClassTreeBuilder::populate()
{
	if (_thread != NULL)
	{
		return; // there is already a worker thread running
	}

	_thread = Glib::Thread::create(sigc::mem_fun(*this, &EClassTreeBuilder::run), true);
}

void EClassTreeBuilder::run()
{
    ScopedDebugTimer timer("EClassTreeBuilder::run()");

	// Travese the entity classes, this will call visit() for each eclass
	GlobalEntityClassManager().forEachEntityClass(*this);

	// Visit the tree populator in order to fill in the column data
	_treePopulator.forEachNode(*this);

	// Sort the model before returning it
	_treeStore->SortModelByColumn(_columns.name);

	// Send the event to our listener
	wxutil::TreeModel::PopulationFinishedEvent finishedEvent;
	finishedEvent.SetTreeModel(_treeStore);

	_finishedHandler->AddPendingEvent(finishedEvent);
}

void EClassTreeBuilder::visit(const IEntityClassPtr& eclass)
{
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

void EClassTreeBuilder::visit(wxutil::TreeModel* store, wxutil::TreeModel::Row& row,
			   const std::string& path, bool isExplicit)
{
	// Get the display path, everything after rightmost slash
	row[_columns.name] = wxVariant(wxDataViewIconText(
		path.substr(path.rfind("/") + 1), _entityIcon));
}

std::string EClassTreeBuilder::getInheritancePathRecursive(const IEntityClassPtr& eclass)
{
	std::string returnValue;

	try 
	{
		EntityClassAttribute attribute = eclass->getAttribute(INHERIT_KEY);

		// Don't use empty or derived "inherit" keys
		if (!attribute.getValue().empty() && !attribute.inherited)
		{
			// Get the inherited eclass first and resolve the path
			IEntityClassPtr parent = GlobalEntityClassManager().findClass(
				attribute.getValue()
			);

			if (parent != NULL)
			{
				returnValue += getInheritancePathRecursive(parent);
			}
			else
			{
				rError() << "EClassTreeBuilder: Cannot resolve inheritance path for "
					<< eclass->getName() << std::endl;
			}

			returnValue += attribute.getValue() + "/";
		}
	}
	catch (std::runtime_error&)
	{
		// no inherit key
	}

	return returnValue;
}

} // namespace ui

