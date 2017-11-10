#ifndef __RECORD_STRUCTURE_H_142__
#define __RECORD_STRUCTURE_H_142__

#include "boost/date_time/posix_time/posix_time.hpp"

#include <boost/shared_ptr.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/extract.hpp>
#include <string>
#include <tr1/tuple>
#include <vector>

#include "cool/cool.h"
#include "libraryUtils/helper.h"

#include <boost/python/module.hpp>
#include <boost/python/def.hpp>
#include <boost/python/class.hpp>
#include <boost/python/tuple.hpp>
#include <boost/python/extract.hpp>
#include "libraryUtils/object_storage_log_setup.h"
#include<sstream>

namespace recordStructure
{
typedef cool::ObjectIoStream<> DefaultIOStream;
enum JournalType
{
	INVALID = 0,
	OPEN_RECORD = 11,
	CLOSE_RECORD = 12,
	CHECKPOINT_RECORD = 13,
	SNAPSHOT_HEADER_RECORD = 14,
	CONTAINER_STAT = 15,
	OBJECT_RECORD = 16,
	SNAPSHOT_CONTAINER_JOURNAL = 1,
	CHECKPOINT_CONTAINER_JOURNAL = 2,
	CONTAINER_START_FLUSH = 3,
	CONTAINER_END_FLUSH = 4,
	ACCEPT_COMPONENTS_MARKER = 5,
	FINAL_OBJECT_RECORD = 21,
	NODE_INFO_FILE_RECORD = 22,
        RING_RECORD = 23,
        COMPONENT_INFO_RECORD = 24,
	CONTAINER_TRANSIENT_RECORD = 25,
	ACCOUNT_TRANSIENT_RECORD = 26,
	UPDATER_TRANSIENT_RECORD = 27,
	OBJECT_TRANSIENT_RECORD = 28,
	GL_SNAPSHOT_RECORD = 29,
	SERVICE_COMPONENT_RECORD = 30,
	GL_RECOVERY_RECORD = 31,
	GL_CHECKPOINT_RECORD = 32,
	TRANSFER_OBJECT_RECORD = 33,
	TRANSIENT_MAP = 34,
	COMPLETE_COMPONENT_RECORD = 35,
	ELECTION_RECORD = 36,
	GL_MOUNT_INFO_RECORD = 37
};

class Record
{
	public:
		inline int64_t get_serialized_size();
		inline int64_t xserialize_type(cool::ObjectIoStream<>& stream);
		inline void serialize_record(char *buffer, int64_t size);
		//inline Record * deserialize_record(char *buffer, uint32_t size);

		virtual int get_type() = 0;
		virtual ~Record() {}
		virtual void xserialize_old(cool::ObjectIoStream<>& stream){OSDLOG(INFO,"THIS SHOULD NOT BE CALLED");}
		virtual void xserialize(cool::ObjectIoStream<>& stream) = 0;
		virtual int64_t get_journal_version() const { return 1;}
};

Record * deserialize_record(char *buffer, int64_t size);

int64_t Record::xserialize_type(cool::ObjectIoStream<>& stream)
{
	int type;
	if (stream.isInputStream())
	{
	       stream.readWrite(type);
	}
	else
	{
	       type = this->get_type();
	       stream.readWrite(type);
	}
	return type;
}

int64_t Record::get_serialized_size()
{
	cool::CountingOutputStream counter;
	cool::ObjectIoStream<> countingStream(counter);
	countingStream.enableTypeChecking(false);
	this->xserialize_type(countingStream);
	this->xserialize(countingStream);
	return counter.getCounter();
}

void Record::serialize_record(char *buffer, int64_t size)
{
	cool::FixedBufferOutputStreamAdapter output(buffer, size);
	cool::ObjectIoStream<> stream(output);
	stream.enableTypeChecking(false);
	this->xserialize_type(stream);
	this->xserialize(stream);
}

/*Record* Record::deserialize_record(char *buffer, uint32_t size)
{
	cool::FixedBufferInputStreamAdapter input(buffer, size);
	cool::ObjectIoStream<> stream(input);
	stream.enableTypeChecking(false);

	Record* record_ptr = NULL;
	int type = record_ptr->xserialize_type(stream);
	switch (type)
	{
		case OBJECT:
			record_ptr = new ObjectRecord();
			record_ptr->xserialize(stream);
			break;
		case CHECKPOINT_CONTAINER_JOURNAL:
			record_ptr = new CheckpointHeader();
			record_ptr->xserialize(stream);
		default:
			break;
	}

	return record_ptr;

}*/

typedef std::pair<uint64_t, int64_t> JournalOffset;

class ContainerStartFlushMarker : public Record
{
	public:
		ContainerStartFlushMarker();
		ContainerStartFlushMarker(std::string path);
		void xserialize(cool::ObjectIoStream<>& stream);
		int get_type();
		std::string get_path();
	private:
		uint32_t reserved;
		uint32_t version;
		std::string path;
};

class ContainerEndFlushMarker : public Record
{

