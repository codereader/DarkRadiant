#include "Patch.h"

#include "iregistry.h"
#include "iuimanager.h"
#include "shaderlib.h"
#include "irenderable.h"
#include "selectable.h"
#include "math/frustum.h"
#include "stringio.h"
#include "texturelib.h"
#include "brush/TextureProjection.h"
#include "brush/Winding.h"
#include "gtkutil/dialog.h"
#include "mainframe.h"
#include "ui/surfaceinspector/SurfaceInspector.h"
#include "ui/patch/PatchInspector.h"

#include "PatchSavedState.h"

// ====== Helper Functions ==================================================================

inline VertexPointer vertexpointer_arbitrarymeshvertex(const ArbitraryMeshVertex* array) {
  return VertexPointer(VertexPointer::pointer(&array->vertex), sizeof(ArbitraryMeshVertex));
}

inline const Colour4b colour_for_index(std::size_t i, std::size_t width) {
  const Vector3 cornerColourVec = ColourSchemes().getColour("patch_vertex_corner");
  const Vector3 insideColourVec = ColourSchemes().getColour("patch_vertex_inside");
  const Colour4b colour_corner(int(cornerColourVec[0]*255), int(cornerColourVec[1]*255), 
  							   int(cornerColourVec[2]*255), 255);
  const Colour4b colour_inside(int(insideColourVec[0]*255), int(insideColourVec[1]*255), 
  							   int(insideColourVec[2]*255), 255);
  
  return (i%2 || (i/width)%2) ? colour_inside : colour_corner;
}

inline bool double_valid(double f) {
  return f == f;
}

// ====== Patch Implementation =========================================================================

// Initialise the shader state variables and the patch type
ShaderPtr Patch::m_state_ctrl;
ShaderPtr Patch::m_state_lattice;
EPatchType Patch::m_type;

// Initialise the cycle cap index member variable
int Patch::m_CycleCapIndex = 0;

	namespace {
		const std::size_t MAX_PATCH_SUBDIVISIONS = 32;
	}

// Constructor
Patch::Patch(scene::Node& node, const Callback& evaluateTransform, const Callback& boundsChanged) :
	m_node(&node),
	m_shader(texdef_name_default()),
	m_undoable_observer(0),
	m_map(0),
	m_render_solid(m_tess),
	m_render_wireframe(m_tess),
	m_render_wireframe_fixed(m_tess),
	m_render_ctrl(GL_POINTS, m_ctrl_vertices),
	m_render_lattice(GL_LINES, m_lattice_indices, m_ctrl_vertices),
	m_transformChanged(false),
	m_evaluateTransform(evaluateTransform),
	m_boundsChanged(boundsChanged)
{
	construct();
}

// Copy constructor	(create this patch from another patch)
Patch::Patch(const Patch& other, scene::Node& node, const Callback& evaluateTransform, const Callback& boundsChanged) :
	m_node(&node),
	m_shader(texdef_name_default()),
	m_undoable_observer(0),
	m_map(0),
	m_render_solid(m_tess),
	m_render_wireframe(m_tess),
	m_render_wireframe_fixed(m_tess),
	m_render_ctrl(GL_POINTS, m_ctrl_vertices),
	m_render_lattice(GL_LINES, m_lattice_indices, m_ctrl_vertices),
	m_transformChanged(false),
	m_evaluateTransform(evaluateTransform),
	m_boundsChanged(boundsChanged)
{
	// Initalise the default values
	construct();

	// Copy over the definitions from the <other> patch  
	m_patchDef3 = other.m_patchDef3;
	m_subdivisions_x = other.m_subdivisions_x;
	m_subdivisions_y = other.m_subdivisions_y;
	setDims(other.m_width, other.m_height);
	copy_ctrl(m_ctrl.data(), other.m_ctrl.data(), other.m_ctrl.data()+(m_width*m_height));
	SetShader(other.m_shader);
	controlPointsChanged();
}

// Another copy constructor
Patch::Patch(const Patch& other) :
	TransformNode(other),
	Bounded(other),
	Cullable(other),
	Snappable(other),
	Undoable(other),
	m_undoable_observer(0),
	m_map(0),
	m_render_solid(m_tess),
	m_render_wireframe(m_tess),
	m_render_wireframe_fixed(m_tess),
	m_render_ctrl(GL_POINTS, m_ctrl_vertices),
	m_render_lattice(GL_LINES, m_lattice_indices, m_ctrl_vertices),
	m_transformChanged(false),
	m_evaluateTransform(other.m_evaluateTransform),
	m_boundsChanged(other.m_boundsChanged)
{
	// Copy over the definitions from the other patch
	m_bOverlay = false;

	m_patchDef3 = other.m_patchDef3;
	m_subdivisions_x = other.m_subdivisions_x;
	m_subdivisions_y = other.m_subdivisions_y;
	setDims(other.m_width, other.m_height);
	copy_ctrl(m_ctrl.data(), other.m_ctrl.data(), other.m_ctrl.data()+(m_width*m_height));
	SetShader(other.m_shader);
	controlPointsChanged();
}

// greebo: Initialises the patch member variables
void Patch::construct() {
	m_bOverlay = false;
	m_width = m_height = 0;

	m_patchDef3 = false;
	m_subdivisions_x = 0;
	m_subdivisions_y = 0;

	// Check, if the shader name is correct
	check_shader();
	
	// Allocate the shader
	captureShader();

	m_xml_state.push_back(XMLState::eDefault);
}

// Get the current control point array
PatchControlArray& Patch::getControlPoints() {
	return m_ctrl;
}

// Same as above, just for const arguments
const PatchControlArray& Patch::getControlPoints() const {
	return m_ctrl;
}

// Get the (temporary) transformed control point array, not the saved ones
PatchControlArray& Patch::getControlPointsTransformed() {
	return m_ctrlTransformed;
}

const PatchControlArray& Patch::getControlPointsTransformed() const {
	return m_ctrlTransformed;
}

std::size_t Patch::getWidth() const {
	return m_width;
}

std::size_t Patch::getHeight() const {
	return m_height;
}

void Patch::setDims (std::size_t w, std::size_t h)
{
  if((w%2)==0)
    w -= 1;
  ASSERT_MESSAGE(w <= MAX_PATCH_WIDTH, "patch too wide");
  if(w > MAX_PATCH_WIDTH)
    w = MAX_PATCH_WIDTH;
  else if(w < MIN_PATCH_WIDTH)
    w = MIN_PATCH_WIDTH;
  
  if((h%2)==0)
    m_height -= 1;
  ASSERT_MESSAGE(h <= MAX_PATCH_HEIGHT, "patch too tall");
  if(h > MAX_PATCH_HEIGHT)
    h = MAX_PATCH_HEIGHT;
  else if(h < MIN_PATCH_HEIGHT)
    h = MIN_PATCH_HEIGHT;

  m_width = w; m_height = h;

  if(m_width * m_height != m_ctrl.size())
  {
    m_ctrl.resize(m_width * m_height);
    onAllocate(m_ctrl.size());
  }
}

void Patch::instanceAttach(const scene::Path& path) {
	if (++m_instanceCounter.m_count == 1) {
		// Notify the shader that one more instance is using this
		m_state->incrementUsed();
		
		m_map = path_find_mapfile(path.begin(), path.end());
		
		// Attach the UndoObserver to this patch
		m_undoable_observer = GlobalUndoSystem().observer(this);
	}
	else {
		// The counter is now >1, throw an error
		ASSERT_MESSAGE(path_find_mapfile(path.begin(), path.end()) == m_map, "node is instanced across more than one file");
	}
}

// Remove the attached instance and decrease the counters
void Patch::instanceDetach(const scene::Path& path) {
	if(--m_instanceCounter.m_count == 0) {
		m_map = 0;
		m_undoable_observer = 0;
		GlobalUndoSystem().release(this);
		m_state->decrementUsed();
	}
}

// Attaches an observer (doh!)
void Patch::attach(Observer* observer) {
	observer->allocate(m_width * m_height);
	// Add this to the internal list
	m_observers.insert(observer);
}

// Detach the previously attached observer
void Patch::detach(Observer* observer) {
	m_observers.erase(observer);
}

// Allocate callback: pass the allocate call to all the observers
void Patch::onAllocate(std::size_t size) {
	for (Observers::iterator i = m_observers.begin(); i != m_observers.end(); ++i) {
		(*i)->allocate(size);
	}
}

// For the TransformNode implementation (localToParent() is abstract and needs to be here)
const Matrix4& Patch::localToParent() const {
	// Just return the identity matrix
	return g_matrix4_identity;
}

// Return the interally stored AABB
const AABB& Patch::localAABB() const {
	return m_aabb_local;
}

VolumeIntersectionValue Patch::intersectVolume(const VolumeTest& test, const Matrix4& localToWorld) const {
	return test.TestAABB(m_aabb_local, localToWorld);
}

// Render functions: solid mode
void Patch::render_solid(Renderer& renderer, const VolumeTest& volume, const Matrix4& localToWorld) const {
	renderer.SetState(m_state, Renderer::eFullMaterials);
	renderer.addRenderable(m_render_solid, localToWorld);
}

// Render functions for WireFrame rendering
void Patch::render_wireframe(Renderer& renderer, const VolumeTest& volume, const Matrix4& localToWorld) const {
	renderer.SetState(m_state, Renderer::eFullMaterials);
	if (m_patchDef3) {
		renderer.addRenderable(m_render_wireframe_fixed, localToWorld);
	}
	else {
		renderer.addRenderable(m_render_wireframe, localToWorld);
	}
}

// greebo: This renders the patch components, namely the lattice and the corner controls
void Patch::render_component(Renderer& renderer, const VolumeTest& volume, const Matrix4& localToWorld) const {
	renderer.SetState(m_state_lattice, Renderer::eWireframeOnly);
	renderer.SetState(m_state_lattice, Renderer::eFullMaterials);
	renderer.addRenderable(m_render_lattice, localToWorld);

	renderer.SetState(m_state_ctrl, Renderer::eWireframeOnly);
	renderer.SetState(m_state_ctrl, Renderer::eFullMaterials);
	renderer.addRenderable(m_render_ctrl, localToWorld);
}

const ShaderPtr& Patch::getState() const {
	return m_state;
}

// Implementation of the abstract method of SelectionTestable
// Called to test if the patch can be selected by the mouse pointer
void Patch::testSelect(Selector& selector, SelectionTest& test) {
	SelectionIntersection best;
	IndexPointer::index_type* pIndex = m_tess.m_indices.data();
	
	for (std::size_t s=0; s<m_tess.m_numStrips; s++) {
		test.TestQuadStrip(vertexpointer_arbitrarymeshvertex(m_tess.m_vertices.data()), IndexPointer(pIndex, m_tess.m_lenStrips), best);
		pIndex += m_tess.m_lenStrips;
	}
	
	if (best.valid()) {
		selector.addIntersection(best);
	}
}

// Transform this patch as defined by the transformation matrix <matrix>
void Patch::transform(const Matrix4& matrix) {
	// Cycle through all the patch control vertices and transform the points
	for (PatchControlIter i = m_ctrlTransformed.data(); 
		 i != m_ctrlTransformed.data() + m_ctrlTransformed.size(); 
		 ++i) 
	{
		matrix4_transform_point(matrix, (*i).m_vertex);
	}
	
	// Check the handedness of the matrix and invert it if needed
	if(matrix4_handedness(matrix) == MATRIX4_LEFTHANDED) {
		PatchControlArray_invert(m_ctrlTransformed, m_width, m_height);
	}
	
	UpdateCachedData();
}

// Called if the patch has changed, so that the scene graph can be updated
void Patch::transformChanged() {
	m_transformChanged = true;
	m_lightsChanged();
	SceneChangeNotify();
}

// Called to evaluate the transform
void Patch::evaluateTransform() {
	// Only do something, if the patch really has changed
	if (m_transformChanged) {
		m_transformChanged = false;
		revertTransform();
		m_evaluateTransform();
	}
}

// Revert the changes, fall back to the saved state in <m_ctrl>
void Patch::revertTransform() {
	m_ctrlTransformed = m_ctrl;
}

// Apply the transformed control array, save it into <m_ctrl> and overwrite the old values
void Patch::freezeTransform() {
	undoSave();
	evaluateTransform();
	// Check if the sizes of the patch control arrays match
	ASSERT_MESSAGE(m_ctrlTransformed.size() == m_ctrl.size(), "Patch::freeze: size mismatch");
	
	// Save the control array over <m_ctrl>
	std::copy(m_ctrlTransformed.begin(), m_ctrlTransformed.end(), m_ctrl.begin());
}

// callback for changed control points
void Patch::controlPointsChanged() {
	transformChanged();
	evaluateTransform();
	UpdateCachedData();
}

// Snaps the control points to the grid
void Patch::snapto(float snap) {
	undoSave();

	for(PatchControlIter i = m_ctrl.data(); i != m_ctrl.data() + m_ctrl.size(); ++i) {
		vector3_snap((*i).m_vertex, snap);
	}

	controlPointsChanged();
}

const std::string& Patch::GetShader() const {
	return m_shader;
}

void Patch::SetShader(const std::string& name) {
		ASSERT_NOTNULL(name.c_str());
  
  		// return, if the shader is the same as the currently used 
	if (shader_equal(m_shader, name))
		return;

	undoSave();

	// Decrement the use count of the shader
	if (m_instanceCounter.m_count != 0) {
		m_state->decrementUsed();
	}
	
	// release the shader
	releaseShader();
	
	// Set the name of the shader and capture it
	m_shader = name;
	captureShader();
	
	// Increment the counter
	if (m_instanceCounter.m_count != 0) {
		m_state->incrementUsed();
	}

	// Check if the shader is ok
	check_shader();
	// Call the callback functions
	textureChanged();
}

int Patch::getShaderFlags() const {
	if(m_state != 0) {
		return m_state->getFlags();
	}
	return 0;
}

// Return a defined patch control vertex at <row>,<col>
PatchControl& Patch::ctrlAt(std::size_t row, std::size_t col) {
	return m_ctrl[row*m_width+col];
}

// The same as above just for const 
const PatchControl& Patch::ctrlAt(std::size_t row, std::size_t col) const {
	return m_ctrl[row*m_width+col];
}

// called just before an action to save the undo state
void Patch::undoSave() {
	// Notify the map
	if (m_map != 0) {
		m_map->changed();
	}
	
	// Notify the undo observer to save this patch state
	if (m_undoable_observer != 0) {
		m_undoable_observer->save(this);
	}
}

// Save the current patch state into a new UndoMemento instance (allocated on heap) and return it to the undo observer
UndoMemento* Patch::exportState() const {
	return new SavedState(m_width, m_height, m_ctrl, m_shader, m_patchDef3, m_subdivisions_x, m_subdivisions_y);
}

// Revert the state of this patch to the one that has been saved in the UndoMemento
void Patch::importState(const UndoMemento* state) {
	undoSave();

	const SavedState& other = *(static_cast<const SavedState*>(state));

	// begin duplicate of SavedState copy constructor, needs refactoring

    // copy construct
	{
		m_width = other.m_width;
		m_height = other.m_height;
		SetShader(other.m_shader);
		m_ctrl = other.m_ctrl;
		onAllocate(m_ctrl.size());
		m_patchDef3 = other.m_patchDef3;
		m_subdivisions_x = other.m_subdivisions_x;
		m_subdivisions_y = other.m_subdivisions_y;
	}

	// end duplicate code

	// Notify that this patch has changed
	textureChanged();
	controlPointsChanged();
}

void Patch::captureShader() {
	m_state = GlobalShaderCache().capture(m_shader.c_str());
}

void Patch::releaseShader() {
	m_state = ShaderPtr(); 
}

void Patch::check_shader() {
	if (!shader_valid(GetShader().c_str())) {
		globalErrorStream() << "patch has invalid texture name: '" << GetShader().c_str() << "'\n";
	}
}

// Patch Destructor
Patch::~Patch() {
	BezierCurveTreeArray_deleteAll(m_tess.m_curveTreeU);
	BezierCurveTreeArray_deleteAll(m_tess.m_curveTreeV);

	// Release the shader
	releaseShader();
	
	// Check if any observers for this patch remain
	ASSERT_MESSAGE(m_observers.empty(), "Patch::~Patch: observers still attached");
}

bool Patch::isValid() const
{
  if(!m_width || !m_height)
  {
    return false;
  }

  for(PatchControlConstIter i = m_ctrl.begin(); i != m_ctrl.end(); ++i)
  {
    if(!double_valid((*i).m_vertex.x())
      || !double_valid((*i).m_vertex.y())
      || !double_valid((*i).m_vertex.z())
      || !double_valid((*i).m_texcoord.x())
      || !double_valid((*i).m_texcoord.y()))
    {
      globalErrorStream() << "patch has invalid control points\n";
      return false;
    }
  }
  return true;
}

