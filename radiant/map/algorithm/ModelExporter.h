#pragma once

#include "inode.h"
#include <map>
#include "PolygonBuffer.h"

namespace map
{

class ModelExporter :
	public scene::NodeVisitor
{
private:
	// All surfaces grouped by Material
	typedef std::map<MaterialPtr, model::PolygonBuffer::Ptr> Surfaces;
	Surfaces _surfaces;

public:
	ModelExporter();

	bool pre(const scene::INodePtr& node) override;
	void post(const scene::INodePtr& node) override;
};

}
