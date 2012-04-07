/**
 * \file
 * Helper functions for dealing with IEntityClass and related objects.
 */
#pragma once

#include "ieclass.h"

#include <vector>

#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/predicate.hpp>

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
                int ix = boost::lexical_cast<int>(sx);
                int iy = boost::lexical_cast<int>(sy);

                // Perform the comparison and return
                return ix < iy;
            }
            catch (boost::bad_lexical_cast&)
            {
                // greebo: Non-numeric operands, use ordinary string comparison
                return sx < sy;
            }
        }
    };

    inline void addIfMatches(AttributeList& list,
                             const EntityClassAttribute& attr,
                             const std::string& prefix,
                             bool includeInherited)
    {
        if (boost::algorithm::istarts_with(attr.getName(), prefix)
            && (includeInherited || !attr.inherited))
        {
            list.push_back(attr);
        }
    }
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
inline AttributeList getSpawnargsWithPrefix(const IEntityClass& eclass,
                                            const std::string& prefix,
                                            bool includeInherited = true)
{
    // Populate the list with with matching attributes
    AttributeList matches;
    eclass.forEachClassAttribute(
        boost::bind(&detail::addIfMatches,
                    boost::ref(matches), _1, prefix, includeInherited),
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
inline std::string getUsage(const IEntityClass& entityClass)
{
    // Find all relevant spawnargs in order
    AttributeList usageAttrs = getSpawnargsWithPrefix(
        entityClass, "editor_usage", false
    );

    // Build the string
    std::ostringstream usage;
    bool firstLine = true;
    BOOST_FOREACH(EntityClassAttribute a, usageAttrs)
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
