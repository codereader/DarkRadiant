#ifndef MOUSEEVENTS_H_
#define MOUSEEVENTS_H_

#include "ieventmanager.h"
#include "iselection.h"

#include <string>
#include <map>
#include "xmlutil/Node.h"
#include "gdk/gdkevents.h"

#include "Modifiers.h"

class MouseEventManager :
	public IMouseEvents
{
	// The mapping between mouse button IDs and "logical" mouse buttons (i.e. MOUSE_LEFT)
	typedef std::map<const std::string, unsigned int> ButtonIdMap;
	
	// This represents a condition that has to fulfilled in order to recognize an event
	struct ConditionStruc {
		unsigned int buttonId;
		unsigned int modifierFlags;
		int minSelectionCount; 
	};
	
	typedef std::map<const ui::XYViewEvent, ConditionStruc> XYConditionMap;
	typedef std::map<const ui::ObserverEvent, ConditionStruc> ObserverConditionMap;
	typedef std::map<const ui::CamViewEvent, ConditionStruc> CameraConditionMap;

	ButtonIdMap _buttonId;
	
	// The conditions for xyview, camview and windowobservers
	XYConditionMap _xyConditions;
	ObserverConditionMap _observerConditions;
	CameraConditionMap _cameraConditions;
	
	ConditionStruc _toggleStrafeCondition;
	ConditionStruc _toggleForwardStrafeCondition;
	float _strafeSpeed;
	float _forwardStrafeFactor;
	
	// The reference to the modifier class (is passed by the EventManager)
	Modifiers& _modifiers;
	SelectionSystem* _selectionSystem;
	
	unsigned int _activeFlags;

public:
	// Constructor
	MouseEventManager(Modifiers& modifiers);
	
	// Destructor
	~MouseEventManager() {}
	
	// Loads all the definitions
	void initialise();

	void connectSelectionSystem(SelectionSystem* selectionSystem);

	// Return the ObserverEvent type for a given GdkEventButton
	ui::CamViewEvent getCameraViewEvent(GdkEventButton* event);

	// Return the ObserverEvent type for a given GdkEventButton
	ui::ObserverEvent getObserverEvent(GdkEventButton* event);
	ui::ObserverEvent getObserverEvent(const unsigned int& state);

	// Return the current XYView event for a GdkEventMotion state or an GdkEventButton
	ui::XYViewEvent getXYViewEvent(GdkEventButton* event);
	ui::XYViewEvent getXYViewEvent(const unsigned int& state);
	
	bool stateMatchesXYViewEvent(const ui::XYViewEvent& xyViewEvent, GdkEventButton* event);
	bool stateMatchesXYViewEvent(const ui::XYViewEvent& xyViewEvent, const unsigned int& state);
	
	bool stateMatchesObserverEvent(const ui::ObserverEvent& observerEvent, GdkEventButton* event);
	
	bool stateMatchesCameraViewEvent(const ui::CamViewEvent& camViewEvent, GdkEventButton* event);
	
	std::string printXYViewEvent(const ui::XYViewEvent& xyViewEvent);
	std::string printObserverEvent(const ui::ObserverEvent& observerEvent);
	
	float getCameraStrafeSpeed();
	float getCameraForwardStrafeFactor();
	bool strafeActive(unsigned int& state);
	bool strafeForwardActive(unsigned int& state);
	
	// Updates the status text with the according mouse event state
	void updateStatusText(GdkEventKey* event);
	
private:
	
	std::string getShortButtonName(const std::string& longName);

	// Loads the button and modifier definitions from the XMLRegistry
	void loadButtonDefinitions();
	
	void loadXYViewEventDefinitions();
	void loadObserverEventDefinitions(); 
	void loadCameraEventDefinitions();
	void loadCameraStrafeDefinitions();
	
	// Constructs a condition out of the given <node>
	ConditionStruc getCondition(xml::Node node);
	
	unsigned int getButtonId(const std::string& buttonName);
	
	// Translates a GTK event->state bitfield and returns it
	unsigned int getButtonFlags(const unsigned int& state);
	
	// Looks up if the given <button> and <modifierState> matches any conditions
	ui::XYViewEvent findXYViewEvent(const unsigned int& button, const unsigned int& modifierFlags);
	ui::ObserverEvent findObserverEvent(const unsigned int& button, const unsigned int& modifierFlags);
	ui::CamViewEvent findCameraViewEvent(const unsigned int& button, const unsigned int& modifierFlags);
	
	// Looks up if the given <button> and <modifierState> and returns true if the given <xyViewEvent> matches
	bool matchXYViewEvent(const ui::XYViewEvent& xyViewEvent, const unsigned int& button, const unsigned int& modifierFlags);
	bool matchObserverEvent(const ui::ObserverEvent& observerEvent, const unsigned int& button, const unsigned int& modifierFlags);
	bool matchCameraViewEvent(const ui::CamViewEvent& camViewEvent, const unsigned int& button, const unsigned int& modifierFlags);
}; // class MouseEvents

#endif /*MOUSEEVENTS_H_*/
