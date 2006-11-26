#include "Patch.h"

#include "shaderlib.h"
#include "renderable.h"
#include "selectable.h"
#include "math/frustum.h"
#include "stringio.h"
#include "xml/xmlelement.h"
#include "signal/signal.h"
#include "plugin.h"
#include "texturelib.h"
#include "brush_primit.h"

#include "PatchSavedState.h"

// ====== Helper Functions ==================================================================

inline VertexPointer vertexpointer_arbitrarymeshvertex(const ArbitraryMeshVertex* array) {
  return VertexPointer(VertexPointer::pointer(&array->vertex), sizeof(ArbitraryMeshVertex));
}

Signal0 g_patchTextureChangedCallbacks;

void Patch_addTextureChangedCallback(const SignalHandler& handler) {
  g_patchTextureChangedCallbacks.connectLast(handler);
}

void Patch_textureChanged() {
  g_patchTextureChangedCallbacks();
}

inline const Colour4b colour_for_index(std::size_t i, std::size_t width) {
  const Vector3 cornerColourVec = ColourSchemes().getColourVector3("patch_vertex_corner");
  const Vector3 insideColourVec = ColourSchemes().getColourVector3("patch_vertex_inside");
  const Colour4b colour_corner(int(cornerColourVec[0]*255), int(cornerColourVec[1]*255), 
  							   int(cornerColourVec[2]*255), 255);
  const Colour4b colour_inside(int(insideColourVec[0]*255), int(insideColourVec[1]*255), 
  							   int(insideColourVec[2]*255), 255);
  
  return (i%2 || (i/width)%2) ? colour_inside : colour_corner;
}

inline bool float_valid(float f) {
  return f == f;
}

// ====== Patch Implementation =========================================================================

// Initialise the shader state variables and the patch type
Shader* Patch::m_state_ctrl;
Shader* Patch::m_state_lattice;
EPatchType Patch::m_type;

// Initialise the cycle cap index member variable
int Patch::m_CycleCapIndex = 0;

// greebo: Another nasty global that is also referenced from patchmanip.cpp, so I'll leave it for the moment
int g_PatchSubdivideThreshold = 4;

// Constructor
Patch::Patch(scene::Node& node, const Callback& evaluateTransform, const Callback& boundsChanged) :
	m_node(&node),
	m_shader(texdef_name_default()),
	m_state(0),
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
	m_state(0),
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
	SetShader(other.m_shader.c_str());
	controlPointsChanged();
}

// Another copy constructor
Patch::Patch(const Patch& other) :
	XMLImporter(other),
	XMLExporter(other),
	TransformNode(other),
	Bounded(other),
	Cullable(other),
	Snappable(other),
	Undoable(other),
	Nameable(other),
	m_state(0),
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
	SetShader(other.m_shader.c_str());
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

