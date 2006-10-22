#include "XMLFilter.h"

#include <iostream>

namespace filters {

// Test visibility of a texture against all rules

bool XMLFilter::isTextureVisible(const std::string& texture) const {
	std::cout << "Filter " << _name << " testing texture " << texture << std::endl;
	return true;	
}	
	

	
} // namespace filters
