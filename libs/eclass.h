/**
 * \file
 * Helper functions for dealing with IEntityClass and related objects.
 */
#pragma once

#include "ieclass.h"

#include <vector>

#include <functional>
#include "string/predicate.h"

namespace eclass
{

typedef std::vector<EntityClassAttribute> AttributeList;

namespace detail
{
    class AttributeSuffixComparator
    {
        // Starting position to convert to a number
        std::size_t _startPos;

    public:

        /// Constructor. Initialise the start position.
        AttributeSuffixComparator(std::size_t startPos)
        : _startPos(startPos)
        { }

        bool operator() (const EntityClassAttribute& x,
                         const EntityClassAttribute& y) const
        {
            // Get both substrings. An empty suffix comes first.
            std::string sx = x.getName().substr(_startPos);
            std::string sy = y.getName().substr(_startPos);
            if (sx.empty())
                return true;
            else if (sy.empty())
                return false;

            // Try numeric sort first, then fall back to lexicographic if the
            // prefixes are not integers.
            try
            {
                int ix = std::stoi(sx);
                int iy = std::stoi(sy);

                // Perform the comparison and return
                return ix < iy;
            }
            catch (std::logic_error&)
            {
                // greebo: Non-numeric operands, use ordinary string comparison
                return sx < sy;
            }
        }
    };
}

/**
 * \brief
 * Return a list of all class spawnargs matching the given prefix.
 *
 * The list is sorted by the numeric or lexicographic ordering of the suffixes.
 * This ensures that "editor_usage1", "editor_usage2" etc are returned in the
 * correct order.
 *
 * \param eclass
 * Entity class object to search
 *
 * \param prefix
 * String prefix for the spawnargs of interest
 *
 * \param includeInherited
 * Whether to include class spawnargs inherited from the parent class. Defaults
 * to true.
 */
inline AttributeList getSpawnargsWithPrefix(const IEntityClassPtr& eclass,
                                            const std::string& prefix,
                                            bool includeInherited = true)
{
    // Populate the list with with matching attributes
    AttributeList matches;
    eclass->forEachAttribute(
        [&](const EntityClassAttribute& a, bool inherited) {
            if (string::istarts_with(a.getName(), prefix) &&
                (includeInherited || !inherited))
            {
                matches.push_back(a);
            }
        },
        true // include editor_keys
    );

    // Sort the list in suffix order before returning
    detail::AttributeSuffixComparator comp(prefix.length());
    std::sort(matches.begin(), matches.end(), comp);

    return matches;
}

/**
 * \brief
 * Get the usage text for an entity class.
 *
 * The usage text consists of the values of all "editor_usage" spawnargs
 * concatenated in order.
 */
inline std::string getUsage(const IEntityClassPtr& entityClass)
{
    // Find all relevant spawnargs in order
    AttributeList usageAttrs = getSpawnargsWithPrefix(
        entityClass, "editor_usage", false
    );

    // Build the string
    std::ostringstream usage;
    bool firstLine = true;
    for (const EntityClassAttribute& a : usageAttrs)
    {
        if (firstLine)
        {
            usage << a.getValue();
            firstLine = false;
        }
        else
        {
            usage << '\n' << a.getValue();
        }
    }
    return usage.str();
}

}