// Get the (temporary) transformed control point array, not the saved ones
PatchControlArray& Patch::getControlPointsTransformed() {
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

void Patch::pushElement(const XMLElement& element) {
	switch (m_xml_state.back().state()) {
		case XMLState::eDefault:
			ASSERT_MESSAGE(string_equal(element.name(), "patch"), "parse error");
			m_xml_state.push_back(XMLState::ePatch);
			break;
		
		case XMLState::ePatch:
			if (string_equal(element.name(), "matrix")) {
				setDims(atoi(element.attribute("width")), atoi(element.attribute("height")));
				m_xml_state.push_back(XMLState::eMatrix);
			}
			else if (string_equal(element.name(), "shader")) {
				m_xml_state.push_back(XMLState::eShader);
			}
			break;
    default:
		ERROR_MESSAGE("parse error");
    } // end switch
}

void Patch::popElement(const char* name) {
	switch (m_xml_state.back().state()) {
		case XMLState::eDefault:
			ERROR_MESSAGE("parse error");
		break;
		case XMLState::ePatch:
		break;
		case XMLState::eMatrix: {
			StringTokeniser content(m_xml_state.back().content());
	
			for (PatchControlIter i = m_ctrl.data(), end = m_ctrl.data() + m_ctrl.size(); i != end; ++i) {
				(*i).m_vertex[0] = string_read_float(content.getToken());
				(*i).m_vertex[1] = string_read_float(content.getToken());
				(*i).m_vertex[2] = string_read_float(content.getToken());
				(*i).m_texcoord[0] = string_read_float(content.getToken());
				(*i).m_texcoord[1] = string_read_float(content.getToken());
			}
			controlPointsChanged();
		}
	    break;
	    case XMLState::eShader: {
			SetShader(m_xml_state.back().content());
		}
		break;
		default:
			ERROR_MESSAGE("parse error");
		} // end switch

	ASSERT_MESSAGE(!m_xml_state.empty(), "popping empty stack");
	m_xml_state.pop_back();
}

std::size_t Patch::write(const char* buffer, std::size_t length) {
	switch(m_xml_state.back().state()) {
		case XMLState::eDefault:	
			break;
		case XMLState::ePatch: 
			break;
		case XMLState::eMatrix: // Fall through
		case XMLState::eShader:
			return m_xml_state.back().write(buffer, length);
			break;
		default:
			ERROR_MESSAGE("parse error");
	}
	return length;
}

void Patch::exportXML(XMLImporter& importer) {
	StaticElement patchElement("patch");
	importer.pushElement(patchElement);

	{
		const StaticElement element("shader");
		importer.pushElement(element);
		importer.write(m_shader.c_str(), strlen(m_shader.c_str()));
		importer.popElement(element.name());
	}

	{
		char width[16], height[16];
		sprintf(width, "%u", Unsigned(m_width));
		sprintf(height, "%u", Unsigned(m_height));
		StaticElement element("matrix");
		element.insertAttribute("width", width);
		element.insertAttribute("height", height);

		importer.pushElement(element);
		{
			for (PatchControlIter i = m_ctrl.data(), end = m_ctrl.data() + m_ctrl.size(); i != end; ++i) {
				importer << (*i).m_vertex[0]
						 << ' ' << (*i).m_vertex[1]
						 << ' ' << (*i).m_vertex[2]
						 << ' ' << (*i).m_texcoord[0]
						 << ' ' << (*i).m_texcoord[1];
			}
		}
		importer.popElement(element.name());
	}

	importer.popElement(patchElement.name());
}

const char* Patch::GetShader() const {
	return m_shader.c_str();
}

void Patch::SetShader(const char* name) {
		ASSERT_NOTNULL(name);
  
  		// return, if the shader is the same as the currently used 
	if (shader_equal(m_shader.c_str(), name))
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
	Patch_textureChanged();
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
	return new SavedState(m_width, m_height, m_ctrl, m_shader.c_str(), m_patchDef3, m_subdivisions_x, m_subdivisions_y);
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
		SetShader(other.m_shader.c_str());
		m_ctrl = other.m_ctrl;
		onAllocate(m_ctrl.size());
		m_patchDef3 = other.m_patchDef3;
		m_subdivisions_x = other.m_subdivisions_x;
		m_subdivisions_y = other.m_subdivisions_y;
	}

	// end duplicate code

	// Notify that this patch has changed
	Patch_textureChanged();
	controlPointsChanged();
}

void Patch::captureShader() {
	m_state = GlobalShaderCache().capture(m_shader.c_str());
}

void Patch::releaseShader() {
	GlobalShaderCache().release(m_shader.c_str());
}