void Patch::UpdateCachedData()
{
  m_ctrl_vertices.clear();
  m_lattice_indices.clear();

  if(!isValid())
  {
    m_tess.m_numStrips = 0;
    m_tess.m_lenStrips = 0;
    m_tess.m_nArrayHeight = 0;
    m_tess.m_nArrayWidth = 0;
    m_tess.m_curveTreeU.resize(0);
    m_tess.m_curveTreeV.resize(0);
    m_tess.m_indices.resize(0);
    m_tess.m_vertices.resize(0);
    m_tess.m_arrayHeight.resize(0);
    m_tess.m_arrayWidth.resize(0);
    m_aabb_local = AABB();
    return;
  }

  BuildTesselationCurves(ROW);
  BuildTesselationCurves(COL);
  BuildVertexArray();
  AccumulateBBox();

  IndexBuffer ctrl_indices;

  m_lattice_indices.reserve(((m_width * (m_height - 1)) + (m_height * (m_width - 1))) << 1);
  ctrl_indices.reserve(m_ctrlTransformed.size());
  {
    UniqueVertexBuffer<PointVertex> inserter(m_ctrl_vertices);
    for(PatchControlIter i = m_ctrlTransformed.begin(); i != m_ctrlTransformed.end(); ++i)
    {
      ctrl_indices.insert(inserter.insert(pointvertex_quantised(PointVertex(reinterpret_cast<const Vertex3f&>((*i).m_vertex), colour_for_index(i - m_ctrlTransformed.begin(), m_width)))));
    }
  }
  {
    for(IndexBuffer::iterator i = ctrl_indices.begin(); i != ctrl_indices.end(); ++i)
    {
      if(std::size_t(i - ctrl_indices.begin()) % m_width)
      {
        m_lattice_indices.insert(*(i - 1));
        m_lattice_indices.insert(*i);
      }
      if(std::size_t(i - ctrl_indices.begin()) >= m_width)
      {
        m_lattice_indices.insert(*(i - m_width));
        m_lattice_indices.insert(*i);
      }
    }
  }

#if 0
  {
    Array<RenderIndex>::iterator first = m_tess.m_indices.begin();
    for(std::size_t s=0; s<m_tess.m_numStrips; s++)
    {
      Array<RenderIndex>::iterator last = first + m_tess.m_lenStrips;

      for(Array<RenderIndex>::iterator i(first); i+2 != last; i += 2)
      {
        ArbitraryMeshTriangle_sumTangents(m_tess.m_vertices[*(i+0)], m_tess.m_vertices[*(i+1)], m_tess.m_vertices[*(i+2)]);
        ArbitraryMeshTriangle_sumTangents(m_tess.m_vertices[*(i+2)], m_tess.m_vertices[*(i+1)], m_tess.m_vertices[*(i+3)]);
      }

      first = last;
    }

    for(Array<ArbitraryMeshVertex>::iterator i = m_tess.m_vertices.begin(); i != m_tess.m_vertices.end(); ++i)
    {
      vector3_normalise(reinterpret_cast<Vector3&>((*i).tangent));
      vector3_normalise(reinterpret_cast<Vector3&>((*i).bitangent));
    }
  }
#endif

  SceneChangeNotify();
}

void Patch::InvertMatrix()
{
  undoSave();

  PatchControlArray_invert(m_ctrl, m_width, m_height);

  controlPointsChanged();
}

void Patch::TransposeMatrix()
{
  undoSave();

  {
    Array<PatchControl> tmp(m_width * m_height);
    copy_ctrl(tmp.data(), m_ctrl.data(), m_ctrl.data() + m_width * m_height);

    PatchControlIter from = tmp.data();
    for(std::size_t h = 0; h != m_height; ++h)
    {
      PatchControlIter to = m_ctrl.data() + h;
      for(std::size_t w = 0; w != m_width; ++w, ++from, to += m_height)
      {
        *to = *from;
      }
    }
  }

  {
    std::size_t tmp = m_width;
    m_width = m_height;
    m_height = tmp;
  }
   
  controlPointsChanged();
}

void Patch::Redisperse(EMatrixMajor mt)
{
  std::size_t w, h, width, height, row_stride, col_stride;
  PatchControl* p1, * p2, * p3;

  undoSave();

  switch(mt)
  {
  case COL:
    width = (m_width-1)>>1;
    height = m_height;
    col_stride = 1;
    row_stride = m_width;
    break;
  case ROW:
    width = (m_height-1)>>1;
    height = m_width;
    col_stride = m_width;
    row_stride = 1;
    break;
  default:
    ERROR_MESSAGE("neither row-major nor column-major");
    return;
  }

  for(h=0;h<height;h++)
  {
    p1 = m_ctrl.data()+(h*row_stride);
    for(w=0;w<width;w++)
    {
      p2 = p1+col_stride;
      p3 = p2+col_stride;
      p2->m_vertex = vector3_mid(p1->m_vertex, p3->m_vertex);
      p1 = p3;
    }
  }
  
  controlPointsChanged();
}

void Patch::InsertRemove(bool bInsert, bool bColumn, bool bFirst) {
	undoSave();

	try {
		if (bInsert) {
			// Decide whether we should insert rows or columns
			if (bColumn) {
				// The insert point is 1 for "beginning" and width-2 for "end"
				insertColumns(bFirst ? 1 : m_width-2);
			}
			else {
				// The insert point is 1 for "beginning" and height-2 for "end"
				insertRows(bFirst ? 1 : m_height-2);
			}
		}
		else {
			// Col/Row Removal
			if (bColumn) {
				// Column removal, pass TRUE
				removePoints(true, bFirst ? 2 : m_width - 3);					
			}
			else {
				// Row removal, pass FALSE
				removePoints(false, bFirst ? 2 : m_height - 3);
			}
		}
	}
	catch (GenericPatchException g) {
		globalErrorStream() << "Error manipulating patch dimensions: " << g.what() << "\n";
	}

	controlPointsChanged();
}

void Patch::appendPoints(bool columns, bool beginning) {
	bool rows = !columns; // Shortcut for readability
	
	if ((columns && m_width + 2 > MAX_PATCH_WIDTH) ||
		(rows && m_height + 2 > MAX_PATCH_HEIGHT))
	{
		globalErrorStream() << "Patch::appendPoints() error: " << 
							   "Cannot make patch any larger.\n";
		return;
	}
	
	// Sanity check passed, now start the action
	undoSave();
	
	// Create a backup of the old control vertices
	PatchControlArray oldCtrl = m_ctrl;
	std::size_t oldHeight = m_height;
	std::size_t oldWidth = m_width;
	
	// Resize this patch
	setDims(columns ? oldWidth+2 : oldWidth, rows ? oldHeight+2 : oldHeight);
	
	// Specify the target row to copy the values to
	std::size_t targetColStart = (columns && beginning) ? 2 : 0;
	std::size_t targetRowStart = (rows && beginning) ? 2 : 0;
	
	// We're copying the old patch matrix into a sub-matrix of the new patch
	// Fill in the control vertex values into the target area using this loop
	for (std::size_t newRow = targetRowStart, oldRow = 0; 
		 newRow < m_height && oldRow < oldHeight; 
		 newRow++, oldRow++)
	{
		for (std::size_t newCol = targetColStart, oldCol = 0; 
			 oldCol < oldWidth && newCol < m_width; 
			 oldCol++, newCol++) 
		{
			// Copy the control vertex from the old patch to the new patch
			ctrlAt(newRow, newCol).m_vertex = oldCtrl[oldRow*oldWidth + oldCol].m_vertex;		
			ctrlAt(newRow, newCol).m_texcoord = oldCtrl[oldRow*oldWidth + oldCol].m_texcoord;
		}
	}
	
	if (columns) {
		// Extrapolate the vertex attributes of the columns
		
		// These are the indices of the new columns
		std::size_t newCol1 = beginning ? 0 : m_width - 1; // The outermost column
		std::size_t newCol2 = beginning ? 1 : m_width - 2; // The nearest column
		
		// This indicates the direction we are taking the base values from
		// If we start at the beginning, we have to take the values on 
		// the "right", hence the +1 index
		int neighbour = beginning ? +1 : -1;
		
		for (std::size_t row = 0; row < m_height; row++) {
			// The distance of the two neighbouring columns, 
			// this is taken as extrapolation value
			Vector3 vertexDiff = ctrlAt(row, newCol2 + neighbour).m_vertex - 
								 ctrlAt(row, newCol2 + 2*neighbour).m_vertex;
			Vector2 texDiff = ctrlAt(row, newCol2 + neighbour).m_texcoord -
							  ctrlAt(row, newCol2 + 2*neighbour).m_texcoord;
			
			// Extrapolate the values of the nearest column
			ctrlAt(row, newCol2).m_vertex = ctrlAt(row, newCol2 + neighbour).m_vertex + vertexDiff;
			ctrlAt(row, newCol2).m_texcoord = ctrlAt(row, newCol2 + neighbour).m_texcoord + texDiff;
			
			// Extrapolate once again linearly from the nearest column to the outermost column
			ctrlAt(row, newCol1).m_vertex = ctrlAt(row, newCol2).m_vertex + vertexDiff;
			ctrlAt(row, newCol1).m_texcoord = ctrlAt(row, newCol2).m_texcoord + texDiff;
		}
	}
	else {
		// Extrapolate the vertex attributes of the rows
		
		// These are the indices of the new rows
		std::size_t newRow1 = beginning ? 0 : m_height - 1; // The outermost row
		std::size_t newRow2 = beginning ? 1 : m_height - 2; // The nearest row
		
		// This indicates the direction we are taking the base values from
		// If we start at the beginning, we have to take the values on 
		// the "right", hence the +1 index
		int neighbour = beginning ? +1 : -1;
		
		for (std::size_t col = 0; col < m_width; col++) {
			// The distance of the two neighbouring rows, 
			// this is taken as extrapolation value
			Vector3 vertexDiff = ctrlAt(newRow2 + neighbour, col).m_vertex - 
								 ctrlAt(newRow2 + 2*neighbour, col).m_vertex;
			Vector2 texDiff = ctrlAt(newRow2 + neighbour, col).m_texcoord -
							  ctrlAt(newRow2 + 2*neighbour, col).m_texcoord;
			
			// Extrapolate the values of the nearest row
			ctrlAt(newRow2, col).m_vertex = ctrlAt(newRow2 + neighbour, col).m_vertex + vertexDiff;
			ctrlAt(newRow2, col).m_texcoord = ctrlAt(newRow2 + neighbour, col).m_texcoord + texDiff;
			
			// Extrapolate once again linearly from the nearest row to the outermost row
			ctrlAt(newRow1, col).m_vertex = ctrlAt(newRow2, col).m_vertex + vertexDiff;
			ctrlAt(newRow1, col).m_texcoord = ctrlAt(newRow2, col).m_texcoord + texDiff;
		}
	}
	
	controlPointsChanged();
}

Patch* Patch::MakeCap(Patch* patch, EPatchCap eType, EMatrixMajor mt, bool bFirst)
{
  std::size_t i, width, height;

  switch(mt)
  {
  case ROW:
    width = m_width;
    height = m_height;
    break;
  case COL:
    width = m_height;
    height = m_width;
    break;
  default:
    ERROR_MESSAGE("neither row-major nor column-major");
    return 0;
  }

  Array<Vector3> p(width);

  std::size_t nIndex = (bFirst) ? 0 : height-1;
  if(mt == ROW)
  {
    for (i=0; i<width; i++)
    {
      p[(bFirst)?i:(width-1)-i] = ctrlAt(nIndex, i).m_vertex;
    }
  }
  else
  {
    for (i=0; i<width; i++)
    {
      p[(bFirst)?i:(width-1)-i] = ctrlAt(i, nIndex).m_vertex;
    }
  }

  patch->ConstructSeam(eType, p.data(), width);
  
  // greebo: Apply natural texture to that patch, to fix the texcoord==1.#INF bug.
  patch->NaturalTexture();
  return patch;
}

void Patch::FlipTexture(int nAxis)
{
  undoSave();

  for(PatchControlIter i = m_ctrl.data(); i != m_ctrl.data() + m_ctrl.size(); ++i)
  {
    (*i).m_texcoord[nAxis] = -(*i).m_texcoord[nAxis];
  }
  
  controlPointsChanged();
}

/** greebo: Helper function that shifts all control points in
 * texture space about <s,t>
 */
void Patch::translateTexCoords(Vector2 translation) {
	// Cycle through all control points and shift them in texture space
	for (PatchControlIter i = m_ctrl.data(); i != m_ctrl.data() + m_ctrl.size(); ++i) {
    	i->m_texcoord += translation;
  	}
}

void Patch::TranslateTexture(float s, float t)
{
  undoSave();

  s = -1 * s / m_state->getTexture().width;
  t = t / m_state->getTexture().height;

	translateTexCoords(Vector2(s,t));
  
  controlPointsChanged();
}

void Patch::ScaleTexture(float s, float t)
{
  undoSave();

  for(PatchControlIter i = m_ctrl.data(); i != m_ctrl.data() + m_ctrl.size(); ++i)
  {
    (*i).m_texcoord[0] *= s;
    (*i).m_texcoord[1] *= t;
  }

  controlPointsChanged();
}

void Patch::RotateTexture(float angle)
{
  undoSave();

  const float s = static_cast<float>(sin(degrees_to_radians(angle)));
  const float c = static_cast<float>(cos(degrees_to_radians(angle)));
    
  for(PatchControlIter i = m_ctrl.data(); i != m_ctrl.data() + m_ctrl.size(); ++i)
  {
    const float x = (*i).m_texcoord[0];
    const float y = (*i).m_texcoord[1];
    (*i).m_texcoord[0] = (x * c) - (y * s);
    (*i).m_texcoord[1] = (y * c) + (x * s);
  }

  controlPointsChanged();
}

/* greebo: Tile the selected texture on the patch (s times t)
 */
void Patch::SetTextureRepeat(float s, float t) {
	std::size_t w, h;
	float sIncr, tIncr, sc, tc;
	
	// The pointer to cycle through the control points
	PatchControl *pDest;

	// Save the current patch state to the undoMemento
	undoSave();

	/* greebo: Calculate the texture width and height increment per control point.
	 * If we have a 4x4 patch and want to tile it 3x3, the distance
	 * from one control point to the next one has to cover 3/4 of a full texture,
	 * hence texture_x_repeat/patch_width and texture_y_repeat/patch_height.*/ 
	sIncr = s / (float)(m_width - 1);
	tIncr = t / (float)(m_height - 1);

	// Set the pointer to the first control point
	pDest = m_ctrl.data();
	
	// Cycle through the patch matrix (row per row)
	// Increment the <tc> counter by <tIncr> increment
	for (h=0, tc = 0.0f; h<m_height; h++, tc+=tIncr) {
		// Cycle through the row points: reset sc to zero 
		// and increment it by sIncr at each step. 
		for (w=0, sc = 0.0f; w<m_width; w++, sc+=sIncr) {
			// Set the texture coordinates
			pDest->m_texcoord[0] = sc;
			pDest->m_texcoord[1] = tc;
			// Set the pointer to the next control point
			pDest++;
		}
	}

	// Notify the patch
	controlPointsChanged();
}

/*
void Patch::SetTextureInfo(TexDef *pt)
{
  if(pt->getShift()[0] || pt->getShift()[1])
    TranslateTexture (pt->getShift()[0], pt->getShift()[1]);
  else if(pt->getScale()[0] || pt->getScale()[1])
  {
    if(pt->getScale()[0] == 0.0f) pt->setScale(0, 1.0f);
    if(pt->getScale()[1] == 0.0f) pt->setScale(1, 1.0f);
    ScaleTexture (pt->getScale()[0], pt->getScale()[1]);
  }
  else if(pt->rotate)
    RotateTexture (pt->rotate);
}
*/

inline int texture_axis(const Vector3& normal)
{
  // axis dominance order: Z, X, Y
  return (normal.x() >= normal.y()) ? (normal.x() > normal.z()) ? 0 : 2 : (normal.y() > normal.z()) ? 1 : 2; 
}

void Patch::CapTexture() {
	const PatchControl& p1 = m_ctrl[m_width];
	const PatchControl& p2 = m_ctrl[m_width*(m_height-1)];
	const PatchControl& p3 = m_ctrl[(m_width*m_height)-1];

  Vector3 normal(g_vector3_identity);

  {
    Vector3 tmp( (p2.m_vertex - m_ctrl[0].m_vertex).crossProduct(p3.m_vertex - m_ctrl[0].m_vertex) );
    if(tmp != g_vector3_identity)
    {
      normal += tmp;
    }
  }
  {
    Vector3 tmp( (p1.m_vertex - p3.m_vertex).crossProduct(m_ctrl[0].m_vertex - p3.m_vertex) );
    if(tmp != g_vector3_identity)
    {
      normal += tmp;
    }
  }

  ProjectTexture(texture_axis(normal));
}

/* uses longest parallel chord to calculate texture coords for each row/col
 * greebo: The default texture scale is used when applying "Natural" texturing.
 * Note: all the comments in this method are by myself, so be careful... ;)
 * 
 * The applied texture is never stretched more than the default
 * texture scale. So if the default texture scale is 0.5, the texture coordinates
 * are set such that the scaling never exceeds this value. (Don't know why 
 * this is called "Natural" but so what...)
 */
