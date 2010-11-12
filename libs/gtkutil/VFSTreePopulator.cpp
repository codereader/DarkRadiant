#include "VFSTreePopulator.h"

namespace gtkutil
{

VFSTreePopulator::VFSTreePopulator(const Glib::RefPtr<Gtk::TreeStore>& store,
									Gtk::TreeModel::iterator toplevel) :
	_store(store),
	_topLevel(toplevel)
{}

// Destructor frees the DirIterMap
VFSTreePopulator::~VFSTreePopulator()
{
	_iters.clear();
}

// Interface add function
void VFSTreePopulator::addPath(const std::string& path)
{
	// Call the addRecursive method to create all necessary nodes
	addRecursive(path);

	// Add the explicit path to the set
	_explicitPaths.insert(path);
}

// Recursive add function
const Gtk::TreeModel::iterator& VFSTreePopulator::addRecursive(const std::string& path)
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
	const Gtk::TreeModel::iterator& parIter =
		slashPos != std::string::npos ? addRecursive(path.substr(0, slashPos)) : _topLevel;

	// Append a node to the tree view for this child
	Gtk::TreeModel::iterator iter = parIter ? _store->append(parIter->children()) : _store->append();

	// Add a copy of the Gtk::TreeModel::iterator to our hashmap and return it
	std::pair<NamedIterMap::iterator, bool> result = _iters.insert(
		NamedIterMap::value_type(path, iter));

	return result.first->second;
}

// Traversal function
void VFSTreePopulator::forEachNode(Visitor& visitor)
{
	// Visit every entry in the iter map
	for (NamedIterMap::iterator i = _iters.begin(); i != _iters.end(); ++i)
	{
		visitor.visit(_store, i->second, i->first,
					  _explicitPaths.find(i->first) != _explicitPaths.end());
	}
}

} // namespace
