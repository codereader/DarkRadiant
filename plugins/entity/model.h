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
#include "traverselib.h"
#include "generic/callback.h"
#include "moduleobserver.h"

class SingletonModel :
	public TraversableNode, // implements scene::Traversable
	public ModuleObserver
{
	ReferenceCache::ResourcePtr _resource;
	scene::INodePtr _node;
public:
	SingletonModel();
	~SingletonModel();
	
	// ModuleObserver implementation
	void realise();
	void unrealise();

	// Update the model to the provided keyvalue
	void modelChanged(const std::string& value);
	typedef MemberCaller1<SingletonModel, const std::string&, &SingletonModel::modelChanged> ModelChangedCaller;

	// Returns the reference to the "singleton" model node
	scene::INodePtr getNode() const;
};

#endif