	public:
		ContainerEndFlushMarker();
		ContainerEndFlushMarker(std::string path, JournalOffset offset);
		void xserialize(cool::ObjectIoStream<>& stream);
		int get_type();
		JournalOffset get_last_commit_offset();
		std::string get_path();
	private:
		std::string path;
		JournalOffset last_commit_offset;
};

class ContainerAcceptComponentMarker : public Record
{
        public:
                ContainerAcceptComponentMarker();
                ContainerAcceptComponentMarker(std::list<int32_t>);
		void xserialize(cool::ObjectIoStream<>& stream); 
		int get_type();
                std::list<int32_t> get_comp_list();
        private:
                std::list<int32_t> comp_list;
};

class ContainerStat : public Record
{

public:
	std::string account;
	std::string container;
	std::string created_at;
	std::string put_timestamp;
	std::string delete_timestamp;
	int64_t object_count;
	int64_t bytes_used;
	std::string hash;
	std::string id;
	std::string status;
	std::string status_changed_at;
	std::map<std::string, std::string>  metadata; 

	ContainerStat(){;}

	ContainerStat(std::string account, std::string container, std::string created_at, \
                        std::string put_timestamp, std::string delete_timestamp, \
                        int64_t object_count, int64_t bytes_used, std::string hash,\
                        std::string id, std::string status, std::string status_changed_at, boost::python::dict metadata)
	{
		//definattion;
		this->account = account;
		this->container = container;
		this->created_at = created_at;
		this->put_timestamp = put_timestamp;
		this->delete_timestamp = delete_timestamp;
		this->object_count = object_count;
		this->bytes_used = bytes_used;
		this->hash = hash;
		this->id = id;
		this->status = status;
		this->status_changed_at = status_changed_at;
		this->set_metadata(metadata);
	}

	ContainerStat(std::string account, std::string container, std::string created_at, \
                        std::string put_timestamp, std::string delete_timestamp, \
                        int64_t object_count, int64_t bytes_used, std::string hash,\
                        std::string id, std::string status, std::string status_changed_at, std::map<std::string, std::string> metadata)
        {
                //definattion;
                this->account = account;
                this->container = container;
                this->created_at = created_at;
                this->put_timestamp = put_timestamp;
                this->delete_timestamp = delete_timestamp;
                this->object_count = object_count;
                this->bytes_used = bytes_used;
                this->hash = hash;
                this->id = id;
                this->status = status;
                this->status_changed_at = status_changed_at;
                this->metadata = metadata;
        }

	ContainerStat(ContainerStat const &s)
        {
                //definattion;
                this->account = s.account;
                this->container = s.container;
                this->created_at = s.created_at;
                this->put_timestamp = s.put_timestamp;
                this->delete_timestamp = s.delete_timestamp;
                this->object_count = s.object_count;
                this->bytes_used = s.bytes_used;
                this->hash = s.hash;
                this->id = s.id;
                this->status = s.status;
                this->status_changed_at = s.status_changed_at;
                this->metadata = s.metadata;
        }

