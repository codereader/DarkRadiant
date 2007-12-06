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

#if !defined(INCLUDED_IMODEL_H)
#define INCLUDED_IMODEL_H

#include "Bounded.h"
#include "irender.h"
#include "inode.h"
#include "imodule.h"

/* Forward decls */
class ArchiveFile;
class AABB;
class ModelSkin;

namespace model {

	/** 
	 * Interface for static models. This interface provides functions for 
	 * obtaining information about a LWO or ASE model, such as its bounding box 
	 * or poly count. The interface also inherits from OpenGLRenderable to allow 
	 * model instances to be used for rendering.
	 */
	 
	class IModel
	: public OpenGLRenderable,
	  public Bounded
	{
	public:
		/**
		 * greebo: The filename (without path) of this model.
		 */
		virtual std::string getFilename() const = 0;
		
		/** Apply the given skin to this model.
		 * 
		 * @param skin
		 * The ModelSkin instance to apply to this model.
		 */
		virtual void applySkin(const ModelSkin& skin) = 0;

		/** Return the number of material surfaces on this model. Each material
		 * surface consists of a set of polygons sharing the same material.
		 */
		virtual int getSurfaceCount() const = 0;
		
		/** Return the number of vertices in this model, equal to the sum of the
		 * vertex count from each surface.
		 */
		virtual int getVertexCount() const = 0;

		/** Return the number of triangles in this model, equal to the sum of the
		 * triangle count from each surface.
		 */
		virtual int getPolyCount() const = 0;
		
		/** Return a vector of strings listing the active materials used in this
		 * model, after any skin remaps. The list is owned by the model instance.
		 */
		virtual const std::vector<std::string>& getActiveMaterials() const = 0;
		
	};
	
	// Smart pointer typedef
	typedef boost::shared_ptr<model::IModel> IModelPtr;


} // namespace model

const std::string MODULE_MODELLOADER("ModelLoader"); // fileType is appended ("ModeLoaderASE")

/** Model loader module API interface.
 */
class ModelLoader :
	public RegisterableModule
{
public:
	/** Load a model from the VFS and return a scene::Node that contains
	 * it.
	 */	
	virtual scene::INodePtr loadModel(ArchiveFile& file) = 0;
	
	/** Load a model from the VFS, and return the IModel
	 * subclass for it.
	 */	 
	virtual model::IModelPtr loadModelFromPath(const std::string& path) = 0;
};

// Acquires the PatchCreator of the given type ("ASE", "NULL", "3DS", etc.)
inline ModelLoader& GlobalModelLoader(const std::string& fileType) {
	boost::shared_ptr<ModelLoader> _modelLoader(
		boost::static_pointer_cast<ModelLoader>(
			module::GlobalModuleRegistry().getModule(MODULE_MODELLOADER + fileType) // e.g. "ModuleLoaderTGA"
		)
	);
	return *_modelLoader;
}

#endif /* _IMODEL_H_ */