void Patch::NaturalTexture() {
	
	// Save the undo memento
	undoSave();

	// Retrieve the default scale from the registry
	float defaultScale = GlobalRegistry().getFloat("user/ui/textures/defaultTextureScale");

	/* In this next block, the routine cycles through all the patch columns and
	 * measures the distances to the next column. 
	 * 
	 * During each column cycle, the longest distance from this column to the 
	 * next is determined. The distance is added to tex and if the sum
	 * is longer than texBest, the sum will be the new texBest.
	 * 
	 * I'm not sure in which case the sum will ever by smaller than texBest,
	 * as the distance to the next column is always >= 0 and hence the sum should 
	 * always be greater than texBest, but maybe there are some cases where
	 * the next column is equal to the current one. 
	 * 
	 * All the distances are converted into "texture lengths" (i.e. multiples of
	 * the scaled texture size in pixels.
	 */	
	{
		float fSize = (float)m_state->getTexture().width * defaultScale;
  
		double texBest = 0;
		double tex = 0;
		
		// Set the pWidth PatchControl pointer onto the first element
		PatchControl* pWidth = m_ctrl.data();
		
		// Cycle through the patch width, 
		// w is the counter, pWidth is the pointer that is cycling through the elements 
		for (std::size_t w=0; w<m_width; w++, pWidth++) {
			
			// Apply the currently active <tex> value to the control point texture coordinates.
			{
				// Create another pointer and set it to the current pWidth pointer
				// then cycle through the height. This "scans" the patch column-wise.
				PatchControl* pHeight = pWidth;
				
				// Note the increment of m_width, which relocates the pointer by a whole row each step 
				for (std::size_t h=0; h<m_height; h++, pHeight+=m_width) {
					// Set the x-coord (or better s-coord?) of the texture to tex.
					// For the first pWidth cycle this is tex=0, so the texture is not shifted at the first vertex
					pHeight->m_texcoord[0] = static_cast<float>(tex);
				}
			}

			// If we reached the last row (m_width - 1) we are finished (all coordinates are applied)
			if (w+1 == m_width)
				break;

			// Determine the longest distance to the next column.
			{
				// Set the pointer pHeight to the current control vertex column
				PatchControl* pHeight = pWidth;
				
				// Again, cycle through the current column
				for (std::size_t h=0; h<m_height; h++, pHeight+=m_width) {
					// v is the vector pointing from one control point to the next neighbour
					Vector3 v(pHeight->m_vertex - (pHeight+1)->m_vertex);
					
					// translate the distance in world coordinates into texture coords 
					// (i.e. divide it by the scaled texture size) and add it to the
					// current tex value
					double length = tex + (v.getLength() / fSize);
					
					// Now compare this calculated "texture length"
					// to the value stored in texBest. If the length is larger,
					// replace texBest with it, so texBest contains the largest 
					// width found so far (in texture coordinates)
					if (fabs(length) > texBest) {
						texBest = length;
					}
				}
			}
			
			// texBest is stored in <tex>, it gets applied in the next column sweep
			tex = texBest;
		}
	}

	// Now the same goes for the texture height, cycle through all the rows
	// and calculate the longest distances, convert them to texture coordinates
	// and apply them to the according texture coordinates.
	{
		float fSize = -(float)m_state->getTexture().height * defaultScale;
		
		double texBest = 0;
		double tex = 0;
		
		// Set the pointer to the first vertex
		PatchControl* pHeight = m_ctrl.data();
		
		// Each row is visited once
		for (std::size_t h=0; h<m_height; h++, pHeight+=m_width) {
			
			// During each row, we cycle through every height point
		    {
		    	PatchControl* pWidth = pHeight;
				for (std::size_t w=0; w<m_width; w++, pWidth++) {
					pWidth->m_texcoord[1] = static_cast<float>(tex);
				}
			}
	
			if (h+1 == m_height)
				break;
	
			{
				PatchControl* pWidth = pHeight;
				for (std::size_t w=0; w<m_width; w++, pWidth++) {
					Vector3 v(pWidth->m_vertex - (pWidth+m_width)->m_vertex);
					double length = tex + (v.getLength() / fSize);
					if(fabs(length) > texBest) texBest = length;
				}
      		}

			tex=texBest;
		}
	}

	// Notify the patch that it control points got changed
	controlPointsChanged();
}

void Patch::AccumulateBBox()
{
  m_aabb_local = AABB();

  for(PatchControlArray::iterator i = m_ctrlTransformed.begin(); i != m_ctrlTransformed.end(); ++i)
  {
    m_aabb_local.includePoint(i->m_vertex);
  }

  m_boundsChanged();
  m_lightsChanged();
}

// Inserts two columns before and after the column having the index <colIndex>
void Patch::insertColumns(std::size_t colIndex) {
	if (colIndex == 0 || colIndex == m_width) {
		throw GenericPatchException("Patch::insertColumns: can't insert at this index."); 
	}
	
	if (m_width + 2 > MAX_PATCH_WIDTH) {
		throw GenericPatchException("Patch::insertColumns: patch has too many columns.");
	}
	
	// Create a backup of the old control vertices
	PatchControlArray oldCtrl = m_ctrl;
	std::size_t oldHeight = m_height;
	std::size_t oldWidth = m_width;
	
	// Resize this patch
	setDims(oldWidth + 2, oldHeight);
	
	// Now fill in the control vertex values and interpolate
	// before and after the insert point.
	for (std::size_t row = 0; row < m_height; row++) {
		
		for (std::size_t newCol = 0, oldCol = 0; 
			 newCol < m_width && oldCol < oldWidth; 
			 newCol++, oldCol++)
		{
			// Is this the insert point?
			if (oldCol == colIndex) {
				// Left column (to be interpolated)
				ctrlAt(row, newCol).m_vertex = float_mid(
					oldCtrl[row*oldWidth + oldCol - 1].m_vertex,
					oldCtrl[row*oldWidth + oldCol].m_vertex
				);
				ctrlAt(row, newCol).m_texcoord = float_mid(
					oldCtrl[row*oldWidth + oldCol - 1].m_texcoord,
					oldCtrl[row*oldWidth + oldCol].m_texcoord
				);
				
				// Set the newCol counter to the middle column
				newCol++;
				ctrlAt(row, newCol).m_vertex = oldCtrl[row*oldWidth + oldCol].m_vertex;		
				ctrlAt(row, newCol).m_texcoord = oldCtrl[row*oldWidth + oldCol].m_texcoord;
				
				// Set newCol to the right column (to be interpolated)
				newCol++;
				ctrlAt(row, newCol).m_vertex = float_mid(
					oldCtrl[row*oldWidth + oldCol].m_vertex,
					oldCtrl[row*oldWidth + oldCol + 1].m_vertex
				);
				ctrlAt(row, newCol).m_texcoord = float_mid(
					oldCtrl[row*oldWidth + oldCol].m_texcoord,
					oldCtrl[row*oldWidth + oldCol + 1].m_texcoord
				);
			}
			else {
				// No special column, just copy the control vertex
				ctrlAt(row, newCol).m_vertex = oldCtrl[row*oldWidth + oldCol].m_vertex;		
				ctrlAt(row, newCol).m_texcoord = oldCtrl[row*oldWidth + oldCol].m_texcoord;
			}
		}
	} 
}

// Inserts two rows before and after the column having the index <colIndex>
void Patch::insertRows(std::size_t rowIndex) {
	if (rowIndex == 0 || rowIndex == m_height) {
		throw GenericPatchException("Patch::insertRows: can't insert at this index."); 
	}
	
	if (m_height + 2 > MAX_PATCH_HEIGHT) {
		throw GenericPatchException("Patch::insertRows: patch has too many rows.");
	}
	
	// Create a backup of the old control vertices
	PatchControlArray oldCtrl = m_ctrl;
	std::size_t oldHeight = m_height;
	std::size_t oldWidth = m_width;
	
	// Resize this patch
	setDims(oldWidth, oldHeight + 2);
	
	// Now fill in the control vertex values and interpolate
	// before and after the insert point.
	for (std::size_t col = 0; col < m_width; col++) {
		
		for (std::size_t newRow = 0, oldRow = 0; 
			 newRow < m_height && oldRow < oldHeight; 
			 newRow++, oldRow++)
		{
			// Is this the insert point?
			if (oldRow == rowIndex) {
				// the column above the insert point (to be interpolated)
				ctrlAt(newRow, col).m_vertex = float_mid(
					oldCtrl[(oldRow-1)*oldWidth + col].m_vertex,
					oldCtrl[oldRow*oldWidth + col].m_vertex
				);
				ctrlAt(newRow, col).m_texcoord = float_mid(
					oldCtrl[(oldRow-1)*oldWidth + col].m_texcoord,
					oldCtrl[oldRow*oldWidth + col].m_texcoord
				);
				
				// Set the newRow counter to the middle row
				newRow++;
				ctrlAt(newRow, col).m_vertex = oldCtrl[oldRow*oldWidth + col].m_vertex;		
				ctrlAt(newRow, col).m_texcoord = oldCtrl[oldRow*oldWidth + col].m_texcoord;
				
				// Set newRow to the lower column (to be interpolated)
				newRow++;
				ctrlAt(newRow, col).m_vertex = float_mid(
					oldCtrl[oldRow*oldWidth + col].m_vertex,
					oldCtrl[(oldRow+1)*oldWidth + col].m_vertex
				);
				ctrlAt(newRow, col).m_texcoord = float_mid(
					oldCtrl[oldRow*oldWidth + col].m_texcoord,
					oldCtrl[(oldRow+1)*oldWidth + col].m_texcoord
				);
			}
			else {
				// No special column, just copy the control vertex
				ctrlAt(newRow, col).m_vertex = oldCtrl[oldRow*oldWidth + col].m_vertex;		
				ctrlAt(newRow, col).m_texcoord = oldCtrl[oldRow*oldWidth + col].m_texcoord;
			}
		}
	} 
}

// Removes the two rows before and after the column/row having the index <index>
void Patch::removePoints(bool columns, std::size_t index) {
	bool rows = !columns; // readability shortcut ;)
	
	if (columns && m_width<5 || !columns && m_height < 5) {
		throw GenericPatchException("Patch::removePoints: can't remove any more rows/columns.");
	}
	
	// Check column index bounds
	if (columns && (index < 2 || index > m_width - 3)) {
		throw GenericPatchException("Patch::removePoints: can't remove columns at this index.");
	}
	
	// Check row index bounds
	if (rows && (index < 2 || index > m_height - 3)) {
		throw GenericPatchException("Patch::removePoints: can't remove rows at this index.");
	} 
	
	// Create a backup of the old control vertices
	PatchControlArray oldCtrl = m_ctrl;
	std::size_t oldHeight = m_height;
	std::size_t oldWidth = m_width;
	
	// Resize this patch
	setDims(columns ? oldWidth - 2 : oldWidth, rows ? oldHeight - 2 : oldHeight);
	
	// Now fill in the control vertex values and skip
	// the rows/cols before and after the remove point.
	for (std::size_t newRow = 0, oldRow = 0; 
		 newRow < m_height && oldRow < oldHeight; 
		 newRow++, oldRow++)
	{
		// Skip the row before and after the removal point
		if (rows && (oldRow == index - 1 || oldRow == index + 1)) {
			// Increase the old row pointer by 1
			oldRow++;
		}
		
		for (std::size_t oldCol = 0, newCol = 0; 
			 oldCol < oldWidth && newCol < m_width; 
			 oldCol++, newCol++) 
		{
			// Skip the column before and after the removal point
			if (columns && (oldCol == index - 1 || oldCol == index + 1)) {
				// Increase the old row pointer by 1
				oldCol++;
			}
			
			// Copy the control vertex from the old patch to the new patch
			ctrlAt(newRow, newCol).m_vertex = oldCtrl[oldRow*oldWidth + oldCol].m_vertex;		
			ctrlAt(newRow, newCol).m_texcoord = oldCtrl[oldRow*oldWidth + oldCol].m_texcoord;
		}
	}
}

void Patch::ConstructSeam(EPatchCap eType, Vector3* p, std::size_t width)
{
  switch(eType)
  {
  case eCapIBevel:
    {
      setDims(3, 3);
      m_ctrl[0].m_vertex = p[0];
      m_ctrl[1].m_vertex = p[1];
      m_ctrl[2].m_vertex = p[1];
      m_ctrl[3].m_vertex = p[1];
      m_ctrl[4].m_vertex = p[1];
      m_ctrl[5].m_vertex = p[1];
      m_ctrl[6].m_vertex = p[2];
      m_ctrl[7].m_vertex = p[1];
      m_ctrl[8].m_vertex = p[1];
    }
    break;
  case eCapBevel:
    {
      setDims(3, 3);
      Vector3 p3(p[2] + (p[0] - p[1]));
      m_ctrl[0].m_vertex = p3;
      m_ctrl[1].m_vertex = p3;
      m_ctrl[2].m_vertex = p[2];
      m_ctrl[3].m_vertex = p3;
      m_ctrl[4].m_vertex = p3;
      m_ctrl[5].m_vertex = p[1];
      m_ctrl[6].m_vertex = p3;
      m_ctrl[7].m_vertex = p3;
      m_ctrl[8].m_vertex = p[0];
    }
    break;
  case eCapEndCap:
    {
      Vector3 p5(vector3_mid(p[0], p[4]));

      setDims(3, 3);
      m_ctrl[0].m_vertex = p[0];
      m_ctrl[1].m_vertex = p5;
      m_ctrl[2].m_vertex = p[4];
      m_ctrl[3].m_vertex = p[1];
      m_ctrl[4].m_vertex = p[2];
      m_ctrl[5].m_vertex = p[3];
      m_ctrl[6].m_vertex = p[2];
      m_ctrl[7].m_vertex = p[2];
      m_ctrl[8].m_vertex = p[2];
    }
    break;
  case eCapIEndCap:
    {
      setDims(5, 3);
      m_ctrl[0].m_vertex = p[4];
      m_ctrl[1].m_vertex = p[3];
      m_ctrl[2].m_vertex = p[2];
      m_ctrl[3].m_vertex = p[1];
      m_ctrl[4].m_vertex = p[0];
      m_ctrl[5].m_vertex = p[3];
      m_ctrl[6].m_vertex = p[3];
      m_ctrl[7].m_vertex = p[2];
      m_ctrl[8].m_vertex = p[1];
      m_ctrl[9].m_vertex = p[1];
      m_ctrl[10].m_vertex = p[3];
      m_ctrl[11].m_vertex = p[3];
      m_ctrl[12].m_vertex = p[2];
      m_ctrl[13].m_vertex = p[1];
      m_ctrl[14].m_vertex = p[1];
    }
    break;
  case eCapCylinder:
    {
      std::size_t mid = (width - 1) >> 1;

      bool degenerate = (mid % 2) != 0;

      std::size_t newHeight = mid + (degenerate ? 2 : 1);

      setDims(3, newHeight);
 
      if(degenerate)
      {
        ++mid;
        for(std::size_t i = width; i != width + 2; ++i)
        {
          p[i] = p[width - 1];
        }
      }

      {
        PatchControl* pCtrl = m_ctrl.data();
        for(std::size_t i = 0; i != m_height; ++i, pCtrl += m_width)
        {
          pCtrl->m_vertex = p[i];
        }
      }
      {
        PatchControl* pCtrl = m_ctrl.data() + 2;
        std::size_t h = m_height - 1;
        for(std::size_t i = 0; i != m_height; ++i, pCtrl += m_width)
        {
          pCtrl->m_vertex = p[h + (h - i)];
        }
      }

      Redisperse(COL);
    }
    break;
  default:
    ERROR_MESSAGE("invalid patch-cap type");
    return;
  }
  CapTexture();
  controlPointsChanged();
}

// greebo: Calculates the nearest patch CORNER vertex from the given <point>
// Note: if this routine returns NULL, something's rotten with the patch
PatchControl* Patch::getClosestPatchControlToPoint(const Vector3& point) {
	
	PatchControl* pBest = NULL;
	
	// Initialise with an illegal distance value
	double closestDist = -1.0;
	
	PatchControl* corners[4];
	
	corners[0] = &m_ctrl[0];
	corners[1] = &m_ctrl[m_width-1];
	corners[2] = &m_ctrl[m_width*(m_height-1)];
	corners[3] = &m_ctrl[m_width*m_height - 1];	
	
	// Cycle through all the control points with an iterator
	//for (PatchControlIter i = m_ctrl.data(); i != m_ctrl.data() + m_ctrl.size(); ++i) {
	for (unsigned int i = 0; i < 4; i++) {
		
		// Calculate the distance of the current vertex
		double candidateDist = (corners[i]->m_vertex - point).getLength();
		
		// Compare the distance to the currently closest one
		if (candidateDist < closestDist || pBest == NULL) {
			// Store this distance as best value so far
			closestDist = candidateDist;
			
			// Store the pointer in <best>
			pBest = corners[i];
		}
	}
	
	return pBest;
}

/* greebo: This calculates the nearest patch control to the given brush <face>
 * 
 * @returns: a pointer to the nearest patch face. (Can technically be NULL, but really should not happen).*/
PatchControl* Patch::getClosestPatchControlToPatch(const Patch& patch) {
	
	// A pointer to the patch vertex closest to the patch
	PatchControl* pBest = NULL;
	// Initialise the best distance with an illegal value
	double closestDist = -1.0;
	
	// Cycle through the winding vertices and calculate the distance to each patch vertex
	for (PatchControlConstIter i = patch.begin(); i != patch.end(); i++) {
		// Retrieve the vertex
		Vector3 patchVertex = i->m_vertex;
		
		// Get the nearest control point to the current otherpatch vertex
		PatchControl* candidate = getClosestPatchControlToPoint(patchVertex);
		
		if (candidate != NULL) {
			
			double candidateDist = (patchVertex - candidate->m_vertex).getLength();
			
			// If we haven't found a best patch control so far or 
			// the candidate distance is even better, save it! 
			if (pBest == NULL || candidateDist < closestDist) {
				// Memorise this patch control
				pBest = candidate;
				closestDist =  candidateDist;
			}
		}
	} // end for<
	
	return pBest;
}

/* greebo: This calculates the nearest patch control to the given brush <face>
 * 
 * @returns: a pointer to the nearest patch face. (Can technically be NULL, but really should not happen).*/
