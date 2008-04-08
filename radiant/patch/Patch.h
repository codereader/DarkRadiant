#ifndef PATCHCLASS_H_
#define PATCHCLASS_H_

#include <vector>

#include "transformlib.h"
#include "scenelib.h"
#include "cullable.h"
#include "editable.h"
#include "nameable.h"
#include "iundo.h"
#include "container/container.h"
#include "irender.h"
#include "mapfile.h"

#include "PatchConstants.h"
#include "PatchControl.h"
#include "PatchTesselation.h"
#include "PatchXMLState.h"
#include "PatchRenderables.h"
#include "brush/TexDef.h"
#include "brush/FacePlane.h"
#include "brush/Face.h"

// This is thrown by the internal patch routines
class GenericPatchException :
	public std::runtime_error
{
public:
	// Constructor
	GenericPatchException(const std::string& what):
		std::runtime_error(what) 
	{}
};

/* greebo: The patch class itself, represented by control vertices. The basic rendering of the patch 
 * is handled here (unselected control points, tesselation lines, shader). 
 * 
 * This class also provides functions to export/import itself to XML.
 */
// parametric surface defined by quadratic bezier control curves
class Patch :
	public TransformNode,
	public Bounded,
	public Cullable,
	public Snappable,
	public Undoable
{
	XMLStateVector m_xml_state;

public:
	// A Patch observer, this is implemented by the PatchNode
	// to re-allocate the patch control instances.
	class Observer {
		public:
			virtual void allocate(std::size_t size) = 0;
	};

private:
	typedef UniqueSet<Observer*> Observers;
	Observers m_observers;

	scene::Node* m_node;

	AABB m_aabb_local; // local bbox

	// greebo: The name of the shader
	std::string m_shader;
	
	ShaderPtr m_state;

	// Patch dimensions
	std::size_t m_width;
	std::size_t m_height;
	
public:
	bool m_patchDef3;
	// The number of subdivisions of this patch
	std::size_t m_subdivisions_x;
	std::size_t m_subdivisions_y;
	
private:
  
	UndoObserver* m_undoable_observer;
  
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
	static ShaderPtr m_state_ctrl;
	static ShaderPtr m_state_lattice;

	// greebo: The vertex list of the control points, can be passed to the RenderableVertexBuffer
	VertexBuffer<PointVertex> m_ctrl_vertices;	
	// The renderable of the control points 
	RenderableVertexBuffer m_render_ctrl;
	
	// The lattice indices and their renderable
	IndexBuffer m_lattice_indices;
	RenderableIndexBuffer m_render_lattice;

	bool m_bOverlay;

	bool m_transformChanged;
	
	// Callback functions when the patch gets changed
	Callback m_evaluateTransform;
	Callback m_boundsChanged;

	// greebo: Initialises the patch member variables
	void construct();

public:
	Callback m_lightsChanged;

	static int m_CycleCapIndex;// = 0;
	
	// The patch type
	static EPatchType m_type;

	// Constructor
	Patch(scene::Node& node, const Callback& evaluateTransform, const Callback& boundsChanged);
	
	// Copy constructors (create this patch from another patch)
	Patch(const Patch& other);
	Patch(const Patch& other, scene::Node& node, const Callback& evaluateTransform, const Callback& boundsChanged);
	
	InstanceCounter m_instanceCounter;

	void instanceAttach(const scene::Path& path);	
	// Remove the attached instance and decrease the counters
	void instanceDetach(const scene::Path& path);

	// Attaches an observer (doh!)
	void attach(Observer* observer);
	
	// Detach the previously attached observer
	void detach(Observer* observer);

	// Allocate callback: pass the allocate call to all the observers
	void onAllocate(std::size_t size);

	// For the TransformNode implementation (localToParent() is abstract and needs to be here), returns identity
	const Matrix4& localToParent() const;
	
	// Return the interally stored AABB
	const AABB& localAABB() const;
	
	VolumeIntersectionValue intersectVolume(const VolumeTest& test, const Matrix4& localToWorld) const;
	
	// Render functions: solid mode, wireframe mode and components 
	void render_solid(Renderer& renderer, const VolumeTest& volume, const Matrix4& localToWorld) const;
	void render_wireframe(Renderer& renderer, const VolumeTest& volume, const Matrix4& localToWorld) const;
	void render_component(Renderer& renderer, const VolumeTest& volume, const Matrix4& localToWorld) const;

	const ShaderPtr& getState() const;
	
	// Implementation of the abstract method of SelectionTestable
	// Called to test if the patch can be selected by the mouse pointer
	void testSelect(Selector& selector, SelectionTest& test);
	
	// Transform this patch as defined by the transformation matrix <matrix>
	void transform(const Matrix4& matrix);
	
	// Called by the transform modifier (owned by the instance) if the transformation gets changed
	void transformChanged();
	
	// The callback type definition
	typedef MemberCaller<Patch, &Patch::transformChanged> TransformChangedCaller;

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

	// Snaps the control points to the grid
	void snapto(float snap);

	void RenderDebug(RenderStateFlags state) const;
	// Renders the normals (indicated by lines) of this patch
	void RenderNormals(RenderStateFlags state) const;

	void popElement(const char* name);
	
	std::size_t write(const char* buffer, std::size_t length);

	void UpdateCachedData();

	// Gets the shader name or sets the shader to <name>
	const std::string& GetShader() const;
	void SetShader(const std::string& name);
	
	// As the name states: get the shader flags of the m_state shader
	int getShaderFlags() const;

	// Const and non-const iterators
	PatchControlIter begin() {
		return m_ctrl.data();
	}
	
	PatchControlConstIter begin() const {
		return m_ctrl.data();
	}
	
	PatchControlIter end() {
		return m_ctrl.data() + m_ctrl.size();
	}
	
	PatchControlConstIter end() const {
		return m_ctrl.data() + m_ctrl.size();
	}

	PatchTesselation& getTesselation();

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
 
	void ConstructPrefab(const AABB& aabb, EPatchPrefab eType, int axis, std::size_t width = 3, std::size_t height = 3);
	void constructPlane(const AABB& aabb, int axis, std::size_t width, std::size_t height);
	void InvertMatrix();
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

	/* greebo: This basically projects all the patch vertices into the brush plane and 
	 * transforms the projected coordinates into the texture plane space */
	void pasteTextureProjected(const Face* face);
	
	// This returns the PatchControl pointer that is closest to the given <point>
	PatchControl* getClosestPatchControlToPoint(const Vector3& point);
	PatchControl* getClosestPatchControlToFace(const Face* face);
	PatchControl* getClosestPatchControlToPatch(const Patch& patch);
	
	// Returns the w,h coordinates within the PatchControlArray of the given <control>
	Vector2 getPatchControlArrayIndices(const PatchControl* control);
	
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
	void createThickenedOpposite(const Patch& sourcePatch, const float& thickness, const int& axis);

	/** greebo: This creates on of the "wall" patches when thickening patches.
	 * 
	 * @sourcePath, targetPatch: these are the top and bottom patches. The wall connects
	 * 				a distinct edge of these two, depending on the wallIndex.
	 *  
	 * @wallIndex: 0..3 (cycle through them to create all four walls).
	 */
	void createThickenedWall(const Patch& sourcePatch, const Patch& targetPatch, const int& wallIndex);

	// called just before an action to save the undo state
	void undoSave();

	// Save the current patch state into a new UndoMemento instance (allocated on heap) and return it to the undo observer
	UndoMemento* exportState() const;

	// Revert the state of this patch to the one that has been saved in the UndoMemento
	void importState(const UndoMemento* state);

	// Initialise the static member variables of this class, called from >> patchmodule.cpp
	static void constructStatic(EPatchType type) {
		Patch::m_type = type;
		Patch::m_state_ctrl = GlobalShaderCache().capture("$POINT");
		Patch::m_state_lattice = GlobalShaderCache().capture("$LATTICE");
	}

	// Release the static member variables of this class, called from >> patchmodule.cpp
	static void destroyStatic() {
		Patch::m_state_ctrl = ShaderPtr();
		Patch::m_state_lattice = ShaderPtr();
	}

	/** greebo: Sets/gets whether this patch is a patchDef3 (fixed tesselation)
	 */
	bool subdivionsFixed() const;
	
	/** greebo: Returns the x,y subdivision values (for tesselation)
	 */
	BasicVector2<unsigned int> getSubdivisions() const;
	
	/** greebo: Sets the subdivision of this patch
	 * 
	 * @isFixed: TRUE, if this patch should be a patchDef3 (fixed tesselation)
	 * @divisions: a two-component vector containing the desired subdivisions
	 */
	void setFixedSubdivisions(bool isFixed, BasicVector2<unsigned int> divisions);

private:
	// This notifies the surfaceinspector/patchinspector about the texture change
	static void textureChanged();

	// greebo: this allocates the shader with the passed name 
	void captureShader();

	// Free the shader from the shadercache
	void releaseShader();

	// greebo: checks, if the shader name is valid
	void check_shader();

	void AccumulateBBox();
  
	void TesselateSubMatrixFixed(ArbitraryMeshVertex* vertices, std::size_t strideX, std::size_t strideY, unsigned int nFlagsX, unsigned int nFlagsY, PatchControl* subMatrix[3][3]);

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
  
public:
	// Destructor
	~Patch();
}; // end class Patch

#endif /*PATCHCLASS_H_*/
