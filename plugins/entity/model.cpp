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

#include "model.h"
#include <boost/algorithm/string/replace.hpp>

SingletonModel::SingletonModel(scene::Traversable& traversable) : 
	_resource(GlobalReferenceCache().capture("")),
	_traversable(traversable)
{
	// Attach this class as ModuleObserver to the Resource
	_resource->attach(*this);
}

SingletonModel::~SingletonModel() {
	_resource->detach(*this);
}

scene::INodePtr SingletonModel::getNode() const {
	// Returns the reference to the "master" model node
	return _node;
}

void SingletonModel::realise() {
	if (_resource->load()) {
		// The model could be loaded, insert into Traversable
		_node = _resource->getNode();
		
		// Don't realise empty model paths
		if (_node != NULL && !_modelPath.empty()) {
			// Add the master model node to the attached Traversable
			_traversable.insert(_node);
		}
	}
}

void SingletonModel::unrealise() {
	if (_node != NULL && !_modelPath.empty()) {
		// Remove the master model node from the attached Traversable
		_traversable.erase(_node);
		
		// Nullify the pointer
		_node = scene::INodePtr();
	}
}

// Update the contained model from the provided keyvalues
void SingletonModel::modelChanged(const std::string& value) {
	// Release the old model
	_resource->detach(*this);
	
	// Now store the new modelpath
    // Sanitise the keyvalue - must use forward slashes
	_modelPath = boost::algorithm::replace_all_copy(value, "\\", "/");
    
    _resource = GlobalReferenceCache().capture(_modelPath);
    _resource->attach(*this);  
}