	void set_metadata(boost::python::dict arg_metadata)
        {
                boost::python::list keys = arg_metadata.keys();
                for (int i = 0; i < len(keys); ++i)
                {
                        boost::python::extract<std::string> extracted_key(keys[i]);
                        if(!extracted_key.check())
                                continue;
                        std::string key = extracted_key;
                        boost::python::extract<std::string> extracted_val(arg_metadata[key]);
                        if(!extracted_val.check())
                                continue;
                        std::string value = extracted_val;
                        this->metadata[key] = value;
                }
        }

        boost::python::dict get_metadata()
        {
                boost::python::dict ret_metadata;
                for(std::map<std::string, std::string>::const_iterator it = this->metadata.begin(); it != this->metadata.end(); ++it)
                {
                           ret_metadata[it->first] = it->second;
                }
                return ret_metadata;
        }


	std::string getAccountName();
	
	void xserialize(cool::ObjectIoStream<>& stream);
	int get_type();
};

#define OBJECT_ADD 1
#define OBJECT_DELETE 2
#define OBJECT_MODIFY 3
#define OBJECT_UNKNOWN_DELETE 254
#define OBJECT_UNKNOWN_ADD 255

class ObjectRecord : public Record
{
private:
	uint64_t row_id;
	std::string created_at;
	uint64_t size;
	uint64_t old_size;
	std::string content_type;
	std::string etag;
	uint64_t deleted;
public:
	ObjectRecord();
	ObjectRecord(
		uint64_t row_id,
		std::string name,
		std::string created_at,
		uint64_t size,
		std::string content_type,
		std::string etag,
		uint64_t deleted = 1,
		uint64_t old_size = 0 
	)
	{
		this->row_id = row_id;
		this->name = name;
		this->created_at = created_at;
		this->size = size;
		this->old_size = old_size;
		this->content_type = content_type;
		this->etag = etag;
		this->deleted = deleted;
	}

	std::string name; // for lamda binding
	uint64_t get_row_id() const;
	std::string get_name() const;
	void set_content_type(std::string content_type);
	void set_name(std::string name);
	uint64_t get_deleted_flag() const;
	void set_deleted_flag(uint64_t deleted);
	std::string get_creation_time() const;
	uint64_t get_size() const;
	uint64_t get_old_size() const;
	std::string get_content_type() const;
	std::string get_etag() const;
	void xserialize(cool::ObjectIoStream<>& stream);
	int get_type();
};

class FinalObjectRecord : public Record
{
	public:
		FinalObjectRecord(std::string path, ObjectRecord obj_record);
		FinalObjectRecord();
//
		FinalObjectRecord(std::string path ,std::string name, std::string created_at, uint64_t size,uint64_t old_size , std::string content_type, std::string etag ,uint64_t deleted );
//row id 

		void xserialize(cool::ObjectIoStream<>& stream);

