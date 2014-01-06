#include "CSG.h"

#include <map>

#include "i18n.h"
#include "itextstream.h"
#include "iundo.h"
#include "igrid.h"
#include "iradiant.h"
#include "imainframe.h"
#include "iselection.h"
#include "ieventmanager.h"

#include "scenelib.h"
#include "shaderlib.h"

#include "registry/registry.h"
#include "brush/Face.h"
#include "brush/Brush.h"
#include "brush/BrushNode.h"
#include "brush/BrushVisit.h"
#include "selection/algorithm/Primitives.h"

#include "gtkutil/dialog/MessageBox.h"
#include "gtkutil/dialog/MessageBox.h"

#include "BrushByPlaneClipper.h"

namespace brush {
namespace algorithm {

const std::string RKEY_EMIT_CSG_SUBTRACT_WARNING("user/ui/brush/emitCSGSubtractWarning");

class FaceMakeBrush :
	public BrushVisitor
{
private:
	const BrushNodePtr& _brush;
	float _offset;
	bool _makeRoom;

public:

	FaceMakeBrush(const BrushNodePtr& brush, float offset, bool makeRoom = false) :
		_brush(brush),
		_offset(offset),
		_makeRoom(makeRoom)
	{}

	void visit(Face& face) const
	{
		if (!face.contributes())
		{
			return;
		}

		scene::INodePtr parent = _brush->getParent();

		scene::INodePtr newNode = GlobalBrushCreator().createBrush();
		BrushNodePtr brushNode = boost::dynamic_pointer_cast<BrushNode>(newNode);
		assert(brushNode != NULL);

		// Add the child to the same parent as the source brush
		parent->addChildNode(brushNode);

		// Move the child brushes to the same layer as their source
		brushNode->assignToLayers(_brush->getLayers());

		// Copy all faces from the source brush
		brushNode->getBrush().copy(_brush->getBrush());

		Node_setSelected(brushNode, true);

		FacePtr newFace = brushNode->getBrush().addFace(face);

		if (newFace != 0)
		{
			newFace->flipWinding();
			newFace->getPlane().offset(_offset);
			newFace->planeChanged();

			if (_makeRoom)
			{
				// Retrieve the normal vector of the "source" face
				brushNode->getBrush().transform(
					Matrix4::getTranslation(face.getPlane().getPlane().normal()*_offset)
				);
				brushNode->getBrush().freezeTransform();
			}
		}

		brushNode->getBrush().removeEmptyFaces();
	}
};

void hollowBrush(const BrushNodePtr& sourceBrush, bool makeRoom)
{
	// Hollow the brush using the current grid size
	FaceMakeBrush visitor(sourceBrush, GlobalGrid().getGridSize(), makeRoom);
	sourceBrush->getBrush().forEachFace(visitor);

	// Now unselect and remove the source brush from the scene
	scene::removeNodeFromParent(sourceBrush);
}

void hollowSelectedBrushes(const cmd::ArgumentList& args) {
	UndoableCommand undo("hollowSelectedBrushes");

	// Find all brushes
	BrushPtrVector brushes = selection::algorithm::getSelectedBrushes();

	// Cycle through the brushes and hollow them
	// We assume that all these selected brushes are visible as well.
	for (std::size_t i = 0; i < brushes.size(); ++i)
	{
		hollowBrush(brushes[i], false);
	}

	SceneChangeNotify();
}

void makeRoomForSelectedBrushes(const cmd::ArgumentList& args) {
	UndoableCommand undo("brushMakeRoom");

	// Find all brushes
	BrushPtrVector brushes = selection::algorithm::getSelectedBrushes();

	// Cycle through the brushes and hollow them
	// We assume that all these selected brushes are visible as well.
	for (std::size_t i = 0; i < brushes.size(); ++i)
	{
		hollowBrush(brushes[i], true);
	}

	SceneChangeNotify();
}

BrushSplitType Brush_classifyPlane(const Brush& brush, const Plane3& plane) {
	brush.evaluateBRep();

	BrushSplitType split;
	for (Brush::const_iterator i(brush.begin()); i != brush.end(); ++i) {
		if ((*i)->contributes()) {
			split += (*i)->getWinding().classifyPlane(plane);
		}
	}

	return split;
}

// Returns true if fragments have been inserted into the given ret_fragments list
bool Brush_subtract(const BrushNodePtr& brush, const Brush& other, BrushPtrVector& ret_fragments)
{
	if (brush->getBrush().localAABB().intersects(other.localAABB()))
	{
		BrushPtrVector fragments;
		fragments.reserve(other.getNumFaces());

		BrushNodePtr back = boost::dynamic_pointer_cast<BrushNode>(brush->clone());

		for (Brush::const_iterator i(other.begin()); i != other.end(); ++i)
		{
			const Face& face = *(*i);

			if (!face.contributes()) continue;

			BrushSplitType split = Brush_classifyPlane(back->getBrush(), face.plane3());

			if (split.counts[ePlaneFront] != 0 && split.counts[ePlaneBack] != 0)
			{
				fragments.push_back(boost::dynamic_pointer_cast<BrushNode>(back->clone()));

				FacePtr newFace = fragments.back()->getBrush().addFace(face);

				if (newFace != 0)
				{
					newFace->flipWinding();
				}

				back->getBrush().addFace(face);
			}
			else if (split.counts[ePlaneBack] == 0)
			{
				return false;
			}
		}

		ret_fragments.insert(ret_fragments.end(), fragments.begin(), fragments.end());
		return true;
	}

	return false;
}

class SubtractBrushesFromUnselected :
	public scene::NodeVisitor
{
	const BrushPtrVector& _brushlist;
	std::size_t& _before;
	std::size_t& _after;

	std::list<scene::INodePtr> _deleteList;
public:
	SubtractBrushesFromUnselected(const BrushPtrVector& brushlist, std::size_t& before, std::size_t& after) :
		_brushlist(brushlist),
		_before(before),
		_after(after)
	{}

	virtual ~SubtractBrushesFromUnselected() {
		for (std::list<scene::INodePtr>::iterator i = _deleteList.begin();
			 i != _deleteList.end(); i++)
		{
			scene::removeNodeFromParent(*i);
		}
	}

	bool pre(const scene::INodePtr& node) {
		return true;
	}

	void post(const scene::INodePtr& node) {
		if (!node->visible()) {
			return;
		}

		Brush* brush = Node_getBrush(node);

		if (brush != NULL && !Node_isSelected(node))
		{
			BrushNodePtr brushNode = boost::dynamic_pointer_cast<BrushNode>(node);

			// Get the parent of this brush
			scene::INodePtr parent = node->getParent();
			assert(parent != NULL); // parent should not be NULL

			BrushPtrVector buffer[2];
			std::size_t swap = 0;

			BrushNodePtr original = boost::dynamic_pointer_cast<BrushNode>(brushNode->clone());

			//Brush* original = new Brush(*brush);
			buffer[swap].push_back(original);

			// Iterate over all selected brushes
			for (BrushPtrVector::const_iterator i(_brushlist.begin()); i != _brushlist.end(); ++i)
			{
				for (BrushPtrVector::iterator j(buffer[swap].begin());
					 j != buffer[swap].end(); ++j)
				{
					if (Brush_subtract(*j, (*i)->getBrush(), buffer[1 - swap]))
					{
						// greebo: Delete not necessary, nodes get deleted automatically by clear() below
						// delete (*j);
					}
					else
					{
						buffer[1 - swap].push_back(*j);
					}
				}

				buffer[swap].clear();
				swap = 1 - swap;
			}

			BrushPtrVector& out = buffer[swap];

			if (out.size() == 1 && out.back() == original)
			{
				// greebo: shared_ptr is taking care of this
				//delete original;
			}
			else
			{
				_before++;

				for (BrushPtrVector::const_iterator i = out.begin(); i != out.end(); ++i)
				{
					_after++;

					scene::INodePtr newBrush = GlobalBrushCreator().createBrush();

					parent->addChildNode(newBrush);

					// Move the new Brush to the same layers as the source node
					newBrush->assignToLayers(node->getLayers());

					(*i)->getBrush().removeEmptyFaces();
					ASSERT_MESSAGE(!(*i)->getBrush().empty(), "brush left with no faces after subtract");

					Node_getBrush(newBrush)->copy((*i)->getBrush());
				}

			    _deleteList.push_back(node);
			}
		}
	}
};

void subtractBrushesFromUnselected(const cmd::ArgumentList& args)
{
	if (registry::getValue<bool>(RKEY_EMIT_CSG_SUBTRACT_WARNING))
	{
		gtkutil::MessageBox box(_("This Is Not Dromed Warning"),
			_("Note: be careful when using the CSG tool, as you might end up\n"
			"with a unnecessary number of tiny brushes and/or leaks.\n"
			"This popup will not be shown again."), ui::IDialog::MESSAGE_CONFIRM,
			GlobalMainFrame().getTopLevelWindow());

		box.run();

		// Disable this warning
        registry::setValue(RKEY_EMIT_CSG_SUBTRACT_WARNING, false);
	}

	// Collect all selected brushes
	BrushPtrVector brushes = selection::algorithm::getSelectedBrushes();

	if (brushes.empty()) {
		rMessage() << _("CSG Subtract: No brushes selected.") << std::endl;
		gtkutil::MessageBox::ShowError(_("CSG Subtract: No brushes selected."), GlobalMainFrame().getTopLevelWindow());
		return;
	}

	rMessage() << "CSG Subtract: Subtracting " << brushes.size() << " brushes.\n";

	UndoableCommand undo("brushSubtract");

	// subtract selected from unselected
	std::size_t before = 0;
	std::size_t after = 0;

	// instantiate a scoped walker class
	{
		SubtractBrushesFromUnselected walker(brushes, before, after);
		GlobalSceneGraph().root()->traverse(walker);
	}

	rMessage() << "CSG Subtract: Result: "
		<< after << " fragment" << (after == 1 ? "" : "s")
		<< " from " << before << " brush" << (before == 1 ? "" : "es") << ".\n";

	SceneChangeNotify();
}

// greebo: TODO: Make this a member method of the Brush class
bool Brush_merge(Brush& brush, const BrushPtrVector& in, bool onlyshape) {
	// gather potential outer faces
	typedef std::vector<const Face*> Faces;
	Faces faces;

	for (BrushPtrVector::const_iterator i(in.begin()); i != in.end(); ++i) {
		(*i)->getBrush().evaluateBRep();

		for (Brush::const_iterator j((*i)->getBrush().begin()); j != (*i)->getBrush().end(); ++j) {
			if (!(*j)->contributes()) {
				continue;
			}

			const Face& face1 = *(*j);

			bool skip = false;

			// test faces of all input brushes
			//!\todo SPEEDUP: Flag already-skip faces and only test brushes from i+1 upwards.
			for (BrushPtrVector::const_iterator k(in.begin()); !skip && k != in.end(); ++k) {
				if (k != i) { // don't test a brush against itself
					for (Brush::const_iterator l((*k)->getBrush().begin()); !skip && l != (*k)->getBrush().end(); ++l) {
						const Face& face2 = *(*l);

						// face opposes another face
						if (face1.plane3() == -face2.plane3()) {
							// skip opposing planes
							skip  = true;
							break;
						}
					}
				}
			}

			// check faces already stored
			for (Faces::const_iterator m = faces.begin(); !skip && m != faces.end(); ++m) {
				const Face& face2 = *(*m);

				// face equals another face
				if (face1.plane3() == face2.plane3()) {
					// if the texture/shader references should be the same but are not
					if (!onlyshape && !shader_equal(
                            face1.getFaceShader().getMaterialName(),
                            face2.getFaceShader().getMaterialName()
                        ))
                    {
						return false;
					}

					// skip duplicate planes
					skip = true;
					break;
				}

				// face1 plane intersects face2 winding or vice versa
				if (Winding::planesConcave(face1.getWinding(), face2.getWinding(), face1.plane3(), face2.plane3())) {
					// result would not be convex
					return false;
				}
			}

			if (!skip) {
				faces.push_back(&face1);
			}
		}
	}

	for (Faces::const_iterator i = faces.begin(); i != faces.end(); ++i) {
		if (!brush.addFace(*(*i))) {
			// result would have too many sides
			return false;
		}
	}

	brush.removeEmptyFaces();
	return true;
}

void mergeSelectedBrushes(const cmd::ArgumentList& args)
{
	// Get the current selection
	BrushPtrVector brushes = selection::algorithm::getSelectedBrushes();

	if (brushes.empty()) {
		rMessage() << _("CSG Merge: No brushes selected.") << std::endl;
		gtkutil::MessageBox::ShowError(_("CSG Merge: No brushes selected."), GlobalMainFrame().getTopLevelWindow());
		return;
	}

	if (brushes.size() < 2) {
		rMessage() << "CSG Merge: At least two brushes have to be selected.\n";
		gtkutil::MessageBox::ShowError("CSG Merge: At least two brushes have to be selected.", GlobalMainFrame().getTopLevelWindow());
		return;
	}

	rMessage() << "CSG Merge: Merging " << brushes.size() << " brushes." << std::endl;

	UndoableCommand undo("mergeSelectedBrushes");

	// Take the last selected node as reference for layers and parent
	scene::INodePtr merged = GlobalSelectionSystem().ultimateSelected();

	scene::INodePtr parent = merged->getParent();
	assert(parent != NULL);

	// Create a new BrushNode
	scene::INodePtr node = GlobalBrushCreator().createBrush();

	// Insert the newly created brush into the (same) parent entity
	parent->addChildNode(node);

	// Move the new brush to the same layers as the merged one
	node->assignToLayers(merged->getLayers());

	// Get the contained brush
	Brush* brush = Node_getBrush(node);

	// Attempt to merge the selected brushes into the new one
	if (!Brush_merge(*brush, brushes, true))
	{
		rWarning() << "CSG Merge: Failed - result would not be convex." << std::endl;
		return;
	}

	ASSERT_MESSAGE(!brush->empty(), "brush left with no faces after merge");

	// Remove the original brushes
	for (BrushPtrVector::iterator i = brushes.begin(); i != brushes.end(); ++i)
	{
		scene::removeNodeFromParent(*i);
	}

	// Select the new brush
	Node_setSelected(node, true);

	rMessage() << "CSG Merge: Succeeded." << std::endl;
	SceneChangeNotify();
}

class BrushSetClipPlane :
	public SelectionSystem::Visitor
{
	Plane3 _plane;
public:
	BrushSetClipPlane(const Plane3& plane) :
		_plane(plane)
	{}

	virtual ~BrushSetClipPlane() {}

	void visit(const scene::INodePtr& node) const {
		BrushNodePtr brush = boost::dynamic_pointer_cast<BrushNode>(node);

		if (brush != NULL && node->visible()) {
			brush->setClipPlane(_plane);
		}
	}
};

/**
 * greebo: Sets the "clip plane" of the selected brushes in the scene.
 */
void setBrushClipPlane(const Plane3& plane) {
	BrushSetClipPlane walker(plane);
	GlobalSelectionSystem().foreachSelected(walker);
}

/**
 * greebo: Splits the selected brushes by the given plane.
 */
void splitBrushesByPlane(const Vector3 planePoints[3], EBrushSplit split)
{
	TextureProjection projection;

	// Collect all selected brushes
	BrushPtrVector brushes = selection::algorithm::getSelectedBrushes();

	// Instantiate a scoped walker
	BrushByPlaneClipper splitter(
		planePoints[0],
		planePoints[1],
		planePoints[2],
		projection,
		split
	);

	splitter.split(brushes);

	SceneChangeNotify();
}

void registerCommands()
{
	GlobalCommandSystem().addCommand("CSGSubtract", subtractBrushesFromUnselected);
	GlobalCommandSystem().addCommand("CSGMerge", mergeSelectedBrushes);
	GlobalCommandSystem().addCommand("CSGHollow", hollowSelectedBrushes);
	GlobalCommandSystem().addCommand("CSGRoom", makeRoomForSelectedBrushes);

	GlobalEventManager().addCommand("CSGSubtract", "CSGSubtract");
	GlobalEventManager().addCommand("CSGMerge", "CSGMerge");
	GlobalEventManager().addCommand("CSGHollow", "CSGHollow");
	GlobalEventManager().addCommand("CSGRoom", "CSGRoom");
}

} // namespace algorithm
} // namespace brush
