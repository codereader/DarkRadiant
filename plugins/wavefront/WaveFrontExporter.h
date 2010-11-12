#ifndef _WAVEFRONT_EXPORTER_H_
#define _WAVEFRONT_EXPORTER_H_

#include <fstream>
#include "iselection.h"
#include "ibrush.h"
#include "ipatch.h"

namespace exporter
{

class WaveFrontExporter :
	public SelectionSystem::Visitor
{
	// The stream we're writing to
	std::ofstream _output;

	std::size_t _exportedBrushes;
	std::size_t _exportedPatches;

	std::size_t _vertexCount;

public:
	WaveFrontExporter(const std::string& outputPath);

	// Start the export process
	void exportSelection();

	// SelectionSystem::Visitor implementation
	void visit(const scene::INodePtr& node) const;

private:
	void exportBrush(IBrush& brush);
	void exportPatch(IPatch& patch);
};

} // namespace exporter

#endif /* _WAVEFRONT_EXPORTER_H_ */
