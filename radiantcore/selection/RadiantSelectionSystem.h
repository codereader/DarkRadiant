#pragma once

#include "iselectiontest.h"
#include "iregistry.h"
#include "irenderable.h"
#include "iselection.h"
#include "iradiant.h"
#include "icommandsystem.h"
#include "imap.h"

#include "selectionlib.h"
#include "math/Matrix4.h"
#include "SelectedNodeList.h"

#include "SceneManipulationPivot.h"

namespace selection
{

class RadiantSelectionSystem :
	public SelectionSystem,
	public Renderable
{
private:
	SceneManipulationPivot _pivot;

	typedef std::set<Observer*> ObserverList;
	ObserverList _observers;

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

private:
	SelectionInfo _selectionInfo;

    sigc::signal<void, const ISelectable&> _sigSelectionChanged;

	typedef std::map<std::size_t, ISceneManipulator::Ptr> Manipulators;
	Manipulators _manipulators;

	// The currently active manipulator
    ISceneManipulator::Ptr _activeManipulator;
    IManipulator::Type _defaultManipulatorType;

	// state
	EMode _mode;
    ComponentSelectionMode _componentMode;

	std::size_t _countPrimitive;
	std::size_t _countComponent;

	// The internal list to keep track of the selected instances (components and primitives)
	typedef SelectedNodeList SelectionListType;
	SelectionListType _selection;
	SelectionListType _componentSelection;

	// The coordinates of the mouse pointer when the manipulation starts
	Vector2 _deviceStart;

	bool nothingSelected() const;

	sigc::signal<void, selection::IManipulator::Type> _sigActiveManipulatorChanged;
	sigc::signal<void, EMode> _sigSelectionModeChanged;
	sigc::signal<void, ComponentSelectionMode> _sigComponentModeChanged;

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

	void SetMode(EMode mode) override;
	EMode Mode() const override;

	void SetComponentMode(ComponentSelectionMode mode) override;
    ComponentSelectionMode ComponentMode() const override;

	sigc::signal<void, EMode>& signal_selectionModeChanged() override;
	sigc::signal<void, ComponentSelectionMode>& signal_componentModeChanged() override;

	// Returns the ID of the registered manipulator
	std::size_t registerManipulator(const ISceneManipulator::Ptr& manipulator) override;
	void unregisterManipulator(const ISceneManipulator::Ptr& manipulator) override;

    IManipulator::Type getActiveManipulatorType() override;
	const ISceneManipulator::Ptr& getActiveManipulator() override;
	void setActiveManipulator(std::size_t manipulatorId) override;
	void setActiveManipulator(IManipulator::Type manipulatorType) override;
	sigc::signal<void, selection::IManipulator::Type>& signal_activeManipulatorChanged() override;

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

	// RegisterableModule implementation
	virtual const std::string& getName() const override;
	virtual const StringSet& getDependencies() const override;
	virtual void initialiseModule(const IApplicationContext& ctx) override;
	virtual void shutdownModule() override;

protected:
	// Traverses the scene and adds any selectable nodes matching the given SelectionTest to the "targetList".
	void testSelectScene(SelectablesList& targetList, SelectionTest& test,
        const VolumeTest& view, SelectionSystem::EMode mode,
        ComponentSelectionMode componentMode);

private:
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
