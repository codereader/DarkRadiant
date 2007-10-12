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

#include "imodule.h"
#include "iscenegraph.h"
#include "irender.h"
#include "iselection.h"
#include "iimage.h"
#include "imodel.h"
#include "igl.h"
#include "ifilesystem.h"
#include "iundo.h"
#include "ifiletypes.h"
#include "stream/textstream.h"

#include <iostream>

#include "md5.h"

class MD5ModelLoader : 
	public ModelLoader
{
public:
	scene::INodePtr loadModel(ArchiveFile& file) {
		return loadMD5Model(file);
	}
  
	// Not implemented
	model::IModelPtr loadModelFromPath(const std::string& name) {
		return model::IModelPtr();
	}
	
	// RegisterableModule implementation
	virtual const std::string& getName() const {
		static std::string _name("ModelLoaderMD5MESH");
		return _name;
	}
	
	virtual const StringSet& getDependencies() const {
		static StringSet _dependencies;

		if (_dependencies.empty()) {
			_dependencies.insert(MODULE_VIRTUALFILESYSTEM);
			_dependencies.insert(MODULE_OPENGL);
			_dependencies.insert(MODULE_UNDOSYSTEM);
			_dependencies.insert(MODULE_SCENEGRAPH);
			_dependencies.insert(MODULE_SHADERCACHE);
			_dependencies.insert(MODULE_SELECTIONSYSTEM);
			_dependencies.insert(MODULE_FILETYPES);
		}

		return _dependencies;
	}
	
	virtual void initialiseModule(const ApplicationContext& ctx) {
		globalOutputStream() << "MD5Model::initialiseModule called.\n";
		
		GlobalFiletypes().addType(
			"model", getName(),
			FileTypePattern("md5 meshes", "*.md5mesh")
		);
	}
};
typedef boost::shared_ptr<MD5ModelLoader> MD5ModelLoaderPtr;

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry) {
	registry.registerModule(MD5ModelLoaderPtr(new MD5ModelLoader));
	
	// Initialise the streams
	const ApplicationContext& ctx = registry.getApplicationContext();
	GlobalOutputStream::instance().setOutputStream(ctx.getOutputStream());
	GlobalErrorStream::instance().setOutputStream(ctx.getErrorStream());
	
	// Remember the reference to the ModuleRegistry
	module::RegistryReference::Instance().setRegistry(registry);
}
