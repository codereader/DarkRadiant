#ifndef VFSTREEPOPULATOR_H_
#define VFSTREEPOPULATOR_H_

#include <gtkmm/treestore.h>

#include <map>
#include <string>
#include <set>

#include <boost/functional/hash/hash.hpp>

namespace gtkutil
{

/**
 * Utility class to construct a Gtk::TreeStore from a series of string paths
 * in the form "models/first/second/object.lwo" or similar. The class accepts
 * the Tree Store and then string paths, one by one, adding each one to the
 * tree in the appropriate place.
 *
 * Since the VFSTreePopulator has no knowledge of the column data to be inserted
 * into the tree, it does not set any of the values but merely calls
 * TreeStore::append() to get an iterator pointing to the newly-added
 * row. In order to insert the data, the calling code must then call the
 * visitor function which will provide the visitor with two objects - the
 * created iterator and the full input string which the visitor can then
 * use to populate the data appropriately.
 *
 * When the VFSTreePopulator is destroyed it will free any temporary structures
 * used during tree creation, such as the hashtable of iterator objects
 * used to locate parent nodes. The TreeModel will not be destroyed since it
 * is owned by the calling code.
 */
class VFSTreePopulator
{
private:
	// The GtkTreeStore to populate
	Glib::RefPtr<Gtk::TreeStore> _store;

	// Toplevel node to add children under
	Gtk::TreeModel::iterator _topLevel;

	// Maps of names to corresponding treemodel iterators, for both intermediate
	// paths and explicitly presented paths
	typedef std::map<std::string, Gtk::TreeModel::iterator> NamedIterMap;
	NamedIterMap _iters;

	// Set of paths that are passed in through addPath(), to distinguish them
	// from intermediate constructed paths
	std::set<std::string> _explicitPaths;

private:

	// Main recursive add function. Accepts a VFS path and adds the new node,
	// calling itself if necessary to add all of the parent directories, then
	// returns a Gtk::TreeModel::iterator pointing to the new node.
	const Gtk::TreeModel::iterator& addRecursive(const std::string& path);

public:

	/**
	 * Construct a VFSTreePopulator which will populate the given tree store.
	 *
	 * @param store
	 * Tree store to populate.
	 *
	 * @param toplevel
	 * TreeModel::iterator pointing to the toplevel node, under which all paths should
	 * be added. Default is empty to indicate that paths should be added under
	 * the tree root.
	 */
	VFSTreePopulator(const Glib::RefPtr<Gtk::TreeStore>& store,
					   Gtk::TreeModel::iterator toplevel = Gtk::TreeModel::iterator());

	/** Destroy the VFSTreePopulator and all temporary data.
	 */
	virtual ~VFSTreePopulator();

	/** Add a single VFS string to the tree, which will be split automatically
	 * and inserted at the correct place in the tree.
	 */
	void addPath(const std::string& path);

	/** Visitor interface.
	 */
	struct Visitor
	{
		virtual ~Visitor() {}

		/**
		 * Visit callback function, called for each node in the tree.
		 *
		 * @param store
		 * The tree store to insert into.
		 *
		 * @param iter
		 * A Gtk::TreeModel::iterator* pointing to the current node.
		 *
		 * @param path
		 * Full VFS path of the current node, as presented to addPath().
		 *
		 * @param isExplicit
		 * true if the path was explicitly inserted via addPath(), false if the
		 * path was created as an intermediate parent of another explicit path.
		 */
		virtual void visit(const Glib::RefPtr<Gtk::TreeStore>& store,
						   const Gtk::TreeModel::iterator& iter,
						   const std::string& path,
						   bool isExplicit) = 0;
	};

	/** Visit each node in the constructed tree, passing the GtkTreeIter* and
	 * the VFS string to the visitor object so that data can be inserted.
	 */
	void forEachNode(Visitor& visitor);
};

}

#endif /*VFSTREEPOPULATOR_H_*/