void Patch::check_shader() {
	if (!shader_valid(GetShader())) {
		globalErrorStream() << "patch has invalid texture name: '" << GetShader() << "'\n";
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
    if(!float_valid((*i).m_vertex.x())
      || !float_valid((*i).m_vertex.y())
      || !float_valid((*i).m_vertex.z())
      || !float_valid((*i).m_texcoord.x())
      || !float_valid((*i).m_texcoord.y()))
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

void Patch::InsertRemove(bool bInsert, bool bColumn, bool bFirst)
{
  undoSave();

  if(bInsert)
  {
    if(bColumn && (m_width + 2 <= MAX_PATCH_WIDTH))
      InsertPoints(COL, bFirst);
    else if(m_height + 2 <= MAX_PATCH_HEIGHT)
      InsertPoints(ROW, bFirst);
  }
  else
  {
    if(bColumn && (m_width - 2 >= MIN_PATCH_WIDTH))
      RemovePoints(COL, bFirst);
    else if(m_height - 2 >= MIN_PATCH_HEIGHT)
      RemovePoints(ROW, bFirst);
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

void Patch::TranslateTexture(float s, float t)
{
  undoSave();

  s = -1 * s / m_state->getTexture().width;
  t = t / m_state->getTexture().height;

  for(PatchControlIter i = m_ctrl.data(); i != m_ctrl.data() + m_ctrl.size(); ++i)
  {
    (*i).m_texcoord[0] += s;
    (*i).m_texcoord[1] += t;
  }

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


void Patch::SetTextureRepeat(float s, float t)
{
  std::size_t w, h;
  float si, ti, sc, tc;
  PatchControl *pDest;
  
  undoSave();

  si = s / (float)(m_width - 1);
  ti = t / (float)(m_height - 1);

  pDest = m_ctrl.data();
  for (h=0, tc = 0.0f; h<m_height; h++, tc+=ti)
  {
    for (w=0, sc = 0.0f; w<m_width; w++, sc+=si) 
    {
      pDest->m_texcoord[0] = sc;
      pDest->m_texcoord[1] = tc;
      pDest++;
    }
  }

  controlPointsChanged();
}

/*
void Patch::SetTextureInfo(texdef_t *pt)
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

void Patch::CapTexture()
{
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

// uses longest parallel chord to calculate texture coords for each row/col
void Patch::NaturalTexture()
{
  undoSave();

  {
    float fSize = (float)m_state->getTexture().width * Texdef_getDefaultTextureScale();
  
    double texBest = 0;
    double tex = 0;
    PatchControl* pWidth = m_ctrl.data();
    for (std::size_t w=0; w<m_width; w++, pWidth++) 
    {
      {
        PatchControl* pHeight = pWidth;
        for (std::size_t h=0; h<m_height; h++, pHeight+=m_width)
          pHeight->m_texcoord[0] = static_cast<float>(tex);
      }

      if(w+1 == m_width)
        break;

      {
        PatchControl* pHeight = pWidth;
        for (std::size_t h=0; h<m_height; h++, pHeight+=m_width)
        {
          Vector3 v(pHeight->m_vertex - (pHeight+1)->m_vertex);
          double length = tex + (v.getLength() / fSize);
          if(fabs(length) > texBest) texBest = length;
        }
      }

      tex=texBest;
    }
  }

  {
    float fSize = -(float)m_state->getTexture().height * Texdef_getDefaultTextureScale();

    double texBest = 0;
    double tex = 0;
    PatchControl* pHeight = m_ctrl.data();
    for (std::size_t h=0; h<m_height; h++, pHeight+=m_width) 
    {
      {
        PatchControl* pWidth = pHeight;
        for (std::size_t w=0; w<m_width; w++, pWidth++)
          pWidth->m_texcoord[1] = static_cast<float>(tex);
      }

      if(h+1 == m_height)
        break;

      {
        PatchControl* pWidth = pHeight;
        for (std::size_t w=0; w<m_width; w++, pWidth++)
        {
          Vector3 v(pWidth->m_vertex - (pWidth+m_width)->m_vertex);
          double length = tex + (v.getLength() / fSize);
          if(fabs(length) > texBest) texBest = length;
        }
      }

      tex=texBest;
    }
  }

  controlPointsChanged();
}



// private:

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

void Patch::InsertPoints(EMatrixMajor mt, bool bFirst)
{
  std::size_t width, height, row_stride, col_stride; 

  switch(mt)
  {
  case ROW:
    col_stride = 1;
    row_stride = m_width;
    width = m_width;
    height = m_height;
    break;
  case COL:
    col_stride = m_width;
    row_stride = 1;
    width = m_height;
    height = m_width;
    break;
  default:
    ERROR_MESSAGE("neither row-major nor column-major");
    return;
  }

  std::size_t pos = 0;
  {
    PatchControl* p1 = m_ctrl.data();
    for(std::size_t w = 0; w != width; ++w, p1 += col_stride)
    {
      {
        PatchControl* p2 = p1;
        for(std::size_t h = 1; h < height; h += 2, p2 += 2 * row_stride)
        {
          if(0)//p2->m_selectable.isSelected())
          {
            pos = h;
            break;
          }
        }
        if(pos != 0)
        {
          break;
        }
      }
  
      {
        PatchControl* p2 = p1;
        for(std::size_t h = 0; h < height; h += 2, p2 += 2 * row_stride)
        {
          if(0)//p2->m_selectable.isSelected())
          {
            pos = h;
            break;
          }
        }
        if(pos != 0)
        {
          break;
        }
      }
    }
  }

  Array<PatchControl> tmp(m_ctrl);

  std::size_t row_stride2, col_stride2;
  switch(mt)
  {
  case ROW:
    setDims(m_width, m_height+2);
    col_stride2 = 1;
    row_stride2 = m_width;
    break;
  case COL:
    setDims(m_width+2, m_height);
    col_stride2 = m_width;
    row_stride2 = 1;
    break;
  default:
    ERROR_MESSAGE("neither row-major nor column-major");
    return;
  }

  if(pos >= height)
  {
    if(bFirst)
    {
      pos = height - 1;
    }
    else
    {
      pos = 2;
    }
  }
  else if(pos == 0)
  {
    pos = 2;
  }
  else if(pos % 2)
  {
    ++pos;
  }


  for(std::size_t w = 0; w != width; ++w)
  {
    PatchControl* p1 = tmp.data() + (w*col_stride);
    PatchControl* p2 = m_ctrl.data() + (w*col_stride2);
    for(std::size_t h = 0; h != height; ++h, p2 += row_stride2, p1 += row_stride)
    {
      if(h == pos)
      {
        p2 += 2 * row_stride2;
      }
      *p2 = *p1;
    }

    p1 = tmp.data() + (w*col_stride+pos*row_stride);
    p2 = m_ctrl.data() + (w*col_stride2+pos*row_stride2);
    
    PatchControl* r2a = (p2+row_stride2);
    PatchControl* r2b = (p2-row_stride2);
    PatchControl* c2a = (p1-2*row_stride);
    PatchControl* c2b = (p1-row_stride);

    // set two new row points
    *(p2+2*row_stride2) = *p1;
    *r2a = *c2b;
    
    for(std::size_t i = 0; i != 3; ++i)
    {
      r2a->m_vertex[i] = float_mid(c2b->m_vertex[i], p1->m_vertex[i]);

      r2b->m_vertex[i] = float_mid(c2a->m_vertex[i], c2b->m_vertex[i]);

      p2->m_vertex[i] = float_mid(r2a->m_vertex[i], r2b->m_vertex[i]);
    }
    for(std::size_t i = 0; i != 2; ++i)
    {
      r2a->m_texcoord[i] = float_mid(c2b->m_texcoord[i], p1->m_texcoord[i]);

      r2b->m_texcoord[i] = float_mid(c2a->m_texcoord[i], c2b->m_texcoord[i]);

      p2->m_texcoord[i] = float_mid(r2a->m_texcoord[i], r2b->m_texcoord[i]);
    }
  }
}

void Patch::RemovePoints(EMatrixMajor mt, bool bFirst)
{
  std::size_t width, height, row_stride, col_stride; 

  switch(mt)
  {
  case ROW:
    col_stride = 1;
    row_stride = m_width;
    width = m_width;
    height = m_height;
    break;
  case COL:
    col_stride = m_width;
    row_stride = 1;
    width = m_height;
    height = m_width;
    break;
  default:
    ERROR_MESSAGE("neither row-major nor column-major");
    return;
  }

  std::size_t pos = 0;
  {
    PatchControl* p1 = m_ctrl.data();
    for(std::size_t w = 0; w != width; ++w, p1 += col_stride)
    {
      {
        PatchControl* p2 = p1;
        for(std::size_t h=1; h < height; h += 2, p2 += 2 * row_stride)
        {
          if(0)//p2->m_selectable.isSelected())
          {
            pos = h;
            break;
          }
        }
        if(pos != 0)
        {
          break;
        }
      }
  
      {
        PatchControl* p2 = p1;
        for(std::size_t h=0; h < height; h += 2, p2 += 2 * row_stride)
        {
          if(0)//p2->m_selectable.isSelected())
          {
            pos = h;
            break;
          }
        }
        if(pos != 0)
        {
          break;
        }
      }
    }
  }

  Array<PatchControl> tmp(m_ctrl);

  std::size_t row_stride2, col_stride2;
  switch(mt)
  {
  case ROW:
    setDims(m_width, m_height-2);
    col_stride2 = 1;
    row_stride2 = m_width;
    break;
  case COL:
    setDims(m_width-2, m_height);
    col_stride2 = m_width;
    row_stride2 = 1;
    break;
  default:
    ERROR_MESSAGE("neither row-major nor column-major");
    return;
  }

  if(pos >= height)
  {
    if(bFirst)
    {
      pos=height-3;
    }
    else
    {
      pos=2;
    }
  }
  else if(pos == 0)
  {
    pos=2;
  }
  else if(pos > height - 3)
  {
    pos = height - 3;
  }
  else if(pos % 2)
  {
    ++pos;
  }

  for(std::size_t w = 0; w != width; w++)
  {
    PatchControl* p1 = tmp.data() + (w*col_stride);
    PatchControl* p2 = m_ctrl.data() + (w*col_stride2);
    for(std::size_t h = 0; h != height; ++h, p2 += row_stride2, p1 += row_stride)
    {
      if(h == pos)
      {
        p1 += 2 * row_stride2; h += 2;
      }
      *p2 = *p1;
    }

    p1 = tmp.data() + (w*col_stride+pos*row_stride);
    p2 = m_ctrl.data() + (w*col_stride2+pos*row_stride2);
    
    for(std::size_t i=0; i<3; i++)
    {
      (p2-row_stride2)->m_vertex[i] = ((p1+2*row_stride)->m_vertex[i]+(p1-2*row_stride)->m_vertex[i]) * 0.5f;

      (p2-row_stride2)->m_vertex[i] = (p2-row_stride2)->m_vertex[i]+(2.0f * ((p1)->m_vertex[i]-(p2-row_stride2)->m_vertex[i]));
    }
    for(std::size_t i=0; i<2; i++)
    {
      (p2-row_stride2)->m_texcoord[i] = ((p1+2*row_stride)->m_texcoord[i]+(p1-2*row_stride)->m_texcoord[i]) * 0.5f;

      (p2-row_stride2)->m_texcoord[i] = (p2-row_stride2)->m_texcoord[i]+(2.0f * ((p1)->m_texcoord[i]-(p2-row_stride2)->m_texcoord[i]));
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

void Patch::ProjectTexture(int nAxis)
{
  undoSave();

  int s, t;
  
  switch (nAxis)
  {
  case 2:
    s = 0;
    t = 1;
    break;
  case 0:
    s = 1;
    t = 2;
    break;
  case 1:
    s = 0;
    t = 2;
    break;
  default:
    ERROR_MESSAGE("invalid axis");
    return;
  }

  float fWidth = 1 / (m_state->getTexture().width * Texdef_getDefaultTextureScale());
  float fHeight = 1 / (m_state->getTexture().height * -Texdef_getDefaultTextureScale());

  for(PatchControlIter i = m_ctrl.data(); i != m_ctrl.data() + m_ctrl.size(); ++i)
  {
    (*i).m_texcoord[0] = (*i).m_vertex[s] * fWidth;
    (*i).m_texcoord[1] = (*i).m_vertex[t] * fHeight;
  }

  controlPointsChanged();
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
      glNormal3fv(normal3f_to_array((m_tess.m_vertices.data() + m_tess.m_indices[i*m_tess.m_lenStrips+j])->normal));
      glTexCoord2fv(texcoord2f_to_array((m_tess.m_vertices.data() + m_tess.m_indices[i*m_tess.m_lenStrips+j])->texcoord));
      glVertex3fv(vertex3f_to_array((m_tess.m_vertices.data() + m_tess.m_indices[i*m_tess.m_lenStrips+j])->vertex));
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
            vertex3f_to_vector3((m_tess.m_vertices.data() + (j*width+i))->vertex) +
            normal3f_to_vector3((m_tess.m_vertices.data() + (j*width+i))->normal) * 8
        );
        glVertex3fv(vertex3f_to_array((m_tess.m_vertices.data() + (j*width+i))->vertex));
        glVertex3fv(&vNormal[0]);
      }
      {
        Vector3 vNormal(          
            vertex3f_to_vector3((m_tess.m_vertices.data() + (j*width+i))->vertex) +
            normal3f_to_vector3((m_tess.m_vertices.data() + (j*width+i))->tangent) * 8
        );
        glVertex3fv(vertex3f_to_array((m_tess.m_vertices.data() + (j*width+i))->vertex));
        glVertex3fv(&vNormal[0]);
      }
      {
        Vector3 vNormal(
            vertex3f_to_vector3((m_tess.m_vertices.data() + (j*width+i))->vertex) +
            normal3f_to_vector3((m_tess.m_vertices.data() + (j*width+i))->bitangent) * 8
        );
        glVertex3fv(vertex3f_to_array((m_tess.m_vertices.data() + (j*width+i))->vertex));
        glVertex3fv(&vNormal[0]);
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

        vertex3f_to_vector3(p->vertex) = point.m_vertex;
        texcoord2f_to_vector2(p->texcoord) = point.m_texcoord;

        ArbitraryMeshVertex a, b, c;

        a.vertex = vertex3f_for_vector3(left.m_vertex);
        a.texcoord = texcoord2f_for_vector2(left.m_texcoord);
        b.vertex = vertex3f_for_vector3(right.m_vertex);
        b.texcoord = texcoord2f_for_vector2(right.m_texcoord);

        if(i != 0)
        {
          c.vertex = vertex3f_for_vector3(up.m_vertex);
          c.texcoord = texcoord2f_for_vector2(up.m_texcoord);
        }
        else
        {
          c.vertex = vertex3f_for_vector3(down.m_vertex);
          c.texcoord = texcoord2f_for_vector2(down.m_texcoord);
        }

        Vector3 normal = (right.m_vertex - left.m_vertex).crossProduct(up.m_vertex - down.m_vertex).getNormalised();

        Vector3 tangent, bitangent;
        ArbitraryMeshTriangle_calcTangents(a, b, c, tangent, bitangent);
        vector3_normalise(tangent);
        vector3_normalise(bitangent);
       
        if(((nFlagsX & AVERAGE) != 0 && i == 0) || ((nFlagsY & AVERAGE) != 0  && j == 0))
        {
          p->normal = Normal3f(normal3f_to_vector3(p->normal) + normal.getNormalised());
          p->tangent = Normal3f(normal3f_to_vector3(p->tangent) + tangent.getNormalised());
          p->bitangent = Normal3f((normal3f_to_vector3(p->bitangent) + bitangent).getNormalised());
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
        a.vertex = vertex3f_for_vector3(vertex_0_0);
        a.texcoord = texcoord2f_for_vector2(texcoord_0_0);
        c.vertex = vertex3f_for_vector3(vertex_0_1);
        c.texcoord = texcoord2f_for_vector2(texcoord_0_1);
      }
      else if(!(nFlagsX & DEGEN_1a) || !(nFlagsX & DEGEN_1b))
      {
        tangentU = vertex_1_1 - vertex_1_0;
        a.vertex = vertex3f_for_vector3(vertex_1_0);
        a.texcoord = texcoord2f_for_vector2(texcoord_1_0);
        c.vertex = vertex3f_for_vector3(vertex_1_1);
        c.texcoord = texcoord2f_for_vector2(texcoord_1_1);
      }
      else
      {
        tangentU = vertex_2_1 - vertex_2_0;
        a.vertex = vertex3f_for_vector3(vertex_2_0);
        a.texcoord = texcoord2f_for_vector2(texcoord_2_0);
        c.vertex = vertex3f_for_vector3(vertex_2_1);
        c.texcoord = texcoord2f_for_vector2(texcoord_2_1);
      }

      Vector3 tangentV;

      if((nFlagsY & DEGEN_0a) && (nFlagsY & DEGEN_1a) && (nFlagsY & DEGEN_2a))
      {
        tangentV = vertex_for_index(m_tess.m_vertices, BX->index + offEndY) - tmp;
        b.vertex = vertex3f_for_vector3(tmp);//m_tess.m_vertices[BX->index + offEndY].vertex;
        b.texcoord = texcoord2f_for_vector2(texTmp);//m_tess.m_vertices[BX->index + offEndY].texcoord;
      }
      else
      {
        tangentV = tmp - vertex_for_index(m_tess.m_vertices, BX->index + offStartY);
        b.vertex = vertex3f_for_vector3(tmp);//m_tess.m_vertices[BX->index + offStartY].vertex;
        b.texcoord = texcoord2f_for_vector2(texTmp); //m_tess.m_vertices[BX->index + offStartY].texcoord;
      }
  

      Vector3 normal, s, t;
      ArbitraryMeshVertex& v = m_tess.m_vertices[offStartY + BX->index];
      Vector3& p = normal3f_to_vector3(v.normal);
      Vector3& ps = normal3f_to_vector3(v.tangent);
      Vector3& pt = normal3f_to_vector3(v.bitangent);

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
        a.vertex = vertex3f_for_vector3(vertex_2_0);
        a.texcoord = texcoord2f_for_vector2(texcoord_2_0);
        c.vertex = vertex3f_for_vector3(vertex_2_1);
        c.texcoord = texcoord2f_for_vector2(texcoord_2_1);
      }
      else if(!(nFlagsX & DEGEN_1a) || !(nFlagsX & DEGEN_1b))
      {
        tangentU = vertex_1_1 - vertex_1_0;
        a.vertex = vertex3f_for_vector3(vertex_1_0);
        a.texcoord = texcoord2f_for_vector2(texcoord_1_0);
        c.vertex = vertex3f_for_vector3(vertex_1_1);
        c.texcoord = texcoord2f_for_vector2(texcoord_1_1);
      }
      else
      {
        tangentU = vertex_0_1 - vertex_0_0;
        a.vertex = vertex3f_for_vector3(vertex_0_0);
        a.texcoord = texcoord2f_for_vector2(texcoord_0_0);
        c.vertex = vertex3f_for_vector3(vertex_0_1);
        c.texcoord = texcoord2f_for_vector2(texcoord_0_1);
      }

      Vector3 tangentV;

      if((nFlagsY & DEGEN_0b) && (nFlagsY & DEGEN_1b) && (nFlagsY & DEGEN_2b))
      {
        tangentV = tmp - vertex_for_index(m_tess.m_vertices, BX->index + offStartY);
        b.vertex = vertex3f_for_vector3(tmp);//m_tess.m_vertices[BX->index + offStartY].vertex;
        b.texcoord = texcoord2f_for_vector2(texTmp);//m_tess.m_vertices[BX->index + offStartY].texcoord;
      }
      else
      {
        tangentV = vertex_for_index(m_tess.m_vertices, BX->index + offEndY) - tmp;
        b.vertex = vertex3f_for_vector3(tmp);//m_tess.m_vertices[BX->index + offEndY].vertex;
        b.texcoord = texcoord2f_for_vector2(texTmp);//m_tess.m_vertices[BX->index + offEndY].texcoord;
      }

      ArbitraryMeshVertex& v = m_tess.m_vertices[offEndY+BX->index];
      Vector3& p = normal3f_to_vector3(v.normal);
      Vector3& ps = normal3f_to_vector3(v.tangent);
      Vector3& pt = normal3f_to_vector3(v.bitangent);

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
  vertex.vertex = vertex3f_for_vector3(ctrl.m_vertex);
  vertex.texcoord = texcoord2f_for_vector2(ctrl.m_texcoord);
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
    b.vertex = vertex3f_for_vector3(tangentX[index0]);
    b.texcoord = texcoord2f_for_vector2(tangentS[index0]);
    c.vertex = vertex3f_for_vector3(tangentY[index1]);
    c.texcoord = texcoord2f_for_vector2(tangentT[index1]);

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