PatchControl* Patch::getClosestPatchControlToFace(const Face* face) {
	
	// A pointer to the patch vertex closest to the face
	PatchControl* pBest = NULL;
	// Initialise the best distance with an illegal value
	double closestDist = -1.0;
	
	// Check for NULL pointer, just to make sure
	if (face != NULL) {
		// Retrieve the winding from the brush face
		const Winding& winding = face->getWinding();
		
		// Cycle through the winding vertices and calculate the distance to each patch vertex
		for (Winding::const_iterator i = winding.begin(); i != winding.end(); ++i) {
			// Retrieve the vertex
			Vector3 faceVertex = i->vertex;
			
			// Get the nearest control point to the current face vertex
			PatchControl* candidate = getClosestPatchControlToPoint(faceVertex);
			
			if (candidate != NULL) {
				
				double candidateDist = (faceVertex - candidate->m_vertex).getLength();
				
				// If we haven't found a best patch control so far or 
				// the candidate distance is even better, save it! 
				if (pBest == NULL || candidateDist < closestDist) {
					// Memorise this patch control
					pBest = candidate;
					closestDist =  candidateDist;
				}
			}
		} // end for
	}
	
	return pBest;
}

Vector2 Patch::getPatchControlArrayIndices(const PatchControl* control) {
	
	// Go through the patch column per column and find the control vertex
	PatchControl* pWidth = m_ctrl.data();
	
	for (std::size_t w=0; w < m_width; w++, pWidth++) {
		// Create another pointer and set it to the current pWidth pointer
		// then cycle through the height. This "scans" the patch column-wise.
		PatchControl* pHeight = pWidth;
		
		// Note the increment of m_width, which relocates the pointer by a whole row each step 
		for (std::size_t h=0; h<m_height; h++, pHeight+=m_width) {
			// Compare the pointers to check if we have found the control
			if (pHeight == control) {
				return Vector2(w,h);
			}
		}
	}
	
	return Vector2(0,0);
}

/* Project the vertex onto the given plane and transform it into the texture space using the worldToTexture matrix
 */
Vector2 getProjectedTextureCoords(const Vector3& vertex, const Plane3& plane, const Matrix4& worldToTexture) {
	// Project the patch vertex onto the brush face plane
	Vector3 projection = plane.getProjection(vertex);
	
	// Transform the projection coordinates into texture space
	Vector3 texcoord = worldToTexture.transform(projection).getVector3();
	
	// Return the texture coordinates
	return Vector2(texcoord[0], texcoord[1]);
}

/* greebo: This routine can be used to create seamless texture transitions from brushes to patches.
 * 
 * The idea is to flatten out the patch so that the distances between the patch control vertices
 * are preserved, but all of them lie flat in a plane. These points can then be projected onto
 * the brush faceplane and this way the texture coordinates can be easily calculated via the
 * world-to-texture-space transformations (the matrix is retrieved from the TexDef class).
 * 
 * The main problem that has to be tackled is not the "natural" texturing itself (the method
 * "natural" already takes care of that), but the goal that the patch/brush texture transition 
 * is seamless. 
 * 
 * The starting point of the "flattening" is the nearest control vertex of the patch (the closest
 * patch vertex to any of the brush winding vertices. Once this point has been found, the patch is
 * systematically flattened into a "virtual" patch plane. From there the points are projected into 
 * the texture space and you're already there.  
 * 
 * Note: This took me quite a bit and it's entirely possible that there is a more clever solution
 * to this, but for this weekend I'm done with this (and it works ;)). 
 * 
 * Note: The angle between patch and brush can also be 90 degrees, the algorithm catches this case
 * and calculates its own virtual patch directions. 
 */
void Patch::pasteTextureNatural(const Face* face) {
	// Check for NULL pointers
	if (face != NULL) {
		
		// Convert the size_t stuff into int, because we need it for signed comparisons
		int patchHeight = static_cast<int>(m_height);
		int patchWidth = static_cast<int>(m_width);
		
		// Get the plane and its normalised normal vector of the face
		Plane3 plane = face->getPlane().plane3().getNormalised();
		Vector3 faceNormal = plane.normal();
		
		// Get the conversion matrix from the FaceTextureDef, the local2World argument is the identity matrix
		Matrix4 worldToTexture = face->getTexdef().m_projection.getWorldToTexture(faceNormal, Matrix4::getIdentity());
		
		// Calculate the nearest corner vertex of this patch (to the face's winding vertices)
		PatchControl* nearestControl = getClosestPatchControlToFace(face);
		
		// Determine the control array indices of the nearest control vertex
		Vector2 indices = getPatchControlArrayIndices(nearestControl);
		
		// this is the point from where the patch is virtually flattened
		int wStart = static_cast<int>(indices.x());
		int hStart = static_cast<int>(indices.y());
		
		// Calculate the increments in the patch array, needed for the loops
		int wIncr = (wStart == patchWidth-1) ? -1 : 1;
		int wEnd = (wIncr<0) ? -1 : patchWidth;
		
		int hIncr = (hStart == patchHeight-1) ? -1 : 1;
		int hEnd = (hIncr<0) ? -1 : patchHeight;
		
		PatchControl* startControl = &m_ctrl[(patchWidth*hStart) + wStart];
		
		// Calculate the base directions that are used to "flatten" the patch
		// These have to be orthogonal to the facePlane normal, so that the texture coordinates
		// can be retrieved by projection onto the facePlane.
		
		// Get the control points of the next column and the next row
		PatchControl& nextColumn = m_ctrl[(patchWidth*(hStart + hIncr)) + wStart];
		PatchControl& nextRow = m_ctrl[(patchWidth*hStart) + (wStart + wIncr)];
		
		// Calculate the world direction of these control points and extract a base
		Vector3 widthVector = (nextRow.m_vertex - startControl->m_vertex);
		Vector3 heightVector = (nextColumn.m_vertex - startControl->m_vertex);

		if (widthVector.getLength() == 0.0f || heightVector.getLength() == 0.0f) {
			gtkutil::errorDialog("Sorry. Patch is not suitable for this kind of operation.",
								 MainFrame_getWindow());
			return;
		}
		
		// Save the undo memento
		undoSave();

		// Calculate the base vectors of the virtual plane the patch is flattened in		
		Vector3 widthBase, heightBase;
		getVirtualPatchBase(widthVector, heightVector, faceNormal, widthBase, heightBase);
		
		// Now cycle (systematically) through all the patch vertices, flatten them out by 
		// calculating the 3D distances of each vertex and projecting them onto the facePlane.
		
		// Initialise the starting point
		PatchControl* prevColumn = startControl; 
		Vector3 prevColumnVirtualVertex = prevColumn->m_vertex;
		
		for (int w = wStart; w != wEnd; w += wIncr) {
			
			// The first control in this row, calculate its virtual coords
			PatchControl* curColumn = &m_ctrl[(patchWidth*hStart) + w];
			
			// The distance between the last column and this column
			double xyzColDist = (curColumn->m_vertex - prevColumn->m_vertex).getLength();
			
			// The vector pointing to the next control point, if it *was* a completely planar patch
			Vector3 curColumnVirtualVertex = prevColumnVirtualVertex + widthBase * xyzColDist;

			// Store this value for the upcoming column cycle			
			PatchControl* prevRow = curColumn;
			Vector3 prevRowVirtualVertex = curColumnVirtualVertex;
			
			// Cycle through all the columns
			for (int h = hStart; h != hEnd; h += hIncr) {
				
				// The current control
				PatchControl* control = &m_ctrl[(patchWidth*h) + w];
				
				// The distance between the last and the current vertex
				double xyzRowDist = (control->m_vertex - prevRow->m_vertex).getLength();
			
				// The vector pointing to the next control point, if it *was* a completely planar patch
				Vector3 virtualControlVertex = prevRowVirtualVertex + heightBase * xyzRowDist;
				
				// Project the virtual vertex onto the brush faceplane and transform it into texture space 
				control->m_texcoord = getProjectedTextureCoords(virtualControlVertex, plane, worldToTexture);
				
				// Update the variables for the next loop
				prevRow = control;
				prevRowVirtualVertex = virtualControlVertex;
			}
			
			// Set the prevColumn control vertex to this one
			prevColumn = curColumn;
			prevColumnVirtualVertex = curColumnVirtualVertex;
		}
	}
	
	// Notify the patch about the change
	controlPointsChanged();
}

void Patch::pasteTextureNatural(Patch& sourcePatch) {
	// Save the undo memento
	undoSave();
	
	// Convert the size_t stuff into int, because we need it for signed comparisons
	int patchHeight = static_cast<int>(m_height);
	int patchWidth = static_cast<int>(m_width);
	
	// Calculate the nearest corner vertex of this patch (to the sourcepatch vertices)
	PatchControl* nearestControl = getClosestPatchControlToPatch(sourcePatch);
	
	PatchControl* refControl = sourcePatch.getClosestPatchControlToPatch(*this);
	
	Vector2 texDiff = refControl->m_texcoord - nearestControl->m_texcoord;
	
	for (int col = 0; col < patchWidth; col++) {
		for (int row = 0; row < patchHeight; row++) {
			// Substract the texture coord difference from each control vertex
			ctrlAt(row, col).m_texcoord += texDiff;
		}
	}
	
	// Notify the patch about the change
	controlPointsChanged();
}

void Patch::pasteTextureProjected(const Face* face) {
	// Save the undo memento
	undoSave();
	
	/* greebo: If there is a face pointer being passed to this method,
	 * the algorithm takes each vertex of the patch, projects it onto
	 * the plane (defined by the brush face) and transforms the coordinates
	 * into the texture space. */
	
	if (face != NULL) {
		// Get the normal vector of the face
		Plane3 plane = face->getPlane().plane3().getNormalised();
		
		// Get the (already normalised) facePlane normal
		Vector3 faceNormal = plane.normal();
		
		// Get the conversion matrix from the FaceTextureDef, the local2World argument is the identity matrix
		Matrix4 worldToTexture = face->getTexdef().m_projection.getWorldToTexture(faceNormal, Matrix4::getIdentity());
		
		// Cycle through all the control points with an iterator
		for (PatchControlIter i = m_ctrl.data(); i != m_ctrl.data() + m_ctrl.size(); ++i) {
			// Project the vertex onto the face plane and transform it into texture space
			i->m_texcoord = getProjectedTextureCoords(i->m_vertex, plane, worldToTexture);
		}
	
		// Notify the patch about the change
		controlPointsChanged();
	}
}

/* This clones the texture u/v coordinates from the <other> patch onto this one
 * Note: the patch dimensions must match exactly for this function to be performed.
 */
void Patch::pasteTextureCoordinates(const Patch* otherPatch) {
	undoSave();
	
	if (otherPatch != NULL) {
		
		if (otherPatch->getWidth() == m_width && otherPatch->getHeight() == m_height) {
			
			PatchControlConstIter other;
			PatchControlIter self;
			
			// Clone the texture coordinates one by one
			for (other = otherPatch->begin(), self = m_ctrl.data(); 
				 other != otherPatch->end(); 
				 ++other, ++self) 
			{
				self->m_texcoord = other->m_texcoord;
			}
			
			// Notify the patch about the change
			controlPointsChanged();
		}
		else {
			globalOutputStream() << "Error: Cannot copy texture coordinates, patch dimensions must match!\n";
		}
	}
}

/* greebo: This gets called when clicking on the "CAP" button in the surface dialogs.
 * 
 * The texture is projected onto either the <x,y>, <y,z>, or <x,z> planes and the result
 * is not stretched in any direction, as the pure components of the vector are translated
 * into texture coordinates. Points on the x-axis that are near to each other get
 * texture coordinates that are "near" to each other.
 * 
 * The argument nAxis can be 0,1,2 and determines, which components are taken to
 * calculate the texture coordinates.
 * 
 * Note: the default texture scale is used to calculate the texture coordinates.
 */
void Patch::ProjectTexture(int nAxis) {
	// Save the undo memento
	undoSave();

	// Hold the component index of the vector (e.g. 0,1 = x,y or 1,2 = y,z)
	int s, t;
  
	switch (nAxis) {
		case 2:		// Projection onto <x,y> plane
			s = 0;
			t = 1;
		break;
		case 0:		// Projection onto <y,z> plane
			s = 1;
			t = 2;
		break;
		case 1:		// Projection onto <x,z> plane
			s = 0;
			t = 2;
		break;
		default:
			ERROR_MESSAGE("invalid axis");
		return;
	}

	// Retrieve the default scale from the registry
	float defaultScale = GlobalRegistry().getFloat("user/ui/defaultTextureScale");

	/* Calculate the conversion factor between world and texture coordinates
	 * by using the image width/height.*/
	float fWidth = 1 / (m_state->getTexture().width * defaultScale);
	float fHeight = 1 / (m_state->getTexture().height * -defaultScale);

	// Cycle through all the control points with an iterator
	for (PatchControlIter i = m_ctrl.data(); i != m_ctrl.data() + m_ctrl.size(); ++i) {
		// Take the according value (e.g. s = x, t = y, depending on the nAxis argument) 
		// and apply the appropriate texture coordinate 
		i->m_texcoord[0] = i->m_vertex[s] * fWidth;
		i->m_texcoord[1] = i->m_vertex[t] * fHeight;
	}

	// Notify the patch about the change
	controlPointsChanged();
}

PatchTesselation& Patch::getTesselation() {
	return m_tess;
}

void Patch::constructPlane(const AABB& aabb, int axis, std::size_t width, std::size_t height)
{
  setDims(width, height);

  int x, y, z;
  switch(axis)
  {
  case 2: x=0; y=1; z=2; break;
  case 1: x=0; y=2; z=1; break;
  case 0: x=1; y=2; z=0; break;
  default:
    ERROR_MESSAGE("invalid view-type");
    return;
  }
  
  if(m_width < MIN_PATCH_WIDTH || m_width > MAX_PATCH_WIDTH) m_width = 3;
  if(m_height < MIN_PATCH_HEIGHT || m_height > MAX_PATCH_HEIGHT) m_height = 3;
  
  Vector3 vStart;
  vStart[x] = aabb.origin[x] - aabb.extents[x];
  vStart[y] = aabb.origin[y] - aabb.extents[y];
  vStart[z] = aabb.origin[z];
  
  float xAdj = fabsf((vStart[x] - (aabb.origin[x] + aabb.extents[x])) / (float)(m_width - 1));
  float yAdj = fabsf((vStart[y] - (aabb.origin[y] + aabb.extents[y])) / (float)(m_height - 1));

  Vector3 vTmp;
  vTmp[z] = vStart[z];
  PatchControl* pCtrl = m_ctrl.data();

  vTmp[y]=vStart[y];
  for (std::size_t h=0; h<m_height; h++)
  {
    vTmp[x]=vStart[x];
    for (std::size_t w=0; w<m_width; w++, ++pCtrl)
    {
      pCtrl->m_vertex = vTmp;
      vTmp[x]+=xAdj;
    }
    vTmp[y]+=yAdj;
  }

  NaturalTexture();
}

