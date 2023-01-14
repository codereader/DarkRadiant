#include "CSG.h"

#include <map>

#include "i18n.h"
#include "itextstream.h"
#include "iundo.h"
#include "igrid.h"
#include "iselection.h"
#include "ientity.h"

#include "scenelib.h"
#include "shaderlib.h"
#include "selectionlib.h"

#include "registry/registry.h"
#include "brush/Face.h"
#include "brush/Brush.h"
#include "brush/BrushNode.h"
#include "brush/BrushVisit.h"
#include "selection/algorithm/Primitives.h"
#include "messages/NotificationMessage.h"
#include "command/ExecutionNotPossible.h"

namespace brush
{

namespace algorithm
{

const std::string RKEY_EMIT_CSG_SUBTRACT_WARNING("user/ui/brush/emitCSGSubtractWarning");

void hollowBrush(const BrushNodePtr& sourceBrush, bool makeRoom)
{
	// Hollow the brush using the current grid size
    sourceBrush->getBrush().forEachFace([&] (Face& face)
    {
        if (!face.contributes())
        {
            return;
        }

        scene::INodePtr parent = sourceBrush->getParent();

        scene::INodePtr newNode = GlobalBrushCreator().createBrush();
        BrushNodePtr brushNode = std::dynamic_pointer_cast<BrushNode>(newNode);
        assert(brushNode);

        float offset = GlobalGrid().getGridSize();

		if (makeRoom)
		{
			face.getPlane().offset(offset);
		}

        // Add the child to the same parent as the source brush
        parent->addChildNode(brushNode);

        // Move the child brushes to the same layer as their source
        brushNode->assignToLayers(sourceBrush->getLayers());

        // Copy all faces from the source brush
        brushNode->getBrush().copy(sourceBrush->getBrush());

		if (makeRoom)
		{
			face.getPlane().offset(-offset);
		}

        Node_setSelected(brushNode, true);

        FacePtr newFace = brushNode->getBrush().addFace(face);

        if (newFace != 0)
        {
            newFace->flipWinding();

			if (!makeRoom)
			{
	            newFace->getPlane().offset(offset);
			}

            newFace->planeChanged();
        }

        brushNode->getBrush().removeEmptyFaces();
    });

	// Now unselect and remove the source brush from the scene
	scene::removeNodeFromParent(sourceBrush);
}

void hollowSelectedBrushes(const cmd::ArgumentList& args) {
	UndoableCommand undo("hollowSelectedBrushes");

	// Find all brushes
	BrushPtrVector brushes = selection::algorithm::getSelectedBrushes();

	// Cycle through the brushes and hollow them
	// We assume that all these selected brushes are visible as well.
	for (const BrushNodePtr& brush : brushes)
	{
		hollowBrush(brush, false);
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

// Returns true if fragments have been inserted into the given ret_fragments list
bool Brush_subtract(const BrushNodePtr& brush, const Brush& other, BrushPtrVector& ret_fragments)
{
	if (brush->getBrush().localAABB().intersects(other.localAABB()))
	{
		BrushPtrVector fragments;
		fragments.reserve(other.getNumFaces());

		BrushNodePtr back = std::dynamic_pointer_cast<BrushNode>(brush->clone());

		for (Brush::const_iterator i(other.begin()); i != other.end(); ++i)
		{
			const Face& face = *(*i);

			if (!face.contributes()) continue;

			BrushSplitType split = back->getBrush().classifyPlane(face.plane3());

			if (split.counts[ePlaneFront] != 0 && split.counts[ePlaneBack] != 0)
			{
				fragments.push_back(std::dynamic_pointer_cast<BrushNode>(back->clone()));

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

	BrushPtrVector _unselectedBrushes;

public:
	SubtractBrushesFromUnselected(const BrushPtrVector& brushlist, std::size_t& before, std::size_t& after) :
		_brushlist(brushlist),
		_before(before),
		_after(after)
	{}

	bool pre(const scene::INodePtr& node) override
	{
		if (!node->visible())
		{
			return false;
		}

		if (Node_isBrush(node) && !Node_isSelected(node))
		{
			_unselectedBrushes.emplace_back(std::dynamic_pointer_cast<BrushNode>(node));
		}

		return true;
	}

	void processUnselectedBrushes()
	{
		for (const auto& node : _unselectedBrushes)
		{
			processNode(node);
		}
	}

private:
	void processNode(const BrushNodePtr& brushNode)
	{
		// Get the parent of this brush
		scene::INodePtr parent = brushNode->getParent();
		assert(parent); // parent must not be NULL

		BrushPtrVector buffer[2];
		std::size_t swap = 0;

		BrushNodePtr original = std::dynamic_pointer_cast<BrushNode>(brushNode->clone());

		//Brush* original = new Brush(*brush);
		buffer[swap].push_back(original);

		// Iterate over all selected brushes
		for (const auto& selectedBrush : _brushlist)
		{
			for (const auto& target : buffer[swap])
			{
				if (Brush_subtract(target, selectedBrush->getBrush(), buffer[1 - swap]))
				{
					// greebo: Delete not necessary, nodes get deleted automatically by clear() below
					// delete (*j);
				}
				else
				{
					buffer[1 - swap].push_back(target);
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
				newBrush->assignToLayers(brushNode->getLayers());

				(*i)->getBrush().removeEmptyFaces();
				ASSERT_MESSAGE(!(*i)->getBrush().empty(), "brush left with no faces after subtract");

				Node_getBrush(newBrush)->copy((*i)->getBrush());
			}

			scene::removeNodeFromParent(brushNode);
		}
	}
};

void subtractBrushesFromUnselected(const cmd::ArgumentList& args)
{
	if (registry::getValue<bool>(RKEY_EMIT_CSG_SUBTRACT_WARNING))
	{
		radiant::NotificationMessage::SendInformation(
			_("Note: be careful when using the CSG tool, as you might end up\n"
			"with an unnecessary number of tiny brushes and/or leaks.\n"
			"This popup will not be shown again."), 
			_("This Is Not Dromed Warning"));

		// Disable this warning
        registry::setValue(RKEY_EMIT_CSG_SUBTRACT_WARNING, false);
	}

	// Collect all selected brushes
	BrushPtrVector brushes = selection::algorithm::getSelectedBrushes();

	if (brushes.empty())
	{
		throw cmd::ExecutionNotPossible(_("CSG Subtract: No brushes selected."));
	}

	rMessage() << "CSG Subtract: Subtracting " << brushes.size() << " brushes.\n";

	UndoableCommand undo("brushSubtract");

	// subtract selected from unselected
	std::size_t before = 0;
	std::size_t after = 0;

	SubtractBrushesFromUnselected walker(brushes, before, after);
	GlobalSceneGraph().root()->traverse(walker);

	walker.processUnselectedBrushes();

	rMessage() << "CSG Subtract: Result: "
		<< after << " fragment" << (after == 1 ? "" : "s")
		<< " from " << before << " brush" << (before == 1 ? "" : "es") << ".\n";

	SceneChangeNotify();
}

// greebo: TODO: Make this a member method of the Brush class
bool Brush_merge(Brush& brush, const BrushPtrVector& in, bool onlyshape) {
	// gather potential outer faces
	typedef std::vector<const Face*> FaceList;
	FaceList faces;

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
			for (FaceList::const_iterator m = faces.begin(); !skip && m != faces.end(); ++m) {
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

	for (FaceList::const_iterator i = faces.begin(); i != faces.end(); ++i) {
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
	auto brushes = selection::algorithm::getSelectedBrushes();

	if (brushes.empty())
	{
		throw cmd::ExecutionNotPossible(_("CSG Merge: No brushes selected."));
	}

	// Group the brushes by their parents
	std::map<scene::INodePtr, BrushPtrVector> brushesByEntity;

	for (const auto& brushNode : brushes)
	{
		auto parent = brushNode->getParent();

		if (brushesByEntity.find(parent) == brushesByEntity.end())
		{
			brushesByEntity[parent] = BrushPtrVector();
		}

		brushesByEntity[parent].emplace_back(brushNode);
	}

	bool selectionIsSuitable = false;
	// At least one group should have more than two members
	for (const auto& pair : brushesByEntity)
	{
		if (pair.second.size() >= 2)
		{
			selectionIsSuitable = true;
			break;
		}
	}

	if (!selectionIsSuitable)
	{
		throw cmd::ExecutionNotPossible(_("CSG Merge: At least two brushes sharing of the same entity have to be selected."));
	}

	UndoableCommand undo("mergeSelectedBrushes");

	bool anythingMerged = false;
	for (const auto& pair : brushesByEntity)
	{
		if (pair.second.size() < 2)
		{
			continue;
		}

		// Take the last selected node as reference for layers and parent
		auto lastBrush = pair.second.back();
		auto parent = lastBrush->getParent();

		assert(Node_isEntity(parent));

		// Create a new BrushNode
		auto newBrush = GlobalBrushCreator().createBrush();

		// Insert the newly created brush into the same parent entity
		parent->addChildNode(newBrush);

		// Move the new brush to the same layers as the merged one
		newBrush->assignToLayers(lastBrush->getLayers());

		// Get the contained brush
		Brush* brush = Node_getBrush(newBrush);

		// Attempt to merge the selected brushes into the new one
		if (!Brush_merge(*brush, pair.second, true))
		{
			continue;
		}

		anythingMerged = true;

		ASSERT_MESSAGE(!brush->empty(), "brush left with no faces after merge");

		// Remove the original brushes
		for (const auto& brush : pair.second)
		{
			scene::removeNodeFromParent(brush);
		}

		// Select the new brush
		Node_setSelected(newBrush, true);
	}

	if (!anythingMerged)
	{
		throw cmd::ExecutionFailure(_("CSG Merge: Failed - result would not be convex"));
	}

	rMessage() << "CSG Merge: Succeeded." << std::endl;
	SceneChangeNotify();
}

void registerCommands()
{
    using selection::pred::haveBrush;

    GlobalCommandSystem().addWithCheck("CSGSubtract", subtractBrushesFromUnselected,
                                       haveBrush);
    GlobalCommandSystem().addWithCheck("CSGMerge", mergeSelectedBrushes, haveBrush);
    GlobalCommandSystem().addWithCheck("CSGHollow", hollowSelectedBrushes, haveBrush);
    GlobalCommandSystem().addWithCheck("CSGRoom", makeRoomForSelectedBrushes, haveBrush);
}

} // namespace algorithm
} // namespace brush
