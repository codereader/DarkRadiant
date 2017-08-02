#pragma once

#include "inode.h"
#include "imodel.h"
#include "imodelsurface.h"
#include "math/AABB.h"
#include "math/Matrix4.h"
#include <map>
#include <list>

namespace model
{

class ModelExporter :
	public scene::NodeVisitor
{
private:
	model::IModelExporterPtr _exporter;

	bool _skipCaulk;
	std::string _caulkMaterial;

	bool _centerObjects;

	std::list<scene::INodePtr> _nodes;

	// The translation centering the objects
	// is identity if _centerObjects is false
	Matrix4 _centerTransform;

public:
	ModelExporter(const model::IModelExporterPtr& exporter);

	// Define whether the exporter should ignore caulked surfaces
	void setSkipCaulkMaterial(bool skipCaulk);

	// Define whether we should arrange the objects around the world origin
	void setCenterObjects(bool centerObjects);

	bool pre(const scene::INodePtr& node) override;

	// Processes the nodes previously collected in the pre() method
	void processNodes();

	/**
	 * Performs the actual export using the given exporter (which has 
	 * already accumulated all geometry to be exported).
	 * The file is written to a temporary file and renamed afterwards.
	 *
	 * throws std::runtime_error in case of failures.
	 */
	static void ExportToPath(const model::IModelExporterPtr& exporter,
		const std::string& outputPath, const std::string& filename);

private:
	AABB calculateModelBounds();

	bool isExportableMaterial(const std::string& materialName);

	void processBrush(const scene::INodePtr& node);
	void processPatch(const scene::INodePtr& node);
};

}
