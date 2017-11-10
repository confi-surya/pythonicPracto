#include "osm/osmServer/osm_info_file.h"
#include "osm/globalLeader/component_balancing.h"
#include "osm/osmServer/utils.h"
#include "libraryUtils/object_storage_log_setup.h"
#include "libraryUtils/object_storage_exception.h"


using namespace object_storage_log_setup;
using namespace OSDException;

namespace component_balancer
{

ComponentBalancer::ComponentBalancer(
	boost::shared_ptr<component_manager::ComponentMapManager> &map_manager,
	cool::SharedPtr<config_file::OSMConfigFileParser> parser_ptr
)
/*
Constructor for ComponentBalancer
*/
	: map_manager(map_manager)
{
	this->service_to_record_type_map_ptr.reset(new ServiceToRecordTypeMapper());
	this->node_info_file_path = parser_ptr->node_info_file_pathValue();
	ServiceToRecordTypeBuilder type_builder;
	type_builder.register_services(this->service_to_record_type_map_ptr);
	this->total_no_of_components = parser_ptr->total_componentsValue();

}

ComponentBalancer::~ComponentBalancer()
/*
Destructor for ComponentBalancer
*/
{
}

// @public interface
int ComponentBalancer::set_healthy_node_list(std::vector<std::string> healthy_node_list)
/*
*
*@desc: Populate the healthy node list maintained by component balancer. It is only called when GL recovers after a failure.
*
*@param: healthy node list maintained by SCI
*
*@Return: an integer value, size of the healthy node list, i.e., total no of healthy nodes in the cluster.
*/
{
	this->healthy_node_list = healthy_node_list;	
	return this->healthy_node_list.size();
}

// @public interface
int ComponentBalancer::set_unhealthy_node_list(std::vector<std::string> failed_node_list)
/*
*
*@desc: Populate the unhealthy node list maintained by component balancer. It is only called when GL recovers after a failure.
*
*@param: failed node list maintained by SCI
*
*@Return: an integer value, size of the unhealthy node list, i.e., total no of unhealthy nodes in the cluster.
*/
{
	this->unhealthy_node_list = failed_node_list;	
	return this->unhealthy_node_list.size();
}

// @public interface
int ComponentBalancer::set_recovered_node_list(std::vector<std::string> recovered_node_list)
/*
*
*@desc: Populate the recovered node list maintained by component balancer. It is only called when GL recovers after a failure.
*
*@param: recovered node list maintained by SCI
*
*@Return: an integer value, size of the recovered node list, i.e., total no of recovered nodes in the cluster.
*/
{
	this->recovered_node_list = recovered_node_list;	
	return this->recovered_node_list.size();
}

// @public interface
void ComponentBalancer::remove_entry_from_unhealthy_node_list(std::string node_id_to_be_removed)
/*
*
*@desc: Removes an entry from the unhealthy node list maintained by component balancer.
*
*@param: node_id to be removed from unhealthy node list
*/
{
	this->unhealthy_node_list.erase(std::remove(
			this->unhealthy_node_list.begin(), 
			this->unhealthy_node_list.end(), 
			node_id_to_be_removed), 
			this->unhealthy_node_list.end()
			);	
	this->recovered_node_list.push_back(node_id_to_be_removed);

	//Sort and unique-ify irecovered_node_list
	sort( this->recovered_node_list.begin(), this->recovered_node_list.end());
	this->recovered_node_list.erase( unique( this->recovered_node_list.begin(), this->recovered_node_list.end()), this->recovered_node_list.end());
}

// @public interface
void ComponentBalancer::reinstantiate_balancer()
{
	OSDLOG(INFO, "Going to clear Component Balancer");
	this->all_node_list.clear();
	this->healthy_node_list.clear();
	this->unhealthy_node_list.clear();
	this->recovered_node_list.clear();
//	this->node_info_file_record.clear();
	this->list_of_services_for_addition.clear();
	this->list_of_services_for_deletion.clear();
	this->list_of_services_for_recovery.clear();
	this->source_services.clear();
	this->destination_services.clear();

	this->node_recovery_dest_map.clear();
	this->internal_basic_map.clear();
	this->internal_planned_map.clear();
	map_manager->clear_global_planned_map();
}

// @public interface
void ComponentBalancer::push_node_info_file_records_into_memory(recordStructure::NodeInfoFileRecord node_info_file_record)
/*
*
*@desc: used by NodeInfoFileReadHandler to populate node info file records into memory maintained by Component Balancer.
*
*@param: Node Info File Record
*/
{
	this->node_info_file_records.push_back(node_info_file_record);
}

void ComponentBalancer::prepare_internal_basic_map(std::string service_name)
/*
*@desc: Transforms the global basic map into an internal strcuture of 
*	Key	Value
*	S_ID	([X1,X2...Xn], n)
*
*@param: Service type for which internal basic map is to be prepared, i.e., account-server, container-server, etc.
*/
{
	this->internal_basic_map.clear();
	std::map<std::string, std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> >::iterator it_basic_map = this->orig_basic_map.find(service_name);
	if (it_basic_map != this->orig_basic_map.end())
	{
		OSDLOG(DEBUG, "Begin Iterating "<<it_basic_map->first<<" key-value pair for "<<service_name<< " in Basic Map");
		std::vector<recordStructure::ComponentInfoRecord>::iterator it_list = (std::tr1::get<0>(it_basic_map->second)).begin();
		int component_number = -1;
		for ( ; it_list != (std::tr1::get<0>(it_basic_map->second)).end(); ++it_list)
		{	
			component_number += 1;
			std::string current_service_id = (*it_list).get_service_id();
			this->add_entry_in_internal_basic_map(component_number, current_service_id);
		}
	}
	OSDLOG(INFO, "Internal Basic Map prepared from Global Basic Map.");
}

void ComponentBalancer::merge_transient_into_internal_basic_map(std::string service_name) 
/*
* @desc: Merges the transient map into internal basic map, the one prepared in prepare_internal_basic_map() interface. Since, transient map holds the components which have been actually transferred to planned destination, they can be re-considered for balancing and transfer.
*
*@param - Service type for which transient map components are to be merged with internal basic map.
*/
{
        GLUtils utils_obj;
	//std::cout<<"Transient Map Size: "<<this->orig_transient_map.size()<<std::endl;
	std::map<std::string, std::tr1::tuple<std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >, float> >::iterator 										it_transient_map = this->orig_transient_map.find(service_name);	
	if (it_transient_map != this->orig_transient_map.end())
	{
		OSDLOG(DEBUG, "Begin Iterating "<<it_transient_map->first<<" key-value pair for "<<service_name<<" in Transient Map");
		OSDLOG(INFO, "Total No of Components in Transient Map: "<<std::tr1::get<0>(it_transient_map->second).size()<< "to be merged into Internal basic Map");
		std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >::iterator it_pair_component = (std::tr1::get<0>(it_transient_map->second)).begin();
		for( ; it_pair_component != (std::tr1::get<0>(it_transient_map->second)).end(); ++it_pair_component)
		{
			int32_t component_number = (*it_pair_component).first;
			std::string current_service_id = ((*it_pair_component).second).get_service_id();
			OSDLOG(DEBUG, "Moving component "<<component_number<<" to "<<current_service_id);
			this->remove_value_from_internal_basic_map(component_number);
			this->add_entry_in_internal_basic_map(component_number, current_service_id);
		}	
	}
	OSDLOG(INFO, "Finished Merging transient map components into Internal Basic Map.");
}

void ComponentBalancer::merge_planned_into_internal_basic_map(std::string service_name)
/*
*
*@desc: Merges the planned map into internal basic map, depending upon the state of original source service & new planned service. These components i.e., the ones in planned map, are not to be considered in subsequent balancing if both the source and the destination services are healthy. 
*
*@param: Service type for which planned map components are to be merged with internal basic map.
*/
{
        GLUtils utils_obj;

	std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > >::iterator 
								it_global_planned_map = this->orig_planned_map.find(service_name);
	if (it_global_planned_map != this->orig_planned_map.end())
        {
		OSDLOG(DEBUG, "Begin Iterating "<<it_global_planned_map->first<<" key-value pair for "<<service_name);
		std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >::iterator it_pair_component = 														(it_global_planned_map->second).begin();
                for( ; it_pair_component != (it_global_planned_map->second).end(); ++it_pair_component)
		{
			int32_t component_number = (*it_pair_component).first;
                        std::string planned_service_id = ((*it_pair_component).second).get_service_id();
			std::map<std::string, std::tr1::tuple<std::vector<int32_t>, int32_t> >::iterator it_internal_basic_map_planned_id = this->internal_basic_map.find(planned_service_id);
			std::vector<std::string> unhealthy_service_list;
			for (std::vector<std::string>::iterator it = this->unhealthy_node_list.begin(); 
									it != this->unhealthy_node_list.end(); ++it)
			{
				//push_back service_id for each *it
				std::string unhealthy_service_id = utils_obj.get_service_id(*it, service_name); 
				unhealthy_service_list.push_back(unhealthy_service_id);	
			}
        		if (!((std::find(unhealthy_service_list.begin(), unhealthy_service_list.end(), 
								planned_service_id)) != unhealthy_service_list.end()))
			{
				//planned service id not found in list of previously failed list of services 
				std::map<std::string, std::tr1::tuple<std::vector<int32_t>, int32_t> >::iterator it_internal_basic_map1 =
                                                                                                this->internal_basic_map.begin();
                                for ( ; it_internal_basic_map1 != this->internal_basic_map.end(); ++it_internal_basic_map1)
                                {
					std::vector<int32_t> comp_list = std::tr1::get<0>(it_internal_basic_map1->second);
 					std::vector<int32_t>::iterator itr1 = comp_list.begin();
					if (std::find(comp_list.begin(), comp_list.end(), component_number) != comp_list.end())
					{
						std::string internal_map_service_id = it_internal_basic_map1->first;
						
						std::vector<std::string>::iterator i1 = 
								std::find(this->list_of_services_for_recovery.begin(), 
								this->list_of_services_for_recovery.end(), internal_map_service_id);
						std::vector<std::string>::iterator i2 = 
								std::find(this->list_of_services_for_deletion.begin(), 
								this->list_of_services_for_deletion.end(), internal_map_service_id);
						if ((i1 != this->list_of_services_for_recovery.end()) || 
									(i2 != this->list_of_services_for_deletion.end()))
						//soure service_id found in list of nodes for deletion/recovery 
						{
							/*Source service id belongs to node being deleted or recovered 
							  do not change the internal basic map, neither change the count. 
							  remove entries from global planned map. 
							*/
							if (map_manager->remove_entry_from_global_planned_map(service_name, 
													component_number))
							{
								OSDLOG(INFO, "Removed "<<component_number<<" from Global planned Map");
							}
							if (this->internal_basic_map.find(planned_service_id) == 
												this->internal_basic_map.end())
							//Most probably a case of node addition of planned_service_id
							{
								std::vector<int32_t> comp_list;
								comp_list.empty();
								int32_t count = 0;
								std::tr1::tuple<std::vector<int32_t>, int32_t>  comp_tuple;
								std::tr1::get<0>(comp_tuple) = comp_list;
								std::tr1::get<1>(comp_tuple) = count;
								this->internal_basic_map[planned_service_id] = comp_tuple;
							}
						}
						else
						{
							/* source service_id neither in failed node list, 
							   nor being transferred from node being deleted/recovered.
							*/
							if (this->internal_basic_map.find(planned_service_id) == 
												this->internal_basic_map.end())
							//Most probably a case of node addition, add service_id with zero components. 
							{
								std::vector<int32_t> comp_list;
								comp_list.empty();
								int32_t count = 0;
								std::tr1::tuple<std::vector<int32_t>, int32_t>  comp_tuple;
								std::tr1::get<0>(comp_tuple) = comp_list;
								std::tr1::get<1>(comp_tuple) = count;
								this->internal_basic_map[planned_service_id] = comp_tuple;
							}

							std::vector<int32_t>::iterator t_itr = std::find(std::tr1::get<0>(it_internal_basic_map1->second).begin(), std::tr1::get<0>(it_internal_basic_map1->second).end(), component_number);
							if(t_itr != std::tr1::get<0>(it_internal_basic_map1->second).end())
							{
								//OSDLOG(DEBUG, "Component "<<component_number<<" is planned");
								(std::tr1::get<0>(it_internal_basic_map1->second)).erase(t_itr);
								(std::tr1::get<1>(it_internal_basic_map1->second)) -= 1;
								//increment the count of planned_service_id in internal basic map
								it_internal_basic_map_planned_id = this->internal_basic_map.find(planned_service_id);
								if (it_internal_basic_map_planned_id != this->internal_basic_map.end())
								{
									std::tr1::get<1>(it_internal_basic_map_planned_id->second) += 1; 
									//OSDLOG(DEBUG, "Incrementing count for components of "<< planned_service_id<<" by 1"); //TODO: Remove this log. done for testing purpose
								}
							}
							
						}
					}
					else
					{
						/* component not owned by current service id
						   skip to next service id (key-value pair) in
						   internal basic map.
						*/
//						std::cout<<"Component "<<component_number<<" not owned by "<<
//							it_internal_basic_map1->first<<". Search with next service"<<std::endl;
					}
				}
			}
			else
			{
				/* do not move the component number, nor change the total count if 
				   planned destination service is found in list of failed service.
				*/
				/*Planned service id belongs to node being deleted or recovered 
				  do not change the internal basic map, neither change the count. 
				  remove entries from global planned map. 
				*/
				map_manager->remove_entry_from_global_planned_map(service_name, 
										component_number);
				OSDLOG(INFO, "Removed "<<component_number<<" from Global planned Map");
			}
		} //iterating list of pair of (component_no, component_info_record), i.e. value part of global_planned_map

	} //if service_name is found in global_planned_map

	OSDLOG(INFO, "Finished Merging Planned map components into Internal Basic Map.");
} //end of function merge_planned_into_internal_basic_map(std::string service_name)

void ComponentBalancer::remove_value_from_internal_basic_map(int32_t component_number)
/*
*@desc: Removes the component (planned or transferred) from source service ID in internal basic map. 
*
*@param: Component Number which is to be removed
*/
{
	//OSDLOG(DEBUG, "Removing Component "<<component_number<<" from Internal Basic Map");
	std::map<std::string, std::tr1::tuple<std::vector<int32_t>, int32_t> >::iterator it_internal_basic_map = this->internal_basic_map.begin();
	for ( ; it_internal_basic_map != this->internal_basic_map.end(); ++it_internal_basic_map)
	{
		int32_t initial_size = (std::tr1::get<0>(it_internal_basic_map->second)).size();
		std::vector<int32_t>::iterator t_itr = std::find(std::tr1::get<0>(it_internal_basic_map->second).begin(), std::tr1::get<0>(it_internal_basic_map->second).end(), component_number);
		if(t_itr != std::tr1::get<0>(it_internal_basic_map->second).end())
		{
			(std::tr1::get<0>(it_internal_basic_map->second)).erase(t_itr);
		}
		int32_t final_size = (std::tr1::get<0>(it_internal_basic_map->second)).size();
		if ((final_size - initial_size) != 0)
		{
			std::tr1::get<1>(it_internal_basic_map->second) -= 1; 
		}
	}
}

bool ComponentBalancer::add_entry_in_internal_basic_map(int32_t component_number, std::string current_service_id)
/*
*@desc: Adds the component to the service to which it is transferred or is planned to be transferred
*
*@param: Component Number which is to be added
*@param: New Service Id to which this component is to be added.
*/
{
	
	bool entry_added = false;
	std::map<std::string, std::tr1::tuple<std::vector<int32_t>, int32_t> >::iterator it_internal_basic_map = this->internal_basic_map.begin();
	if ((it_internal_basic_map = this->internal_basic_map.find(current_service_id)) !=
                                                                                        this->internal_basic_map.end())
	{
		//Insert component_number in list <0> and add 1 in <1>
		std::tr1::get<0>((it_internal_basic_map)->second).push_back(component_number);
		std::tr1::get<1>((it_internal_basic_map)->second) += 1;
		entry_added = true;
	}
	else
	{
		//Add key (current_service_id) and make tuple as value
		std::vector<int32_t> comp_list;
		comp_list.push_back(component_number);
		int32_t count = 1;
		std::tr1::tuple<std::vector<int32_t>, int32_t>  comp_tuple;
		std::tr1::get<0>(comp_tuple) = comp_list;
		std::tr1::get<1>(comp_tuple) = count;
		this->internal_basic_map.insert(std::pair<std::string, std::tr1::tuple<std::vector<int32_t>, int32_t> >
									(current_service_id, comp_tuple));
		entry_added = true;
	}
	return entry_added;
}

bool ComponentBalancer::balance_components(std::string service_name)
/*
*@desc: The core logic of the balancing algorithm, Total number of components are divided among the total number of healthy nodes in the cluster.  Services are divided into - source service, i.e. the services which have current  components greater than the expected number of components, -destination service, i.e. the services which have current components less than the expected number of components.
*
*@param: Service Name, i.e. account-server, container-server, etc, for which balancing is to be done.
*
*@return: bool status, true or false.
*/
{
	OSDLOG(DEBUG, "Beginning to Balance Components");
        GLUtils utils_obj;
	this->source_services.clear();
	this->destination_services.clear();
	
	std::vector<std::string> existing_healthy_service_list;
	std::vector<std::string> new_healthy_service_list;

	//Sort and unique-ify healthy_nodes_list
	sort( this->healthy_node_list.begin(), this->healthy_node_list.end());
	this->healthy_node_list.erase( unique( this->healthy_node_list.begin(), this->healthy_node_list.end()), this->healthy_node_list.end());

        for (std::vector<std::string>::iterator it = this->healthy_node_list.begin();
                                                                       it != this->healthy_node_list.end(); ++it)
	{
		//push_back service_id for each *it
		std::string healthy_service_id = utils_obj.get_service_id(*it, service_name);
		std::map<std::string, std::tr1::tuple<std::vector<int32_t>, int32_t> >::iterator it_internal_basic_map = this->internal_basic_map.begin();
		if ((it_internal_basic_map = this->internal_basic_map.find(healthy_service_id)) !=
                                                                                        this->internal_basic_map.end())
		{
			existing_healthy_service_list.push_back(healthy_service_id);
		}
		else
		{ 	
			//healthy service id for which node addition was received 
			//but components are not balanced onto it. 
			OSDLOG(INFO, "Healthy Service "<<healthy_service_id<<" doesn't has any components. Balancing Required");
			new_healthy_service_list.push_back(healthy_service_id);	
		}
	}
	for (std::vector<std::string>::iterator it = this->list_of_services_for_addition.begin(); it != 
									this->list_of_services_for_addition.end(); ++it)
	{
		new_healthy_service_list.push_back(*it);
	}
	//Sort and unique-ify healthy_service_list
	sort( new_healthy_service_list.begin(), new_healthy_service_list.end());
	new_healthy_service_list.erase( unique( new_healthy_service_list.begin(), new_healthy_service_list.end()), new_healthy_service_list.end());

/*
//--------------------
//Improvements for HYD-7724

	for (std::vector<std::string>::iterator it_new_service = new_healthy_service_list.begin(); it_new_service != 
									new_healthy_service_list.end(); ++it_new_service)
	{
		std::vector<int32_t> comp_list;
		comp_list.clear();
		int32_t count = 0;
		std::tr1::tuple<std::vector<int32_t>, int32_t>  comp_tuple;
		std::tr1::get<0>(comp_tuple) = comp_list;
		std::tr1::get<1>(comp_tuple) = count;
		this->internal_basic_map.insert(std::pair<std::string, std::tr1::tuple<std::vector<int32_t>, int32_t> >
										(*it_new_service, comp_tuple));
		OSDLOG(INFO, "Added entry for " << *it_new_service <<" in internal basic map with ZERO Components");
	}
//-------------------
*/

	int32_t total_no_of_healthy_nodes = this->healthy_node_list.size();
	
	if (total_no_of_healthy_nodes == 0 || total_no_of_healthy_nodes < 0)
	{
		OSDLOG(ERROR, "No Nodes available in Cluster to balanced components.");
		return false;
	}
	OSDLOG(INFO, "Total No of Healthy Nodes: "<<total_no_of_healthy_nodes);

	int32_t balanced_components_per_node = (this->total_no_of_components / total_no_of_healthy_nodes);
	int32_t unbalanced_components = (this->total_no_of_components % total_no_of_healthy_nodes);
	std::map<std::string, std::tr1::tuple<std::vector<int32_t>, int32_t> >::iterator it_internal_basic_map = this->internal_basic_map.begin();
	for ( ; it_internal_basic_map != this->internal_basic_map.end(); ++it_internal_basic_map)
	{
		std::string service_id = it_internal_basic_map->first;
		std::vector<std::string>::iterator index1, index2;
		index1 = std::find (this->list_of_services_for_deletion.begin(), 
						this->list_of_services_for_deletion.end(), service_id);
		index2 = std::find (this->list_of_services_for_recovery.begin(), 
						this->list_of_services_for_recovery.end(), service_id);
		if (index1 != this->list_of_services_for_deletion.end() || index2 != this->list_of_services_for_recovery.end())
		{
			OSDLOG(INFO, "Service "<<service_id<< " belongs to node being deleted or recovered");
			//Add entry in this->source_services list with exp as 0.
			std::tr1::tuple<std::string, int32_t, int32_t> t1;
			std::tr1::get<0>(t1) = service_id;
			std::tr1::get<1>(t1) = std::tr1::get<1>(it_internal_basic_map->second);
			std::tr1::get<2>(t1) = 0;
			OSDLOG(INFO, "Service "<<service_id<<" with "<< std::tr1::get<1>(t1) <<" components is to be balanced with "<<std::tr1::get<2>(t1)<<" components");
			this->source_services.push_back(t1);
		}
		else if (std::find(existing_healthy_service_list.begin(), existing_healthy_service_list.end(), service_id) != existing_healthy_service_list.end())
		{
			std::tr1::tuple<std::string, int32_t, int32_t> t1;
			std::tr1::get<0>(t1) = service_id; 
			std::tr1::get<1>(t1) = std::tr1::get<1>(it_internal_basic_map->second);
			if (unbalanced_components > 0)
			{
				std::tr1::get<2>(t1) = balanced_components_per_node + 1;
				unbalanced_components -= 1;
			}
			else
			{
				std::tr1::get<2>(t1) = balanced_components_per_node;
			}
			OSDLOG(INFO, "Service "<<service_id<<" with "<<std::tr1::get<1>(t1)<<" components is to be balanced with "<<std::tr1::get<2>(t1)<<" components");
			if (std::tr1::get<1>(t1) > std::tr1::get<2>(t1))
			{
				if (this->map_manager->is_global_planned_empty(service_name))
				{
					this->source_services.push_back(t1);
				}
				else
				{
					OSDLOG(INFO, "Ignoring "<<service_id<<" as source_service in balancing.");
				}

				/*
				//if ((this->list_of_services_for_addition.size() == 0) && (std::tr1::get<1>(t1) == balanced_components_per_node)) //Do not initate balancing for just unbalanced_components.
				if ((this->list_of_services_for_addition.size() == 0) and ((this->list_of_services_for_recovery.size() != 0) or (this->list_of_services_for_deletion.size() != 0)))
				{
					//Fix for HYD-7724: Ignoring unnecessary balancing of healthy nodes in recovery.
					OSDLOG(INFO, "Ignoring "<<service_id<<" as source_service in balancing for node recovery/deletion as it belongs to a healthy node.");
				}
				else
				{
					this->source_services.push_back(t1);
				}
				*/
			}
			else if (std::tr1::get<1>(t1) < std::tr1::get<2>(t1))
			{
				this->destination_services.push_back(t1);
			}
		}
		/*
		//Improvement for HYD-7724
		else
		{
			std::tr1::tuple<std::string, int32_t, int32_t> t1;
			std::tr1::get<0>(t1) = service_id; 
			std::tr1::get<1>(t1) = std::tr1::get<1>(it_internal_basic_map->second);
			if (unbalanced_components > 0)
			{
				std::tr1::get<2>(t1) = balanced_components_per_node + 1;
				unbalanced_components -= 1;
			}
			else
			{
				std::tr1::get<2>(t1) = balanced_components_per_node;
			}
			if (std::tr1::get<1>(t1) == 0)
			{
				OSDLOG(INFO, "Service "<<service_id<<" with "<<std::tr1::get<1>(t1)<<" components is to be added into cluster with "<<std::tr1::get<2>(t1)<<" components");
			}
			else
			{
				OSDLOG(INFO, "Service "<<service_id<<" with "<<std::tr1::get<1>(t1)<<" components is to be balanced with "<<std::tr1::get<2>(t1)<<" components");
			}
			if (std::tr1::get<1>(t1) > std::tr1::get<2>(t1))
			{
				if (this->list_of_services_for_addition.size() == 0)
				{
					//Fix for HYD-7724: Ignoring unnecessary balancing of healthy nodes in recovery.
					OSDLOG(INFO, "Ignoring "<<service_id<<" as source_service in balancing for node recovery/deletion as it isn't an unhealthy node");
				}
				else
				{
					this->source_services.push_back(t1);
				}
			}
			else if (std::tr1::get<1>(t1) < std::tr1::get<2>(t1))
			{
				std::string node_id = utils_obj.get_node_id(service_id);
				if ((this->list_of_services_for_addition.size() == 0) && (std::find(this->unhealthy_node_list.begin(), this->unhealthy_node_list.end(), node_id) != this->unhealthy_node_list.end()) || (std::find(this->recovered_node_list.begin(), this->recovered_node_list.end(), node_id) != this->recovered_node_list.end()))
				{
					OSDLOG(INFO, "Ignoring "<<service_id<<" as destination_service in balancing for node recovery/deletion as it is an unhealthy node");
				}
				else
				{
					this->destination_services.push_back(t1);
				}
			}
		}
	*/
	}
	//add entry for service_id for nodes being added in the cluster. Add them into list this->destination_services.
	for (std::vector<std::string>::iterator it = new_healthy_service_list.begin(); it != 
									new_healthy_service_list.end(); ++it)
	{
		std::string service_id = *it;
		std::tr1::tuple<std::string, int32_t, int32_t> t1;
		std::tr1::get<0>(t1) = service_id;
		std::tr1::get<1>(t1) = 0;
		if (unbalanced_components > 0)
		{
			std::tr1::get<2>(t1) = balanced_components_per_node + 1;
			unbalanced_components -= 1;
		}
                else
		{
			std::tr1::get<2>(t1) = balanced_components_per_node;
		}
		OSDLOG(INFO, "Service "<<service_id<<" with "<<std::tr1::get<1>(t1)<<" components is to be added into cluster with "<<std::tr1::get<2>(t1)<<" components");
		this->destination_services.push_back(t1);
	}
	if (unbalanced_components != 0)
	{
		OSDLOG(ERROR, unbalanced_components<<" Components still left to be balanced");
		return false;
	}

	//-----------------TODO - Remove Statements below. Done for Logging/testing purpose -----------------
	std::vector<std::tr1::tuple<std::string, int32_t, int32_t> >::iterator itrtr1 = this->source_services.begin();
	OSDLOG(INFO, "Source Services, Current Components, Expected Composents -------------");
	for ( ; itrtr1 != this->source_services.end(); ++itrtr1)
	{
		OSDLOG(INFO, std::tr1::get<0>(*itrtr1)<<"\t"<<std::tr1::get<1>(*itrtr1)<<"\t"<<std::tr1::get<2>(*itrtr1));
	}
	std::vector<std::tr1::tuple<std::string, int32_t, int32_t> >::iterator itrtr2 = this->destination_services.begin();
        OSDLOG(INFO, "Destination Services, Current Components, Expected Components -------------");
        for ( ; itrtr2 != this->destination_services.end(); ++itrtr2)
        {
               OSDLOG(INFO, std::tr1::get<0>(*itrtr2)<<"\t"<<std::tr1::get<1>(*itrtr2)<<"\t"<<std::tr1::get<2>(*itrtr2));
        }
	//------------------------------------------------------------------------------------------
	
/*	if ((this->source_services.size() > 0) && (this->destination_services.size() <= 0))
	{
		OSDLOG(ERROR, "No Destination Service found for Balancing components");
		return false;
	}
*/
	return true;
}

// @public interface
bool ComponentBalancer::check_if_nodes_already_recovered(std::vector<std::string> aborted_service_list)
/*
*@desc: Used to check if any node in the list of services which ABORTED the transfer of components has already been recovered or not.
*
*@param: List of services for which transfer was ABORTED.
*
*@return: True, if any node in aborted_service_list is found in unhealthy_node_list. False, otherwise.
*/
{
	GLUtils utils_obj;
	//return true if any node in aborted_service_list is found in unhealthy_node_list or recovered_node_list.
	std::vector<std::string>::iterator itr = aborted_service_list.begin();
	for ( ; itr != aborted_service_list.end(); ++itr)
        {
		std::string node_id = utils_obj.get_node_id(*itr);
		if (std::find(this->unhealthy_node_list.begin(), this->unhealthy_node_list.end(), node_id) != this->unhealthy_node_list.end() || std::find(this->recovered_node_list.begin(), this->recovered_node_list.end(), node_id) != this->recovered_node_list.end())
		{
			//return true if value present in list of unhealthy nodes or recovered nodes.
			OSDLOG(DEBUG, "Node "<<node_id<<" present in list of unhealthy/recovered nodes.");
			return true;
		}
	}
	return false;
}

// @public interface
std::pair<std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > >, bool> ComponentBalancer::balance_osd_cluster(
	std::string service_type,
	std::vector<std::string> healthy_nodes,
	std::vector<std::string> node_deletion,
	std::vector<std::string> node_recovery
)
/*
*
*@desc: 
*
*@param:
*@param:
*@param:
*@param:
*
*@return
*/
{
	boost::recursive_mutex::scoped_lock lock(this->mtx);
	OSDLOG(INFO, "Balancing OSD Cluster");
	this->map_manager->get_map(this->orig_basic_map, this->orig_transient_map, this->orig_planned_map);
	OSDLOG(DEBUG, "Basic, Transient and Planned map sizes are: " << this->orig_basic_map.size() << ", " << this->orig_transient_map.size() << ", " << this->orig_planned_map.size());
	std::map<std::string, std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> >::iterator it_basic_map = this->orig_basic_map.find(service_type);

	OSDLOG(INFO, "Size of healthy nodes received for balancing: " << healthy_nodes.size());
	std::vector<std::string> node_addition = healthy_nodes;
	std::pair<std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > >, bool> new_planned = 
					this->get_new_planned_map(service_type, node_addition, node_deletion, node_recovery, true);
	OSDLOG(INFO, "Balancing of Cluster Completed: "<< new_planned.second);
	return new_planned;
}

// @public interface
std::pair<std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > >, bool> ComponentBalancer::get_new_planned_map(
						std::string service_type, 
						std::vector<std::string> node_addition, 
						std::vector<std::string> node_deletion, 
						std::vector<std::string> node_recovery,
						bool flag
						)
/*
*@desc: One of the Entry points for balancing of components. SCI calls for a new planned map when in OSD Cluster,  Nodes are to be added, deleted or recovered. 
*
*@param: Service Type for which planned map is requested, i.e. account-server, container-server, etc.
*@param: List of Services to be added into OSD Cluster
*@param: List of services to be deleted from OSD cluster
*@param: List of services to be recovered.
*
*@return: Internal Planned map corresponsing to the imput parameters of the following format:
*	    Key			Value
*	    Source_SID		[(X1, Destination_CIR)......(Xn, Destination_CIR)]
*/
{
	boost::recursive_mutex::scoped_lock lock(this->mtx);
	if (!(flag))
	{
		OSDLOG(DEBUG, "Receving map from Component_Manager");
		map_manager->get_map(this->orig_basic_map, this->orig_transient_map, this->orig_planned_map);
	}
	OSDLOG(INFO, "Preparing new planned map for "<<service_type);
	this->internal_planned_map.clear();
	GLUtils utils_obj;
	this->list_of_services_for_addition.clear();
	this->list_of_services_for_addition = node_addition;
	
	//Sort and unique-ify list_of_services_for_addition
	sort( this->list_of_services_for_addition.begin(), this->list_of_services_for_addition.end());
	this->list_of_services_for_addition.erase( unique( this->list_of_services_for_addition.begin(), this->list_of_services_for_addition.end()), this->list_of_services_for_addition.end());

	std::vector<std::string>::iterator itr1 = this->list_of_services_for_addition.begin();
	for ( ; itr1 != this->list_of_services_for_addition.end(); ++itr1)
	{	
		std::string node_id = utils_obj.get_node_id(*itr1);
		//remove node entry from unhealthy node list in case node addition comes for same node.
		std::vector<std::string>::iterator unhealthy_itr = std::find(this->unhealthy_node_list.begin(), this->unhealthy_node_list.end(), node_id);
		if(unhealthy_itr != this->unhealthy_node_list.end())
		{
			this->unhealthy_node_list.erase(unhealthy_itr);
		}
		//remove node entry from recovered node list in case node addition comes for same node.
		std::vector<std::string>::iterator recovered_itr = std::find(this->recovered_node_list.begin(), this->recovered_node_list.end(), node_id);
		if(recovered_itr != this->recovered_node_list.end())
		{
			this->recovered_node_list.erase(recovered_itr);
		}
		//add node entry in healthy_node_list
		if (std::find(this->healthy_node_list.begin(), this->healthy_node_list.end(), node_id) == 
											this->healthy_node_list.end())
		{
			this->healthy_node_list.push_back(node_id);	
		}

	}
	if (this->list_of_services_for_addition.size() > 0) //Read Node Info File Records and populate memory for new nodes.
	{
		this->node_info_file_records.empty(); //Clear the records that were previously collected from node info file
		char const * const osd_config = ::getenv("OBJECT_STORAGE_CONFIG");
		boost::shared_ptr<osm_info_file_manager::NodeInfoFileReadHandler> node_info_file_reader;
		if(osd_config)
		{

			node_info_file_reader.reset(new osm_info_file_manager::NodeInfoFileReadHandler(this->node_info_file_path, 
						boost::bind(&ComponentBalancer::push_node_info_file_records_into_memory, this, _1)));
		}
		else
		{
			node_info_file_reader.reset(new osm_info_file_manager::NodeInfoFileReadHandler(CONFIG_FILESYSTEM,
						boost::bind(&ComponentBalancer::push_node_info_file_records_into_memory, this, _1)));
		}
		node_info_file_reader->recover_record(); //brings node_info_file records into memory
	}

	this->list_of_services_for_deletion.clear();
	this->list_of_services_for_deletion = node_deletion;
	
	//Sort and unique-ify list_of_services_for_deletion
	sort( this->list_of_services_for_deletion.begin(), this->list_of_services_for_deletion.end());
	this->list_of_services_for_deletion.erase( unique( this->list_of_services_for_deletion.begin(), this->list_of_services_for_deletion.end()), this->list_of_services_for_deletion.end());

	std::vector<std::string>::iterator itr2 = this->list_of_services_for_deletion.begin();
	for ( ; itr2 != this->list_of_services_for_deletion.end(); ++itr2)
	{	
		std::string node_id = utils_obj.get_node_id(*itr2);
		//add node in unhealthy node list in case node deletion comes for a node. 
		if (std::find(this->unhealthy_node_list.begin(), this->unhealthy_node_list.end(), node_id) == 
											this->unhealthy_node_list.end())
		{
			this->unhealthy_node_list.push_back(node_id);	
		}
		//remove node entry from healthy node list in case node deletion comes for same node.
		std::vector<std::string>::iterator t_itr = std::find(this->healthy_node_list.begin(), this->healthy_node_list.end(), node_id);
		if(t_itr != this->healthy_node_list.end())
		{
			this->healthy_node_list.erase(t_itr);
		}
	}

	this->list_of_services_for_recovery.clear();
	this->list_of_services_for_recovery = node_recovery;

	//Sort and unique-ify list_of_services_for_recovery
	sort( this->list_of_services_for_recovery.begin(), this->list_of_services_for_recovery.end());
	this->list_of_services_for_recovery.erase( unique( this->list_of_services_for_recovery.begin(), this->list_of_services_for_recovery.end()), this->list_of_services_for_recovery.end());

	std::vector<std::string>::iterator itr3 = this->list_of_services_for_recovery.begin();
	for ( ; itr3 != this->list_of_services_for_recovery.end(); ++itr3)
	{
		std::string node_id = utils_obj.get_node_id(*itr3);
		//add node in unhealthy node list in case node recovery comes for a nodei.
		if (std::find(this->unhealthy_node_list.begin(), this->unhealthy_node_list.end(), node_id) ==
                                                                                        this->unhealthy_node_list.end())
                {
			this->unhealthy_node_list.push_back(node_id);	
		}
		//remove node entry from healthy node list in case node recovery comes for same node.
		std::vector<std::string>::iterator t_itr = std::find(this->healthy_node_list.begin(), this->healthy_node_list.end(), node_id);
		if(t_itr != this->healthy_node_list.end())
		{
			this->healthy_node_list.erase(t_itr);
		}
	}
	
	//Sort and unique-ify unhealthy_nodes_list
	sort( this->unhealthy_node_list.begin(), this->unhealthy_node_list.end());
	this->unhealthy_node_list.erase( unique( this->unhealthy_node_list.begin(), this->unhealthy_node_list.end()), this->unhealthy_node_list.end());
	//Sort and unique-ify healthy_nodes_list
	sort( this->healthy_node_list.begin(), this->healthy_node_list.end());
	this->healthy_node_list.erase( unique( this->healthy_node_list.begin(), this->healthy_node_list.end()), this->healthy_node_list.end());


	this->prepare_internal_basic_map(service_type);
	this->merge_transient_into_internal_basic_map(service_type);
	this->merge_planned_into_internal_basic_map(service_type);

	
	// Check if Service ID given for node addition does not already exists in the cluster. 
	// Remove from list of node addition if it already exists.
	std::vector<std::string> node_add_list_copy = this->list_of_services_for_addition;

	this->list_of_services_for_addition.clear();
	OSDLOG(INFO, "Initial Node Addition list Size: "<<node_add_list_copy.size());
	std::vector<std::string>::iterator it_na = node_add_list_copy.begin();
	for (; it_na != node_add_list_copy.end(); ++it_na)
	{
		OSDLOG(DEBUG, "Service name in Node Addition list: "<<*it_na);
		if (this->internal_basic_map.find(*it_na) != this->internal_basic_map.end())
		{
			OSDLOG(INFO, "Service "<<*it_na<<" already exists in internal basic map, ignoring for node addition");
		}
		else
		{
			this->list_of_services_for_addition.push_back(*it_na);
			OSDLOG(INFO, "Adding service "<< *it_na << "in final Node Addition List");
		}
	}
	OSDLOG(INFO, "Final Node Addition list Size: "<<this->list_of_services_for_addition.size());
	bool ret = this->balance_components(service_type);
	if (ret)
	{
		this->transfer_components(service_type);
	}
	return std::make_pair(this->internal_planned_map, ret); 
}

void ComponentBalancer::transfer_components(std::string service_name)
/*
*@desc: The core logic of transfering components from source services to destination services. 
*
*@param - Service Name for which the components are to be transferred.
*/
{
	OSDLOG(INFO, "Beginning to transfer components");
	GLUtils utils_obj;
	std::vector<std::tr1::tuple<std::string, int32_t, int32_t> >::iterator itrtr1 = this->source_services.begin();
	std::vector<std::tr1::tuple<std::string, int32_t, int32_t> >::iterator itrtr2 = this->destination_services.begin();
	std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > >::iterator it_internal_planned_map = this->internal_planned_map.begin();
	std::map<std::string, std::tr1::tuple<std::vector<int32_t>, int32_t> >::iterator it_internal_basic_map1, it_internal_basic_map2 = this->internal_basic_map.begin();

	/*
	//Improvement for HYD-7724
	int32_t total_components_in_cluster = 0;
	for ( std::map<std::string, std::tr1::tuple<std::vector<int32_t>, int32_t> >::iterator it_internal_basic = this->internal_basic_map.begin(); it_internal_basic != this->internal_basic_map.end(); ++it_internal_basic)
	{
		OSDLOG(INFO, "Service "<<it_internal_basic->first<<" has "<<std::tr1::get<1>(it_internal_basic->second)<<" components");
		total_components_in_cluster += std::tr1::get<1>(it_internal_basic->second);
	}
	OSDLOG(INFO, "Total No of Components, currently distributed in cluster = " << total_components_in_cluster);

	if ((total_components_in_cluster == 0) && (this->source_services.size() == 0))
	*/
	if ((this->internal_basic_map.size() == 0) && (this->source_services.size() == 0))
	//Case for First Time Node Addition in OSD Cluster
	{
		OSDLOG(INFO, "Zero Components currently in Cluster & Source Service List empty");
		int32_t compnumber = 0;
		std::vector<recordStructure::ComponentInfoRecord> comp_info_records;
		for ( ; itrtr2 != this->destination_services.end(); itrtr2++)
                {
			OSDLOG(INFO, "Destination Service: "<<std::tr1::get<0>(*itrtr2)<<"\t"<<std::tr1::get<1>(*itrtr2)<<"\t"<<std::tr1::get<2>(*itrtr2));
			while((std::tr1::get<1>(*itrtr2) != std::tr1::get<2>(*itrtr2))) //while curr is != exp
			{
				if (this->add_entry_in_internal_basic_map(compnumber, std::tr1::get<0>(*itrtr2)))
				{
					// Update Global Map
					recordStructure::ComponentInfoRecord component_record;
					component_record = map_manager->get_service_info(std::tr1::get<0>(*itrtr2), 
											this->node_info_file_records);
					//std::cout<<"Adding Entry in Basic Map for "<<compnumber<<" for "<<std::tr1::get<0>(*itrtr2)<<std::endl;
					//this->map_manager->add_entry_in_basic_map(service_name, compnumber, component_record);
					comp_info_records.push_back(component_record);
					std::tr1::get<1>(*itrtr2) += 1;
					compnumber += 1;
				}
				else
				{
					OSDLOG(ERROR, "Error! Failed to Trasnfer");
					break;
				}
			}
			int type = this->service_to_record_type_map_ptr->get_type_for_service(service_name);
			this->map_manager->prepare_basic_map(service_name, comp_info_records); //update basic map. 
			this->map_manager->write_in_component_file(service_name, type); //write in component info file
		}
	}
	else
	{
		OSDLOG(INFO, "Size of Source Service:"<<this->source_services.size());
		for ( ; itrtr1 != this->source_services.end(); ++itrtr1)
		{
			OSDLOG(INFO, "SOURCE: "<<std::tr1::get<0>(*itrtr1)<<"\t"<<std::tr1::get<1>(*itrtr1)<<"\t"<<std::tr1::get<2>(*itrtr1));
			it_internal_basic_map1 = this->internal_basic_map.find(std::tr1::get<0>(*itrtr1));
			if (it_internal_basic_map1 == this->internal_basic_map.end())
			{
				OSDLOG(ERROR, "Source Service Id not found in Internal Basic Map");
				break;
			}
			if ((std::tr1::get<0>(it_internal_basic_map1->second).size() == 0) || (std::tr1::get<1>(*itrtr1) == std::tr1::get<2>(*itrtr1)))
			{
				continue;
			}
			for ( itrtr2 = this->destination_services.begin(); itrtr2 != this->destination_services.end(); ++itrtr2)
			{
				if (std::tr1::get<1>(*itrtr1) == std::tr1::get<2>(*itrtr1))
				{
					OSDLOG(INFO, "SOURCE BALANCED: "<<std::tr1::get<0>(*itrtr1)<<"\t"<<std::tr1::get<1>(*itrtr1)<<"\t"<<std::tr1::get<2>(*itrtr1));
					break;
				}
				if (std::tr1::get<1>(*itrtr2) == std::tr1::get<2>(*itrtr2))
				{
					continue;
				}

				//get to the source service key-value pair in internal basic map
				OSDLOG(INFO, "DESTINATION: "<<std::tr1::get<0>(*itrtr2)<<"\t"<<std::tr1::get<1>(*itrtr2)<<"\t"<<std::tr1::get<2>(*itrtr2));
				while ((std::tr1::get<1>(*itrtr1) != std::tr1::get<2>(*itrtr1)) && (std::tr1::get<1>(*itrtr2) != std::tr1::get<2>(*itrtr2)) && (std::tr1::get<0>(it_internal_basic_map1->second).size()) != 0)
				{
					//transfer components
					/* pop a component from front of the source service id
					   decrement the total_no_of_component_with_service in internal basic map by 1
					   decrement the current component value with source id by 1
					*/
					int32_t component_to_be_transferred = (std::tr1::get<0>(it_internal_basic_map1->second)).front();
					//OSDLOG(INFO, "Beginning to Transfer Component: "<<component_to_be_transferred);
					(std::tr1::get<0>(it_internal_basic_map1->second)).erase(std::tr1::get<0>(it_internal_basic_map1->second).begin());
					std::tr1::get<1>(it_internal_basic_map1->second) -= 1;
					std::tr1::get<1>(*itrtr1) -= 1;

					/*   increment the current component value with destination id by 1
					*/
					std::tr1::get<1>(*itrtr2) += 1;
				
					//Prepare Component Info File Record for Destination Service
					recordStructure::ComponentInfoRecord component_record;
					component_record = map_manager->get_service_info( std::tr1::get<0>(*itrtr2), this->node_info_file_records);
					
					//prepare internal planned map
					if ((it_internal_planned_map = this->internal_planned_map.find(std::tr1::get<0>(*itrtr1))) !=
												this->internal_planned_map.end())
					{
						//OSDLOG(DEBUG, "Adding component "<<component_to_be_transferred<<" in internal planned map");
						std::pair<int32_t, recordStructure::ComponentInfoRecord> p1;
						p1.first = component_to_be_transferred;
						p1.second = component_record;
						//Update Internal Planned Map
						(it_internal_planned_map->second).push_back(p1);
						//Update Global Planned Map
						map_manager->add_entry_in_global_planned_map(service_name, 
										component_to_be_transferred, component_record);
					}
					else
					{
						OSDLOG(DEBUG, "Adding component "<<component_to_be_transferred<<" in internal planned map");
						std::pair<int32_t, recordStructure::ComponentInfoRecord> p2;
						p2.first = component_to_be_transferred;
						p2.second = component_record; 
						std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > l2;
						l2.clear();
						l2.push_back(p2);
						//Update Internal Planned Map
						this->internal_planned_map.insert(std::pair<std::string, 
								std::vector<std::pair<int32_t, 
								recordStructure::ComponentInfoRecord> > >(std::tr1::get<0>(*itrtr1), l2));
						//Update Global Planned Map
						map_manager->add_entry_in_global_planned_map(service_name, 
										component_to_be_transferred, component_record);
					}
				}
				if (std::tr1::get<1>(*itrtr2) < std::tr1::get<2>(*itrtr2))
				{
					//do not update itrtr2
					OSDLOG(INFO, "DESTINATION NOT BALANCED: "<<std::tr1::get<0>(*itrtr2)<<"\t"<<std::tr1::get<1>(*itrtr2)<<"\t"<<std::tr1::get<2>(*itrtr2));
				}
				else
				{
					OSDLOG(INFO, "DESTINATION BALANCED: "<<std::tr1::get<0>(*itrtr2)<<"\t"<<std::tr1::get<1>(*itrtr2)<<"\t"<<std::tr1::get<2>(*itrtr2));
				}
			}
			OSDLOG(INFO, "SOURCE BALANCED: "<<std::tr1::get<0>(*itrtr1)<<"\t"<<std::tr1::get<1>(*itrtr1)<<"\t"<<std::tr1::get<2>(*itrtr1));
		}
	}
}

// @public interface
std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > ComponentBalancer::get_new_balancing_for_source(
					std::string service_type, 
					std::string source_service_id, 
					std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > planned_components
					)
/*
*@Desc: Balances the components for a particular Service.
*
*@param: Service Type, i.e. account-server, container-server, etc. 
*@param: Service Id for which balancing is to be done.
*@param:previously planned components from source service_ID
*
*@return - A list of pair of Component and Destination Service Component Info Record.
*/
{
	//balance components and transfer only from source_service_id passed as argument.
	boost::recursive_mutex::scoped_lock lock(this->mtx);
	this->internal_planned_map.clear();
	map_manager->get_map(this->orig_basic_map, this->orig_transient_map, this->orig_planned_map);
	OSDLOG(INFO, "Balancing for "<<source_service_id);
	
	//Remove already planned components
	std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >::iterator already_planned_itr = planned_components.begin();
	for (; already_planned_itr != planned_components.end(); ++already_planned_itr)
	{
		int32_t component_no = (*already_planned_itr).first;
		OSDLOG(DEBUG, "Removing entry for component "<<component_no<<" from global planned map");
		map_manager->remove_entry_from_global_planned_map(service_type, component_no);
	}

	this->prepare_internal_basic_map(service_type);
	this->merge_transient_into_internal_basic_map(service_type);
	this->merge_planned_into_internal_basic_map(service_type);

	this->list_of_services_for_addition.clear();
	this->list_of_services_for_deletion.clear();
	this->list_of_services_for_recovery.clear();

	if (!(this->balance_components(service_type)))
        {
                OSDLOG(ERROR, "Failed to balance components")
		//TODO:break or return empty list. 
		std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > test_list;
		test_list.clear();
		return test_list;
        }
	
	//enusre source_services list contains only single entry, that of source_service_id.
	std::vector<std::tr1::tuple<std::string, int32_t, int32_t> >::iterator itr1 = this->source_services.begin();
	for( ; itr1 != this->source_services.end(); )
	{
		if (std::tr1::get<0>(*itr1) != source_service_id)
		{
			this->source_services.erase(itr1); //remove tuple
		}
		else
		{
			++itr1;
		}
	}
        // transfer actual components from source_service_id to destination_services
	this->transfer_components(service_type);
 	
	std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > new_planned_components;
	new_planned_components.clear();
	std::map<std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > >::iterator it_int_pmap;
	if ((it_int_pmap = this->internal_planned_map.find(source_service_id)) != this->internal_planned_map.end())
	{
		new_planned_components = it_int_pmap->second;
	}
	return new_planned_components; 
}

// @public interface
void ComponentBalancer::update_recovery_dest_map(std::string destination_node, std::string service_name)
/*
*@desc: Used to update recovery destination map populated while finding target node as recovery process destination.
*
*@param: Destination Node where recovery process was running.
*@param: service for which recovery was being done. 
*/
{
			
	std::map<std::string, std::vector<std::string> >::iterator it_recovery_dest_map;
	if ((it_recovery_dest_map = this->node_recovery_dest_map.find(destination_node)) != this->node_recovery_dest_map.end())
	{
		if (std::find((it_recovery_dest_map->second).begin(), (it_recovery_dest_map->second).end(), service_name) != (it_recovery_dest_map->second).end())
		{
			//remove service_name from vector
			(it_recovery_dest_map->second).erase(
				std::remove((it_recovery_dest_map->second).begin(), 
				(it_recovery_dest_map->second).end(), 
				service_name), (it_recovery_dest_map->second).end()
				);
			if ((it_recovery_dest_map->second).size() == 0)
			{
				//remove key from map.
				this->node_recovery_dest_map.erase(destination_node);
				OSDLOG(INFO, "Removed entry for "<<destination_node<<" for recovery process of "<<service_name);
			}
		}
		else
		{
			OSDLOG(DEBUG, "No Recovery Process for "<<service_name<< " was scheduled on "<<destination_node);
		}
	}
	else
	{
		OSDLOG(DEBUG, "No Recovery Process was scheduled on "<<destination_node);
	}
}

// @public interface
std::string ComponentBalancer::get_target_node_for_recovery(std::string service_for_recovery)
/*
Get the Target node where the recovery process can be hosted.

//Param - Unhealthy Service ID whose recovery is to be done. 
//Param - List of unhealthy nodes
//Param - List of nodes where recovery process is already hosted.

//Returns - Node ID where recovery process can be hosted. 
*/
{
	//OSDLOG(INFO, "Finding target node for Recovery of "<< service_for_recovery ); 
	GLUtils utils_obj;

	std::string target_node_id = "NO_NODE_FOUND";
/*	
	//Prepare All Node List 
	char const * const osd_config = ::getenv("OBJECT_STORAGE_CONFIG");
	osm_info_file_manager::NodeInfoFileReadHandler *rhandler2;
	if(osd_config)
	{
		rhandler2 = new osm_info_file_manager::NodeInfoFileReadHandler(this->node_info_file_path);
	}
	else
	{
		rhandler2 = new osm_info_file_manager::NodeInfoFileReadHandler(CONFIG_FILESYSTEM);
	}

	std::list<std::string> tmp = rhandler2->list_node_ids();
	this->all_node_list.insert(this->all_node_list.begin(), tmp.begin(), tmp.end());
	delete rhandler2;
	
	//Sort and unique-ify all_node_list built from node_info_file
	sort( this->all_node_list.begin(), this->all_node_list.end() );
	this->all_node_list.erase( unique( this->all_node_list.begin(), this->all_node_list.end() ), this->all_node_list.end() );
	
	//Sort and unique-ify unhealthy_nodes_list
	sort( this->unhealthy_node_list.begin(), this->unhealthy_node_list.end());
	this->unhealthy_node_list.erase( unique( this->unhealthy_node_list.begin(), this->unhealthy_node_list.end()), this->unhealthy_node_list.end());

	int asize = this->all_node_list.size() + this->unhealthy_node_list.size();       
	std::vector<std::string> healthy_node_list(asize);
	std::vector<std::string>::iterator it_healthy_nodes;
	//find the healthy nodes in the system.
	it_healthy_nodes = std::set_difference(
			this->all_node_list.begin(), 
			this->all_node_list.end(), 
			this->unhealthy_node_list.begin(), 
			this->unhealthy_node_list.end(), 
			healthy_node_list.begin()
			);
	healthy_node_list.resize(it_healthy_nodes-healthy_node_list.begin());

	int bsize = healthy_node_list.size() + this->recovered_node_list.size();
	std::vector<std::string> healthy_destination_list(bsize);
	it_healthy_nodes = std::set_difference(
			healthy_node_list.begin(),
			healthy_node_list.end(),
			this->recovered_node_list.begin(),
			this->recovered_node_list.end(),
			healthy_destination_list.begin()
			);
	healthy_destination_list.resize(it_healthy_nodes-healthy_destination_list.begin());	
*/
	std::vector<std::string> healthy_destination_list = this->healthy_node_list;

	std::string node_id = utils_obj.get_node_id(service_for_recovery);
	healthy_destination_list.erase(std::remove(healthy_destination_list.begin(), healthy_destination_list.end(), node_id), healthy_destination_list.end());

	for (std::vector<std::string>::iterator itrtrtr = healthy_destination_list.begin(); itrtrtr != healthy_destination_list.end(); ++itrtrtr)
	{
		OSDLOG(DEBUG, "Healthy Nodes as candidates for recovery destination: "<< *itrtrtr);
	}
	
	std::string service_name = utils_obj.get_service_name(service_for_recovery);
	std::map<std::string, std::vector<std::string> >::iterator it_recovery_dest_map;
	for (std::vector<std::string>::iterator it = healthy_destination_list.begin(); it != healthy_destination_list.end(); it++)
	{
		if ((it_recovery_dest_map = this->node_recovery_dest_map.find(*it)) != this->node_recovery_dest_map.end())
		{
			if (std::find((it_recovery_dest_map->second).begin(), (it_recovery_dest_map->second).end(), service_name) != (it_recovery_dest_map->second).end())
			{
				OSDLOG(INFO, "Node: "<<*it<<" already has a recovery process running for "<<service_name);
				continue;
			}
			else
			{
				//insert service name in vector, return *it
				(it_recovery_dest_map->second).push_back(service_name);
				OSDLOG(INFO, "Node: "<<*it<<" found as Recovery Process Destination");
				return *it; 
			}
		}
		else
		{
			//insert *it as key and service name in value
			std::vector<std::string> value;
			value.push_back(service_name);
			this->node_recovery_dest_map.insert(std::make_pair(*it, value));
			OSDLOG(INFO, "Node: "<<*it<<" found as Recovery Process Destination");
			return *it;
		}
	}

	OSDLOG(INFO, target_node_id<<" for Recovery Process Destination");
	return target_node_id;
}

} //end of namespace component_balancer
