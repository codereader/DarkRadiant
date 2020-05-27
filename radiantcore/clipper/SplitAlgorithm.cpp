#include "SplitAlgorithm.h"

#include "iselection.h"
#include "brush/BrushNode.h"
#include "selection/algorithm/Primitives.h"
#include "BrushByPlaneClipper.h"

namespace algorithm
{

class BrushSetClipPlane :
	public SelectionSystem::Visitor
{
	Plane3 _plane;
public:
	BrushSetClipPlane(const Plane3& plane) :
		_plane(plane)
	{}

	virtual ~BrushSetClipPlane() {}

	void visit(const scene::INodePtr& node) const override
	{
		BrushNodePtr brush = std::dynamic_pointer_cast<BrushNode>(node);

		if (brush && node->visible())
		{
			brush->setClipPlane(_plane);
		}
	}
};

void setBrushClipPlane(const Plane3& plane)
{
	BrushSetClipPlane walker(plane);
	GlobalSelectionSystem().foreachSelected(walker);
}

void splitBrushesByPlane(const Vector3 planePoints[3], EBrushSplit split)
{
	// Collect all selected brushes
	BrushPtrVector brushes = selection::algorithm::getSelectedBrushes();

	// Instantiate a scoped walker
	BrushByPlaneClipper splitter(
		planePoints[0],
		planePoints[1],
		planePoints[2],
		split
	);

	splitter.split(brushes);

	SceneChangeNotify();
}

}
