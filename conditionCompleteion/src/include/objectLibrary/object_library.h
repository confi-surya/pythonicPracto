#include <unistd.h>
#include <openssl/md5.h> 
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <string>
#include <map>
#include <list>
#include<cstdlib>

#define EXPORT_PATH  "/export"
#define DATADIR  "data"
#define METADIR  "meta"


using namespace std;
namespace object_library
{

class ObjectLibrary
{
public:
	ObjectLibrary(uint32_t num_writer, uint32_t num_reader);
	~ObjectLibrary();

	void write_chunk(int32_t write_fd, char * chunk, int32_t len, int32_t poll_fd);
	void read_chunk(int32_t read_fd, char * chunk, int32_t len, int32_t pool_fd);
	void init_md5_hash(int32_t write_fd);
	std::string get_md5_hash(int32_t write_fd);
	void process_bulk_delete(list<boost::tuple<int ,std::string> > obj_hash_list, std::map<string,boost::tuple<string,string> > ring_map,std::string host_info);

private:
	void write_chunk_internal(int32_t write_fd, char * chunk, int32_t len, int32_t poll_fd);
	void read_chunk_internal(int32_t read_fd, char * chunk, int32_t len, int32_t poll_fd);
	bool update_md5_hash(int32_t fd, char * chunk, int32_t len);
	void process_bulk_delete_internal(list<boost::tuple<int ,std::string> > obj_hash_list, std::map<string,boost::tuple<string,string> > ring_map,std::string host_info);
	
	boost::shared_ptr<boost::thread_group> writer_thread_pool;
	boost::shared_ptr<boost::asio::io_service> writer_io_service_ptr_; 
	boost::asio::io_service & writer_io_service_obj;
	boost::shared_ptr<boost::asio::io_service::work> writer_work;

	boost::shared_ptr<boost::thread_group> reader_thread_pool;
	boost::shared_ptr<boost::asio::io_service> reader_io_service_ptr_; 
	boost::asio::io_service & reader_io_service_obj;
	boost::shared_ptr<boost::asio::io_service::work> reader_work;

	std::map<int32_t, MD5_CTX> md5_hash_map;
	static boost::mutex mutex;
	bool bulk_delete_done;
};

			
} // namespace object_library

