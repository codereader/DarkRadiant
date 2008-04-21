#ifndef SPECIFIER_H_
#define SPECIFIER_H_

#include "SpecifierType.h"
#include <vector>

namespace objectives
{

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

    /*
     * Get the specifier value.
     */
    const std::string& getValue() const {
        return _value;
    }
};

/**
 * Specifier list type.
 */
typedef std::vector<Specifier> SpecifierList;

}

#endif /* SPECIFIER_H_ */
