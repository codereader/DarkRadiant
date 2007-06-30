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

#if !defined(INCLUDED_IREFERENCE_H)
#define INCLUDED_IREFERENCE_H

#include "inode.h"
#include "generic/constant.h"

#include <string>
#include <boost/shared_ptr.hpp>

class ModuleObserver;

class Resource
{
public:
  virtual bool load() = 0;
  virtual bool save() = 0;
  virtual void flush() = 0;
  virtual void refresh() = 0;
  virtual scene::INodePtr getNode() = 0;
  virtual void setNode(scene::INodePtr node) = 0;
  virtual void attach(ModuleObserver& observer) = 0;
  virtual void detach(ModuleObserver& observer) = 0;
  virtual void realise() = 0;
  virtual void unrealise() = 0;
};

class EntityCreator;

class ReferenceCache
{
public:
  INTEGER_CONSTANT(Version, 1);
  STRING_CONSTANT(Name, "reference");

	/* Resource pointer type */
	typedef boost::shared_ptr<Resource> ResourcePtr;
	
	/**
	 * Capture a named model resource, and return a pointer to it.
	 */
	virtual ResourcePtr capture(const std::string& path) = 0;
	
  virtual void release(const std::string& path) = 0;

  virtual void setEntityCreator(EntityCreator& entityCreator) = 0;
};

#include "modulesystem.h"

template<typename Type>
class GlobalModule;
typedef GlobalModule<ReferenceCache> GlobalReferenceModule;

template<typename Type>
class GlobalModuleRef;
typedef GlobalModuleRef<ReferenceCache> GlobalReferenceModuleRef;

inline ReferenceCache& GlobalReferenceCache()
{
  return GlobalReferenceModule::getTable();
}

#endif
