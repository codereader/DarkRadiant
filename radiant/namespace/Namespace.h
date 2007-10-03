#ifndef NAMESPACE_H_
#define NAMESPACE_H_

#include <map>
#include <vector>
#include "inamespace.h"
#include "iscenegraph.h"
#include "uniquenames.h"
#include "NameObserver.h"

class Namespace : 
	public INamespace
{
	typedef std::map<NameCallback, NameObserver> Names;
	Names m_names;
	UniqueNames m_uniqueNames;
	
	// This is the list populated by gatherNamespaced(), see below
	typedef std::vector<NamespacedPtr> NamespacedList;
	NamespacedList _cloned;

public:
	void attach(const NameCallback& setName, const NameCallbackCallback& attachObserver);
	void detach(const NameCallback& setName, const NameCallbackCallback& detachObserver);

	void makeUnique(const char* name, const NameCallback& setName) const;

	void mergeNames(const Namespace& other) const;
	
	/** greebo: Collects all Namespaced nodes in the subgraph,
	 * 			whose starting point is defined by <root>.
	 * 			This stores all the Namespaced* objects into 
	 * 			a local list, which can subsequently be used 
	 * 			by mergeClonedNames().
	 */
	void gatherNamespaced(scene::INodePtr root);
	
	/** greebo: This moves all gathered Namespaced nodes into this
	 * 			Namespace, making sure that all names are properly
	 * 			made unique.
	 */
	void mergeClonedNames();
	
	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);
}; // class BasicNamespace

#endif /*NAMESPACE_H_*/
