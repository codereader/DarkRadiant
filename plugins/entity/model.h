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

#if !defined(INCLUDED_MODEL_H)
#define INCLUDED_MODEL_H

#include "ireference.h"
#include "itraversable.h"
#include "generic/callback.h"
#include "moduleobserver.h"

class SingletonModel :
	public Resource::Observer
{
	ReferenceCache::ResourcePtr _resource;
	scene::INodePtr _node;
	
	// The traversable container, where the model node can be added to
	scene::Traversable& _traversable;

	std::string _modelPath;

public:
	SingletonModel(scene::Traversable& traversable);
	~SingletonModel();
	
	// Resource::Observer implementation
	
	// This gets called, when the ModelResource is loaded and the scene::INodePtr
	// can be inserted into the scene::Traversable (which is what this method does).
	void onResourceRealise();
	// The counter-part of the above
	void onResourceUnrealise();

	// Update the model to the provided keyvalue, this removes the old scene::Node
	// and inserts the new one after acquiring the ModelResource from the ReferenceCache.
	void modelChanged(const std::string& value);
	typedef MemberCaller1<SingletonModel, const std::string&, &SingletonModel::modelChanged> ModelChangedCaller;

	// Returns the reference to the "singleton" model node
	scene::INodePtr getNode() const;
};

#endif