void Patch::ConstructPrefab(const AABB& aabb, EPatchPrefab eType, int axis, std::size_t width, std::size_t height)
{
  Vector3 vPos[3];
    
  if(eType != ePlane)
  {
    vPos[0] = aabb.origin - aabb.extents;
    vPos[1] = aabb.origin;
    vPos[2] = aabb.origin + aabb.extents;
  }
  
  if(eType == ePlane)
  {
    constructPlane(aabb, axis, width, height);
  }
  else if(eType == eSqCylinder
    || eType == eCylinder
    || eType == eDenseCylinder
    || eType == eVeryDenseCylinder
    || eType == eCone
    || eType == eSphere)
  {
    unsigned char *pIndex;
    unsigned char pCylIndex[] =
    {
      0, 0,
      1, 0,
      2, 0,
      2, 1,
      2, 2,
      1, 2,
      0, 2,
      0, 1,
      0, 0
    };

    
    PatchControl *pStart;
    switch(eType)
    {
    case eSqCylinder: setDims(9, 3);
      pStart = m_ctrl.data();
      break;
    case eDenseCylinder: 
    case eVeryDenseCylinder: 
    case eCylinder:
      setDims(9, 3);
      pStart = m_ctrl.data() + 1;
      break;
    case eCone: setDims(9, 3);
      pStart = m_ctrl.data() + 1;
      break;
    case eSphere:
      setDims(9, 5);
      pStart = m_ctrl.data() + (9+1);
      break;
    default:
      ERROR_MESSAGE("this should be unreachable");
      return;
    }

    for(std::size_t h=0; h<3; h++, pStart+=9)
    {
      pIndex = pCylIndex;
      PatchControl* pCtrl = pStart;
      for(std::size_t w=0; w<8; w++, pCtrl++)
      {
        pCtrl->m_vertex[0] = vPos[pIndex[0]][0];
        pCtrl->m_vertex[1] = vPos[pIndex[1]][1];
        pCtrl->m_vertex[2] = vPos[h][2];
        pIndex+=2;
      }
    }

    switch(eType)
    {
    case eSqCylinder:
      {
        PatchControl* pCtrl=m_ctrl.data();
        for(std::size_t h=0; h<3; h++, pCtrl+=9)
        {
          pCtrl[8].m_vertex = pCtrl[0].m_vertex;
        }
      }
      break;
    case eDenseCylinder:
    case eVeryDenseCylinder:
    case eCylinder:
      {
        PatchControl* pCtrl=m_ctrl.data();
        for (std::size_t h=0; h<3; h++, pCtrl+=9)
        {
          pCtrl[0].m_vertex = pCtrl[8].m_vertex;
        }
      }
      break;
    case eCone:
      {
        PatchControl* pCtrl=m_ctrl.data();
        for (std::size_t h=0; h<2; h++, pCtrl+=9)
        {
          pCtrl[0].m_vertex = pCtrl[8].m_vertex;
        }
      }
      {
        PatchControl* pCtrl=m_ctrl.data()+9*2;
        for (std::size_t w=0; w<9; w++, pCtrl++)
        {
          pCtrl->m_vertex[0] = vPos[1][0];
          pCtrl->m_vertex[1] = vPos[1][1];
          pCtrl->m_vertex[2] = vPos[2][2];
        }
      }
      break;
    case eSphere:
      {
        PatchControl* pCtrl=m_ctrl.data()+9;
        for (std::size_t h=0; h<3; h++, pCtrl+=9)
        {
          pCtrl[0].m_vertex = pCtrl[8].m_vertex;
        }
      }
      {
        PatchControl* pCtrl = m_ctrl.data();
        for (std::size_t w=0; w<9; w++, pCtrl++)
        {
          pCtrl->m_vertex[0] = vPos[1][0];
          pCtrl->m_vertex[1] = vPos[1][1];
          pCtrl->m_vertex[2] = vPos[2][2];
        }
      }
      {
        PatchControl* pCtrl = m_ctrl.data()+(9*4);
        for (std::size_t w=0; w<9; w++, pCtrl++)
        {
          pCtrl->m_vertex[0] = vPos[1][0];
          pCtrl->m_vertex[1] = vPos[1][1];
          pCtrl->m_vertex[2] = vPos[2][2];
        }
      }
    default:
      ERROR_MESSAGE("this should be unreachable");
      return;
    }
  }
  else if  (eType == eBevel)
  {
    unsigned char *pIndex;
    unsigned char pBevIndex[] =
    {
      0, 0,
      2, 0,
      2, 2,
    };

    setDims(3, 3);

    PatchControl* pCtrl = m_ctrl.data();
    for(std::size_t h=0; h<3; h++)
    {
      pIndex=pBevIndex;
      for(std::size_t w=0; w<3; w++, pIndex+=2, pCtrl++)
      {
        pCtrl->m_vertex[0] = vPos[pIndex[0]][0];
        pCtrl->m_vertex[1] = vPos[pIndex[1]][1];
        pCtrl->m_vertex[2] = vPos[h][2];
      }
    }
  }
  else if(eType == eEndCap)
  {
    unsigned char *pIndex;
    unsigned char pEndIndex[] =
    {
      2, 0,
      2, 2,
      1, 2,
      0, 2,
      0, 0,
    };

    setDims(5, 3);

    PatchControl* pCtrl = m_ctrl.data();
    for(std::size_t h=0; h<3; h++)
    {
      pIndex=pEndIndex;
      for(std::size_t w=0; w<5; w++, pIndex+=2, pCtrl++)
      {
        pCtrl->m_vertex[0] = vPos[pIndex[0]][0];
        pCtrl->m_vertex[1] = vPos[pIndex[1]][1];
        pCtrl->m_vertex[2] = vPos[h][2];
      }
    }
  }

  if(eType == eDenseCylinder)
  {
    InsertRemove(true, false, true);
  }

  if(eType == eVeryDenseCylinder)
  {
    InsertRemove(true, false, false);
    InsertRemove(true, false, true);
  }

  NaturalTexture();
}

void Patch::RenderDebug(RenderStateFlags state) const
{
  for (std::size_t i = 0; i<m_tess.m_numStrips; i++)
  {
    glBegin(GL_QUAD_STRIP);
    for (std::size_t j = 0; j<m_tess.m_lenStrips; j++)
    {
      glNormal3dv((m_tess.m_vertices.data() + m_tess.m_indices[i*m_tess.m_lenStrips+j])->normal);
      glTexCoord2dv((m_tess.m_vertices.data() + m_tess.m_indices[i*m_tess.m_lenStrips+j])->texcoord);
      glVertex3dv((m_tess.m_vertices.data() + m_tess.m_indices[i*m_tess.m_lenStrips+j])->vertex);
    }
    glEnd();
  }
}

void RenderablePatchSolid::RenderNormals() const
{
  const std::size_t width = m_tess.m_numStrips+1;
  const std::size_t height = m_tess.m_lenStrips>>1;
  glBegin(GL_LINES);
  for(std::size_t i=0;i<width;i++)
  {
    for(std::size_t j=0;j<height;j++)
    {
      {
        Vector3 vNormal(
            ((m_tess.m_vertices.data() + (j*width+i))->vertex) +
			((m_tess.m_vertices.data() + (j*width+i))->normal).getNormalised() * 8
        );
        glVertex3dv((m_tess.m_vertices.data() + (j*width+i))->vertex);
        glVertex3dv(vNormal);
      }
      {
        Vector3 vNormal(          
            ((m_tess.m_vertices.data() + (j*width+i))->vertex) +
            ((m_tess.m_vertices.data() + (j*width+i))->tangent).getNormalised() * 8
        );
        glVertex3dv((m_tess.m_vertices.data() + (j*width+i))->vertex);
        glVertex3dv(vNormal);
      }
      {
        Vector3 vNormal(
            ((m_tess.m_vertices.data() + (j*width+i))->vertex) +
            ((m_tess.m_vertices.data() + (j*width+i))->bitangent).getNormalised() * 8
        );
        glVertex3dv((m_tess.m_vertices.data() + (j*width+i))->vertex);
        glVertex3dv(vNormal);
      }
    }
  }
  glEnd();
}

#define DEGEN_0a  0x01
#define DEGEN_1a  0x02
#define DEGEN_2a  0x04
#define DEGEN_0b  0x08
#define DEGEN_1b  0x10
#define DEGEN_2b  0x20
#define SPLIT     0x40
#define AVERAGE   0x80


unsigned int subarray_get_degen(PatchControlIter subarray, std::size_t strideU, std::size_t strideV)
{
  unsigned int nDegen = 0;
  const PatchControl* p1;
  const PatchControl* p2;

  p1 = subarray;
  p2 = p1 + strideU;
  if(p1->m_vertex == p2->m_vertex)
    nDegen |= DEGEN_0a;
  p1 = p2;
  p2 = p1 + strideU;
  if(p1->m_vertex == p2->m_vertex)
    nDegen |= DEGEN_0b;

  p1 = subarray + strideV;
  p2 = p1 + strideU;
  if(p1->m_vertex == p2->m_vertex)
    nDegen |= DEGEN_1a;
  p1 = p2;
  p2 = p1 + strideU;
  if(p1->m_vertex == p2->m_vertex)
    nDegen |= DEGEN_1b;

  p1 = subarray + (strideV << 1);
  p2 = p1 + strideU;
  if(p1->m_vertex == p2->m_vertex)
    nDegen |= DEGEN_2a;
  p1 = p2;
  p2 = p1 + strideU;
  if(p1->m_vertex == p2->m_vertex)
    nDegen |= DEGEN_2b;

  return nDegen;
}


inline void deCasteljau3(const Vector3& P0, const Vector3& P1, const Vector3& P2, Vector3& P01, Vector3& P12, Vector3& P012)
{
  P01 = vector3_mid(P0, P1);
  P12 = vector3_mid(P1, P2);
  P012 = vector3_mid(P01, P12);
}

inline void BezierInterpolate3( const Vector3& start, Vector3& left, Vector3& mid, Vector3& right, const Vector3& end )
{
  left = vector3_mid(start, mid);
  right = vector3_mid(mid, end);
  mid = vector3_mid(left, right);
}

inline void BezierInterpolate2( const Vector2& start, Vector2& left, Vector2& mid, Vector2& right, const Vector2& end )
{
  left[0]= float_mid(start[0], mid[0]);
  left[1] = float_mid(start[1], mid[1]);
  right[0] = float_mid(mid[0], end[0]);
  right[1] = float_mid(mid[1], end[1]);
  mid[0] = float_mid(left[0], right[0]);
  mid[1] = float_mid(left[1], right[1]);
}


inline Vector2& texcoord_for_index(Array<ArbitraryMeshVertex>& vertices, std::size_t index)
{
  return reinterpret_cast<Vector2&>(vertices[index].texcoord);
}

inline Vector3& vertex_for_index(Array<ArbitraryMeshVertex>& vertices, std::size_t index)
{
  return reinterpret_cast<Vector3&>(vertices[index].vertex);
}

inline Vector3& normal_for_index(Array<ArbitraryMeshVertex>& vertices, std::size_t index)
{
  return reinterpret_cast<Vector3&>(vertices[index].normal);
}

inline Vector3& tangent_for_index(Array<ArbitraryMeshVertex>& vertices, std::size_t index)
{
  return reinterpret_cast<Vector3&>(vertices[index].tangent);
}

inline Vector3& bitangent_for_index(Array<ArbitraryMeshVertex>& vertices, std::size_t index)
{
  return reinterpret_cast<Vector3&>(vertices[index].bitangent);
}

inline const Vector2& texcoord_for_index(const Array<ArbitraryMeshVertex>& vertices, std::size_t index)
{
  return reinterpret_cast<const Vector2&>(vertices[index].texcoord);
}

inline const Vector3& vertex_for_index(const Array<ArbitraryMeshVertex>& vertices, std::size_t index)
{
  return reinterpret_cast<const Vector3&>(vertices[index].vertex);
}

inline const Vector3& normal_for_index(const Array<ArbitraryMeshVertex>& vertices, std::size_t index)
{
  return reinterpret_cast<const Vector3&>(vertices[index].normal);
}

inline const Vector3& tangent_for_index(const Array<ArbitraryMeshVertex>& vertices, std::size_t index)
{
  return reinterpret_cast<const Vector3&>(vertices[index].tangent);
}

inline const Vector3& bitangent_for_index(const Array<ArbitraryMeshVertex>& vertices, std::size_t index)
{
  return reinterpret_cast<const Vector3&>(vertices[index].bitangent);
}

#include "math/curve.h"

inline PatchControl QuadraticBezier_evaluate(const PatchControl* firstPoint, double t)
{
  PatchControl result = { Vector3(0, 0, 0), Vector2(0, 0) };
  double denominator = 0;

  {
    double weight = BernsteinPolynomial<Zero, Two>::apply(t);
    result.m_vertex += firstPoint[0].m_vertex * weight;
    result.m_texcoord += firstPoint[0].m_texcoord * weight;
    denominator += weight;
  }
  {
    double weight = BernsteinPolynomial<One, Two>::apply(t);
    result.m_vertex += firstPoint[1].m_vertex * weight;
    result.m_texcoord += firstPoint[1].m_texcoord * weight;
    denominator += weight;
  }
  {
    double weight = BernsteinPolynomial<Two, Two>::apply(t);
    result.m_vertex  += firstPoint[2].m_vertex * weight;
    result.m_texcoord += firstPoint[2].m_texcoord * weight;
    denominator += weight;
  }

  result.m_vertex /= denominator;
  result.m_texcoord /= denominator;
  return result;
}

inline Vector3 vector3_linear_interpolated(const Vector3& a, const Vector3& b, double t)
{
  return a*(1.0 - t) + b*t;
}

inline Vector2 vector2_linear_interpolated(const Vector2& a, const Vector2& b, double t)
{
  return a*(1.0 - t) + b*t;
}

void normalise_safe(Vector3& normal)
{
  if(normal != g_vector3_identity)
  {
    vector3_normalise(normal);
  }
}

inline void QuadraticBezier_evaluate(const PatchControl& a, const PatchControl& b, const PatchControl& c, double t, PatchControl& point, PatchControl& left, PatchControl& right)
{
  left.m_vertex = vector3_linear_interpolated(a.m_vertex, b.m_vertex, t);
  left.m_texcoord = vector2_linear_interpolated(a.m_texcoord, b.m_texcoord, t);
  right.m_vertex = vector3_linear_interpolated(b.m_vertex, c.m_vertex, t);
  right.m_texcoord = vector2_linear_interpolated(b.m_texcoord, c.m_texcoord, t);
  point.m_vertex = vector3_linear_interpolated(left.m_vertex, right.m_vertex, t);
  point.m_texcoord = vector2_linear_interpolated(left.m_texcoord, right.m_texcoord, t);
}

void Patch::TesselateSubMatrixFixed(ArbitraryMeshVertex* vertices, std::size_t strideX, std::size_t strideY, unsigned int nFlagsX, unsigned int nFlagsY, PatchControl* subMatrix[3][3])
{
  double incrementU = 1.0 / m_subdivisions_x;
  double incrementV = 1.0 / m_subdivisions_y;
  const std::size_t width = m_subdivisions_x + 1;
  const std::size_t height = m_subdivisions_y + 1;

  for(std::size_t i = 0; i != width; ++i)
  {
    double tU = (i + 1 == width) ? 1 : i * incrementU;
    PatchControl pointX[3];
    PatchControl leftX[3];
    PatchControl rightX[3];
    QuadraticBezier_evaluate(*subMatrix[0][0], *subMatrix[0][1], *subMatrix[0][2], tU, pointX[0], leftX[0], rightX[0]);
    QuadraticBezier_evaluate(*subMatrix[1][0], *subMatrix[1][1], *subMatrix[1][2], tU, pointX[1], leftX[1], rightX[1]);
    QuadraticBezier_evaluate(*subMatrix[2][0], *subMatrix[2][1], *subMatrix[2][2], tU, pointX[2], leftX[2], rightX[2]);

    ArbitraryMeshVertex* p = vertices + i * strideX;
    for(std::size_t j = 0; j != height; ++j)
    {
      if((j == 0 || j + 1 == height) && (i == 0 || i + 1 == width))
      {
      }
      else
      {
        double tV = (j + 1 == height) ? 1 : j * incrementV;

        PatchControl pointY[3];
        PatchControl leftY[3];
        PatchControl rightY[3];
        QuadraticBezier_evaluate(*subMatrix[0][0], *subMatrix[1][0], *subMatrix[2][0], tV, pointY[0], leftY[0], rightY[0]);
        QuadraticBezier_evaluate(*subMatrix[0][1], *subMatrix[1][1], *subMatrix[2][1], tV, pointY[1], leftY[1], rightY[1]);
        QuadraticBezier_evaluate(*subMatrix[0][2], *subMatrix[1][2], *subMatrix[2][2], tV, pointY[2], leftY[2], rightY[2]);

        PatchControl point;
        PatchControl left;
        PatchControl right;
        QuadraticBezier_evaluate(pointX[0], pointX[1], pointX[2], tV, point, left, right);
        PatchControl up;
        PatchControl down;
        QuadraticBezier_evaluate(pointY[0], pointY[1], pointY[2], tU, point, up, down);

        p->vertex = Vertex3f(point.m_vertex);
        p->texcoord = point.m_texcoord;

        ArbitraryMeshVertex a, b, c;

        a.vertex = Vertex3f(left.m_vertex);
        a.texcoord = left.m_texcoord;
        b.vertex = Vertex3f(right.m_vertex);
        b.texcoord = right.m_texcoord;

        if(i != 0)
        {
          c.vertex = Vertex3f(up.m_vertex);
          c.texcoord = up.m_texcoord;
        }
        else
        {
          c.vertex = Vertex3f(down.m_vertex);
          c.texcoord = down.m_texcoord;
        }

        Vector3 normal = (right.m_vertex - left.m_vertex).crossProduct(up.m_vertex - down.m_vertex).getNormalised();

        Vector3 tangent, bitangent;
        ArbitraryMeshTriangle_calcTangents(a, b, c, tangent, bitangent);
        tangent = tangent.getNormalised();
        bitangent = bitangent.getNormalised();
       
        if(((nFlagsX & AVERAGE) != 0 && i == 0) || ((nFlagsY & AVERAGE) != 0  && j == 0))
        {
          p->normal = Normal3f(p->normal + normal.getNormalised());
          p->tangent = Normal3f(p->tangent + tangent.getNormalised());
          p->bitangent = Normal3f((p->bitangent + bitangent).getNormalised());
        }
        else
        {
          p->normal = Normal3f(normal);
          p->tangent = Normal3f(tangent);
          p->bitangent = Normal3f(bitangent);
        }
      }

      p += strideY;
    }
  }
}

