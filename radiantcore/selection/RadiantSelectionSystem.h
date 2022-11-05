#pragma once

#include "irenderable.h"
#include "iselection.h"
#include "iselectiontest.h"
#include "icommandsystem.h"
#include "imap.h"

#include "selectionlib.h"
#include "SelectedNodeList.h"

#include "SceneManipulationPivot.h"

namespace selection
{

class RadiantSelectionSystem final :
	public SelectionSystem,
	public Renderable,
    public ISceneSelectionTesterFactory
{
private:
	SceneManipulationPivot _pivot;

    std::set<Observer*> _observers;

	// The 3D volume surrounding the most recent selection.
	WorkZone _workZone;

	// When this is set to TRUE, the idle callback will emit a scenegraph change call
	// This is to avoid massive calls to GlobalSceneGraph().sceneChanged() on each
	// and every selection change.
	mutable bool _requestWorkZoneRecalculation;

	// A simple set that gets filled after the SelectableSortedSet is populated.
	// greebo: I used this to merge two SelectionPools (entities and primitives)
	// with a preferred sorting (see RadiantSelectionSystem::testSelectScene)
	typedef std::list<ISelectable*> SelectablesList;

	SelectionInfo _selectionInfo;

    sigc::signal<void, const ISelectable&> _sigSelectionChanged;

    std::map<std::size_t, ISceneManipulator::Ptr> _manipulators;

	// The currently active manipulator
    ISceneManipulator::Ptr _activeManipulator;
    IManipulator::Type _defaultManipulatorType;

	// state
	SelectionMode _mode;
    ComponentSelectionMode _componentMode;

	std::size_t _countPrimitive;
	std::size_t _countComponent;

	// The internal list to keep track of the selected instances (components and primitives)
    SelectedNodeList _selection;
    SelectedNodeList _componentSelection;

	// The coordinates of the mouse pointer when the manipulation starts
	Vector2 _deviceStart;

	bool nothingSelected() const;

	sigc::signal<void, IManipulator::Type> _sigActiveManipulatorChanged;
	sigc::signal<void, SelectionMode> _sigSelectionModeChanged;
	sigc::signal<void, ComponentSelectionMode> _sigComponentModeChanged;

    bool _selectionFocusActive;
    std::set<scene::INodePtr> _selectionFocusPool;

public:
	RadiantSelectionSystem();

	/** greebo: Returns a structure with all the related
	 * information about the current selection (brush count,
	 * entity count, etc.)
	 */
	const SelectionInfo& getSelectionInfo() override;

	void onSceneBoundsChanged();

	void pivotChanged() override;

  	void pivotChangedSelection(const ISelectable& selectable);

	void addObserver(Observer* observer) override;
	void removeObserver(Observer* observer) override;

	void setSelectionMode(SelectionMode mode) override;
    SelectionMode getSelectionMode() const override;

	void SetComponentMode(ComponentSelectionMode mode) override;
    ComponentSelectionMode ComponentMode() const override;

	sigc::signal<void, SelectionMode>& signal_selectionModeChanged() override;
	sigc::signal<void, ComponentSelectionMode>& signal_componentModeChanged() override;

	// Returns the ID of the registered manipulator
	std::size_t registerManipulator(const ISceneManipulator::Ptr& manipulator) override;
	void unregisterManipulator(const ISceneManipulator::Ptr& manipulator) override;

    IManipulator::Type getActiveManipulatorType() override;
	const ISceneManipulator::Ptr& getActiveManipulator() override;
	void setActiveManipulator(std::size_t manipulatorId) override;
	void setActiveManipulator(IManipulator::Type manipulatorType) override;
	sigc::signal<void, IManipulator::Type>& signal_activeManipulatorChanged() override;

	std::size_t countSelected() const override;
	std::size_t countSelectedComponents() const override;

	void onSelectedChanged(const scene::INodePtr& node, const ISelectable& selectable) override;
	void onComponentSelection(const scene::INodePtr& node, const ISelectable& selectable) override;

    SelectionChangedSignal signal_selectionChanged() const override
    {
        return _sigSelectionChanged;
    }

	scene::INodePtr ultimateSelected() override;
	scene::INodePtr penultimateSelected() override;

	void setSelectedAll(bool selected) override;
	void setSelectedAllComponents(bool selected) override;

	void foreachSelected(const std::function<void(const scene::INodePtr&)>& functor) override;
	void foreachSelectedComponent(const Visitor& visitor) override;
	void foreachSelectedComponent(const std::function<void(const scene::INodePtr&)>& functor) override;

	void foreachBrush(const std::function<void(Brush&)>& functor) override;
	void foreachFace(const std::function<void(IFace&)>& functor) override;
	void foreachPatch(const std::function<void(IPatch&)>& functor) override;

	std::size_t getSelectedFaceCount() override;
	IFace& getSingleSelectedFace() override;

	void deselectAll();

	void selectPoint(SelectionTest& test, EModifier modifier, bool face) override;
	void selectArea(SelectionTest& test, EModifier modifier, bool face) override;

	void onManipulationStart() override;
	void onManipulationChanged() override;
	void onManipulationEnd() override;
	void onManipulationCancelled() override;

	const WorkZone& getWorkZone() override;
	Vector3 getCurrentSelectionCenter() override;

    void onPreRender(const VolumeTest& volume) override;
    void renderHighlights(IRenderableCollector& collector, const VolumeTest& volume) override
    {}

	void setRenderSystem(const RenderSystemPtr& renderSystem) override
	{}

	std::size_t getHighlightFlags() override
	{
		return Highlight::NoHighlight; // never highlighted
	}

	const Matrix4& getPivot2World() override;

    void toggleSelectionFocus() override;
    bool selectionFocusIsActive() override;
    AABB getSelectionFocusBounds() override;

	// RegisterableModule implementation
	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const IApplicationContext& ctx) override;
	void shutdownModule() override;

