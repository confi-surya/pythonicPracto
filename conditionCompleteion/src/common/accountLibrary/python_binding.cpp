#include "boost/python.hpp"
#include "indexing_suite/value_traits.hpp"
#include "indexing_suite/container_suite.hpp"
#include "indexing_suite/list.hpp"
#include "accountLibrary/account_library.h"

using namespace boost::python;

namespace bp = boost::python;

namespace boost { namespace python { namespace indexing {

template<>
struct value_traits< recordStructure::ContainerRecord >{

    static bool const equality_comparable = false;


    static bool const less_than_comparable = false;


    template<typename PythonClass, typename Policy>
    static void visit_container_class(PythonClass &, Policy const &){

    }

};

}/*indexing*/ } /*python*/ } /*boost*/


BOOST_PYTHON_MODULE(libaccountLib){
    bp::class_< std::list< recordStructure::ContainerRecord > >("list_less__recordStructure_scope_ContainerRecord__greater_")
        .def(bp::indexing::list_suite< std::list< recordStructure::ContainerRecord > >());

bp::class_< accountInfoFile::AccountLibraryImpl >( "AccountLibraryImpl", bp::init< >() )
	.def("event_wait", &::accountInfoFile::AccountLibraryImpl::event_wait)
	.def("event_wait_stop", &::accountInfoFile::AccountLibraryImpl::event_wait_stop)
        .def("add_container", &::accountInfoFile::AccountLibraryImpl::add_container)
	.def("create_account", &::accountInfoFile::AccountLibraryImpl::create_account)
	.def("delete_account", &::accountInfoFile::AccountLibraryImpl::delete_account)
	.def("get_account_stat", &::accountInfoFile::AccountLibraryImpl::get_account_stat)
	.def("list_container", &::accountInfoFile::AccountLibraryImpl::list_container)
	.def("set_account_stat", &::accountInfoFile::AccountLibraryImpl::set_account_stat)
	.def("delete_container", &::accountInfoFile::AccountLibraryImpl::delete_container)
	.def("update_container", &::accountInfoFile::AccountLibraryImpl::update_container);


bp::class_< recordStructure::AccountStat >("AccountStat", bp::init< >())
	.def( bp::init< std::string, std::string, std::string, std::string, uint64_t, uint64_t, uint64_t, std::string, std::string, std::string, std::string, boost::python::dict >())
	.def("get_account", &::recordStructure::AccountStat::get_account)
	.def("get_created_at", &::recordStructure::AccountStat::get_created_at)
	.def("get_put_timestamp", &::recordStructure::AccountStat::get_put_timestamp)
	.def("get_delete_timestamp", &::recordStructure::AccountStat::get_delete_timestamp)
	.def("get_container_count", &::recordStructure::AccountStat::get_container_count)
	.def("get_object_count", &::recordStructure::AccountStat::get_object_count)
	.def("get_bytes_used", &::recordStructure::AccountStat::get_bytes_used)
	.def("get_hash", &::recordStructure::AccountStat::get_hash)
	.def("get_id", &::recordStructure::AccountStat::get_id)
	.def("get_status", &::recordStructure::AccountStat::get_status)
	.def("get_status_changed_at", &::recordStructure::AccountStat::get_status_changed_at)
	.def("get_metadata", &::recordStructure::AccountStat::get_metadata);


bp::class_< recordStructure::ContainerRecord >("ContainerRecord", bp::init< >())
	.def( bp::init< uint64_t, std::string, std::string, std::string, std::string, uint64_t, uint64_t, bool > ())
	.def("get_ROWID", &::recordStructure::ContainerRecord::get_ROWID)
	.def("get_name", &::recordStructure::ContainerRecord::get_name)
	.def("get_hash", &::recordStructure::ContainerRecord::get_hash)
	.def("get_put_timestamp", &::recordStructure::ContainerRecord::get_put_timestamp)
	.def("get_delete_timestamp", &::recordStructure::ContainerRecord::get_delete_timestamp)
	.def("get_object_count", &::recordStructure::ContainerRecord::get_object_count)
	.def("get_bytes_used", &::recordStructure::ContainerRecord::get_bytes_used)
	.def("get_deleted", &::recordStructure::ContainerRecord::get_deleted)
	.def("set_name", &::recordStructure::ContainerRecord::set_name);


    {//::StatusAndResult::Status
       typedef bp::class_< StatusAndResult::Status > Status_exposer_t;
       Status_exposer_t Status_exposer = Status_exposer_t( "Status", bp::init< >());
       bp::scope Status_scope( Status_exposer );
       Status_exposer.def( bp::init< uint16_t >((bp::arg("return_status"))));
       {//::StatusAndResult::Status::get_return_status
          typedef ::uint16_t ( ::StatusAndResult::Status::*get_return_status_function_type )(  ) ;
          Status_exposer.def(
              "get_return_status"
              , get_return_status_function_type( &::StatusAndResult::Status::get_return_status ) );
       }
    }

    {//::StatusAndResult::AccountStatWithStatus      
         typedef bp::class_< StatusAndResult::AccountStatWithStatus, bp::bases<StatusAndResult::Status > > AccountStatWithStatus_exposer_t;//("AccountStatWithStatus", bp::init< >());
         AccountStatWithStatus_exposer_t AccountStatWithStatus_exposer = AccountStatWithStatus_exposer_t( "AccountStatWithStatus", bp::init< >());
         bp::scope AccountStatWithStatus_scope( AccountStatWithStatus_exposer );
         AccountStatWithStatus_exposer.def( bp::init< uint16_t, recordStructure::AccountStat >((bp::arg("return_status"), bp::arg("account_stat"))));
         {//::StatusAndResult::Status::get_account_stat
            typedef ::recordStructure::AccountStat ( ::StatusAndResult::AccountStatWithStatus::*get_account_stat_function_type )(  ) ;
            AccountStatWithStatus_exposer.def(
                 "get_account_stat"
                 , get_account_stat_function_type(  &::StatusAndResult::AccountStatWithStatus::get_account_stat ) );
         }
    }

{//::StatusAndResult::ListContainerWithStatus
       typedef bp::class_< StatusAndResult::ListContainerWithStatus, bp::bases<StatusAndResult::Status > > ListContainerWithStatus_exposer_t;
       ListContainerWithStatus_exposer_t ListContainerWithStatus_exposer = ListContainerWithStatus_exposer_t( "ListContainerWithStatus", bp::init< >());
       bp::scope ListContainerWithStatus_scope( ListContainerWithStatus_exposer );
       ListContainerWithStatus_exposer.def( bp::init< uint16_t, std::list<recordStructure::ContainerRecord> >((bp::arg("return_status"), bp::arg("container_record"))));
       {//::StatusAndResult::ListContainerWithStatus::get_container_record

          typedef ::std::list<recordStructure::ContainerRecord> ( ::StatusAndResult::ListContainerWithStatus::*get_container_record_function_type )(  ) ;

          ListContainerWithStatus_exposer.def(
                  "get_container_record"
                  ,get_container_record_function_type( &::StatusAndResult::ListContainerWithStatus::get_container_record ) );
       }
    }

}
