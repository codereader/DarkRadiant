#include "EventMapper.h"

#include "iregistry.h"
#include "iselection.h"
#include <iostream>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>

namespace ui {

	namespace {
		// Needed for boost::algorithm::split		
		typedef std::vector<std::string> StringParts;
	}

EventMapper::EventMapper() {
	loadButtonDefinitions();
	loadModifierDefinitions();
	
	loadXYViewEventDefinitions();
	loadObserverEventDefinitions();
	
	loadCameraEventDefinitions();
	loadCameraStrafeDefinitions();
}

unsigned int EventMapper::getButtonId(const std::string& buttonName) {
	ButtonIdMap::iterator it = _buttonId.find(buttonName);
   	if (it != _buttonId.end()) {
   		return it->second;
   	}
   	else {
   		globalOutputStream() << "EventMapper: Warning: Button " << buttonName.c_str() << " not found, returning ID=0\n";
   		return 0;
   	}
}

int EventMapper::getModifierBitIndex(const std::string& modifierName) {
	ModifierBitIndexMap::iterator it = _modifierBitIndex.find(modifierName);
   	if (it != _modifierBitIndex.end()) {
   		return it->second;
   	}
   	else {
   		globalOutputStream() << "EventMapper: Warning: Modifier " << modifierName.c_str() << " not found, returning -1\n";
   		return -1;
   	}
}

unsigned int EventMapper::getModifierFlags(const std::string& modifierStr) {
	StringParts parts;
	boost::algorithm::split(parts, modifierStr, boost::algorithm::is_any_of("+"));
	
	// Do we have any modifiers at all?
	if (parts.size() > 0) {
		unsigned int returnValue = 0;
		
		// Cycle through all the modifier names and construct the bitfield
		for (unsigned int i = 0; i < parts.size(); i++) {
			if (parts[i] == "") continue;
			
			// Try to find the modifierBitIndex
			int bitIndex = getModifierBitIndex(parts[i]);
						
			// Was anything found? 
			if (bitIndex >= 0) {
				unsigned int bitValue = (1 << static_cast<unsigned int>(bitIndex));
				returnValue |= bitValue;
			}
		}
		
		return returnValue;
	}
	else {
		return 0;
	}
}

ConditionStruc EventMapper::getCondition(xml::Node node) {
	const std::string button = node.getAttributeValue("button");
	const std::string modifiers = node.getAttributeValue("modifiers");
	const std::string minSelectionCount = node.getAttributeValue("minSelectionCount");
	
	ConditionStruc returnValue;
	
	returnValue.buttonId = getButtonId(button);
	returnValue.modifierFlags = getModifierFlags(modifiers);
	
	try {
		returnValue.minSelectionCount = boost::lexical_cast<int>(minSelectionCount);
	}
	catch (boost::bad_lexical_cast e) {
		returnValue.minSelectionCount = DEFAULT_MIN_SELECTION_COUNT;
	}
		
	return returnValue;
}

void EventMapper::loadCameraStrafeDefinitions() {
	// Find all the camera strafe definitions
	xml::NodeList strafeList = GlobalRegistry().findXPath("user/ui/interface/cameraview/strafemode");
	
	if (strafeList.size() > 0) {
		// Get the strafe condition flags
		_toggleStrafeCondition.modifierFlags = getModifierFlags(strafeList[0].getAttributeValue("toggle"));
		_toggleForwardStrafeCondition.modifierFlags = getModifierFlags(strafeList[0].getAttributeValue("forward"));
		
		try {
			_strafeSpeed = boost::lexical_cast<float>(strafeList[0].getAttributeValue("speed"));
		}
		catch (boost::bad_lexical_cast e) {
			_strafeSpeed = DEFAULT_STRAFE_SPEED;
		}
		
		try {
			_forwardStrafeFactor = boost::lexical_cast<float>(strafeList[0].getAttributeValue("forwardFactor"));
		}
		catch (boost::bad_lexical_cast e) {
			_forwardStrafeFactor = 1.0f;
		}
	}
	else {
		// No Camera strafe definitions found!
		globalOutputStream() << "EventMapper: Critical: No camera strafe definitions found!\n";
	}
}

void EventMapper::loadCameraEventDefinitions() {
	// Find all the camera definitions
	xml::NodeList eventList = GlobalRegistry().findXPath("user/ui/interface/cameraview//event");
	
	if (eventList.size() > 0) {
		globalOutputStream() << "EventMapper: Camera Definitions found: " << eventList.size() << "\n";
		for (unsigned int i = 0; i < eventList.size(); i++) {
			// Get the event name
			const std::string eventName = eventList[i].getAttributeValue("name");
			
			// Check if any recognised event names are found and construct the according condition.
			if (eventName == "EnableFreeLookMode") {
				_cameraConditions[camEnableFreeLookMode] = getCondition(eventList[i]);
			}
			else if (eventName == "DisableFreeLookMode") {
				_cameraConditions[camDisableFreeLookMode] = getCondition(eventList[i]);
			}
			else {
				globalOutputStream() << "EventMapper: Warning: Ignoring unkown event name: " << eventName.c_str() << "\n";
			}
		}
	}
	else {
		// No Camera definitions found!
		globalOutputStream() << "EventMapper: Critical: No camera event definitions found!\n";
	}
}

void EventMapper::loadObserverEventDefinitions() {
	// Find all the xy view definitions
	xml::NodeList eventList = GlobalRegistry().findXPath("user/ui/interface/observer//event");
	
	if (eventList.size() > 0) {
		globalOutputStream() << "EventMapper: Observer Definitions found: " << eventList.size() << "\n";
		for (unsigned int i = 0; i < eventList.size(); i++) {
			// Get the event name
			const std::string eventName = eventList[i].getAttributeValue("name");
			
			// Check if any recognised event names are found and construct the according condition.
			if (eventName == "Manipulate") {
				_observerConditions[obsManipulate] = getCondition(eventList[i]);
			}
			else if (eventName == "Select") {
				_observerConditions[obsSelect] = getCondition(eventList[i]);
			}
			else if (eventName == "ToggleSelection") {
				_observerConditions[obsToggle] = getCondition(eventList[i]);
			}
			else if (eventName == "ToggleFaceSelection") {
				_observerConditions[obsToggleFace] = getCondition(eventList[i]);
			}
			else if (eventName == "CycleSelection") {
				_observerConditions[obsReplace] = getCondition(eventList[i]);
			}
			else if (eventName == "CycleFaceSelection") {
				_observerConditions[obsReplaceFace] = getCondition(eventList[i]);
			}
			else if (eventName == "CopyTexture") {
				_observerConditions[obsCopyTexture] = getCondition(eventList[i]);
			}
			else if (eventName == "PasteTexture") {
				_observerConditions[obsPasteTexture] = getCondition(eventList[i]);
			}
			else {
				globalOutputStream() << "EventMapper: Warning: Ignoring unkown event name: " << eventName.c_str() << "\n";
			}
		}
	}
	else {
		// No observer definitions found!
		globalOutputStream() << "EventMapper: Critical: No observer event definitions found!\n";
	}
}

void EventMapper::loadXYViewEventDefinitions() {
	// Find all the xy view definitions
	xml::NodeList eventList = GlobalRegistry().findXPath("user/ui/interface/xyview//event");
	
	if (eventList.size() > 0) {
		globalOutputStream() << "EventMapper: XYView Definitions found: " << eventList.size() << "\n";
		for (unsigned int i = 0; i < eventList.size(); i++) {
			// Get the event name
			const std::string eventName = eventList[i].getAttributeValue("name");
			
			// Check if any recognised event names are found and construct the according condition.
			if (eventName == "MoveView") {
				_xyConditions[xyMoveView] = getCondition(eventList[i]);
			}
			else if (eventName == "Select") {
				_xyConditions[xySelect] = getCondition(eventList[i]);
			}
			else if (eventName == "Zoom") {
				_xyConditions[xyZoom] = getCondition(eventList[i]);
			}
			else if (eventName == "CameraMove") {
				_xyConditions[xyCameraMove] = getCondition(eventList[i]);
			}
			else if (eventName == "CameraAngle") {
				_xyConditions[xyCameraAngle] = getCondition(eventList[i]);
			}
			else if (eventName == "NewBrushDrag") {
				_xyConditions[xyNewBrushDrag] = getCondition(eventList[i]);
			}
			else {
				globalOutputStream() << "EventMapper: Warning: Ignoring unkown event name: " << eventName.c_str() << "\n";
			}
		}
	}
	else {
		// No event definitions found!
		globalOutputStream() << "EventMapper: Critical: No XYView event definitions found!\n";
	}
}

void EventMapper::loadButtonDefinitions() {
	// Find all button definitions
	xml::NodeList buttonList = GlobalRegistry().findXPath("user/ui/interface/buttons//button");
	
	if (buttonList.size() > 0) {
		globalOutputStream() << "EventMapper: Buttons found: " << buttonList.size() << "\n";
		for (unsigned int i = 0; i < buttonList.size(); i++) {
			const std::string name = buttonList[i].getAttributeValue("name");
			
			unsigned int id;
			try {
				id = boost::lexical_cast<unsigned int>(buttonList[i].getAttributeValue("id"));
			}
			catch (boost::bad_lexical_cast e) {
				id = 0;
			}
			
			if (name != "" && id > 0) {
				//std::cout << "EventMapper: Found button definition " << name.c_str() << " with ID " << id << "\n";
				
				// Save the button ID into the map
				_buttonId[name] = id;
			} 
			else {
				globalOutputStream() << "EventMapper: Warning: Invalid button definition found.\n";
			}
		}
	}
	else {
		// No Button definitions found!
		globalOutputStream() << "EventMapper: Critical: No button definitions found!\n";
	}
}

void EventMapper::loadModifierDefinitions() {
	// Find all button definitions
	xml::NodeList modifierList = GlobalRegistry().findXPath("user/ui/interface/modifiers//modifier");
	
	if (modifierList.size() > 0) {
		globalOutputStream() << "EventMapper: Modifiers found: " << modifierList.size() << "\n";
		for (unsigned int i = 0; i < modifierList.size(); i++) {
			const std::string name = modifierList[i].getAttributeValue("name");
			
			int bitIndex;
			try {
				bitIndex = boost::lexical_cast<int>(modifierList[i].getAttributeValue("bitIndex"));
			}
			catch (boost::bad_lexical_cast e) {
				bitIndex = -1;
			}
			
			if (name != "" && bitIndex >= 0) {
				//std::cout << "EventMapper: Found modifier definition " << name.c_str() << " with BitIndex " << bitIndex << "\n";
				
				// Save the modifier ID into the map
				_modifierBitIndex[name] = static_cast<unsigned int>(bitIndex);
			} 
			else {
				globalOutputStream() << "EventMapper: Warning: Invalid modifier definition found.\n";
			}
		}
	}
	else {
		// No Button definitions found!
		globalOutputStream() << "EventMapper: Critical: No modifiers definitions found!\n";
	}
}

// Returns a bit field with the according modifier flags set 
unsigned int EventMapper::getKeyboardFlags(const unsigned int& state) {
	unsigned int returnValue = 0;
	
	if ((state & GDK_CONTROL_MASK) != 0) {
    	returnValue |= (1 << getModifierBitIndex("CONTROL"));
	}
	
	if ((state & GDK_SHIFT_MASK) != 0) {
    	returnValue |= (1 << getModifierBitIndex("SHIFT"));
	}
	
	if ((state & GDK_MOD1_MASK) != 0) {
    	returnValue |= (1 << getModifierBitIndex("ALT"));
	}
	
	return returnValue;
}

// Retrieves the button from an GdkEventMotion state 
unsigned int EventMapper::getButtonFlags(const unsigned int& state) {
	if ((state & GDK_BUTTON1_MASK) != 0) return 1;
	if ((state & GDK_BUTTON2_MASK) != 0) return 2;
	if ((state & GDK_BUTTON3_MASK) != 0) return 3;

	return 0;
}

CamViewEvent EventMapper::findCameraViewEvent(const unsigned int& button, const unsigned int& modifierFlags) {
	for (CameraConditionMap::iterator it = _cameraConditions.begin(); it != _cameraConditions.end(); it++) {
		CamViewEvent event = it->first;
		ConditionStruc conditions = it->second;
		
		if (button == conditions.buttonId 
			&& modifierFlags == conditions.modifierFlags 
			&& static_cast<int>(GlobalSelectionSystem().countSelected()) >= conditions.minSelectionCount) 
		{
			return event;
		}
	}
	return camNothing;
}

XYViewEvent EventMapper::findXYViewEvent(const unsigned int& button, const unsigned int& modifierFlags) {
	for (XYConditionMap::iterator it = _xyConditions.begin(); it != _xyConditions.end(); it++) {
		XYViewEvent event = it->first;
		ConditionStruc conditions = it->second;
		
		if (button == conditions.buttonId 
			&& modifierFlags == conditions.modifierFlags 
			&& static_cast<int>(GlobalSelectionSystem().countSelected()) >= conditions.minSelectionCount) 
		{
			return event;
		}
	}
	return xyNothing;
}

ObserverEvent EventMapper::findObserverEvent(const unsigned int& button, const unsigned int& modifierFlags) {
	for (ObserverConditionMap::iterator it = _observerConditions.begin(); it != _observerConditions.end(); it++) {
		ObserverEvent event = it->first;
		ConditionStruc conditions = it->second;
		
		if (button == conditions.buttonId 
			&& modifierFlags == conditions.modifierFlags 
			&& static_cast<int>(GlobalSelectionSystem().countSelected()) >= conditions.minSelectionCount) 
		{
			return event;
		}
	}
	return obsNothing;
}

CamViewEvent EventMapper::getCameraViewEvent(GdkEventButton* event) {
	unsigned int button = event->button;
	unsigned int modifierFlags = getKeyboardFlags(event->state);
	
	return findCameraViewEvent(button, modifierFlags);
}

XYViewEvent EventMapper::getXYViewEvent(GdkEventButton* event) {
	unsigned int button = event->button;
	unsigned int modifierFlags = getKeyboardFlags(event->state);
	
	return findXYViewEvent(button, modifierFlags);
}

// The same as above, just with a state as argument rather than a GdkEventButton
XYViewEvent EventMapper::getXYViewEvent(const unsigned int& state) {
	unsigned int button = getButtonFlags(state);
	unsigned int modifierFlags = getKeyboardFlags(state);
	
	return findXYViewEvent(button, modifierFlags);
}

bool EventMapper::matchXYViewEvent(const XYViewEvent& xyViewEvent, const unsigned int& button, const unsigned int& modifierFlags) {
	XYConditionMap::iterator it = _xyConditions.find(xyViewEvent);
   	if (it != _xyConditions.end()) {
   		// Load the condition
   		ConditionStruc conditions = it->second;
   		
		return (button == conditions.buttonId 
				&& modifierFlags == conditions.modifierFlags 
				&& static_cast<int>(GlobalSelectionSystem().countSelected()) >= conditions.minSelectionCount);
   	}
   	else {
   		globalOutputStream() << "EventMapper: Warning: Query for event " << xyViewEvent << ": not found.\n";
   		return false;
   	}
}

bool EventMapper::matchObserverEvent(const ObserverEvent& observerEvent, const unsigned int& button, const unsigned int& modifierFlags) {
	ObserverConditionMap::iterator it = _observerConditions.find(observerEvent);
   	if (it != _observerConditions.end()) {
   		// Load the condition
   		ConditionStruc conditions = it->second;
		
		return (button == conditions.buttonId 
				&& modifierFlags == conditions.modifierFlags 
				&& static_cast<int>(GlobalSelectionSystem().countSelected()) >= conditions.minSelectionCount);
   	}
   	else {
   		globalOutputStream() << "EventMapper: Warning: Query for event " << observerEvent << ": not found.\n";
   		return false;
   	}
}

bool EventMapper::matchCameraViewEvent(const CamViewEvent& camViewEvent, const unsigned int& button, const unsigned int& modifierFlags) {
	CameraConditionMap::iterator it = _cameraConditions.find(camViewEvent);
   	if (it != _cameraConditions.end()) {
   		// Load the condition
   		ConditionStruc conditions = it->second;
   		
		return (button == conditions.buttonId 
				&& modifierFlags == conditions.modifierFlags 
				&& static_cast<int>(GlobalSelectionSystem().countSelected()) >= conditions.minSelectionCount);
   	}
   	else {
   		globalOutputStream() << "EventMapper: Warning: Query for event " << camViewEvent << ": not found.\n";
   		return false;
   	}
}

bool EventMapper::stateMatchesXYViewEvent(const XYViewEvent& xyViewEvent, GdkEventButton* event) {
	return matchXYViewEvent(xyViewEvent, event->button, getKeyboardFlags(event->state));
}

// The same as above, just with a state as argument rather than a GdkEventButton
bool EventMapper::stateMatchesXYViewEvent(const XYViewEvent& xyViewEvent, const unsigned int& state) {
	return matchXYViewEvent(xyViewEvent, getButtonFlags(state), getKeyboardFlags(state));
}

bool EventMapper::stateMatchesObserverEvent(const ObserverEvent& observerEvent, GdkEventButton* event) {
	return matchObserverEvent(observerEvent, event->button, getKeyboardFlags(event->state));
}

bool EventMapper::stateMatchesCameraViewEvent(const CamViewEvent& camViewEvent, GdkEventButton* event) {
	return matchCameraViewEvent(camViewEvent, event->button, getKeyboardFlags(event->state));
}

ObserverEvent EventMapper::getObserverEvent(GdkEventButton* event) {
	unsigned int button = event->button;
	unsigned int modifierFlags = getKeyboardFlags(event->state);
	
	return findObserverEvent(button, modifierFlags);
}

ObserverEvent EventMapper::getObserverEvent(const unsigned int& state) {
	unsigned int button = getButtonFlags(state);
	unsigned int modifierFlags = getKeyboardFlags(state);
	
	return findObserverEvent(button, modifierFlags);
}

std::string EventMapper::printXYViewEvent(const XYViewEvent& xyViewEvent) {
	
	switch (xyViewEvent) {
		case xyNothing: return "Nothing";
		case xyMoveView: return "MoveView";
		case xySelect: return "Select";
		case xyZoom: return "Zoom";
		case xyCameraMove: return "CameraMove";
		case xyCameraAngle: return "CameraAngle";
		case xyNewBrushDrag: return "NewBrushDrag";
		default: return "Unknown event";
	}
}

std::string EventMapper::printObserverEvent(const ObserverEvent& observerEvent) {
	
	switch (observerEvent) {
		case obsNothing: return "Nothing";
		case obsManipulate: return "Manipulate";
		case obsSelect: return "Select"; 
		case obsToggle: return "Toggle";
		case obsToggleFace: return "ToggleFace";
		case obsReplace: return "Replace";
		case obsReplaceFace: return "ReplaceFace";
		case obsCopyTexture: return "CopyTexture";
		case obsPasteTexture: return "PasteTexture";		
		default: return "Unknown event";
	}
}

float EventMapper::getCameraStrafeSpeed() {
	return _strafeSpeed;
}

float EventMapper::getCameraForwardStrafeFactor() {
	return _forwardStrafeFactor;
}

bool EventMapper::strafeActive(unsigned int& state) {
	return ((getKeyboardFlags(state) & _toggleStrafeCondition.modifierFlags) != 0);
}

bool EventMapper::strafeForwardActive(unsigned int& state) {
	return ((getKeyboardFlags(state) & _toggleForwardStrafeCondition.modifierFlags) != 0);
}

} // namespace ui

ui::EventMapper& GlobalEventMapper() {
	static ui::EventMapper _mapper;
	return _mapper;
}
