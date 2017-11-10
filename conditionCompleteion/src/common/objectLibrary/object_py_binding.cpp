#include <boost/python.hpp>
#include "objectLibrary/object_library.h"

BOOST_PYTHON_MODULE(libObjectLib)
{
	using namespace boost::python;
	class_<object_library::ObjectLibrary>("ObjectLibrary", init<uint32_t, uint32_t>())
			.def("write_chunk", &object_library::ObjectLibrary::write_chunk)
			.def("read_chunk", &object_library::ObjectLibrary::read_chunk)
			.def("init_md5_hash", &object_library::ObjectLibrary::init_md5_hash)
			.def("get_md5_hash", &object_library::ObjectLibrary::get_md5_hash)
			;
}
