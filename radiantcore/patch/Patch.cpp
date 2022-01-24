#include "Patch.h"

#include "i18n.h"
#include "ipatch.h"
#include "shaderlib.h"
#include "irenderable.h"
#include "itextstream.h"
#include "iselectiontest.h"

#include "registry/registry.h"
#include "math/Frustum.h"
#include "math/Ray.h"
#include "texturelib.h"
#include "brush/TextureProjection.h"
#include "brush/Winding.h"
#include "command/ExecutionFailure.h"
#include "selection/algorithm/Shader.h"
#include "selection/algorithm/Texturing.h"

#include "PatchSavedState.h"
#include "PatchNode.h"

// ====== Helper Functions ==================================================================

inline VertexPointer vertexpointer_arbitrarymeshvertex(const ArbitraryMeshVertex* array) {
  return VertexPointer(&array->vertex, sizeof(ArbitraryMeshVertex));
}

inline const Colour4b colour_for_index(std::size_t i, std::size_t width)
{
	static const Vector3& cornerColourVec = GlobalPatchModule().getSettings().getVertexColour(patch::PatchEditVertexType::Corners);
	static const Vector3& insideColourVec = GlobalPatchModule().getSettings().getVertexColour(patch::PatchEditVertexType::Inside);
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

// Constructor
Patch::Patch(PatchNode& node) :
    _node(node),
    _undoStateSaver(nullptr),
    _transformChanged(false),
    _tesselationChanged(true),
    _shader(texdef_name_default())
{
    construct();
}

// Copy constructor (create this patch from another patch)
Patch::Patch(const Patch& other, PatchNode& node) :
    IPatch(other),
    Bounded(other),
    Snappable(other),
    IUndoable(other),
    _node(node),
    _undoStateSaver(nullptr),
    _transformChanged(false),
    _tesselationChanged(true),
    _shader(other._shader.getMaterialName())
{
    // Initalise the default values
    construct();

    // Copy over the definitions from the <other> patch
    _patchDef3 = other._patchDef3;
    _subDivisions = other._subDivisions;
    setDims(other._width, other._height);
    copy_ctrl(_ctrl.begin(), other._ctrl.begin(), other._ctrl.begin()+(_width*_height));
    _shader.setMaterialName(other._shader.getMaterialName());
    controlPointsChanged();
}

void Patch::construct()
{
    _width = _height = 0;

    _patchDef3 = false;
    _subDivisions = Subdivisions(0, 0);

    // Check, if the shader name is correct
    check_shader();
}

// Get the current control point array
PatchControlArray& Patch::getControlPoints() {
    return _ctrl;
}

// Same as above, just for const arguments
const PatchControlArray& Patch::getControlPoints() const {
    return _ctrl;
}

// Get the (temporary) transformed control point array, not the saved ones
PatchControlArray& Patch::getControlPointsTransformed() {
    return _ctrlTransformed;
}

const PatchControlArray& Patch::getControlPointsTransformed() const {
    return _ctrlTransformed;
}

std::size_t Patch::getWidth() const {
    return _width;
}

std::size_t Patch::getHeight() const {
    return _height;
}

void Patch::setDims(std::size_t w, std::size_t h)
{
  if((w%2)==0)
    w -= 1;
  ASSERT_MESSAGE(w <= MAX_PATCH_WIDTH, "patch too wide");
  if(w > MAX_PATCH_WIDTH)
    w = MAX_PATCH_WIDTH;
  else if(w < MIN_PATCH_WIDTH)
    w = MIN_PATCH_WIDTH;

  if((h%2)==0)
    _height -= 1;
  ASSERT_MESSAGE(h <= MAX_PATCH_HEIGHT, "patch too tall");
  if(h > MAX_PATCH_HEIGHT)
    h = MAX_PATCH_HEIGHT;
  else if(h < MIN_PATCH_HEIGHT)
    h = MIN_PATCH_HEIGHT;

    _width = w;
    _height = h;

    if(_width * _height != _ctrl.size())
    {
        _ctrl.resize(_width * _height);
        _ctrlTransformed.resize(_ctrl.size());
        _node.updateSelectableControls();
    }
}

PatchNode& Patch::getPatchNode()
{
    return _node;
}

void Patch::connectUndoSystem(IUndoSystem& undoSystem)
{
    assert(!_undoStateSaver);

    // Acquire a new state saver
    _undoStateSaver = undoSystem.getStateSaver(*this);
}

// Remove the attached instance and decrease the counters
void Patch::disconnectUndoSystem(IUndoSystem& undoSystem)
{
    assert(_undoStateSaver);

    _undoStateSaver = nullptr;
    undoSystem.releaseStateSaver(*this);
}

// Return the interally stored AABB
const AABB& Patch::localAABB() const
{
    return _localAABB;
}

RenderSystemPtr Patch::getRenderSystem() const
{
    return _renderSystem.lock();
}

void Patch::setRenderSystem(const RenderSystemPtr& renderSystem)
{
    _renderSystem = renderSystem;
    _shader.setRenderSystem(renderSystem);
}

// Implementation of the abstract method of SelectionTestable
// Called to test if the patch can be selected by the mouse pointer
void Patch::testSelect(Selector& selector, SelectionTest& test)
{
    // ensure the tesselation is up to date
    updateTesselation();

    // The updateTesselation routine might have produced a degenerate patch, catch this
    if (_mesh.vertices.empty()) return;

    SelectionIntersection best;
    IndexPointer::index_type* pIndex = &_mesh.indices.front();

    for (std::size_t s=0; s<_mesh.numStrips; s++) {
        test.TestQuadStrip(vertexpointer_arbitrarymeshvertex(&_mesh.vertices.front()), IndexPointer(pIndex, _mesh.lenStrips), best);
        pIndex += _mesh.lenStrips;
    }

    if (best.isValid()) {
        selector.addIntersection(best);
    }
}

// Transform this patch as defined by the transformation matrix <matrix>
void Patch::transform(const Matrix4& matrix)
{
    // Cycle through all the patch control vertices and transform the points
    for (PatchControlIter i = _ctrlTransformed.begin();
         i != _ctrlTransformed.end();
         ++i)
    {
        i->vertex = matrix.transformPoint(i->vertex);
    }

    // Check the handedness of the matrix and invert it if needed
    if(matrix.getHandedness() == Matrix4::LEFTHANDED)
    {
        PatchControlArray_invert(_ctrlTransformed, _width, _height);
    }

    // Mark this patch as changed
    transformChanged();
}

// Called if the patch has changed, so that the dirty flags are set
void Patch::transformChanged()
{
    _transformChanged = true;
    _tesselationChanged = true;
}

// Called to evaluate the transform
void Patch::evaluateTransform()
{
    // Only do something, if the patch really has changed
    if (_transformChanged)
    {
        _transformChanged = false;
        revertTransform();
        _node.evaluateTransform();
    }
}

// Revert the changes, fall back to the saved state in <_ctrl>
void Patch::revertTransform()
{
    _ctrlTransformed = _ctrl;
}

// Apply the transformed control array, save it into <_ctrl> and overwrite the old values
void Patch::freezeTransform()
{
    undoSave();

    // Save the transformed working set array over _ctrl
    _ctrl = _ctrlTransformed;

    // Don't call controlPointsChanged() here since that one will re-apply the
    // current transformation matrix, possible the second time.
    transformChanged();
    updateTesselation();

    for (Observers::iterator i = _observers.begin(); i != _observers.end();)
    {
        (*i++)->onPatchControlPointsChanged();
    }
}

// callback for changed control points
void Patch::controlPointsChanged()
{
    transformChanged();
    evaluateTransform();
    updateTesselation();
    _node.onControlPointsChanged();

    for (Observers::iterator i = _observers.begin(); i != _observers.end();)
    {
        (*i++)->onPatchControlPointsChanged();
    }
}

// Snaps the control points to the grid
void Patch::snapto(float snap)
{
    undoSave();

    for(PatchControlIter i = _ctrl.begin(); i != _ctrl.end(); ++i)
    {
        i->vertex.snap(snap);
    }

    controlPointsChanged();
}

const std::string& Patch::getShader() const
{
    return _shader.getMaterialName();
}

void Patch::setShader(const std::string& name)
{
    undoSave();

    _shader.setMaterialName(name);

    // Check if the shader is ok
    check_shader();
    // Call the callback functions
    textureChanged();
}

const SurfaceShader& Patch::getSurfaceShader() const
{
    return _shader;
}

SurfaceShader& Patch::getSurfaceShader()
{
    return _shader;
}

bool Patch::hasVisibleMaterial() const
{
    if (!_shader.getGLShader()) return false;

    const MaterialPtr& material = _shader.getGLShader()->getMaterial();
    return material && material->isVisible();
}

float Patch::getTextureAspectRatio() const
{
    return _shader.getTextureAspectRatio();
}

int Patch::getShaderFlags() const
{
    if (_shader.getGLShader() != 0)
    {
        return _shader.getGLShader()->getFlags();
    }

    return 0;
}

// Return a defined patch control vertex at <row>,<col>
PatchControl& Patch::ctrlAt(std::size_t row, std::size_t col) {
    return _ctrl[row*_width+col];
}

// The same as above just for const
const PatchControl& Patch::ctrlAt(std::size_t row, std::size_t col) const {
    return _ctrl[row*_width+col];
}

PatchControl& Patch::getTransformedCtrlAt(std::size_t row, std::size_t col)
{
    if (_ctrlTransformed.empty())
    {
        _ctrlTransformed = _ctrl;
    }

    return _ctrlTransformed[row * _width + col];
}

// called just before an action to save the undo state
void Patch::undoSave()
{
    // Notify the undo observer to save this patch state
    if (_undoStateSaver != NULL)
    {
        _undoStateSaver->saveState();
    }
}

// Save the current patch state into a new UndoMemento instance (allocated on heap) and return it to the undo observer
IUndoMementoPtr Patch::exportState() const
{
    return IUndoMementoPtr(new SavedState(_width, _height, _ctrl, _patchDef3, _subDivisions.x(), _subDivisions.y(), _shader.getMaterialName()));
}

// Revert the state of this patch to the one that has been saved in the UndoMemento
void Patch::importState(const IUndoMementoPtr& state)
{
    undoSave();

    const SavedState& other = *(std::static_pointer_cast<SavedState>(state));

    // begin duplicate of SavedState copy constructor, needs refactoring

    // copy construct
    {
        _width = other.m_width;
        _height = other.m_height;
        _ctrl = other.m_ctrl;
        _ctrlTransformed = _ctrl;
        _node.updateSelectableControls();
        _patchDef3 = other.m_patchDef3;
        _subDivisions = Subdivisions(other.m_subdivisions_x, other.m_subdivisions_y);
        _shader.setMaterialName(other._materialName);
    }

    // end duplicate code

    // Notify that this patch has changed
    textureChanged();
    controlPointsChanged();
}

void Patch::check_shader()
{
    if (!shader_valid(getShader().c_str()))
    {
        rError() << "patch has invalid texture name: '" << getShader() << "'\n";
    }
}

// Patch Destructor
Patch::~Patch()
{
    for (Observers::iterator i = _observers.begin(); i != _observers.end();)
    {
        (*i++)->onPatchDestruction();
    }
}

bool Patch::isValid() const
{
  if(!_width || !_height)
  {
    return false;
  }

  for(PatchControlConstIter i = _ctrl.begin(); i != _ctrl.end(); ++i)
  {
    if(!double_valid((*i).vertex.x())
      || !double_valid((*i).vertex.y())
      || !double_valid((*i).vertex.z())
      || !double_valid((*i).texcoord.x())
      || !double_valid((*i).texcoord.y()))
    {
      rError() << "patch has invalid control points\n";
      return false;
    }
  }
  return true;
}

bool Patch::isDegenerate() const {

    if (!isValid()) {
        // Invalid patches are also "degenerate"
        return true;
    }

    Vector3 prev(0,0,0);

    // Compare each control's 3D coordinates with the previous one and break out
    // on the first non-equal one
    for (PatchControlConstIter i = _ctrl.begin(); i != _ctrl.end(); ++i) {

        // Skip the first comparison
        if (i != _ctrl.begin() && !math::isNear(i->vertex, prev, 0.0001)) {
            return false;
        }

        // Remember the coords of this vertex
        prev = i->vertex;
    }

    // The loop went through, all vertices the same
    return true;
}

void Patch::updateTesselation(bool force)
{
    // Only do something if the tesselation has actually changed
    if (!_tesselationChanged && !force) return;

    _tesselationChanged = false;

    if (!isValid())
    {
        _mesh.clear();
        _localAABB = AABB();
        return;
    }

    // Run the tesselation code
    _mesh.generate(_width, _height, _ctrlTransformed, subdivisionsFixed(), getSubdivisions(), _node.getRenderEntity());

    updateAABB();

    _node.onTesselationChanged();
}

void Patch::invertMatrix()
{
  undoSave();

  PatchControlArray_invert(_ctrl, _width, _height);

  controlPointsChanged();
}

void Patch::transposeMatrix()
{
    undoSave();

    // greebo: create a new temporary control array to hold the "old" matrix
    PatchControlArray tmp = _ctrl;

    std::size_t i = 0;

    for (std::size_t w = 0; w < _width; ++w)
    {
        for (std::size_t h = 0; h < _height; ++h)
        {
            // Copy elements such that the columns end up as rows
            _ctrl[i++] = tmp[h*_width + w];
        }
    }

    std::swap(_width, _height);

    controlPointsChanged();
}

void Patch::Redisperse(EMatrixMajor mt)
{
  std::size_t w, h, width, height, row_stride, col_stride;
  PatchControlIter p1, p2, p3;

  undoSave();

  switch(mt)
  {
  case COL:
    width = (_width-1)>>1;
    height = _height;
    col_stride = 1;
    row_stride = _width;
    break;
  case ROW:
    width = (_height-1)>>1;
    height = _width;
    col_stride = _width;
    row_stride = 1;
    break;
  default:
    ERROR_MESSAGE("neither row-major nor column-major");
    return;
  }

  for(h=0;h<height;h++)
  {
    p1 = _ctrl.begin()+(h*row_stride);
    for(w=0;w<width;w++)
    {
      p2 = p1+col_stride;
      p3 = p2+col_stride;
      p2->vertex = math::midPoint(p1->vertex, p3->vertex);
      p1 = p3;
    }
  }

  controlPointsChanged();
}

void Patch::redisperseRows()
{
	Redisperse(ROW);
}

void Patch::redisperseColumns()
{
	Redisperse(COL);
}

void Patch::insertRemove(bool insert, bool column, bool first)
{
    undoSave();

	try
	{
		if (insert)
		{
			// Decide whether we should insert rows or columns
			if (column) {
				// The insert point is 1 for "beginning" and width-2 for "end"
				insertColumns(first ? 1 : _width-2);
			}
			else {
				// The insert point is 1 for "beginning" and height-2 for "end"
				insertRows(first ? 1 : _height-2);
			}
		}
		else {
			// Col/Row Removal
			if (column) {
				// Column removal, pass TRUE
				removePoints(true, first ? 2 : _width - 3);
			}
			else {
				// Row removal, pass FALSE
				removePoints(false, first ? 2 : _height - 3);
			}
		}
	}
	catch (const GenericPatchException& g) {
		rError() << "Error manipulating patch dimensions: " << g.what() << "\n";
	}

    controlPointsChanged();
}

void Patch::appendPoints(bool columns, bool beginning) {
    bool rows = !columns; // Shortcut for readability

    if ((columns && _width + 2 > MAX_PATCH_WIDTH) ||
        (rows && _height + 2 > MAX_PATCH_HEIGHT))
    {
        rError() << "Patch::appendPoints() error: " <<
                               "Cannot make patch any larger.\n";
        return;
    }

    // Sanity check passed, now start the action
    undoSave();

    // Create a backup of the old control vertices
    PatchControlArray oldCtrl = _ctrl;
    std::size_t oldHeight = _height;
    std::size_t oldWidth = _width;

    // Resize this patch
    setDims(columns ? oldWidth+2 : oldWidth, rows ? oldHeight+2 : oldHeight);

    // Specify the target row to copy the values to
    std::size_t targetColStart = (columns && beginning) ? 2 : 0;
    std::size_t targetRowStart = (rows && beginning) ? 2 : 0;

    // We're copying the old patch matrix into a sub-matrix of the new patch
    // Fill in the control vertex values into the target area using this loop
    for (std::size_t newRow = targetRowStart, oldRow = 0;
         newRow < _height && oldRow < oldHeight;
         newRow++, oldRow++)
    {
        for (std::size_t newCol = targetColStart, oldCol = 0;
             oldCol < oldWidth && newCol < _width;
             oldCol++, newCol++)
        {
            // Copy the control vertex from the old patch to the new patch
            ctrlAt(newRow, newCol).vertex = oldCtrl[oldRow*oldWidth + oldCol].vertex;
            ctrlAt(newRow, newCol).texcoord = oldCtrl[oldRow*oldWidth + oldCol].texcoord;
        }
    }

    if (columns) {
        // Extrapolate the vertex attributes of the columns

        // These are the indices of the new columns
        std::size_t newCol1 = beginning ? 0 : _width - 1; // The outermost column
        std::size_t newCol2 = beginning ? 1 : _width - 2; // The nearest column

        // This indicates the direction we are taking the base values from
        // If we start at the beginning, we have to take the values on
        // the "right", hence the +1 index
        int neighbour = beginning ? +1 : -1;

        for (std::size_t row = 0; row < _height; row++) {
            // The distance of the two neighbouring columns,
            // this is taken as extrapolation value
            Vector3 vertexDiff = ctrlAt(row, newCol2 + neighbour).vertex -
                                 ctrlAt(row, newCol2 + 2*neighbour).vertex;
            Vector2 texDiff = ctrlAt(row, newCol2 + neighbour).texcoord -
                              ctrlAt(row, newCol2 + 2*neighbour).texcoord;

            // Extrapolate the values of the nearest column
            ctrlAt(row, newCol2).vertex = ctrlAt(row, newCol2 + neighbour).vertex + vertexDiff;
            ctrlAt(row, newCol2).texcoord = ctrlAt(row, newCol2 + neighbour).texcoord + texDiff;

            // Extrapolate once again linearly from the nearest column to the outermost column
            ctrlAt(row, newCol1).vertex = ctrlAt(row, newCol2).vertex + vertexDiff;
            ctrlAt(row, newCol1).texcoord = ctrlAt(row, newCol2).texcoord + texDiff;
        }
    }
    else {
        // Extrapolate the vertex attributes of the rows

        // These are the indices of the new rows
        std::size_t newRow1 = beginning ? 0 : _height - 1; // The outermost row
        std::size_t newRow2 = beginning ? 1 : _height - 2; // The nearest row

        // This indicates the direction we are taking the base values from
        // If we start at the beginning, we have to take the values on
        // the "right", hence the +1 index
        int neighbour = beginning ? +1 : -1;

        for (std::size_t col = 0; col < _width; col++) {
            // The distance of the two neighbouring rows,
            // this is taken as extrapolation value
            Vector3 vertexDiff = ctrlAt(newRow2 + neighbour, col).vertex -
                                 ctrlAt(newRow2 + 2*neighbour, col).vertex;
            Vector2 texDiff = ctrlAt(newRow2 + neighbour, col).texcoord -
                              ctrlAt(newRow2 + 2*neighbour, col).texcoord;

            // Extrapolate the values of the nearest row
            ctrlAt(newRow2, col).vertex = ctrlAt(newRow2 + neighbour, col).vertex + vertexDiff;
            ctrlAt(newRow2, col).texcoord = ctrlAt(newRow2 + neighbour, col).texcoord + texDiff;

            // Extrapolate once again linearly from the nearest row to the outermost row
            ctrlAt(newRow1, col).vertex = ctrlAt(newRow2, col).vertex + vertexDiff;
            ctrlAt(newRow1, col).texcoord = ctrlAt(newRow2, col).texcoord + texDiff;
        }
    }

    controlPointsChanged();
}

Patch* Patch::MakeCap(Patch* patch, patch::CapType eType, EMatrixMajor mt, bool bFirst)
{
  std::size_t i, width, height;

  switch(mt)
  {
  case ROW:
    width = _width;
    height = _height;
    break;
  case COL:
    width = _height;
    height = _width;
    break;
  default:
    ERROR_MESSAGE("neither row-major nor column-major");
    return 0;
  }

  std::vector<Vector3> p(width);

  std::size_t nIndex = (bFirst) ? 0 : height-1;
  if(mt == ROW)
  {
    for (i=0; i<width; i++)
    {
      p[(bFirst) ? i : (width-1) - i] = ctrlAt(nIndex, i).vertex;
    }
  }
  else
  {
    for (i=0; i<width; i++)
    {
      p[(bFirst) ? i : (width-1) - i] = ctrlAt(i, nIndex).vertex;
    }
  }

  patch->ConstructSeam(eType, &p.front(), width);

  // greebo: Apply natural texture to that patch, to fix the texcoord==1.#INF bug.
  patch->scaleTextureNaturally();
  return patch;
}

void Patch::flipTexture(int nAxis)
{
    selection::algorithm::TextureFlipper::FlipPatch(*this, nAxis);
}

/** greebo: Helper function that shifts all control points in
 * texture space about <s,t>
 */
void Patch::translateTexCoords(const Vector2& translation)
{
	// Cycle through all control points and shift them in texture space
	for (PatchControlIter i = _ctrl.begin(); i != _ctrl.end(); ++i)
	{
    	i->texcoord += translation;
  	}
}

void Patch::translateTexture(float s, float t)
{
    undoSave();

    s = -1 * s / _shader.getWidth();
    t = t / _shader.getHeight();

    translateTexCoords(Vector2(s,t));

    controlPointsChanged();
}

void Patch::scaleTexture(float s, float t)
{
    selection::algorithm::TextureScaler::ScalePatch(*this, { s, t });
}

void Patch::rotateTexture(float angle)
{
    selection::algorithm::TextureRotator::RotatePatch(*this, degrees_to_radians(angle));
}

void Patch::fitTexture(float s, float t)
{
    // Save the current patch state to the undoMemento
    undoSave();

    /* greebo: Calculate the texture width and height increment per control point.
     * If we have a 4x4 patch and want to tile it 3x3, the distance
     * from one control point to the next one has to cover 3/4 of a full texture,
     * hence texture_x_repeat/patch_width and texture_y_repeat/patch_height.*/
    float sIncr = s / static_cast<float>(_width - 1);
    float tIncr = t / static_cast<float>(_height - 1);

    // Set the pointer to the first control point
    PatchControlIter pDest = _ctrl.begin();

    float tc = 0;

    // Cycle through the patch matrix (row per row)
    // Increment the <tc> counter by <tIncr> increment
    for (std::size_t h=0; h < _height; h++, tc += tIncr)
    {
        float sc = 0;

        // Cycle through the row points: reset sc to zero
        // and increment it by sIncr at each step.
        for (std::size_t w = 0; w < _width; w++, sc += sIncr)
        {
            // Set the texture coordinates
            pDest->texcoord[0] = sc;
            pDest->texcoord[1] = tc;
            // Set the pointer to the next control point
            pDest++;
        }
    }

    // Notify the patch
    controlPointsChanged();
}

void Patch::scaleTextureNaturally()
{
    // Save the undo memento
    undoSave();

    // Retrieve the default scale from the registry
    auto defaultScale = registry::getValue<float>("user/ui/textures/defaultTextureScale");

    // Cycles through all the patch columns and assigns s/t coordinates.
    // During each column or row cycle, the highest world distance between columns or rows 
    // determines the distance in UV space (longest distance is taken).
    // World distances are scaled to UV space with the actual texture width/height,
    // scaled by the value in the registry.

    auto horizScale = 1.0f / (static_cast<float>(_shader.getWidth()) * defaultScale);

    double texcoordX = 0;

    // Cycle through the patch width,
    for (std::size_t w = 0; w < _width; w++)
    {
        // Apply the currently active <tex> value to the control point texture coordinates.
        for (std::size_t h = 0; h < _height; h++)
        {
            // Set the x-coord (or better s-coord?) of the texture to tex.
            // For the first width cycle this is tex=0, so the texture is not shifted at the first vertex
            ctrlAt(h, w).texcoord[0] = texcoordX;
        }

        // If we reached the last row (_width - 1) we are finished (all coordinates are applied)
        if (w + 1 == _width) break;

        // Determine the texcoord of the next column
        double highestNextTexCoord = 0;

        // Determine the longest distance to the next column.
        // Again, cycle through the current column
        for (std::size_t h = 0; h < _height; h++)
        {
            // v is the vector pointing from one control point to the next neighbour
            auto worldDistance = ctrlAt(h, w).vertex - ctrlAt(h, w + 1).vertex;

            // Scale the distance in world coordinates into texture coords
            double nextTexcoordX = texcoordX + worldDistance.getLength() * horizScale;

            // Use the farthest extrapolated texture cooord
            highestNextTexCoord = std::max(highestNextTexCoord, nextTexcoordX);
        }

        // Remember the highest found texcoord, assign it to the next column
        texcoordX = highestNextTexCoord;
    }

    // Now the same goes for the texture height, cycle through all the rows
    // and calculate the longest distances, convert them to texture coordinates
    // and apply them to the according texture coordinates.
    auto vertScale = 1.0f / (static_cast<float>(_shader.getHeight()) * defaultScale);

    double texcoordY = 0;

    // Each row is visited once
    for (std::size_t h = 0; h < _height; h++)
    {
        // Visit every vertex in this row, assigning the current texCoordY
        for (std::size_t w = 0; w < _width; w++)
        {
            ctrlAt(h, w).texcoord[1] = -texcoordY;
        }

        if (h + 1 == _height) break;

        double highestNextTexCoord = 0;

        for (std::size_t w = 0; w < _width; w++)
        {
            auto worldDistance = ctrlAt(h, w).vertex - ctrlAt(h + 1, w).vertex;

            double nextTexcoordY = texcoordY + worldDistance.getLength() * vertScale;

            highestNextTexCoord = std::max(highestNextTexCoord, nextTexcoordY);
        }

        texcoordY = highestNextTexCoord;
    }

    // Notify the patch that it control points got changed
    controlPointsChanged();
}

void Patch::updateAABB()
{
    AABB aabb;

    for(PatchControlIter i = _ctrlTransformed.begin(); i != _ctrlTransformed.end(); ++i)
    {
        aabb.includePoint(i->vertex);
    }

    // greebo: Only trigger the callbacks if the bounds actually changed
    if (_localAABB != aabb)
    {
        _localAABB = aabb;

        _node.boundsChanged();
    }
}

// Inserts two columns before and after the column having the index <colIndex>
void Patch::insertColumns(std::size_t colIndex) {
    if (colIndex == 0 || colIndex == _width) {
        throw GenericPatchException("Patch::insertColumns: can't insert at this index.");
    }

    if (_width + 2 > MAX_PATCH_WIDTH) {
        throw GenericPatchException("Patch::insertColumns: patch has too many columns.");
    }

    // Create a backup of the old control vertices
    PatchControlArray oldCtrl = _ctrl;
    std::size_t oldHeight = _height;
    std::size_t oldWidth = _width;

    // Resize this patch
    setDims(oldWidth + 2, oldHeight);

    // Now fill in the control vertex values and interpolate
    // before and after the insert point.
    for (std::size_t row = 0; row < _height; row++) {

        for (std::size_t newCol = 0, oldCol = 0;
             newCol < _width && oldCol < oldWidth;
             newCol++, oldCol++)
        {
            // Is this the insert point?
            if (oldCol == colIndex) {
                // Left column (to be interpolated)
                ctrlAt(row, newCol).vertex = float_mid(
                    oldCtrl[row*oldWidth + oldCol - 1].vertex,
                    oldCtrl[row*oldWidth + oldCol].vertex
                );
                ctrlAt(row, newCol).texcoord = float_mid(
                    oldCtrl[row*oldWidth + oldCol - 1].texcoord,
                    oldCtrl[row*oldWidth + oldCol].texcoord
                );

                // Set the newCol counter to the middle column
                newCol++;
                ctrlAt(row, newCol).vertex = oldCtrl[row*oldWidth + oldCol].vertex;
                ctrlAt(row, newCol).texcoord = oldCtrl[row*oldWidth + oldCol].texcoord;

                // Set newCol to the right column (to be interpolated)
                newCol++;
                ctrlAt(row, newCol).vertex = float_mid(
                    oldCtrl[row*oldWidth + oldCol].vertex,
                    oldCtrl[row*oldWidth + oldCol + 1].vertex
                );
                ctrlAt(row, newCol).texcoord = float_mid(
                    oldCtrl[row*oldWidth + oldCol].texcoord,
                    oldCtrl[row*oldWidth + oldCol + 1].texcoord
                );
            }
            else {
                // No special column, just copy the control vertex
                ctrlAt(row, newCol).vertex = oldCtrl[row*oldWidth + oldCol].vertex;
                ctrlAt(row, newCol).texcoord = oldCtrl[row*oldWidth + oldCol].texcoord;
            }
        }
    }
}

// Inserts two rows before and after the column having the index <colIndex>
void Patch::insertRows(std::size_t rowIndex) {
    if (rowIndex == 0 || rowIndex == _height) {
        throw GenericPatchException("Patch::insertRows: can't insert at this index.");
    }

    if (_height + 2 > MAX_PATCH_HEIGHT) {
        throw GenericPatchException("Patch::insertRows: patch has too many rows.");
    }

    // Create a backup of the old control vertices
    PatchControlArray oldCtrl = _ctrl;
    std::size_t oldHeight = _height;
    std::size_t oldWidth = _width;

    // Resize this patch
    setDims(oldWidth, oldHeight + 2);

    // Now fill in the control vertex values and interpolate
    // before and after the insert point.
    for (std::size_t col = 0; col < _width; col++) {

        for (std::size_t newRow = 0, oldRow = 0;
             newRow < _height && oldRow < oldHeight;
             newRow++, oldRow++)
        {
            // Is this the insert point?
            if (oldRow == rowIndex) {
                // the column above the insert point (to be interpolated)
                ctrlAt(newRow, col).vertex = float_mid(
                    oldCtrl[(oldRow-1)*oldWidth + col].vertex,
                    oldCtrl[oldRow*oldWidth + col].vertex
                );
                ctrlAt(newRow, col).texcoord = float_mid(
                    oldCtrl[(oldRow-1)*oldWidth + col].texcoord,
                    oldCtrl[oldRow*oldWidth + col].texcoord
                );

                // Set the newRow counter to the middle row
                newRow++;
                ctrlAt(newRow, col).vertex = oldCtrl[oldRow*oldWidth + col].vertex;
                ctrlAt(newRow, col).texcoord = oldCtrl[oldRow*oldWidth + col].texcoord;

                // Set newRow to the lower column (to be interpolated)
                newRow++;
                ctrlAt(newRow, col).vertex = float_mid(
                    oldCtrl[oldRow*oldWidth + col].vertex,
                    oldCtrl[(oldRow+1)*oldWidth + col].vertex
                );
                ctrlAt(newRow, col).texcoord = float_mid(
                    oldCtrl[oldRow*oldWidth + col].texcoord,
                    oldCtrl[(oldRow+1)*oldWidth + col].texcoord
                );
            }
            else {
                // No special column, just copy the control vertex
                ctrlAt(newRow, col).vertex = oldCtrl[oldRow*oldWidth + col].vertex;
                ctrlAt(newRow, col).texcoord = oldCtrl[oldRow*oldWidth + col].texcoord;
            }
        }
    }
}

// Removes the two rows before and after the column/row having the index <index>
void Patch::removePoints(bool columns, std::size_t index) {
    bool rows = !columns; // readability shortcut ;)

    if ((columns && _width<5) || (!columns && _height < 5))
    {
        throw GenericPatchException("Patch::removePoints: can't remove any more rows/columns.");
    }

    // Check column index bounds
    if (columns && (index < 2 || index > _width - 3)) {
        throw GenericPatchException("Patch::removePoints: can't remove columns at this index.");
    }

    // Check row index bounds
    if (rows && (index < 2 || index > _height - 3)) {
        throw GenericPatchException("Patch::removePoints: can't remove rows at this index.");
    }

    // Create a backup of the old control vertices
    PatchControlArray oldCtrl = _ctrl;
    std::size_t oldHeight = _height;
    std::size_t oldWidth = _width;

    // Resize this patch
    setDims(columns ? oldWidth - 2 : oldWidth, rows ? oldHeight - 2 : oldHeight);

    // Now fill in the control vertex values and skip
    // the rows/cols before and after the remove point.
    for (std::size_t newRow = 0, oldRow = 0;
         newRow < _height && oldRow < oldHeight;
         newRow++, oldRow++)
    {
        // Skip the row before and after the removal point
        if (rows && (oldRow == index - 1 || oldRow == index + 1)) {
            // Increase the old row pointer by 1
            oldRow++;
        }

        for (std::size_t oldCol = 0, newCol = 0;
             oldCol < oldWidth && newCol < _width;
             oldCol++, newCol++)
        {
            // Skip the column before and after the removal point
            if (columns && (oldCol == index - 1 || oldCol == index + 1)) {
                // Increase the old row pointer by 1
                oldCol++;
            }

            // Copy the control vertex from the old patch to the new patch
            ctrlAt(newRow, newCol).vertex = oldCtrl[oldRow*oldWidth + oldCol].vertex;
            ctrlAt(newRow, newCol).texcoord = oldCtrl[oldRow*oldWidth + oldCol].texcoord;
        }
    }
}

void Patch::ConstructSeam(patch::CapType eType, Vector3* p, std::size_t width)
{
  switch(eType)
  {
  case patch::CapType::InvertedBevel:
    {
      setDims(3, 3);
      _ctrl[0].vertex = p[0];
      _ctrl[1].vertex = p[1];
      _ctrl[2].vertex = p[1];
      _ctrl[3].vertex = p[1];
      _ctrl[4].vertex = p[1];
      _ctrl[5].vertex = p[1];
      _ctrl[6].vertex = p[2];
      _ctrl[7].vertex = p[1];
      _ctrl[8].vertex = p[1];
    }
    break;
  case patch::CapType::Bevel:
    {
      setDims(3, 3);
      Vector3 p3(p[2] + (p[0] - p[1]));
      _ctrl[0].vertex = p3;
      _ctrl[1].vertex = p3;
      _ctrl[2].vertex = p[2];
      _ctrl[3].vertex = p3;
      _ctrl[4].vertex = p3;
      _ctrl[5].vertex = p[1];
      _ctrl[6].vertex = p3;
      _ctrl[7].vertex = p3;
      _ctrl[8].vertex = p[0];
    }
    break;
  case patch::CapType::EndCap:
    {
      Vector3 p5(math::midPoint(p[0], p[4]));

      setDims(3, 3);
      _ctrl[0].vertex = p[0];
      _ctrl[1].vertex = p5;
      _ctrl[2].vertex = p[4];
      _ctrl[3].vertex = p[1];
      _ctrl[4].vertex = p[2];
      _ctrl[5].vertex = p[3];
      _ctrl[6].vertex = p[2];
      _ctrl[7].vertex = p[2];
      _ctrl[8].vertex = p[2];
    }
    break;
  case patch::CapType::InvertedEndCap:
    {
      setDims(5, 3);
      _ctrl[0].vertex = p[4];
      _ctrl[1].vertex = p[3];
      _ctrl[2].vertex = p[2];
      _ctrl[3].vertex = p[1];
      _ctrl[4].vertex = p[0];
      _ctrl[5].vertex = p[3];
      _ctrl[6].vertex = p[3];
      _ctrl[7].vertex = p[2];
      _ctrl[8].vertex = p[1];
      _ctrl[9].vertex = p[1];
      _ctrl[10].vertex = p[3];
      _ctrl[11].vertex = p[3];
      _ctrl[12].vertex = p[2];
      _ctrl[13].vertex = p[1];
      _ctrl[14].vertex = p[1];
    }
    break;
  case patch::CapType::Cylinder:
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
        PatchControlIter pCtrl = _ctrl.begin();
        for(std::size_t i = 0; i != _height; ++i, pCtrl += _width)
        {
          pCtrl->vertex = p[i];
        }
      }
      {
        PatchControlIter pCtrl = _ctrl.begin() + 2;
        std::size_t h = _height - 1;
        for(std::size_t i = 0; i != _height; ++i, pCtrl += _width)
        {
          pCtrl->vertex = p[h + (h - i)];

          if (i == _height - 1) break; // prevent iterator from being incremented post bounds
        }
      }

      Redisperse(COL);
    }
    break;
  default:
    ERROR_MESSAGE("invalid patch-cap type");
    return;
  }
  controlPointsChanged();
}

