#include "PatchNode.h"

#include "ifilter.h"
#include "ientity.h"
#include "iradiant.h"
#include "icounter.h"
#include "math/Frustum.h"
#include "math/Hash.h"

// Construct a PatchNode with no arguments
PatchNode::PatchNode(patch::PatchDefType type) :
	scene::SelectableNode(),
	m_dragPlanes(std::bind(&PatchNode::selectedChangedComponent, this, std::placeholders::_1)),
	m_render_selected(GL_POINTS),
	m_patch(*this),
    _untransformedOriginChanged(true),
    _selectedControlVerticesNeedUpdate(true),
    _renderableSurfaceSolid(m_patch.getTesselation()),
    _renderableSurfaceWireframe(m_patch.getTesselation())
{
	m_patch.setFixedSubdivisions(type == patch::PatchDefType::Def3, Subdivisions(m_patch.getSubdivisions()));
}

// Copy Constructor
PatchNode::PatchNode(const PatchNode& other) :
	scene::SelectableNode(other),
	scene::Cloneable(other),
	Snappable(other),
	IPatchNode(other),
	SelectionTestable(other),
	ComponentSelectionTestable(other),
	ComponentEditable(other),
	ComponentSnappable(other),
	PlaneSelectable(other),
	LitObject(other),
	Transformable(other),
	m_dragPlanes(std::bind(&PatchNode::selectedChangedComponent, this, std::placeholders::_1)),
	m_render_selected(GL_POINTS),
	m_patch(other.m_patch, *this), // create the patch out of the <other> one
    _untransformedOriginChanged(true),
    _selectedControlVerticesNeedUpdate(true),
    _renderableSurfaceSolid(m_patch.getTesselation()),
    _renderableSurfaceWireframe(m_patch.getTesselation())
{
}

PatchNode::~PatchNode()
{
    _renderableSurfaceSolid.clear();
    _renderableSurfaceWireframe.clear();
}

scene::INode::Type PatchNode::getNodeType() const
{
	return Type::Patch;
}

std::string PatchNode::getFingerprint()
{
    constexpr std::size_t SignificantDigits = scene::SignificantFingerprintDoubleDigits;

    if (m_patch.getHeight() * m_patch.getWidth() == 0)
    {
        return std::string(); // empty patches produce an empty fingerprint
    }

    math::Hash hash;

    // Width & Height
    hash.addSizet(m_patch.getHeight());
    hash.addSizet(m_patch.getWidth());

    // Subdivision Settings
    if (m_patch.subdivisionsFixed())
    {
        hash.addSizet(static_cast<std::size_t>(m_patch.getSubdivisions().x()));
        hash.addSizet(static_cast<std::size_t>(m_patch.getSubdivisions().y()));
    }

    // Material Name
    hash.addString(m_patch.getShader());

    // Combine all control point data
    for (const auto& ctrl : m_patch.getControlPoints())
    {
        hash.addVector3(ctrl.vertex, SignificantDigits);
        hash.addDouble(ctrl.texcoord.x(), SignificantDigits);
        hash.addDouble(ctrl.texcoord.y(), SignificantDigits);
    }

    return hash;
}

void PatchNode::allocate(std::size_t size) {
	// Clear the control instance vector and reserve <size> memory
	m_ctrl_instances.clear();
	m_ctrl_instances.reserve(size);

	// greebo: Cycle through the patch's control vertices and add them as PatchControlInstance to the vector
	// The PatchControlInstance constructor takes a pointer to a PatchControl and the SelectionChanged callback
	// The passed callback points back to this class (the member method selectedChangedComponent() is called).
	for(PatchControlIter i = m_patch.begin(); i != m_patch.end(); ++i)
	{
		m_ctrl_instances.push_back(
			PatchControlInstance(*i, std::bind(&PatchNode::selectedChangedComponent, this, std::placeholders::_1))
		);
	}
}

const AABB& PatchNode::localAABB() const {
	return m_patch.localAABB();
}

std::string PatchNode::name() const {
	return "Patch";
}

Patch& PatchNode::getPatchInternal() {
	return m_patch;
}

IPatch& PatchNode::getPatch() {
	return m_patch;
}

// Snappable implementation
void PatchNode::snapto(float snap) {
	m_patch.snapto(snap);
}

bool PatchNode::selectedVertices() {
	// Cycle through all the instances and return true as soon as the first selected one is found
	for (PatchControlInstances::iterator i = m_ctrl_instances.begin(); i != m_ctrl_instances.end(); ++i) {
		if (i->isSelected()) {
			return true;
		}
	}
    return false;
}

