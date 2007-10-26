#ifndef ATTRIBUTESUFFIXCOMPARATOR_H_
#define ATTRIBUTESUFFIXCOMPARATOR_H_

#include "ieclass.h"

#include <boost/lexical_cast.hpp>

/**
 * Comparison functor for EntityClassAttributes. Orders attributes based on the
 * numerical ordering of their suffixes.
 */
class AttributeSuffixComparator
{
	// Starting position to convert to a number
	std::size_t _startPos;
	
public:
	
	/**
	 * Constructor. Initialise the start position.
	 */
	AttributeSuffixComparator(std::size_t startPos)
	: _startPos(startPos)
	{ }
	
	/**
	 * Functor operator.
	 */
	bool operator() (const EntityClassAttribute& x, 
					 const EntityClassAttribute& y) const 
	{
		// Get both substrings. An empty suffix comes first.
		std::string sx = x.name.substr(_startPos);
		std::string sy = y.name.substr(_startPos);
		if (sx.empty())
			return true;
		else if (sy.empty())
			return false;
		
		// Extract both integers and compare them. If one or other cannot be 
		// converted to an integer, return false anyway.
		try {
			int ix = boost::lexical_cast<int>(sx);
			int iy = boost::lexical_cast<int>(sy);

			// Perform the comparison and return
			return ix < iy;
		}
		catch (boost::bad_lexical_cast e) {
			std::cerr 
				<< "[eclassmgr] AttributeSuffixComparator: cannot compare '"
				<< x.name << "' with '" << y.name << "' at position " 
				<< _startPos << ": invalid integer." << std::endl;
			return false;
		}
		
	}
};

#endif /*ATTRIBUTESUFFIXCOMPARATOR_H_*/