		int get_type();
		ObjectRecord & get_object_record();
		std::string get_path();
	private:
		std::string path;
		ObjectRecord obj_record;
};

class TransferObjectRecord : public Record
{
	public:
		TransferObjectRecord(FinalObjectRecord final_obj_record, uint64_t, uint64_t);
		TransferObjectRecord();
		void xserialize(cool::ObjectIoStream<>& stream);
		int get_type();
		FinalObjectRecord & get_final_object_record();
		uint64_t get_offset_file();
		uint64_t get_offset_id();
		void set_offset(uint64_t, uint64_t);
	private:
		FinalObjectRecord final_obj_record;
		uint64_t file_id;
		uint64_t id;
};

template<class T>
std::string genericSerializator(T const & o)
{       
        //abhishek-san
        T & object = const_cast<T &>(o); // we only get state here         
        std::ostringstream oStream(std::ios_base::binary);
        cool::StdOutputStreamAdapter sos(oStream);                         
        
        DefaultIOStream::Flags const typeInfo(DefaultIOStream::TYPE_CHECKING_DISABLED);
        DefaultIOStream oos(                                               
                sos,                                                       
                typeInfo                                                   
        );
        
        std::cout << "Hello world" << std::endl;                           
        oos.readWrite(object);
                                                                           
        std::string const serializedObject = oStream.str();                
        
        //usl::Crc32Producer crcProducer;                                  
        //crcProducer.process_bytes(serializedObject.c_str(), serializedObject.length());
        //cool::uint32 crc = crcProducer.checksum();                       
        cool::uint32 crc = 100;                                            
        
        oos.readWrite(crc);                                                
        
        return oStream.str();                                              
}                                                   


 
struct transfer_object_record_pickle_suite : boost::python::pickle_suite
{
    static
    boost::python::tuple
    getinitargs(const TransferObjectRecord  & w)
    {
//        cool::unused(w);
        //using namespace boost::python;

        //std::string const serializedData = genericSerializator(w);
        //return boost::python::make_tuple(serializedData);
        return boost::python::make_tuple();

    }
    static boost::python::tuple getstate(const TransferObjectRecord  & w)
    {
//        cool::unused(w);
        using namespace boost::python;

        std::string const serializedData = genericSerializator(w);
        return boost::python::make_tuple(serializedData);

    }

    static void setstate(TransferObjectRecord & object, boost::python::tuple state)
    {
//        cool::unused(w);
        using namespace boost::python;
//      boost::python::object lenRes = state.attr("__len__")();
//      cool::uint32 const stateLen = boost::python::extract<cool::uint32>(lenRes);


        std::string serializedData = extract<std::string>(state[0]);
        cool::uint32 const expectedCrc = 100;

        std::istringstream iStream(serializedData, std::ios_base::binary);
        cool::StdInputStreamAdapter sis(iStream);

        DefaultIOStream::Flags const typeInfo(DefaultIOStream::TYPE_CHECKING_DISABLED);
        DefaultIOStream ois(
                sis,
                typeInfo
        );

        ois.readWrite(object);
        cool::uint32 crc(0);
        ois.readWrite(crc);

    }


 };


struct final_object_record_pickle_suite : boost::python::pickle_suite
{
    static
    boost::python::tuple
    getinitargs(const FinalObjectRecord  & w)
    {
//        cool::unused(w);
        //using namespace boost::python;

        //std::string const serializedData = genericSerializator(w);
        //return boost::python::make_tuple(serializedData);
        return boost::python::make_tuple();

    }
    static boost::python::tuple getstate(const FinalObjectRecord  & w)
    {
//        cool::unused(w);
        using namespace boost::python;

        std::string const serializedData = genericSerializator(w);
        return boost::python::make_tuple(serializedData);

    }

    static void setstate(FinalObjectRecord & object, boost::python::tuple state)
    {
//        cool::unused(w);
        using namespace boost::python;
//      boost::python::object lenRes = state.attr("__len__")();
//      cool::uint32 const stateLen = boost::python::extract<cool::uint32>(lenRes);


        std::string serializedData = extract<std::string>(state[0]);
        cool::uint32 const expectedCrc = 100;

        std::istringstream iStream(serializedData, std::ios_base::binary);
        cool::StdInputStreamAdapter sis(iStream);

        DefaultIOStream::Flags const typeInfo(DefaultIOStream::TYPE_CHECKING_DISABLED);
        DefaultIOStream ois(
                sis,
                typeInfo
        );

        ois.readWrite(object);
        cool::uint32 crc(0);
        ois.readWrite(crc);

    }


 };

class SnapshotHeader: public Record
{
	public:
		SnapshotHeader(JournalOffset offset);
		SnapshotHeader();
		JournalOffset get_last_commit_offset();
		void xserialize(cool::ObjectIoStream<>& stream);
		int get_type();
	private:
		JournalOffset last_commit_offset;
};

class CheckpointHeader: public Record
{
	public:
		CheckpointHeader();
		JournalOffset get_last_commit_offset();
		CheckpointHeader(JournalOffset offset);
		void xserialize(cool::ObjectIoStream<>& stream);
		int get_type();
	private:
		JournalOffset last_commit_offset;
};

class ActiveRecord: public Record
{
/* Dummy implementation of record class
 * Contains Request class pointer
 * helper_utils::Callback class pointer
 */
	public:
		ActiveRecord();
		ActiveRecord(int operation,std::string object_name, std::string request_method, std::string object_id, int64_t id);
		~ActiveRecord();
		ActiveRecord(boost::shared_ptr<helper_utils::Request> request);
		void set_time(int64_t time);
		int64_t get_time();
		void set_offset(int64_t offset);
		int64_t get_offset();
//		boost::shared_ptr<helper_utils::Request> get_request();
		std::string get_object_name();
		std::string get_object_id();
		int64_t get_transaction_id();
		void set_transaction_id(int64_t transaction_id);
		int get_operation_type();
		std::string get_request_method();
		void set_operation_type(helper_utils::lockType operation);
		int get_type();
		void xserialize(cool::ObjectIoStream<>& stream);
		void xserialize_old(cool::ObjectIoStream<>& stream);