void PatchNode::snapComponents(float snap) {
	// Are there any selected vertices
	if (selectedVertices()) {
		// Tell the patch to save the current undo state
		m_patch.undoSave();

		// Cycle through all the selected control instances and snap them to the grid
		for (PatchControlInstances::iterator i = m_ctrl_instances.begin(); i != m_ctrl_instances.end(); ++i) {
			if(i->isSelected()) {
				i->snapto(snap);
			}
		}
		// Tell the patch that control points have changed
		m_patch.controlPointsChanged();
	}
}

// Test the Patch instance for selection
void PatchNode::testSelect(Selector& selector, SelectionTest& test)
{
    // Check if this patch has a twosided material
    const auto& shader = m_patch.getSurfaceShader().getGLShader();
    bool isTwosided = shader && shader->getMaterial()->getCullType() == Material::CULL_NONE;

    test.BeginMesh(localToWorld(), isTwosided);

    // Pass the selection test call to the patch
    m_patch.testSelect(selector, test);
}

void PatchNode::selectPlanes(Selector& selector, SelectionTest& test, const PlaneCallback& selectedPlaneCallback) {
	test.BeginMesh(localToWorld());

	// Check if the drag planes pass the selection test
	m_dragPlanes.selectPlanes(m_patch.localAABB(), selector, test, selectedPlaneCallback);
}

void PatchNode::selectReversedPlanes(Selector& selector, const SelectedPlanes& selectedPlanes) {
	// Pass the call to the according dragplane member method
	m_dragPlanes.selectReversedPlanes(m_patch.localAABB(), selector, selectedPlanes);
}

void PatchNode::selectCtrl(bool selected) {
	// Cycle through all ControlInstances and set them to <select>
	for (PatchControlInstances::iterator i = m_ctrl_instances.begin(); i != m_ctrl_instances.end(); ++i) {
		i->setSelected(selected);
	}
}

bool PatchNode::isSelectedComponents() const {
	// Cycle through all ControlInstances and return true on the first selected one that is found
	for (PatchControlInstances::const_iterator i = m_ctrl_instances.begin(); i != m_ctrl_instances.end(); ++i) {
		if (i->isSelected()) {
			return true;
		}
	}
	return false;
}

void PatchNode::setSelectedComponents(bool select, selection::ComponentSelectionMode mode) {
	// If we are in vertex edit mode, set the control vertices to <select>
	if (mode == selection::ComponentSelectionMode::Vertex) {
		selectCtrl(select);
	}
	// If we are in vertex edit mode, set the dragplanes to <select>
	else if (mode == selection::ComponentSelectionMode::Face) {
		m_dragPlanes.setSelected(select);
	}
}

void PatchNode::invertSelectedComponents(selection::ComponentSelectionMode mode)
{
	if (mode == selection::ComponentSelectionMode::Vertex)
	{
		// Cycle through the transformed patch vertices and set the colour of all selected control vertices to BLUE (hardcoded)
		for (PatchControlInstance& i : m_ctrl_instances)
		{
			i.setSelected(!i.isSelected());
		}
	}
}

void PatchNode::testSelectComponents(Selector& selector, SelectionTest& test, selection::ComponentSelectionMode mode)
{
	test.BeginMesh(localToWorld());

	// Only react to eVertex selection mode
	switch(mode) {
        case selection::ComponentSelectionMode::Vertex:
        {
			// Cycle through all the control instances and test them for selection
			for (auto& i : m_ctrl_instances)
            {
				i.testSelect(selector, test);
			}
		}
		break;
		default:
		break;
	}
}

const AABB& PatchNode::getSelectedComponentsBounds() const {
	// Create a new axis aligned bounding box
	m_aabb_component = AABB();

	// Cycle through all the instances and extend the bounding box by using the selected control points
	for (PatchControlInstances::const_iterator i = m_ctrl_instances.begin(); i != m_ctrl_instances.end(); ++i) {
		if (i->isSelected()) {
			m_aabb_component.includePoint(i->control.vertex);
		}
	}

	return m_aabb_component;
}

bool PatchNode::isVisible() const
{
	return visible() && hasVisibleMaterial();
}

bool PatchNode::hasVisibleMaterial() const
{
	return m_patch.getSurfaceShader().getGLShader()->getMaterial()->isVisible();
}

void PatchNode::selectedChangedComponent(const ISelectable& selectable)
{
    // We need to update our vertex colours next time we render them
    _selectedControlVerticesNeedUpdate = true;

	// Notify the selection system that this PatchNode was selected. The RadiantSelectionSystem adds
	// this to its internal list of selected nodes.
	GlobalSelectionSystem().onComponentSelection(SelectableNode::getSelf(), selectable);
}

// Clones this node, allocates a new Node on the heap and passes itself to the constructor of the new node
scene::INodePtr PatchNode::clone() const
{
	return std::make_shared<PatchNode>(*this);
}

