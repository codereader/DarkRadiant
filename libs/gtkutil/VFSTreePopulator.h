#ifndef VFSTREEPOPULATOR_H_
#define VFSTREEPOPULATOR_H_

#include <gtk/gtktreestore.h>
#include <ext/hash_map>
#include <string>

#include <boost/functional/hash/hash.hpp>

namespace gtkutil
{
	
/** Utility class to construct a GtkTreeStore from a series of string paths
 * in the form "models/first/second/object.lwo" or similar. The class accepts
 * the Tree Store and then string paths, one by one, adding each one to the
 * tree in the appropriate place.
 * 
 * Since the VFSTreePopulator has no knowledge of the column data to be inserted
 * into the tree, it does not set any of the values but merely calls
 * gtk_tree_store_append to get a GtkTreeIter* pointing to the newly-added
 * row. In order to insert the data, the calling code must then call the 
 * visitor function which will provide the visitor with two objects - the
 * created GtkTreeIter*, and the full input string which the visitor can then
 * use to populate the data appropriately.
 * 
 * When the VFSTreePopulator is destroyed it will free any temporary structures
 * used during tree creation, such as the hashtable of GtkTreeIter* objects
 * used to locate parent nodes. The GtkTreeModel will not be destroyed since it
 * is owned by the calling code.
 */
class VFSTreePopulator
{
	// The GtkTreeStore to populate
	GtkTreeStore* _store;
	
	// Maps of names to corresponding GtkTreeIter* nodes, for both intermediate
	// paths and explicitly presented paths
	typedef __gnu_cxx::hash_map<std::string, 
								GtkTreeIter*, 
								boost::hash<std::string> > NamedIterMap;
	NamedIterMap _iters;
	
private:

	// Main recursive add function. Accepts a VFS path and adds the new node,
	// calling itself if necessary to add all of the parent directories, then
	// returns a GtkTreeIter* pointing to the new node.
	GtkTreeIter* addRecursive(const std::string& path);
	
public:

	/** Construct a VFSTreePopulator which will populate the given tree store.
	 */
	VFSTreePopulator(GtkTreeStore* store);

	/** Destroy the VFSTreePopulator and all temporary data.
	 */
	~VFSTreePopulator();
	
	/** Add a single VFS string to the tree, which will be split automatically
	 * and inserted at the correct place in the tree.
	 */
	void addPath(const std::string& path);

	/** Visitor interface.
	 */
	struct Visitor {
		virtual void visit(GtkTreeStore* store,
						   GtkTreeIter* iter, 
						   const std::string& path) = 0;
	};
	
	/** Visit each node in the constructed tree, passing the GtkTreeIter* and
	 * the VFS string to the visitor object so that data can be inserted.
	 */
	void forEachNode(Visitor& visitor);
};

}

#endif /*VFSTREEPOPULATOR_H_*/
