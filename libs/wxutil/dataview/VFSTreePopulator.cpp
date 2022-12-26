#include "VFSTreePopulator.h"

#include "TreeModel.h"

namespace wxutil
{

VFSTreePopulator::VFSTreePopulator(const TreeModel::Ptr& store, const wxDataViewItem& toplevel) :
	_store(store),
	_topLevel(toplevel)
{}

// Destructor frees the DirIterMap
VFSTreePopulator::~VFSTreePopulator()
{
	_iters.clear();
}

void VFSTreePopulator::setTopLevelItem(const wxDataViewItem& topLevel)
{
    _topLevel = topLevel;
}

// Interface add function
void VFSTreePopulator::addPath(const std::string& path)
{
	// Pass an empty ColumnPopulationCallback
    addRecursive(path, [](TreeModel::Row&, const std::string&, const std::string&, bool) {});

    // Add the explicit path to the set, we need it later
    // when being visited by the Visitor implementation
    _explicitPaths.insert(path);
}

void VFSTreePopulator::addPath(const std::string& path, const ColumnPopulationCallback& func)
{
    // Call the addRecursive method to create all necessary nodes
    addRecursive(path, func);

    // Note that this implementation doesn't maintain the _explicitPaths set
    // since we don't need that information.
}

// Recursive add function
const wxDataViewItem& VFSTreePopulator::addRecursive(const std::string& path, 
    const ColumnPopulationCallback& func, int recursionLevel)
{
	// Look up candidate in the map and return it if found
	NamedIterMap::iterator it = _iters.find(path);

	if (it != _iters.end())
	{
		return it->second;
	}

	/* Otherwise, split the path on its rightmost slash, call recursively on the
	 * first half in order to add the parent node, then add the second half as
	 * a child. Recursive bottom-out is when there is no slash (top-level node).
	 */

	// Find rightmost slash
	std::size_t slashPos = path.rfind("/");

	// Call recursively to get parent iter, leaving it at the toplevel if
	// there is no slash
	const wxDataViewItem& parIter = slashPos != std::string::npos ? 
        addRecursive(path.substr(0, slashPos), func, recursionLevel + 1) : _topLevel;

	// Append a node to the tree view for this child
	TreeModel::Row row = _store->AddItemUnderParent(parIter);

    // Call the population callback. If recursionLevel > 0 we had at least one
    // path split operation, so this must be a folder
    func(row, path, slashPos != std::string::npos ? path.substr(slashPos+1) : path, recursionLevel > 0);

	// Add a copy of the wxDataViewItem to our hashmap and return it
	std::pair<NamedIterMap::iterator, bool> result = _iters.insert(
		NamedIterMap::value_type(path, row.getItem()));

	return result.first->second;
}

// Traversal function
void VFSTreePopulator::forEachNode(Visitor& visitor)
{
	// Visit every entry in the iter map
	for (NamedIterMap::iterator i = _iters.begin(); i != _iters.end(); ++i)
	{
		TreeModel::Row row(i->second, *_store);

		visitor.visit(*_store, row, i->first, _explicitPaths.find(i->first) != _explicitPaths.end());
	}
}

} // namespace
