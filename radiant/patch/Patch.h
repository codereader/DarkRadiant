#pragma once

#include <vector>

#include "generic/callback.h"
#include "transformlib.h"
#include "editable.h"
#include "iundo.h"
#include "irender.h"
#include "mapfile.h"

#include "PatchConstants.h"
#include "PatchControl.h"
#include "PatchTesselation.h"
#include "PatchRenderables.h"
#include "brush/TexDef.h"
#include "brush/FacePlane.h"
#include "brush/Face.h"

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
private:
	PatchNode& _node;

	std::size_t _instanceCounter;

	typedef std::set<IPatch::Observer*> Observers;
	Observers _observers;

	AABB m_aabb_local; // local bbox

	// greebo: The name of the shader
	std::string m_shader;

	ShaderPtr _shader;

	// Patch dimensions
	std::size_t m_width;
	std::size_t m_height;

public:
	bool m_patchDef3;
	// The number of subdivisions of this patch
	std::size_t m_subdivisions_x;
	std::size_t m_subdivisions_y;

private:

	IUndoStateSaver* _undoStateSaver;

  	// The pointer to the map file
	MapFile* m_map;

	// dynamically allocated array of control points, size is m_width*m_height
	PatchControlArray m_ctrl;				// the true control array
	PatchControlArray m_ctrlTransformed;	// a temporary control array used during transformations, so that the
											// changes can be reverted and overwritten by <m_ctrl>

	// The tesselation for this patch
	PatchTesselation m_tess;

	// The OpenGL renderables for three rendering modes
	RenderablePatchSolid m_render_solid;
	RenderablePatchWireframe m_render_wireframe;
	RenderablePatchFixedWireframe m_render_wireframe_fixed;

	// The shader states for the control points and the lattice
	ShaderPtr _pointShader;
	ShaderPtr _latticeShader;

	// greebo: The vertex list of the control points, can be passed to the RenderableVertexBuffer
    std::vector<VertexCb> m_ctrl_vertices;
	// The renderable of the control points
	RenderableVertexBuffer _renderableCtrlPoints;

	// The lattice indices and their renderable
	IndexBuffer m_lattice_indices;
	RenderableIndexBuffer _renderableLattice;

	bool m_bOverlay;

	bool m_transformChanged;

	// TRUE if the patch tesselation needs an update
	bool _tesselationChanged;

	// Callback functions when the patch gets changed
	Callback m_evaluateTransform;
	Callback m_boundsChanged;

	// The rendersystem we're attached to, to acquire materials
	RenderSystemWeakPtr _renderSystem;

	// greebo: Initialises the patch member variables
	void construct();

