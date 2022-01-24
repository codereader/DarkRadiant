#pragma once

#include <vector>

#include "transformlib.h"
#include "editable.h"
#include "iundo.h"
#include "irender.h"
#include "SurfaceShader.h"

#include "PatchConstants.h"
#include "PatchControl.h"
#include "PatchTesselation.h"
#include "PatchRenderables.h"
#include "brush/FacePlane.h"
#include "brush/Face.h"
#include <sigc++/signal.h>

class PatchNode;
class Ray;

/* greebo: The patch class itself, represented by control vertices. The basic rendering of the patch
 * is handled here (unselected control points, tesselation lines, shader).
 *
 * This class also provides functions to export/import itself to XML.
 */
// parametric surface defined by quadratic bezier control curves
class Patch :
	public IPatch,
	public Bounded,
	public Snappable,
	public IUndoable
{
    friend class PatchNode;
	PatchNode& _node;

	typedef std::set<IPatch::Observer*> Observers;
	Observers _observers;

	AABB _localAABB; // local bbox

	// Patch dimensions
	std::size_t _width;
	std::size_t _height;

	IUndoStateSaver* _undoStateSaver;

	// dynamically allocated array of control points, size is _width*_height
	PatchControlArray _ctrl;			// the true control array
	PatchControlArray _ctrlTransformed;	// a temporary control array used during transformations, so that the
										// changes can be reverted and overwritten by <_ctrl>

	// The tesselation for this patch
	PatchTesselation _mesh;

	bool _transformChanged;

	// TRUE if the patch tesselation needs an update
	bool _tesselationChanged;

	// The rendersystem we're attached to, to acquire materials
	RenderSystemWeakPtr _renderSystem;

    // Shader container, taking care of use count
    SurfaceShader _shader;

	// If true, this patch is using fixed subdivisions
	bool _patchDef3;

	// Fixed subdivision layout of this patch
	Subdivisions _subDivisions;

	// greebo: Initialises the patch member variables
	void construct();

public:
	// Constructor
	Patch(PatchNode& node);

	// Copy constructors (create this patch from another patch)
	Patch(const Patch& other, PatchNode& node);

	~Patch();

	PatchNode& getPatchNode();

	void attachObserver(Observer* observer) override;
	void detachObserver(Observer* observer) override;

	void connectUndoSystem(IUndoSystem& undoSystem);
    void disconnectUndoSystem(IUndoSystem& undoSystem);

	const AABB& localAABB() const override;

    RenderSystemPtr getRenderSystem() const;
	void setRenderSystem(const RenderSystemPtr& renderSystem);

	// Implementation of the abstract method of SelectionTestable
	// Called to test if the patch can be selected by the mouse pointer
	void testSelect(Selector& selector, SelectionTest& test);

	// Transform this patch as defined by the transformation matrix <matrix>
	void transform(const Matrix4& matrix);

	// Called by the PatchNode if the transformation gets changed
	void transformChanged();

	// Called to evaluate the transform
	void evaluateTransform();

	// Revert the changes, fall back to the saved state in <m_ctrl>
	void revertTransform() override;
	// Apply the transformed control array, save it into <m_ctrl> and overwrite the old values
	void freezeTransform() override;

	// callback for changed control points
	void controlPointsChanged() override;

	// Check if the patch has invalid control points or width/height are zero
	bool isValid() const override;

	// Check whether all control vertices are in the same 3D spot (with minimal tolerance)
	bool isDegenerate() const override;

	// Snaps the control points to the grid
	void snapto(float snap) override;

	// Gets the shader name or sets the shader to <name>
	const std::string& getShader() const override;
	void setShader(const std::string& name) override;

    const SurfaceShader& getSurfaceShader() const;
    SurfaceShader& getSurfaceShader();

	// greebo: returns true if the patch's shader is visible, false otherwise
	bool hasVisibleMaterial() const override;

    float getTextureAspectRatio() const override;

	// As the name states: get the shader flags of the m_state shader
	int getShaderFlags() const;

	// Const and non-const iterators
	PatchControlIter begin() {
		return _ctrl.begin();
	}

	PatchControlConstIter begin() const {
		return _ctrl.begin();
	}

	PatchControlIter end() {
		return _ctrl.end();
	}

	PatchControlConstIter end() const {
		return _ctrl.end();
	}

	PatchTesselation& getTesselation();

	PatchRenderIndices getRenderIndices() const override;

	// Returns a copy of the tesselated geometry
	PatchMesh getTesselatedPatchMesh() const override;

	// Get the current control point array
	PatchControlArray& getControlPoints();
	const PatchControlArray& getControlPoints() const;
	// Get the (temporary) transformed control point array, not the saved ones
	PatchControlArray& getControlPointsTransformed();
	const PatchControlArray& getControlPointsTransformed() const;

	// Set the dimensions of this patch to width <w>, height <h>
	void setDims(std::size_t w, std::size_t h) override;

	// Get the patch dimensions
	std::size_t getWidth() const override;
	std::size_t getHeight() const override;

	// Return a defined patch control vertex at <row>,<col>
	PatchControl& ctrlAt(std::size_t row, std::size_t col) override;
	// The same as above just for const
	const PatchControl& ctrlAt(std::size_t row, std::size_t col) const override;

    PatchControl& getTransformedCtrlAt(std::size_t row, std::size_t col) override;

 	/** greebo: Inserts two columns before and after the column with index <colIndex>.
 	 * 			Throws an GenericPatchException if an error occurs.
 	 */
 	void insertColumns(std::size_t colIndex) override;

 	/** greebo: Inserts two rows before and after the row with index <rowIndex>.
 	 * 			Throws an GenericPatchException if an error occurs.
 	 */
 	void insertRows(std::size_t rowIndex) override;

 	/** greebo: Removes columns or rows right before and after the col/row
 	 * 			with the given index, reducing the according dimension by 2.
 	 */
 	void removePoints(bool columns, std::size_t index) override;

 	/** greebo: Appends two rows or columns at the beginning or the end.
 	 */
 	void appendPoints(bool columns, bool beginning) override;

	void ConstructPrefab(const AABB& aabb, EPatchPrefab eType, EViewType viewType, std::size_t width = 3, std::size_t height = 3);
	void constructPlane(const AABB& aabb, int axis, std::size_t width, std::size_t height);
	void constructBevel(const AABB& aabb, EViewType viewType);
	void constructEndcap(const AABB& aabb, EViewType viewType);
	void invertMatrix() override;

	void transposeMatrix() override;
	void Redisperse(EMatrixMajor mt);
	void redisperseRows() override;
	void redisperseColumns() override;
	void insertRemove(bool insert, bool column, bool first) override;
  	Patch* MakeCap(Patch* patch, patch::CapType eType, EMatrixMajor mt, bool bFirst);
	void ConstructSeam(patch::CapType eType, Vector3* p, std::size_t width);

	void flipTexture(int axis) override;

	/** greebo: This translates the texture as much towards
	 * 			the origin as possible. The patch appearance stays unchanged.
	 */
	void normaliseTexture() override;

	/** greebo: Translate all control vertices in texture space
	 * with the given translation vector (helper method, no undoSave() call)
	 */
	void translateTexCoords(const Vector2& translation);

	// This is the same as above, but with undoSave() for use in command sequences
	void translateTexture(float s, float t) override;
	void scaleTexture(float s, float t) override;
	void rotateTexture(float angle) override; // angle in degrees
	void fitTexture(float repeatS, float repeatT) override; // call with s=1 t=1 for FIT

	/* uses longest parallel chord to calculate texture coords for each row/col
	 * greebo: The default texture scale is used when applying "Natural" texturing.
	 * The applied texture is never stretched more than the default
	 * texture scale. So if the default texture scale is 0.5, the texture coordinates
	 * are set such that the scaling never exceeds this value.
	 */
	void scaleTextureNaturally() override;

	// Aligns the patch texture along the given side/border - if possible
	void alignTexture(AlignEdge type) override;

	/* greebo: This basically projects all the patch vertices into the brush plane and
	 * transforms the projected coordinates into the texture plane space */
	void pasteTextureProjected(const Face* face);

	// This returns the PatchControl pointer that is closest to the given <point>
	PatchControlIter getClosestPatchControlToPoint(const Vector3& point);
	PatchControlIter getClosestPatchControlToFace(const Face* face);
	PatchControlIter getClosestPatchControlToPatch(const Patch& patch);

	// Returns the w,h coordinates within the PatchControlArray of the given <control>
	Vector2 getPatchControlArrayIndices(const PatchControlIter& control);

	/* This takes the texture from the given brush face and applies it to this patch.
	 * It determines the closest control vertex of this patch to the brush and
	 * tries to continue the texture seamlessly. The resulting texturing is undistorted.
     * Might throw a cmd::ExecutionFailure if the patch is not suitable.
     */
	void pasteTextureNatural(const Face* face);

	/** greebo: Pastes the texture from the given sourcepatch
	 * trying to make the transition seamless.
	 */
	void pasteTextureNatural(Patch& sourcePatch);

	void pasteTextureCoordinates(const Patch* otherPatch);

	/** greebo: Tries to make the texture transition seamless (from
	 * the source patch to the this patch), leaving the sourcepatch
	 * intact.
	 */
	void stitchTextureFrom(Patch& sourcePatch);

	/** greebo: Converts this patch as thickened counterpart of the given <sourcePatch>
	 * with the given <thickness> along the chosen <axis>
	 *
	 * @axis: 0 = x-axis, 1 = y-axis, 2 = z-axis, 3 = vertex normals
	 */
	void createThickenedOpposite(const Patch& sourcePatch, const float thickness, const int axis);

	/** greebo: This creates on of the "wall" patches when thickening patches.
	 *
	 * @sourcePath, targetPatch: these are the top and bottom patches. The wall connects
	 * 				a distinct edge of these two, depending on the wallIndex.
	 *
	 * @wallIndex: 0..3 (cycle through them to create all four walls).
	 */
	void createThickenedWall(const Patch& sourcePatch, const Patch& targetPatch, const int wallIndex);

	// called just before an action to save the undo state
	void undoSave() override;

	// Save the current patch state into a new UndoMemento instance (allocated on heap) and return it to the undo observer
	IUndoMementoPtr exportState() const override;

	// Revert the state of this patch to the one that has been saved in the UndoMemento
	void importState(const IUndoMementoPtr& state) override;

	/** greebo: Gets whether this patch is a patchDef3 (fixed tesselation)
	 */
	bool subdivisionsFixed() const override;

	/** greebo: Returns the x,y subdivision values (for tesselation)
	 */
	const Subdivisions& getSubdivisions() const override;

	/** greebo: Sets the subdivision of this patch
	 *
	 * @isFixed: TRUE, if this patch should be a patchDef3 (fixed tesselation)
	 * @divisions: a two-component vector containing the desired subdivisions
	 */
	void setFixedSubdivisions(bool isFixed, const Subdivisions& divisions) override;

	// Calculate the intersection of the given ray with the full patch mesh,
	// returns true on intersection and fills in the out variable
	bool getIntersection(const Ray& ray, Vector3& intersection);

	// Static signal holder, signal is emitted after any patch texture has changed
	static sigc::signal<void>& signal_patchTextureChanged();

    void updateTesselation(bool force = false) override;
    void queueTesselationUpdate();

private:
	// This notifies the surfaceinspector/patchinspector about the texture change
	void textureChanged();

	// greebo: checks, if the shader name is valid
	void check_shader();

	void updateAABB();
};