    ISceneSelectionTester::Ptr createSceneSelectionTester(SelectionMode mode) override;

private:
    bool nodeCanBeSelectionTested(const scene::INodePtr& node);

    // Sets the selection status of the given selectable. The selection status will
    // be propagated to groups if the current selection mode / focus is allowing that
    void setSelectionStatus(ISelectable* selectable, bool selected);

	// Traverses the scene and adds any selectable nodes matching the given SelectionTest to the "targetList".
	void testSelectScene(SelectablesList& targetList, SelectionTest& test,
        const VolumeTest& view, SelectionMode mode);

	bool higherEntitySelectionPriority() const;

	void notifyObservers(const scene::INodePtr& node, bool isComponent);

	std::size_t getManipulatorIdForType(IManipulator::Type type);

	// Command targets used to connect to the event system
	void toggleManipulatorModeCmd(const cmd::ArgumentList& args);
	void toggleManipulatorMode(IManipulator::Type type);
	void toggleManipulatorModeById(std::size_t manipId);

	void activateDefaultMode();

	void toggleEntityMode(const cmd::ArgumentList& args);
	void toggleGroupPartMode(const cmd::ArgumentList& args);
	void toggleMergeActionMode(const cmd::ArgumentList& args);

	void toggleSelectionFocus(const cmd::ArgumentList& args);

	void toggleComponentMode(ComponentSelectionMode mode);
	void toggleComponentModeCmd(const cmd::ArgumentList& args);

	void onManipulatorModeChanged();
	void onComponentModeChanged();

	void checkComponentModeSelectionMode(const ISelectable& selectable); // connects to the selection change signal

	void performPointSelection(const SelectablesList& candidates, EModifier modifier);
	void onSelectionPerformed();

	void deselectCmd(const cmd::ArgumentList& args);

	void onMapEvent(IMap::MapEvent ev);
};

}