public:
	static int m_CycleCapIndex;// = 0;

	// Constructor
	Patch(PatchNode& node, const Callback& evaluateTransform, const Callback& boundsChanged);

	// Copy constructors (create this patch from another patch)
	Patch(const Patch& other, PatchNode& node, const Callback& evaluateTransform, const Callback& boundsChanged);

	~Patch();

	PatchNode& getPatchNode();

	void attachObserver(Observer* observer);
	void detachObserver(Observer* observer);

	void instanceAttach(MapFile* map);
	// Remove the attached instance and decrease the counters
	void instanceDetach(MapFile* map);

	// Allocate callback: pass the allocate call to all the observers
	void onAllocate(std::size_t size);

	// For the TransformNode implementation (localToParent() is abstract and needs to be here), returns identity
	const Matrix4& localToParent() const;

	// Return the interally stored AABB
	const AABB& localAABB() const;

	// Render functions: solid mode, wireframe mode and components
	void render_solid(RenderableCollector& collector, const VolumeTest& volume, 
					  const Matrix4& localToWorld, const IRenderEntity& entity) const;
	void render_wireframe(RenderableCollector& collector, const VolumeTest& volume, const Matrix4& localToWorld) const;

    /// Submit renderable edge and face points
	void submitRenderablePoints(RenderableCollector& collector,
                                const VolumeTest& volume,
                                const Matrix4& localToWorld) const;

	void setRenderSystem(const RenderSystemPtr& renderSystem);

	const ShaderPtr& getState() const;

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
	void revertTransform();
	// Apply the transformed control array, save it into <m_ctrl> and overwrite the old values
	void freezeTransform();

	// callback for changed control points
	void controlPointsChanged();

	// Check if the patch has invalid control points or width/height are zero
	bool isValid() const;

	// Check whether all control vertices are in the same 3D spot (with minimal tolerance)
	bool isDegenerate() const;

	// Snaps the control points to the grid
	void snapto(float snap);

	void RenderDebug(RenderStateFlags state) const;
	// Renders the normals (indicated by lines) of this patch
	void RenderNormals(RenderStateFlags state) const;

	// Gets the shader name or sets the shader to <name>
	const std::string& getShader() const;
	void setShader(const std::string& name);

	// greebo: returns true if the patch's shader is visible, false otherwise
	bool hasVisibleMaterial() const;

	// As the name states: get the shader flags of the m_state shader
	int getShaderFlags() const;

	// Const and non-const iterators
	PatchControlIter begin() {
		return m_ctrl.begin();
	}

	PatchControlConstIter begin() const {
		return m_ctrl.begin();
	}

	PatchControlIter end() {
		return m_ctrl.end();
	}

	PatchControlConstIter end() const {
		return m_ctrl.end();
	}

	PatchTesselation& getTesselation();

	// Returns a copy of the tesselated geometry
	PatchMesh getTesselatedPatchMesh() const;

	// Get the current control point array
	PatchControlArray& getControlPoints();
	const PatchControlArray& getControlPoints() const;
	// Get the (temporary) transformed control point array, not the saved ones
	PatchControlArray& getControlPointsTransformed();
	const PatchControlArray& getControlPointsTransformed() const;

	// Set the dimensions of this patch to width <w>, height <h>
	void setDims(std::size_t w, std::size_t h);

	// Get the patch dimensions
	std::size_t getWidth() const;
	std::size_t getHeight() const;

	// Return a defined patch control vertex at <row>,<col>
	PatchControl& ctrlAt(std::size_t row, std::size_t col);
	// The same as above just for const
	const PatchControl& ctrlAt(std::size_t row, std::size_t col) const;

 	/** greebo: Inserts two columns before and after the column with index <colIndex>.
 	 * 			Throws an GenericPatchException if an error occurs.
 	 */
 	void insertColumns(std::size_t colIndex);

 	/** greebo: Inserts two rows before and after the row with index <rowIndex>.
 	 * 			Throws an GenericPatchException if an error occurs.
 	 */
 	void insertRows(std::size_t rowIndex);

 	/** greebo: Removes columns or rows right before and after the col/row
 	 * 			with the given index, reducing the according dimension by 2.
 	 */
 	void removePoints(bool columns, std::size_t index);

 	/** greebo: Appends two rows or columns at the beginning or the end.
 	 */
 	void appendPoints(bool columns, bool beginning);

	void ConstructPrefab(const AABB& aabb, EPatchPrefab eType, EViewType viewType, std::size_t width = 3, std::size_t height = 3);
	void constructPlane(const AABB& aabb, int axis, std::size_t width, std::size_t height);
	void constructBevel(const AABB& aabb, EViewType viewType);
	void constructEndcap(const AABB& aabb, EViewType viewType);
	void InvertMatrix();

	/**
	 * greebo: This algorithm will transpose the patch matrix
	 * such that the actual control vertex contents remain the same
	 * but their indices in the patch matrix change.
	 * Rows become columns and vice versa.
	 */
	void TransposeMatrix();
	void Redisperse(EMatrixMajor mt);
	void InsertRemove(bool bInsert, bool bColumn, bool bFirst);
  	Patch* MakeCap(Patch* patch, EPatchCap eType, EMatrixMajor mt, bool bFirst);
	void ConstructSeam(EPatchCap eType, Vector3* p, std::size_t width);

	void FlipTexture(int nAxis);

	/** greebo: This translates the texture as much towards
	 * 			the origin as possible. The patch appearance stays unchanged.
	 */
	void normaliseTexture();

	/** greebo: Translate all control vertices in texture space
	 * with the given translation vector (helper method, no undoSave() call)
	 */
	void translateTexCoords(Vector2 translation);

	// This is the same as above, but with undoSave() for use in command sequences
	void TranslateTexture(float s, float t);
	void ScaleTexture(float s, float t);
	void RotateTexture(float angle);
	void SetTextureRepeat(float s, float t); // call with s=1 t=1 for FIT
	void CapTexture();
	void NaturalTexture();
	void ProjectTexture(int nAxis);

	// Aligns the patch texture along the given side/border - if possible
	void alignTexture(EAlignType align);

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
	 * tries to continue the texture seamlessly. The resulting texturing is undistorted.*/
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
	void undoSave();

	// Save the current patch state into a new UndoMemento instance (allocated on heap) and return it to the undo observer
	IUndoMementoPtr exportState() const;

	// Revert the state of this patch to the one that has been saved in the UndoMemento
	void importState(const IUndoMementoPtr& state);

	/** greebo: Sets/gets whether this patch is a patchDef3 (fixed tesselation)
	 */
	bool subdivionsFixed() const;

	/** greebo: Returns the x,y subdivision values (for tesselation)
	 */
	Subdivisions getSubdivisions() const;

	/** greebo: Sets the subdivision of this patch
	 *
	 * @isFixed: TRUE, if this patch should be a patchDef3 (fixed tesselation)
	 * @divisions: a two-component vector containing the desired subdivisions
	 */
	void setFixedSubdivisions(bool isFixed, const Subdivisions& divisions);

	// Calculate the intersection of the given ray with the full patch mesh, 
	// returns true on intersection and fills in the out variable
	bool getIntersection(const Ray& ray, Vector3& intersection);

private:
	// This notifies the surfaceinspector/patchinspector about the texture change
	void textureChanged();

	void updateTesselation();

	// greebo: this allocates the shader with the passed name
	void captureShader();

	// Free the shader from the shadercache
	void releaseShader();

	// greebo: checks, if the shader name is valid
	void check_shader();

	void updateAABB();

	void TesselateSubMatrixFixed(ArbitraryMeshVertex* vertices,
								 std::size_t strideX, std::size_t strideY,
								 unsigned int nFlagsX, unsigned int nFlagsY,
								 PatchControlIter subMatrix[3][3]);

	// uses binary trees representing bezier curves to recursively tesselate a bezier sub-patch
	void TesselateSubMatrix( const BezierCurveTree *BX, const BezierCurveTree *BY,
                           std::size_t offStartX, std::size_t offStartY,
                           std::size_t offEndX, std::size_t offEndY,
                           std::size_t nFlagsX, std::size_t nFlagsY,
                           Vector3& left, Vector3& mid, Vector3& right,
                           Vector2& texLeft, Vector2& texMid, Vector2& texRight,
                           bool bTranspose );

	// tesselates the entire surface
	void BuildTesselationCurves(EMatrixMajor major);
	void accumulateVertexTangentSpace(std::size_t index, Vector3 tangentX[6], Vector3 tangentY[6], Vector2 tangentS[6], Vector2 tangentT[6], std::size_t index0, std::size_t index1);
	void BuildVertexArray();
};