void Patch::TesselateSubMatrix( const BezierCurveTree *BX, const BezierCurveTree *BY,
                                        std::size_t offStartX, std::size_t offStartY,
                                        std::size_t offEndX, std::size_t offEndY,
                                        std::size_t nFlagsX, std::size_t nFlagsY,
                                        Vector3& left, Vector3& mid, Vector3& right,
                                        Vector2& texLeft, Vector2& texMid, Vector2& texRight,
                                        bool bTranspose )
{
  int newFlagsX, newFlagsY;

  Vector3 tmp;
  Vector3 vertex_0_0, vertex_0_1, vertex_1_0, vertex_1_1, vertex_2_0, vertex_2_1;
  Vector2 texTmp;
  Vector2 texcoord_0_0, texcoord_0_1, texcoord_1_0, texcoord_1_1, texcoord_2_0, texcoord_2_1;

  {
   // texcoords

    BezierInterpolate2( texcoord_for_index(m_tess.m_vertices, offStartX + offStartY),
                     texcoord_0_0,
                     texcoord_for_index(m_tess.m_vertices, BX->index + offStartY),
                     texcoord_0_1,
                     texcoord_for_index(m_tess.m_vertices, offEndX + offStartY) );


    BezierInterpolate2( texcoord_for_index(m_tess.m_vertices, offStartX + offEndY),
                     texcoord_2_0,
                     texcoord_for_index(m_tess.m_vertices, BX->index + offEndY),
                     texcoord_2_1,
                     texcoord_for_index(m_tess.m_vertices, offEndX + offEndY) );

    texTmp = texMid;

    BezierInterpolate2(texLeft,
                      texcoord_1_0,
                      texTmp,
                      texcoord_1_1,
                      texRight);

    if(!BezierCurveTree_isLeaf(BY))
    {
      texcoord_for_index(m_tess.m_vertices, BX->index + BY->index) = texTmp;
    }

  
    if(!BezierCurveTree_isLeaf(BX->left))
    {
      texcoord_for_index(m_tess.m_vertices, BX->left->index + offStartY) = texcoord_0_0;
      texcoord_for_index(m_tess.m_vertices, BX->left->index + offEndY) = texcoord_2_0;

      if(!BezierCurveTree_isLeaf(BY))
      {
        texcoord_for_index(m_tess.m_vertices, BX->left->index + BY->index) = texcoord_1_0;
      }
    }
    if(!BezierCurveTree_isLeaf(BX->right))
    {
      texcoord_for_index(m_tess.m_vertices, BX->right->index + offStartY) = texcoord_0_1;
      texcoord_for_index(m_tess.m_vertices, BX->right->index + offEndY) = texcoord_2_1;

      if(!BezierCurveTree_isLeaf(BY))
      {
        texcoord_for_index(m_tess.m_vertices, BX->right->index + BY->index) = texcoord_1_1;
      }
    }


    // verts

    BezierInterpolate3( vertex_for_index(m_tess.m_vertices, offStartX + offStartY),
                     vertex_0_0,
                     vertex_for_index(m_tess.m_vertices, BX->index + offStartY),
                     vertex_0_1,
                     vertex_for_index(m_tess.m_vertices, offEndX + offStartY) );


    BezierInterpolate3( vertex_for_index(m_tess.m_vertices, offStartX + offEndY),
                     vertex_2_0,
                     vertex_for_index(m_tess.m_vertices, BX->index + offEndY),
                     vertex_2_1,
                     vertex_for_index(m_tess.m_vertices, offEndX + offEndY) );


    tmp = mid;

    BezierInterpolate3( left,
                     vertex_1_0,
                     tmp,
                     vertex_1_1,
                     right );

    if(!BezierCurveTree_isLeaf(BY))
    {
      vertex_for_index(m_tess.m_vertices, BX->index + BY->index) = tmp;
    }

  
    if(!BezierCurveTree_isLeaf(BX->left))
    {
      vertex_for_index(m_tess.m_vertices, BX->left->index + offStartY) = vertex_0_0;
      vertex_for_index(m_tess.m_vertices, BX->left->index + offEndY) = vertex_2_0;

      if(!BezierCurveTree_isLeaf(BY))
      {
        vertex_for_index(m_tess.m_vertices, BX->left->index + BY->index) = vertex_1_0;
      }
    }
    if(!BezierCurveTree_isLeaf(BX->right))
    {
      vertex_for_index(m_tess.m_vertices, BX->right->index + offStartY) = vertex_0_1;
      vertex_for_index(m_tess.m_vertices, BX->right->index + offEndY) = vertex_2_1;

      if(!BezierCurveTree_isLeaf(BY))
      {
        vertex_for_index(m_tess.m_vertices, BX->right->index + BY->index) = vertex_1_1;
      }
    }

    // normals

    if(nFlagsX & SPLIT)
    {
      ArbitraryMeshVertex a, b, c;
      Vector3 tangentU;
 
      if(!(nFlagsX & DEGEN_0a) || !(nFlagsX & DEGEN_0b))
      {
        tangentU = vertex_0_1 - vertex_0_0;
        a.vertex = Vertex3f(vertex_0_0);
        a.texcoord = texcoord_0_0;
        c.vertex = Vertex3f(vertex_0_1);
        c.texcoord = texcoord_0_1;
      }
      else if(!(nFlagsX & DEGEN_1a) || !(nFlagsX & DEGEN_1b))
      {
        tangentU = vertex_1_1 - vertex_1_0;
        a.vertex = Vertex3f(vertex_1_0);
        a.texcoord = texcoord_1_0;
        c.vertex = Vertex3f(vertex_1_1);
        c.texcoord = texcoord_1_1;
      }
      else
      {
        tangentU = vertex_2_1 - vertex_2_0;
        a.vertex = Vertex3f(vertex_2_0);
        a.texcoord = texcoord_2_0;
        c.vertex = Vertex3f(vertex_2_1);
        c.texcoord = texcoord_2_1;
      }

      Vector3 tangentV;

      if((nFlagsY & DEGEN_0a) && (nFlagsY & DEGEN_1a) && (nFlagsY & DEGEN_2a))
      {
        tangentV = vertex_for_index(m_tess.m_vertices, BX->index + offEndY) - tmp;
        b.vertex = Vertex3f(tmp);//m_tess.m_vertices[BX->index + offEndY].vertex;
        b.texcoord = texTmp;//m_tess.m_vertices[BX->index + offEndY].texcoord;
      }
      else
      {
        tangentV = tmp - vertex_for_index(m_tess.m_vertices, BX->index + offStartY);
        b.vertex = Vertex3f(tmp);//m_tess.m_vertices[BX->index + offStartY].vertex;
        b.texcoord = texTmp; //m_tess.m_vertices[BX->index + offStartY].texcoord;
      }
  

      Vector3 normal, s, t;
      ArbitraryMeshVertex& v = m_tess.m_vertices[offStartY + BX->index];
      Vector3& p = v.normal;
      Vector3& ps = v.tangent;
      Vector3& pt = v.bitangent;

      if(bTranspose)
      {
        normal = tangentV.crossProduct(tangentU);
      }
      else
      {
        normal = tangentU.crossProduct(tangentV);
      }
      normalise_safe(normal);

      ArbitraryMeshTriangle_calcTangents(a, b, c, s, t);
      normalise_safe(s);
      normalise_safe(t);

      if(nFlagsX & AVERAGE)
      {
        p = (p + normal).getNormalised();
        ps = (ps + s).getNormalised();
        pt = (pt + t).getNormalised();
      }
      else
      {
        p = normal;
        ps = s;
        pt = t;
      }
    }

    {
      ArbitraryMeshVertex a, b, c;
      Vector3 tangentU;

      if(!(nFlagsX & DEGEN_2a) || !(nFlagsX & DEGEN_2b))
      {
        tangentU = vertex_2_1 - vertex_2_0;
        a.vertex = Vertex3f(vertex_2_0);
        a.texcoord = texcoord_2_0;
        c.vertex = Vertex3f(vertex_2_1);
        c.texcoord = texcoord_2_1;
      }
      else if(!(nFlagsX & DEGEN_1a) || !(nFlagsX & DEGEN_1b))
      {
        tangentU = vertex_1_1 - vertex_1_0;
        a.vertex = Vertex3f(vertex_1_0);
        a.texcoord = texcoord_1_0;
        c.vertex = Vertex3f(vertex_1_1);
        c.texcoord = texcoord_1_1;
      }
      else
      {
        tangentU = vertex_0_1 - vertex_0_0;
        a.vertex = Vertex3f(vertex_0_0);
        a.texcoord = texcoord_0_0;
        c.vertex = Vertex3f(vertex_0_1);
        c.texcoord = texcoord_0_1;
      }

      Vector3 tangentV;

      if((nFlagsY & DEGEN_0b) && (nFlagsY & DEGEN_1b) && (nFlagsY & DEGEN_2b))
      {
        tangentV = tmp - vertex_for_index(m_tess.m_vertices, BX->index + offStartY);
        b.vertex = Vertex3f(tmp);//m_tess.m_vertices[BX->index + offStartY].vertex;
        b.texcoord = texTmp;//m_tess.m_vertices[BX->index + offStartY].texcoord;
      }
      else
      {
        tangentV = vertex_for_index(m_tess.m_vertices, BX->index + offEndY) - tmp;
        b.vertex = Vertex3f(tmp);//m_tess.m_vertices[BX->index + offEndY].vertex;
        b.texcoord = texTmp;//m_tess.m_vertices[BX->index + offEndY].texcoord;
      }

      ArbitraryMeshVertex& v = m_tess.m_vertices[offEndY+BX->index];
      Vector3& p = v.normal;
      Vector3& ps = v.tangent;
      Vector3& pt = v.bitangent;

      if(bTranspose)
      {
        p = tangentV.crossProduct(tangentU);
      }
      else
      {
        p = tangentU.crossProduct(tangentV);
      }
      normalise_safe(p);

      ArbitraryMeshTriangle_calcTangents(a, b, c, ps, pt);
      normalise_safe(ps);
      normalise_safe(pt);
    }
  }

  
  newFlagsX = newFlagsY = 0;

  if((nFlagsX & DEGEN_0a) && (nFlagsX & DEGEN_0b))
  {
    newFlagsX |= DEGEN_0a;
    newFlagsX |= DEGEN_0b;
  }
  if((nFlagsX & DEGEN_1a) && (nFlagsX & DEGEN_1b))
  {
    newFlagsX |= DEGEN_1a;
    newFlagsX |= DEGEN_1b;
  }
  if((nFlagsX & DEGEN_2a) && (nFlagsX & DEGEN_2b))
  {
    newFlagsX |= DEGEN_2a;
    newFlagsX |= DEGEN_2b;
  }
  if((nFlagsY & DEGEN_0a) && (nFlagsY & DEGEN_1a) && (nFlagsY & DEGEN_2a))
  {
    newFlagsY |= DEGEN_0a;
    newFlagsY |= DEGEN_1a;
    newFlagsY |= DEGEN_2a;
  }
  if((nFlagsY & DEGEN_0b) && (nFlagsY & DEGEN_1b) && (nFlagsY & DEGEN_2b))
  {
    newFlagsY |= DEGEN_0b;
    newFlagsY |= DEGEN_1b;
    newFlagsY |= DEGEN_2b;
  }

  
  //if((nFlagsX & DEGEN_0a) && (nFlagsX & DEGEN_1a) && (nFlagsX & DEGEN_2a)) { newFlagsX |= DEGEN_0a; newFlagsX |= DEGEN_1a; newFlagsX |= DEGEN_2a; }
  //if((nFlagsX & DEGEN_0b) && (nFlagsX & DEGEN_1b) && (nFlagsX & DEGEN_2b)) { newFlagsX |= DEGEN_0b; newFlagsX |= DEGEN_1b; newFlagsX |= DEGEN_2b; }
  
  newFlagsX |= (nFlagsX & SPLIT);
  newFlagsX |= (nFlagsX & AVERAGE);
      
  if(!BezierCurveTree_isLeaf(BY))
  {
    {
      int nTemp = newFlagsY;

      if((nFlagsY & DEGEN_0a) && (nFlagsY & DEGEN_0b))
      {
        newFlagsY |= DEGEN_0a;
        newFlagsY |= DEGEN_0b;
      }
      newFlagsY |= (nFlagsY & SPLIT);
      newFlagsY |= (nFlagsY & AVERAGE);

      Vector3& p = vertex_for_index(m_tess.m_vertices, BX->index+BY->index);
      Vector3 vTemp(p);

      Vector2& p2 = texcoord_for_index(m_tess.m_vertices, BX->index+BY->index);
      Vector2 stTemp(p2);

      TesselateSubMatrix( BY, BX->left,
                          offStartY, offStartX,
                          offEndY, BX->index,
                          newFlagsY, newFlagsX,
                          vertex_0_0, vertex_1_0, vertex_2_0,
                          texcoord_0_0, texcoord_1_0, texcoord_2_0,
                          !bTranspose );

      newFlagsY = nTemp;
      p = vTemp;
      p2 = stTemp;
    }

    if((nFlagsY & DEGEN_2a) && (nFlagsY & DEGEN_2b)) { newFlagsY |= DEGEN_2a; newFlagsY |= DEGEN_2b; }
    
    TesselateSubMatrix( BY, BX->right,
                        offStartY, BX->index,
                        offEndY, offEndX,
                        newFlagsY, newFlagsX,
                        vertex_0_1, vertex_1_1, vertex_2_1,
                        texcoord_0_1, texcoord_1_1, texcoord_2_1,
                        !bTranspose );
  }
  else
  {
    if(!BezierCurveTree_isLeaf(BX->left))
    {
      TesselateSubMatrix( BX->left,  BY,
                          offStartX, offStartY,
                          BX->index, offEndY,
                          newFlagsX, newFlagsY,
                          left, vertex_1_0, tmp,
                          texLeft, texcoord_1_0, texTmp,
                          bTranspose );
    }

    if(!BezierCurveTree_isLeaf(BX->right))
    {
      TesselateSubMatrix( BX->right, BY,
                          BX->index, offStartY,
                          offEndX, offEndY,
                          newFlagsX, newFlagsY,
                          tmp, vertex_1_1, right,
                          texTmp, texcoord_1_1, texRight,
                          bTranspose );
    }
  }

}

void Patch::BuildTesselationCurves(EMatrixMajor major)
{
  std::size_t nArrayStride, length, cross, strideU, strideV;
  switch(major)
  {
  case ROW:
    nArrayStride = 1;
    length = (m_width - 1) >> 1;
    cross = m_height;
    strideU = 1;
    strideV = m_width;

    if(!m_patchDef3)
    {
      BezierCurveTreeArray_deleteAll(m_tess.m_curveTreeU);
    }

    break;
  case COL:
    nArrayStride = m_tess.m_nArrayWidth;
    length = (m_height - 1) >> 1;
    cross = m_width;
    strideU = m_width;
    strideV = 1;

    if(!m_patchDef3)
    {
      BezierCurveTreeArray_deleteAll(m_tess.m_curveTreeV);
    }

    break;
  default:
    ERROR_MESSAGE("neither row-major nor column-major");
    return;
  }

  Array<std::size_t> arrayLength(length);
  Array<BezierCurveTree*> pCurveTree(length);

  std::size_t nArrayLength = 1;

  if(m_patchDef3)
  {
    for(Array<std::size_t>::iterator i = arrayLength.begin(); i != arrayLength.end(); ++i)
    {
      *i = Array<std::size_t>::value_type((major == ROW) ? m_subdivisions_x : m_subdivisions_y);
      nArrayLength += *i;
    }
  }
  else
  {
    // create a list of the horizontal control curves in each column of sub-patches
    // adaptively tesselate each horizontal control curve in the list
    // create a binary tree representing the combined tesselation of the list
    for(std::size_t i = 0; i != length; ++i)
    {
      PatchControl* p1 = m_ctrlTransformed.data() + (i * 2 * strideU);
      GSList* pCurveList = 0;
      for(std::size_t j = 0; j < cross; j += 2)
      {
        PatchControl* p2 = p1+strideV;
        PatchControl* p3 = p2+strideV;

        // directly taken from one row of control points
        {
          BezierCurve* pCurve = new BezierCurve;
          pCurve->crd = (p1+strideU)->m_vertex;
          pCurve->left = p1->m_vertex;
          pCurve->right = (p1+(strideU<<1))->m_vertex;
          pCurveList = g_slist_prepend(pCurveList, pCurve);
        }

        if(j+2 >= cross)
        {
          break;
        }
        
        // interpolated from three columns of control points
        {
          BezierCurve* pCurve = new BezierCurve;
          pCurve->crd = vector3_mid((p1+strideU)->m_vertex, (p3+strideU)->m_vertex);
          pCurve->left = vector3_mid(p1->m_vertex, p3->m_vertex);
          pCurve->right = vector3_mid((p1+(strideU<<1))->m_vertex, (p3+(strideU<<1))->m_vertex);
    
          pCurve->crd = vector3_mid(pCurve->crd, (p2+strideU)->m_vertex);
          pCurve->left = vector3_mid(pCurve->left, p2->m_vertex);
          pCurve->right = vector3_mid(pCurve->right, (p2+(strideU<<1))->m_vertex);
          pCurveList = g_slist_prepend(pCurveList, pCurve);
        }

        p1 = p3;
      }

      pCurveTree[i] = new BezierCurveTree;
      BezierCurveTree_FromCurveList(pCurveTree[i], pCurveList);
      for(GSList* l = pCurveList; l != 0; l = g_slist_next(l))
      {
        delete static_cast<BezierCurve*>((*l).data);
      }
      g_slist_free(pCurveList);

      // set up array indices for binary tree
      // accumulate subarray width
      arrayLength[i] = Array<std::size_t>::value_type(BezierCurveTree_Setup(pCurveTree[i], nArrayLength, nArrayStride) - (nArrayLength - 1));
      // accumulate total array width
      nArrayLength += arrayLength[i];
    }
  }

  switch(major)
  {
  case ROW:
    m_tess.m_nArrayWidth = nArrayLength;
    std::swap(m_tess.m_arrayWidth, arrayLength);

    if(!m_patchDef3)
    {
      std::swap(m_tess.m_curveTreeU, pCurveTree);
    }
    break;
  case COL:
    m_tess.m_nArrayHeight = nArrayLength;
    std::swap(m_tess.m_arrayHeight, arrayLength);

    if(!m_patchDef3)
    {
      std::swap(m_tess.m_curveTreeV, pCurveTree);
    }
    break;
  }
}

inline void vertex_assign_ctrl(ArbitraryMeshVertex& vertex, const PatchControl& ctrl)
{
  vertex.vertex = Vertex3f(ctrl.m_vertex);
  vertex.texcoord = ctrl.m_texcoord;
}

inline void vertex_clear_normal(ArbitraryMeshVertex& vertex)
{
  vertex.normal = Normal3f(0, 0, 0);
  vertex.tangent = Normal3f(0, 0, 0);
  vertex.bitangent = Normal3f(0, 0, 0);
}
      
