#pragma once

#include "inode.h"
#include "ientity.h"
#include "ProcFile.h"
#include "math/Vector3.h"

namespace map
{

/** 
 * Processor class that takes a root node (representing a map)
 * to generate a ProcFile containing the model and shadow data.
 */
class ProcCompiler
{
private:
	// The map node we're processing
	scene::INodePtr _root;

	// The working copy
	ProcFilePtr _procFile;

public:
	ProcCompiler(const scene::INodePtr& root);

	// Generate the .proc file
	ProcFilePtr generateProcFile();

private:
	void generateBrushData();
};

} // namespace
