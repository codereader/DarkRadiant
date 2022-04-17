#include "ResponseEffect.h"

#include "i18n.h"
#include "string/convert.h"
#include "string/replace.h"
#include <regex>

ResponseEffect::ResponseEffect() :
	_state(true),
	_origState(_state),
	_eclass(IEntityClassPtr()),
	_argumentListBuilt(false),
	_inherited(false)
{}

// Copy constructor
ResponseEffect::ResponseEffect(const ResponseEffect& other) :
	_effectName(other._effectName),
	_origName(other._origName),
	_state(other._state),
	_origState(other._origState),
	_args(other._args),
	_eclass(other._eclass),
	_argumentListBuilt(other._argumentListBuilt),
	_inherited(other._inherited)
{}

std::string ResponseEffect::getName() const {
	return _effectName;
}

bool ResponseEffect::nameIsOverridden() {
	return (_inherited && _effectName != _origName);
}

void ResponseEffect::setInherited(bool inherited) {
	_inherited = inherited;
}

bool ResponseEffect::isInherited() const {
	return _inherited;
}

bool ResponseEffect::isActive() const {
	return _state;
}

void ResponseEffect::setActive(bool active, bool inherited) {
	if (_inherited && !inherited) {
		// This is an override action, just set the _state, not _origState
		_state = active;
	}
	else {
		// Ordinary write operation, set both state and origState
		_state = active;
		_origState = _state;
	}
}

bool ResponseEffect::activeIsOverridden() {
	return (_inherited && _state != _origState);
}

void ResponseEffect::setName(const std::string& name, bool inherited) {
	if (_inherited && !inherited) {
		// This is an override action
		_effectName = name;
	}
	else {
		// Ordinary write operation, save both values
		_effectName = name;
		_origName = name;
	}

	// Update the entityclass pointer
	_eclass = ResponseEffectTypes::Instance().getEClassForName(_effectName);

	// Build the argument list, if it hasn't been built up till now
	if (!_argumentListBuilt) {
		_argumentListBuilt = true;
		buildArgumentList();
	}
}

std::string ResponseEffect::getArgument(unsigned int index) const {
	ArgumentList::const_iterator i = _args.find(index);

	// Return "" if the argument was not found
	return (i != _args.end()) ? i->second.value : "";
}

bool ResponseEffect::argIsOverridden(unsigned int index) {
	ArgumentList::const_iterator i = _args.find(index);

	if (i != _args.end()) {
		return (i->second.value != i->second.origValue);
	}
	return false;
}

void ResponseEffect::setArgument(unsigned int index, const std::string& value, bool inherited) {
	ArgumentList::const_iterator i = _args.find(index);

	if (_inherited && !inherited) {
		// This is an override operation
		if (i != _args.end()) {
			// Value exists, write to the <value> member
			_args[index].value = value;
		}
		else {
			// Value doesn't exist yet, initialise and write an empty "backup" value
			// This indicates that the value was not set in the inheritance tree
			Argument newArgument;
			newArgument.value = value;
			newArgument.origValue = "";

			// Store the argument in the map
			_args[index] = newArgument;
		}
	}
	else {
		if (i != _args.end()) {
			// Argument exists
			_args[index].value = value;
			_args[index].origValue = value;
		}
		else {
			// Argument doesn't exist, create it
			Argument newArgument;
			newArgument.value = value;
			newArgument.origValue = value;

			// Store the argument in the map
			_args[index] = newArgument;
		}
	}
}

std::string ResponseEffect::getCaption() const {
	return (_eclass != NULL)
		   ? _eclass->getAttributeValue("editor_caption")
		   : "";
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
		std::string argType = _eclass->getAttributeValue("editor_argType" + string::to_string(i));
		std::string argDesc = _eclass->getAttributeValue("editor_argDesc" + string::to_string(i));
		std::string argTitle = _eclass->getAttributeValue("editor_argTitle" + string::to_string(i));
		std::string optional = _eclass->getAttributeValue("editor_argOptional" + string::to_string(i));

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

std::string ResponseEffect::removeMarkup(const std::string& input)
{
	std::regex expr("(<[A-Za-z]+>)|(</[A-Za-z]+>)");
	return std::regex_replace(input, expr, "");
}

std::string ResponseEffect::getArgumentStr()
{
	if (_eclass == NULL) return _("Error: eclass pointer invalid.");

	std::string returnValue = _eclass->getAttributeValue("editor_argString");
	returnValue = removeMarkup(returnValue);

	for (ArgumentList::iterator i = _args.begin(); i != _args.end(); i++) {
		std::string needle = "[arg" + string::to_string(i->first) + "]";
		std::string replacement = i->second.value;

		// Check for a bool
		if (i->second.type == "b") {
			replacement = (i->second.value.empty()) ? _("no") : _("yes");
		}

		string::replace_all(returnValue, needle, replacement);
	}

	return returnValue;
}
