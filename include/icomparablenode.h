#pragma once

#include "inode.h"

namespace scene
{

/**
 * Prototype of a comparable scene node, providing hash information
 * for comparison to another node. Nodes of the same type can be compared against each other.
 */
class IComparableNode :
    public virtual INode
{
public:
    virtual ~IComparableNode() {}

    // Returns the fingerprint (hash) of this node, to allow for quick 
    // matching against other nodes of the same type. Fingerprints of different
    // types are not comparable, be sure to check the node type first.
    virtual std::size_t getFingerprint() = 0;
};

// The number of digits that are considered when hashing floating point values in fingerprinting
constexpr std::size_t SignificantFingerprintDoubleDigits = 6;

}
