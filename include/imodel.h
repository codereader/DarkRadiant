#pragma once

#include "Bounded.h"
#include "irender.h"
#include "inode.h"
#include "imodule.h"
#include "imodelsurface.h"
#include <vector>

#include "math/Vector3.h"

/* Forward decls */
class AABB;
class ModelSkin;
class Matrix4;

namespace model
{

typedef std::vector<std::string> StringList;

/**
 * \brief
 * Interface for static models.
 *
 * This interface provides functions for obtaining information about a LWO or
 * ASE model, such as its bounding box or poly count. 
 */
class IModel : public Bounded
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

	/**
     * \brief
     * Return a vector of strings listing the active materials used in this
	 * model, after any skin remaps.
     *
     * The list is owned by the model instance.
	 */
	virtual const StringList& getActiveMaterials() const = 0;

	/**
     * \brief
     * Return the surface with the given index
     *
     * Retrieves the interface of a specific surface, to get access to the
     * surface's polygons and vertices.
	 *
	 * \param surfaceNum
     * The surface index, must be in [0..getSurfaceCount())
	 */
	virtual const IModelSurface& getSurface(unsigned surfaceNum) const = 0;
};

// Smart pointer typedefs
typedef std::shared_ptr<IModel> IModelPtr;
typedef std::weak_ptr<IModel> IModelWeakPtr;

/**
 * greebo: Each node in the scene that represents "just" a model,
 *         derives from this class. Use a cast on this class to
 *         identify model nodes in the scene.
 */
class ModelNode
{
public:
    virtual ~ModelNode()
	{}

	// Returns the contained IModel
	virtual const IModel& getIModel() const = 0;

	// Returns the contained IModel
	virtual IModel& getIModel() = 0;

	// Returns true if this model's scale has been modified
	// and needs to be written to file
	virtual bool hasModifiedScale() = 0;

	// Returns the current scale of this model
	virtual Vector3 getModelScale() = 0;
};
typedef std::shared_ptr<ModelNode> ModelNodePtr;

class IModelExporter;
typedef std::shared_ptr<IModelExporter> IModelExporterPtr;

/**
* Exporter Interface for models (meshes).
*/
class IModelExporter
{
public:
	virtual ~IModelExporter()
	{}

	// Virtual constructor idiom. Use this method to generate a new
	// instance of the implementing subclass. This way the model format manager
	// can create a fresh instance of this exporter on demand.
	virtual IModelExporterPtr clone() = 0;

	// The display name for referencing this exporter in the GUI
	virtual const std::string& getDisplayName() const = 0;

	// Returns the uppercase file extension this exporter is suitable for
	virtual const std::string& getExtension() const = 0;

	// Adds the given Surface to the exporter's queue
	// The given transform is applied to the surface before the vertices are added to the queue.
	// Note: Scaling components of the matrix are not treated separately here.
	virtual void addSurface(const IModelSurface& surface, const Matrix4& localToWorld) = 0;

	// Adds the given set of polygons to the named surface
	// The given transform is applied to the surface before the vertices are added to the queue.
	// Note: Scaling components of the matrix are not treated separately here.
	// Note 2: Polygons are expected to have their vertices defined counter-clockwise
	virtual void addPolygons(const std::string& materialName, 
		const std::vector<ModelPolygon>& polys, const Matrix4& localToWorld) = 0;

    // Export to the given file in the given path (which must be absolute).
    // If writing to multiple files (like .obj+.mtl), the implementation
    // may deduct the alternative file paths from this one. 
    // Note: the user will already have confirmed to overwrite any existing
    // file with that path, but hasn't explicitly confirmed overwriting 
    // secondary or tertiary output files.
    // Throws std::runtime_error in case of failures.
    virtual void exportToPath(const std::string& outputPath, const std::string& filename) = 0;
};

/**
 * Importer interface for models. An importer must be able
 * to load a model (node) from the VFS and from an absolute path.
 * The importer instance shouldn't maintain an internal state,
 * such that the same instance can be used to load several models,
 * from different client code.
 */
class IModelImporter
{
public:
	// Returns the uppercase file extension this exporter is suitable for
	virtual const std::string& getExtension() const = 0;

	/**
	* greebo: Returns a newly created model node for the given model name.
	*
	* @modelName: This is usually the value of the "model" spawnarg of entities.
	*
	* @returns: the newly created modelnode (can be NULL if the model was not found).
	*/
	virtual scene::INodePtr loadModel(const std::string& modelName) = 0;

	/**
	* Load a model from the given (maybe be VFS or absolute), and return the IModel subclass for it.
	*
	* @returns: the IModelPtr containing the renderable model or
	* an empty IModelPtr if the model loader could not load the file.
	*/
	virtual model::IModelPtr loadModelFromPath(const std::string& path) = 0;
};
typedef std::shared_ptr<IModelImporter> IModelImporterPtr;

class IModelFormatManager :
	public RegisterableModule
{
public:
	virtual ~IModelFormatManager()
	{}

	// Register/unregister an importer class
	virtual void registerImporter(const IModelImporterPtr& importer) = 0;
	virtual void unregisterImporter(const IModelImporterPtr& importer) = 0;

	// Find an importer for the given extension, returns the NullModelLoader if nothing found
	// Passing in an empty extension will return the NullModelLoader as well
	virtual IModelImporterPtr getImporter(const std::string& extension) = 0;

	// Register/unregister an exporter class
	virtual void registerExporter(const IModelExporterPtr& exporter) = 0;
	virtual void unregisterExporter(const IModelExporterPtr& exporter) = 0;

	// Find an exporter for the given extension, returns empty if nothing found
	virtual IModelExporterPtr getExporter(const std::string& extension) = 0;

	// Calls the functor with each registered model importer as argument
	virtual void foreachImporter(const std::function<void(const IModelImporterPtr&)>& functor) = 0;

	// Calls the functor with each registered model exporter as argument
	virtual void foreachExporter(const std::function<void(const IModelExporterPtr&)>& functor) = 0;
};

} // namespace model

// Utility methods
inline bool Node_isModel(const scene::INodePtr& node) 
{
	return std::dynamic_pointer_cast<model::ModelNode>(node) != nullptr;
}

inline model::ModelNodePtr Node_getModel(const scene::INodePtr& node)
{
	return std::dynamic_pointer_cast<model::ModelNode>(node);
}

// Contains the default format used for exporting scaled models
const char* const RKEY_DEFAULT_MODEL_EXPORT_FORMAT = "user/ui/map/defaultScaledModelExportFormat";

const char* const MODULE_MODELFORMATMANAGER("ModelFormatManager");

inline model::IModelFormatManager& GlobalModelFormatManager()
{
	static module::InstanceReference<model::IModelFormatManager> _reference(MODULE_MODELFORMATMANAGER);
	return _reference;
}
