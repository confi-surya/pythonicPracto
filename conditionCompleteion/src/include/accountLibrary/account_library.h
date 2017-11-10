#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include<boost/asio/io_service.hpp>
#include<queue>

//#include<list>
#include "status.h"
#include "accountLibrary/account_info_file_manager.h"
#include "accountLibrary/record_structure.h"
#include "libraryUtils/object_storage_exception.h"
#include "libraryUtils/object_storage_log_setup.h"

using namespace OSDException;
using namespace StatusAndResult;

namespace accountInfoFile
{

class AccountLibrary
{
public:
	virtual void add_container(
			int64_t event_id,
			Status & result,
			std::string path,
			boost::shared_ptr<recordStructure::ContainerRecord> container_record) = 0;

	virtual void list_container(
			int64_t event_id,
			ListContainerWithStatus & result,
			std::string path,
			uint64_t count = 0,
			std::string marker = "",
			std::string end_marker = "",
			std::string prefix = "",
			std::string delimiter = "") = 0;

	virtual void update_container(
			int64_t event_id,
			Status & result,
			std::string path,
			std::list<recordStructure::ContainerRecord> container_records) = 0;

	virtual void delete_container(
			int64_t event_id,
			Status & result,
			std::string path,
			boost::shared_ptr<recordStructure::ContainerRecord> container_record) = 0;
	
	virtual void get_account_stat(
			int64_t event_id,
			AccountStatWithStatus & result,
			std::string const & temp_path,
			std::string const & path) = 0;

	virtual void set_account_stat(
			int64_t event_id,
			Status & result,
			std::string const & path,
			boost::shared_ptr<recordStructure::AccountStat>  account_stat) = 0;

	virtual void create_account(
			int64_t event_id,
			Status & result,
			std::string const & temp_path,
			std::string const & path,
			boost::shared_ptr<recordStructure::AccountStat>  account_stat) = 0;

	virtual void delete_account(
			int64_t event_id,
			Status & result,
			std::string const & temp_path,
			std::string const & path) = 0;

	virtual int64_t event_wait() = 0;
	virtual void event_wait_stop() = 0;
	virtual ~AccountLibrary() = 0;
};


class AccountLibraryImpl : public AccountLibrary
{

public:
	AccountLibraryImpl();
	AccountLibraryImpl(const AccountLibraryImpl &);


        virtual void add_container(
			int64_t event_id,
			Status & result,
                        std::string path,
                        boost::shared_ptr<recordStructure::ContainerRecord> container_record);

        virtual void list_container(
			int64_t event_id,
			ListContainerWithStatus & result,
                        std::string path,
                        uint64_t count = 0,
                        std::string marker = "",
                        std::string end_marker = "",
                        std::string prefix = "",
                        std::string delimiter = "");

        virtual void update_container(
			int64_t event_id,
			Status & result,
                        std::string path,
                        std::list<recordStructure::ContainerRecord> container_records); 

        virtual void delete_container(
			int64_t event_id,
			Status & result,
                        std::string path,
                        boost::shared_ptr<recordStructure::ContainerRecord> container_record);

        virtual void get_account_stat(
			int64_t event_id,
			AccountStatWithStatus & result,
			std::string const & temp_path,
                        std::string const & path);

        virtual void set_account_stat(
			int64_t event_id,
			Status & result,
                        std::string const & path,
                        boost::shared_ptr<recordStructure::AccountStat>  account_stat);

        virtual void create_account(
			int64_t event_id,
			Status & result,
                        std::string const & temp_path,
                        std::string const & path,
                        boost::shared_ptr<recordStructure::AccountStat>  account_stat);

        virtual void delete_account(
			int64_t event_id,
			Status & result,
			std::string const & temp_path,
                        std::string const & path);

	int64_t event_wait();
	void event_wait_stop();
	~AccountLibraryImpl();

private:
	boost::shared_ptr<AccountInfoFileManager> acc_file_manager;
	std::queue<int64_t> waiting_events;
	boost::asio::io_service io_service_obj;
	boost::shared_ptr<boost::asio::io_service::work> work;
	boost::thread_group thread_pool;
	boost::mutex mutex;
	boost::condition condition;
	bool stop_requested;

	void finish_event(int64_t id);
	virtual void add_container_internal(
			int64_t event_id,
			Status & result,
                        std::string path,
                        boost::shared_ptr<recordStructure::ContainerRecord> container_record);

        virtual void list_container_internal(
			int64_t event_id,
			ListContainerWithStatus & result,
                        std::string path,
                        uint64_t count = 0,
                        std::string marker = "",
                        std::string end_marker = "",
                        std::string prefix = "",
                        std::string delimiter = "");

        virtual void update_container_internal(
			int64_t event_id,
			Status & result,
                        std::string path,
                        std::list<recordStructure::ContainerRecord> container_records);

        virtual void delete_container_internal(
			int64_t event_id,
			Status & result,
                        std::string path,
                        boost::shared_ptr<recordStructure::ContainerRecord> container_record);

        virtual void get_account_stat_internal(
			int64_t event_id,
			AccountStatWithStatus & result,
			std::string const & temp_path,
                        std::string const & path);

	
        virtual void set_account_stat_internal(
			int64_t event_id,
			Status & result,
                        std::string const & path,
                        boost::shared_ptr<recordStructure::AccountStat>  account_stat);

        virtual void create_account_internal(
			int64_t event_id,
			Status & result,
                        std::string const & temp_path,
                        std::string const & path,
                        boost::shared_ptr<recordStructure::AccountStat>  account_stat);

        virtual void delete_account_internal(
			int64_t event_id,
			Status & result,
			std::string const & temp_path,
                        std::string const & path);

	
};

}

