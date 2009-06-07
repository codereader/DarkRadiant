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
class AABB;
class ModelSkin;

namespace model {

	/** 
	 * Interface for static models. This interface provides functions for 
	 * obtaining information about a LWO or ASE model, such as its bounding box 
	 * or poly count. The interface also inherits from OpenGLRenderable to allow 
	 * model instances to be used for rendering.
	 */
	typedef std::vector<std::string> MaterialList;
	 
	class IModel
	: public OpenGLRenderable,
	  public Bounded
	{
	public:
		/**
		 * greebo: The filename (without path) of this model.
		 */
		virtual std::string getFilename() const = 0;

		/**
		 * greebo: Returns the VFS path which can be used to load
		 *         this model from the modelcache.
		 */
		virtual std::string getModelPath() const = 0;

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
		virtual const MaterialList& getActiveMaterials() const = 0;
		
	};
	
	// Smart pointer typedefs
	typedef boost::shared_ptr<IModel> IModelPtr;
	typedef boost::weak_ptr<IModel> IModelWeakPtr;

	/**
	 * greebo: Each node in the scene that represents "just" a model,
	 *         derives from this class. Use a cast on this class to
	 *         identify model nodes in the scene.
	 */
	class ModelNode {
	public:
		// Returns the contained IModel
		virtual const IModel& getIModel() const = 0;
	};
	typedef boost::shared_ptr<ModelNode> ModelNodePtr;

} // namespace model

// Utility methods
inline bool Node_isModel(const scene::INodePtr& node) {
	return boost::dynamic_pointer_cast<model::ModelNode>(node) != NULL;
}

inline model::ModelNodePtr Node_getModel(const scene::INodePtr& node) {
	return boost::dynamic_pointer_cast<model::ModelNode>(node);
}

const std::string MODULE_MODELLOADER("ModelLoader"); // fileType is appended ("ModeLoaderASE")

/** Model loader module API interface.
 */
class ModelLoader :
	public RegisterableModule
{
public:
	/**
	 * greebo: Returns a newly created model node for the given model name.
	 * 
	 * @modelName: This is usually the value of the "model" spawnarg of entities.
	 *
	 * @returns: the newly created modelnode (can be NULL if the model was not found).
	 */
	virtual scene::INodePtr loadModel(const std::string& modelName) = 0;
	
	/** 
	 * Load a model from the VFS, and return the IModel subclass for it.
	 *
	 * @returns: the IModelPtr containing the renderable model or 
	 *           NULL if the model loader could not load the file.
	 */	 
	virtual model::IModelPtr loadModelFromPath(const std::string& path) = 0;
};
typedef boost::shared_ptr<ModelLoader> ModelLoaderPtr;

// Acquires the PatchCreator of the given type ("ASE", "NULL", "3DS", etc.)
inline ModelLoader& GlobalModelLoader(const std::string& fileType) {
	ModelLoaderPtr _modelLoader(
		boost::static_pointer_cast<ModelLoader>(
			module::GlobalModuleRegistry().getModule(MODULE_MODELLOADER + fileType) // e.g. "ModeLoaderASE"
		)
	);
	return *_modelLoader;
}

#endif /* _IMODEL_H_ */
