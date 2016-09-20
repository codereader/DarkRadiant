#pragma once

#include <ostream>
#include <sstream>
#include "inode.h"
#include <map>

namespace map
{

class InfoFileExporter
{
private:
	// The stream we're writing to
	std::ostream& _stream;

public:
	// The constructor prepares the output stream
	InfoFileExporter(std::ostream& stream);

	// Cleans up the scene on destruction
	~InfoFileExporter();

	// Is called by the owning MapExporter
	// Requirements: node must not be NULL and not a model/particle node.
	void visitEntity(const scene::INodePtr& node, std::size_t entityNum);
	void visitPrimitive(const scene::INodePtr& node, std::size_t entityNum, std::size_t primitiveNum);
};
typedef std::shared_ptr<InfoFileExporter> InfoFileExporterPtr;

} // namespace