void PatchNode::onInsertIntoScene(scene::IMapRootNode& root)
{
    // Mark the GL shader as used from now on, this is used by the TextureBrowser's filtering
    m_patch.getSurfaceShader().setInUse(true);
    _renderableSurfaceSolid.queueUpdate();
    _renderableSurfaceWireframe.queueUpdate();

	m_patch.connectUndoSystem(root.getUndoSystem());
	GlobalCounters().getCounter(counterPatches).increment();

    // Update the origin information needed for transformations
    _untransformedOrigin = worldAABB().getOrigin();

	SelectableNode::onInsertIntoScene(root);
}

void PatchNode::onRemoveFromScene(scene::IMapRootNode& root)
{
    // De-select this node
	setSelected(false);

	// De-select all child components as well
	setSelectedComponents(false, selection::ComponentSelectionMode::Vertex);

	GlobalCounters().getCounter(counterPatches).decrement();

	m_patch.disconnectUndoSystem(root.getUndoSystem());

    _renderableSurfaceSolid.clear();
    _renderableSurfaceWireframe.clear();
    m_patch.getSurfaceShader().setInUse(false);

	SelectableNode::onRemoveFromScene(root);
}

bool PatchNode::getIntersection(const Ray& ray, Vector3& intersection)
{
	return m_patch.getIntersection(ray, intersection);
}

bool PatchNode::intersectsLight(const RendererLight& light) const {
	return light.lightAABB().intersects(worldAABB());
}

void PatchNode::onPreRender(const VolumeTest& volume)
{
    // Don't do anything when invisible
    if (!isForcedVisible() && !m_patch.hasVisibleMaterial()) return;

    // Defer the tesselation calculation to the last minute
    m_patch.evaluateTransform();
    m_patch.updateTesselation();

    if (volume.fill())
    {
        _renderableSurfaceSolid.update(m_patch._shader.getGLShader());
    }
    else
    {
        _renderableSurfaceWireframe.update(_renderEntity->getWireShader());
    }
}

void PatchNode::renderSolid(IRenderableCollector& collector, const VolumeTest& volume) const
{
	// Don't render invisible patches
	if (!isForcedVisible() && !m_patch.hasVisibleMaterial()) return;

#ifdef RENDERABLE_GEOMETRY
    if (isSelected())
    {
        // Send the patch geometry for rendering highlights
        collector.addGeometry(const_cast<Patch&>(m_patch)._solidRenderable, 
            IRenderableCollector::Highlight::Primitives | IRenderableCollector::Highlight::Flags::Faces);
    }
#endif
    assert(_renderEntity); // patches rendered without parent - no way!

#if 0
    // Render the patch itself
    collector.addRenderable(
        *m_patch._shader.getGLShader(), m_patch._solidRenderable,
        localToWorld(), this, _renderEntity
    );
#endif
#if DEBUG_PATCH_NTB_VECTORS
    m_patch._renderableVectors.render(collector, volume, localToWorld());
#endif
}

void PatchNode::renderWireframe(IRenderableCollector& collector, const VolumeTest& volume) const
{
	// Don't render invisible shaders
	if (!isForcedVisible() && !m_patch.hasVisibleMaterial()) return;

	const_cast<Patch&>(m_patch).evaluateTransform();

	// Pass the call to the patch instance, it adds the renderable
	m_patch.renderWireframe(collector, volume, localToWorld(), *_renderEntity);

	// Render the selected components
	renderComponentsSelected(collector, volume);
}

void PatchNode::renderHighlights(IRenderableCollector& collector, const VolumeTest& volume)
{
    collector.addRenderable(
        *m_patch._shader.getGLShader(), m_patch._solidRenderable,
        localToWorld(), this, _renderEntity
    );

    // Render the selected components
    renderComponentsSelected(collector, volume);
}

void PatchNode::setRenderSystem(const RenderSystemPtr& renderSystem)
{
	SelectableNode::setRenderSystem(renderSystem);

	m_patch.setRenderSystem(renderSystem);
    _renderableSurfaceSolid.clear();
    _renderableSurfaceWireframe.clear();

	if (renderSystem)
	{
		m_state_selpoint = renderSystem->capture("$SELPOINT");
	}
	else
	{
		m_state_selpoint.reset();
	}
}

// Renders the components of this patch instance
void PatchNode::renderComponents(IRenderableCollector& collector, const VolumeTest& volume) const
{
	// Don't render invisible shaders
	if (!m_patch.getSurfaceShader().getGLShader()->getMaterial()->isVisible()) return;

	// greebo: Don't know yet, what evaluateTransform() is really doing
	const_cast<Patch&>(m_patch).evaluateTransform();

	// Only render the components, if we are in the according ComponentMode
	if (GlobalSelectionSystem().ComponentMode() == selection::ComponentSelectionMode::Vertex)
    {
		m_patch.submitRenderablePoints(collector, volume, localToWorld());
	}
}