inline void tangents_remove_degenerate(Vector3 tangents[6], Vector2 textureTangents[6], unsigned int flags)
{
  if(flags & DEGEN_0a)
  {
    const std::size_t i =
      (flags & DEGEN_0b)
      ? (flags & DEGEN_1a)
        ? (flags & DEGEN_1b)
          ? (flags & DEGEN_2a)
            ? 5
            : 4
          : 3
        : 2
      : 1;
    tangents[0] = tangents[i];
    textureTangents[0] = textureTangents[i];
  }
  if(flags & DEGEN_0b)
  {
    const std::size_t i =
      (flags & DEGEN_0a)
      ? (flags & DEGEN_1b)
        ? (flags & DEGEN_1a)
          ? (flags & DEGEN_2b)
            ? 4
            : 5
          : 2
        : 3
      : 0;
    tangents[1] = tangents[i];
    textureTangents[1] = textureTangents[i];
  }
  if(flags & DEGEN_2a)
  {
    const std::size_t i =
      (flags & DEGEN_2b)
      ? (flags & DEGEN_1a)
        ? (flags & DEGEN_1b)
          ? (flags & DEGEN_0a)
            ? 1
            : 0
          : 3
        : 2
      : 5;
    tangents[4] = tangents[i];
    textureTangents[4] = textureTangents[i];
  }
  if(flags & DEGEN_2b)
  {
    const std::size_t i =
      (flags & DEGEN_2a)
      ? (flags & DEGEN_1b)
        ? (flags & DEGEN_1a)
          ? (flags & DEGEN_0b)
            ? 0
            : 1
          : 2
        : 3
      : 4;
    tangents[5] = tangents[i];
    textureTangents[5] = textureTangents[i];
  }
}

void bestTangents00(unsigned int degenerateFlags, double dot, double length, std::size_t& index0, std::size_t& index1)
{
  if(fabs(dot + length) < 0.001) // opposing direction = degenerate
  {
    if(!(degenerateFlags & DEGEN_1a)) // if this tangent is degenerate we cannot use it
    {
      index0 = 2;
      index1 = 0;
    }
    else if(!(degenerateFlags & DEGEN_0b))
    {
      index0 = 0;
      index1 = 1;
    }
    else
    {
      index0 = 1;
      index1 = 0;
    }
  }
  else if(fabs(dot - length) < 0.001) // same direction = degenerate
  {
    if(degenerateFlags & DEGEN_0b)
    {
      index0 = 0;
      index1 = 1;
    }
    else
    {
      index0 = 1;
      index1 = 0;
    }
  }
}

void bestTangents01(unsigned int degenerateFlags, double dot, double length, std::size_t& index0, std::size_t& index1)
{
  if(fabs(dot - length) < 0.001) // same direction = degenerate
  {
    if(!(degenerateFlags & DEGEN_1a)) // if this tangent is degenerate we cannot use it
    {
      index0 = 2;
      index1 = 1;
    }
    else if(!(degenerateFlags & DEGEN_2b))
    {
      index0 = 4;
      index1 = 0;
    }
    else
    {
      index0 = 5;
      index1 = 1;
    }
  }
  else if(fabs(dot + length) < 0.001) // opposing direction = degenerate
  {
    if(degenerateFlags & DEGEN_2b)
    {
      index0 = 4;
      index1 = 0;
    }
    else
    {
      index0 = 5;
      index1 = 1;
    }
  }
}
 
void bestTangents10(unsigned int degenerateFlags, double dot, double length, std::size_t& index0, std::size_t& index1)
{
  if(fabs(dot - length) < 0.001) // same direction = degenerate
  {
    if(!(degenerateFlags & DEGEN_1b)) // if this tangent is degenerate we cannot use it
    {
      index0 = 3;
      index1 = 4;
    }
    else if(!(degenerateFlags & DEGEN_0a))
    {
      index0 = 1;
      index1 = 5;
    }
    else
    {
      index0 = 0;
      index1 = 4;
    }
  }
  else if(fabs(dot + length) < 0.001) // opposing direction = degenerate
  {
    if(degenerateFlags & DEGEN_0a)
    {
      index0 = 1;
      index1 = 5;
    }
    else
    {
      index0 = 0;
      index1 = 4;
    }
  }
}

void bestTangents11(unsigned int degenerateFlags, double dot, double length, std::size_t& index0, std::size_t& index1)
{
  if(fabs(dot + length) < 0.001) // opposing direction = degenerate
  {
    if(!(degenerateFlags & DEGEN_1b)) // if this tangent is degenerate we cannot use it
    {
      index0 = 3;
      index1 = 5;
    }
    else if(!(degenerateFlags & DEGEN_2a))
    {
      index0 = 5;
      index1 = 4;
    }
    else
    {
      index0 = 4;
      index1 = 5;
    }
  }
  else if(fabs(dot - length) < 0.001) // same direction = degenerate
  {
    if(degenerateFlags & DEGEN_2a)
    {
      index0 = 5;
      index1 = 4;
    }
    else
    {
      index0 = 4;
      index1 = 5;
    }
  }
}

void Patch::accumulateVertexTangentSpace(std::size_t index, Vector3 tangentX[6], Vector3 tangentY[6], Vector2 tangentS[6], Vector2 tangentT[6], std::size_t index0, std::size_t index1)
{
  {
    Vector3 normal(tangentX[index0].crossProduct(tangentY[index1]));
    if(normal != g_vector3_identity)
    {
      normal_for_index(m_tess.m_vertices, index) += normal.getNormalised();
    }
  }

  {
    ArbitraryMeshVertex a, b, c;
    a.vertex = Vertex3f(0, 0, 0);
    a.texcoord = TexCoord2f(0, 0);
    b.vertex = Vertex3f(tangentX[index0]);
    b.texcoord = tangentS[index0];
    c.vertex = Vertex3f(tangentY[index1]);
    c.texcoord = tangentT[index1];

    Vector3 s, t;
    ArbitraryMeshTriangle_calcTangents(a, b, c, s, t);
    if(s != g_vector3_identity)
    {
      tangent_for_index(m_tess.m_vertices, index) += s.getNormalised();
    }
    if(t != g_vector3_identity)
    {
      bitangent_for_index(m_tess.m_vertices, index) += t.getNormalised();
    }
  }
}

const std::size_t PATCH_MAX_VERTEX_ARRAY = 1048576;

void Patch::BuildVertexArray()
{
  const std::size_t strideU = 1;
  const std::size_t strideV = m_width;

  const std::size_t numElems = m_tess.m_nArrayWidth*m_tess.m_nArrayHeight; // total number of elements in vertex array

  const bool bWidthStrips = (m_tess.m_nArrayWidth >= m_tess.m_nArrayHeight); // decide if horizontal strips are longer than vertical


  // allocate vertex, normal, texcoord and primitive-index arrays
  m_tess.m_vertices.resize(numElems);
  m_tess.m_indices.resize(m_tess.m_nArrayWidth *2 * (m_tess.m_nArrayHeight - 1));

  // set up strip indices
  if(bWidthStrips)
  {
    m_tess.m_numStrips = m_tess.m_nArrayHeight-1;
    m_tess.m_lenStrips = m_tess.m_nArrayWidth*2;
  
    for(std::size_t i=0; i<m_tess.m_nArrayWidth; i++)
    {
      for(std::size_t j=0; j<m_tess.m_numStrips; j++)
      {
        m_tess.m_indices[(j*m_tess.m_lenStrips)+i*2] = RenderIndex(j*m_tess.m_nArrayWidth+i);
        m_tess.m_indices[(j*m_tess.m_lenStrips)+i*2+1] = RenderIndex((j+1)*m_tess.m_nArrayWidth+i);
        // reverse because radiant uses CULL_FRONT
        //m_tess.m_indices[(j*m_tess.m_lenStrips)+i*2+1] = RenderIndex(j*m_tess.m_nArrayWidth+i);
        //m_tess.m_indices[(j*m_tess.m_lenStrips)+i*2] = RenderIndex((j+1)*m_tess.m_nArrayWidth+i);
      }
    }
  }
  else
  {
    m_tess.m_numStrips = m_tess.m_nArrayWidth-1;
    m_tess.m_lenStrips = m_tess.m_nArrayHeight*2;

    for(std::size_t i=0; i<m_tess.m_nArrayHeight; i++)
    {
      for(std::size_t j=0; j<m_tess.m_numStrips; j++)
      {
        m_tess.m_indices[(j*m_tess.m_lenStrips)+i*2] = RenderIndex(((m_tess.m_nArrayHeight-1)-i)*m_tess.m_nArrayWidth+j);
        m_tess.m_indices[(j*m_tess.m_lenStrips)+i*2+1] = RenderIndex(((m_tess.m_nArrayHeight-1)-i)*m_tess.m_nArrayWidth+j+1);
        // reverse because radiant uses CULL_FRONT
        //m_tess.m_indices[(j*m_tess.m_lenStrips)+i*2+1] = RenderIndex(((m_tess.m_nArrayHeight-1)-i)*m_tess.m_nArrayWidth+j);
        //m_tess.m_indices[(j*m_tess.m_lenStrips)+i*2] = RenderIndex(((m_tess.m_nArrayHeight-1)-i)*m_tess.m_nArrayWidth+j+1);
        
      }
    }
  }

  {
    PatchControlIter pCtrl = m_ctrlTransformed.data();
    for(std::size_t j = 0, offStartY = 0; j+1 < m_height; j += 2, pCtrl += (strideU + strideV))
    {
      // set up array offsets for this sub-patch
      const bool leafY = (m_patchDef3) ? false : BezierCurveTree_isLeaf(m_tess.m_curveTreeV[j>>1]);
      const std::size_t offMidY = (m_patchDef3) ? 0 : m_tess.m_curveTreeV[j>>1]->index;
      const std::size_t widthY = m_tess.m_arrayHeight[j>>1] * m_tess.m_nArrayWidth;
      const std::size_t offEndY = offStartY + widthY;

      for(std::size_t i = 0, offStartX = 0; i+1 < m_width; i += 2, pCtrl += (strideU << 1))
      {
        const bool leafX = (m_patchDef3) ? false : BezierCurveTree_isLeaf(m_tess.m_curveTreeU[i>>1]);
        const std::size_t offMidX = (m_patchDef3) ? 0 : m_tess.m_curveTreeU[i>>1]->index;
        const std::size_t widthX = m_tess.m_arrayWidth[i>>1];
        const std::size_t offEndX = offStartX + widthX;

        PatchControl *subMatrix[3][3];
        subMatrix[0][0] = pCtrl;
        subMatrix[0][1] = subMatrix[0][0]+strideU;
        subMatrix[0][2] = subMatrix[0][1]+strideU;
        subMatrix[1][0] = subMatrix[0][0]+strideV;
        subMatrix[1][1] = subMatrix[1][0]+strideU;
        subMatrix[1][2] = subMatrix[1][1]+strideU;
        subMatrix[2][0] = subMatrix[1][0]+strideV;
        subMatrix[2][1] = subMatrix[2][0]+strideU;
        subMatrix[2][2] = subMatrix[2][1]+strideU;

        // assign on-patch control points to vertex array
        if(i == 0 && j == 0)
        {
          vertex_clear_normal(m_tess.m_vertices[offStartX + offStartY]);
        }
        vertex_assign_ctrl(m_tess.m_vertices[offStartX + offStartY], *subMatrix[0][0]);
        if(j == 0)
        {
          vertex_clear_normal(m_tess.m_vertices[offEndX + offStartY]);
        }
        vertex_assign_ctrl(m_tess.m_vertices[offEndX + offStartY], *subMatrix[0][2]);
        if(i == 0)
        {
          vertex_clear_normal(m_tess.m_vertices[offStartX + offEndY]);
        }
        vertex_assign_ctrl(m_tess.m_vertices[offStartX + offEndY], *subMatrix[2][0]);
      
        vertex_clear_normal(m_tess.m_vertices[offEndX + offEndY]);
        vertex_assign_ctrl(m_tess.m_vertices[offEndX + offEndY], *subMatrix[2][2]);

        if(!m_patchDef3)
        {
          // assign remaining control points to vertex array
          if(!leafX)
          {
            vertex_assign_ctrl(m_tess.m_vertices[offMidX + offStartY], *subMatrix[0][1]);
            vertex_assign_ctrl(m_tess.m_vertices[offMidX + offEndY], *subMatrix[2][1]);
          }
          if(!leafY)
          {
            vertex_assign_ctrl(m_tess.m_vertices[offStartX + offMidY], *subMatrix[1][0]);
            vertex_assign_ctrl(m_tess.m_vertices[offEndX + offMidY], *subMatrix[1][2]);

            if(!leafX)
            {
              vertex_assign_ctrl(m_tess.m_vertices[offMidX + offMidY], *subMatrix[1][1]);
            }
          }
        }

        // test all 12 edges for degeneracy
        unsigned int nFlagsX = subarray_get_degen(pCtrl, strideU, strideV);
        unsigned int nFlagsY = subarray_get_degen(pCtrl, strideV, strideU);
        Vector3 tangentX[6], tangentY[6];
        Vector2 tangentS[6], tangentT[6];

        // set up tangents for each of the 12 edges if they were not degenerate
        if(!(nFlagsX & DEGEN_0a))
        {
          tangentX[0] = subMatrix[0][1]->m_vertex - subMatrix[0][0]->m_vertex;
          tangentS[0] = subMatrix[0][1]->m_texcoord - subMatrix[0][0]->m_texcoord;
        }
        if(!(nFlagsX & DEGEN_0b))
        {
          tangentX[1] = subMatrix[0][2]->m_vertex - subMatrix[0][1]->m_vertex;
          tangentS[1] = subMatrix[0][2]->m_texcoord - subMatrix[0][1]->m_texcoord;
        }
        if(!(nFlagsX & DEGEN_1a))
        {
          tangentX[2] = subMatrix[1][1]->m_vertex - subMatrix[1][0]->m_vertex;
          tangentS[2] = subMatrix[1][1]->m_texcoord - subMatrix[1][0]->m_texcoord;
        }
        if(!(nFlagsX & DEGEN_1b))
        {
          tangentX[3] = subMatrix[1][2]->m_vertex - subMatrix[1][1]->m_vertex;
          tangentS[3] = subMatrix[1][2]->m_texcoord - subMatrix[1][1]->m_texcoord;
        }
        if(!(nFlagsX & DEGEN_2a))
        {
          tangentX[4] = subMatrix[2][1]->m_vertex - subMatrix[2][0]->m_vertex;
          tangentS[4] = subMatrix[2][1]->m_texcoord - subMatrix[2][0]->m_texcoord;
        }
        if(!(nFlagsX & DEGEN_2b))
        {
          tangentX[5] = subMatrix[2][2]->m_vertex - subMatrix[2][1]->m_vertex;
          tangentS[5] = subMatrix[2][2]->m_texcoord - subMatrix[2][1]->m_texcoord;
        }

        if(!(nFlagsY & DEGEN_0a))
        {
          tangentY[0] = subMatrix[1][0]->m_vertex - subMatrix[0][0]->m_vertex;
          tangentT[0] = subMatrix[1][0]->m_texcoord - subMatrix[0][0]->m_texcoord;
        }
        if(!(nFlagsY & DEGEN_0b))
        {
          tangentY[1] = subMatrix[2][0]->m_vertex - subMatrix[1][0]->m_vertex;
          tangentT[1] = subMatrix[2][0]->m_texcoord - subMatrix[1][0]->m_texcoord;
        }
        if(!(nFlagsY & DEGEN_1a))
        {
          tangentY[2] = subMatrix[1][1]->m_vertex - subMatrix[0][1]->m_vertex;
          tangentT[2] = subMatrix[1][1]->m_texcoord - subMatrix[0][1]->m_texcoord;
        }
        if(!(nFlagsY & DEGEN_1b))
        {
          tangentY[3] = subMatrix[2][1]->m_vertex - subMatrix[1][1]->m_vertex;
          tangentT[3] = subMatrix[2][1]->m_texcoord - subMatrix[1][1]->m_texcoord;
        }
        if(!(nFlagsY & DEGEN_2a))
        {
          tangentY[4] = subMatrix[1][2]->m_vertex - subMatrix[0][2]->m_vertex;
          tangentT[4] = subMatrix[1][2]->m_texcoord - subMatrix[0][2]->m_texcoord;
        }
        if(!(nFlagsY & DEGEN_2b))
        {
          tangentY[5] = subMatrix[2][2]->m_vertex - subMatrix[1][2]->m_vertex;
          tangentT[5] = subMatrix[2][2]->m_texcoord - subMatrix[1][2]->m_texcoord;
        }

        // set up remaining edge tangents by borrowing the tangent from the closest parallel non-degenerate edge
        tangents_remove_degenerate(tangentX, tangentS, nFlagsX);
        tangents_remove_degenerate(tangentY, tangentT, nFlagsY);

        {
          // x=0, y=0
          std::size_t index = offStartX + offStartY;
          std::size_t index0 = 0;
          std::size_t index1 = 0;

          double dot = tangentX[index0].dot(tangentY[index1]);
          double length = tangentX[index0].getLength() * tangentY[index1].getLength();

          bestTangents00(nFlagsX, dot, length, index0, index1);

          accumulateVertexTangentSpace(index, tangentX, tangentY, tangentS, tangentT, index0, index1);
        }

        {
          // x=1, y=0
          std::size_t index = offEndX + offStartY;
          std::size_t index0 = 1;
          std::size_t index1 = 4;

          double dot = tangentX[index0].dot(tangentY[index1]);
          double length = tangentX[index0].getLength() * tangentY[index1].getLength();

          bestTangents10(nFlagsX, dot, length, index0, index1);

          accumulateVertexTangentSpace(index, tangentX, tangentY, tangentS, tangentT, index0, index1);
        }

        {
          // x=0, y=1
          std::size_t index = offStartX + offEndY;
          std::size_t index0 = 4;
          std::size_t index1 = 1;

          double dot = tangentX[index0].dot(tangentY[index1]);
          double length = tangentX[index1].getLength() * tangentY[index1].getLength();

          bestTangents01(nFlagsX, dot, length, index0, index1);

          accumulateVertexTangentSpace(index, tangentX, tangentY, tangentS, tangentT, index0, index1);
        }

        {
          // x=1, y=1
          std::size_t index = offEndX + offEndY;
          std::size_t index0 = 5;
          std::size_t index1 = 5;

          double dot = tangentX[index0].dot(tangentY[index1]);
          double length = tangentX[index0].getLength() * tangentY[index1].getLength();

          bestTangents11(nFlagsX, dot, length, index0, index1);

          accumulateVertexTangentSpace(index, tangentX, tangentY, tangentS, tangentT, index0, index1);
        }

        //normalise normals that won't be accumulated again
        if(i!=0 || j!=0)
        {
          normalise_safe(normal_for_index(m_tess.m_vertices, offStartX + offStartY));
          normalise_safe(tangent_for_index(m_tess.m_vertices, offStartX + offStartY));
          normalise_safe(bitangent_for_index(m_tess.m_vertices, offStartX + offStartY));
        }
        if(i+3 == m_width)
        {
          normalise_safe(normal_for_index(m_tess.m_vertices, offEndX + offStartY));
          normalise_safe(tangent_for_index(m_tess.m_vertices, offEndX + offStartY));
          normalise_safe(bitangent_for_index(m_tess.m_vertices, offEndX + offStartY));
        }
        if(j+3 == m_height)
        {
          normalise_safe(normal_for_index(m_tess.m_vertices, offStartX + offEndY));
          normalise_safe(tangent_for_index(m_tess.m_vertices, offStartX + offEndY));
          normalise_safe(bitangent_for_index(m_tess.m_vertices, offStartX + offEndY));
        }
        if(i+3 == m_width && j+3 == m_height)
        {
          normalise_safe(normal_for_index(m_tess.m_vertices, offEndX + offEndY));
          normalise_safe(tangent_for_index(m_tess.m_vertices, offEndX + offEndY));
          normalise_safe(bitangent_for_index(m_tess.m_vertices, offEndX + offEndY));
        }

        // set flags to average normals between shared edges
        if(j != 0)
        {
          nFlagsX |= AVERAGE;
        }
        if(i != 0)
        {
          nFlagsY |= AVERAGE;
        }
        // set flags to save evaluating shared edges twice
        nFlagsX |= SPLIT;
        nFlagsY |= SPLIT;    
      
        // if the patch is curved.. tesselate recursively
        // use the relevant control curves for this sub-patch
        if(m_patchDef3)
        {
          TesselateSubMatrixFixed(m_tess.m_vertices.data() + offStartX + offStartY, 1, m_tess.m_nArrayWidth, nFlagsX, nFlagsY, subMatrix);
        }
        else
        {
          if(!leafX)
          {
            TesselateSubMatrix( m_tess.m_curveTreeU[i>>1], m_tess.m_curveTreeV[j>>1],
                                offStartX, offStartY, offEndX, offEndY, // array offsets
                                nFlagsX, nFlagsY,
                                subMatrix[1][0]->m_vertex, subMatrix[1][1]->m_vertex, subMatrix[1][2]->m_vertex,
                                subMatrix[1][0]->m_texcoord, subMatrix[1][1]->m_texcoord, subMatrix[1][2]->m_texcoord,
                                false );
          }
          else if(!leafY)
          {
            TesselateSubMatrix( m_tess.m_curveTreeV[j>>1], m_tess.m_curveTreeU[i>>1],
                                offStartY, offStartX, offEndY, offEndX, // array offsets
                                nFlagsY, nFlagsX,
                                subMatrix[0][1]->m_vertex, subMatrix[1][1]->m_vertex, subMatrix[2][1]->m_vertex,
                                subMatrix[0][1]->m_texcoord, subMatrix[1][1]->m_texcoord, subMatrix[2][1]->m_texcoord,
                                true );
          }
        }

        offStartX = offEndX;
      }
      offStartY = offEndY;
    }
  }
}

