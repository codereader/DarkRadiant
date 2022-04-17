#include "EClassTreeBuilder.h"

#include "itextstream.h"
#include "EClassTree.h"
#include "debugging/ScopedDebugTimer.h"
#include "wxutil/dataview/TreeModel.h"

#include "wxutil/Bitmap.h"

namespace ui
{

namespace
{
	const char* ENTITY_ICON = "cmenu_add_entity.png";
	const std::string INHERIT_KEY("inherit");
}

EClassTreeBuilder::EClassTreeBuilder(const EClassTreeColumns& columns,
									 wxEvtHandler* finishedHandler) :
	wxThread(wxTHREAD_JOINABLE),
	_columns(columns),
	_treeStore(new wxutil::TreeModel(_columns)),
	_finishedHandler(finishedHandler),
	_treePopulator(_treeStore)
{
	wxBitmap icon = wxutil::GetLocalBitmap(ENTITY_ICON);
	_entityIcon.CopyFromBitmap(icon);
}

EClassTreeBuilder::~EClassTreeBuilder()
{
	// We might have a running thread, wait for it
	if (IsRunning())
	{
		Delete();
	}
}

wxThread::ExitCode EClassTreeBuilder::Entry()
{
	ScopedDebugTimer timer("EClassTreeBuilder::run()");

	// Travese the entity classes, this will call visit() for each eclass
	GlobalEntityClassManager().forEachEntityClass(*this);

	if (TestDestroy()) return static_cast<wxThread::ExitCode>(0);

	// Visit the tree populator in order to fill in the column data
	_treePopulator.forEachNode(*this);

	if (TestDestroy()) return static_cast<wxThread::ExitCode>(0);

	// Sort the model before returning it
	_treeStore->SortModelByColumn(_columns.name);

	if (!TestDestroy())
	{
		// Send the event to our listener, only if we are not forced to finish
		wxQueueEvent(_finishedHandler, new wxutil::TreeModel::PopulationFinishedEvent(_treeStore));
	}

	return static_cast<wxThread::ExitCode>(0);
}

void EClassTreeBuilder::populate()
{
	if (IsRunning()) return;

	Run();
}

void EClassTreeBuilder::visit(const IEntityClassPtr& eclass)
{
	if (TestDestroy())
	{
		return;
	}

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

void EClassTreeBuilder::visit(wxutil::TreeModel& /* store */, wxutil::TreeModel::Row& row,
			   const std::string& path, bool isExplicit)
{
	if (TestDestroy()) return;

	// Get the display path, everything after rightmost slash
	row[_columns.name] = wxVariant(wxDataViewIconText(
		path.substr(path.rfind("/") + 1), _entityIcon));
}

std::string EClassTreeBuilder::getInheritancePathRecursive(const IEntityClassPtr& eclass)
{
    std::string returnValue;

    try {
        std::string attribute = eclass->getAttributeValue(
            INHERIT_KEY, false /* includeInherited*/
        );

        // Don't use empty "inherit" keys
        if (!attribute.empty()) {
            // Get the inherited eclass first and resolve the path
            IEntityClassPtr parent = GlobalEntityClassManager().findClass(attribute);
            if (parent) {
                returnValue += getInheritancePathRecursive(parent);
            } else {
                rError() << "EClassTreeBuilder: Cannot resolve inheritance path for "
                         << eclass->getName() << std::endl;
            }

            returnValue += attribute + "/";
        }
    }
    catch (std::runtime_error&) {
        // no inherit key
    }

    return returnValue;
}

} // namespace ui