	private:
		//boost::shared_ptr<helper_utils::Request> request;
		int operation;
		std::string object_name;
		std::string request_method;
		std::string object_id;
		int64_t id;
		int64_t time;
		int64_t version;
		int64_t temp;
};



struct active_object_record_pickle_suite : boost::python::pickle_suite
{
    static
    boost::python::tuple
    getinitargs(const ActiveRecord  & w)
    {
//        cool::unused(w);
        //using namespace boost::python;

        //std::string const serializedData = genericSerializator(w);
        //return boost::python::make_tuple(serializedData);
        return boost::python::make_tuple();

    }
    static boost::python::tuple getstate(const ActiveRecord  & w)
    {
//        cool::unused(w);
        using namespace boost::python;

        std::string const serializedData = genericSerializator(w);
        return boost::python::make_tuple(serializedData);

    }


    static void setstate(ActiveRecord & object, boost::python::tuple state)
    {
//        cool::unused(w);
        using namespace boost::python;
//      boost::python::object lenRes = state.attr("__len__")();
//      cool::uint32 const stateLen = boost::python::extract<cool::uint32>(lenRes);


        std::string serializedData = extract<std::string>(state[0]);

        cool::uint32 const expectedCrc = 100;

        std::istringstream iStream(serializedData, std::ios_base::binary);
        cool::StdInputStreamAdapter sis(iStream);

        DefaultIOStream::Flags const typeInfo(DefaultIOStream::TYPE_CHECKING_DISABLED);
        DefaultIOStream ois(
                sis,
                typeInfo
        );

        ois.readWrite(object);
        cool::uint32 crc(0);
        ois.readWrite(crc);

    }


 };

class SnapshotHeaderRecord: public Record
{
	public:
		SnapshotHeaderRecord();
		SnapshotHeaderRecord(int64_t last_transaction_id);
		~SnapshotHeaderRecord();
		int64_t get_last_transaction_id() const;
		int64_t get_journal_version() const;
		int get_type();
		void xserialize(cool::ObjectIoStream<>& stream);
	private:
		int64_t last_transaction_id;
		int64_t journal_version;
};

class CheckpointRecord: public Record
{
	public:
		CheckpointRecord();
		~CheckpointRecord();
		CheckpointRecord(int64_t lat_transaction_id, std::vector<int64_t> const & id_list);
		int get_type();
		int64_t get_last_transaction_id() const;
		void xserialize(cool::ObjectIoStream<>& stream);
		//std::tr1::tuple<int64_t, int64_t *> get_id_list();
		std::vector<int64_t> get_id_list();
	private:
		//std::tr1::tuple<int64_t, int64_t *> id_list;
		int64_t last_transaction_id;
		std::vector<int64_t> id_list;
};

class CloseRecord: public Record
{
	public:
		CloseRecord();
		~CloseRecord();
		CloseRecord(int64_t transaction_id, std::string object_name);
		int64_t get_transaction_id();
		int get_type();
		void xserialize(cool::ObjectIoStream<>& stream);
	private:
		int64_t transaction_id;
		std::string object_name;
};

/*class GLSnapshotHeader: public Record
{
	public:
		GLSnapshotHeader(uint64_t latest_state_change_id);
		~GLSnapshotHeader();
		int64_t get_last_state_change_id();
		int get_type();
		void xserialize(cool::ObjectIoStream<>& stream);
	private:
		float version;
		uint64_t latest_state_change_id;
};*/

class GLCheckpointHeader: public Record
{
	public:
		GLCheckpointHeader(std::vector<std::string> state_change_ids);
		~GLCheckpointHeader();
		int get_type();
		std::vector<std::string> get_id_list();
		void xserialize(cool::ObjectIoStream<>& stream);
	private:
		int version;
		std::vector<std::string> id_list;
};

class ComponentInfoRecord: public Record
{
        private:
                std::string service_id;
                std::string service_ip;
                int32_t service_port;
        public:
                ComponentInfoRecord();
                ComponentInfoRecord(std::string service_id, std::string service_ip, int32_t service_port);
                ~ComponentInfoRecord();
                void set_service_id(std::string service_id);
                std::string get_service_id() const;
                void set_service_ip(std::string service_ip);
                std::string get_service_ip() const;
                void set_service_port(int32_t service_port);
                int32_t get_service_port() const;
                int get_type();
                void xserialize(cool::ObjectIoStream<>& stream);
		bool operator==(const ComponentInfoRecord &ob);
		bool operator< (const ComponentInfoRecord& ob) const;
};

class CompleteComponentRecord: public Record
{
	public:
		CompleteComponentRecord(
			std::vector<ComponentInfoRecord> comp_obj_list,
			float service_version,
			float gl_version
		);
		CompleteComponentRecord()
		{}
		std::vector<ComponentInfoRecord> get_component_lists();
		float get_service_version();
		float get_gl_version();
		void xserialize(cool::ObjectIoStream<>& stream);
		int get_type();
	private:
		std::vector<ComponentInfoRecord> comp_obj_list;
		float service_version;
		float gl_version;
		int32_t total_comp_count;
		std::string version_in_str;
};

class ServiceComponentCouple: public Record
{
	public:
		ServiceComponentCouple(int32_t component_no, ComponentInfoRecord cir_obj);
		ServiceComponentCouple();
		int32_t get_component();
		int get_type();
		ComponentInfoRecord & get_component_info_record();
		void xserialize(cool::ObjectIoStream<>& stream);
	private:
		int32_t component;
		ComponentInfoRecord comp_info_obj;
};

class TransientMapRecord: public Record
{
	public:
		TransientMapRecord(std::vector<ServiceComponentCouple>, float version, int service_type);
		TransientMapRecord();
		std::vector<ServiceComponentCouple> get_service_comp_obj();
		float get_version();
		int get_type();
		int get_service_type();
		void xserialize(cool::ObjectIoStream<>& stream);
	private:
		std::vector<ServiceComponentCouple> serv_comp_list;
		float version;
		int service_type;
		int32_t comp_count;
		std::string version_in_str;
};

class GLJournalRecord: public Record
{
	public:
		GLJournalRecord();
		int get_type();
		void xserialize(cool::ObjectIoStream<>& stream);
	private:
		float version;
		uint64_t state_change_id;
		int operation_type;
		std::string source;
		std::vector<std::string> destinations;
		//transient map
};

class NodeInfoFileRecord: public Record
{
        private:
                std::string node_id;
                std::string node_ip;
                int32_t localLeader_port;
		int32_t accountService_port;
		int32_t containerService_port;
		int32_t objectService_port;
		int32_t accountUpdaterService_port;
		int32_t proxy_port;
                int node_status;

