#ifndef EVENTMAPPER_H_
#define EVENTMAPPER_H_

#include <string>
#include <map>
#include "xmlutil/Node.h"
#include "gdk/gdkevents.h"

#include "EventLib.h"

namespace ui 
{
	
	namespace {
		float DEFAULT_STRAFE_SPEED = 0.65f;
		int DEFAULT_MIN_SELECTION_COUNT = -1;
		
		// The mapping between mouse button IDs and "logical" mouse buttons (i.e. MOUSE_LEFT)
		typedef std::map<const std::string, unsigned int> ButtonIdMap;
		typedef std::map<const std::string, unsigned int> ModifierBitIndexMap;
		
		// This represents a condition that has to fulfilled in order to recognize an event
		struct ConditionStruc {
			unsigned int buttonId;
			unsigned int modifierFlags;
			int minSelectionCount; 
		};
		
		typedef std::map<const XYViewEvent, ConditionStruc> XYConditionMap;
		typedef std::map<const ObserverEvent, ConditionStruc> ObserverConditionMap;
		typedef std::map<const CamViewEvent, ConditionStruc> CameraConditionMap;
	}

class EventMapper {

	ButtonIdMap _buttonId;
	ModifierBitIndexMap _modifierBitIndex;
	
	// The conditions for xyview, camview and windowobservers
	XYConditionMap _xyConditions;
	ObserverConditionMap _observerConditions;
	CameraConditionMap _cameraConditions;
	
	ConditionStruc _toggleStrafeCondition;
	ConditionStruc _toggleForwardStrafeCondition;
	float _strafeSpeed;
	float _forwardStrafeFactor;

public:
	// Constructor
	EventMapper();
	
	// Destructor
	~EventMapper() {};

	// Return the ObserverEvent type for a given GdkEventButton
	CamViewEvent getCameraViewEvent(GdkEventButton* event);

	// Return the ObserverEvent type for a given GdkEventButton
	ObserverEvent getObserverEvent(GdkEventButton* event);
	ObserverEvent getObserverEvent(const unsigned int& state);

	// Return the current XYView event for a GdkEventMotion state or an GdkEventButton
	XYViewEvent getXYViewEvent(GdkEventButton* event);
	XYViewEvent getXYViewEvent(const unsigned int& state);
	
	bool stateMatchesXYViewEvent(const XYViewEvent& xyViewEvent, GdkEventButton* event);
	bool stateMatchesXYViewEvent(const XYViewEvent& xyViewEvent, const unsigned int& state);
	
	bool stateMatchesObserverEvent(const ObserverEvent& observerEvent, GdkEventButton* event);
	
	bool stateMatchesCameraViewEvent(const CamViewEvent& camViewEvent, GdkEventButton* event);
	
	std::string printXYViewEvent(const XYViewEvent& xyViewEvent);
	std::string printObserverEvent(const ObserverEvent& observerEvent);
	
	float getCameraStrafeSpeed();
	float getCameraForwardStrafeFactor();
	bool EventMapper::strafeActive(unsigned int& state);
	bool EventMapper::strafeForwardActive(unsigned int& state);

private:

	// Loads the button and modifier definitions from the XMLRegistry
	void loadButtonDefinitions();
	void loadModifierDefinitions();
	
	void loadXYViewEventDefinitions();
	void loadObserverEventDefinitions(); 
	void loadCameraEventDefinitions();
	void loadCameraStrafeDefinitions();
	
	// Constructs a condition out of the given <node>
	ConditionStruc getCondition(xml::Node node);
	
	unsigned int getButtonId(const std::string& buttonName);
	// Returns the bit index of the specified modifier by name (returns -1 if failed)
	int getModifierBitIndex(const std::string& modifierName);
	
	// Constructs a modifier bit field out of the given string
	unsigned int getModifierFlags(const std::string& modifierStr);
	
	// Translates a GTK event->state bitfield and returns it
	unsigned int getKeyboardFlags(const unsigned int& state);
	unsigned int getButtonFlags(const unsigned int& state);
	
	// Looks up if the given <button> and <modifierState> matches any conditions
	XYViewEvent findXYViewEvent(const unsigned int& button, const unsigned int& modifierFlags);
	ObserverEvent findObserverEvent(const unsigned int& button, const unsigned int& modifierFlags);
	CamViewEvent findCameraViewEvent(const unsigned int& button, const unsigned int& modifierFlags);
	
	// Looks up if the given <button> and <modifierState> and returns true if the given <xyViewEvent> matches
	bool matchXYViewEvent(const XYViewEvent& xyViewEvent, const unsigned int& button, const unsigned int& modifierFlags);
	bool matchObserverEvent(const ObserverEvent& observerEvent, const unsigned int& button, const unsigned int& modifierFlags);
	bool matchCameraViewEvent(const CamViewEvent& camViewEvent, const unsigned int& button, const unsigned int& modifierFlags);
};

} // namespace ui

ui::EventMapper& GlobalEventMapper();

#endif /*EVENTMAPPER_H_*/
