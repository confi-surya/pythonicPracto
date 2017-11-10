#include "libraryUtils/record_structure.h"
#include "libraryUtils/object_storage_log_setup.h"

namespace recordStructure
{

Record * deserialize_record(char *buffer, int64_t size)
{
	cool::FixedBufferInputStreamAdapter input(buffer, size);
	cool::ObjectIoStream<> stream(input);
	stream.enableTypeChecking(false);
	Record * record = NULL;
	int type = record->xserialize_type(stream);
	switch (type)
	{
	case OPEN_RECORD:
		record = new ActiveRecord();
                OSDLOG(INFO,"Size of RECORD is : " << size);
		if ( size < 174 )
                        record->xserialize_old(stream);
                else
                        record->xserialize(stream);
		break;
	case CLOSE_RECORD:
		record = new CloseRecord();
		record->xserialize(stream);
		break;
	case SNAPSHOT_HEADER_RECORD:
		record = new SnapshotHeaderRecord();
		record->xserialize(stream);
		break;
	case CHECKPOINT_RECORD:
		OSDLOG(DEBUG, "deserialize_record CHECKPOINT_RECORD");
		record = new CheckpointRecord();
		record->xserialize(stream);
		break;
	case CONTAINER_STAT:
		record = new ContainerStat();
		record->xserialize(stream);
		break;
	case FINAL_OBJECT_RECORD:
		record = new FinalObjectRecord();
		record->xserialize(stream);
		break;
	case OBJECT_RECORD:
		record = new ObjectRecord();
		record->xserialize(stream);
		break;
	case SNAPSHOT_CONTAINER_JOURNAL:
		record = new SnapshotHeader();
		record->xserialize(stream);
		break;
	case CHECKPOINT_CONTAINER_JOURNAL:
		record = new CheckpointHeader();
		record->xserialize(stream);
		break;
	case CONTAINER_START_FLUSH:
		record = new ContainerStartFlushMarker();
		record->xserialize(stream);
		break;
	case CONTAINER_END_FLUSH:
		record = new ContainerEndFlushMarker();
		record->xserialize(stream);
		break;
	case NODE_INFO_FILE_RECORD: 
		record = new NodeInfoFileRecord();
		record->xserialize(stream);
		break;
	case RING_RECORD:
                record = new RingRecord();
                record->xserialize(stream);
                break;
 	case COMPONENT_INFO_RECORD:
                record = new ComponentInfoRecord();
                record->xserialize(stream);
                break;
	case COMPLETE_COMPONENT_RECORD:
		record = new CompleteComponentRecord();
		record->xserialize(stream);
		break;
	case TRANSIENT_MAP:
		record = new TransientMapRecord();
		record->xserialize(stream);
		break;
	case GL_MOUNT_INFO_RECORD:
		record = new GLMountInfoRecord();
		record->xserialize(stream);
		break;
	case ELECTION_RECORD:
		record = new ElectionRecord();
		record->xserialize(stream);
		break;
        case ACCEPT_COMPONENTS_MARKER:
		record = new ContainerAcceptComponentMarker();
		record->xserialize(stream);
		break;
	default:
		break;
	}
	return record;
}

std::string ContainerStat::getAccountName()
{
	return this->account;
}

int ContainerStat::get_type()
{
	return CONTAINER_STAT;
}

FinalObjectRecord::FinalObjectRecord()
{
}

FinalObjectRecord::FinalObjectRecord(std::string path, ObjectRecord obj_record):path(path), obj_record(obj_record)
{
}

FinalObjectRecord::FinalObjectRecord(std::string path ,std::string name, std::string created_at, uint64_t size,uint64_t old_size , std::string content_type, std::string etag ,uint64_t deleted): path(path),obj_record(ObjectRecord(1,name,created_at,size,content_type,etag,deleted,old_size))
{
//(std::string path ,std::string name, std::string created_at, uint64_t size,uint64_t old_size , std::string content_type, std::string etag ,uint64_t deleted

}

void FinalObjectRecord::xserialize(cool::ObjectIoStream<>& stream)
{
	stream.readWrite(this->path);
	this->obj_record.xserialize(stream);
}

TransferObjectRecord::TransferObjectRecord(FinalObjectRecord final_obj_record, uint64_t file, uint64_t id):final_obj_record(final_obj_record), file_id(file), id(id)
{
}

TransferObjectRecord::TransferObjectRecord()
{
}

void TransferObjectRecord::xserialize(cool::ObjectIoStream<>& stream)
{
	stream.readWrite(this->file_id);
	stream.readWrite(this->id);
	this->final_obj_record.xserialize(stream);
}

uint64_t TransferObjectRecord::get_offset_file()
{
        return this->file_id;
}

uint64_t TransferObjectRecord::get_offset_id()
{
	return this->id;
}

FinalObjectRecord & TransferObjectRecord::get_final_object_record()
{
        return this->final_obj_record;
}

int TransferObjectRecord::get_type()
{
	return TRANSFER_OBJECT_RECORD;
}

void TransferObjectRecord::set_offset(uint64_t file, uint64_t id)
{
	this->file_id = file;
	this->id = id;
}

ObjectRecord::ObjectRecord()
{
}

uint64_t ObjectRecord::get_row_id() const
{
	return this->row_id;
}

std::string ObjectRecord::get_name() const
{
	return this->name;
}

void ObjectRecord::set_name(std::string name)
{
	this->name = name;
}

void ObjectRecord::set_content_type(std::string content_type)
{
	this->content_type = content_type;
}

int ObjectRecord::get_type()
{
	return OBJECT_RECORD;
}

int FinalObjectRecord::get_type()
{
	return FINAL_OBJECT_RECORD;
}

std::string FinalObjectRecord::get_path()
{
	return this->path;
}

ObjectRecord & FinalObjectRecord::get_object_record()
{
	return this->obj_record;
}

uint64_t ObjectRecord::get_deleted_flag() const
{
	//return this->deleted == 2 ? true: false;	//TODO To be changed to enum object_type
	return this->deleted;	//TODO To be changed to enum object_type
}

void ObjectRecord::set_deleted_flag(uint64_t deleted) 
{
	this->deleted = deleted;   //TODO To be changed to enum object_type
}

std::string ObjectRecord::get_creation_time() const
{
	return this->created_at;
}

uint64_t ObjectRecord::get_size() const
{
	return this->size;
}

uint64_t ObjectRecord::get_old_size() const
{
	return this->old_size;
}

std::string ObjectRecord::get_content_type() const
{
	return this->content_type;
}

void ContainerStat::xserialize(cool::ObjectIoStream<>& stream)
{
	stream.readWrite(this->account);
	stream.readWrite(this->container);
	stream.readWrite(this->created_at);
	stream.readWrite(this->put_timestamp);
	stream.readWrite(this->delete_timestamp);
	stream.readWrite(this->object_count);
	stream.readWrite(this->bytes_used);
	stream.readWrite(this->hash);
	stream.readWrite(this->id);
	stream.readWrite(this->status);
	stream.readWrite(this->status_changed_at);
	stream.readWrite(this->metadata);
}

void ObjectRecord::xserialize(cool::ObjectIoStream<>& stream)
{
	stream.readWrite(this->name);
	stream.readWrite(this->deleted);
	stream.readWrite(this->created_at);
	stream.readWrite(this->size);
	stream.readWrite(this->content_type);
	stream.readWrite(this->etag);
	stream.readWrite(this->old_size);
}

std::string ObjectRecord::get_etag() const
{
	return this->etag;
}

SnapshotHeader::SnapshotHeader()
{
}

SnapshotHeader::SnapshotHeader(JournalOffset offset) : last_commit_offset(offset)
{
}

int SnapshotHeader::get_type()
{
	return SNAPSHOT_CONTAINER_JOURNAL;
}

JournalOffset SnapshotHeader::get_last_commit_offset()
{
	return this->last_commit_offset;
}

void SnapshotHeader::xserialize(cool::ObjectIoStream<>& stream)
{
	stream.readWrite(this->last_commit_offset);
}

JournalOffset CheckpointHeader::get_last_commit_offset()
{
	return this->last_commit_offset;
}

CheckpointHeader::CheckpointHeader()
{
}

int CheckpointHeader::get_type()
{
	return CHECKPOINT_CONTAINER_JOURNAL;
}

CheckpointHeader::CheckpointHeader(JournalOffset offset) : last_commit_offset(offset)
{
}

void CheckpointHeader::xserialize(cool::ObjectIoStream<>& stream)
{
	stream.readWrite(this->last_commit_offset);
}

ContainerStartFlushMarker::ContainerStartFlushMarker()
{
}

ContainerStartFlushMarker::ContainerStartFlushMarker(
	std::string path
):
	path(path)
{
	reserved = 1;
	version = 1;
}

int ContainerStartFlushMarker::get_type()
{
	return CONTAINER_START_FLUSH;
}

std::string ContainerStartFlushMarker::get_path()
{
	return this->path;
}

void ContainerStartFlushMarker::xserialize(cool::ObjectIoStream<>& stream)
{
	stream.readWrite(this->path);
	stream.readWrite(this->reserved);
	stream.readWrite(this->version);
}

ContainerEndFlushMarker::ContainerEndFlushMarker()
{
}

ContainerEndFlushMarker::ContainerEndFlushMarker(
	std::string path,
	JournalOffset last_commit_offset
):
	path(path),
	last_commit_offset(last_commit_offset)
{
}

int ContainerEndFlushMarker::get_type()
{
	return CONTAINER_END_FLUSH;
}

std::string ContainerEndFlushMarker::get_path()
{
	return this->path;
}

JournalOffset ContainerEndFlushMarker::get_last_commit_offset()
{
	return this->last_commit_offset;
}

void ContainerEndFlushMarker::xserialize(cool::ObjectIoStream<>& stream)
{
	stream.readWrite(this->path);
	stream.readWrite(this->last_commit_offset);
}

ContainerAcceptComponentMarker::ContainerAcceptComponentMarker()
{
}

ContainerAcceptComponentMarker::ContainerAcceptComponentMarker(std::list<int32_t> component_number_list):comp_list(component_number_list)
{
}

void ContainerAcceptComponentMarker::xserialize(cool::ObjectIoStream<>& stream)
{
	stream.readWrite(this->comp_list);
}

int ContainerAcceptComponentMarker::get_type()
{
        return ACCEPT_COMPONENTS_MARKER;
}

std::list<int32_t> ContainerAcceptComponentMarker::get_comp_list()
{
    return (this->comp_list);
}

ActiveRecord::ActiveRecord(boost::shared_ptr<helper_utils::Request> request)
:
	operation(request->get_operation_type()),
	id(0)
{
	this->object_name = request->get_object_name();
	this->request_method = request->get_request_method();
	this->object_id = request->get_object_id();
	this->version = 1;
	this->temp = NULL;
}

ActiveRecord::ActiveRecord(int operation,std::string object_name, std::string request_method, std::string object_id, int64_t id)
:
	operation(operation),
	object_name(object_name),
	request_method(request_method),
	object_id(object_id),
	id(id),
	time(0),
	version(1),
	temp(NULL)
{
}

ActiveRecord::ActiveRecord()
{
	this->version = 1;
        this->temp = NULL;
}

ActiveRecord::~ActiveRecord()
{
}

void ActiveRecord::set_time(int64_t time)
{
	this->time = time;
}

int64_t ActiveRecord::get_time(){
	return this->time;
}

std::string ActiveRecord::get_request_method()
{
	return this->request_method;
}

int ActiveRecord::get_type()
{
	return OPEN_RECORD;
}


void ActiveRecord::xserialize_old(cool::ObjectIoStream<>& stream)
{
        stream.readWrite(this->object_name);
        stream.readWrite(this->request_method);
        stream.readWrite(this->id);
        stream.readWrite(this->operation);
        stream.readWrite(this->object_id);
        //stream.readWrite(this->time);
}


void ActiveRecord::xserialize(cool::ObjectIoStream<>& stream)
{
	stream.readWrite(this->object_name);
	stream.readWrite(this->request_method);
	stream.readWrite(this->id);
	stream.readWrite(this->operation);
	stream.readWrite(this->object_id);
	stream.readWrite(this->time);
	stream.readWrite(this->version);
	stream.readWrite(this->temp);
}

/*boost::shared_ptr<helper_utils::Request> ActiveRecord::get_request()
{
	return this->request;
}*/

int ActiveRecord::get_operation_type()
{
	return this->operation;
}

std::string ActiveRecord::get_object_id()
{
	//return this->request->get_object_id();
	return this->object_id;
}

std::string ActiveRecord::get_object_name()
{
	//return this->request->get_object_name();
	return this->object_name;
}

void ActiveRecord::set_transaction_id(int64_t transaction_id)
{
	this->id = transaction_id;
}

int64_t ActiveRecord::get_transaction_id()
{
	return this->id;
}

CloseRecord::CloseRecord()
{
}

CloseRecord::~CloseRecord()
{
}

CloseRecord::CloseRecord(int64_t transaction_id, std::string object_name):transaction_id(transaction_id), object_name(object_name)
{
}

int CloseRecord::get_type()
{
	return CLOSE_RECORD;
}

void CloseRecord::xserialize(cool::ObjectIoStream<>& stream)
{
	stream.readWrite(this->object_name);
	stream.readWrite(this->transaction_id);
}

int64_t CloseRecord::get_transaction_id()
{
	return this->transaction_id;
}

SnapshotHeaderRecord::SnapshotHeaderRecord()
	: journal_version(1)
{
}

SnapshotHeaderRecord::SnapshotHeaderRecord(int64_t last_transaction_id)
	: last_transaction_id(last_transaction_id), journal_version(1)
{
}

SnapshotHeaderRecord::~SnapshotHeaderRecord()
{
}

int SnapshotHeaderRecord::get_type()
{
	return SNAPSHOT_HEADER_RECORD;
}

int64_t SnapshotHeaderRecord::get_last_transaction_id() const
{
	return this->last_transaction_id;
}

int64_t SnapshotHeaderRecord::get_journal_version() const
{               
        return this->journal_version;
}

void SnapshotHeaderRecord::xserialize(cool::ObjectIoStream<>& stream)
{
	stream.readWrite(this->last_transaction_id);
	try
        {
                stream.readWrite(this->journal_version);
        }
        catch(...)
        {
                OSDLOG(INFO, "Old Journal File. Setting journal version to 0");
                this->journal_version = 0;
        }
}

CheckpointRecord::CheckpointRecord()
{
}

std::vector<int64_t> CheckpointRecord::get_id_list()
{
	return this->id_list;
}

CheckpointRecord::CheckpointRecord(
	int64_t last_transaction_id,
	std::vector<int64_t> const & id_list
)
	: last_transaction_id(last_transaction_id), id_list(id_list)
{
}

CheckpointRecord::~CheckpointRecord()
{
}

int CheckpointRecord::get_type()
{
	return CHECKPOINT_RECORD;
}

int64_t CheckpointRecord::get_last_transaction_id() const
{
	return this->last_transaction_id;
}

void CheckpointRecord::xserialize(cool::ObjectIoStream<>& stream)
{
	stream.readWrite(this->last_transaction_id);
	stream.readWrite(this->id_list);
}

NodeInfoFileRecord::NodeInfoFileRecord()
{
}

int NodeInfoFileRecord::get_type()
{
	return NODE_INFO_FILE_RECORD;
}

void NodeInfoFileRecord::set_node_id(std::string node_id)
{
        this->node_id = node_id;
}

std::string NodeInfoFileRecord::get_node_id()
{
        return this->node_id;
}

void NodeInfoFileRecord::set_node_ip(std::string node_ip)
{
        this->node_ip = node_ip;
}

std::string NodeInfoFileRecord::get_node_ip()
{
        return this->node_ip;
}

void NodeInfoFileRecord::set_localLeader_port(int32_t localLeader_port)
{
        this->localLeader_port = localLeader_port;
}

int32_t NodeInfoFileRecord::get_localLeader_port()
{
        return this->localLeader_port;
}

void NodeInfoFileRecord::set_accountService_port(int32_t accountService_port)
{
        this->accountService_port = accountService_port;
}

int32_t NodeInfoFileRecord::get_accountService_port()
{
        return this->accountService_port;
}

void NodeInfoFileRecord::set_containerService_port(int32_t containerService_port)
{
        this->containerService_port = containerService_port;
}

int32_t NodeInfoFileRecord::get_containerService_port()
{
        return this->containerService_port;
}

void NodeInfoFileRecord::set_objectService_port(int32_t objectService_port)
{
        this->objectService_port = objectService_port;
}

int32_t NodeInfoFileRecord::get_objectService_port()
{
        return this->objectService_port;
}

void NodeInfoFileRecord::set_accountUpdaterService_port(int32_t accountUpdaterService_port)
{
        this->accountUpdaterService_port = accountUpdaterService_port;
}

int32_t NodeInfoFileRecord::get_accountUpdaterService_port()
{
        return this->accountUpdaterService_port;
}

void NodeInfoFileRecord::set_proxy_port(int32_t proxy_port)
{
	this->proxy_port = proxy_port;
}

int32_t NodeInfoFileRecord::get_proxy_port()
{
	return this->proxy_port;
}

void NodeInfoFileRecord::set_node_status(int node_status)
{
        this->node_status = node_status;
}

int NodeInfoFileRecord::get_node_status()
{
        return this->node_status;
}

void NodeInfoFileRecord::xserialize(cool::ObjectIoStream<>& stream)
{
        stream.readWrite(this->node_id);
        stream.readWrite(this->node_ip);
        stream.readWrite(this->localLeader_port);
        stream.readWrite(this->accountService_port);
        stream.readWrite(this->containerService_port);
        stream.readWrite(this->objectService_port);
        stream.readWrite(this->accountUpdaterService_port);
	stream.readWrite(this->proxy_port);
        stream.readWrite(this->node_status);
}

NodeInfoFileRecord::~NodeInfoFileRecord()
{
}

//---------- Ring Record - Common for Account/Container/Object ------------//

RingRecord::RingRecord() //Declared in record_structure.h
{
}

RingRecord::~RingRecord()
{
}
 
void RingRecord::set_data()
{
	std::string temp_fs_list[10] = {"OSP_01",  "OSP_02", "OSP_03", "OSP_04",
		"OSP_05", "OSP_06", "OSP_07", "OSP_08", "OSP_09", "OSP_10"};
	std::string temp_account_level_dir_list[10] = {"a01",
		"a02", "a03", "a04", "a05", "a06", "a07", "a08", "a09", "a10"};
	std::string temp_container_level_dir_list[10] = {"c01",
		"c02", "c03", "c04", "c05", "c06", "c07", "c08", "c09", "c10"};
	std::string temp_object_level_dir_list[20] = {"o01", "o02",
		"o03", "o04", "o05", "o06", "o07", "o08", "o09", "o10", "o11", "o12",
		"o13", "o14", "o15", "o16", "o17", "o18", "o19", "o20"};

	for (int i=0; i<10; i++)
	{
		this->fs_list.push_back(temp_fs_list[i]);
		this->account_level_dir_list.push_back(temp_account_level_dir_list[i]);
		this->container_level_dir_list.push_back(temp_container_level_dir_list[i]);
	}

	for (int i=0; i<20; i++)
	{
		this->object_level_dir_list.push_back(temp_object_level_dir_list[i]);
	}
	
		
}

int RingRecord::get_type()
{
        return RING_RECORD;
}

std::vector<std::string> RingRecord::get_fs_list()
{
	return this->fs_list;
}

std::vector<std::string> RingRecord::get_account_level_dir_list()
{
	return this->account_level_dir_list;
}

std::vector<std::string> RingRecord::get_container_level_dir_list()
{
	return this->container_level_dir_list;
}

std::vector<std::string> RingRecord::get_object_level_dir_list()
{
	return this->object_level_dir_list;
}

void RingRecord::xserialize(cool::ObjectIoStream<>& stream)
{
	stream.readWrite(this->fs_list);
	stream.readWrite(this->account_level_dir_list);
	stream.readWrite(this->container_level_dir_list);
	stream.readWrite(this->object_level_dir_list);
}


CompleteComponentRecord::CompleteComponentRecord(
	std::vector<recordStructure::ComponentInfoRecord> comp_obj_list,
	float service_version,
	float gl_version
):
	comp_obj_list(comp_obj_list),
	service_version(service_version),
	gl_version(gl_version)
{
	this->total_comp_count = comp_obj_list.size();
	this->version_in_str = boost::lexical_cast<std::string>(this->gl_version);
}

std::vector<recordStructure::ComponentInfoRecord> CompleteComponentRecord::get_component_lists()
{
	return this->comp_obj_list;
}

float CompleteComponentRecord::get_service_version()
{
	return this->service_version;
}

float CompleteComponentRecord::get_gl_version()
{
	this->gl_version = boost::lexical_cast<float>(this->version_in_str);
	return this->gl_version;
}

int CompleteComponentRecord::get_type()
{
	return COMPLETE_COMPONENT_RECORD;
}

void CompleteComponentRecord::xserialize(cool::ObjectIoStream<>& stream)
{
	stream.readWrite(this->total_comp_count);
	std::vector<recordStructure::ComponentInfoRecord>::iterator it;
	//for(; it != this->comp_obj_list.end(); ++it)
	if(this->comp_obj_list.empty())
	{
		this->comp_obj_list.resize(this->total_comp_count); //TODO: Use a MACRO for no_of_components
		it = this->comp_obj_list.begin();
		for(int32_t i = 1; i <= this->total_comp_count; i++)
		{
			(*it).xserialize(stream);
			it++;
		}
	}
	else
	{
		it = this->comp_obj_list.begin();
		for(; it != this->comp_obj_list.end(); ++it)
		{
			(*it).xserialize(stream);
		}
	}
	stream.readWrite(this->service_version);
	//stream.readWrite(this->gl_version);
	stream.readWrite(this->version_in_str);

}

ComponentInfoRecord::ComponentInfoRecord()
{
}

ComponentInfoRecord::ComponentInfoRecord(
	std::string service_id,
	std::string service_ip,
	int32_t service_port
):
	service_id(service_id),
	service_ip(service_ip),
	service_port(service_port)
{
}

ComponentInfoRecord::~ComponentInfoRecord()
{
}

int ComponentInfoRecord::get_type()
{
        return COMPONENT_INFO_RECORD;
}

bool ComponentInfoRecord::operator==(const ComponentInfoRecord & ob)
{
	return (this->service_id == ob.service_id and this->service_ip == ob.service_ip and this->service_port == ob.service_port);
}

bool ComponentInfoRecord::operator< (const ComponentInfoRecord& ob) const
{
	//return (this->service_id != ob.service_id);
	return (this->service_id < ob.service_id);
}

void ComponentInfoRecord::set_service_id(std::string service_id)
{
        this->service_id = service_id;
}

std::string ComponentInfoRecord::get_service_id() const
{
        return this->service_id;
}

void ComponentInfoRecord::set_service_ip(std::string service_ip)
{
        this->service_ip = service_ip;
}

std::string ComponentInfoRecord::get_service_ip() const
{
        return this->service_ip;
}
void ComponentInfoRecord::set_service_port(int32_t service_port)
{
        this->service_port = service_port;
}

int32_t ComponentInfoRecord::get_service_port() const
{
        return this->service_port;
}

void ComponentInfoRecord::xserialize(cool::ObjectIoStream<>& stream)
{
	stream.readWrite(this->service_id);
	stream.readWrite(this->service_ip);
	stream.readWrite(this->service_port);
}

ServiceComponentCouple::ServiceComponentCouple()
{
}

ServiceComponentCouple::ServiceComponentCouple(
	int32_t component_no,
	ComponentInfoRecord cir_obj
):
	component(component_no),
	comp_info_obj(cir_obj)
{	
}

void ServiceComponentCouple::xserialize(cool::ObjectIoStream<>& stream)
{
	stream.readWrite(component);
	this->comp_info_obj.xserialize(stream);
}

int ServiceComponentCouple::get_type()
{
	return SERVICE_COMPONENT_RECORD;
}

TransientMapRecord::TransientMapRecord()
{
}

TransientMapRecord::TransientMapRecord(
	std::vector<ServiceComponentCouple> serv_comp_list,
	float version,
	int service_type
):
	serv_comp_list(serv_comp_list),
	version(version),
	service_type(service_type)
{
	this->comp_count = this->serv_comp_list.size();
	this->version_in_str = boost::lexical_cast<std::string>(this->version);
}

std::vector<ServiceComponentCouple> TransientMapRecord::get_service_comp_obj()
{
	return this->serv_comp_list;
}

float TransientMapRecord::get_version()
{
	this->version = boost::lexical_cast<float>(this->version_in_str);
	return this->version;
}

int TransientMapRecord::get_service_type()
{
	return this->service_type;
}

int TransientMapRecord::get_type()
{
	return TRANSIENT_MAP;
}

void TransientMapRecord::xserialize(cool::ObjectIoStream<>& stream)
{
	std::vector<ServiceComponentCouple>::iterator it;
	stream.readWrite(this->comp_count);
	if(this->serv_comp_list.empty())
	{
		this->serv_comp_list.resize(this->comp_count);	 //TODO: Use a MACRO for no_of_components
		it = this->serv_comp_list.begin();
		for(int32_t i = 1; i <= comp_count; i++)
		{
			(*it).xserialize(stream);
			it++;
		}
	}
	else
	{
		it = this->serv_comp_list.begin();
		for (; it != this->serv_comp_list.end(); ++it)
		{
			(*it).xserialize(stream);
		}
	}
	stream.readWrite(this->version_in_str);
	stream.readWrite(this->service_type);
}

ComponentInfoRecord & ServiceComponentCouple::get_component_info_record()
{
	return this->comp_info_obj;
}

int32_t ServiceComponentCouple::get_component()
{
	return this->component;
}

/*GLSnapshotHeader::GLSnapshotHeader(uint64_t latest_state_change_id):version(1.0),latest_state_change_id(latest_state_change_id)
{
}

GLSnapshotHeader::~GLSnapshotHeader()
{
}

int GLSnapshotHeader::get_type()
{
	return GL_SNAPSHOT_RECORD;
}

void GLSnapshotHeader::xserialize(cool::ObjectIoStream<>& stream)
{
	stream.readWrite(this->version);
	switch(version == 1.0)
	{
		case 1.0:
			stream.readWrite(this->latest_state_change_id);
			break;
		default:
			break;
	}
}*/

GLCheckpointHeader::GLCheckpointHeader(std::vector<std::string> state_change_ids): version(1), id_list(state_change_ids)
{
}

void GLCheckpointHeader::xserialize(cool::ObjectIoStream<>& stream)
{
	stream.readWrite(this->version);
	switch(version)
	{
		case 1:
			stream.readWrite(this->id_list);
			break;
		default:
			break;
	}
}

int GLCheckpointHeader::get_type()
{
	return GL_CHECKPOINT_RECORD;
}

GLCheckpointHeader::~GLCheckpointHeader()
{
}

std::vector<std::string> GLCheckpointHeader::get_id_list()
{
	return this->id_list;
}

RecoveryRecord::RecoveryRecord(
	int operation, std::string source, int status
):
	operation(operation),
	state_change_node(source),	//This is service id, not node id
	status(status)
{

}

void RecoveryRecord::clear_planned_map()
{
	this->planned_map.clear();
}

RecoveryRecord::RecoveryRecord(
	int operation,
	std::list<std::string> nodes_being_added,
	int status
):
	operation(operation),
	status(status)
{
	std::list<std::string>::iterator it = nodes_being_added.begin();
	for(; it != nodes_being_added.end(); ++it)
		this->state_change_node.append(*it);
}

/*std::string RecoveryRecord::get_keyname_for_sci_map()
{
	std::string complete_name;
	if (operation == 1 and !this->nodes_being_added.empty())	//Multiple nodes are being added
	{
		std::list<std::string>::iterator it = this->nodes_being_added.begin();
		for(; it != this->nodes_being_added.end(); ++it)
			complete_name.append(*it);
	}
	else
	{
		complete_name = this->source;
	}
	return complete_name;
}*/

//This returns service id
std::string RecoveryRecord::get_state_change_node()
{
	return this->state_change_node;
}

void RecoveryRecord::set_status(int status)
{
	this->status = status;
}

int RecoveryRecord::get_type()
{
	return GL_RECOVERY_RECORD;
}

int RecoveryRecord::get_operation()
{
	return this->operation;
}

void RecoveryRecord::set_planned_map(std::vector<std::pair<int32_t, ComponentInfoRecord> > planned_map)
{
	this->planned_map = planned_map;
}

std::vector<std::pair<int32_t, ComponentInfoRecord> > RecoveryRecord::get_planned_map()
{
	return this->planned_map;
}

void RecoveryRecord::remove_from_planned_map(std::vector<std::pair<int32_t, ComponentInfoRecord> > moved_components)
{
	OSDLOG(INFO, "Before erasing elements, planned map size is: " << this->planned_map.size());
	std::vector<std::pair<int32_t, ComponentInfoRecord> >::iterator it1 = this->planned_map.begin();
	std::vector<std::pair<int32_t, ComponentInfoRecord> >::iterator it2 = moved_components.begin();
	bool increment_vec = true;
	for( it1 = this->planned_map.begin(); it1 != this->planned_map.end(); )
	{
		for ( it2 = moved_components.begin(); it2 != moved_components.end(); )
		{
			if ((*it1).first == (*it2).first and (*it1).second == (*it2).second)
			{
				it1 = this->planned_map.erase(it1);
				it2 = moved_components.erase(it2);
				increment_vec = false;
				break;
			}
			else
			{
				++it2;
			}
		}
		if(increment_vec)
		{
			++it1;
		}
		increment_vec = true;
	}
	OSDLOG(INFO, "After erasing elements, planned map size is: " << this->planned_map.size());
}

void RecoveryRecord::xserialize(cool::ObjectIoStream<>& stream)
{
	stream.readWrite(this->operation);
	stream.readWrite(this->state_change_node);
	stream.readWrite(this->status);
	stream.readWrite(this->destination);
}

/* Returns Recovery Process destination node */
std::string RecoveryRecord::get_destination_node()
{
	return this->destination;
}

int RecoveryRecord::get_status()
{
	return this->status;
}

void RecoveryRecord::set_destination(std::string destination)
{
	this->destination = destination;
}

void RecoveryRecord::set_operation(int operation)
{
	this->operation = operation;
}

ElectionRecord::ElectionRecord(std::string node_id, int connection_count)
{
	this->node_id = node_id;
	this->connection_count = connection_count;
}

ElectionRecord::ElectionRecord()
{
}

std::string ElectionRecord::get_node_id()
{
	return this->node_id;
}

int ElectionRecord::get_connection_count()
{
	return this->connection_count;
}

void ElectionRecord::xserialize(cool::ObjectIoStream<>& stream)
{
	stream.readWrite(this->node_id);
	stream.readWrite(this->connection_count);
}

int ElectionRecord::get_type()
{
	return ELECTION_RECORD;
}

GLMountInfoRecord::GLMountInfoRecord(
	std::string failed_node,
	std::string destination_node
):
	failed_node(failed_node),
	destination_node(destination_node)
{
}

GLMountInfoRecord::GLMountInfoRecord()
{
}

std::string GLMountInfoRecord::get_failed_node_id()
{
	return this->failed_node;
}

std::string GLMountInfoRecord::get_destination_node_id()
{
	return this->destination_node;
}

int GLMountInfoRecord::get_type()
{
	return GL_MOUNT_INFO_RECORD;
}

void GLMountInfoRecord::xserialize(cool::ObjectIoStream<>& stream)
{
	stream.readWrite(this->failed_node);
	stream.readWrite(this->destination_node);
}

/*FinalRecoveryRecord::FinalRecoveryRecord(
	std::string service_id,
	boost::shared_ptr<RecoveryRecord> rec_record
):
	service_id(service_id),
	rec_record(rec_record)
{
}

int FinalRecoveryRecord::get_type()
{
	return GL_FINAL_RECOVERY_RECORD;
}

void FinalRecoveryRecord::xserialize(cool::ObjectIoStream<>& stream)
{
	stream.readWrite(this->service_id);
	this->rec_record->xserialize(stream);
}

boost::shared_ptr<RecoveryRecord> FinalRecoveryRecord::get_recovery_record()
{
	return this->rec_record;
}

std::string FinalRecoveryRecord::get_service_id()
{
	return this->service_id;
}*/


} // namespace recordStructure