        public:
                NodeInfoFileRecord();
                NodeInfoFileRecord(std::string node_id, std::string node_ip, int32_t localLeader_port, int32_t accountService_port, int32_t containerService_port, int32_t objectService_port, int32_t accountUpdaterService_port, int32_t proxy_port, int node_status)
                {
                        this->node_id = node_id;
                        this->node_ip = node_ip;
                        this->localLeader_port = localLeader_port;
			this->accountService_port = accountService_port;
			this->containerService_port = containerService_port;
			this->objectService_port = objectService_port;
			this->accountUpdaterService_port = accountUpdaterService_port;
			this->proxy_port = proxy_port;
                        this->node_status = node_status;
                }

                ~NodeInfoFileRecord();
		int get_type();
                void set_node_id(std::string node_id);
                std::string get_node_id();
                void set_node_ip(std::string node_ip);
                std::string get_node_ip();
                void set_localLeader_port(int32_t localLeader_port);
                int32_t get_localLeader_port();
		void set_accountService_port(int32_t accountService_port);
		int32_t get_accountService_port();
		void set_containerService_port(int32_t containerService_port);
		int32_t get_containerService_port();
		void set_objectService_port(int32_t objectService_port);
		int32_t get_objectService_port();
		void set_accountUpdaterService_port(int32_t accountUpdaterService_port);
		int32_t get_accountUpdaterService_port();
		void set_proxy_port(int32_t proxy_port);
		int32_t get_proxy_port();
                void set_node_status(int node_status);
                int get_node_status();
                void xserialize(cool::ObjectIoStream<>& stream);
};

class RingRecord: public Record
{
        private: 
		std::vector<std::string> fs_list;
		std::vector<std::string> account_level_dir_list;
		std::vector<std::string> container_level_dir_list; 
		std::vector<std::string> object_level_dir_list;
		


