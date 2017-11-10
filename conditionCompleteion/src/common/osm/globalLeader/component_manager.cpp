#include <math.h>
#include <boost/make_shared.hpp>
#include <boost/filesystem.hpp>

#include "communication/callback.h"
#include "osm/osmServer/osm_info_file.h"
#include "osm/globalLeader/component_manager.h"
#include "communication/message_interface_handlers.h"
#include "communication/message_type_enum.pb.h"

namespace component_manager
{

ComponentMapManager::ComponentMapManager(
	cool::SharedPtr<config_file::OSMConfigFileParser> parser_ptr
):
	global_map_version(1.0),
	transient_map_version(0.0)
{
	this->node_info_file_path = parser_ptr->node_info_file_pathValue();
	this->osm_info_file_path = parser_ptr->osm_info_file_pathValue();
	this->service_to_record_type_map_ptr.reset(new ServiceToRecordTypeMapper());
	ServiceToRecordTypeBuilder type_builder;
	type_builder.register_services(this->service_to_record_type_map_ptr);
}

void ComponentMapManager::get_map(
	std::map<std::string, std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> > &orig_basic_map,
	std::map<std::string, std::tr1::tuple<std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >, float> > &orig_transient_map,
	std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > > &orig_planned_map
	)
{
	boost::recursive_mutex::scoped_lock lock(this->mtx);
	orig_basic_map = this->basic_map;
	orig_transient_map = this->transient_map;
	orig_planned_map = this->planned_map;
}

size_t ComponentMapManager::get_transient_map_size()
{
	return this->transient_map.size();
}

std::map<std::string, std::tr1::tuple<std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >, float> >& ComponentMapManager::get_transient_map()
{
	boost::recursive_mutex::scoped_lock lock(this->mtx);
	return this->transient_map;
}

/* Called only during recovery from Journal */
void ComponentMapManager::set_transient_map_version(float version)
{
	if(version > this->transient_map_version)
		this->transient_map_version = version;
}

float ComponentMapManager::get_transient_map_version()
{
	return this->transient_map_version;
}

void ComponentMapManager::set_global_map_version(float version)
{
	boost::recursive_mutex::scoped_lock lock(this->mtx);
	this->global_map_version = version;
}

boost::shared_ptr<recordStructure::ComponentInfoRecord> ComponentMapManager::get_service_object(std::string service_id)
{
	boost::recursive_mutex::scoped_lock lock(this->mtx);
	GLUtils util_obj;
	std::string service_name = util_obj.get_service_name(service_id);
	boost::shared_ptr<recordStructure::ComponentInfoRecord> comp_info_record;
	bool found = false;
	std::map<std::string, std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> >::iterator it = this->basic_map.find(service_name);
	if(it != this->basic_map.end())
	{
		std::vector<recordStructure::ComponentInfoRecord> comp_list = std::tr1::get<0>(it->second);
		std::vector<recordStructure::ComponentInfoRecord>::iterator itr = comp_list.begin();
		for(; itr != comp_list.end(); ++itr)
		{
			if((*itr).get_service_id() == service_id)
			{
				//comp_info_record.reset(&(*itr));
				comp_info_record = boost::make_shared<recordStructure::ComponentInfoRecord>((*itr));
				found = true;
				break;
			}
		}
	}
	if(!found)
	{
		std::map<std::string, std::tr1::tuple<std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >, float> >::iterator it1 = this->transient_map.find(service_name);
		if(it1 != this->transient_map.end())
		{
			std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > comp_vec = std::tr1::get<0>(it1->second);
			std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >::iterator itr1 = comp_vec.begin();
			for(; itr1 != comp_vec.end(); ++itr1)
			{
				if((*itr1).second.get_service_id() == service_id)
				{
					//comp_info_record.reset(&((*itr1).second));
					comp_info_record = boost::make_shared<recordStructure::ComponentInfoRecord>((*itr1).second);
					break;
				}
			}
		}
	}
	return comp_info_record;
}

std::map<std::string, std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> >& ComponentMapManager::get_basic_map()
{
	boost::recursive_mutex::scoped_lock lock(this->mtx);
	return this->basic_map;
}

std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > >&  ComponentMapManager::get_planned_map()
{
	boost::recursive_mutex::scoped_lock lock(this->mtx);
	return this->planned_map;
}

/* it is actually converting transient map into record which can be flushed on journal */
std::vector<boost::shared_ptr<recordStructure::Record> > ComponentMapManager::convert_full_transient_map_into_journal_record()
{
	OSDLOG(DEBUG, "Preparing transient map to be written onto journal");
	std::map<std::string, std::tr1::tuple<std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >, float> >::iterator it = 
													this->transient_map.begin();

	std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >::iterator it1;
	std::vector<boost::shared_ptr<recordStructure::Record> > complete_tmap;
	std::vector<boost::shared_ptr<recordStructure::ServiceComponentCouple> > trans_mem_records;
	for ( ; it != this->transient_map.end(); ++it )
	{
		int type = this->service_to_record_type_map_ptr->get_type_for_service(it->first);
		complete_tmap.push_back(this->convert_tmap_into_journal_record(it->first, type));
	}
/*		for ( it1 = std::tr1::get<0>(it->second).begin(); it1 != std::tr1::get<0>(it->second).end(); ++it1 )
		{
			boost::shared_ptr<recordStructure::ServiceComponentCouple> transient_map(
						new recordStructure::ServiceComponentCouple(it1->first, it1->second, type));
			trans_mem_records.push_back(transient_map);
		}
	}
	return std::make_pair(trans_mem_records, it->second);
*/
	return complete_tmap;
}

/* it is actually converting transient map into record which can be flushed on journal (one service at a time) */
boost::shared_ptr<recordStructure::Record> ComponentMapManager::convert_tmap_into_journal_record(std::string service_name, int type)
{	
	boost::recursive_mutex::scoped_lock lock(this->mtx);
	OSDLOG(DEBUG, "Coverting Transient Map into Journal Record");
	std::map<std::string, std::tr1::tuple<std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >, float> >::iterator 
					it = this->transient_map.find(service_name);

	std::vector<recordStructure::ServiceComponentCouple> trans_mem_records;
	if ( it != this->transient_map.end() )
	{
		std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >::iterator it1 = std::tr1::get<0>(it->second).begin();
		for ( ; it1 != std::tr1::get<0>(it->second).end() ; ++it1 )
		{
			recordStructure::ServiceComponentCouple transient_map(it1->first, it1->second);
			trans_mem_records.push_back(transient_map);
		}
	}
	//boost::shared_ptr<recordStructure::Record> tmap_ptr(new recordStructure::TransientMapRecord(trans_mem_records, std::tr1::get<1>(it->second), type));
	boost::shared_ptr<recordStructure::Record> tmap_ptr(new recordStructure::TransientMapRecord(trans_mem_records, this->transient_map_version, type));
	OSDLOG(INFO, "Transient Map version while writing on journal " << this->transient_map_version);
	return tmap_ptr;
}

void ComponentMapManager::update_object_component_info_file(
	std::vector<std::string> nodes_registered,
	std::vector<std::string> nodes_failed
)
{
	std::string object_comp_info_file = this->osm_info_file_path + "/" + osm_info_file_manager::OBJECT_COMPONENT_INFO_FILE_NAME;
//	boost::filesystem::wpath file(object_comp_info_file);
	try
	{
		if(boost::filesystem::exists(object_comp_info_file))
			boost::filesystem::remove(object_comp_info_file);
	}
	catch(boost::filesystem::filesystem_error &ex)
	{
		OSDLOG(ERROR, "Filesystem not accessible: " << ex.what());
		PASSERT(false, "Unable to delete object component file");
	}
	nodes_registered.insert( nodes_registered.end(), nodes_failed.begin(), nodes_failed.end() );
	std::vector<std::string>::iterator it = nodes_registered.begin();
	for (; it != nodes_registered.end(); ++it)
	{
		std::string service_id = *it + "_" + OBJECT_SERVICE;
		this->add_object_entry_in_basic_map(OBJECT_SERVICE, service_id);
	}
}

void ComponentMapManager::recover_from_component_info_file()
{
	OSDLOG(INFO, "Recovering from Component Info files");
//	this->recover_components_for_service(OBJECT_SERVICE);	//This will be re-created from entries in node info file records during recovery
	this->recover_components_for_service(CONTAINER_SERVICE);
	this->recover_components_for_service(ACCOUNT_SERVICE);
	this->recover_components_for_service(ACCOUNT_UPDATER);
	OSDLOG(INFO, "Recovered from Component Info Files, basic map size is " << this->basic_map.size());
	std::map<std::string, std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> >::iterator it1 = this->basic_map.find(ACCOUNT_UPDATER);
	OSDLOG(DEBUG, "Size of comp list for AU is " << std::tr1::get<0>(it1->second).size());
	OSDLOG(INFO, "Final Global Map version recovered from Component Info Files is: " << this->global_map_version);

}

void ComponentMapManager::recover_components_for_service(std::string service_name)
{
	OSDLOG(INFO, "Recovering Component for "<<service_name);
	osm_info_file_manager::ComponentInfoFileReadHandler hndlr(this->osm_info_file_path);
	int type = this->service_to_record_type_map_ptr->get_type_for_service(service_name);
	if(service_name == CONTAINER_SERVICE or service_name == ACCOUNT_SERVICE or service_name == ACCOUNT_UPDATER)
	{
		boost::shared_ptr<recordStructure::CompleteComponentRecord> complete_component_info_file_rcrd;
		bool read_status = hndlr.recover_distributed_service_record(type, complete_component_info_file_rcrd);
		if(read_status)
		{
			std::vector<recordStructure::ComponentInfoRecord> vec = complete_component_info_file_rcrd->get_component_lists();
			OSDLOG(INFO, "Component list found from Component Info file for service " << service_name << " is " << vec.size());
			float service_version = complete_component_info_file_rcrd->get_service_version();
			this->basic_map.insert(std::make_pair(service_name, std::tr1::make_tuple(vec, service_version)));
			OSDLOG(DEBUG, "Global Map version recovered from component info file of " << service_name << " is: " << complete_component_info_file_rcrd->get_gl_version());
			if(this->global_map_version < complete_component_info_file_rcrd->get_gl_version())
				this->global_map_version = complete_component_info_file_rcrd->get_gl_version();
		}
	}
	else if(service_name == OBJECT_SERVICE)
	{
		std::vector<recordStructure::ComponentInfoRecord> component_info_file_rcrd_list;
		bool read_status = hndlr.recover_object_service_record(type, component_info_file_rcrd_list);
		if(read_status)
			this->basic_map.insert(std::make_pair(service_name, std::tr1::make_tuple(component_info_file_rcrd_list, 0.0)));
	}
}

void ComponentMapManager::update_transient_map_during_recovery(
	std::string service_name,
	std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > transferred_comp
)
{
	std::map<std::string, std::tr1::tuple<std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >, float> >::iterator itr;
	itr = this->transient_map.find(service_name);
	if ( itr != this->transient_map.end() )
	{
		OSDLOG(DEBUG, "Inserting components in TMap");
		std::tr1::get<0>(itr->second) = transferred_comp;
	}
	else
	{
		OSDLOG(INFO, "Service name " << service_name << " not found in Transient Map, adding it");
		std::tr1::tuple<std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >, float> t1 = std::tr1::make_tuple(transferred_comp, 0);
		this->transient_map.insert(std::make_pair(service_name, t1));
	}
	std::map<std::string, std::tr1::tuple<std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >, float> >::iterator itr_test = this->transient_map.find(service_name);
	//std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >::iterator itttt = (std::tr1::get<0>(itr_test->second)).begin();
	OSDLOG(INFO, "No of components for " << service_name << " in Transient map are " << (std::tr1::get<0>(itr_test->second)).size());
}

//HYD-7699
std::pair<std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >, bool> ComponentMapManager::update_transient_map(
	std::string service_name,
	std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > transferred_comp
)
{
	boost::recursive_mutex::scoped_lock lock(this->mtx);
	std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > actual_transferred_comp;
	OSDLOG(INFO, "Transferred comp list size is " << transferred_comp.size());

	std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > >::iterator
							it_global_planned_map = this->planned_map.find(service_name);

	if(it_global_planned_map != this->planned_map.end())
	{
	OSDLOG(INFO, "No of Components in Global Planned map: "<< it_global_planned_map->second.size());

	std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >::iterator it = transferred_comp.begin();
	std::map<std::string, std::tr1::tuple<std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >, float> >::iterator itr;
	bool found = true;
	for ( ; it != transferred_comp.end(); ++it )
	{
		bool status = this->remove_entry_from_global_planned_map(service_name, (*it).first);
		if(status)
		{
			itr = this->transient_map.find(service_name);
			if ( itr != this->transient_map.end() )
			{
				(std::tr1::get<0>(itr->second)).push_back((*it));
				found = true;
			}
			else
			{
				OSDLOG(INFO, "Service name " << service_name << " not found in Transient Map, adding it");
				std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > vec;
				vec.push_back(*it);
				std::tr1::tuple<std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >, float> t1 = std::tr1::make_tuple(vec, 0);
				this->transient_map.insert(std::make_pair(service_name, t1));
				found = true;
			}
			actual_transferred_comp.push_back(std::make_pair((*it).first, (*it).second));
		}
	}
	itr = this->transient_map.find(service_name);
	if(itr != this->transient_map.end() and found == true)
	{
		std::tr1::get<1>(itr->second) += 0.1;
		this->transient_map_version += 0.0001;
	}

	OSDLOG(INFO, "Global Planned Map size is " << it_global_planned_map->second.size());

	std::map<std::string, std::tr1::tuple<std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >, float> >::iterator itr_test = 
												this->transient_map.find(service_name);
	std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >::iterator itttt = (std::tr1::get<0>(itr_test->second)).begin();
	OSDLOG(INFO, "No of components for " << service_name << " in Transient map are " << (std::tr1::get<0>(itr_test->second)).size());
	}
	else
	{
		OSDLOG(INFO, "No planned entries found against " << service_name << " service in Global Planned Map");
	}
	bool ret_val = false;
	if(actual_transferred_comp.size() < transferred_comp.size())
		ret_val = false;
	else
		ret_val = true;

	return std::make_pair(actual_transferred_comp, ret_val);
}

bool ComponentMapManager::remove_entry_from_global_planned_map(std::string service_type, int32_t component_number)
{
	boost::recursive_mutex::scoped_lock lock(this->mtx);
	std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > >::iterator
	                                                                it_global_planned_map = this->planned_map.find(service_type);
	bool status = false;
	if (it_global_planned_map != this->planned_map.end())
	{
		std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > &component_list = it_global_planned_map->second;
		std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >::iterator 
		                                                                        it_pair_component = component_list.begin();
		for( ; it_pair_component != component_list.end();)
		{
			if ((*it_pair_component).first == component_number)
			{
				//Erase this pair from vector
				
				//OSDLOG(DEBUG, "Removing entry for "<< (*it_pair_component).first << " component from " <<
				//					it_global_planned_map->first << " from planned map");
				(it_global_planned_map->second).erase(it_pair_component);
				status = true;
				break;
			}
			else
			{
				++it_pair_component;
			}
		}
	}
	if(it_global_planned_map->second.empty())
	{
		this->planned_map.erase(service_type);
	} 
	return status;
}

bool ComponentMapManager::is_global_planned_empty(std::string service_name)
{
	boost::recursive_mutex::scoped_lock lock(this->mtx);
	std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > >::iterator
	                                                                it_global_planned_map = this->planned_map.find(service_name);
	if(it_global_planned_map != this->planned_map.end())
	{
		return false;
	}
	return true;
}

void ComponentMapManager::remove_object_entry_from_map(std::string service_id)
{
	boost::recursive_mutex::scoped_lock lock(this->mtx);
	OSDLOG(DEBUG, "Removing "<< service_id <<" from Info File and Basic Map");
	//Remove entry of component info record from Object Component Info File
	osm_info_file_manager::ComponentInfoFileWriteHandler whandler(this->osm_info_file_path);
	bool status = whandler.delete_record(osm_info_file_manager::OBJECT_COMPONENT_INFO_FILE, service_id);
	if (status)
	{
		OSDLOG(INFO, "Info Record for "<<service_id<< " removed from Component Info File");
		//Remove entry from basic map
		std::map<std::string, std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> >::iterator 									it_basic_map = this->basic_map.begin();
		if ((it_basic_map = this->basic_map.find(OBJECT_SERVICE)) != this->basic_map.end())
		{
			std::vector<recordStructure::ComponentInfoRecord> comp_info_rec = std::tr1::get<0>(it_basic_map->second);
			for (std::vector<recordStructure::ComponentInfoRecord>::iterator it_comp = comp_info_rec.begin(); it_comp != comp_info_rec.end(); )
			{
				if ((*it_comp).get_service_id() == service_id)
				{
					comp_info_rec.erase(it_comp);
					std::tr1::get<0>(it_basic_map->second) = comp_info_rec;
					std::tr1::get<1>(it_basic_map->second) += 0.1;
					OSDLOG(INFO, "Info Record for "<<service_id<< " removed from Global Basic Map");
					break;
				}
				else
				{
					++it_comp;
				}
			}
		}
	}
}

void ComponentMapManager::send_get_service_component_ack(
	boost::shared_ptr<comm_messages::GetServiceComponentAck> ownership_ack_ptr,
	boost::shared_ptr<Communication::SynchronousCommunication> comm
)
{
	OSDLOG(INFO, "Sending get_my_ownership response");
	SuccessCallBack callback_obj;
	MessageExternalInterface::sync_send_get_service_component_ack(
					ownership_ack_ptr.get(),
					comm,
					boost::bind(&SuccessCallBack::success_handler, &callback_obj, _1),
					boost::bind(&SuccessCallBack::failure_handler, &callback_obj)
				);	
	OSDLOG(DEBUG, "Sent get_my_ownership response");

}

std::vector<uint32_t> ComponentMapManager::get_component_ownership(
	std::string service_id
)
{
	GLUtils util_obj;
	std::string service_name = util_obj.get_service_name(service_id);
	std::pair<std::map<std::string, std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> >, float> gmap_full = this->get_global_map();
	std::map<std::string, std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> > gmap = gmap_full.first;
	std::map<std::string, std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> >::iterator it = 
							gmap.find(service_name);
	boost::shared_ptr<ErrorStatus> error_status;
	boost::shared_ptr<comm_messages::GetServiceComponentAck> req_ack;
	std::vector<uint32_t> owner_list;
	if(it != gmap.end())
	{
		std::vector<recordStructure::ComponentInfoRecord> serv_obj_list = std::tr1::get<0>(it->second);
		std::vector<recordStructure::ComponentInfoRecord>::iterator it1 = serv_obj_list.begin();
		for(int i = 0; it1 != serv_obj_list.end(); ++it1, ++i)
		{
			if ( (*it1).get_service_id() == service_id )
			{
				owner_list.push_back(i);
			}
		}
//		error_status.reset(new ErrorStatus(0, "SUCCESS"));
	}
	else
	{
		OSDLOG(INFO, service_id << " doesn't own any component");
	}
	return owner_list;
/*	else
	{
		error_status.reset(new ErrorStatus(1, "NO SUCH SERVICE FOUND IN GLOBAL MAP"));
	}
	
	req_ack.reset(new comm_messages::GetServiceComponentAck(owner_list, error_status));
*/	
}	

void ComponentMapManager::add_entry_in_global_planned_map(
	std::string service_type,
	int32_t component_number,
	recordStructure::ComponentInfoRecord comp_info_record
)
{
	boost::recursive_mutex::scoped_lock lock(this->mtx);
	std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > >::iterator
	                                                                        it_global_planned_map = this->planned_map.begin();
	if ((it_global_planned_map = this->planned_map.find(service_type)) != this->planned_map.end())
	{
		std::pair<int32_t, recordStructure::ComponentInfoRecord> p1;
		p1.first = component_number;
		p1.second = comp_info_record;
		(it_global_planned_map->second).push_back(p1);
	}
	else
	{
		std::string key = service_type;
		std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > comp_rec_list;
		std::pair<int32_t, recordStructure::ComponentInfoRecord> p1;
                p1.first = component_number;
                p1.second = comp_info_record;
		comp_rec_list.push_back(p1);
		this->planned_map.insert(std::pair<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > >(key, comp_rec_list));

	}
}

bool ComponentMapManager::add_object_entry_in_basic_map(std::string service_type, std::string service_id)
{
	if (service_type != OBJECT_SERVICE)
	{
		return false;
	}
	//Check required for service type other than Object_Service??
	OSDLOG(INFO, "Adding object entry in Component Info File for service: " << service_id);
	osm_info_file_manager::NodeInfoFileReadHandler read_hndlr(this->node_info_file_path);
	GLUtils util_obj;
	std::string node_id = util_obj.get_node_id(service_id);
	boost::shared_ptr<recordStructure::NodeInfoFileRecord> rcrd = read_hndlr.get_record_from_id(node_id);
	recordStructure::ComponentInfoRecord comp_info_record(service_id, rcrd->get_node_ip(), rcrd->get_objectService_port());
	boost::recursive_mutex::scoped_lock lock(this->mtx);
	std::map<std::string, std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> >::iterator 									it_basic_map = this->basic_map.begin();
	if ((it_basic_map = this->basic_map.find(service_type)) != this->basic_map.end())
	{
		std::tr1::get<0>(it_basic_map->second).push_back(comp_info_record);
		std::tr1::get<1>(it_basic_map->second) += 0.1;
	}
	else
	{
		OSDLOG(INFO, "Key: " << service_type << " does not exist in basic map.");
		std::string key = service_type;
		std::vector<recordStructure::ComponentInfoRecord> comp_rec_list;
		comp_rec_list.clear();
		comp_rec_list.push_back(comp_info_record);
		float ver = 0.1;
		std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> comp_ver_tuple;
		std::tr1::get<0>(comp_ver_tuple) = comp_rec_list;
		std::tr1::get<1>(comp_ver_tuple) = ver;
		this->basic_map.insert(std::pair<std::string, std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> >(key, comp_ver_tuple));
		OSDLOG(DEBUG, "Added "<< comp_info_record.get_service_id() << " in Global Basic Map");
	}
	//boost::shared_ptr<recordStructure::ComponentInfoRecord> file_rcrd(&comp_info_record);
	boost::shared_ptr<recordStructure::ComponentInfoRecord> file_rcrd = boost::make_shared<recordStructure::ComponentInfoRecord>(comp_info_record);
	osm_info_file_manager::ComponentInfoFileWriteHandler whandler(this->osm_info_file_path);
        bool status = whandler.add_record(osm_info_file_manager::OBJECT_COMPONENT_INFO_FILE, file_rcrd);
	OSDLOG(DEBUG, "Status of writing in Object Info File: " << status);
        return status;
}

//Called only when first node(s) is added in OSD cluster.
void ComponentMapManager::prepare_basic_map(std::string service_type, std::vector<recordStructure::ComponentInfoRecord> comp_info_records) 
{
	std::string key = service_type;
	float ver = 0.1;
	std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> comp_ver_tuple;
	std::tr1::get<0>(comp_ver_tuple) = comp_info_records;
	std::tr1::get<1>(comp_ver_tuple) = ver;
	OSDLOG(INFO, "Adding Key-Value pair for service " << service_type << " in basic map during first set of NA");
	this->basic_map[service_type] = comp_ver_tuple; 
	this->global_map_version += 1.0;
	//this->global_map_version = 2.0;
	/*if ((it_basic_map = this->basic_map.find(service_type)) != this->basic_map.end())
	{
		this->basic_map[service_type] = comp_ver_tuple; 
	}
	else
	{
	this->basic_map.insert(std::pair<std::string, std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> >(key, comp_ver_tuple));
	}*/
}

void ComponentMapManager::clear_global_map() //interface used if Size of OSD cluster goes down to 0, i.e. no healthy nodes available
{
	boost::recursive_mutex::scoped_lock lock(this->mtx);
	this->basic_map.clear();
	this->transient_map.clear();
	this->planned_map.clear();
}

void ComponentMapManager::clear_global_planned_map() //interface usedwhen recovering cluster with All failed nodes.
{
	this->planned_map.clear();
}

//Used when system stabilizes and basic map and transient map are merged and written on component info files
void ComponentMapManager::merge_basic_and_transient_map()
{
	boost::recursive_mutex::scoped_lock lock(this->mtx);
	std::map<std::string, std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> >::iterator basic_map_itr = 
												this->basic_map.find(CONTAINER_SERVICE);
	std::map<std::string, std::tr1::tuple<std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >, float> >::iterator
									transient_map_itr = this->transient_map.find(CONTAINER_SERVICE);

	OSDLOG(INFO, "Global Map version is " << this->global_map_version << " and transient map version is " << this->transient_map_version);
	this->global_map_version = ceil(this->global_map_version + this->transient_map_version);
	OSDLOG(INFO, "After ceiling Global Map version, version is " << this->global_map_version);

	if (basic_map_itr != this->basic_map.end() and transient_map_itr != this->transient_map.end())
	{
		OSDLOG(INFO, "Merging Basic and Transient Map for " << CONTAINER_SERVICE);
		std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > tra_vec = std::tr1::get<0>(transient_map_itr->second);
		std::vector<recordStructure::ComponentInfoRecord> &basic_vec = std::tr1::get<0>(basic_map_itr->second);

		std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >::iterator tra_vec_itr = tra_vec.begin();
		for ( ; tra_vec_itr != tra_vec.end() ; ++tra_vec_itr )
		{
			std::pair<int32_t, recordStructure::ComponentInfoRecord> t_pair = *tra_vec_itr;
			basic_vec[t_pair.first] = t_pair.second;
			std::tr1::get<1>(basic_map_itr->second) += std::tr1::get<1>(transient_map_itr->second);	
		}
		this->write_in_component_file(CONTAINER_SERVICE, osm_info_file_manager::CONTAINER_COMPONENT_INFO_FILE);
	}

	basic_map_itr = this->basic_map.find(ACCOUNT_SERVICE);
	transient_map_itr = this->transient_map.find(ACCOUNT_SERVICE);

	if (basic_map_itr != this->basic_map.end() and transient_map_itr != this->transient_map.end())
	{
		OSDLOG(INFO, "Merging Basic and Transient Map for " << ACCOUNT_SERVICE);
		std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > tra_vec = std::tr1::get<0>(transient_map_itr->second);
		std::vector<recordStructure::ComponentInfoRecord> &basic_vec = std::tr1::get<0>(basic_map_itr->second);

		std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >::iterator tra_vec_itr = tra_vec.begin();
		for ( ; tra_vec_itr != tra_vec.end() ; ++tra_vec_itr )
		{
			std::pair<int32_t, recordStructure::ComponentInfoRecord> t_pair = *tra_vec_itr;
			basic_vec[t_pair.first] = t_pair.second;
			std::tr1::get<1>(basic_map_itr->second) += std::tr1::get<1>(transient_map_itr->second);
		}
		this->write_in_component_file(ACCOUNT_SERVICE, osm_info_file_manager::ACCOUNT_COMPONENT_INFO_FILE);

	}

	basic_map_itr = this->basic_map.find(ACCOUNT_UPDATER);
	transient_map_itr = this->transient_map.find(ACCOUNT_UPDATER);

	if (basic_map_itr != this->basic_map.end() and transient_map_itr != this->transient_map.end())
	{
		OSDLOG(INFO, "Merging Basic and Transient Map for " << ACCOUNT_UPDATER);
		std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > tra_vec = std::tr1::get<0>(transient_map_itr->second);
		std::vector<recordStructure::ComponentInfoRecord> &basic_vec = std::tr1::get<0>(basic_map_itr->second);

		std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >::iterator tra_vec_itr = tra_vec.begin();
		for ( ; tra_vec_itr != tra_vec.end() ; ++tra_vec_itr )
		{
			std::pair<int32_t, recordStructure::ComponentInfoRecord> t_pair = *tra_vec_itr;
			basic_vec[t_pair.first] = t_pair.second;
			std::tr1::get<1>(basic_map_itr->second) += std::tr1::get<1>(transient_map_itr->second);
		}
		this->write_in_component_file(ACCOUNT_UPDATER, osm_info_file_manager::ACCOUNTUPDATER_COMPONENT_INFO_FILE);
	}
	this->transient_map.clear();
	this->transient_map_version = 0;
}

bool ComponentMapManager::write_in_component_file(std::string service_name, int info_file)
{
	boost::recursive_mutex::scoped_lock lock(this->mtx);
	std::map<std::string, std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> >::iterator basic_map_itr =
												this->basic_map.find(service_name);
	boost::shared_ptr<recordStructure::CompleteComponentRecord> file_rcrd(
							new recordStructure::CompleteComponentRecord(
								std::tr1::get<0>(basic_map_itr->second),
								std::tr1::get<1>(basic_map_itr->second),
								ceil(this->global_map_version)
							)
						);
	osm_info_file_manager::ComponentInfoFileWriteHandler whandler(this->osm_info_file_path);
	bool status = whandler.add_record(info_file, file_rcrd);
	return status;
}

float ComponentMapManager::get_global_map_version()
{
	boost::recursive_mutex::scoped_lock lock(this->mtx);
	return (this->global_map_version + this->transient_map_version);
}

bool ComponentMapManager::send_ack_for_comp_trnsfr_msg(
	boost::shared_ptr<Communication::SynchronousCommunication> comm_ptr,
	int ack_type
)
{
	boost::recursive_mutex::scoped_lock lock(this->mtx);
	OSDLOG(DEBUG, "Sending ack for Comp Trans Msg");
	std::pair<std::map<std::string, std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> >, float> g_map_full = this->get_global_map();
	std::map<std::string, std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> > g_map = g_map_full.first;
	std::map<std::string, std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> >::iterator itr;

	itr = g_map.find(CONTAINER_SERVICE);
	std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> cont_pair;
	std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> account_pair;
	std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> updater_pair;
	std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> object_pair;

	if(itr != g_map.end())
		cont_pair = itr->second;

	itr = g_map.find(ACCOUNT_SERVICE);
	if(itr != g_map.end())
		account_pair = itr->second;

	itr = g_map.find(ACCOUNT_UPDATER);
	if(itr != g_map.end())
		updater_pair = itr->second;

	itr = g_map.find(OBJECT_SERVICE);
	if(itr != g_map.end())
		object_pair = itr->second;

	SuccessCallBack callback_obj;
	if(ack_type == messageTypeEnum::typeEnums::COMP_TRANSFER_INFO_ACK)
	{
		OSDLOG(INFO, "Sending COMP_TRANSFER_INFO_ACK with GL map version " << g_map_full.second);
		boost::shared_ptr<comm_messages::CompTransferInfoAck> comp_trnsfer_ack(
			new comm_messages::CompTransferInfoAck(cont_pair, account_pair, updater_pair, object_pair, g_map_full.second)
			);
		MessageExternalInterface::sync_send_component_transfer_ack(
			comp_trnsfer_ack.get(),
			comm_ptr,
			boost::bind(&SuccessCallBack::success_handler, &callback_obj, _1),
			boost::bind(&SuccessCallBack::failure_handler, &callback_obj)
			);
	}
	if(ack_type == messageTypeEnum::typeEnums::COMP_TRANSFER_FINAL_STAT_ACK)
	{
		OSDLOG(INFO, "Sending COMP_TRANSFER_FINAL_STAT_ACK " << g_map_full.second);
		boost::shared_ptr<comm_messages::CompTransferFinalStatAck> comp_trnsfer_ack(
			new comm_messages::CompTransferFinalStatAck(cont_pair, account_pair, updater_pair, object_pair, this->global_map_version)
			);
		if(comm_ptr != NULL)
		{
			MessageExternalInterface::sync_send_component_transfer_final_status_ack(
				comp_trnsfer_ack.get(),
				comm_ptr,
				boost::bind(&SuccessCallBack::success_handler, &callback_obj, _1),
				boost::bind(&SuccessCallBack::failure_handler, &callback_obj)
				);
		}
		else
		{
			OSDLOG(ERROR, "Connection is closed");
		}
	}
	OSDLOG(INFO, "Sent ack for Comp Trans Msg");
	return callback_obj.get_result();
				
}

void ComponentMapManager::send_get_global_map_ack(boost::shared_ptr<Communication::SynchronousCommunication> comm)
{
	boost::recursive_mutex::scoped_lock lock(this->mtx);
	std::pair<std::map<std::string, std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> >, float> g_map_full = this->get_global_map();
	std::map<std::string, std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> > g_map = g_map_full.first;
	OSDLOG(INFO, "Global Map size is : " << g_map.size());
	std::map<std::string, std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> >::iterator itr;
	std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> cont_pair;
	std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> account_pair;
	std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> updater_pair;
	std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> object_pair;

	itr = g_map.find(CONTAINER_SERVICE);
	if(itr != g_map.end())
		cont_pair = itr->second;

	itr = g_map.find(ACCOUNT_SERVICE);
	if(itr != g_map.end())
		account_pair = itr->second;

	itr = g_map.find(ACCOUNT_UPDATER);
	if(itr != g_map.end())
		updater_pair = itr->second;

	itr = g_map.find(OBJECT_SERVICE);
	if(itr != g_map.end())
		object_pair = itr->second;

	boost::shared_ptr<comm_messages::GlobalMapInfo> gmap_ack(
			new comm_messages::GlobalMapInfo(cont_pair, account_pair, updater_pair, object_pair, g_map_full.second)
			);
	SuccessCallBack callback_obj;
	MessageExternalInterface::sync_send_global_map_info(
			gmap_ack.get(),
			comm, 
			boost::bind(&SuccessCallBack::success_handler, &callback_obj, _1),
			boost::bind(&SuccessCallBack::failure_handler, &callback_obj)
			);
	OSDLOG(DEBUG, "Sent GET_GLOBAL_MAP_ACK");
}

//Used when Global Map is requested by OSD services
std::pair<std::map<std::string, std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> >, float> ComponentMapManager::get_global_map()
{
	boost::recursive_mutex::scoped_lock lock(this->mtx);
	//Merge transient and basic map and return
	std::map<std::string, std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> > basic_map_copy = this->basic_map;

	std::map<std::string, std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> >::iterator basic_map_itr = basic_map_copy.find(CONTAINER_SERVICE);
	std::map<std::string, std::tr1::tuple<std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >, float> >::iterator transient_map_itr = this->transient_map.find(CONTAINER_SERVICE);

	if (basic_map_itr != basic_map_copy.end() and transient_map_itr != this->transient_map.end())
	{
		std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > tra_vec = std::tr1::get<0>(transient_map_itr->second);
		std::vector<recordStructure::ComponentInfoRecord> &basic_vec = std::tr1::get<0>(basic_map_itr->second);

		std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >::iterator tra_vec_itr = tra_vec.begin();
		for ( ; tra_vec_itr != tra_vec.end() ; ++tra_vec_itr )
		{
			std::pair<int32_t, recordStructure::ComponentInfoRecord> t_pair = *tra_vec_itr;
			basic_vec[t_pair.first] = t_pair.second;
		}
		std::tr1::get<1>(basic_map_itr->second) += std::tr1::get<1>(transient_map_itr->second);
		OSDLOG(INFO, "Finished Merging Container Server components in Basic and Transient Map");
	}

	basic_map_itr = basic_map_copy.find(ACCOUNT_SERVICE);
	transient_map_itr = this->transient_map.find(ACCOUNT_SERVICE);


	if (basic_map_itr != basic_map_copy.end() and transient_map_itr != this->transient_map.end())
	{
		std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > tra_vec = std::tr1::get<0>(transient_map_itr->second);
		std::vector<recordStructure::ComponentInfoRecord> &basic_vec = std::tr1::get<0>(basic_map_itr->second);

		std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >::iterator tra_vec_itr = tra_vec.begin();
		for ( ; tra_vec_itr != tra_vec.end() ; ++tra_vec_itr )
		{
			std::pair<int32_t, recordStructure::ComponentInfoRecord> t_pair = *tra_vec_itr;
			basic_vec[t_pair.first] = t_pair.second;
		}
		std::tr1::get<1>(basic_map_itr->second) += std::tr1::get<1>(transient_map_itr->second);

		OSDLOG(INFO, "Finished Merging Account Server components in Basic and Transient Map");
	}

	basic_map_itr = basic_map_copy.find(ACCOUNT_UPDATER);
	transient_map_itr = this->transient_map.find(ACCOUNT_UPDATER);


	if (basic_map_itr != basic_map_copy.end() and transient_map_itr != this->transient_map.end())
	{
		std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > tra_vec = std::tr1::get<0>(transient_map_itr->second);
		std::vector<recordStructure::ComponentInfoRecord> &basic_vec = std::tr1::get<0>(basic_map_itr->second);

		std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >::iterator tra_vec_itr = tra_vec.begin();
		for ( ; tra_vec_itr != tra_vec.end() ; ++tra_vec_itr )
		{
			std::pair<int32_t, recordStructure::ComponentInfoRecord> t_pair = *tra_vec_itr;
			basic_vec[t_pair.first] = t_pair.second;
		}
		std::tr1::get<1>(basic_map_itr->second) += std::tr1::get<1>(transient_map_itr->second);
		OSDLOG(INFO, "Finished Merging Account-Updater Server components in Basic and Transient Map");

	}
	float version = this->global_map_version + this->transient_map_version;
	std::pair<std::map<std::string, std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> >, float> basic_full_map_copy = std::make_pair(basic_map_copy, version);
	OSDLOG(INFO, "Basic Map copy size is: " << basic_map_copy.size() << " and map version is " << version);
	return basic_full_map_copy;
}

recordStructure::ComponentInfoRecord ComponentMapManager::get_service_info(std::string service_id)
{
            recordStructure::ComponentInfoRecord ret_obj;
            GLUtils utils_obj;
	    std::string service_name = utils_obj.get_service_name(service_id);
	    boost::recursive_mutex::scoped_lock lock(this->mtx);
	    std::map<std::string, std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> >::iterator it = this->basic_map.find(service_name);
	    std::vector<recordStructure::ComponentInfoRecord> service_list = std::tr1::get<0>(it->second);
	    std::vector<recordStructure::ComponentInfoRecord>::iterator it1 = service_list.begin();
	    for( ; it1 != service_list.end(); ++it1 )
	    {
	    	if((*it1).get_service_id() == service_id)
		return (*it1);
	    }
	    return ret_obj;
}

recordStructure::ComponentInfoRecord ComponentMapManager::get_service_info(std::string dest_serv_id, std::vector<recordStructure::NodeInfoFileRecord> node_info_file_records)
{
	GLUtils utils_obj;
	std::string service_name = utils_obj.get_service_name(dest_serv_id);
	std::string dest_node_id = utils_obj.get_node_id(dest_serv_id);
	std::string dest_serv_ip;
	int32_t dest_serv_port = -1;
	std::vector<recordStructure::NodeInfoFileRecord>::iterator nifr_itr = node_info_file_records.begin();
	for (; nifr_itr != node_info_file_records.end(); ++nifr_itr)
	{
		if ((nifr_itr)->get_node_id() == dest_node_id)
		{
			dest_serv_ip = (nifr_itr)->get_node_ip();
			if (service_name == CONTAINER_SERVICE)
			{
				dest_serv_port = (nifr_itr)->get_containerService_port();
			}
			else if (service_name == ACCOUNT_SERVICE)
			{
				dest_serv_port = (nifr_itr)->get_accountService_port();
			}
			else if (service_name == ACCOUNT_UPDATER)
			{
				dest_serv_port = (nifr_itr)->get_accountUpdaterService_port();
			}
		}
	}
	recordStructure::ComponentInfoRecord component_record(dest_serv_id, dest_serv_ip, dest_serv_port);
	return component_record;
}


} // namespace component_manager
