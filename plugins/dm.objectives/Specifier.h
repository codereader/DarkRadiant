#ifndef SPECIFIER_H_
#define SPECIFIER_H_

#include "SpecifierType.h"

#include <vector>
#include <ostream>

#include <boost/shared_ptr.hpp>

namespace objectives {

class Component;

/**
 * A Specifier is a simple data class which combines a SpecifierType with its
 * associated string value.
 */
class Specifier
{
    // The type
    SpecifierType _type;

    // The value
    std::string _value;

public:

    /**
     * Main constructor.
     */
    Specifier(
        const SpecifierType& type = SpecifierType::SPEC_NONE(),
        const std::string& value = ""
    )
    : _type(type), _value(value)
    { }

    /**
     * Get the SpecifierType.
     */
    const SpecifierType& getType() const {
        return _type;
    }

	/**
     * Set the SpecifierType to something new
     */
    void setType(const SpecifierType& type) {
        _type = type;
    }

    /**
     * Get the specifier value.
     */
    const std::string& getValue() const {
        return _value;
    }

	/**
	 * Set the specifier value to the given string.
	 */
	void setValue(const std::string& value) {
        _value = value;
    }

    /**
     * Get a user-friendly string representing the contents of this Specifier.
     */
    std::string toString() const {
        if (_type == SpecifierType::SPEC_NONE()) {
            return _type.getDisplayName();
        }
        else {
            return _type.getDisplayName() + " : " + _value;
        }
    }

	/**
	 * greebo: Returns a human-readable chunk of text based on the values
	 * found in the given Component object. This is used for the user-friendly
	 * display of components.
	 */
	std::string getSentence(Component& component);

public:

    /**
     * Specifier number enum.
     */
    enum SpecifierNumber {
        FIRST_SPECIFIER,
        SECOND_SPECIFIER,
        MAX_SPECIFIERS
    };

};

/**
 * Specifier pointer type.
 */
typedef boost::shared_ptr<Specifier> SpecifierPtr;

/**
 * Specifier list type.
 */
typedef std::vector<SpecifierPtr> SpecifierList;

/**
 * Stream insertion operator for Specifiers.
 */
inline std::ostream& operator<< (std::ostream& os, const Specifier& spec)
{
    os << "Specifier { type = " << spec.getType().getName()
       << ", value = " << spec.getValue() << " }";
    return os;
}

}

#endif /* SPECIFIER_H_ */
