#pragma once

#include "iregistry.h"
#include "irenderable.h"
#include "iselection.h"
#include "icommandsystem.h"

#include "selectionlib.h"
#include "math/Matrix4.h"
#include "gtkutil/event/SingleIdleCallback.h"
#include "Manipulator.h"
#include "Manipulatables.h"
#include "TranslateManipulator.h"
#include "RotateManipulator.h"
#include "ScaleManipulator.h"
#include "DragManipulator.h"
#include "ClipManipulator.h"
#include "Selectors.h"
#include "SelectedNodeList.h"

/* greebo: This can be tricky to understand (and I don't know if I do :D), but
 * I'll try:
 *
 * This is the implementation of the Abstract Base Class SelectionSystem, so
 * this is the point where the selected instances are kept track of. It provides
 * functions to select points and areas, and makes sure that the according
 * instances are being selected on mouse release. The mouse calls are handled by
 * the RadiantWindowObserver (or its helper classes SelectObserver and
 * ManipulateObserver). The observers call the routines here like SelectPoint()
 * and SelectArea().
 *
 * Basically, this class responds to the calls that come from
 * RadiantWindowObserver. If the user clicks anything, the mouse callback
 * function of the WindowObserverSystem is invoked. These observer routines call
 * the methods of the selectionsystem.
 *
 * For example, if the left mouse button is pressed, the Observer onMouseDown
 * calls the SelectManipulator method of this system, which itself makes use of
 * the Manipulator classes and their testSelect routines.
 *
 * On mouse release, the according manipulator methods are invoked and these in
 * turn call back here. The actual transformations are performed using
 * TransformationVisitors like TranslateSelected or such.
 *
 * Note that the RadiantSelectionSystem class derives from all the possible
 * Manipulatables (Translatable, Rotatable, Scalable). This way the system can
 * pass _itself_ to the Manipulator constructors, which call their
 * Manipulatable's Transform() method, which in turn calls the
 * RadiantSelectionSystem's method translate() (in the example of translation).
 * So basically the dog seems to be hunting its own tail here, but then again,
 * it doesn't.
 *
 */
// RadiantSelectionSystem
class RadiantSelectionSystem :
	public SelectionSystem,
	public Translatable,
	public Rotatable,
	public Scalable,
	public Renderable,
	protected gtkutil::SingleIdleCallback
{
	mutable Matrix4 _pivot2world;
	Matrix4 _pivot2worldStart;
	Matrix4 _manip2pivotStart;

	typedef std::list<Observer*> ObserverList;
	ObserverList _observers;

	Vector3 _translation;
	Quaternion _rotation;
	Vector3 _scale;

	// The 3D volume surrounding the most recent selection.
	selection::WorkZone _workZone;

	// When this is set to TRUE, the idle callback will emit a scenegraph change call
	// This is to avoid massive calls to GlobalSceneGraph().sceneChanged() on each
	// and every selection change.
	mutable bool _requestSceneGraphChange;
	mutable bool _requestWorkZoneRecalculation;

public:
	static ShaderPtr _state;
private:
	SelectionInfo _selectionInfo;

    sigc::signal<void, const Selectable&> _sigSelectionChanged;

	EManipulatorMode _defaultManipulatorMode;
	EManipulatorMode _manipulatorMode;
	// The currently active manipulator
	Manipulator* _manipulator;
	bool _currentManipulatorModeSupportsComponentEditing;

	// state
	bool _undoBegun;
	EMode _mode;
	EComponentMode _componentMode;

	std::size_t _countPrimitive;
	std::size_t _countComponent;

	// The possible manipulators
	TranslateManipulator _translateManipulator;
	RotateManipulator _rotateManipulator;
	ScaleManipulator _scaleManipulator;
	DragManipulator _dragManipulator;
	ClipManipulator _clipManipulator;

	// The internal list to keep track of the selected instances (components and primitives)
	typedef SelectedNodeList SelectionListType;
	SelectionListType _selection;
	SelectionListType _componentSelection;

	void ConstructPivot();
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

  	void pivotChangedSelection(const Selectable& selectable);

	void addObserver(Observer* observer);
	void removeObserver(Observer* observer);

	void SetMode(EMode mode);
	EMode Mode() const;

	void SetComponentMode(EComponentMode mode);
	EComponentMode ComponentMode() const;

	void SetManipulatorMode(EManipulatorMode mode);
	EManipulatorMode ManipulatorMode() const;

	std::size_t countSelected() const;
	std::size_t countSelectedComponents() const;

	void onSelectedChanged(const scene::INodePtr& node, const Selectable& selectable);
	void onComponentSelection(const scene::INodePtr& node, const Selectable& selectable);

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

	bool SelectManipulator(const render::View& view, const Vector2& device_point, const Vector2& device_epsilon);

	void deselectAll();

	void SelectPoint(const render::View& view, const Vector2& device_point, const Vector2& device_epsilon, EModifier modifier, bool face);
	void SelectArea(const render::View& view, const Vector2& device_point, const Vector2& device_delta, EModifier modifier, bool face);

	// These are the "callbacks" that are used by the Manipulatables
	void translate(const Vector3& translation);
	void rotate(const Quaternion& rotation);
	void scale(const Vector3& scaling);

	void outputTranslation(std::ostream& ostream);
	void outputRotation(std::ostream& ostream);
	void outputScale(std::ostream& ostream);

	void rotateSelected(const Quaternion& rotation);
	void translateSelected(const Vector3& translation);
	void scaleSelected(const Vector3& scaling);

	void MoveSelected(const render::View& view, const Vector2& devicePoint);

	void cancelMove();

	void endMove();
	void freezeTransforms();

	const selection::WorkZone& getWorkZone();

	void renderSolid(RenderableCollector& collector, const VolumeTest& volume) const;
	void renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const;

	void setRenderSystem(const RenderSystemPtr& renderSystem)
	{}

	bool isHighlighted() const
	{
		return false; // never highlighted
	}

	const Matrix4& GetPivot2World() const;

	static void constructStatic();
	static void destroyStatic();

	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);
	virtual void shutdownModule();

protected:
	// Called when GTK is idle to recalculate the workzone (if necessary)
	virtual void onGtkIdle();

	// Traverses the scene and adds any selectable nodes matching the given SelectionTest to the "targetList".
	void testSelectScene(SelectablesList& targetList, SelectionTest& test,
						 const render::View& view, SelectionSystem::EMode mode,
						 SelectionSystem::EComponentMode componentMode);

private:
	void notifyObservers(const scene::INodePtr& node, bool isComponent);

	// Command targets used to connect to the event system
	void toggleDefaultManipulatorMode(bool newState);
	void toggleDragManipulatorMode(bool newState);
	void toggleTranslateManipulatorMode(bool newState);
	void toggleRotateManipulatorMode(bool newState);
	void toggleClipManipulatorMode(bool newState);
	void toggleManipulatorMode(EManipulatorMode mode, bool newState);

	void activateDefaultMode();

	void toggleEntityMode(bool newState);
	void toggleGroupPartMode(bool newState);

	void toggleVertexComponentMode(bool newState);
	void toggleFaceComponentMode(bool newState);
	void toggleEdgeComponentMode(bool newState);
	void toggleComponentMode(EComponentMode mode, bool newState);

	void onManipulatorModeChanged();
	void onComponentModeChanged();

	void checkComponentModeSelectionMode(const Selectable& selectable); // connects to the selection change signal

	void deselectCmd(const cmd::ArgumentList& args);
};
