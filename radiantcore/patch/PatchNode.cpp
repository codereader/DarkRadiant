#include "PatchNode.h"

#include "ifilter.h"
#include "ientity.h"
#include "iradiant.h"
#include "icounter.h"
#include "math/Frustum.h"
#include "math/Hash.h"

PatchNode::PatchNode(patch::PatchDefType type) :
	scene::SelectableNode(),
	m_dragPlanes(std::bind(&PatchNode::selectedChangedComponent, this, std::placeholders::_1)),
	m_patch(*this),
    _untransformedOriginChanged(true),
    _renderableSurfaceSolid(m_patch.getTesselation(), true),
    _renderableSurfaceWireframe(m_patch.getTesselation(), false),
    _renderableCtrlLattice(m_patch, m_ctrl_instances),
    _renderableCtrlPoints(m_patch, m_ctrl_instances)
{
	m_patch.setFixedSubdivisions(type == patch::PatchDefType::Def3, Subdivisions(m_patch.getSubdivisions()));
}

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
	Transformable(other),
	m_dragPlanes(std::bind(&PatchNode::selectedChangedComponent, this, std::placeholders::_1)),
	m_patch(other.m_patch, *this), // create the patch out of the <other> one
    _untransformedOriginChanged(true),
    _renderableSurfaceSolid(m_patch.getTesselation(), true),
    _renderableSurfaceWireframe(m_patch.getTesselation(), false),
    _renderableCtrlLattice(m_patch, m_ctrl_instances),
    _renderableCtrlPoints(m_patch, m_ctrl_instances)
{
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

void PatchNode::updateSelectableControls()
{
	// Clear the control instance vector and reserve <size> memory
	m_ctrl_instances.clear();

    // We link the instances to the working control point set
    auto& ctrlPoints = m_patch.getControlPointsTransformed();

	m_ctrl_instances.reserve(ctrlPoints.size());

	// greebo: Cycle through the patch's control vertices and add them as PatchControlInstance to the vector
	// The PatchControlInstance constructor takes a pointer to a PatchControl and the SelectionChanged callback
	// The passed callback points back to this class (the member method selectedChangedComponent() is called).
	for(auto& ctrl : ctrlPoints)
	{
		m_ctrl_instances.emplace_back(ctrl, 
            std::bind(&PatchNode::selectedChangedComponent, this, std::placeholders::_1));
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

void PatchNode::snapComponents(float snap)
{
	// Are there any selected vertices
    if (!selectedVertices()) return;

    // Cycle through all the selected control instances and snap them to the grid
    for (auto& vertex : m_ctrl_instances)
    {
	    if (vertex.isSelected())
        {
            vertex.snapto(snap);
	    }
    }

    // Save the transformed control point array to the real set
    m_patch.freezeTransform();
    // Tell the patch that control points have changed
    m_patch.controlPointsChanged();
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
    _renderableCtrlPoints.queueUpdate();

	// Notify the selection system that this PatchNode was selected. The RadiantSelectionSystem adds
	// this to its internal list of selected nodes.
	GlobalSelectionSystem().onComponentSelection(SelectableNode::getSelf(), selectable);
}

// Clones this node, allocates a new Node on the heap and passes itself to the constructor of the new node
scene::INodePtr PatchNode::clone() const
{
	return std::make_shared<PatchNode>(*this);
}

void PatchNode::updateAllRenderables()
{
    _renderableSurfaceSolid.queueUpdate();
    _renderableSurfaceWireframe.queueUpdate();
    _renderableCtrlLattice.queueUpdate();
    _renderableCtrlPoints.queueUpdate();
}

void PatchNode::hideAllRenderables()
{
    _renderableSurfaceSolid.hide();
    _renderableSurfaceWireframe.hide();
    _renderableCtrlLattice.hide();
    _renderableCtrlPoints.hide();
}

void PatchNode::clearAllRenderables()
{
    _renderableSurfaceSolid.clear();
    _renderableSurfaceWireframe.clear();
    _renderableCtrlLattice.clear();
    _renderableCtrlPoints.clear();
}

void PatchNode::onInsertIntoScene(scene::IMapRootNode& root)
{
    // Mark the GL shader as used from now on, this is used by the TextureBrowser's filtering
    m_patch.getSurfaceShader().setInUse(true);

    // When inserting a patch into the scene, it gets a parent entity assigned
    // The colour of that entity will influence the tesselation's vertex colours
    m_patch.queueTesselationUpdate();

    updateAllRenderables();

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

    clearAllRenderables();

    m_patch.getSurfaceShader().setInUse(false);

	SelectableNode::onRemoveFromScene(root);
}

bool PatchNode::getIntersection(const Ray& ray, Vector3& intersection)
{
	return m_patch.getIntersection(ray, intersection);
}

void PatchNode::onPreRender(const VolumeTest& volume)
{
    // Defer the tesselation calculation to the last minute
    m_patch.evaluateTransform();
    m_patch.updateTesselation();

    _renderableSurfaceSolid.update(m_patch._shader.getGLShader());
    _renderableSurfaceWireframe.update(_renderEntity->getWireShader());

    _renderableSurfaceSolid.attachToEntity(_renderEntity);

    if (isSelected() && GlobalSelectionSystem().ComponentMode() == selection::ComponentSelectionMode::Vertex)
    {
        // Selected patches in component mode render the lattice connecting the control points
        _renderableCtrlLattice.update(_ctrlLatticeShader);
        _renderableCtrlPoints.update(_ctrlPointShader);
    }
    else
    {
        _renderableCtrlPoints.hide();
        _renderableCtrlLattice.hide();

        // Queue an update the next time it's rendered
        _renderableCtrlPoints.queueUpdate();
        _renderableCtrlLattice.queueUpdate();
    }
}

void PatchNode::renderHighlights(IRenderableCollector& collector, const VolumeTest& volume)
{
    if (GlobalSelectionSystem().Mode() != selection::SelectionSystem::eComponent)
    {
        // The coloured selection overlay should use the same triangulated surface to avoid z fighting
        collector.setHighlightFlag(IRenderableCollector::Highlight::Faces, true);
        collector.setHighlightFlag(IRenderableCollector::Highlight::Primitives, false);
        collector.addHighlightRenderable(_renderableSurfaceSolid, localToWorld());
    }

    // The selection outline (wireframe) should use the quadrangulated surface
    collector.setHighlightFlag(IRenderableCollector::Highlight::Faces, false);
    collector.setHighlightFlag(IRenderableCollector::Highlight::Primitives, true);
    collector.addHighlightRenderable(_renderableSurfaceWireframe, localToWorld());
}

void PatchNode::setRenderSystem(const RenderSystemPtr& renderSystem)
{
	SelectableNode::setRenderSystem(renderSystem);

	m_patch.setRenderSystem(renderSystem);

    clearAllRenderables();

    if (renderSystem)
	{
        _ctrlPointShader = renderSystem->capture(BuiltInShaderType::BigPoint);
        _ctrlLatticeShader = renderSystem->capture(BuiltInShaderType::PatchLattice);
	}
	else
	{
        _ctrlPointShader.reset();
        _ctrlLatticeShader.reset();
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
		m_patch.transform(m_dragPlanes.evaluateTransform(matrix.translation()));
	}
}

void PatchNode::_onTransformationChanged()
{
	m_patch.transformChanged();

    updateAllRenderables();
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

void PatchNode::onTesselationChanged()
{
    updateAllRenderables();
}

void PatchNode::onControlPointsChanged()
{
    updateAllRenderables();
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
        hideAllRenderables();
    }
    else
    {
        // Queue an update, renderables are automatically shown in onPreRender
        updateAllRenderables();
    }
}
