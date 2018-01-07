#include "PatchNode.h"

#include "ifilter.h"
#include "ientity.h"
#include "iradiant.h"
#include "icounter.h"
#include "math/Frustum.h"

// Construct a PatchNode with no arguments
PatchNode::PatchNode(bool patchDef3) :
	scene::SelectableNode(),
	m_dragPlanes(std::bind(&PatchNode::selectedChangedComponent, this, std::placeholders::_1)),
	m_render_selected(GL_POINTS),
	m_lightList(&GlobalRenderSystem().attachLitObject(*this)),
	m_patch(*this),
    _untransformedOriginChanged(true)
{
	m_patch.setFixedSubdivisions(patchDef3, Subdivisions(m_patch.getSubdivisions()));

	SelectableNode::setTransformChangedCallback(Callback(std::bind(&PatchNode::lightsChanged, this)));
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
	m_lightList(&GlobalRenderSystem().attachLitObject(*this)),
	m_patch(other.m_patch, *this), // create the patch out of the <other> one
    _untransformedOriginChanged(true)
{
	SelectableNode::setTransformChangedCallback(Callback(std::bind(&PatchNode::lightsChanged, this)));
}

PatchNode::~PatchNode()
{
	GlobalRenderSystem().detachLitObject(*this);
}

scene::INode::Type PatchNode::getNodeType() const
{
	return Type::Patch;
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

void PatchNode::lightsChanged()
{
	m_lightList->setDirty();
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
void PatchNode::testSelect(Selector& selector, SelectionTest& test) {
	// Do not select patch if it is filtered
	if (!isVisible())
		return;

    test.BeginMesh(localToWorld(), true);
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

void PatchNode::setSelectedComponents(bool select, SelectionSystem::EComponentMode mode) {
	// If we are in vertex edit mode, set the control vertices to <select>
	if (mode == SelectionSystem::eVertex) {
		selectCtrl(select);
	}
	// If we are in vertex edit mode, set the dragplanes to <select>
	else if (mode == SelectionSystem::eFace) {
		m_dragPlanes.setSelected(select);
	}
}

void PatchNode::invertSelectedComponents(SelectionSystem::EComponentMode mode)
{
	if (mode == SelectionSystem::eVertex)
	{
		// Cycle through the transformed patch vertices and set the colour of all selected control vertices to BLUE (hardcoded)
		for (PatchControlInstance& i : m_ctrl_instances)
		{
			i.setSelected(!i.isSelected());
		}
	}
}

void PatchNode::testSelectComponents(Selector& selector, SelectionTest& test, SelectionSystem::EComponentMode mode) {
	test.BeginMesh(localToWorld());

	// Only react to eVertex selection mode
	switch(mode) {
		case SelectionSystem::eVertex: {
			// Cycle through all the control instances and test them for selection
			for (PatchControlInstances::iterator i = m_ctrl_instances.begin(); i != m_ctrl_instances.end(); ++i) {
				i->testSelect(selector, test);
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

void PatchNode::selectedChangedComponent(const ISelectable& selectable) {
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

	m_patch.connectUndoSystem(root.getUndoChangeTracker());
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
	setSelectedComponents(false, SelectionSystem::eVertex);

	GlobalCounters().getCounter(counterPatches).decrement();

	m_patch.disconnectUndoSystem(root.getUndoChangeTracker());

    m_patch.getSurfaceShader().setInUse(false);

	SelectableNode::onRemoveFromScene(root);
}

bool PatchNode::getIntersection(const Ray& ray, Vector3& intersection)
{
	return m_patch.getIntersection(ray, intersection);
}

bool PatchNode::intersectsLight(const RendererLight& light) const {
	return light.intersectsAABB(worldAABB());
}

void PatchNode::renderSolid(RenderableCollector& collector, const VolumeTest& volume) const
{
	// Don't render invisible shaders
	if (!isForcedVisible() && !m_patch.hasVisibleMaterial()) return;

	const_cast<Patch&>(m_patch).evaluateTransform();

	assert(_renderEntity); // patches rendered without parent - no way!

	// Pass the call to the patch instance, it adds the renderable
	m_patch.renderSolid(collector, volume, localToWorld(), *_renderEntity, *m_lightList);

	// Render the selected components
	renderComponentsSelected(collector, volume);
}

void PatchNode::renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const
{
	// Don't render invisible shaders
	if (!isForcedVisible() && !m_patch.hasVisibleMaterial()) return;

	const_cast<Patch&>(m_patch).evaluateTransform();

	// Pass the call to the patch instance, it adds the renderable
	m_patch.renderWireframe(collector, volume, localToWorld(), *_renderEntity);

	// Render the selected components
	renderComponentsSelected(collector, volume);
}

void PatchNode::setRenderSystem(const RenderSystemPtr& renderSystem)
{
	SelectableNode::setRenderSystem(renderSystem);

	m_patch.setRenderSystem(renderSystem);

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
void PatchNode::renderComponents(RenderableCollector& collector, const VolumeTest& volume) const
{
	// Don't render invisible shaders
	if (!m_patch.getSurfaceShader().getGLShader()->getMaterial()->isVisible()) return;

	// greebo: Don't know yet, what evaluateTransform() is really doing
	const_cast<Patch&>(m_patch).evaluateTransform();

	// Only render the components, if we are in the according ComponentMode
	if (GlobalSelectionSystem().ComponentMode() == SelectionSystem::eVertex)
    {
		m_patch.submitRenderablePoints(collector, volume, localToWorld());
	}
}

void PatchNode::update_selected() const {
	// Clear the renderable point vector that represents the selection
	m_render_selected.clear();

	// Cycle through the transformed patch vertices and set the colour of all selected control vertices to BLUE (hardcoded)
	PatchControlConstIter ctrl = m_patch.getControlPointsTransformed().begin();

	for (PatchControlInstances::const_iterator i = m_ctrl_instances.begin();
		 i != m_ctrl_instances.end(); ++i, ++ctrl)
	{
		if (i->isSelected()) {
			const Colour4b colour_selected(0, 0, 0, 255);
			// Add this patch control instance to the render list
			m_render_selected.push_back(VertexCb(reinterpret_cast<const Vertex3f&>(ctrl->vertex), colour_selected));
		}
	}
}

void PatchNode::renderComponentsSelected(RenderableCollector& collector, const VolumeTest& volume) const
{
	// greebo: Don't know yet, what evaluateTransform() is really doing
	const_cast<Patch&>(m_patch).evaluateTransform();

	// Rebuild the array of selected control vertices
	update_selected();

	// If there are any selected components, add them to the collector
	if (!m_render_selected.empty())
    {
		collector.setHighlightFlag(RenderableCollector::Highlight::Primitives, false);
		collector.addRenderable(m_state_selpoint, m_render_selected, localToWorld());
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
	}

	// Also, check if there are any drag planes selected
	// this should only be true when the transform is a pure translation.
	if (m_dragPlanes.isSelected())
	{
		m_patch.transform(m_dragPlanes.evaluateTransform(matrix.t().getVector3()));
	}
}

void PatchNode::_onTransformationChanged()
{
	m_patch.transformChanged();
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
