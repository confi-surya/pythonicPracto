#include "transactionLibrary/transaction_library.h"
#include "libraryUtils/helper.h"
#include <boost/python.hpp>

#include "indexing_suite/value_traits.hpp"

#include "indexing_suite/container_suite.hpp"

#include "indexing_suite/map.hpp"

#include "indexing_suite/list.hpp"


using namespace boost::python;

namespace bp = boost::python;

namespace boost { namespace python { namespace indexing {
//
template<>
struct value_traits< recordStructure::ActiveRecord >
{

	static bool const equality_comparable = false;

	static bool const less_than_comparable = false;

	template<typename PythonClass, typename Policy>
	static void visit_container_class(PythonClass &, Policy const &){

}

};
//

}/*indexing*/ } /*python*/ } /*boost*/

BOOST_PYTHON_MODULE(libTransactionLib)
{
	class_< std::map< int64_t, recordStructure::ActiveRecord > >("map_less__recordStructure_scope_ActiveRecord__greater_")
	.def( indexing::map_suite< std::map< int64_t, recordStructure::ActiveRecord > >() );

	class_<std::list<recordStructure::ActiveRecord > >("list_less__recordStructure_scope_ContainerRecord__greater_")
        .def(indexing::list_suite<std::list<recordStructure::ActiveRecord> >())
    ;


	StatusAndResult::Status<bool> (transaction_library::TransactionLibrary::*release_lock_by_id)(int64_t, int32_t) = &transaction_library::TransactionLibrary::release_lock;
	StatusAndResult::Status<bool> (transaction_library::TransactionLibrary::*release_lock_by_name)(std::string, int32_t) = &transaction_library::TransactionLibrary::release_lock;

	class_<transaction_library::TransactionLibrary>("TransactionLibrary", init<std::string, int, int>())
		.def("start_recovery", &transaction_library::TransactionLibrary::start_recovery)
		.def("add_transaction", &transaction_library::TransactionLibrary::add_transaction)
		.def("release_lock", release_lock_by_id)
		.def("release_lock", release_lock_by_name)
		.def("event_wait", &transaction_library::TransactionLibrary::event_wait)
		.def("event_wait_stop", &transaction_library::TransactionLibrary::event_wait_stop)
		.def("get_transaction_map", &transaction_library::TransactionLibrary::get_transaction_map)
		.def("accept_transaction_data", &transaction_library::TransactionLibrary::accept_transaction_data)
		.def("upgrade_accept_transaction_data", &transaction_library::TransactionLibrary::accept_transaction_data)
		.def("extract_transaction_data", &transaction_library::TransactionLibrary::extract_transaction_data)
		.def("extract_trans_journal_data", &transaction_library::TransactionLibrary::extract_trans_journal_data)
		.def("commit_recovered_data",&transaction_library::TransactionLibrary::commit_recovered_data)
		.def("revert_back_trans_data",&transaction_library::TransactionLibrary::revert_back_trans_data)
		.def("add_bulk_delete_transaction", &transaction_library::TransactionLibrary::add_bulk_delete_transaction)
		.def("recover_active_record", &transaction_library::TransactionLibrary::recover_active_record)

	;

	class_<helper_utils::Request>("Request", init<std::string, std::string, int, std::string>())
	;
	//template <>
	class_<StatusAndResult::Status<bool> >("Status", no_init)
		//.def(init< bool >())
		.def(init<bool, std::string>())
		//.def(init<bool, int64_t>())
		.def("get_return_status", &StatusAndResult::Status<bool>::get_return_status)
		.def("get_failure_reason", &StatusAndResult::Status<bool>::get_failure_reason)
		.def("get_error_code", &StatusAndResult::Status<bool>::get_error_code)
		.def("get_value", &StatusAndResult::Status<bool>::get_value)

	;
	class_<StatusAndResult::Status<int64_t> >("Status", no_init)
		.def("get_error_code", &StatusAndResult::Status<int64_t>::get_error_code)
		.def("get_failure_reason", &StatusAndResult::Status<int64_t>::get_failure_reason)
		.def("get_return_status", &StatusAndResult::Status<int64_t>::get_return_status)
		.def("get_value", &StatusAndResult::Status<int64_t>::get_value)

	;

	class_<recordStructure::ActiveRecord>("ActiveRecord", init<int, std::string, std::string, std::string, int64_t>())
		.def(init<>())
		.def("get_object_id", &recordStructure::ActiveRecord::get_object_id)
		.def("get_transaction_id", &recordStructure::ActiveRecord::get_transaction_id)
		.def("get_operation_type", &recordStructure::ActiveRecord::get_operation_type)
		.def("get_request_method", &recordStructure::ActiveRecord::get_request_method)
		.def("get_time", &recordStructure::ActiveRecord::get_time)
                .def("set_transaction_id", &recordStructure::ActiveRecord::set_transaction_id)
		.def("get_object_name", &recordStructure::ActiveRecord::get_object_name)
                .def_pickle(recordStructure::active_object_record_pickle_suite());

	;
}
