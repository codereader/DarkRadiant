#include "PatchNode.h"

#include "ifilter.h"
#include "ientity.h"
#include "iradiant.h"
#include "icounter.h"
#include "math/Frustum.h"

// Construct a PatchNode with no arguments
PatchNode::PatchNode(bool patchDef3) :
	scene::SelectableNode(),
	m_dragPlanes(boost::bind(&PatchNode::selectedChangedComponent, this, _1)),
	m_render_selected(GL_POINTS),
	m_lightList(&GlobalRenderSystem().attachLitObject(*this)),
	m_patch(*this,
			Callback(boost::bind(&PatchNode::evaluateTransform, this)),
			Callback(boost::bind(&SelectableNode::boundsChanged, this))) // create the m_patch member with the node parameters
{
	m_patch.m_patchDef3 = patchDef3;

	SelectableNode::setTransformChangedCallback(Callback(boost::bind(&PatchNode::lightsChanged, this)));
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
	m_dragPlanes(boost::bind(&PatchNode::selectedChangedComponent, this, _1)),
	m_render_selected(GL_POINTS),
	m_lightList(&GlobalRenderSystem().attachLitObject(*this)),
	m_patch(other.m_patch,
			*this,
			Callback(boost::bind(&PatchNode::evaluateTransform, this)),
			Callback(boost::bind(&SelectableNode::boundsChanged, this))) // create the patch out of the <other> one
{
	SelectableNode::setTransformChangedCallback(Callback(boost::bind(&PatchNode::lightsChanged, this)));
}

PatchNode::~PatchNode()
{
	GlobalRenderSystem().detachLitObject(*this);
}

scene::INode::Type PatchNode::getNodeType() const
{
	return Type::Primitive;
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
			PatchControlInstance(*i, boost::bind(&PatchNode::selectedChangedComponent, this, _1))
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
	return m_patch.getState()->getMaterial()->isVisible();
}

void PatchNode::invertSelected()
{
	// Override default behaviour of SelectableNode, we have components

	if (GlobalSelectionSystem().Mode() == SelectionSystem::eComponent)
	{
		// Cycle through the transformed patch vertices and set the colour of all selected control vertices to BLUE (hardcoded)
		PatchControlIter ctrl = m_patch.getControlPointsTransformed().begin();

		for (PatchControlInstances::iterator i = m_ctrl_instances.begin(); i != m_ctrl_instances.end(); ++i, ++ctrl)
		{
			i->invertSelected();
		}
	}
	else // primitive mode
	{
		// Invert the selection of the patch itself
		SelectableNode::invertSelected();
	}
}

void PatchNode::selectedChangedComponent(const Selectable& selectable) {
	// Notify the selection system that this PatchNode was selected. The RadiantSelectionSystem adds
	// this to its internal list of selected nodes.
	GlobalSelectionSystem().onComponentSelection(SelectableNode::getSelf(), selectable);
}

// Clones this node, allocates a new Node on the heap and passes itself to the constructor of the new node
scene::INodePtr PatchNode::clone() const {
	return scene::INodePtr(new PatchNode(*this));
}

void PatchNode::onInsertIntoScene()
{
	m_patch.instanceAttach(scene::findMapFile(getSelf()));
	GlobalCounters().getCounter(counterPatches).increment();

	SelectableNode::onInsertIntoScene();
}

void PatchNode::onRemoveFromScene()
{
	// De-select this node
	setSelected(false);

	// De-select all child components as well
	setSelectedComponents(false, SelectionSystem::eVertex);

	GlobalCounters().getCounter(counterPatches).decrement();

	m_patch.instanceDetach(scene::findMapFile(getSelf()));

	SelectableNode::onRemoveFromScene();
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
	if (!m_patch.hasVisibleMaterial()) return;

	const_cast<Patch&>(m_patch).evaluateTransform();
	collector.setLights(*m_lightList);

	assert(_renderEntity); // patches rendered without parent - no way!

	// Pass the call to the patch instance, it adds the renderable
	m_patch.render_solid(collector, volume, localToWorld(), *_renderEntity);

	// Render the selected components
	renderComponentsSelected(collector, volume);
}

void PatchNode::renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const
{
	// Don't render invisible shaders
	if (!m_patch.getState()->getMaterial()->isVisible()) return;

	const_cast<Patch&>(m_patch).evaluateTransform();

	// Pass the call to the patch instance, it adds the renderable
	m_patch.render_wireframe(collector, volume, localToWorld());

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
	if (!m_patch.getState()->getMaterial()->isVisible()) return;

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
		collector.highlightPrimitives(false);
		collector.SetState(PatchNode::m_state_selpoint, RenderableCollector::eWireframeOnly);
		collector.SetState(PatchNode::m_state_selpoint, RenderableCollector::eFullMaterials);
		collector.addRenderable(m_render_selected, localToWorld());
	}
}

bool PatchNode::isHighlighted() const
{
	return isSelected();
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
}
