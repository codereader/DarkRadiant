#ifndef _SYS_OBJECT_H_
#define _SYS_OBJECT_H_

#include "itextstream.h"
#include <boost/python.hpp>

namespace script {

class SysObject
{
public:
	void print(const std::string& str) {
		globalOutputStream() << str << std::endl;
	}
};
typedef boost::python::class_<SysObject> SysObjectClass;

} // namespace script

#endif /* _SYS_OBJECT_H_ */