        public:
                RingRecord();
                ~RingRecord();
		void set_data();
		std::vector<std::string> get_fs_list();
		std::vector<std::string> get_account_level_dir_list();
		std::vector<std::string> get_container_level_dir_list();
		std::vector<std::string> get_object_level_dir_list();
                int get_type();
                void xserialize(cool::ObjectIoStream<>& stream);
};

class RecoveryRecord : public Record
{
	public:
		RecoveryRecord(int operation, std::string source, int status);		//For Recovery, Deletion
		RecoveryRecord(int operation, std::list<std::string> nodes_being_added, int status);		//For Recovery, Deletion
		std::string get_state_change_node();
		void set_destination(std::string destination);
		std::string get_destination_node();
//		std::string get_keyname_for_sci_map();
		std::list<std::string> get_list_of_nodes_getting_added();
		void set_status(int status);
		int get_status();
		int get_type();
		void xserialize(cool::ObjectIoStream<>& stream);
		int get_operation();
		void set_operation(int operation);
		void set_planned_map(std::vector<std::pair<int32_t, ComponentInfoRecord> > planned_map);
		std::vector<std::pair<int32_t, ComponentInfoRecord> > get_planned_map();
		void remove_from_planned_map(std::vector<std::pair<int32_t, ComponentInfoRecord> > moved_components);
		void clear_planned_map();	//Only for testing purpose
	private:
		int operation;			// Addition, Deletion, Recovery, Balancing
		std::string state_change_node;	//Node failed, added, deleted (key in map)
		int status;			// system_state::StateChangeStatus
		std::string destination;	//On which recovery process is being created
		std::vector<std::pair<int32_t, ComponentInfoRecord> > planned_map;
		std::list<std::string> nodes_being_added;
};

class ElectionRecord : public Record
{
public:
	ElectionRecord(std::string node_id, int connection_count);
	ElectionRecord();
	int get_type();
	std::string get_node_id();
	int get_connection_count();
	void xserialize(cool::ObjectIoStream<>& stream);
private:
	std::string node_id;
	int connection_count;
};

class GLMountInfoRecord: public Record
{
	public:
		GLMountInfoRecord(std::string, std::string);
		GLMountInfoRecord();
		std::string get_failed_node_id();
		std::string get_destination_node_id();
		int get_type();
		void xserialize(cool::ObjectIoStream<>& stream);
	private:
		std::string failed_node;
		std::string destination_node;
};


} //namespace recordStructure

#endif

