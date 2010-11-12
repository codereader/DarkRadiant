#ifndef LOGIC_H_
#define LOGIC_H_

#include <string>
#include <boost/shared_ptr.hpp>

namespace objectives {

/**
 * greebo: This is everything needed to define the logic
 * for succeeding a mission or objective. For now, this is
 * just the strings containing the boolean ANDs, ORs and NOTs.
 */
struct Logic
{
	std::string successLogic;
	std::string failureLogic;
};
typedef boost::shared_ptr<Logic> LogicPtr;

} // namespace objectives

#endif /* LOGIC_H_ */
