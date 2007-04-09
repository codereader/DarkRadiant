#include "ResponseEffect.h"

#include "string/string.h"
#include <boost/algorithm/string/replace.hpp>

ResponseEffect::ResponseEffect() :
	_state(true),
	_eclass(IEntityClassPtr())
{}

// Copy constructor
ResponseEffect::ResponseEffect(const ResponseEffect& other) :
	_effectName(other._effectName),
	_state(other._state),
	_args(other._args),
	_eclass(other._eclass) 
{}

std::string ResponseEffect::getName() const {
	return _effectName;
}

bool ResponseEffect::isActive() const {
	return _state;
}

void ResponseEffect::setActive(bool active) {
	_state = active;
}

void ResponseEffect::setName(const std::string& name) {
	_effectName = name;
	// Update the entityclass pointer
	_eclass = ResponseEffectTypes::Instance().getEClassForName(name);
	
	// Build the argument list, if there are still zero arguments
	if (_args.size() == 0) {
		buildArgumentList();
	}
}

std::string ResponseEffect::getArgument(unsigned int index) const {
	ArgumentList::const_iterator i = _args.find(index);
	
	// Return "" if the argument was not found
	return (i != _args.end()) ? i->second.value : "";
}

void ResponseEffect::setArgument(unsigned int index, const std::string& value) {
	ArgumentList::const_iterator i = _args.find(index);
	
	if (i == _args.end()) {
		// Argument doesn't exist, create it
		Argument newArgument;
		newArgument.value = value;
		
		// Store the argument in the map
		_args[index] = newArgument;
	}
	else {
		// Argument exists
		_args[index].value = value;
	}
}

std::string ResponseEffect::getCaption() const {
	return (_eclass != NULL) ? _eclass->getValueForKey("editor_caption") : ""; 
}

IEntityClassPtr ResponseEffect::getEClass() const {
	return _eclass;
}

ResponseEffect::ArgumentList& ResponseEffect::getArguments() {
	return _args;
}

void ResponseEffect::buildArgumentList() {
	if (_eclass == NULL) return;
	
	for (int i = 1; i < 1000; i++) {
		std::string argType = _eclass->getValueForKey("editor_argType" + intToStr(i));
		std::string argDesc = _eclass->getValueForKey("editor_argDesc" + intToStr(i));
		std::string argTitle = _eclass->getValueForKey("editor_argTitle" + intToStr(i));
		std::string optional = _eclass->getValueForKey("editor_argOptional" + intToStr(i));
		
		if (argType != "") {
			// Check if the argument exists
			ArgumentList::iterator found = _args.find(i);
			if (found == _args.end()) {
				Argument newArgument;
				_args[i] = newArgument;
			}
			
			// Load the values into the structure
			_args[i].type = argType;
			_args[i].desc = argDesc;
			_args[i].title = argTitle;
			_args[i].optional = (optional == "1");
		}
		else {
			// Empty argument type found, break the loop
			break;
		}
	}
}

void ResponseEffect::clearArgumentList() {
	_args.clear();
}

std::string ResponseEffect::getArgumentStr() {
	if (_eclass == NULL) return "Error: eclass pointer invalid.";
	
	std::string returnValue = _eclass->getValueForKey("editor_argString");
	
	for (ArgumentList::iterator i = _args.begin(); i != _args.end(); i++) {
		std::string needle = "[arg" + intToStr(i->first) + "]";
		std::string replacement = i->second.value;
		
		// Check for a bool
		if (i->second.type == "b") {
			replacement = (i->second.value.empty()) ? "no" : "yes";
		}
		
		boost::algorithm::replace_all(returnValue, needle, replacement); 
	}
	
	return returnValue;
}
