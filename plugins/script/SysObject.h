#ifndef _SYS_OBJECT_H_
#define _SYS_OBJECT_H_

#include "itextstream.h"
#include <boost/python.hpp>

namespace script {

class SysObject
{
public:
	void print() {
		globalOutputStream() << "Boost::Python is working!" << std::endl;
	}
};

/*BOOST_PYTHON_MODULE(darkradiant)
{
	boost::python::class_<SysObject>("Sys")
        .def("print", &SysObject::print)
    ;
}*/

} // namespace script

#endif /* _SYS_OBJECT_H_ */
