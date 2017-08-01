#pragma once

#include "inode.h"
#include "imodel.h"
#include "imodelsurface.h"
#include <map>

namespace model
{

class ModelExporter :
	public scene::NodeVisitor
{
private:
	model::IModelExporterPtr _exporter;

public:
	ModelExporter(const model::IModelExporterPtr& exporter);

	bool pre(const scene::INodePtr& node) override;

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
	void processBrush(const scene::INodePtr& node);
	void processPatch(const scene::INodePtr& node);
};

}
