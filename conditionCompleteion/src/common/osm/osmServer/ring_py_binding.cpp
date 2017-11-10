#include "osm/osmServer/ring_file.h"
#include <boost/python.hpp>

#include "indexing_suite/value_traits.hpp"

#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

#include "indexing_suite/container_suite.hpp"

#include "indexing_suite/list.hpp"

using namespace boost::python;

namespace bp = boost::python;

BOOST_PYTHON_MODULE(libOsmService) {
	class_< std::vector<std::string> >("list_ring_file")
	.def( vector_indexing_suite< std::vector<std::string> >() );

	class_<ring_file_manager::RingFileReadHandler>("RingFileReadHandler", init<std::string>())
		.def("get_fs_list", &ring_file_manager::RingFileReadHandler::get_fs_list)
		.def("get_account_level_dir_list", &ring_file_manager::RingFileReadHandler::get_account_level_dir_list)
		.def("get_container_level_dir_list", &ring_file_manager::RingFileReadHandler::get_container_level_dir_list)
		.def("get_object_level_dir_list", &ring_file_manager::RingFileReadHandler::get_object_level_dir_list)

	;
}
