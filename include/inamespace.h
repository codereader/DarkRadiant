/*
Copyright (C) 2001-2006, William Joseph.
All Rights Reserved.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#if !defined(INCLUDED_NAMESPACE_H)
#define INCLUDED_NAMESPACE_H

#include "inode.h"
#include "imodule.h"
#include "generic/callbackfwd.h"

typedef Callback1<const std::string&> NameCallback;
typedef Callback1<const NameCallback&> NameCallbackCallback;

const std::string MODULE_NAMESPACE("Namespace");

class INamespace :
	public RegisterableModule
{
public:
  virtual void attach(const NameCallback& setName, const NameCallbackCallback& attachObserver) = 0;
  virtual void detach(const NameCallback& setName, const NameCallbackCallback& detachObserver) = 0;
  virtual void makeUnique(const char* name, const NameCallback& setName) const = 0;

	/** greebo: Collects all Namespaced nodes in the subgraph,
	 * 			whose starting point is defined by <root>.
	 * 			This stores all the Namespaced* objects into 
	 * 			a local list, which can subsequently be used 
	 * 			by mergeClonedNames().
	 */
	virtual void gatherNamespaced(scene::INodePtr root) = 0;
	
	/** greebo: This moves all gathered Namespaced nodes into this
	 * 			Namespace, making sure that all names are properly
	 * 			made unique.
	 */
	virtual void mergeClonedNames() = 0;
};

class Namespaced
{
public:
	virtual void setNamespace(INamespace& space) = 0;
};
typedef boost::shared_ptr<Namespaced> NamespacedPtr;

inline INamespace& GlobalNamespace() {
	// Cache the reference locally
	static INamespace& _namespace(
		*boost::static_pointer_cast<INamespace>(
			module::GlobalModuleRegistry().getModule(MODULE_NAMESPACE)
		)
	);
	return _namespace;
}
#endif
