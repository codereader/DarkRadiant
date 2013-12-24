#pragma once

#include <string>
#include <stdexcept>
#include <vector>
#include "iscenegraph.h"
#include "icommandsystem.h"
#include "iselection.h"
#include "ipatch.h"

class Face;
class Patch;
class Brush;
class PatchNode;
class BrushNode;
typedef boost::shared_ptr<BrushNode> BrushNodePtr;
typedef boost::shared_ptr<PatchNode> PatchNodePtr;
typedef std::vector<PatchNodePtr> PatchPtrVector;
typedef std::vector<BrushNodePtr> BrushPtrVector;
typedef std::vector<Face*> FacePtrVector;
class TextureProjection;

enum EBrushPrefab
{
	eBrushCuboid = 0,
	eBrushPrism,
	eBrushCone,
	eBrushSphere,
	eNumPrefabTypes,
};

namespace selection {

	/** greebo: This is thrown if some of the routines
	 * below fail to retrieve the requested selection.
	 */
	class InvalidSelectionException :
		public std::runtime_error
	{
	public:
		// Constructor
		InvalidSelectionException(const std::string& what):
			std::runtime_error(what)
		{}
	};

	namespace algorithm {

	/**
	 * Call the given functor for each selected face,
	 * only considering faces that were selected in component selection mode.
	 */
	void forEachSelectedFaceComponent(const std::function<void(Face&)>& functor);

	/** greebo: Returns the number of the selected face instances.
	 */
	int selectedFaceCount();

	/** greebo: Retrieves the reference to the last selected face.
	 *
	 * Throws an selection::InvalidSelectionException on failure.
	 *
	 * @returns: the Face& reference of the last element.
	 */
	Face& getLastSelectedFace();

	/** greebo: Retrieves the first selected patch.
	 *
	 * Throws an selection::InvalidSelectionException on failure.
	 *
	 * @returns: a reference to the patch.
	 */
	Patch& getLastSelectedPatch();

	/** greebo: Retrieves a list of selected Patches, populated
	 * by a scene walker.
	 *
	 * @returns: the vector with the selected patch pointers.
	 */
	PatchPtrVector getSelectedPatches();

	/** greebo: Retrieves a list of selected Brushes, populated
	 * by a scene walker.
	 *
	 * @returns: the vector with the selected brush pointers.
	 */
	BrushPtrVector getSelectedBrushes();

	/** greebo: Retrieves a list of selected Faces (those selected
	 * 			with Ctrl-Shift-LMB by default).
	 *
	 * @returns: the vector with the selected face pointers.
	 */
	FacePtrVector getSelectedFaces();

	/** greebo: Tries to create a collision model from the current
	 * 			selection. The basic check for a single selected
	 * 			func_clipmodel is done here and the CM object is created.
	 */
	void createCMFromSelection(const cmd::ArgumentList& args);

	/** greebo: Creates a coplanar patch for each selected face instance.
	 */
	void createDecalsForSelectedFaces(const cmd::ArgumentList& args);

	/**
	 * greebo: Applies the visportal/nodraw texture combo to the selected brushes.
	 */
	void makeVisportal(const cmd::ArgumentList& args);

	/**
	 * greebo: Surrounds the current model selection with a monsterclip brush.
	 */
	void surroundWithMonsterclip(const cmd::ArgumentList& args);

	/**
	 * Resizes the given brush to match the given bounding box, using the given shader
	 * to texture all the faces of the brush.
	 */
	void resizeBrushToBounds(Brush& brush, const AABB& aabb, const std::string& shader);

	/**
	 * Resizes all selected brushes to fit the given bounding box.
	 */
	void resizeBrushesToBounds(const AABB& aabb, const std::string& shader);

	/**
	 * Construct a defined brush prefab type out of the given brush.
	 */
	void constructBrushPrefab(Brush& brush, EBrushPrefab type, const AABB& bounds, 
							  std::size_t sides, const std::string& shader, 
							  const TextureProjection& projection);

	/**
	 * Constructs a prefab of the given brush type, for each selected brush.
	 * The given number of sides and the given shader are assigned to the newly designed brush.
	 */
	void constructBrushPrefabs(EBrushPrefab type, std::size_t sides, const std::string& shader);

	/**
	 * Command target for brush commands like "BrushMakePrefab 0", "BrushMakePrefab 1", etc.
	 */
	void brushMakePrefab(const cmd::ArgumentList& args);

	/**
	 * Command target for brush commands like "BrushMakeSided 4", etc.
	 */
	void brushMakeSided(const cmd::ArgumentList& args);

	/**
	 * Command target for "makeStructure"/"makeDetail"
	 */
	void brushSetDetailFlag(const cmd::ArgumentList& args);

	} // namespace algorithm
} // namespace selection
