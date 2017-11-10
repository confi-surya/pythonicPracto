#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <errno.h>
#include <boost/tuple/tuple.hpp>
#include "objectLibrary/object_library.h"
#include "libraryUtils/object_storage_log_setup.h"
#include "communication/communication.h"
#include "communication/message.h"
#include "communication/message_interface_handlers.h"

namespace object_library
{

boost::mutex ObjectLibrary::mutex;

ObjectLibrary::ObjectLibrary(uint32_t num_writer, uint32_t num_reader)
	: writer_io_service_ptr_(new boost::asio::io_service()),
	  writer_io_service_obj(*writer_io_service_ptr_),
	  reader_io_service_ptr_(new boost::asio::io_service()),
	  reader_io_service_obj(*reader_io_service_ptr_)
{
	OSDLOG(DEBUG, "Constructing Object Library");
	this->writer_thread_pool.reset(new boost::thread_group());
	this->writer_work.reset(new boost::asio::io_service::work(this->writer_io_service_obj));
	for (uint32_t i = 0; i < num_writer; i ++)
	{
		this->writer_thread_pool->create_thread(boost::bind(&boost::asio::io_service::run,
				&this->writer_io_service_obj));
	}

	this->reader_thread_pool.reset(new boost::thread_group());
	this->reader_work.reset(new boost::asio::io_service::work(this->reader_io_service_obj));
	for (uint32_t i = 0; i < num_reader; i ++)
	{
		this->reader_thread_pool->create_thread(boost::bind(&boost::asio::io_service::run,
				&this->reader_io_service_obj));
	}
	OSDLOG(INFO, "Object Library constructed");
}

ObjectLibrary::~ObjectLibrary()
{
	this->writer_work.reset();
	this->writer_thread_pool->join_all();
	this->reader_work.reset();
	this->reader_thread_pool->join_all();
	OSDLOG(INFO, "Object Library destructed");
}

void ObjectLibrary::write_chunk(int32_t write_fd, char * chunk, int32_t len, int32_t poll_fd)
{
	this->writer_io_service_obj.post(boost::bind(&ObjectLibrary::write_chunk_internal,
			this, write_fd, chunk, len, poll_fd));
}

void ObjectLibrary::write_chunk_internal(int32_t write_fd, char * chunk, int32_t len,
		int32_t poll_fd)
{
	int32_t ret = -1;
	if (this->update_md5_hash(write_fd, chunk, len))
	{
		ret = ::write(write_fd, chunk, len);
                if (ret != len){
			OSDLOG(ERROR, "Failed to write data. Error : " << strerror(errno));
		}
	}
	::write(poll_fd, &ret, sizeof(ret));
}

void ObjectLibrary::read_chunk(int32_t read_fd, char * chunk, int32_t len, int32_t poll_fd)
{
	this->reader_io_service_obj.post(boost::bind(&ObjectLibrary::read_chunk_internal,
			this, read_fd, chunk, len, poll_fd));
}

void ObjectLibrary::read_chunk_internal(int32_t read_fd, char * chunk, int32_t len,
		int32_t poll_fd)
{
	int32_t ret = ::read(read_fd, chunk, len);
	if (ret != len){
		OSDLOG(ERROR, "Failed to read data. Error : " << strerror(errno));
	}
	this->update_md5_hash(read_fd, chunk, ret);
	::write(poll_fd, &ret, sizeof(ret));
}

void ObjectLibrary::init_md5_hash(int32_t fd)
{
	boost::mutex::scoped_lock lock(this->mutex);
	std::map<int32_t, MD5_CTX>::iterator iter = this->md5_hash_map.find(fd);
	if (iter == this->md5_hash_map.end())
	{
		std::pair<std::map<int32_t, MD5_CTX>::iterator, bool> res =
				this->md5_hash_map.insert(std::make_pair(fd, MD5_CTX()));
		iter = res.first;
	}
	else
	{
		OSDLOG(ERROR, "Hash already exists, overwriting the entry");
	}
	::MD5_Init(&iter->second);
}

bool ObjectLibrary::update_md5_hash(int32_t fd, char * chunk, int32_t len)
{
	boost::mutex::scoped_lock lock(this->mutex);
	std::map<int32_t, MD5_CTX>::iterator iter = this->md5_hash_map.find(fd);
	if (iter != this->md5_hash_map.end())
	{
		lock.unlock();
		::MD5_Update(&iter->second, chunk, len);
		return true;
	}
	else
	{
		OSDLOG(ERROR, "Entry does not exist in hash_map");
		return false;
	}
}

std::string ObjectLibrary::get_md5_hash(int32_t fd)
{
	boost::mutex::scoped_lock lock(this->mutex);
	unsigned char hash[MD5_DIGEST_LENGTH];
	std::map<int32_t, MD5_CTX>::iterator iter = this->md5_hash_map.find(fd);
	if (iter != this->md5_hash_map.end())
	{
		::MD5_Final(hash, &iter->second);
		std::ostringstream buf;
		buf << std::setfill('0') << std::hex;
		for (int32_t i = 0; i < MD5_DIGEST_LENGTH; i++)
		{
			buf << std::setw(2) << static_cast<uint32_t>(hash[i]);
		}
		this->md5_hash_map.erase(iter);
		return buf.str();
	}
	else
	{
		OSDLOG(ERROR, "Returing empty string");
		return "";
	}
}

// Support to delete the objects in background
void ObjectLibrary::process_bulk_delete(list<boost::tuple<int ,std::string> > obj_hash_list, std::map<string,boost::tuple<string,string> > ring_map,std::string host_info)
{
        this->writer_io_service_obj.post(boost::bind(&ObjectLibrary::process_bulk_delete_internal,
                        this, obj_hash_list, ring_map,host_info));
}

void ObjectLibrary::process_bulk_delete_internal(list<boost::tuple<int ,std::string> > obj_hash_list, std::map<string,boost::tuple<string,string> > ring_map,std::string host_info)
{

//if object list size if more than 1000(Actual number is TBD) .Request will not be entertained .
        if(obj_hash_list.size()>1000)
        {
                return  ;
        }

//Fetch host informmation 
	std::vector<std::string> host_data ;
        host_data.clear();
        boost::split(host_data, host_info,boost::is_any_of(":"));

//set up communication first .

	boost::shared_ptr<Communication::SynchronousCommunication> comm(
                                        	 new Communication::SynchronousCommunication(host_data[0],
                                                 atoi((host_data[1]).c_str())));			//host name and port number is required	
	if(!comm->is_connected())
	{
		
		OSDLOG(INFO, "ObjectLibrary::Connection to container service is failed .");
		return ;
	}

//delete objects 
        for(list<boost::tuple<int ,std::string> >::iterator it =obj_hash_list.begin();it!=obj_hash_list.end();it++)
          {
                std::vector<std::string> path_tokens ;
		path_tokens.clear();
		std::string file_system = "";
		std::string path_dir[2]; 
		boost::split(path_tokens, boost::get<1>(*it),boost::is_any_of("/"));             
		//path_tokens will contain account hash ,container hash	,object hash .
		
		for(int count=0;count<3;count++)
		{
			if(ring_map.find(path_tokens[count])!=ring_map.end())
			{
			 		
				path_dir[count] = boost::get<1>(ring_map[path_tokens[count]]);
				file_system = boost::get<0>(ring_map[path_tokens[count]]);			
			}
			else
			{
				//hash value is not there in the ring map .Exit .
				OSDLOG(INFO, "ObjectLibrary::process_bulk_delete_internal Hash value is not present in ring map .Object won't be deleted ");
				return ;
			}
		}
		//Object data file path 	
                                                                        
		std::string data_file_path = std::string(EXPORT_PATH) + file_system + path_dir[0] + path_tokens[0]
						+path_dir[1]+ path_tokens[1]+path_dir[2]+ std::string(DATADIR)
						+ path_tokens[2] + ".data";              		
		//Meta data path  
		std::string meta_file_path = std::string(EXPORT_PATH) + file_system +path_dir[0] + path_tokens[0]
						+path_dir[1]+ path_tokens[1]+path_dir[2]+ std::string(METADIR)
						+ path_tokens[2] + ".meta";

		//fetch the objects from list and delete them
                if(boost::filesystem::exists(boost::filesystem::path(data_file_path)))
                {
                        boost::filesystem::remove(boost::filesystem::path(data_file_path));
                }

		//send http request to container service
		OSDLOG(INFO, "ObjectLibrary::process_bulk_delete_internal Send container update request to container service ");

/*This block of code is commented because change are yet to done in communication library .

		// New Msg update_container needs to be  added in communication library .
                boost::shared_ptr<comm_messages::UpdateContainer> update_container(
                                                   	          new comm_messages::UpdateContainer(meta_file_path,"DELETE"));

		 MessageExternalInterface::sync_send_update_container(
                                                        update_container.get(),
                                                        comm,
                                                        host_data[0],
                                                        atoi((host_data[1]).c_str())
                                                        );
	
      */ 


/*This block of code is commented because change are yet to done in communication library .
        
                // New Msg update_container needs to be  added in communication library .
                boost::shared_ptr<comm_messages::ReleaseTransLock> release_lock(
                                                                  new comm_messages::ReleaseTransLock(boost::get<1>(*it),"UNLOCK"));
        
                 MessageExternalInterface::sync_send_update_container(
                                                        release_lock.get(),
                                                        comm,
                                                        host_data[0],
                                                        atoi((host_data[1]).c_str())
                                                        );
                
      */ 

	 }



        return ;
}


}


