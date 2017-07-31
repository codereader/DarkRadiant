#pragma once

#include "inode.h"
#include "imodel.h"
#include "imodelsurface.h"
#include <map>
#include <ostream>

namespace map
{

class ModelExporter :
	public scene::NodeVisitor
{
private:
	model::IModelExporterPtr _exporter;

public:
	ModelExporter(const model::IModelExporterPtr& exporter);

	bool pre(const scene::INodePtr& node) override;
	void post(const scene::INodePtr& node) override;

	void writeToStream(std::ostream& stream);

private:
	void processBrush(const scene::INodePtr& node);
	void processPatch(const scene::INodePtr& node);
};

}
