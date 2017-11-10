#include "containerLibrary/container_library.h"
#include "libraryUtils/record_structure.h"
#include <boost/python.hpp>

#include "indexing_suite/value_traits.hpp"

#include "indexing_suite/container_suite.hpp"

#include "indexing_suite/list.hpp"

#include "indexing_suite/pair.hpp"

using namespace boost::python;

namespace bp = boost::python;
namespace boost { namespace python { namespace indexing {

template<>
struct value_traits< recordStructure::ObjectRecord >
{

	static bool const equality_comparable = false;

	static bool const less_than_comparable = false;

	template<typename PythonClass, typename Policy>
	static void visit_container_class(PythonClass &, Policy const &){

}
};

}/*indexing*/ } /*python*/ } /*boost*/

BOOST_PYTHON_MODULE(libContainerLib) {
	class_< std::list< recordStructure::ObjectRecord > >("list_less__recordStructure_scope_ContainerRecord__greater_")
	.def( indexing::list_suite< std::list< recordStructure::ObjectRecord > >() );

	class_<std::list<int32_t> >("list_less__component_names__greater_")
        .def( indexing::list_suite<std::list<int32_t> >() );

	class_<container_library::LibraryImpl>("LibraryImpl", init<std::string, int>())
		.def("create_container", &container_library::LibraryImpl::create_container)
		.def("delete_container", &container_library::LibraryImpl::delete_container)
		.def("update_container", &container_library::LibraryImpl::update_container)
		.def("list_container", &container_library::LibraryImpl::list_container)
		.def("get_container_stat", &container_library::LibraryImpl::get_container_stat)
		.def("set_container_stat", &container_library::LibraryImpl::set_container_stat)
		.def("start_recovery", &container_library::LibraryImpl::start_recovery)
		.def("event_wait", &container_library::LibraryImpl::event_wait)
		.def("event_wait_stop", &container_library::LibraryImpl::event_wait_stop)
		.def("accept_container_data", &container_library::LibraryImpl::accept_container_data)
		.def("extract_container_data", &container_library::LibraryImpl::extract_container_data)
		.def("extract_cont_journal_data", &container_library::LibraryImpl::extract_cont_journal_data)
		.def("commit_recovered_data", &container_library::LibraryImpl::commit_recovered_data)
		.def("flush_all_data",&container_library::LibraryImpl::flush_all_data)
		.def("revert_back_cont_data",&container_library::LibraryImpl::revert_back_cont_data)
		.def("remove_transferred_records",&container_library::LibraryImpl::remove_transferred_records)
		.def("update_bulk_delete_records", &container_library::LibraryImpl::update_bulk_delete_records)
//		.def("get_container_stat_for_updater", &container_library::LibraryImpl::get_container_stat_for_updater)
//		.staticmethod("get_container_stat_for_updater")

	;
/*
	class_<recordStructure::ContainerStat>("ContainerStat", init<std::string,
			std::string, uint64_t, uint64_t, uint64_t, uint32_t, uint64_t, std::string,
			std::string, std::string, uint64_t, std::string>())
		.def("getAccountName", &recordStructure::ContainerStat::getAccountName)
	;
*/
	class_<recordStructure::ObjectRecord>("ObjectRecord", init<uint64_t, std::string,
			std::string , uint64_t, std::string, std::string, uint64_t, uint64_t>())
		.def(init<>())
		.def("get_row_id", &recordStructure::ObjectRecord::get_row_id)
		.def("get_name", &recordStructure::ObjectRecord::get_name)
		.def("set_name", &recordStructure::ObjectRecord::set_name)
		.def("get_deleted_flag", &recordStructure::ObjectRecord::get_deleted_flag)
		.def("get_creation_time", &recordStructure::ObjectRecord::get_creation_time)
		.def("get_size", &recordStructure::ObjectRecord::get_size)
		.def("get_old_size", &recordStructure::ObjectRecord::get_old_size)
//		.def("get_content_type", &recordStructure::ObjectRecord::get_content_type)
		.def("get_etag", &recordStructure::ObjectRecord::get_etag)
//		.def("set_content_type", &recordStructure::ObjectRecord::set_content_type)
		// Compacting getter and setter as a property. - Sanchit
                .add_property("content_type", &recordStructure::ObjectRecord::get_content_type, &recordStructure::ObjectRecord::set_content_type);

	;
        
	class_<recordStructure::FinalObjectRecord>("FinalObjectRecord", init<std::string, 
								recordStructure::ObjectRecord>())
		.def(init<>())
		.def(init<std::string, std::string, std::string, uint64_t, uint64_t, std::string, std::string, uint64_t>())
		.def("get_path",&recordStructure::FinalObjectRecord::get_path)
		.def("get_object_record",&recordStructure::FinalObjectRecord::get_object_record, return_value_policy<reference_existing_object>())
                .def_pickle(recordStructure::final_object_record_pickle_suite());


	class_<recordStructure::TransferObjectRecord>("TransferObjectRecord", init<recordStructure::FinalObjectRecord,uint64_t, uint64_t>())
		.def(init<>())
		.def("get_final_object_record",&recordStructure::TransferObjectRecord::get_final_object_record,return_value_policy<reference_existing_object> ())
                .def_pickle(recordStructure::transfer_object_record_pickle_suite());

	{ //::recordStructure::ContainerStat
	typedef bp::class_< recordStructure::ContainerStat > ContainerStat_exposer_t;
	ContainerStat_exposer_t ContainerStat_exposer = ContainerStat_exposer_t( "ContainerStat", bp::init< >() );
	bp::scope ContainerStat_scope( ContainerStat_exposer );
	ContainerStat_exposer.def( bp::init< std::string, std::string, std::string, std::string, std::string, int64_t, uint64_t, 
		std::string, std::string, std::string, std::string, boost::python::dict >
		(( bp::arg("account"), bp::arg("container"), bp::arg("created_at"), bp::arg("put_timestamp"), 
		bp::arg("delete_timestamp"), bp::arg("object_count"), bp::arg("bytes_used"), bp::arg("hash"), 
		bp::arg("id"), bp::arg("status"), bp::arg("status_changed_at"), bp::arg("metadata") )) );
	{ //::recordStructure::ContainerStat::get_account
	
	    typedef ::std::string ( ::recordStructure::ContainerStat::*getAccountName_function_type )(  ) ;
	    
	    ContainerStat_exposer.def( 
		"getAccountName"
		, getAccountName_function_type( &::recordStructure::ContainerStat::getAccountName ) );


	
	}//TODO asrivastava, added during merging      
	ContainerStat_exposer.def_readwrite( "account", &recordStructure::ContainerStat::account );
	ContainerStat_exposer.def_readwrite( "container", &recordStructure::ContainerStat::container );
	ContainerStat_exposer.def_readwrite( "created_at", &recordStructure::ContainerStat::created_at );
	ContainerStat_exposer.def_readwrite( "bytes_used", &recordStructure::ContainerStat::bytes_used );
	ContainerStat_exposer.def_readwrite( "created_at", &recordStructure::ContainerStat::created_at );
	ContainerStat_exposer.def_readwrite( "delete_timestamp", &recordStructure::ContainerStat::delete_timestamp );
	ContainerStat_exposer.def_readwrite( "hash", &recordStructure::ContainerStat::hash );
	ContainerStat_exposer.def_readwrite( "id", &recordStructure::ContainerStat::id );
	ContainerStat_exposer.add_property( "metadata", &recordStructure::ContainerStat::get_metadata, 
								&recordStructure::ContainerStat::set_metadata);
	ContainerStat_exposer.def_readwrite( "object_count", &recordStructure::ContainerStat::object_count );
	ContainerStat_exposer.def_readwrite( "put_timestamp", &recordStructure::ContainerStat::put_timestamp );
	ContainerStat_exposer.def_readwrite( "status", &recordStructure::ContainerStat::status );
	ContainerStat_exposer.def_readwrite( "status_changed_at", &recordStructure::ContainerStat::status_changed_at );
	//bp::register_ptr_to_python< boost::shared_ptr< recordStructure::ContainerStat > >();
	}
	{//::StatusAndResult::Status
		typedef bp::class_< StatusAndResult::Status > Status_exposer_t;
		Status_exposer_t Status_exposer = Status_exposer_t( "Status", bp::init< >());
		bp::scope Status_scope( Status_exposer );
		Status_exposer.def( bp::init< uint16_t >((bp::arg("return_status"))));
		{//::StatusAndResult::Status::get_return_status

		  typedef ::uint16_t ( ::StatusAndResult::Status::*get_return_status_function_type )(  ) ;

		  Status_exposer.def(
			"get_return_status",
			get_return_status_function_type( &::StatusAndResult::Status::get_return_status ) );
		}
	}

	{////::StatusAndResult::ContainerStatWithStatus      
	 typedef bp::class_< StatusAndResult::ContainerStatWithStatus, bp::bases<StatusAndResult::Status > > ContainerStatWithStatus_exposer_t;//("ContainerStatWithStatus", bp::init< >());
	ContainerStatWithStatus_exposer_t ContainerStatWithStatus_exposer = ContainerStatWithStatus_exposer_t( "ContainerStatWithStatus", bp::init< >());
	bp::scope ContainerStatWithStatus_scope( ContainerStatWithStatus_exposer );
	ContainerStatWithStatus_exposer.def_readwrite( "container_stat", &StatusAndResult::ContainerStatWithStatus::container_stat);
	ContainerStatWithStatus_exposer.def( bp::init< uint16_t, recordStructure::ContainerStat >((bp::arg("return_status"), bp::arg("container_stat"))));
	{//::StatusAndResult::ContainerStatWithStatus::get_container_stat

		typedef ::recordStructure::ContainerStat ( ::StatusAndResult::ContainerStatWithStatus::*get_container_stat_function_type )(  ) ;
		ContainerStatWithStatus_exposer.def(
		"get_container_stat",
		get_container_stat_function_type(  &::StatusAndResult::ContainerStatWithStatus::get_container_stat ) );
	}
	}


	{//::StatusAndResult::ListObjectWithStatus
		typedef bp::class_< StatusAndResult::ListObjectWithStatus, bp::bases<StatusAndResult::Status > > ListObjectWithStatus_exposer_t;
		ListObjectWithStatus_exposer_t ListObjectWithStatus_exposer = ListObjectWithStatus_exposer_t( "ListObjectWithStatus", bp::init< >());
		bp::scope ListObjectWithStatus_scope( ListObjectWithStatus_exposer );
		ListObjectWithStatus_exposer.def_readwrite( "object_record", &StatusAndResult::ListObjectWithStatus::object_record);
#if 0
	ListObjectWithStatus_exposer.def( bp::init< uint16_t, std::list<recordStructure::ObjectRecord> >((bp::arg("return_status"), bp::arg("object_record"))));
	{//::StatusAndResult::ListObjectWithStatus::get_object_record

		typedef ::std::list<recordStructure::ObjectRecord> ( ::StatusAndResult::ListObjectWithStatus::*get_object_record_function_type )(  ) ;

		ListObjectWithStatus_exposer.def(
		"get_object_record",
		get_object_record_function_type( &::StatusAndResult::ListObjectWithStatus::get_object_record ) );
	}
	#endif
	}


/* Private interface probably not requied to be exposed
	{ //::recordStructure::ContainerStat::get_size

	    typedef ::uint64_t ( ::recordStructure::ContainerStat::*get_size_function_type )(  ) ;
	    
	    ContainerStat_exposer.def( 
		"get_size"
		, get_size_function_type( &::recordStructure::ContainerStat::get_size ) );

	}
	{ //::recordStructure::ContainerStat::set_size

	    typedef ::std::string ( ::recordStructure::ContainerStat::*set_size_function_type )(  ) ;
	    
	    ContainerStat_exposer.def( 
		"set_size"
		, set_size_function_type( &::recordStructure::ContainerStat::set_size ) );

	}
	//TODO asrivastava, added during merging      
	ContainerStat_exposer.def_readwrite( "account", &recordStructure::ContainerStat::account );
	ContainerStat_exposer.def_readwrite( "container", &recordStructure::ContainerStat::container );
	ContainerStat_exposer.def_readwrite( "created_at", &recordStructure::ContainerStat::created_at );
	ContainerStat_exposer.def_readwrite( "bytes_used", &recordStructure::ContainerStat::bytes_used );
	ContainerStat_exposer.def_readwrite( "created_at", &recordStructure::ContainerStat::created_at );
	ContainerStat_exposer.def_readwrite( "delete_timestamp", &recordStructure::ContainerStat::delete_timestamp );
	ContainerStat_exposer.def_readwrite( "hash", &recordStructure::ContainerStat::hash );
	ContainerStat_exposer.def_readwrite( "id", &recordStructure::ContainerStat::id );
	ContainerStat_exposer.add_property( "metadata", &recordStructure::ContainerStat::get_metadata, 
								&recordStructure::ContainerStat::set_metadata);
	ContainerStat_exposer.def_readwrite( "object_count", &recordStructure::ContainerStat::object_count );
	ContainerStat_exposer.def_readwrite( "put_timestamp", &recordStructure::ContainerStat::put_timestamp );
	ContainerStat_exposer.def_readwrite( "status", &recordStructure::ContainerStat::status );
	ContainerStat_exposer.def_readwrite( "status_changed_at", &recordStructure::ContainerStat::status_changed_at );
	//bp::register_ptr_to_python< boost::shared_ptr< recordStructure::ContainerStat > >();
}

*/
}
