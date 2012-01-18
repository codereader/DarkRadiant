#include "ProcCompiler.h"

namespace map
{

ProcCompiler::ProcCompiler(const scene::INodePtr& root) :
	_root(root)
{}

ProcFilePtr ProcCompiler::generateProcFile()
{
	_procFile.reset(new ProcFile);

	// Load all entities into proc entities
	

	return _procFile;
}

namespace
{

class ToolDataGenerator :
	public scene::NodeVisitor
{
private:
	ProcFilePtr _procFile;

public:
	ToolDataGenerator(const ProcFilePtr& procFile) :
		_procFile(procFile)
	{}

	bool pre(const scene::INodePtr& node)
	{
		if (Node_isEntity(node))
		{
			_procFile->entities.push_back(ProcEntity());
			

			
			return true;
		}

		return true;
	}
};

}

void ProcCompiler::generateBrushData()
{
	ToolDataGenerator generator(_procFile);
	_root->traverse(generator);
}

} // namespace
