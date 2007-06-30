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

#include "nameable.h"
#include "picomodel.h"
#include "RenderablePicoModel.h"
#include "PicoModelInstance.h"

#include "iarchive.h"
#include "idatastream.h"
#include "imodel.h"

#include "generic/static.h"
#include "shaderlib.h"
#include "instancelib.h"
#include "transformlib.h"
#include "traverselib.h"
#include "render.h"

#include "os/path.h"

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <string>

class PicoModelNode : 
	public scene::Node, 
	public scene::Instantiable,
	public Nameable
{
  InstanceSet m_instances;

	// The actual model
	model::RenderablePicoModel _picoModel;

	std::string _name;

public:
  /** Construct a PicoModelNode with the parsed picoModel_t struct and the
   * provided file extension.
   */
  PicoModelNode(picoModel_t* model, const std::string& ext) :  
	_picoModel(model, ext), // pass extension down to the PicoModel
	_name(model->fileName)
  {}

  scene::Instance* create(const scene::Path& path, scene::Instance* parent)
  {
    return new model::PicoModelInstance(path, parent, _picoModel);
  }
  void forEachInstance(const scene::Instantiable::Visitor& visitor)
  {
    m_instances.forEachInstance(visitor);
  }
  void insert(const scene::Path& path, scene::Instance* instance)
  {
    m_instances.insert(path, instance);
  }
  scene::Instance* erase(const scene::Path& path)
  {
    return m_instances.erase(path);
  }
  
  virtual std::string name() const {
  	return _name;
  }
};


size_t picoInputStreamReam(void* inputStream, unsigned char* buffer, size_t length)
{
  return reinterpret_cast<InputStream*>(inputStream)->read(buffer, length);
}

/* Use the picomodel library to load the contents of the given file 
 * and return a Node containing the model.
 */

scene::INodePtr loadPicoModel(const picoModule_t* module, ArchiveFile& file) {

	// Determine the file extension (ASE or LWO) to pass down to the PicoModel
	std::string fName = file.getName();
	boost::algorithm::to_lower(fName);
	std::string fExt = fName.substr(fName.size() - 3, 3);

	picoModel_t* model = PicoModuleLoadModelStream(module, &file.getInputStream(), picoInputStreamReam, file.size(), 0);
	scene::INodePtr modelNode(new PicoModelNode(model, fExt));
	PicoFreeModel(model);
	return modelNode;
}

/* Load the provided file as a model object and return as an IModel
 * shared pointer.
 */
 
model::IModelPtr loadIModel(const picoModule_t* module, ArchiveFile& file) {

	// Determine the file extension (ASE or LWO) to pass down to the PicoModel
	std::string fName = file.getName();
	boost::algorithm::to_lower(fName);
	std::string fExt = fName.substr(fName.size() - 3, 3);

	picoModel_t* model = PicoModuleLoadModelStream(module, &file.getInputStream(), picoInputStreamReam, file.size(), 0);
	
	model::IModelPtr modelObj(new model::RenderablePicoModel(model, fExt));
	PicoFreeModel(model);
	return modelObj;
	
} 