// greebo: Calculates the nearest patch CORNER vertex from the given <point>
// Note: if this routine returns end(), something's rotten with the patch
PatchControlIter Patch::getClosestPatchControlToPoint(const Vector3& point) {

    PatchControlIter pBest = end();

    // Initialise with an illegal distance value
    double closestDist = -1.0;

    PatchControlIter corners[4] = {
        _ctrl.begin(),
        _ctrl.begin() + (_width-1),
        _ctrl.begin() + (_width*(_height-1)),
        _ctrl.begin() + (_width*_height - 1)
    };

    // Cycle through all the control points with an iterator
    //for (PatchControlIter i = _ctrl.begin(); i != _ctrl.end(); ++i) {
    for (unsigned int i = 0; i < 4; i++) {

        // Calculate the distance of the current vertex
        double candidateDist = (corners[i]->vertex - point).getLength();

        // Compare the distance to the currently closest one
        if (candidateDist < closestDist || pBest == end()) {
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
 * @returns: a pointer to the nearest patch face. (Can technically be end(), but really should not happen).*/
PatchControlIter Patch::getClosestPatchControlToPatch(const Patch& patch) {

    // A pointer to the patch vertex closest to the patch
    PatchControlIter pBest = end();

    // Initialise the best distance with an illegal value
    double closestDist = -1.0;

    // Cycle through the winding vertices and calculate the distance to each patch vertex
    for (PatchControlConstIter i = patch.begin(); i != patch.end(); ++i)
    {
        // Retrieve the vertex
        const Vector3& patchVertex = i->vertex;

        // Get the nearest control point to the current otherpatch vertex
        PatchControlIter candidate = getClosestPatchControlToPoint(patchVertex);

        if (candidate != end())
        {
            double candidateDist = (patchVertex - candidate->vertex).getLength();

            // If we haven't found a best patch control so far or
            // the candidate distance is even better, save it!
            if (pBest == end() || candidateDist < closestDist)
            {
                // Memorise this patch control
                pBest = candidate;
                closestDist =  candidateDist;
            }
        }
    } // end for

    return pBest;
}

/* greebo: This calculates the nearest patch control to the given brush <face>
 *
 * @returns: a pointer to the nearest patch face. (Can technically be end(), but really should not happen).*/
PatchControlIter Patch::getClosestPatchControlToFace(const Face* face)
{
    // A pointer to the patch vertex closest to the face
    PatchControlIter pBest = end();

    // Initialise the best distance with an illegal value
    double closestDist = -1.0;

    // Check for NULL pointer, just to make sure
    if (face != NULL)
    {
        // Retrieve the winding from the brush face
        const Winding& winding = face->getWinding();

        // Cycle through the winding vertices and calculate the distance to each patch vertex
        for (Winding::const_iterator i = winding.begin(); i != winding.end(); ++i) {
            // Retrieve the vertex
            const Vector3& faceVertex = i->vertex;

            // Get the nearest control point to the current face vertex
            PatchControlIter candidate = getClosestPatchControlToPoint(faceVertex);

            if (candidate != end())
            {
                double candidateDist = (faceVertex - candidate->vertex).getLength();

                // If we haven't found a best patch control so far or
                // the candidate distance is even better, save it!
                if (pBest == end() || candidateDist < closestDist)
                {
                    // Memorise this patch control
                    pBest = candidate;
                    closestDist =  candidateDist;
                }
            }
        } // end for
    }

    return pBest;
}

Vector2 Patch::getPatchControlArrayIndices(const PatchControlIter& control)
{
    std::size_t count = 0;

    // Go through the patch column per column and find the control vertex
    for (PatchControlIter p = _ctrl.begin(); p != _ctrl.end(); ++p, ++count)
    {
        // Compare the iterators to check if we have found the control
        if (p == control)
        {
            int row = static_cast<int>(floor(static_cast<float>(count) / _width));
            int col = static_cast<int>(count % _width);

            return Vector2(col, row);
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
    Vector3 texcoord = worldToTexture.transformPoint(projection);

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
void Patch::pasteTextureNatural(const Face* face)
{
    // Check for NULL pointers
    if (face == nullptr) return;

    // Convert the size_t stuff into int, because we need it for signed comparisons
    int patchHeight = static_cast<int>(_height);
    int patchWidth = static_cast<int>(_width);

    // Get the plane and its normalised normal vector of the face
    Plane3 plane = face->getPlane().getPlane().getNormalised();
    Vector3 faceNormal = plane.normal();

    // Get the conversion matrix from the FaceTextureDef, the local2World argument is the identity matrix
    Matrix4 worldToTexture = face->getProjection().getWorldToTexture(faceNormal, Matrix4::getIdentity());

    // Calculate the nearest corner vertex of this patch (to the face's winding vertices)
    PatchControlIter nearestControl = getClosestPatchControlToFace(face);

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

    PatchControl* startControl = &_ctrl[(patchWidth*hStart) + wStart];

    // Calculate the base directions that are used to "flatten" the patch
    // These have to be orthogonal to the facePlane normal, so that the texture coordinates
    // can be retrieved by projection onto the facePlane.

    // Get the control points of the next column and the next row
    PatchControl& nextColumn = _ctrl[(patchWidth*(hStart + hIncr)) + wStart];
    PatchControl& nextRow = _ctrl[(patchWidth*hStart) + (wStart + wIncr)];

    // Calculate the world direction of these control points and extract a base
    Vector3 widthVector = (nextRow.vertex - startControl->vertex);
    Vector3 heightVector = (nextColumn.vertex - startControl->vertex);

    if (widthVector.getLength() == 0.0f || heightVector.getLength() == 0.0f)
    {
		throw cmd::ExecutionFailure(
			_("Sorry. Patch is not suitable for this kind of operation.")
	    );
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
    Vector3 prevColumnVirtualVertex = prevColumn->vertex;

    for (int w = wStart; w != wEnd; w += wIncr) {

        // The first control in this row, calculate its virtual coords
        PatchControl* curColumn = &_ctrl[(patchWidth*hStart) + w];

        // The distance between the last column and this column
        double xyzColDist = (curColumn->vertex - prevColumn->vertex).getLength();

        // The vector pointing to the next control point, if it *was* a completely planar patch
        Vector3 curColumnVirtualVertex = prevColumnVirtualVertex + widthBase * xyzColDist;

        // Store this value for the upcoming column cycle
        PatchControl* prevRow = curColumn;
        Vector3 prevRowVirtualVertex = curColumnVirtualVertex;

        // Cycle through all the columns
        for (int h = hStart; h != hEnd; h += hIncr) {

            // The current control
            PatchControl* control = &_ctrl[(patchWidth*h) + w];

            // The distance between the last and the current vertex
            double xyzRowDist = (control->vertex - prevRow->vertex).getLength();

            // The vector pointing to the next control point, if it *was* a completely planar patch
            Vector3 virtualControlVertex = prevRowVirtualVertex + heightBase * xyzRowDist;

            // Project the virtual vertex onto the brush faceplane and transform it into texture space
            control->texcoord = getProjectedTextureCoords(virtualControlVertex, plane, worldToTexture);

            // Update the variables for the next loop
            prevRow = control;
            prevRowVirtualVertex = virtualControlVertex;
        }

        // Set the prevColumn control vertex to this one
        prevColumn = curColumn;
        prevColumnVirtualVertex = curColumnVirtualVertex;
    }

    // Notify the patch about the change
    controlPointsChanged();
}

void Patch::pasteTextureNatural(Patch& sourcePatch) {
    // Save the undo memento
    undoSave();

    // Convert the size_t stuff into int, because we need it for signed comparisons
    int patchHeight = static_cast<int>(_height);
    int patchWidth = static_cast<int>(_width);

    // Calculate the nearest corner vertex of this patch (to the sourcepatch vertices)
    PatchControlIter nearestControl = getClosestPatchControlToPatch(sourcePatch);

    PatchControlIter refControl = sourcePatch.getClosestPatchControlToPatch(*this);

    Vector2 texDiff = refControl->texcoord - nearestControl->texcoord;

    for (int col = 0; col < patchWidth; col++) {
        for (int row = 0; row < patchHeight; row++) {
            // Substract the texture coord difference from each control vertex
            ctrlAt(row, col).texcoord += texDiff;
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
        Plane3 plane = face->getPlane().getPlane().getNormalised();

        // Get the (already normalised) facePlane normal
        Vector3 faceNormal = plane.normal();

        // Get the conversion matrix from the FaceTextureDef, the local2World argument is the identity matrix
        Matrix4 worldToTexture = face->getProjection().getWorldToTexture(faceNormal, Matrix4::getIdentity());

        // Cycle through all the control points with an iterator
        for (PatchControlIter i = _ctrl.begin(); i != _ctrl.end(); ++i) {
            // Project the vertex onto the face plane and transform it into texture space
            i->texcoord = getProjectedTextureCoords(i->vertex, plane, worldToTexture);
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

        if (otherPatch->getWidth() == _width && otherPatch->getHeight() == _height) {

            PatchControlConstIter other;
            PatchControlIter self;

            // Clone the texture coordinates one by one
            for (other = otherPatch->begin(), self = _ctrl.begin();
                 other != otherPatch->end();
                 ++other, ++self)
            {
                self->texcoord = other->texcoord;
            }

            // Notify the patch about the change
            controlPointsChanged();
        }
        else {
            rMessage() << "Error: Cannot copy texture coordinates, patch dimensions must match!\n";
        }
    }
}

void Patch::alignTexture(AlignEdge align)
{
    if (isDegenerate()) return;

    // A 5x3 patch has (5-1)x2 + (3-1)x2 edges at the border

    // The edges in texture space, sorted the same as in the winding
    std::vector<Vector2> texEdges;
    std::vector<Vector2> texCoords;

    // Calculate all edges in texture space
    for (std::size_t h = 0; h < _height-1; ++h)
    {
        for (std::size_t w = 0; w < _width-1; ++w)
        {
            texEdges.push_back(ctrlAt(0, w).texcoord - ctrlAt(0, w+1).texcoord);
            texCoords.push_back(ctrlAt(0,w).texcoord);

            texEdges.push_back(ctrlAt(_height-1, w+1).texcoord - ctrlAt(_height-1, w).texcoord);
            texCoords.push_back(ctrlAt(_height-1, w+1).texcoord);
        }

        texEdges.push_back(ctrlAt(h, 0).texcoord - ctrlAt(h+1, 0).texcoord);
        texCoords.push_back(ctrlAt(h, 0).texcoord);

        texEdges.push_back(ctrlAt(h+1, _width-1).texcoord - ctrlAt(h, _width-1).texcoord);
        texCoords.push_back(ctrlAt(h+1, _width-1).texcoord);
    }

    // Find the edge which is nearest to the s,t base vector, to classify them as "top" or "left"
    std::size_t bottomEdge = findBestEdgeForDirection(Vector2(1,0), texEdges);
    std::size_t leftEdge = findBestEdgeForDirection(Vector2(0,1), texEdges);
    std::size_t rightEdge = findBestEdgeForDirection(Vector2(0,-1), texEdges);
    std::size_t topEdge = findBestEdgeForDirection(Vector2(-1,0), texEdges);

    // The bottom edge is the one with the larger T texture coordinate
    if (texCoords[topEdge].y() > texCoords[bottomEdge].y())
    {
        std::swap(topEdge, bottomEdge);
    }

    // The right edge is the one with the larger S texture coordinate
    if (texCoords[rightEdge].x() < texCoords[leftEdge].x())
    {
        std::swap(rightEdge, leftEdge);
    }

    // Find the winding vertex index we're calculating the delta for
    std::size_t coordIndex = 0;
    // The dimension to move (1 for top/bottom, 0 for left right)
    std::size_t dim = 0;

	switch (align)
	{
	case IPatch::AlignEdge::Top:
		coordIndex = topEdge;
		dim = 1;
		break;
	case IPatch::AlignEdge::Bottom:
		coordIndex = bottomEdge;
		dim = 1;
		break;
	case IPatch::AlignEdge::Left:
		coordIndex = leftEdge;
		dim = 0;
		break;
	case IPatch::AlignEdge::Right:
		coordIndex = rightEdge;
		dim = 0;
		break;
	};

    Vector2 snapped = texCoords[coordIndex];

    // Snap the dimension we're going to change only (s for left/right, t for top/bottom)
    snapped[dim] = float_snapped(snapped[dim], 1.0);

    Vector2 delta = snapped - texCoords[coordIndex];

    // Shift the texture such that we hit the snapped coordinate
    translateTexCoords(delta);

    controlPointsChanged();
}

PatchTesselation& Patch::getTesselation()
{
    // Ensure the tesselation is up to date
    updateTesselation();

    return _mesh;
}

PatchRenderIndices Patch::getRenderIndices() const
{
	// Ensure the tesselation is up to date
	const_cast<Patch&>(*this).updateTesselation();

	PatchRenderIndices info;

	info.indices = _mesh.indices;
	info.lenStrips = _mesh.lenStrips;
	info.numStrips = _mesh.numStrips;

	return info;
}

PatchMesh Patch::getTesselatedPatchMesh() const
{
    // Ensure the tesselation is up to date
    const_cast<Patch&>(*this).updateTesselation();

    PatchMesh mesh;

    mesh.width = _mesh.width;
    mesh.height = _mesh.height;

    for (std::vector<ArbitraryMeshVertex>::const_iterator i = _mesh.vertices.begin();
        i != _mesh.vertices.end(); ++i)
    {
        VertexNT v;

        v.vertex = i->vertex;
        v.texcoord = i->texcoord;
        v.normal = i->normal;

        mesh.vertices.push_back(v);
    }

    return mesh;
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

  if(_width < MIN_PATCH_WIDTH || _width > MAX_PATCH_WIDTH) _width = 3;
  if(_height < MIN_PATCH_HEIGHT || _height > MAX_PATCH_HEIGHT) _height = 3;

  Vector3 vStart;
  vStart[x] = aabb.origin[x] - aabb.extents[x];
  vStart[y] = aabb.origin[y] - aabb.extents[y];
  vStart[z] = aabb.origin[z];

  auto xAdj = std::abs((vStart[x] - (aabb.origin[x] + aabb.extents[x])) / static_cast<Vector3::ElementType>(_width - 1));
  auto yAdj = std::abs((vStart[y] - (aabb.origin[y] + aabb.extents[y])) / static_cast<Vector3::ElementType>(_height - 1));

  Vector3 vTmp;
  vTmp[z] = vStart[z];
  PatchControlIter pCtrl = _ctrl.begin();

  vTmp[y]=vStart[y];
  for (std::size_t h=0; h<_height; h++)
  {
    vTmp[x]=vStart[x];
    for (std::size_t w=0; w<_width; w++, ++pCtrl)
    {
      pCtrl->vertex = vTmp;
      vTmp[x]+=xAdj;
    }
    vTmp[y]+=yAdj;
  }

  scaleTextureNaturally();
}

// Returns the dimension for the given viewtype, used by the patch prefab routines
// constDim will be the dimension which is held constant for each patch row,
// matching to the view vector, e.g. Z for the XY viewtype
// It is ensured that dim1 < dim2
inline void assignDimsForViewType(EViewType viewType, std::size_t& dim1, std::size_t& dim2, std::size_t& constDim)
{
    switch (viewType)
    {
        case XY: constDim = 2; break; // z coordinate is incremented each patch row
        case YZ: constDim = 0; break; // x coordinate is incremented each patch row
        case XZ: constDim = 1; break; // y coordinate is incremented each patch row
    };

    // Calculate the other two dimensions, such that colDim1 < colDim2
    dim1 = (constDim + 1) % 3;
    dim2 = (constDim + 2) % 3;

    if (dim2 < dim1)
    {
        std::swap(dim1, dim2);
    }
}

void Patch::constructBevel(const AABB& aabb, EViewType viewType)
{
    Vector3 vPos[3] =
    {
        aabb.origin - aabb.extents,
        aabb.origin,
        aabb.origin + aabb.extents
    };

    std::size_t dim1 = 0, dim2 = 0, constDim = 0;
    assignDimsForViewType(viewType, dim1, dim2, constDim);

    std::size_t lowlowhigh[3] = { 0, 0, 2 };
    std::size_t lowhighhigh[3] = { 0, 2, 2 };

    setDims(3, 3);

    PatchControlIter ctrl = _ctrl.begin();

    for (std::size_t h = 0; h < 3; ++h)
    {
        for (std::size_t w = 0; w < 3; ++w, ++ctrl)
        {
            // One of the dimensions stays constant per row
            ctrl->vertex[constDim] = vPos[h][constDim];

            // One dimension goes like "low", "low", "high" in a row
            ctrl->vertex[dim1] = vPos[ lowlowhigh[w] ][dim1];

            // One dimension goes like "low", "high", "high" in a row
            ctrl->vertex[dim2] = vPos[ lowhighhigh[w] ][dim2];
        }
    }

	if (viewType == XZ)
	{
		invertMatrix();
	}
}

void Patch::constructEndcap(const AABB& aabb, EViewType viewType)
{
    Vector3 vPos[3] =
    {
        aabb.origin - aabb.extents,
        aabb.origin,
        aabb.origin + aabb.extents
    };

    std::size_t pEndIndex[] =
    {
        2, 0,
        2, 2,
        1, 2,
        0, 2,
        0, 0,
    };

    // Define the "row" dimension, e.g. z for an XY-oriented patch
    std::size_t dim1 = 0, dim2 = 0, constDim = 0;
    assignDimsForViewType(viewType, dim1, dim2, constDim);

    setDims(5, 3);

    PatchControlIter pCtrl = _ctrl.begin();

    for (std::size_t h = 0; h < 3; ++h)
    {
        std::size_t* pIndex = pEndIndex;

        for (std::size_t w = 0; w < 5; ++w, pIndex += 2, ++pCtrl)
        {
            pCtrl->vertex[dim1] = vPos[pIndex[0]][dim1];
            pCtrl->vertex[dim2] = vPos[pIndex[1]][dim2];
            pCtrl->vertex[constDim] = vPos[h][constDim];
        }
    }

	if (viewType != XZ)
	{
		invertMatrix();
	}
}

void Patch::ConstructPrefab(const AABB& aabb, EPatchPrefab eType, EViewType viewType, std::size_t width, std::size_t height)
{
    if (eType == ePlane)
    {
        constructPlane(aabb, viewType, width, height);
    }
    else if (eType == eBevel)
    {
        constructBevel(aabb, viewType);
    }
    else if (eType == eEndCap)
    {
        constructEndcap(aabb, viewType);
    }
    else if (eType == eSqCylinder || eType == eCylinder ||
             eType == eDenseCylinder || eType == eVeryDenseCylinder ||
             eType == eCone || eType == eSphere)
    {
        Vector3 vPos[3] =
        {
            aabb.origin - aabb.extents,
            aabb.origin,
            aabb.origin + aabb.extents,
        };

        PatchControlIter pStart;

        switch(eType)
        {
        case eSqCylinder:
            setDims(9, 3);
            pStart = _ctrl.begin();
            break;
        case eDenseCylinder:
        case eVeryDenseCylinder:
        case eCylinder:
            setDims(9, 3);
            pStart = _ctrl.begin() + 1;
            break;
        case eCone: setDims(9, 3);
            pStart = _ctrl.begin() + 1;
            break;
        case eSphere:
            setDims(9, 5);
            pStart = _ctrl.begin() + (9+1);
            break;
        default:
            ERROR_MESSAGE("this should be unreachable");
            return;
        }

        // greebo: Determine which dimensions are assigned, depending on the view type

        // Define the "row" dimension, e.g. z for an XY-oriented cylinder
        std::size_t colDim1 = 0, colDim2 = 0, rowDim = 0;
        assignDimsForViewType(viewType, colDim1, colDim2, rowDim);

        // As first measure, assign a closed, axis-aligned loop of vertices for each patch row
        // Depending on the prefab type, further actions are performed in the switch statement below
        {
            // greebo: the other "column" dimensions are using the same pattern for each view
            // 0 = min, 1 = mid, 2 = max
            std::size_t pCylIndex[] =
            {
                0, 0,
                1, 0,
                2, 0,
                2, 1,
                2, 2,
                1, 2,
                0, 2,
                0, 1,
                0, 0,
            };

            for (std::size_t h = 0; h < 3; ++h)
            {
                std::size_t* pIndex = pCylIndex;

                PatchControlIter pCtrl = pStart;

                for (std::size_t w = 0; w < 8; ++w, ++pCtrl)
                {
                    // For the "row" dimension, we use the patch height 0..2
                    pCtrl->vertex[rowDim] = vPos[h][rowDim];

                    // Assign the other two "column" dimensions
                    pCtrl->vertex[colDim1] = vPos[pIndex[0]][colDim1];
                    pCtrl->vertex[colDim2] = vPos[pIndex[1]][colDim2];

                    pIndex += 2;
                }

                // Go to the next line, but only do that if we're not at the last one already
                // to not increment the pStart iterator beyond the end of the container
                if (h < 2) pStart += 9;
            }
        }

        switch(eType)
        {
        case eSqCylinder:
            {
                PatchControlIter pCtrl = _ctrl.begin();

                for (std::size_t h = 0; h < 3; ++h)
                {
                    pCtrl[8].vertex = pCtrl[0].vertex;

                    // Go to the next line
                    if (h < 2) pCtrl+=9;
                }
            }
            break;

        case eDenseCylinder:
        case eVeryDenseCylinder:
        case eCylinder:
            {
                // Regular cylinders get the first column snapped to the last one
                // to form a closed loop
                PatchControlIter pCtrl = _ctrl.begin();

                for (std::size_t h = 0; h < 3; ++h)
                {
                    pCtrl[0].vertex = pCtrl[8].vertex;

                    // Go to the next line
                    if (h < 2) pCtrl+=9;
                }
            }
            break;
        case eCone:
            // Close the control vertex loop of cones
            {
                PatchControlIter pCtrl = _ctrl.begin();

                for (std::size_t h = 0; h < 2; ++h)
                {
                    pCtrl[0].vertex = pCtrl[8].vertex;
                    // Go to the next line
                    if (h < 1) pCtrl+=9;
                }
            }
            // And "merge" the vertices of the last row into one single point
            {
                PatchControlIter pCtrl = _ctrl.begin() + 9*2;

                for (std::size_t w = 0; w < 9; ++w, ++pCtrl)
                {
                    pCtrl->vertex[colDim1] = vPos[1][colDim1];
                    pCtrl->vertex[colDim2] = vPos[1][colDim2];
                    pCtrl->vertex[rowDim] = vPos[2][rowDim];
                }
            }
            break;
        case eSphere:
            // Close the vertex loop for spheres too (middle row)
            {
                PatchControlIter pCtrl = _ctrl.begin() + 9;

                for (std::size_t h = 0; h < 3; ++h)
                {
                    pCtrl[0].vertex = pCtrl[8].vertex;

                    // Go to the next line
                    if (h < 2) pCtrl+=9;
                }
            }
            // Merge the first and last row vertices into one single point
            {
                PatchControlIter pCtrl = _ctrl.begin();

                for (std::size_t w = 0; w < 9; ++w, ++pCtrl)
                {
                    pCtrl->vertex[colDim1] = vPos[1][colDim1];
                    pCtrl->vertex[colDim2] = vPos[1][colDim2];
                    pCtrl->vertex[rowDim] = vPos[0][rowDim];
                }
            }
            {
                PatchControlIter pCtrl = _ctrl.begin() + (9*4);

                for (std::size_t w = 0; w < 9; ++w, ++pCtrl)
                {
                    pCtrl->vertex[colDim1] = vPos[1][colDim1];
                    pCtrl->vertex[colDim2] = vPos[1][colDim2];
                    pCtrl->vertex[rowDim] = vPos[2][rowDim];
                }
            }
            break;
        default:
            ERROR_MESSAGE("this should be unreachable");
            return;
        }

		if (eType == eDenseCylinder)
		{
			insertRemove(true, false, true);
		}

		if (eType == eVeryDenseCylinder)
		{
			insertRemove(true, false, false);
			insertRemove(true, false, true);
		}

		if (viewType == XZ)
		{
			invertMatrix();
		}
	}

	scaleTextureNaturally();
}

namespace
{

Vector3 getAverageNormal(const Vector3& normal1, const Vector3& normal2, double thickness)
{
    // Beware of normals with 0 length
    if (normal1.getLengthSquared() == 0) return normal2;
    if (normal2.getLengthSquared() == 0) return normal1;

    // Both normals have length > 0
    Vector3 n1 = normal1.getNormalised();
    Vector3 n2 = normal2.getNormalised();

    // Get the angle bisector
    Vector3 normal = (n1 + n2).getNormalised();

    // Now calculate the length correction out of the angle
    // of the two normals
    auto factor = cos(n1.angle(n2) * 0.5);

    // Stretch the normal to fit the required thickness
    normal *= thickness;

    // Check for div by zero (if the normals are antiparallel)
    // and stretch the resulting normal, if necessary
    if (factor != 0)
    {
        normal /= factor;
    }

    return normal;
}

inline void calculateColTangentForCtrl(const Patch& sourcePatch, std::size_t row, std::size_t col, Vector3 colTangent[2])
{
    const auto& curCtrl = sourcePatch.ctrlAt(row, col);
    auto sourceWidth = sourcePatch.getWidth();

    // Are we at the beginning/end of the column?
    if (col == 0 || col == sourceWidth - 1)
    {
        // Get the next row index
        std::size_t nextCol = (col == sourceWidth - 1) ? (col - 1) : (col + 1);

        const PatchControl& colNeighbour = sourcePatch.ctrlAt(row, nextCol);

        // One available tangent
        colTangent[0] = colNeighbour.vertex - curCtrl.vertex;
        // Reverse it if we're at the end of the column
        colTangent[0] *= (col == sourceWidth - 1) ? -1 : +1;
    }
    // We are in between, two tangents can be calculated
    else
    {
        // Take two neighbouring vertices that should form a line segment
        const PatchControl& neighbour1 = sourcePatch.ctrlAt(row, col + 1);
        const PatchControl& neighbour2 = sourcePatch.ctrlAt(row, col - 1);

        // Calculate both available tangents
        colTangent[0] = neighbour1.vertex - curCtrl.vertex;
        colTangent[1] = neighbour2.vertex - curCtrl.vertex;

        // Reverse the second one
        colTangent[1] *= -1;

        // Cull redundant tangents
        if (math::isParallel(colTangent[1], colTangent[0]))
        {
            colTangent[1] = Vector3(0, 0, 0);
        }
    }
}

inline void calculateRowTangentForCtrl(const Patch& sourcePatch, std::size_t row, std::size_t col, Vector3 rowTangent[2])
{
    const auto& curCtrl = sourcePatch.ctrlAt(row, col);
    auto sourceHeight = sourcePatch.getHeight();

    // Are we at the beginning or the end?
    if (row == 0 || row == sourceHeight - 1)
    {
        // Yes, only calculate one row tangent
        // Get the next row index
        std::size_t nextRow = (row == sourceHeight - 1) ? (row - 1) : (row + 1);

        const PatchControl& rowNeighbour = sourcePatch.ctrlAt(nextRow, col);

        // First tangent
        rowTangent[0] = rowNeighbour.vertex - curCtrl.vertex;
        // Reverse it accordingly
        rowTangent[0] *= (row == sourceHeight - 1) ? -1 : +1;
    }
    else
    {
        // Two tangents to calculate
        const PatchControl& rowNeighbour1 = sourcePatch.ctrlAt(row + 1, col);
        const PatchControl& rowNeighbour2 = sourcePatch.ctrlAt(row - 1, col);

        // First tangent
        rowTangent[0] = rowNeighbour1.vertex - curCtrl.vertex;
        rowTangent[1] = rowNeighbour2.vertex - curCtrl.vertex;

        // Reverse the second one
        rowTangent[1] *= -1;

        // Cull redundant tangents
        if (math::isParallel(rowTangent[1], rowTangent[0]))
        {
            rowTangent[1] = Vector3(0, 0, 0);
        }
    }
}

Vector3 calculateNormalForTangents(Vector3 colTangent[2], Vector3 rowTangent[2], const float thickness)
{
    Vector3 normal;

    // If two column tangents are available, take the length-corrected average
    if (colTangent[1].getLengthSquared() > 0)
    {
        // Two column normals to calculate
        Vector3 normal1 = rowTangent[0].cross(colTangent[0]);
        Vector3 normal2 = rowTangent[0].cross(colTangent[1]);

        if (normal1.getLengthSquared() > 0)
        {
            normal1.normalise();
        }

        if (normal2.getLengthSquared() > 0)
        {
            normal2.normalise();
        }

        normal = getAverageNormal(normal1, normal2, thickness);

        // Scale the normal down, as it is multiplied with thickness later on
        normal /= thickness;
    }
    else
    {
        // One column tangent available, maybe we have a second rowtangent?
        if (rowTangent[1].getLengthSquared() > 0)
        {
            // Two row normals to calculate
            Vector3 normal1 = rowTangent[0].cross(colTangent[0]);
            Vector3 normal2 = rowTangent[1].cross(colTangent[0]);

            if (normal1.getLengthSquared() > 0)
            {
                normal1.normalise();
            }

            if (normal2.getLengthSquared() > 0)
            {
                normal2.normalise();
            }

            normal = getAverageNormal(normal1, normal2, thickness);

            // Scale the normal down, as it is multiplied with thickness later on
            normal /= thickness;
        }
        else
        {
            normal = rowTangent[0].cross(colTangent[0]);

            if (normal.getLengthSquared() > 0)
            {
                normal.normalise();
            }
        }
    }

    return normal;
}

}

void Patch::createThickenedOpposite(const Patch& sourcePatch,
                                    const float thickness,
                                    const int axis)
{
    // Clone the dimensions from the other patch
    setDims(sourcePatch.getWidth(), sourcePatch.getHeight());

    // Also inherit the tesselation from the source patch
    setFixedSubdivisions(sourcePatch.subdivisionsFixed(), sourcePatch.getSubdivisions());

    // Copy the shader from the source patch
    setShader(sourcePatch.getShader());

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

    for (std::size_t col = 0; col < _width; col++)
    {
        for (std::size_t row = 0; row < _height; row++)
        {
            // The current control vertex on the other patch
            const PatchControl& curCtrl = sourcePatch.ctrlAt(row, col);

            Vector3 normal;

            // Are we extruding along vertex normals (i.e. extrudeAxis == 0,0,0)?
            if (extrudeAxis == Vector3(0,0,0))
            {
                // The col tangents (empty if 0,0,0)
                Vector3 colTangent[2] = { Vector3(0,0,0), Vector3(0,0,0) };

                calculateColTangentForCtrl(sourcePatch, row, col, colTangent);

                // Calculate the tangent vectors to the next row
                Vector3 rowTangent[2] = { Vector3(0,0,0), Vector3(0,0,0) };

                calculateRowTangentForCtrl(sourcePatch, row, col, rowTangent);

                normal = calculateNormalForTangents(colTangent, rowTangent, thickness);
            }
            else
            {
                // Take the predefined extrude direction instead
                normal = extrudeAxis;
            }

            // Store the new coordinates into this patch at the current coords
            ctrlAt(row, col).vertex = curCtrl.vertex + normal*thickness;

            // Clone the texture cooordinates of the source patch
            ctrlAt(row, col).texcoord = curCtrl.texcoord;
        }
    }

    // Notify the patch about the change
    controlPointsChanged();
}

void Patch::createThickenedWall(const Patch& sourcePatch,
                                const Patch& targetPatch,
                                const int wallIndex)
{
    // Copy the shader from the source patch
    setShader(sourcePatch.getShader());

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

    bool sourceTesselationFixed = sourcePatch.subdivisionsFixed();
    Subdivisions sourceTesselationX(sourcePatch.getSubdivisions().x(), 1);
    Subdivisions sourceTesselationY(sourcePatch.getSubdivisions().y(), 1);

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
        Vector3 sourceCoord = sourceCtrl[idx].vertex;
        Vector3 targetCoord = targetCtrl[idx].vertex;
        Vector3 middleCoord = (sourceCoord + targetCoord) / 2;

        // Now assign the vertex coordinates
        ctrlAt(0, col).vertex = sourceCoord;
        ctrlAt(1, col).vertex = middleCoord;
        ctrlAt(2, col).vertex = targetCoord;
    }

	if (wallIndex == 0 || wallIndex == 3) {
		invertMatrix();
	}

    // Notify the patch about the change
    controlPointsChanged();
}

void Patch::stitchTextureFrom(Patch& sourcePatch) {
    // Save the undo memento
    undoSave();

    // Convert the size_t stuff into int, because we need it for signed comparisons
    int patchHeight = static_cast<int>(_height);
    int patchWidth = static_cast<int>(_width);

    // Calculate the nearest corner vertex of this patch (to the sourcepatch vertices)
    PatchControlIter nearestControl = getClosestPatchControlToPatch(sourcePatch);

    PatchControlIter refControl = sourcePatch.getClosestPatchControlToPatch(*this);

    // Get the distance in texture space
    Vector2 texDiff = refControl->texcoord - nearestControl->texcoord;

    // The floored values
    Vector2 floored(floor(fabs(texDiff[0])), floor(fabs(texDiff[1])));

    // Compute the shift applicable to all vertices
    Vector2 shift;
    shift[0] = (fabs(texDiff[0])>1.0E-4) ? -floored[0] * texDiff[0]/fabs(texDiff[0]) : 0.0f;
    shift[1] = (fabs(texDiff[1])>1.0E-4) ? -floored[1] * texDiff[1]/fabs(texDiff[1]) : 0.0f;

    // Now shift all the texture vertices in the right direction, so that this patch
    // is getting as close as possible to the origin in texture space.
    for (PatchControlIter i = _ctrl.begin(); i != _ctrl.end(); ++i) {
        i->texcoord += shift;
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

                    auto dist = (other.vertex - self.vertex).getLength();

                    // Allow the coords to be a _bit_ distant
                    if (fabs(dist) < 0.005) {
                        // Assimilate the texture coordinates
                        self.texcoord = other.texcoord;
                    }
                }
            }
        }
    }

    // Notify the patch about the change
    controlPointsChanged();
}

void Patch::normaliseTexture()
{
    selection::algorithm::TextureNormaliser::NormalisePatch(*this);
}

const Subdivisions& Patch::getSubdivisions() const
{
    return _subDivisions;
}

void Patch::setFixedSubdivisions(bool isFixed, const Subdivisions& divisions)
{
    undoSave();

    _patchDef3 = isFixed;
    _subDivisions = divisions;

	if (_subDivisions.x() == 0)
	{
		_subDivisions.x() = 4;
	}

	if (_subDivisions.y() == 0)
	{
		_subDivisions.y() = 4;
	}

    SceneChangeNotify();
    textureChanged();
    controlPointsChanged();
}

bool Patch::subdivisionsFixed() const
{
    return _patchDef3;
}

bool Patch::getIntersection(const Ray& ray, Vector3& intersection)
{
    std::vector<RenderIndex>::const_iterator stripStartIndex = _mesh.indices.begin();

    // Go over each quad strip and intersect the ray with its triangles
    for (std::size_t strip = 0; strip < _mesh.numStrips; ++strip)
    {
        // Iterate over the indices. The +2 increment will lead up to the next quad
        for (std::vector<RenderIndex>::const_iterator indexIter = stripStartIndex;
            indexIter + 2 < stripStartIndex + _mesh.lenStrips; indexIter += 2)
        {
            Vector3 triangleIntersection;

            // Run a selection test against the quad's triangles
            {
                const Vector3& p1 = _mesh.vertices[*indexIter].vertex;
                const Vector3& p2 = _mesh.vertices[*(indexIter + 1)].vertex;
                const Vector3& p3 = _mesh.vertices[*(indexIter + 2)].vertex;

                if (ray.intersectTriangle(p1, p2, p3, triangleIntersection) == Ray::POINT)
                {
                    intersection = triangleIntersection;
                    return true;
                }
            }

            {
                const Vector3& p1 = _mesh.vertices[*(indexIter + 2)].vertex;
                const Vector3& p2 = _mesh.vertices[*(indexIter + 1)].vertex;
                const Vector3& p3 = _mesh.vertices[*(indexIter + 3)].vertex;

                if (ray.intersectTriangle(p1, p2, p3, triangleIntersection) == Ray::POINT)
                {
                    intersection = triangleIntersection;
                    return true;
                }
            }
        }

        stripStartIndex += _mesh.lenStrips;
    }

    return false;
}

void Patch::textureChanged()
{
    _node.onMaterialChanged();

    for (auto i = _observers.begin(); i != _observers.end();)
    {
        (*i++)->onPatchTextureChanged();
    }

    signal_patchTextureChanged().emit();
}

void Patch::attachObserver(Observer* observer)
{
    _observers.insert(observer);
}

void Patch::detachObserver(Observer* observer)
{
    _observers.erase(observer);
}

sigc::signal<void>& Patch::signal_patchTextureChanged()
{
    static sigc::signal<void> _sigPatchTextureChanged;
    return _sigPatchTextureChanged;
}

void Patch::queueTesselationUpdate()
{
    _tesselationChanged = true;
}