void Patch::createThickenedOpposite(const Patch& sourcePatch, 
									const float& thickness, 
									const int& axis) 
{
	// Clone the dimensions from the other patch
	setDims(sourcePatch.getWidth(), sourcePatch.getHeight());

	// Also inherit the tesselation from the source patch
	setFixedSubdivisions(sourcePatch.subdivionsFixed(), sourcePatch.getSubdivisions());
	
	// Copy the shader from the source patch
	SetShader(sourcePatch.GetShader());
	
	// if extrudeAxis == 0,0,0 the patch is extruded along its vertex normals
	Vector3 extrudeAxis(0,0,0);
	
	switch (axis) {
		case 0: // X-Axis
			extrudeAxis = Vector3(1,0,0);
			break;
		case 1: // Y-Axis
			extrudeAxis = Vector3(0,1,0);
			break;
		case 2: // Z-Axis
			extrudeAxis = Vector3(0,0,1);
			break;
		default:
			// Default value already set during initialisation
			break;
	}
	
	for (std::size_t col = 0; col < m_width; col++) {
		for (std::size_t row = 0; row < m_height; row++) {
			
			// The current control vertex on the other patch
			const PatchControl& curCtrl = sourcePatch.ctrlAt(row, col);
			
			// The col vertices that are averaged (or ignored if 0,0,0)
			Vector3 colTangent[2] = {
				Vector3(0,0,0),
				Vector3(0,0,0)
			};
			
			// Are we at the end of the column?
			if (col == m_width-1) {
				// Take the one neighbour vertex
				const PatchControl& neighbour = sourcePatch.ctrlAt(row, col-1);
				// Only fill one of the rowTangents
				colTangent[0] = neighbour.m_vertex - curCtrl.m_vertex;
				// Reverse it, as it faces the other direction
				colTangent[0] *= -1;
			}
			// Are we at the beginning of the column?
			else if (col == 0) {
				// Take the one neighbour vertex
				const PatchControl& neighbour = sourcePatch.ctrlAt(row, col+1);
				// Only fill one of the rowTangents
				colTangent[0] = neighbour.m_vertex - curCtrl.m_vertex;
			}
			// We are in between, two normals tangents can be calculated
			else {
				// Take two neighbouring vertices that should form a line segment
				const PatchControl& neighbour1 = sourcePatch.ctrlAt(row, col+1);
				const PatchControl& neighbour2 = sourcePatch.ctrlAt(row, col-1);
				
				// Calculate both available tangents
				colTangent[0] = neighbour1.m_vertex - curCtrl.m_vertex;
				colTangent[1] = neighbour2.m_vertex - curCtrl.m_vertex;
				
				// Reverse the second one
				colTangent[1] *= -1;
			}
			
			// Get the next row index
			std::size_t nextRow = (row == m_height-1) ? (row - 1) : (row + 1);
			
			const PatchControl& rowNeighbour = sourcePatch.ctrlAt(nextRow, col);
			
			// Calculate the tangent vector to the next row
			Vector3 rowTangent = rowNeighbour.m_vertex - curCtrl.m_vertex;
			// Reverse it accordingly
			rowTangent *= (row == m_height-1) ? -1 : +1;
			
			Vector3 normal;
			
			// Are we extruding along vertex normals (i.e. extrudeAxis == 0,0,0)?
			if (extrudeAxis == Vector3(0,0,0)) {
				// Calculate the normal vector with the first tangent
				// this gives us the first direction of the thickening
				normal = rowTangent.crossProduct(colTangent[0]).getNormalised();
				
				// Check if we have another normal available
				if (colTangent[1] != Vector3(0,0,0)) {
					
					// Calculate the other normal
					Vector3 normal2 = rowTangent.crossProduct(colTangent[1]).getNormalised();
					
					// Now calculate the length correction out of the angle 
					// of the two normals
					float factor = cos(normal.angle(normal2)/2);
			
					// Calculate the mean value and normalise it
					normal += normal2;
					normal /= 2;
					normal = normal.getNormalised();
					
					// Check for div by zero (if the normals are antiparallel)
					// and stretch the resulting normal, if necessary
					if (factor != 0) {
						normal /= factor;
					}
					else {
						normal = Vector3(0,0,0);
					}
				}
			}
			else {
				// Take the predefined extrude direction instead
				normal = extrudeAxis;
			}
			
			// Store the new coordinates into this patch at the current coords
			ctrlAt(row, col).m_vertex = curCtrl.m_vertex + normal*thickness;
			
			// Clone the texture cooordinates of the source patch
			ctrlAt(row, col).m_texcoord = curCtrl.m_texcoord;
		}
	}
	
	// Notify the patch about the change
	controlPointsChanged();
}

void Patch::createThickenedWall(const Patch& sourcePatch, 
								const Patch& targetPatch, 
								const int& wallIndex) 
{
	// Copy the shader from the source patch
	SetShader(sourcePatch.GetShader());
	
	// The start and end control vertex indices
	int start = 0;
	int end = 0;
	// The increment (incr = 1 for the "long" edge, incr = width for the "short" edge)
	int incr = 1;
	
	// These are the target dimensions of this wall
	// The width is depending on which edge is "seamed".
	int cols = 0;
	int rows = 3;
	
	int sourceWidth = static_cast<int>(sourcePatch.getWidth());
	int sourceHeight = static_cast<int>(sourcePatch.getHeight());

	bool sourceTesselationFixed = sourcePatch.subdivionsFixed();
	BasicVector2<unsigned int> sourceTesselationX(sourcePatch.getSubdivisions().x(), 1);
	BasicVector2<unsigned int> sourceTesselationY(sourcePatch.getSubdivisions().y(), 1);
	
	// Determine which of the four edges have to be connected
	// and calculate the start, end & stepsize for the following loop
	switch (wallIndex) {
		case 0:
			cols = sourceWidth;
			start = 0;
			end = sourceWidth - 1;
			incr = 1;
			setFixedSubdivisions(sourceTesselationFixed, sourceTesselationX);
			break;
		case 1:
			cols = sourceWidth;
			start = sourceWidth * (sourceHeight-1);
			end = sourceWidth*sourceHeight - 1;
			incr = 1;
			setFixedSubdivisions(sourceTesselationFixed, sourceTesselationX);
			break;
		case 2:
			cols = sourceHeight;
			start = 0;
			end = sourceWidth*(sourceHeight-1);
			incr = sourceWidth;
			setFixedSubdivisions(sourceTesselationFixed, sourceTesselationY);
			break;
		case 3:
			cols = sourceHeight;
			start = sourceWidth - 1;
			end = sourceWidth*sourceHeight - 1;
			incr = sourceWidth;
			setFixedSubdivisions(sourceTesselationFixed, sourceTesselationY);
			break;
	}
	
	setDims(cols, rows);
	
	const PatchControlArray& sourceCtrl = sourcePatch.getControlPoints();
	const PatchControlArray& targetCtrl = targetPatch.getControlPoints(); 
	
	int col = 0;
	// Now go through the control vertices with these calculated stepsize 
	for (int idx = start; idx <= end; idx += incr, col++) {
		Vector3 sourceCoord = sourceCtrl[idx].m_vertex;
		Vector3 targetCoord = targetCtrl[idx].m_vertex;
		Vector3 middleCoord = (sourceCoord + targetCoord) / 2;
		
		// Now assign the vertex coordinates
		ctrlAt(0, col).m_vertex = sourceCoord;
		ctrlAt(1, col).m_vertex = middleCoord;
		ctrlAt(2, col).m_vertex = targetCoord;
	}
	
	if (wallIndex == 0 || wallIndex == 3) {
		InvertMatrix();
	}
	
	// Notify the patch about the change
	controlPointsChanged();
	
	// Texture the patch "naturally"
	NaturalTexture();
}

void Patch::stitchTextureFrom(Patch& sourcePatch) {
	// Save the undo memento
	undoSave();
	
	// Convert the size_t stuff into int, because we need it for signed comparisons
	int patchHeight = static_cast<int>(m_height);
	int patchWidth = static_cast<int>(m_width);
	
	// Calculate the nearest corner vertex of this patch (to the sourcepatch vertices)
	PatchControl* nearestControl = getClosestPatchControlToPatch(sourcePatch);
	
	PatchControl* refControl = sourcePatch.getClosestPatchControlToPatch(*this);
	
	// Get the distance in texture space
	Vector2 texDiff = refControl->m_texcoord - nearestControl->m_texcoord;
	
	// The floored values
	Vector2 floored(floor(fabs(texDiff[0])), floor(fabs(texDiff[1])));
	
	// Compute the shift applicable to all vertices
	Vector2 shift;
	shift[0] = (fabs(texDiff[0])>1.0E-4) ? -floored[0] * texDiff[0]/fabs(texDiff[0]) : 0.0f;
	shift[1] = (fabs(texDiff[1])>1.0E-4) ? -floored[1] * texDiff[1]/fabs(texDiff[1]) : 0.0f;
		
	// Now shift all the texture vertices in the right direction, so that this patch
	// is getting as close as possible to the origin in texture space.
	for (PatchControlIter i = m_ctrl.data(); i != m_ctrl.data() + m_ctrl.size(); ++i) {
		i->m_texcoord += shift;
	}
	
	int sourceHeight = static_cast<int>(sourcePatch.getHeight());
	int sourceWidth = static_cast<int>(sourcePatch.getWidth());
	
	// Go through all the 3D vertices and see if they are shared by the other patch
	for (int col = 0; col < patchWidth; col++) {
		for (int row = 0; row < patchHeight; row++) {
			
			// The control vertex that is to be manipulated			
			PatchControl& self = ctrlAt(row, col);
			
			// Check all the other patch controls for spatial coincidences
			for (int srcCol = 0; srcCol < sourceWidth; srcCol++) {
				for (int srcRow = 0; srcRow < sourceHeight; srcRow++) {
					// Get the other control
					const PatchControl& other = sourcePatch.ctrlAt(srcRow, srcCol);
					
					float dist = (other.m_vertex - self.m_vertex).getLength();
					
					// Allow the coords to be a _bit_ distant  
					if (fabs(dist) < 0.005f) {
						// Assimilate the texture coordinates
						self.m_texcoord = other.m_texcoord;
					}
				}
			}
		}
	}
	
	// Notify the patch about the change
	controlPointsChanged();
}

void Patch::normaliseTexture() {
	// Find the nearest control vertex
	
	// Initialise the compare value
	PatchControlIter nearestControl = m_ctrl.data();
	
	// Cycle through all the control points with an iterator
	for (PatchControlIter i = m_ctrl.data(); i != m_ctrl.data() + m_ctrl.size(); ++i) {
		// Take the according value (e.g. s = x, t = y, depending on the nAxis argument) 
		// and apply the appropriate texture coordinate
		if (i->m_texcoord.getLength() < nearestControl->m_texcoord.getLength()) {
			nearestControl = i;
		}
	}
	
	// Get the nearest texcoord
	Vector2 texcoord = nearestControl->m_texcoord;
	
	// The floored values
	Vector2 floored(floor(fabs(texcoord[0])), floor(fabs(texcoord[1])));
	
	// Compute the shift applicable to all vertices
	Vector2 shift;
	shift[0] = (fabs(texcoord[0])>1.0E-4) ? -floored[0] * texcoord[0]/fabs(texcoord[0]) : 0.0f;
	shift[1] = (fabs(texcoord[1])>1.0E-4) ? -floored[1] * texcoord[1]/fabs(texcoord[1]) : 0.0f;

	// Is there anything to do at all?		
	if (shift.getLength() > 0.0f) {
		// Save the undo memento
		undoSave();
		
		// Now shift all the texture vertices in the right direction, so that this patch
		// is getting as close as possible to the origin in texture space.
		for (PatchControlIter i = m_ctrl.data(); i != m_ctrl.data() + m_ctrl.size(); ++i) {
			i->m_texcoord += shift;
		}
		
		// Notify the patch about the change
		controlPointsChanged();
	}
}

BasicVector2<unsigned int> Patch::getSubdivisions() const {
	return BasicVector2<unsigned int>(m_subdivisions_x, m_subdivisions_y);
}

void Patch::setFixedSubdivisions(bool isFixed, BasicVector2<unsigned int> divisions) {
	undoSave();
		
	m_patchDef3 = isFixed;
	m_subdivisions_x = divisions[0];
	m_subdivisions_y = divisions[1];
	
	if (m_subdivisions_x == 0) {
		m_subdivisions_x = 4;
	}
	else if (m_subdivisions_x > MAX_PATCH_SUBDIVISIONS) {
		m_subdivisions_x = MAX_PATCH_SUBDIVISIONS;
	}
	
	if (m_subdivisions_y == 0) {
		m_subdivisions_y = 4;
	}
	else if (m_subdivisions_y > MAX_PATCH_SUBDIVISIONS) {
		m_subdivisions_y = MAX_PATCH_SUBDIVISIONS;
	}
	
	SceneChangeNotify();
	textureChanged();
	controlPointsChanged();
}

bool Patch::subdivionsFixed() const {
	return m_patchDef3;
}

void Patch::textureChanged() {
	ui::SurfaceInspector::Instance().update(); // Triggers TexTool update
	ui::PatchInspector::Instance().update();
}