void PatchNode::updateSelectedControlVertices() const
{
    if (!_selectedControlVerticesNeedUpdate) return;

    _selectedControlVerticesNeedUpdate = false;

	// Clear the renderable point vector that represents the selection
	m_render_selected.clear();

	// Cycle through the transformed patch vertices and set the colour of all selected control vertices to BLUE (hardcoded)
	auto ctrl = m_patch.getControlPointsTransformed().begin();

	for (auto i = m_ctrl_instances.begin(); i != m_ctrl_instances.end(); ++i, ++ctrl)
	{
		if (i->isSelected())
        {
			const Colour4b colour_selected(0, 0, 0, 255);
			// Add this patch control instance to the render list
			m_render_selected.push_back(VertexCb(reinterpret_cast<const Vertex3f&>(ctrl->vertex), colour_selected));
		}
	}
}

void PatchNode::renderComponentsSelected(IRenderableCollector& collector, const VolumeTest& volume) const
{
	const_cast<Patch&>(m_patch).evaluateTransform();

	// Rebuild the array of selected control vertices
    updateSelectedControlVertices();

	// If there are any selected components, add them to the collector
	if (!m_render_selected.empty())
    {
		collector.setHighlightFlag(IRenderableCollector::Highlight::Primitives, false);
		collector.addRenderable(*m_state_selpoint, m_render_selected, localToWorld());
	}
}

std::size_t PatchNode::getHighlightFlags()
{
	if (!isSelected()) return Highlight::NoHighlight;

	return isGroupMember() ? (Highlight::Selected | Highlight::GroupMember) : Highlight::Selected;
}

void PatchNode::evaluateTransform()
{
	Matrix4 matrix = calculateTransform();

	// Avoid transform calls when an identity matrix is passed,
	// this equality check is cheaper than all the stuff going on in transform().
	if (matrix == Matrix4::getIdentity()) return;

	if (getType() == TRANSFORM_PRIMITIVE)
	{
		m_patch.transform(matrix);
	}
	else
	{
		transformComponents(matrix);
	}
}

void PatchNode::transformComponents(const Matrix4& matrix) {
	// Are there any selected vertices?
	if (selectedVertices())
	{
		// Set the iterator to the start of the (transformed) control points array
		PatchControlIter ctrl = m_patch.getControlPointsTransformed().begin();

		// Cycle through the patch control instances and transform the selected ones
		// greebo: Have to investigate this further, why there are actually two iterators needed
		for (PatchNode::PatchControlInstances::iterator i = m_ctrl_instances.begin();
			 i != m_ctrl_instances.end(); ++i, ++ctrl)
		{
			if (i->isSelected())
			{
				ctrl->vertex = matrix.transformPoint(ctrl->vertex);
			}
		}

		// mark this patch transform as dirty
		m_patch.transformChanged();
        _selectedControlVerticesNeedUpdate = true;
	}

	// Also, check if there are any drag planes selected
	// this should only be true when the transform is a pure translation.
	if (m_dragPlanes.isSelected())
	{
		m_patch.transform(m_dragPlanes.evaluateTransform(matrix.translation()));
	}
}

void PatchNode::_onTransformationChanged()
{
	m_patch.transformChanged();
    _renderableSurfaceSolid.queueUpdate();
    _renderableSurfaceWireframe.queueUpdate();
}

void PatchNode::_applyTransformation()
{
	// First, revert the changes, then recalculate the transformation and then freeze the changes
	m_patch.revertTransform();
	evaluateTransform();
	m_patch.freezeTransform();

    _untransformedOriginChanged = true;
}

const Vector3& PatchNode::getUntransformedOrigin()
{
    if (_untransformedOriginChanged)
    {
        _untransformedOriginChanged = false;
        _untransformedOrigin = worldAABB().getOrigin();
    }

    return _untransformedOrigin;
}

void PatchNode::onControlPointsChanged()
{
    _renderableSurfaceSolid.queueUpdate();
    _renderableSurfaceWireframe.queueUpdate();
}

void PatchNode::onMaterialChanged()
{
    _renderableSurfaceSolid.queueUpdate();
    _renderableSurfaceWireframe.queueUpdate();
}

void PatchNode::onVisibilityChanged(bool visible)
{
    SelectableNode::onVisibilityChanged(visible);

    if (!visible)
    {
        // Disconnect our renderable when the node is hidden
        _renderableSurfaceSolid.clear();
        _renderableSurfaceWireframe.clear();
    }
    else
    {
        // Update the vertex buffers next time we need to render
        _renderableSurfaceSolid.queueUpdate();
        _renderableSurfaceWireframe.queueUpdate();
    }
}
