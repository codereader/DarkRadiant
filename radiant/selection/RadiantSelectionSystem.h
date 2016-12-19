#pragma once

#include "iregistry.h"
#include "irenderable.h"
#include "iselection.h"
#include "iradiant.h"
#include "icommandsystem.h"
#include "imap.h"

#include "selectionlib.h"
#include "math/Matrix4.h"
#include "wxutil/event/SingleIdleCallback.h"
#include "SelectedNodeList.h"

#include "Translatable.h"
#include "Scalable.h"
#include "Rotatable.h"
#include "ManipulationPivot.h"

namespace selection
{

class RadiantSelectionSystem :
	public SelectionSystem,
	public Rotatable,
	public Scalable,
	public Renderable,
	protected wxutil::SingleIdleCallback
{
	ManipulationPivot _pivot;

	mutable Matrix4 _pivot2world;
	Matrix4 _pivot2worldStart;
	Matrix4 _manip2pivotStart;

	typedef std::list<Observer*> ObserverList;
	ObserverList _observers;

	Quaternion _rotation;
	Vector3 _scale;

	// The 3D volume surrounding the most recent selection.
	WorkZone _workZone;

	// When this is set to TRUE, the idle callback will emit a scenegraph change call
	// This is to avoid massive calls to GlobalSceneGraph().sceneChanged() on each
	// and every selection change.
	mutable bool _requestSceneGraphChange;
	mutable bool _requestWorkZoneRecalculation;

	// A simple set that gets filled after the SelectableSortedSet is populated.
	// greebo: I used this to merge two SelectionPools (entities and primitives)
	// with a preferred sorting (see RadiantSelectionSystem::testSelectScene)
	typedef std::list<ISelectable*> SelectablesList;

public:
	static ShaderPtr _state;
private:
	SelectionInfo _selectionInfo;

    sigc::signal<void, const ISelectable&> _sigSelectionChanged;

	typedef std::map<std::size_t, ManipulatorPtr> Manipulators;
	Manipulators _manipulators;

	// The currently active manipulator
	ManipulatorPtr _activeManipulator;
	Manipulator::Type _defaultManipulatorType;

	// state
	EMode _mode;
	EComponentMode _componentMode;

	std::size_t _countPrimitive;
	std::size_t _countComponent;

	// The internal list to keep track of the selected instances (components and primitives)
	typedef SelectedNodeList SelectionListType;
	SelectionListType _selection;
	SelectionListType _componentSelection;

	void recalculatePivot2World();
	mutable bool _pivotChanged;
	bool _pivotMoving;

	// The coordinates of the mouse pointer when the manipulation starts
	Vector2 _deviceStart;

	bool nothingSelected() const;

	void keyChanged();

public:

	RadiantSelectionSystem();

	/** greebo: Returns a structure with all the related
	 * information about the current selection (brush count,
	 * entity count, etc.)
	 */
	const SelectionInfo& getSelectionInfo();

	void onSceneBoundsChanged();

	void pivotChanged() const;

  	void pivotChangedSelection(const ISelectable& selectable);

	void addObserver(Observer* observer);
	void removeObserver(Observer* observer);

	void SetMode(EMode mode);
	EMode Mode() const;

	void SetComponentMode(EComponentMode mode);
	EComponentMode ComponentMode() const;

	// Returns the ID of the registered manipulator
	std::size_t registerManipulator(const ManipulatorPtr& manipulator);
	void unregisterManipulator(const ManipulatorPtr& manipulator);

	Manipulator::Type getActiveManipulatorType() override;
	const ManipulatorPtr& getActiveManipulator() override;
	void setActiveManipulator(std::size_t manipulatorId) override;
	void setActiveManipulator(Manipulator::Type manipulatorType) override;

	std::size_t countSelected() const;
	std::size_t countSelectedComponents() const;

	void onSelectedChanged(const scene::INodePtr& node, const ISelectable& selectable);
	void onComponentSelection(const scene::INodePtr& node, const ISelectable& selectable);

    SelectionChangedSignal signal_selectionChanged() const
    {
        return _sigSelectionChanged;
    }

	scene::INodePtr ultimateSelected();
	scene::INodePtr penultimateSelected();

	void setSelectedAll(bool selected);
	void setSelectedAllComponents(bool selected);

	void foreachSelected(const Visitor& visitor);
	void foreachSelectedComponent(const Visitor& visitor);

	void foreachSelected(const std::function<void(const scene::INodePtr&)>& functor);
	void foreachSelectedComponent(const std::function<void(const scene::INodePtr&)>& functor);

	void foreachBrush(const std::function<void(Brush&)>& functor);
	void foreachFace(const std::function<void(Face&)>& functor);
	void foreachPatch(const std::function<void(Patch&)>& functor);

	void startMove();

	void deselectAll();

	void SelectPoint(const render::View& view, const Vector2& device_point, const Vector2& device_epsilon, EModifier modifier, bool face);
	void SelectArea(const render::View& view, const Vector2& device_point, const Vector2& device_delta, EModifier modifier, bool face);

	// These are the "callbacks" that are used by the Manipulatables
	void rotate(const Quaternion& rotation);
	void scale(const Vector3& scaling);

	void onManipulationStart() override;
	void onManipulationChanged() override;
	void onManipulationEnd() override;

	void rotateSelected(const Quaternion& rotation);
	void translateSelected(const Vector3& translation);
	void scaleSelected(const Vector3& scaling);

	void freezeTransforms();

	const WorkZone& getWorkZone();

	void renderSolid(RenderableCollector& collector, const VolumeTest& volume) const override;
	void renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const override;

	void setRenderSystem(const RenderSystemPtr& renderSystem) override
	{}

	std::size_t getHighlightFlags() override
	{
		return Highlight::NoHighlight; // never highlighted
	}

	const Matrix4& getPivot2World() const override;

	static void constructStatic();
	static void destroyStatic();

	// RegisterableModule implementation
	virtual const std::string& getName() const override;
	virtual const StringSet& getDependencies() const override;
	virtual void initialiseModule(const ApplicationContext& ctx) override;
	virtual void shutdownModule() override;

protected:
	// Called when the app is idle to recalculate the workzone (if necessary)
	virtual void onIdle();

	// Traverses the scene and adds any selectable nodes matching the given SelectionTest to the "targetList".
	void testSelectScene(SelectablesList& targetList, SelectionTest& test,
						 const render::View& view, SelectionSystem::EMode mode,
						 SelectionSystem::EComponentMode componentMode);

private:
	void notifyObservers(const scene::INodePtr& node, bool isComponent);

	// Command targets used to connect to the event system
	void toggleManipulatorMode(Manipulator::Type type, bool newState);

	void activateDefaultMode();

	void toggleEntityMode(bool newState);
	void toggleGroupPartMode(bool newState);

	void toggleComponentMode(EComponentMode mode, bool newState);

	void onManipulatorModeChanged();
	void onComponentModeChanged();

	void checkComponentModeSelectionMode(const ISelectable& selectable); // connects to the selection change signal

	void performPointSelection(const SelectablesList& candidates, EModifier modifier);

	void deselectCmd(const cmd::ArgumentList& args);

	void onMapEvent(IMap::MapEvent ev);
};

}
