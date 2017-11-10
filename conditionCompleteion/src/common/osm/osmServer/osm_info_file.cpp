#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>

#include <cool/debug.h>
#include "osm/osmServer/osm_info_file.h"
#include "cool/cool.h"

#include "libraryUtils/object_storage_log_setup.h"
#include "libraryUtils/object_storage_exception.h"

using namespace object_storage_log_setup;
using namespace OSDException;

namespace osm_info_file_manager
{


NodeInfoFileReadHandler::NodeInfoFileReadHandler(
	std::string path,
        boost::function<void(recordStructure::NodeInfoFileRecord)> push_node_info_file_records_into_memory
	)
	: path(path), 
	push_node_info_file_records_into_memory(push_node_info_file_records_into_memory),
	deserializor(new serializor_manager::Deserializor())
{
	this->recovery_initializer = "COMPONENT_BALANCING";
	this->buffer.resize(MAX_READ_CHUNK_SIZE);
        this->cur_buffer_ptr = &this->buffer[0];
}

NodeInfoFileReadHandler::NodeInfoFileReadHandler(
	std::string path,
	boost::shared_ptr<gl_monitoring::Monitoring> monitoring
	)
	: path(path), monitoring(monitoring), deserializor(new serializor_manager::Deserializor())
{
	this->recovery_initializer = "GL_MONITORING";
	this->buffer.resize(MAX_READ_CHUNK_SIZE);
        this->cur_buffer_ptr = &this->buffer[0];
}

NodeInfoFileReadHandler::NodeInfoFileReadHandler(
	std::string path
	)
	: path(path), deserializor(new serializor_manager::Deserializor())
{
	this->buffer.resize(MAX_READ_CHUNK_SIZE);
        this->cur_buffer_ptr = &this->buffer[0];
}

NodeInfoFileReadHandler::~NodeInfoFileReadHandler()
{
	if (this->file_handler != NULL)
	{
		this->file_handler->close();
	}
}

bool NodeInfoFileReadHandler::init()
/*
*@desc: initializes the file handler for Node Info File
*
*@return: true if file is opened successfully in Read mode, false otherwise. 
*/
{
        //OSDLOG(INFO, "Initializing Node Info File Handler");
        try
        {
                this->file_handler.reset(new fileHandler::FileHandler(make_info_file_path(NODE_INFO_FILE, this->path), fileHandler::OPEN_FOR_READ));
        }
	catch(OSDException::FileNotFoundException& e)
	{
		OSDLOG(ERROR, "Node Info file Not Found");
		return false;
	}
        catch(OSDException::OpenFileException& e)
        {
                OSDLOG(ERROR, "Unable to open Node Info File");
                return false;
        }
        return true;
}

bool NodeInfoFileReadHandler::prepare_read_record()
/*
*@desc: For recovering from Node Info File, file handler is set topoint to the beginning of the file and buffer is initialized.
*
*@return: True is file handler is initialized and pre-requirements are satisfied for reading records, false otherwise.
*/
{
        if (this->init()) //initialize file handler
        {
                this->buffer.resize(MAX_READ_CHUNK_SIZE);
                this->file_handler->seek(0); //while recovering, point to the start of the file, initially.
                this->cur_buffer_ptr = &this->buffer[0];
                return true;
        }
        return false;
}

// @public interface
bool NodeInfoFileReadHandler::recover_record() //Entry point for Recovery from Node Info File.
/*
*@desc: buffer is prepared and populated with file contents and records are read and recovered. 
*
*@return: true if record is recovered successfully, false otherwise. 
*/
{
//      OSDLOG(INFO, "Starting Reading Node Info File");
        if (this->prepare_read_record())
        {
                this->file_handler->read(&this->buffer[0], MAX_READ_CHUNK_SIZE);
                this->read_record();
                return true;
        }
        return false;
}

void NodeInfoFileReadHandler::read_record()
/*
*@desc: records are read from the buffer and forwarded to recover_info_file_record() interface for appt. recovery handling
*/
{
        boost::shared_ptr<recordStructure::Record> record;
        while (true) //fetch next record until the record size/record is not NULL (i.e. till last record)
        {
                record = this->get_next_record(); //actually fetching the next record
                if (record == NULL)
                {
                        OSDLOG(DEBUG, "Last record read, Record is NULL");
                        break; //Indicates last record has been read
                }
                else
                {
                        this->recover_info_file_record(record);
                }
        }
        this->file_handler->close();
        OSDLOG(DEBUG, "Node Info File Reading/Recovery complete");
}

void NodeInfoFileReadHandler::recover_info_file_record(boost::shared_ptr<recordStructure::Record> record)
/*
*@desc: on the basis of the caller, it is decided what recovery process is to be carried. If recovery is being initiated from component_balancing, the records are populated in the memory maintained by component balancer. If recvovery is being initiated from monitoring, the recrd entries are added in the monitoring table.
*
*@param: record which is to be recovered
*/
{
	boost::shared_ptr<recordStructure::NodeInfoFileRecord> node_info_file_rcrd = 
		boost::dynamic_pointer_cast<recordStructure::NodeInfoFileRecord>(record);
	if (this->recovery_initializer == "GL_MONITORING")
        {
		this->monitoring->add_entries_while_recovery(node_info_file_rcrd->get_node_id(), 
						gl_monitoring::STRM, node_info_file_rcrd->get_node_status());
        }
        else if (this->recovery_initializer == "COMPONENT_BALANCING")
        {
		this->push_node_info_file_records_into_memory(*node_info_file_rcrd);
        }
	else
	{
		OSDLOG(ERROR, "Node Info File Records not Recovered");
	}
}

boost::shared_ptr<recordStructure::NodeInfoFileRecord> NodeInfoFileReadHandler::get_record_from_id(std::string id)
/*
*@desc: used to find node info file record from the node is passed
*
*@param: node id for which record is to be found
*
*@return: node info file record
*/
{
	OSDLOG(INFO, "Finding " << id << " in Node Info File");
	boost::shared_ptr<recordStructure::NodeInfoFileRecord> return_record;
        if (this->prepare_read_record())
        {
                this->file_handler->read(&this->buffer[0], MAX_READ_CHUNK_SIZE);
                boost::shared_ptr<recordStructure::Record> record;
                while (true)
                {
                        record = this->get_next_record();
                        if (!record)
                        {
                                OSDLOG(DEBUG, "Last record already read, Record is NULL");
                                break; //Indicates last record has been read
                        }
                        else
                        {
				boost::shared_ptr<recordStructure::NodeInfoFileRecord> node_info_file_rcrd =
                                                        boost::dynamic_pointer_cast<recordStructure::NodeInfoFileRecord>(record);
                                if (node_info_file_rcrd->get_node_id() == id)
                                {
					OSDLOG(INFO, "Info Record found for ID: " << id);
					return_record = node_info_file_rcrd;
					break;
                                }
			}
		}
	}
	return return_record;
}

std::list<std::string> NodeInfoFileReadHandler::list_node_ids()
/*
*@desc: interface used to prepare a list of all the nodes(node IDs) in the cluster
*
*@return: list of node ids
*/
{
        std::list<std::string> return_list;
	std::list<std::pair<std::string, int> > node_status_map;
        OSDLOG(DEBUG, "Reading Node Info File");           
        if (this->prepare_read_record())
        {
                this->file_handler->read(&this->buffer[0], MAX_READ_CHUNK_SIZE);
                this->prepare_list(return_list, node_status_map);
                return return_list;
        }
        OSDLOG(ERROR, "Unable to list node_ids");
        return return_list;
}

std::list<std::pair<std::string, int> > NodeInfoFileReadHandler::get_node_status_map()
/*
*@desc: interface used to prepare a list of all the nodes(node IDs) in the cluster
*
*@return: list of node ids with its status
*/
{
        std::list<std::string> return_list;
	std::list<std::pair<std::string, int> > node_status_map;
        if (this->prepare_read_record())
        {
                this->file_handler->read(&this->buffer[0], MAX_READ_CHUNK_SIZE);
                this->prepare_list(return_list, node_status_map);
		return node_status_map;
        }
	OSDLOG(ERROR, "Unable to prepare node status map");
        return node_status_map;
}


void NodeInfoFileReadHandler::prepare_list(
	std::list<std::string> &list_of_node_ids,
	std::list<std::pair<std::string, int> > &node_status_map
)
/*
*@desc: helper function to prepare list of node ids
*
@param: reference of list of node is and list of node status map
*/
{
	boost::shared_ptr<recordStructure::Record> record;
	while (true) //fetch next record until the record size/record is not NULL (i.e. till last record)
        {
                record = this->get_next_record(); //actually fetching the next record
                if (!record)
                {
                        OSDLOG(DEBUG, "Last record prepared, Record is NULL");
                        break; //Indicates last record has been read
                }
                else
                {
                        boost::shared_ptr<recordStructure::NodeInfoFileRecord> node_info_file_rcrd =
                                        boost::dynamic_pointer_cast<recordStructure::NodeInfoFileRecord>(record);
			//TODO:SANCHIT - Check status and push_back only healthy nodes in cluster
                        list_of_node_ids.push_back(node_info_file_rcrd->get_node_id());
			node_status_map.push_back(std::make_pair(node_info_file_rcrd->get_node_id(), node_info_file_rcrd->get_node_status()));
                }
        }
        this->file_handler->close();
        OSDLOG(INFO, "Node Info File Reading/Recovery complete");
}

boost::shared_ptr<recordStructure::Record> NodeInfoFileReadHandler::get_next_record()
/*
*@desc: helper function to deserialize next node info file record from buffer.
*
*@param: deserialized record
*/
{
        recordStructure::Record * record;
        int64_t record_size = this->deserializor->get_osm_info_file_record_size(this->cur_buffer_ptr);
        if (record_size <= 0)
        {
        	boost::shared_ptr<recordStructure::Record> return_record;
                return return_record; //record return will be NULL. Exit condition for while loop
        }
        try
        {
	        record = this->deserializor->osm_info_file_deserialize(this->cur_buffer_ptr, record_size);
        }
        catch(...)
        {
                OSDLOG(ERROR, "Invalid Node Info File Record (NULL)");
        	boost::shared_ptr<recordStructure::Record> return_record;
                return return_record; //record return will be NULL. Exit condition for while loop
        }
        this->cur_buffer_ptr += record_size;
        boost::shared_ptr<recordStructure::Record> return_record(record);
        return return_record;
}



ComponentInfoFileReadHandler::ComponentInfoFileReadHandler(
	std::string path
	)
	: path(path)
{
	this->deserializor.reset(new serializor_manager::Deserializor());
        this->buffer.resize(MAX_READ_CHUNK_SIZE);
        this->cur_buffer_ptr = &this->buffer[0];
}

ComponentInfoFileReadHandler::~ComponentInfoFileReadHandler()
{
	if (this->file_handler != NULL)
	{
		this->file_handler->close();
	}
}

bool ComponentInfoFileReadHandler::init(int infoFileType)
/*
*@desc: initializes the file handler for Component Info File
*
*@return: true if file is opened successfully in Read mode, false otherwise. 
*/
{
        try
        {
                this->file_handler.reset(new fileHandler::FileHandler(make_info_file_path(infoFileType, this->path), fileHandler::OPEN_FOR_READ));
        }
        catch(...)
        {
                OSDLOG(ERROR, "Unable to open component info file");
                return false;
        }
        return true;
}

bool ComponentInfoFileReadHandler::prepare_read_record(int infoFileType)
/*
@desc: For reading from Component Info File, file handler is set to point to the beginning of the file and buffer is initialized.
*
*@return: True is file handler is initialized and pre-requirements are satisfied for reading records, false otherwise.
*/
{
        if (this->init(infoFileType)) //initialize file handler
        {
                this->buffer.resize(MAX_READ_CHUNK_SIZE);
                this->file_handler->seek(0); //while recovering, point to the start of the file, initially.
                this->cur_buffer_ptr = &this->buffer[0];
                return true;
        }
        return false;
}

bool ComponentInfoFileReadHandler::recover_distributed_service_record(
		int infoFileType, 
		boost::shared_ptr<recordStructure::CompleteComponentRecord> &complete_component_info_file_rcrd
		)
{
	std::vector<recordStructure::ComponentInfoRecord> component_info_file_rcrd_list;
	if (this->recover_record(infoFileType, complete_component_info_file_rcrd, component_info_file_rcrd_list))
	{
		return true;
	}
	return false;
}

bool ComponentInfoFileReadHandler::recover_object_service_record(
		int infoFileType,
		std::vector<recordStructure::ComponentInfoRecord> &component_info_file_rcrd_list
		)
{
	boost::shared_ptr<recordStructure::CompleteComponentRecord> complete_component_info_file_rcrd(new recordStructure::CompleteComponentRecord);
	if (this->recover_record(infoFileType, complete_component_info_file_rcrd, component_info_file_rcrd_list))
	{
		return true;
	}
	return false;
}

bool ComponentInfoFileReadHandler::recover_record(
		int infoFileType,
		boost::shared_ptr<recordStructure::CompleteComponentRecord> &complete_component_info_file_rcrd,
		std::vector<recordStructure::ComponentInfoRecord> &component_info_file_rcrd_list
		) //Entry point for Recovery from Node Info File.
/*
*@desc: buffer is prepared and populated with file contents and records are read and recovered. 
*
*@return: true if record is recovered successfully, false otherwise. 
*/
{
	OSDLOG(INFO, "Starting Reading Component Info File");
        if (this->prepare_read_record(infoFileType))
        {
                this->file_handler->read(&this->buffer[0], MAX_READ_CHUNK_SIZE);
                this->read_record(infoFileType, complete_component_info_file_rcrd, component_info_file_rcrd_list);
                return true;
        }
        return false;
}

void ComponentInfoFileReadHandler::read_record(
		int infoFileType,
		boost::shared_ptr<recordStructure::CompleteComponentRecord> &complete_component_info_file_rcrd,
		std::vector<recordStructure::ComponentInfoRecord> &component_info_file_rcrd_list
		)
/*
*@desc: records are read from the buffer and forwarded to recover_info_file_record() interface for appt. recovery handling
*
*@param: info file type, i.e., ACCOUNT_COMPONENT_INFO_FILE, CONTAINER_COMPONENT_INFO_FILE, ACCOUNTUPDATER_COMPONENT_INFO_FILE, OBJECT_COMPONENT_INFO_FILE 
*/
{
        boost::shared_ptr<recordStructure::Record> record;
        int record_count = 0;
        while (true) //fetch next record until the record size/record is not NULL (i.e. till last record)
        {
                record = this->get_next_record(); //actually fetching the next record
                if (!record)
                {
                        OSDLOG(DEBUG, "Last record already read, Record is NULL");
                        break; //Indicates last record has been read
                }
                else
                {
                        record_count += 1;
                        this->recover_info_file_record(infoFileType, record, complete_component_info_file_rcrd, component_info_file_rcrd_list);
                }
        }
        this->file_handler->close();
        OSDLOG(INFO, "Component Info File Reading/Recovery complete");       
}

void ComponentInfoFileReadHandler::recover_info_file_record(
		int infoFileType, 
		boost::shared_ptr<recordStructure::Record> record,
		boost::shared_ptr<recordStructure::CompleteComponentRecord> &complete_component_info_file_rcrd,
		std::vector<recordStructure::ComponentInfoRecord> &component_info_file_rcrd_list
		)
/*
*@desc: helper function for recovery of component info file records
*
*@param: info file type, i.e., ACCOUNT_COMPONENT_INFO_FILE, CONTAINER_COMPONENT_INFO_FILE, ACCOUNTUPDATER_COMPONENT_INFO_FILE, OBJECT_COMPONENT_INFO_FILE
*@param: record to be recovered
*/
{
	if (infoFileType == OBJECT_COMPONENT_INFO_FILE)
	{
		boost::shared_ptr<recordStructure::ComponentInfoRecord> component_info_file_rcrd = 
			boost::dynamic_pointer_cast<recordStructure::ComponentInfoRecord>(record);
		component_info_file_rcrd_list.push_back(*component_info_file_rcrd);
	}
	else
	{
		boost::shared_ptr<recordStructure::CompleteComponentRecord> complete_component_info_file_record =
			boost::dynamic_pointer_cast<recordStructure::CompleteComponentRecord>(record);
		complete_component_info_file_rcrd.reset(
				new recordStructure::CompleteComponentRecord(*complete_component_info_file_record)
				);
	}
}

boost::shared_ptr<recordStructure::Record> ComponentInfoFileReadHandler::get_next_record()
/*
*@desc: helper function to deserialize next component info file record from buffer.
*
*@param: deserialized record
*/
{
        recordStructure::Record * record;
        int64_t record_size = this->deserializor->get_osm_info_file_record_size(this->cur_buffer_ptr);
        if (record_size <= 0)
        {
        	boost::shared_ptr<recordStructure::Record> return_record;
                return return_record; //record return will be NULL. Exit condition for while loop
        }
        try
        {
        	record = this->deserializor->osm_info_file_deserialize(this->cur_buffer_ptr, record_size);
        }
        catch(...)
        {
                OSDLOG(ERROR, "Invalid Component Info File Record (NULL)");
        	boost::shared_ptr<recordStructure::Record> return_record;
                return return_record; //record return will be NULL. Exit condition for while loop
        }
        this->cur_buffer_ptr += record_size;
        boost::shared_ptr<recordStructure::Record> return_record(record);
        return return_record;
}


//---------------------Info File Writer--------------------------//


NodeInfoFileWriteHandler::NodeInfoFileWriteHandler(
	std::string path
):
	path(path),
	serializor(new serializor_manager::Serializor()),
        deserializor(new serializor_manager::Deserializor())
{
	this->max_file_length = 10487560;
}

NodeInfoFileWriteHandler::~NodeInfoFileWriteHandler()
{
	if(this->file_handler != NULL)
        {
                this->file_handler->close();
        }
        delete serializor;
}

ComponentInfoFileWriteHandler::ComponentInfoFileWriteHandler(
        std::string path
):
        path(path),
	serializor(new serializor_manager::Serializor()),
        deserializor(new serializor_manager::Deserializor())
{
        this->max_file_length = 10487560;
}

ComponentInfoFileWriteHandler::~ComponentInfoFileWriteHandler()
{
	if(this->file_handler != NULL)
        {
                this->file_handler->close();
        }
        delete serializor;
}

bool NodeInfoFileWriteHandler::init()
/*
*@desc: initializes the file handler for Node Info File
*
*@return: true if file is opened successfully in APPEND mode, false otherwise. 
*/
{
	bool is_initialized = false;
	try
	{
		this->file_handler.reset(new fileHandler::FileHandler(make_info_file_path(NODE_INFO_FILE, this->path),
												 fileHandler::OPEN_FOR_APPEND));
		is_initialized = true;
	}
	catch(...)
	{
		OSDLOG(ERROR, "Problem opening Node Info File");         //return false
		is_initialized = false;
	}
	return is_initialized;
}

bool NodeInfoFileWriteHandler::add_record(std::list<boost::shared_ptr<recordStructure::Record> > record_list)
/*
*@desc: Interface to add record in Node Info File
*
*@param: list of records to be added into Node Info File
*
*@return: true if record is added in file, false otherwise
*/
{
        boost::mutex::scoped_lock lock(this->mtx);
        if (!(this->init())) //initialize file handler
        {
                OSDLOG(ERROR, "Failed to Open Node Info File");
                return false;
        }
	std::list<boost::shared_ptr<recordStructure::Record> >::iterator it_node_records = record_list.begin();
	for (; it_node_records != record_list.end(); ++it_node_records)
	{
		if (this->add(*it_node_records)) //actual call to the add interface
		{
			continue;
		}
		else
		{
			OSDLOG(ERROR, "Problem Adding Record in Node Info File");
			return false;
		}
	}
	bool status = this->file_handler->sync();
	return status;
}

bool NodeInfoFileWriteHandler::add(boost::shared_ptr<recordStructure::Record> rec)
/*
*@desc: helper function to add record in Node info File
*
*@param: record to be added into file
*
*@return: true if record is added in file, false otherwise
*/
{
	recordStructure::Record * record = rec.get();
        int64_t current_offset = this->file_handler->seek_end(); //point to the end of the file
        int64_t record_size = this->serializor->get_osm_info_file_serialized_size(record);
        if (current_offset == 0)
        {
                OSDLOG(DEBUG, " Node Info File Empty! Adding record at offset: " << current_offset);
        }
        else if(((this->max_file_length - current_offset) < record_size)) //assuming file size will never reach the max_file_length
                                                                          //Considering a max of 165 nodes in the cluster
        {
                OSDLOG(ERROR, "Unexpected Behaviour! No Space Left in file");
                //this->file_handler->close();
		return false;
        }
        this->process_write(record, record_size); //serialize and write call for file
        return true;
}

void NodeInfoFileWriteHandler::process_write(recordStructure::Record * record_ptr, int64_t record_size)
/*
*@desc: actual writing interface 
*
*@param: record to be serialized
*@param: record size to be serialized 
*/
{
        char *buffer = new char[record_size];
        memset(buffer, '\0', record_size);
        this->serializor->osm_info_file_serialize(record_ptr, buffer, record_size);
        this->file_handler->write(buffer, record_size);
        delete[] buffer;
}

bool NodeInfoFileWriteHandler::delete_update_helper(boost::shared_ptr<recordStructure::Record> record)
/*
*@desc: helper function for update and delete of record from node info file
*
*@param: record to be updated or deleted
*
*@return: true if deletion/updation is successfull, false otherwise
*/
{
        boost::mutex::scoped_lock lock(this->mtx);
        try
        {
                this->file_handler.reset(new fileHandler::FileHandler(make_temp_file_path(NODE_INFO_FILE, this->path), fileHandler::OPEN_FOR_APPEND));
        }
        catch(...)
        {
		OSDLOG(ERROR, "Failed to open Node Info File");
                return false;
        }
        this->add(record); //interface to serialize and add record into temp file
        return true;
}

void NodeInfoFileWriteHandler::rename_temp_file()
/*
*@desc: helper function for renaming temorary file to Node Info File
*/
{
        if(::rename(make_temp_file_path(NODE_INFO_FILE, this->path).c_str(),
                                        make_info_file_path(NODE_INFO_FILE, this->path).c_str()) == -1)
        {
                OSDLOG(ERROR, "Unable to rename file "<<make_temp_file_path(NODE_INFO_FILE, this->path)
                                                     <<" : "<<strerror(errno));
                throw OSDException::RenameFileException();
        }
}

//bool NodeInfoFileWriteHandler::update_record(boost::shared_ptr<recordStructure::Record> record, int comp_no)
bool NodeInfoFileWriteHandler::update_record(std::string node_id, int node_status)
/*
*@desc: Interface for updating Node info File record
*
*@param: node id for which record is to be updated
*@param: node status which is to be updated
*/
{
	//Check is record corresponding to updated Node ID exists in node info file 
	boost::shared_ptr<osm_info_file_manager::NodeInfoFileReadHandler> read_handler (new osm_info_file_manager::NodeInfoFileReadHandler(this->path)); //path
	std::list<std::string> node_id_list = read_handler->list_node_ids();
	if (std::find(node_id_list.begin(), node_id_list.end(), node_id) == node_id_list.end())
	{
		OSDLOG(ERROR, "Node Id: "<< node_id<<" to be updated does not exist.");
		return false;
	}
	this->buffer.resize(MAX_READ_CHUNK_SIZE);
	try
	{
        	this->read_file_handler.reset(new fileHandler::FileHandler(make_info_file_path(NODE_INFO_FILE, this->path), fileHandler::OPEN_FOR_READ));
	}
	catch(...)
	{
		OSDLOG(ERROR, "Unable to open file");
		return false;
	}
	this->readbuff.resize(MAX_READ_CHUNK_SIZE);
        this->read_file_handler->seek(0);
        this->read_file_handler->read(&this->readbuff[0], MAX_READ_CHUNK_SIZE);
        this->cur_read_buffer_ptr = &this->readbuff[0];
	std::string updated_node_id = node_id;
	int updated_status = node_status;
	
	while(true)
        {
                recordStructure::Record * exist_record = NULL;
                int64_t record_size = this->deserializor->get_osm_info_file_record_size(this->cur_read_buffer_ptr);
                if (record_size <= 0)
                {
                        OSDLOG(ERROR, "Invalid Record to be updated in Node Info File");
                        break;
                }
                exist_record = this->deserializor->osm_info_file_deserialize(this->cur_read_buffer_ptr, record_size);
                if (exist_record == NULL)
                {
                        OSDLOG(ERROR, "Record NULL. Exiting update interface");
                        break;
                }
		boost::shared_ptr<recordStructure::Record> existing_record(exist_record);
		boost::shared_ptr<recordStructure::NodeInfoFileRecord> node_info_file_rcrd = 
					boost::dynamic_pointer_cast<recordStructure::NodeInfoFileRecord>(existing_record);
	        if (updated_node_id == node_info_file_rcrd->get_node_id())
		{
			node_info_file_rcrd->set_node_status(updated_status); 
			if (this->delete_update_helper(node_info_file_rcrd))
			{
				this->cur_read_buffer_ptr += record_size;
				OSDLOG(INFO, "Record Updated with Node Id: "<<node_info_file_rcrd->get_node_id());
			}
		}
       		else
		{
			if (this->delete_update_helper(existing_record))
			{
				this->cur_read_buffer_ptr += record_size;
			}
		}
	}
	this->rename_temp_file();
	this->read_file_handler->close();
	return true;
}

bool NodeInfoFileWriteHandler::delete_record(std::string node_id)
/*
*@desc: Interface for deleting Node info File record
*
*@param: node id for which record is to be deleted
*
*@return: true if record is deleted, false otherwise
*/
{
	//Check is record corresponding to updated Node ID exists in node info file 
	boost::shared_ptr<osm_info_file_manager::NodeInfoFileReadHandler> read_handler (new osm_info_file_manager::NodeInfoFileReadHandler(this->path)); //path
	std::list<std::string> node_id_list = read_handler->list_node_ids();
	if (std::find(node_id_list.begin(), node_id_list.end(), node_id) == node_id_list.end())
	{
		OSDLOG(ERROR, "Node Id: "<<node_id<<" to be deleted does not exist.");
		return false;
	}
        boost::mutex::scoped_lock lock(this->mtx);
	try
	{
       		this->read_file_handler.reset(new fileHandler::FileHandler(make_info_file_path(NODE_INFO_FILE, this->path), fileHandler::OPEN_FOR_READ));
                this->file_handler.reset(new fileHandler::FileHandler(make_temp_file_path(NODE_INFO_FILE, this->path), fileHandler::OPEN_FOR_APPEND));
	}
	catch(...)
	{
		OSDLOG(ERROR, "Unable to open Node Info File");
		return false;
	}
        this->readbuff.resize(MAX_READ_CHUNK_SIZE);
        this->read_file_handler->seek(0);
        this->read_file_handler->read(&this->readbuff[0], MAX_READ_CHUNK_SIZE);
        this->cur_read_buffer_ptr = &this->readbuff[0];
        std::string deleted_node_id = node_id;
	
        while(true)
        {
                recordStructure::Record * exist_record = NULL;
                int64_t record_size = this->deserializor->get_osm_info_file_record_size(this->cur_read_buffer_ptr);
                if (record_size <= 0)
                {
                        OSDLOG(ERROR, "Invalid Record Size for deletion");
                        break;
                }
                try
		{
			exist_record = this->deserializor->osm_info_file_deserialize(this->cur_read_buffer_ptr, record_size);
		}
		catch(...)
		{
			OSDLOG(ERROR, "Invalid Record (NULL). Exiting delete interface");
                        break;
                }
		boost::shared_ptr<recordStructure::Record> existing_record(exist_record);
                boost::shared_ptr<recordStructure::NodeInfoFileRecord> node_info_file_rcrd =
                                        boost::dynamic_pointer_cast<recordStructure::NodeInfoFileRecord>(existing_record);
                if (deleted_node_id == node_info_file_rcrd->get_node_id())
                {
                        this->cur_read_buffer_ptr += record_size;
                        OSDLOG(INFO, "Record Deleted with Node Id: "<<node_info_file_rcrd->get_node_id());
                }
                else
                {
                        //this->delete_update_helper(existing_record);
                        if (this->add(existing_record))
			{
                        	this->cur_read_buffer_ptr += record_size;
			}
                }
        }
	this->rename_temp_file();
	this->read_file_handler->close();
        return true;
}

bool ComponentInfoFileWriteHandler::init(int infoFileType)
/*
*@desc: initializes the file handler for Component Info File
*
*@param: info file type, i.e., ACCOUNT_COMPONENT_INFO_FILE, CONTAINER_COMPONENT_INFO_FILE, ACCOUNTUPDATER_COMPONENT_INFO_FILE, OBJECT_COMPONENT_INFO_FILE
*
*@return: true if file is opened successfully in APPEND mode, false otherwise. 
*/
{
	bool is_initialized = false;
	try
	{
		if (infoFileType == ACCOUNT_COMPONENT_INFO_FILE || infoFileType == CONTAINER_COMPONENT_INFO_FILE || infoFileType == ACCOUNTUPDATER_COMPONENT_INFO_FILE)
		{
			//open temp, write and rename
			this->file_handler.reset(new fileHandler::FileHandler(make_temp_file_path(infoFileType, this->path), fileHandler::OPEN_FOR_APPEND));
			is_initialized = true;
		}
		else if (infoFileType == OBJECT_COMPONENT_INFO_FILE)
		{
			this->file_handler.reset(new fileHandler::FileHandler(make_info_file_path(infoFileType, this->path), fileHandler::OPEN_FOR_APPEND));
			is_initialized = true;
		}
		else
		{
	                is_initialized = false;
		}
	}
	catch(...)
	{
		OSDLOG(ERROR, "Problem opening Component Info File");
		is_initialized = false;
	}
	return is_initialized;

}

bool ComponentInfoFileWriteHandler::add(boost::shared_ptr<recordStructure::Record> rec)
/*
*@desc: helper function to add record in Component InfoFile
*
*@param: record to be added into component info file 
*
*@return: true, if record is successfully added into file, false otherwise
*/
{
	recordStructure::Record * record = rec.get();
        int64_t current_offset = this->file_handler->seek_end(); //point to the end of the file
        int64_t record_size = this->serializor->get_osm_info_file_serialized_size(record);
        if (current_offset == 0)
        {
                OSDLOG(INFO, "File Empty! Adding record at offset: " << current_offset);
        }
        else if(((this->max_file_length - current_offset) < record_size)) //assuming file size will never reach the max_file_length
                                                                          //Considering a max of 165 nodes in the cluster
        {
                OSDLOG(ERROR, "Unexpected Behaviour! No Space Left in file");
                //this->file_handler->close();
		return false;
        }
        this->process_write(record, record_size); //serialize and write call for file
        return true;
}

bool ComponentInfoFileWriteHandler::add_record(int infoFileType, boost::shared_ptr<recordStructure::Record> record) //CompleteComponentRecord
/*
*@desc: Interface to add record in Component Info File
*
*@param: info file type, i.e., ACCOUNT_COMPONENT_INFO_FILE, CONTAINER_COMPONENT_INFO_FILE, ACCOUNTUPDATER_COMPONENT_INFO_FILE, OBJECT_COMPONENT_INFO_FILE
*@param: record to be added into Node Info File
*
*@return: true if record is added in file, false otherwise
*/
{
	bool is_record_added = false;
	if (infoFileType == ACCOUNT_COMPONENT_INFO_FILE || infoFileType == CONTAINER_COMPONENT_INFO_FILE || infoFileType == ACCOUNTUPDATER_COMPONENT_INFO_FILE || infoFileType == OBJECT_COMPONENT_INFO_FILE)
        {
	        boost::mutex::scoped_lock lock(this->mtx);
        	if (!(this->init(infoFileType))) //initialize file handler
	        {
        	        OSDLOG(ERROR, "Failed to Open Component Info File");
                	is_record_added = false;
			return is_record_added;
	        }
		if (this->add(record)) //actual call to the add interface
		{
			is_record_added = true;
			if (infoFileType == ACCOUNT_COMPONENT_INFO_FILE || infoFileType == CONTAINER_COMPONENT_INFO_FILE || infoFileType == ACCOUNTUPDATER_COMPONENT_INFO_FILE)
			{
				this->rename_temp_file(infoFileType);
			}
			return is_record_added;
		}
		else
		{
			OSDLOG(ERROR, "Problem Adding Record");
			is_record_added = false;
		}

	}
	return is_record_added;
}

void ComponentInfoFileWriteHandler::process_write(recordStructure::Record * record_ptr, int64_t record_size)
/*
*@desc: actual writing interface 
*
*@param: record to be serialized
*@param: record size to be serialized 
*/
{
        char *buffer = new char[record_size];
        memset(buffer, '\0', record_size);
        this->serializor->osm_info_file_serialize(record_ptr, buffer, record_size);
        this->file_handler->write(buffer, record_size);
        delete[] buffer;
}

bool ComponentInfoFileWriteHandler::delete_update_helper(int infoFileType, boost::shared_ptr<recordStructure::Record> record)
/*
*@desc: helper function for update and delete of record from component info file
*
*@param: record to be updated or deleted
*
*@return: true if deletion/updation is successfull, false otherwise
*/
{
        boost::mutex::scoped_lock lock(this->mtx);
        try
        {
                this->file_handler.reset(new fileHandler::FileHandler(make_temp_file_path(infoFileType, this->path), fileHandler::OPEN_FOR_APPEND));
        }
        catch(...)
        {
		OSDLOG(ERROR, "Failed to open temporary Component Info File");
                return false;
        }
        this->add(record); //interface to serialize and add record into temp file
        return true;
}

bool ComponentInfoFileWriteHandler::update_record(int infoFileType, boost::shared_ptr<recordStructure::Record> record)
/*
*@desc: Interface for updating Component info File record
*
*@param: info file type, i.e., ACCOUNT_COMPONENT_INFO_FILE, CONTAINER_COMPONENT_INFO_FILE, ACCOUNTUPDATER_COMPONENT_INFO_FILE, OBJECT_COMPONENT_INFO_FILE
*@param: record which is to be updated
*
*@return: true if record is updated, false otherwise
*/
{
	if (infoFileType != OBJECT_COMPONENT_INFO_FILE)
	{
		return false;
	}
	try
	{
        	this->read_file_handler.reset(new fileHandler::FileHandler(make_info_file_path(infoFileType, this->path), fileHandler::OPEN_FOR_READ));
	}
	catch(...)
	{
		OSDLOG(ERROR, "Unable to open Component Info file");
		return false;
	}
 
        this->readbuff.resize(MAX_READ_CHUNK_SIZE);
        this->read_file_handler->seek(0);
        this->read_file_handler->read(&this->readbuff[0], MAX_READ_CHUNK_SIZE);
        this->cur_read_buffer_ptr = &this->readbuff[0];
        boost::shared_ptr<recordStructure::ComponentInfoRecord> updated_record = 
			boost::dynamic_pointer_cast<recordStructure::ComponentInfoRecord>(record);
	std::string updated_service_id = updated_record->get_service_id();
        while(true)
        {
                recordStructure::Record * exist_record;
                int64_t record_size = this->deserializor->get_osm_info_file_record_size(this->cur_read_buffer_ptr);
                if (record_size <= 0)
                {
                        OSDLOG(ERROR, "Invalid Record for update");
                        break;
                }
		try
                {
	                exist_record = this->deserializor->osm_info_file_deserialize(this->cur_read_buffer_ptr, record_size);
                }
                catch(...)
                {
			OSDLOG(ERROR, "Invalid Record (NULL). Exiting update interface");
                        break;
                }
                boost::shared_ptr<recordStructure::Record> existing_record(exist_record);
                boost::shared_ptr<recordStructure::ComponentInfoRecord> component_info_file_rcrd =
                                        boost::dynamic_pointer_cast<recordStructure::ComponentInfoRecord>(existing_record);
                if (component_info_file_rcrd->get_service_id() == updated_service_id)
                {
			if (this->delete_update_helper(infoFileType, updated_record))
                        {
                                this->cur_read_buffer_ptr += record_size;
                                OSDLOG(INFO, "Record Updated for Service Id: "<<updated_service_id);
                        }
                }
                else
                {
			OSDLOG(DEBUG, " Record to be Update is not found");
                        if (this->delete_update_helper(infoFileType, existing_record))
			{
                        	this->cur_read_buffer_ptr += record_size;
			}
                }
        }
        this->rename_temp_file(infoFileType);
        this->read_file_handler->close();
	return true;
}

bool ComponentInfoFileWriteHandler::delete_record(int infoFileType, std::string deleted_service_id)
/*
*@desc: Interface for deleting Component info File record
*
*@param: info file type, i.e., ACCOUNT_COMPONENT_INFO_FILE, CONTAINER_COMPONENT_INFO_FILE, ACCOUNTUPDATER_COMPONENT_INFO_FILE, OBJECT_COMPONENT_INFO_FILE
*@param: record is to be deleted
*
*@return: true if record is deleted, false otherwise
*/
{ 
	//Required for deleting service info from object info file.
	if (infoFileType != OBJECT_COMPONENT_INFO_FILE)
	{
		OSDLOG(ERROR, "Delete not possible");
		return false;
	}
        boost::mutex::scoped_lock lock(this->mtx);
	OSDLOG(INFO, "Deleting Record from Object Component Info File");
	try
	{
        	this->read_file_handler.reset(new fileHandler::FileHandler(make_info_file_path(infoFileType, this->path), fileHandler::OPEN_FOR_READ));
                this->file_handler.reset(new fileHandler::FileHandler(make_temp_file_path(infoFileType, this->path), fileHandler::OPEN_FOR_CREATE));
	}
	catch(...)
	{
		OSDLOG(ERROR, "Unable to open file");
		return false;
	}
        this->readbuff.resize(MAX_READ_CHUNK_SIZE);
        this->read_file_handler->seek(0);
        this->read_file_handler->read(&this->readbuff[0], MAX_READ_CHUNK_SIZE);
        this->cur_read_buffer_ptr = &this->readbuff[0];
	bool deleted = false;
        while(true)
        {
                recordStructure::Record * exist_record;
                int64_t record_size = this->deserializor->get_osm_info_file_record_size(this->cur_read_buffer_ptr);
                if (record_size <= 0)
                {
                        OSDLOG(ERROR, "Invalid Record in delete of component InfoFile");
                        break;
                }
                try
		{
			exist_record = this->deserializor->osm_info_file_deserialize(this->cur_read_buffer_ptr, record_size);
		}
		catch(...)
		{
                        OSDLOG(ERROR, "Invalid Record (NULL). Exiting delete interface");
                        break;
                }
                boost::shared_ptr<recordStructure::Record> existing_record(exist_record);
                boost::shared_ptr<recordStructure::ComponentInfoRecord> component_info_file_rcrd =
                                        boost::dynamic_pointer_cast<recordStructure::ComponentInfoRecord>(existing_record);
                if (deleted_service_id == component_info_file_rcrd->get_service_id())
                {
                        this->cur_read_buffer_ptr += record_size;
                        OSDLOG(INFO, "Record Deleted with Service Id: "<<component_info_file_rcrd->get_service_id());
			deleted = true;
                }
                else
                {
                        //this->delete_update_helper(infoFileType, existing_record);
                        if (this->add(existing_record))
			{
                        	this->cur_read_buffer_ptr += record_size;
			}
                }
        }
	this->rename_temp_file(infoFileType);
	this->read_file_handler->close();
	this->file_handler->close();
	return deleted;
}

void ComponentInfoFileWriteHandler::rename_temp_file(int infoFileType)
/*
*@desc: interface to rename temp file to original info file
*
*@param: info file type, i.e., ACCOUNT_COMPONENT_INFO_FILE, CONTAINER_COMPONENT_INFO_FILE, ACCOUNTUPDATER_COMPONENT_INFO_FILE, OBJECT_COMPONENT_INFO_FILE
*/
{
	OSDLOG(INFO, "Info File Path name: "<< make_info_file_path(infoFileType, this->path));
	
        if(::rename(make_temp_file_path(infoFileType, this->path).c_str(),
                                        make_info_file_path(infoFileType, this->path).c_str()) == -1)
        {
                OSDLOG(ERROR, "Unable to rename file "<<make_temp_file_path(infoFileType, this->path)
                                                     <<" : "<<strerror(errno));
                throw OSDException::RenameFileException();
        }
}

std::string const make_info_file_path(int infoFileType, std::string const path)
/*
*@desc: function to prepare info file path
*
*@param: info file type, i.e., ACCOUNT_COMPONENT_INFO_FILE, CONTAINER_COMPONENT_INFO_FILE, ACCOUNTUPDATER_COMPONENT_INFO_FILE, OBJECT_COMPONENT_INFO_FILE
*@param: path of original info file location
*
*@return: path of info file
*/
{
        std::string return_path;
        switch (infoFileType)
        {
        case NODE_INFO_FILE:
                return_path = path + "/" + NODE_INFO_FILE_NAME;
                break;
        case ACCOUNT_COMPONENT_INFO_FILE:
                return_path = path + "/" + ACCOUNT_COMPONENT_INFO_FILE_NAME;
                break;
        case CONTAINER_COMPONENT_INFO_FILE:
                return_path = path + "/" + CONTAINER_COMPONENT_INFO_FILE_NAME;
                break;
        case ACCOUNTUPDATER_COMPONENT_INFO_FILE:
                return_path = path + "/" + ACCOUNTUPDATER_COMPONENT_INFO_FILE_NAME;
                break;
        case OBJECT_COMPONENT_INFO_FILE:
                return_path = path + "/" + OBJECT_COMPONENT_INFO_FILE_NAME;
                break;
        default:
                break;
        }
        return return_path;
}

std::string const make_temp_file_path(int infoFileType, std::string const path)
/*
*@desc: function to prepare temp info file path
*
*@param: info file type, i.e., ACCOUNT_COMPONENT_INFO_FILE, CONTAINER_COMPONENT_INFO_FILE, ACCOUNTUPDATER_COMPONENT_INFO_FILE, OBJECT_COMPONENT_INFO_FILE
*@param: path of original info file
*
*@return: path of temporary file
*/
{
        std::string temp_string = "temp";
        std::string return_temp_path;
        switch (infoFileType)
        {
        case NODE_INFO_FILE:
                return_temp_path = path + "/" + NODE_INFO_FILE_NAME + "_" +
                                                        boost::lexical_cast<std::string>(temp_string);
                break;
        case ACCOUNT_COMPONENT_INFO_FILE:
                return_temp_path = path + "/" + ACCOUNT_COMPONENT_INFO_FILE_NAME + "_" +
                                                        boost::lexical_cast<std::string>(temp_string);
                break;
        case CONTAINER_COMPONENT_INFO_FILE:
                return_temp_path = path + "/" + CONTAINER_COMPONENT_INFO_FILE_NAME + "_" +
                                                        boost::lexical_cast<std::string>(temp_string);
                break;
        case ACCOUNTUPDATER_COMPONENT_INFO_FILE:
                return_temp_path = path + "/" + ACCOUNTUPDATER_COMPONENT_INFO_FILE_NAME + "_" +
                                                        boost::lexical_cast<std::string>(temp_string);
                break;
        case OBJECT_COMPONENT_INFO_FILE:
                return_temp_path = path + "/" + OBJECT_COMPONENT_INFO_FILE_NAME + "_" +
                                                        boost::lexical_cast<std::string>(temp_string);
                break;
        default:
                break;
        }
        return return_temp_path;
}


} // namespace osm_info_file_manager
