//this is a dummy implementation.
#include "communication/message.h"
#include "communication/message_type_enum.pb.h"
#include "communication/message_binding.pb.h"
#include "libraryUtils/object_storage_log_setup.h"

namespace comm_messages
{

HeartBeat::HeartBeat():
	msg("HEARTBEAT"),
	type(messageTypeEnum::typeEnums::HEART_BEAT)
{
}

HeartBeat::HeartBeat(std::string service_id, uint32_t sequence, int hfs_stat): 
	service_id(service_id),
	msg("HEARTBEAT"),
	type(messageTypeEnum::typeEnums::HEART_BEAT),
	sequence(sequence),	//TODO: Devesh: sequence should be calculated inside this class not from outside.
	hfs_stat(hfs_stat)
{
}

HeartBeat::HeartBeat(std::string service_id, uint32_t sequence):
	service_id(service_id),
	msg("HEARTBEAT"),
	type(messageTypeEnum::typeEnums::HEART_BEAT),
	sequence(sequence)	//TODO: Devesh: sequence should be calculated inside this class not from outside.
{
}

bool HeartBeat::serialize(std::string &payload_string)
{
	network_messages::heartBeat ob;
	ob.set_msg(this->msg);
	ob.set_sequence(this->sequence);
	ob.set_service_id(this->service_id);
	ob.set_hfs_stat(this->hfs_stat);
	return ob.SerializeToString(&payload_string);
}

bool HeartBeat::deserialize(const char * payload_string, uint32_t size){
	network_messages::heartBeat ob;

	std::string s(payload_string, size);

	if (!ob.ParseFromString(s)){
		OSDLOG(DEBUG, "Deserialization failed")
		return false;;
	};
	this->service_id = ob.service_id();
	this->sequence = ob.sequence();
	this->hfs_stat = ob.hfs_stat();
	this->msg = ob.msg();

	return true;
}

uint64_t HeartBeat::get_fd(){
	return this->fd;
}

void HeartBeat::set_fd(uint64_t fd){
	this->fd = fd;
}

std::string HeartBeat::get_service_id(){
	return this->service_id;
}

void HeartBeat::set_service_id(std::string service_id){
	this->service_id = service_id;
}

void HeartBeat::set_hfs_stat(int hfs_stat)
{
	this->hfs_stat = hfs_stat;
}

int HeartBeat::get_hfs_stat() const
{
	return this->hfs_stat;
}

const uint64_t HeartBeat::get_type(){
	return this->type;
}

uint32_t HeartBeat::get_sequence()
{
	return this->sequence;
}

std::string HeartBeat::get_msg()
{
	return this->msg;
}

HeartBeatAck::HeartBeatAck():
	type(messageTypeEnum::typeEnums::HEART_BEAT_ACK)
{
}

HeartBeatAck::HeartBeatAck(uint32_t sequence):
	msg("HEARTBEAT"),
	type(messageTypeEnum::typeEnums::HEART_BEAT_ACK),
	sequence(sequence)
{
	this->node_stat = 110; //Invalid Node
}

HeartBeatAck::HeartBeatAck(uint32_t sequence, int node_stat):
	msg("HEARTBEAT"),
	type(messageTypeEnum::typeEnums::HEART_BEAT_ACK),
	sequence(sequence),
	node_stat(node_stat)
{
}

bool HeartBeatAck::serialize(std::string & payload_string)
{
	network_messages::heartBeatAck ob;
	ob.set_sequence(this->sequence);
	ob.set_node_stat(this->node_stat);
	ob.set_msg(this->msg);
	return ob.SerializeToString(&payload_string);
}

bool HeartBeatAck::deserialize(const char * payload_string, uint32_t size)
{
	network_messages::heartBeatAck ob;

	std::string s(payload_string, size);

	if (!ob.ParseFromString(s)){
		OSDLOG(DEBUG, "Deserialization failed")
		return false;;
	};
	this->msg = ob.msg();
	this->sequence = ob.sequence();
	this->node_stat = ob.node_stat();

	return true;
}

uint64_t HeartBeatAck::get_fd(){
	return this->fd;
}

void HeartBeatAck::set_fd(uint64_t fd){
	this->fd = fd;
}

std::string HeartBeatAck::get_msg(){
	return this->msg;
}

const uint64_t HeartBeatAck::get_type(){
	return this->type;
}

uint32_t HeartBeatAck::get_sequence()
{
	return this->sequence;
}

int HeartBeatAck::get_node_stat() const
{
	return this->node_stat;
}

void HeartBeatAck::set_node_stat(int node_stat)
{
	this->node_stat = node_stat;
}

GetGlobalMap::GetGlobalMap():
	type(messageTypeEnum::typeEnums::GET_GLOBAL_MAP)
{
}

GetGlobalMap::GetGlobalMap(std::string service_id):
	service_id(service_id),
	type(messageTypeEnum::typeEnums::GET_GLOBAL_MAP)
{
}

bool GetGlobalMap::serialize(std::string & payload_string){
	network_messages::GetGlobalMap ob;
	ob.set_service_id(this->service_id);
	return ob.SerializeToString(&payload_string);
}

bool GetGlobalMap::deserialize(const char * payload_string, uint32_t size){
	network_messages::GetGlobalMap ob;

	std::string s(payload_string, size);

	if (!ob.ParseFromString(s)){
		OSDLOG(DEBUG, "Deserialization failed")
		return false;;
	};
	this->service_id = ob.service_id();

	return true;
}

uint64_t GetGlobalMap::get_fd(){
	return this->fd;
}

void GetGlobalMap::set_fd(uint64_t fd){
	this->fd = fd;
}

std::string GetGlobalMap::get_service_id(){
	return this->service_id;
}

void GetGlobalMap::set_service_id(std::string service_id){
	this->service_id = service_id;
}

const uint64_t GetGlobalMap::get_type(){
	return this->type;
}

GlobalMapInfo::GlobalMapInfo():
	type(messageTypeEnum::typeEnums::GLOBAL_MAP_INFO),
	status(true)
{
}

GlobalMapInfo::GlobalMapInfo(
	std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> container_pair,
	std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> account_pair,
	std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> updater_pair,
	std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> object_pair,
	float version
):
	container_pair(container_pair),
	account_pair(account_pair),
	updater_pair(updater_pair),
	object_pair(object_pair),
	type(messageTypeEnum::typeEnums::GLOBAL_MAP_INFO),
	version(version),
	status(true)
{
}

bool GlobalMapInfo::serialize(std::string & payload_string)
{
	network_messages::GlobalMapInfo ob;

	ob.set_version(this->version);
	ob.set_status(this->status);


	network_messages::GlobalMapInfo_service * container = ob.mutable_container();
	network_messages::GlobalMapInfo_service * account = ob.mutable_account();
	network_messages::GlobalMapInfo_service * updater = ob.mutable_updater();
	network_messages::GlobalMapInfo_service * object = ob.mutable_object();

	{
		std::vector<recordStructure::ComponentInfoRecord>::iterator it = std::tr1::get<0>(this->container_pair).begin();
		for (; it != std::tr1::get<0>(this->container_pair).end(); ++it)
		{
			network_messages::service_obj * service_list = container->add_service_list();
			service_list->set_service_id((*it).get_service_id());
			service_list->set_ip((*it).get_service_ip());
			service_list->set_port((*it).get_service_port());
		}
		container->set_version(std::tr1::get<1>(this->container_pair));
	}

	{
		std::vector<recordStructure::ComponentInfoRecord>::iterator it = std::tr1::get<0>(this->account_pair).begin();
		for (; it != std::tr1::get<0>(this->account_pair).end(); ++it)
		{
			network_messages::service_obj * service_list = account->add_service_list();
			service_list->set_service_id((*it).get_service_id());
			service_list->set_ip((*it).get_service_ip());
			service_list->set_port((*it).get_service_port());
		}
		account->set_version(std::tr1::get<1>(this->account_pair));

	}

	{
		std::vector<recordStructure::ComponentInfoRecord>::iterator it = std::tr1::get<0>(this->updater_pair).begin();
		for (; it != std::tr1::get<0>(this->updater_pair).end(); ++it)
		{
			network_messages::service_obj * service_list = updater->add_service_list();
			service_list->set_service_id((*it).get_service_id());
			service_list->set_ip((*it).get_service_ip());
			service_list->set_port((*it).get_service_port());
		}
		updater->set_version(std::tr1::get<1>(this->updater_pair));
	}

	{
		std::vector<recordStructure::ComponentInfoRecord>::iterator it = std::tr1::get<0>(this->object_pair).begin();
		for (; it != std::tr1::get<0>(this->object_pair).end(); ++it)
		{
			network_messages::service_obj * service_list = object->add_service_list();
			service_list->set_service_id((*it).get_service_id());
			service_list->set_ip((*it).get_service_ip());
			service_list->set_port((*it).get_service_port());
		}
		object->set_version(std::tr1::get<1>(this->object_pair));
	}

	return ob.SerializeToString(&payload_string);
}

bool GlobalMapInfo::deserialize(const char * payload_string, uint32_t size)
{
	network_messages::GlobalMapInfo ob;
	std::string s(payload_string, size);
	if (!ob.ParseFromString(s)){
		OSDLOG(DEBUG, "Deserialization failed")
		return false;;
	};

	this->version = ob.version();
	this->status = ob.status();


	network_messages::GlobalMapInfo_service container = ob.container();
	network_messages::GlobalMapInfo_service account = ob.account();
	network_messages::GlobalMapInfo_service updater = ob.updater();
	network_messages::GlobalMapInfo_service object = ob.object();

	{
		std::vector<recordStructure::ComponentInfoRecord> service_object_list;
		for (int i = 0; i < container.service_list_size(); i++)
		{
			recordStructure::ComponentInfoRecord serv_obj;
			network_messages::service_obj service_obj = container.service_list(i);
			serv_obj.set_service_id(service_obj.service_id());
			serv_obj.set_service_ip(service_obj.ip());
			serv_obj.set_service_port(service_obj.port());
			service_object_list.push_back(serv_obj);
		}
		float container_version = container.version();
		this->container_pair = std::tr1::make_tuple(service_object_list, container_version);
	}

	{
		std::vector<recordStructure::ComponentInfoRecord> service_object_list;
		for (int i = 0; i < account.service_list_size(); i++)
		{
			recordStructure::ComponentInfoRecord serv_obj;
			network_messages::service_obj service_obj = account.service_list(i);
			serv_obj.set_service_id(service_obj.service_id());
			serv_obj.set_service_ip(service_obj.ip());
			serv_obj.set_service_port(service_obj.port());
			service_object_list.push_back(serv_obj);
		}
		float account_version = account.version();
		this->account_pair = std::tr1::make_tuple(service_object_list, account_version);
	}

	{
		std::vector<recordStructure::ComponentInfoRecord> service_object_list;
		for (int i = 0; i < updater.service_list_size(); i++)
		{
			recordStructure::ComponentInfoRecord serv_obj;
			network_messages::service_obj service_obj = updater.service_list(i);
			serv_obj.set_service_id(service_obj.service_id());
			serv_obj.set_service_ip(service_obj.ip());
			serv_obj.set_service_port(service_obj.port());
			service_object_list.push_back(serv_obj);
		}
		float updater_version = updater.version();
		this->updater_pair = std::tr1::make_tuple(service_object_list, updater_version);
	}

	{
		std::vector<recordStructure::ComponentInfoRecord> service_object_list;
		for (int i = 0; i < object.service_list_size(); i++)
		{
			recordStructure::ComponentInfoRecord serv_obj;
			network_messages::service_obj service_obj = object.service_list(i);
			serv_obj.set_service_id(service_obj.service_id());
			serv_obj.set_service_ip(service_obj.ip());
			serv_obj.set_service_port(service_obj.port());
			service_object_list.push_back(serv_obj);
		}
		float object_version = object.version();
		this->object_pair = std::tr1::make_tuple(service_object_list, object_version);
	}

	return true;
}

bool GlobalMapInfo::get_status(){
	return this->status;
}

void GlobalMapInfo::set_status(bool status)
{
	this->status = status;
}

float GlobalMapInfo::get_version()
{
	return this->version;
}

void GlobalMapInfo::set_version(float version)
{
	this->version = version;
}

/*void GlobalMapInfo::add_service_object_in_container(boost::shared_ptr<ServiceObject> serv_obj)
{
	this->container_pair.first->push_back(serv_obj);
}

void GlobalMapInfo::add_service_object_in_account(boost::shared_ptr<ServiceObject> serv_obj)
{
	this->account_pair.first->push_back(serv_obj);
}

void GlobalMapInfo::add_service_object_in_updater(boost::shared_ptr<ServiceObject> serv_obj)
{
	this->updater_pair.first->push_back(serv_obj);
}

void GlobalMapInfo::add_service_object_in_objectService(boost::shared_ptr<ServiceObject> serv_obj)
{
	this->object_pair.first->push_back(serv_obj);
}*/

void GlobalMapInfo::set_version_in_container(float version)
{
	std::tr1::get<1>(this->container_pair) = version;
}

void GlobalMapInfo::set_version_in_account(float version)
{
	std::tr1::get<1>(this->account_pair) = version;
}

void GlobalMapInfo::set_version_in_updater(float version)
{
	std::tr1::get<1>(this->updater_pair) = version;
}

void GlobalMapInfo::set_version_in_object(float version)
{
	std::tr1::get<1>(this->object_pair) = version;
}

float GlobalMapInfo::get_version_in_container()
{
	return std::tr1::get<1>(this->container_pair);
}

float GlobalMapInfo::get_version_in_account()
{
	return std::tr1::get<1>(this->account_pair);
}

float GlobalMapInfo::get_version_in_updater()
{
	return std::tr1::get<1>(this->updater_pair);
}

float GlobalMapInfo::get_version_in_object()
{
	return std::tr1::get<1>(this->object_pair);
}

std::vector<recordStructure::ComponentInfoRecord> GlobalMapInfo::get_service_object_of_container()
{
	return std::tr1::get<0>(this->container_pair);
}

std::vector<recordStructure::ComponentInfoRecord> GlobalMapInfo::get_service_object_of_account()
{
	return std::tr1::get<0>(this->account_pair);
}

std::vector<recordStructure::ComponentInfoRecord> GlobalMapInfo::get_service_object_of_updater()
{
	return std::tr1::get<0>(this->updater_pair);
}

std::vector<recordStructure::ComponentInfoRecord> GlobalMapInfo::get_service_object_of_objectService()
{
	return std::tr1::get<0>(this->object_pair);
}

uint64_t GlobalMapInfo::get_fd()
{
	return this->fd;
}

void GlobalMapInfo::set_fd(uint64_t fd){
	this->fd = fd;
}

const uint64_t GlobalMapInfo::get_type(){
	return this->type;
}

GetServiceComponent::GetServiceComponent():
	type(messageTypeEnum::typeEnums::GET_SERVICE_COMPONENT)
{
}

GetServiceComponent::GetServiceComponent(std::string service_id):
	service_id(service_id),
	type(messageTypeEnum::typeEnums::GET_SERVICE_COMPONENT)
{
}

bool GetServiceComponent::serialize(std::string & payload_string)
{
	network_messages::GetServiceComponent ob;
	ob.set_service_id(this->service_id);
	return ob.SerializeToString(&payload_string);
}

bool GetServiceComponent::deserialize(const char * payload_string, uint32_t size){
	network_messages::GetServiceComponent ob;
	std::string s(payload_string, size);
	if (!ob.ParseFromString(s)){
		OSDLOG(DEBUG, "Deserialization failed")
		return false;;
	};
	this->service_id = ob.service_id();

	return true;
}

uint64_t GetServiceComponent::get_fd(){
	return this->fd;
}

void GetServiceComponent::set_fd(uint64_t fd){
	this->fd = fd;
}

std::string GetServiceComponent::get_service_id(){
	return this->service_id;
}

void GetServiceComponent::set_service_id(std::string service_id){
	this->service_id = service_id;
}

const uint64_t GetServiceComponent::get_type(){
	return this->type;
}

GetServiceComponentAck::GetServiceComponentAck():
	type(messageTypeEnum::typeEnums::GET_SERVICE_COMPONENT_ACK)
{
}

GetServiceComponentAck::GetServiceComponentAck(
	std::vector<uint32_t> component_list,
	boost::shared_ptr<ErrorStatus> error_status
):
	component_list(component_list),
	error_status(error_status),
	type(messageTypeEnum::typeEnums::GET_SERVICE_COMPONENT_ACK)
{
}

bool GetServiceComponentAck::serialize(std::string & payload_string)
{
	network_messages::GetServiceComponentAck ob;
	for (std::vector<uint32_t>::iterator it = this->component_list.begin(); it != this->component_list.end(); ++it)
	{
		ob.add_component_list(*it);
	}

	network_messages::errorStatus * error_status = ob.mutable_err();
	error_status->set_msg(this->error_status->get_msg());
	error_status->set_code(this->error_status->get_code());

	return ob.SerializeToString(&payload_string);
}

bool GetServiceComponentAck::deserialize(const char * payload_string, uint32_t size)
{
	network_messages::GetServiceComponentAck ob;
	std::string s(payload_string, size);
	if (!ob.ParseFromString(s)){
		OSDLOG(DEBUG, "Deserialization failed")
		return false;;
	};
	for (int i = 0; i < ob.component_list_size(); i++)
	{
		this->component_list.push_back(ob.component_list(i));
	}
	this->error_status.reset(new ErrorStatus());
	this->error_status->set_code(ob.err().code());
	this->error_status->set_msg(ob.err().msg());

	return true;
}

uint64_t GetServiceComponentAck::get_fd()
{
	return this->fd;
}

void GetServiceComponentAck::set_fd(uint64_t fd)
{
	this->fd = fd;
}

const uint64_t GetServiceComponentAck::get_type()
{
	return this->type;
}

void GetServiceComponentAck::set_component_list(std::vector<uint32_t> component_list)
{
	this->component_list = component_list;
}

std::vector<uint32_t> GetServiceComponentAck::get_component_list()
{
	return this->component_list;
}

OsdStartMonitoring::OsdStartMonitoring():
	type(messageTypeEnum::typeEnums::OSD_START_MONITORING)
{
}

OsdStartMonitoring::OsdStartMonitoring(std::string service_id, uint32_t port, std::string ip):
	service_id(service_id),
	port(port),
	ip(ip),
	type(messageTypeEnum::typeEnums::OSD_START_MONITORING)
{
}

bool OsdStartMonitoring::serialize(std::string & payload_string){
	network_messages::OsdStartMonitoring ob;
	ob.set_service_id(this->service_id);
	ob.set_port(this->port);
	ob.set_ip(this->ip);
	return ob.SerializeToString(&payload_string);
}

bool OsdStartMonitoring::deserialize(const char * payload_string, uint32_t size){
	network_messages::OsdStartMonitoring ob;
	std::string s(payload_string, size);
	if (!ob.ParseFromString(s)){
		OSDLOG(DEBUG, "Deserialization failed")
		return false;;
	};
	this->service_id = ob.service_id();
	this->ip = ob.ip();
	this->port = ob.port();

	return true;
}

uint64_t OsdStartMonitoring::get_fd(){
	return this->fd;
}

void OsdStartMonitoring::set_fd(uint64_t fd){
	this->fd = fd;
}

std::string OsdStartMonitoring::get_ip()
{
	return this->ip;
}

void OsdStartMonitoring::set_ip(std::string ip)
{
	this->ip = ip;
}

uint32_t OsdStartMonitoring::get_port()
{
	return this->port;
}

void OsdStartMonitoring::set_port(uint32_t port)
{
	this->port = port;
}

std::string OsdStartMonitoring::get_service_id(){
	return this->service_id;
}

void OsdStartMonitoring::set_service_id(std::string service_id){
	this->service_id = service_id;
}

const uint64_t OsdStartMonitoring::get_type(){
	return this->type;
}

OsdStartMonitoringAck::OsdStartMonitoringAck():
	type(messageTypeEnum::typeEnums::OSD_START_MONITORING_ACK)
{
}

OsdStartMonitoringAck::OsdStartMonitoringAck(std::string service_id, boost::shared_ptr<ErrorStatus> error_status):
	service_id(service_id),
	error_status(error_status),
	type(messageTypeEnum::typeEnums::OSD_START_MONITORING_ACK)
{
}

bool OsdStartMonitoringAck::serialize(std::string & payload_string){
	network_messages::OsdStartMonitoringAck ob;
	ob.set_service_id(this->service_id);
	network_messages::errorStatus * error_status = ob.mutable_error();
	error_status->set_msg(this->error_status->get_msg());
	error_status->set_code(this->error_status->get_code());
	return ob.SerializeToString(&payload_string);
}

bool OsdStartMonitoringAck::deserialize(const char * payload_string, uint32_t size){
	network_messages::OsdStartMonitoringAck ob;
	std::string s(payload_string, size);
	if (!ob.ParseFromString(s)){
		OSDLOG(DEBUG, "Deserialization failed")
		return false;;
	};
	this->service_id = ob.service_id();
	network_messages::errorStatus error = ob.error();
	this->error_status.reset(new ErrorStatus(
						error.code(),
						error.msg()
					)
				);

	return true;
}

uint64_t OsdStartMonitoringAck::get_fd(){
	return this->fd;
}

void OsdStartMonitoringAck::set_fd(uint64_t fd){
	this->fd = fd;
}

std::string OsdStartMonitoringAck::get_service_id(){
	return this->service_id;
}

void OsdStartMonitoringAck::set_service_id(std::string service_id){
	this->service_id = service_id;
}

const uint64_t OsdStartMonitoringAck::get_type(){
	return this->type;
}

boost::shared_ptr<ErrorStatus> OsdStartMonitoringAck::get_error_status()
{
	return this->error_status;
}

void OsdStartMonitoringAck::set_error_status(boost::shared_ptr<ErrorStatus> error_status)
{
	this->error_status = error_status;
}

LocalLeaderStartMonitoring::LocalLeaderStartMonitoring():
	type(messageTypeEnum::typeEnums::LL_START_MONITORING)
{
}

LocalLeaderStartMonitoring::LocalLeaderStartMonitoring(std::string service_id):
	service_id(service_id),
	type(messageTypeEnum::typeEnums::LL_START_MONITORING)
{
}

bool LocalLeaderStartMonitoring::serialize(std::string & payload_string){
	network_messages::LocalLeaderStartMonitoring ob;
	ob.set_service_id(this->service_id);
	return ob.SerializeToString(&payload_string);
}

bool LocalLeaderStartMonitoring::deserialize(const char * payload_string, uint32_t size){
	network_messages::LocalLeaderStartMonitoring ob;
	std::string s(payload_string, size);
	if (!ob.ParseFromString(s)){
		OSDLOG(DEBUG, "Deserialization failed")
		return false;;
	};
	this->service_id = ob.service_id();

	return true;
}

uint64_t LocalLeaderStartMonitoring::get_fd(){
	return this->fd;
}

void LocalLeaderStartMonitoring::set_fd(uint64_t fd){
	this->fd = fd;
}

std::string LocalLeaderStartMonitoring::get_service_id(){
	return this->service_id;
}

void LocalLeaderStartMonitoring::set_service_id(std::string service_id){
	this->service_id = service_id;
}

const uint64_t LocalLeaderStartMonitoring::get_type(){
	return this->type;
}

LocalLeaderStartMonitoringAck::LocalLeaderStartMonitoringAck():
	type(messageTypeEnum::typeEnums::LL_START_MONITORING_ACK),
	status(true)
{
}

LocalLeaderStartMonitoringAck::LocalLeaderStartMonitoringAck(std::string service_id):
	service_id(service_id),
	type(messageTypeEnum::typeEnums::LL_START_MONITORING_ACK),
	status(true)
{
}

bool LocalLeaderStartMonitoringAck::serialize(std::string & payload_string)
{
	network_messages::LocalLeaderStartMonitoringAck ob;
	ob.set_service_id(this->service_id);
	ob.set_status(this->status);
	return ob.SerializeToString(&payload_string);
}

bool LocalLeaderStartMonitoringAck::deserialize(const char * payload_string, uint32_t size){
	network_messages::LocalLeaderStartMonitoringAck ob;
	std::string s(payload_string, size);
	if (!ob.ParseFromString(s)){
		OSDLOG(DEBUG, "Deserialization failed")
		return false;;
	};
	this->service_id = ob.service_id();
	this->status = ob.status();

	return true;
}

uint64_t LocalLeaderStartMonitoringAck::get_fd(){
	return this->fd;
}

void LocalLeaderStartMonitoringAck::set_fd(uint64_t fd){
	this->fd = fd;
}

std::string LocalLeaderStartMonitoringAck::get_service_id(){
	return this->service_id;
}

void LocalLeaderStartMonitoringAck::set_service_id(std::string service_id){
	this->service_id = service_id;
}

const uint64_t LocalLeaderStartMonitoringAck::get_type(){
	return this->type;
}

bool LocalLeaderStartMonitoringAck::get_status(){
	return this->status;
}

void LocalLeaderStartMonitoringAck::set_status(bool status){
	this->status = status;
}

RecvProcStartMonitoring::RecvProcStartMonitoring():
	type(messageTypeEnum::typeEnums::RECV_PROC_START_MONITORING)
{
}

RecvProcStartMonitoring::RecvProcStartMonitoring(std::string proc_id):
	type(messageTypeEnum::typeEnums::RECV_PROC_START_MONITORING),
	proc_id(proc_id)
{
}

bool RecvProcStartMonitoring::serialize(std::string & payload_string){
	network_messages::RecvProcStartMonitoring ob;
	ob.set_proc_id(this->proc_id);
	return ob.SerializeToString(&payload_string);
}

bool RecvProcStartMonitoring::deserialize(const char * payload_string, uint32_t size){
	network_messages::RecvProcStartMonitoring ob;
	std::string s(payload_string, size);
	if (!ob.ParseFromString(s)){
		OSDLOG(DEBUG, "Deserialization failed")
		return false;;
	};
	this->proc_id = ob.proc_id();

	return true;
}

uint64_t RecvProcStartMonitoring::get_fd(){
	return this->fd;
}

void RecvProcStartMonitoring::set_fd(uint64_t fd){
	this->fd = fd;
}

std::string RecvProcStartMonitoring::get_proc_id(){
	return this->proc_id;
}

void RecvProcStartMonitoring::set_proc_id(std::string proc_id){
	this->proc_id = proc_id;
}

const uint64_t RecvProcStartMonitoring::get_type(){
	return this->type;
}

RecvProcStartMonitoringAck::RecvProcStartMonitoringAck():
	type(messageTypeEnum::typeEnums::RECV_PROC_START_MONITORING_ACK)
{
}

RecvProcStartMonitoringAck::RecvProcStartMonitoringAck(
	std::map<recordStructure::ComponentInfoRecord, std::list<uint32_t> > service_map,
	recordStructure::ComponentInfoRecord source_service,
	bool status
):
	type(messageTypeEnum::typeEnums::RECV_PROC_START_MONITORING_ACK),
	service_map(service_map),
	source_service(source_service),
	status(status)
{
}

bool RecvProcStartMonitoringAck::serialize(std::string & payload_string)
{
	network_messages::RecvProcStartMonitoringAck ob;
	std::map<recordStructure::ComponentInfoRecord, std::list<uint32_t> >::iterator it;
	for (it=this->service_map.begin(); it != this->service_map.end(); ++it)
	{
		network_messages::RecvProcStartMonitoringAck_entry * service_entry = ob.add_service_name();
		network_messages::service_obj * serv_obj = service_entry->mutable_service();
		serv_obj->set_service_id((it->first).get_service_id());
		serv_obj->set_ip(it->first.get_service_ip());
		serv_obj->set_port(it->first.get_service_port());
		for (std::list<uint32_t>::iterator lit=it->second.begin(); lit != it->second.end(); ++lit)
		{
			service_entry->add_component_list(*lit);
		}
	}

	network_messages::service_obj * source_service_object = ob.mutable_source_service();
	source_service_object->set_service_id(this->source_service.get_service_id());
	source_service_object->set_ip(this->source_service.get_service_ip());
	source_service_object->set_port(this->source_service.get_service_port());
	ob.set_status(this->status);
	return ob.SerializeToString(&payload_string);

}

bool RecvProcStartMonitoringAck::deserialize(const char * payload_string, uint32_t size)
{
	network_messages::RecvProcStartMonitoringAck ob;
	std::string s(payload_string, size);
	if (!ob.ParseFromString(s)){
		OSDLOG(DEBUG, "Deserialization failed")
		return false;;
	};
	this->status = ob.status();

	network_messages::service_obj source_service_object = ob.source_service();
	this->source_service.set_service_id(source_service_object.service_id());
	this->source_service.set_service_ip(source_service_object.ip());
	this->source_service.set_service_port(source_service_object.port());
	for (int i = 0; i < ob.service_name_size(); ++i)
	{
		network_messages::RecvProcStartMonitoringAck_entry entry = ob.service_name(i);
		network_messages::service_obj entry_service_object = entry.service();

		recordStructure::ComponentInfoRecord service_obj;

		service_obj.set_service_id(entry_service_object.service_id());
		service_obj.set_service_ip(entry_service_object.ip());
		service_obj.set_service_port(entry_service_object.port());
		std::list<uint32_t> comp_list;
		for (int j = 0; j < entry.component_list_size(); ++j)
		{
			uint32_t comp = entry.component_list(j);
			comp_list.push_back(comp);
		}
		this->service_map.insert(std::make_pair(service_obj, comp_list));
	}

	return true;
}

uint64_t RecvProcStartMonitoringAck::get_fd(){
	return this->fd;
}

void RecvProcStartMonitoringAck::set_fd(uint64_t fd){
	this->fd = fd;
}

const uint64_t RecvProcStartMonitoringAck::get_type(){
	return this->type;
}

recordStructure::ComponentInfoRecord RecvProcStartMonitoringAck::get_source_service_object()
{
	return this->source_service;
}

void RecvProcStartMonitoringAck::set_source_service_object(recordStructure::ComponentInfoRecord serv)
{
	this->source_service = serv;
}

std::map<recordStructure::ComponentInfoRecord, std::list<uint32_t> > RecvProcStartMonitoringAck::get_service_map()
{
	return this->service_map;
}

void RecvProcStartMonitoringAck::set_service_map(std::map<recordStructure::ComponentInfoRecord, std::list<uint32_t> > serv)
{
	this->service_map = serv;
}

CompTransferInfo::CompTransferInfo():
	type(messageTypeEnum::typeEnums::COMP_TRANSFER_INFO)
{
}

CompTransferInfo::CompTransferInfo(
	std::string service_id,
	std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > comp_service_list

):
	type(messageTypeEnum::typeEnums::COMP_TRANSFER_INFO),
	service_id(service_id),
	comp_service_list(comp_service_list)
{
}

bool CompTransferInfo::serialize(std::string & payload_string)
{
	network_messages::CompTransferInfo ob;

	ob.set_service_id(this->service_id);
	std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> >::iterator it;
	for (it = this->comp_service_list.begin(); it != this->comp_service_list.end(); ++it)
	{
		network_messages::CompTransferInfo_pair * pair = ob.add_component_service_pair();
		network_messages::service_obj * serv_obj = pair->mutable_dest_service();

		pair->set_component((*it).first);
		serv_obj->set_service_id((*it).second.get_service_id());
		serv_obj->set_ip((*it).second.get_service_ip());
		serv_obj->set_port((*it).second.get_service_port());
	}
	return ob.SerializeToString(&payload_string);
}

bool CompTransferInfo::deserialize(const char * payload_string, uint32_t size)
{
	network_messages::CompTransferInfo ob;
	std::string s(payload_string, size);
	if (!ob.ParseFromString(s)){
		OSDLOG(DEBUG, "Deserialization failed")
		return false;;
	};

	this->service_id = ob.service_id();
	for (int i = 0; i < ob.component_service_pair_size(); ++i)
	{
		network_messages::CompTransferInfo_pair pair = ob.component_service_pair(i);
		network_messages::service_obj dest_service_object = pair.dest_service();

		int32_t comp = pair.component();
		recordStructure::ComponentInfoRecord serv_comp_obj;
		serv_comp_obj.set_service_id(dest_service_object.service_id());
		serv_comp_obj.set_service_ip(dest_service_object.ip());
		serv_comp_obj.set_service_port(dest_service_object.port());
		this->comp_service_list.push_back(std::make_pair(comp, serv_comp_obj));
	}

	return true;
}

uint64_t CompTransferInfo::get_fd()
{
	return this->fd;
}

void CompTransferInfo::set_fd(uint64_t fd){
	this->fd = fd;
}

const uint64_t CompTransferInfo::get_type(){
	return this->type;
}

void CompTransferInfo::set_service_id(std::string service_id)
{
	this->service_id = service_id;
}

std::string CompTransferInfo::get_service_id()
{
	return this->service_id;
}

void CompTransferInfo::set_comp_service_list(std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > serv)
{
	this->comp_service_list = serv;
}

std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > CompTransferInfo::get_comp_service_list()
{
	return this->comp_service_list;
}

CompTransferFinalStat::CompTransferFinalStat():
	type(messageTypeEnum::typeEnums::COMP_TRANSFER_FINAL_STAT),
	status(true)
{
}

CompTransferFinalStat::CompTransferFinalStat(
	std::string service_id,
	std::vector<std::pair<int32_t, bool> > comp_pair_list
):
	service_id(service_id),
	type(messageTypeEnum::typeEnums::COMP_TRANSFER_FINAL_STAT),
	comp_pair_list(comp_pair_list),
	status(true)
{
}

bool CompTransferFinalStat::serialize(std::string & payload_string){
	network_messages::CompTranferFinalStat ob;

	ob.set_service_id(this->service_id);
	for (std::vector<std::pair<int32_t, bool> >::iterator it = this->comp_pair_list.begin(); it != this->comp_pair_list.end(); ++it){
		network_messages::CompTranferFinalStat_pair * pair = ob.add_comp_status_list();
		pair->set_component((*it).first);
		pair->set_status((*it).second);
	}

	ob.set_final_status(true);
	return ob.SerializeToString(&payload_string);
}

bool CompTransferFinalStat::deserialize(const char * payload_string, uint32_t size){
	network_messages::CompTranferFinalStat ob;

	std::string s(payload_string, size);
	if (!ob.ParseFromString(s)){
		OSDLOG(DEBUG, "Deserialization failed")
		return false;;
	};

	this->service_id = ob.service_id();
	for (int i = 0; i < ob.comp_status_list_size(); ++i){
		network_messages::CompTranferFinalStat_pair pair = ob.comp_status_list(i);
		this->comp_pair_list.push_back(comp_status_pair(pair.component(), pair.status()));
	}
	this->status = ob.final_status();

	return true;
}

uint64_t CompTransferFinalStat::get_fd(){
	return this->fd;
}

void CompTransferFinalStat::set_fd(uint64_t fd){
	this->fd = fd;
}

const uint64_t CompTransferFinalStat::get_type(){
	return this->type;
}

bool CompTransferFinalStat::get_status(){
	return this->status;
}

void CompTransferFinalStat::set_status(bool status){
	this->status = status;
}

std::vector<std::pair<int32_t, bool> > CompTransferFinalStat::get_component_pair_list()
{
	return this->comp_pair_list;
}

void CompTransferFinalStat::set_component_pair_list(std::vector<std::pair<int32_t, bool> > serv)
{
	this->comp_pair_list = serv;
}

std::string CompTransferFinalStat::get_service_id()
{
	return this->service_id;
}

void CompTransferFinalStat::set_service_id(std::string service_id)
{
	this->service_id = service_id;
}

CompTransferInfoAck::CompTransferInfoAck(
):
	gl_map_info_obj(),
	type(messageTypeEnum::typeEnums::COMP_TRANSFER_INFO_ACK)
{
}

CompTransferInfoAck::CompTransferInfoAck(
	std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> container_pair,
	std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> account_pair,
	std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> updater_pair,
	std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> object_pair,
	float version
):
	gl_map_info_obj(
		container_pair,
		account_pair,
		updater_pair,
		object_pair,
		version
	),
	type(messageTypeEnum::typeEnums::COMP_TRANSFER_INFO_ACK)
{
}

bool CompTransferInfoAck::serialize(std::string & payload_string){
	return this->gl_map_info_obj.serialize(payload_string);
}

bool CompTransferInfoAck::deserialize(const char * payload_string, uint32_t size){
	return this->gl_map_info_obj.deserialize(payload_string, size);
}

const uint64_t CompTransferInfoAck::get_type()
{
	return this->type;
}

uint64_t CompTransferInfoAck::get_fd(){
	return this->fd;
}

void CompTransferInfoAck::set_fd(uint64_t fd){
	this->fd = fd;
}

CompTransferFinalStatAck::CompTransferFinalStatAck(
):
	gl_map_info_obj(),
	type(messageTypeEnum::typeEnums::COMP_TRANSFER_FINAL_STAT_ACK)
{
}

CompTransferFinalStatAck::CompTransferFinalStatAck(
	std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> container_pair,
	std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> account_pair,
	std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> updater_pair,
	std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> object_pair,
	float version
):
	gl_map_info_obj(container_pair, account_pair, updater_pair, object_pair, version),
	type(messageTypeEnum::typeEnums::COMP_TRANSFER_FINAL_STAT_ACK)
{
}

bool CompTransferFinalStatAck::serialize(std::string & payload_string){
	return this->gl_map_info_obj.serialize(payload_string);
}

bool CompTransferFinalStatAck::deserialize(const char * payload_string, uint32_t size){
	return this->gl_map_info_obj.deserialize(payload_string, size);
}

const uint64_t CompTransferFinalStatAck::get_type()
{
	return this->type;
}

uint64_t CompTransferFinalStatAck::get_fd(){
	return this->fd;
}

void CompTransferFinalStatAck::set_fd(uint64_t fd){
	this->fd = fd;
}



// TODO: devesh: need to confirm the correct structure for this request

/*
TransferComp::TransferComp():
	type(messageTypeEnum::typeEnums::OSD_START_MONITORING)
{
}

TransferComp::TransferComp(service_list comp_service_list):
	type(messageTypeEnum::typeEnums::OSD_START_MONITORING),
	comp_service_list(comp_service_list)
{
}

bool TransferComp::serialize(std::string & payload_string){
	network_messages::TransferComp ob;

	for (service_list::iterator it = this->comp_service_list.begin(); it != this->comp_service_list.end(); ++it){
		network_messages::TransferComp_pair * pair = ob.add_comp_service_list();
		network_messages::service_obj * dest_service_obj = pair->mutable_dest_service();

		dest_service_obj->set_service_id((*it).second->get_service_id());
		dest_service_obj->set_ip((*it).second->get_ip());
		dest_service_obj->set_port((*it).second->get_port());

		pair->add_component((*it).first);
	}
	return ob.SerializeToString(&payload_string);
}

bool TransferComp::serialize(char * payload_string, uint32_t size){
	network_messages::TransferComp ob;

	for (int i = 0; i < ob.comp_service_list_size(); ++i){
		network_messages::TransferComp_pair pair = ob.comp_service_list(i);
		network_messages::service_obj serv_obj = pair.dest_service();

		ptr_service_obj dest_service_obj(new ServiceObject());
		dest_service_obj->set_service_id(serv_obj.service_id());
		dest_service_obj->set_ip(serv_obj.ip());
		dest_service_obj->set_port(serv_obj.port());

		this->comp_service_list.push_back(comp_pair(pair.component(), dest_service_obj));
	}
	std::string s(payload_string, size);
	if (!ob.ParseFromString(s)){
		OSDLOG(DEBUG, "Deserialization failed")
		return false;;
	};
}

uint64_t TransferComp::get_fd(){
	return this->fd;
}

void TransferComp::set_fd(uint64_t fd){
	this->fd = fd;
}

const uint64_t TransferComp::get_type(){
	return this->type;
}

service_list TransferComp::get_service_list()
{
	return this->comp_service_list;
}

void TransferComp::set_service_list(service_list serv)
{
	this->comp_service_list = serv;
}


*/
/*
ErrorMessage::ErrorMessage(uint64_t error_num): 
	type(messageTypeEnum::typeEnums::ERROR_MSG),
	error_num(error_num)
{
}

const uint64_t ErrorMessage::get_type(){
	return this->type;
}*/

NodeAdditionGl::NodeAdditionGl():
        type(messageTypeEnum::typeEnums::NODE_ADDITION_GL)
{
}

NodeAdditionGl::NodeAdditionGl(std::string service_id):
        service_id(service_id),
        type(messageTypeEnum::typeEnums::NODE_ADDITION_GL)
{
}


bool NodeAdditionGl::serialize(std::string & payload_string){
        network_messages::NodeAdditionGl ob;
        ob.set_service_id(this->service_id);
        return ob.SerializeToString(&payload_string);
}

bool NodeAdditionGl::deserialize(const char * payload_string, uint32_t size){
        network_messages::NodeAdditionGl ob;
        std::string s(payload_string, size);
	if (!ob.ParseFromString(s)){
		OSDLOG(DEBUG, "Deserialization failed")
		return false;;
	};
        this->service_id = ob.service_id();

	return true;
}

std::string NodeAdditionGl::get_service_id(){
        return this->service_id;
}

uint64_t NodeAdditionGl::get_fd(){
        return this->fd;
}

void NodeAdditionGl::set_fd(uint64_t fd){
        this->fd = fd;
}

void NodeAdditionGl::set_service_id(std::string service_id){
        this->service_id = service_id;
}

const uint64_t NodeAdditionGl::get_type(){
        return this->type;
}

NodeAdditionGlAck::NodeAdditionGlAck():
        type(messageTypeEnum::typeEnums::NODE_ADDITION_GL_ACK)
{
}

NodeAdditionGlAck::NodeAdditionGlAck(
        std::list<boost::shared_ptr<ServiceObject> > service_obj_list,
        boost::shared_ptr<ErrorStatus>  error_status
        ):
        service_obj_list(service_obj_list),
        error_status(error_status),
        type(messageTypeEnum::typeEnums::NODE_ADDITION_GL_ACK)
{
}

bool NodeAdditionGlAck::serialize(std::string & payload_string) {
        network_messages::NodeAdditionGlAck ob;
        std::list<boost::shared_ptr<ServiceObject> > service_object_list(this->service_obj_list);
        for (std::list<boost::shared_ptr<ServiceObject> >::iterator it=service_object_list.begin(); it != service_object_list.end(); ++it)
        {
                network_messages::service_obj * service_list = ob.add_service_list();
                service_list->set_service_id((*it)->get_service_id());
                service_list->set_ip((*it)->get_ip());
                service_list->set_port((*it)->get_port());
        }
        network_messages::errorStatus* error = ob.mutable_status();
        error->set_code(this->error_status->get_code());
        error->set_msg(this->error_status->get_msg());
        return ob.SerializeToString(&payload_string);
}

bool NodeAdditionGlAck::deserialize(const char * payload_string, uint32_t size){
        network_messages::NodeAdditionGlAck ob;
        std::string s(payload_string, size);
	if (!ob.ParseFromString(s)){
		OSDLOG(DEBUG, "Deserialization failed")
		return false;;
	};

        for (int i = 0; i < ob.service_list_size(); ++i){
                network_messages::service_obj entry_service_object = ob.service_list(i);
                boost::shared_ptr<ServiceObject> service_obj(new ServiceObject());

                service_obj->set_service_id(entry_service_object.service_id());
                service_obj->set_ip(entry_service_object.ip());
                service_obj->set_port(entry_service_object.port());

                this->service_obj_list.push_back(service_obj);
        }
        network_messages::errorStatus err_stat = ob.status();
        this->error_status.reset(new ErrorStatus());
        this->error_status->set_code(err_stat.code());
        this->error_status->set_msg(err_stat.msg());

	return true;
}

void NodeAdditionGlAck::add_service_object(boost::shared_ptr<ServiceObject> service_obj){
        this->service_obj_list.push_back(service_obj);
}

std::list<boost::shared_ptr<ServiceObject> >  NodeAdditionGlAck::get_service_objects(){
        return this->service_obj_list;
}

uint64_t NodeAdditionGlAck::get_fd(){
        return this->fd;
}

void NodeAdditionGlAck::set_fd(uint64_t fd){
        this->fd = fd;
}

boost::shared_ptr<ErrorStatus> NodeAdditionGlAck::get_error_status(){
        return this->error_status;
}

void NodeAdditionGlAck::set_error_status(boost::shared_ptr<ErrorStatus> err_stat){
        this->error_status = err_stat;
}

const uint64_t NodeAdditionGlAck::get_type(){
        return this->type;
}

NodeAdditionFinalAck::NodeAdditionFinalAck():type(messageTypeEnum::typeEnums::NODE_ADDITION_FINAL_ACK)
{
}

NodeAdditionFinalAck::NodeAdditionFinalAck(bool success)
	:type(messageTypeEnum::typeEnums::NODE_ADDITION_FINAL_ACK),
	success(success)
{}

bool NodeAdditionFinalAck::serialize(std::string & payload_string)
{
	network_messages::NodeAdditionFinalAck ob;
	ob.set_status(this->success);
	return ob.SerializeToString(&payload_string);
}

const uint64_t NodeAdditionFinalAck::get_type()
{
	return this->type;
}

bool NodeAdditionFinalAck::deserialize(const char * payload_string, uint32_t size)
{
	network_messages::NodeAdditionFinalAck ob;
	std::string s(payload_string, size);
	if (!ob.ParseFromString(s))
	{
		OSDLOG(DEBUG, "Deserialization failed");
		return false;
	}
	this->success = ob.status();
	return true;
}

bool NodeAdditionFinalAck::get_status()
{
	return this->success;
}

NodeAdditionCli::NodeAdditionCli()
        :type(messageTypeEnum::typeEnums::NODE_ADDITION_CLI)
{}

NodeAdditionCli::NodeAdditionCli(std::list<boost::shared_ptr<ServiceObject> > node_list)
        :node_list(node_list),
         type(messageTypeEnum::typeEnums::NODE_ADDITION_CLI)
{
}

bool NodeAdditionCli::serialize(std::string & payload_string){
        network_messages::NodeAdditionCli ob;
        std::list<boost::shared_ptr<ServiceObject> > node_object_list(this->node_list);
        for (std::list<boost::shared_ptr<ServiceObject> >::iterator it=node_object_list.begin(); it != node_object_list.end(); ++it)
        {
                network_messages::service_obj * service_list = ob.add_node_list();
                service_list->set_service_id((*it)->get_service_id());
                service_list->set_ip((*it)->get_ip());
                service_list->set_port((*it)->get_port());
        }
        return ob.SerializeToString(&payload_string);
}

bool NodeAdditionCli::deserialize(const char * payload_string, uint32_t size){
        network_messages::NodeAdditionCli ob;
        std::string s(payload_string, size);
	if (!ob.ParseFromString(s)){
		OSDLOG(DEBUG, "Deserialization failed")
		return false;;
	};

//	this->node_list.reset(new std::list<boost::shared_ptr<ServiceObject> >);
        for (int i = 0; i < ob.node_list_size(); ++i){
                network_messages::service_obj entry_service_object = ob.node_list(i);
                boost::shared_ptr<ServiceObject> service_obj(new ServiceObject());

                service_obj->set_service_id(entry_service_object.service_id());
                service_obj->set_ip(entry_service_object.ip());
                service_obj->set_port(entry_service_object.port());

                this->node_list.push_back(service_obj);
        }
	return true;
}

void NodeAdditionCli::add_node_object(boost::shared_ptr<ServiceObject> service_obj){
        this->node_list.push_back(service_obj);
}

std::list<boost::shared_ptr<ServiceObject> > NodeAdditionCli::get_node_objects(){
        return this->node_list;
}

uint64_t NodeAdditionCli::get_fd(){
        return this->fd;
}

void NodeAdditionCli::set_fd(uint64_t fd){
        this->fd = fd;
}

const uint64_t NodeAdditionCli::get_type(){
        return this->type;
}

NodeAdditionCliAck::NodeAdditionCliAck()
        :type(messageTypeEnum::typeEnums::NODE_ADDITION_CLI_ACK)
{}

NodeAdditionCliAck::NodeAdditionCliAck(
	std::list<std::pair<boost::shared_ptr<ServiceObject>, boost::shared_ptr<ErrorStatus> > > node_status_list
):
	node_status_list(node_status_list),
	type(messageTypeEnum::typeEnums::NODE_ADDITION_CLI_ACK)
{
}

bool NodeAdditionCliAck::serialize(std::string & payload_string){
        network_messages::NodeAdditionCliAck ob;
	
	for ( std::list<std::pair<boost::shared_ptr<ServiceObject>, boost::shared_ptr<ErrorStatus> > >::iterator it = this->node_status_list.begin(); it != this->node_status_list.end(); it++ )
        {
                network_messages::NodeAdditionCliAck_pair* node_pair = ob.add_node_ack_list();
                network_messages::service_obj * serv_obj = node_pair->mutable_node();
                network_messages::errorStatus * err_obj = node_pair->mutable_status();

                serv_obj->set_service_id((*it).first->get_service_id());
                serv_obj->set_ip((*it).first->get_ip());
                serv_obj->set_port((*it).first->get_port());

                err_obj->set_code((*it).second->get_code());
                err_obj->set_msg((*it).second->get_msg());
        }
        return ob.SerializeToString(&payload_string);
}

bool NodeAdditionCliAck::deserialize(const char * payload_string, uint32_t size){
        network_messages::NodeAdditionCliAck ob;
        std::string s(payload_string, size);
	if (!ob.ParseFromString(s)){
		OSDLOG(DEBUG, "Deserialization failed")
		return false;;
	};

        for (int i = 0; i < ob.node_ack_list_size(); ++i){
                network_messages::NodeAdditionCliAck_pair  pair = ob.node_ack_list(i);
                network_messages::service_obj  pair_service_obj = pair.node();
                network_messages::errorStatus  err_obj = pair.status();

                boost::shared_ptr<ServiceObject> service_obj(new ServiceObject());
                service_obj->set_service_id(pair_service_obj.service_id());
                service_obj->set_ip(pair_service_obj.ip());
                service_obj->set_port(pair_service_obj.port());

                boost::shared_ptr<ErrorStatus> err_stat(new ErrorStatus());
                err_stat->set_code(err_obj.code());
                err_stat->set_msg(err_obj.msg());

                this->node_status_list.push_back(std::make_pair(service_obj, err_stat));

        }

	return true;
}

void NodeAdditionCliAck::add_node_in_node_status_list(boost::shared_ptr<ServiceObject> service_obj, boost::shared_ptr<ErrorStatus> err_stat){        
        this->node_status_list.push_back(std::make_pair(service_obj,err_stat));
}       
        
std::list<std::pair<boost::shared_ptr<ServiceObject>, boost::shared_ptr<ErrorStatus> > > NodeAdditionCliAck::get_node_status_list(){
        return this->node_status_list;
}               
                
uint64_t NodeAdditionCliAck::get_fd(){
        return this->fd;
}               
                
void NodeAdditionCliAck::set_fd(uint64_t fd){
        this->fd = fd;
}               
                
const uint64_t NodeAdditionCliAck::get_type(){
        return this->type;
}

NodeRetire::NodeRetire() 
	:type(messageTypeEnum::typeEnums::NODE_RETIRE)
{
}

NodeRetire::NodeRetire(boost::shared_ptr<ServiceObject> node)
	:node(node), type(messageTypeEnum::typeEnums::NODE_RETIRE)
{
}

bool NodeRetire::serialize(std::string & payload_string){
	network_messages::NodeRetire ob;	
	
	network_messages::service_obj* node_obj = ob.mutable_node();
	node_obj->set_service_id(this->node->get_service_id());
	node_obj->set_ip(this->node->get_ip());
	node_obj->set_port(this->node->get_port());

	return ob.SerializeToString(&payload_string);
}

bool NodeRetire::deserialize(const char * payload_string, uint32_t size){
	network_messages::NodeRetire ob;
        std::string s(payload_string, size);
	if (!ob.ParseFromString(s)){
		OSDLOG(DEBUG, "Deserialization failed")
		return false;;
	};

	network_messages::service_obj node_obj = ob.node();
	this->node.reset(new ServiceObject());
	this->node->set_service_id(node_obj.service_id());
	this->node->set_ip(node_obj.ip());
	this->node->set_port(node_obj.port());

	return true;
}

void NodeRetire::set_node(boost::shared_ptr<ServiceObject> node){
	this->node = node;
}

boost::shared_ptr<ServiceObject> NodeRetire::get_node(){
	return this->node;
}

uint64_t NodeRetire::get_fd(){
        return this->fd;
}

void NodeRetire::set_fd(uint64_t fd){
        this->fd = fd;
}

const uint64_t NodeRetire::get_type(){
        return this->type;
}

NodeRetireAck::NodeRetireAck()
        :type(messageTypeEnum::typeEnums::NODE_RETIRE_ACK)
{
	this->error_status.reset(new ErrorStatus);
}

NodeRetireAck::NodeRetireAck(std::string node_id, boost::shared_ptr<ErrorStatus> error_status)
        :node_id(node_id), error_status(error_status), type(messageTypeEnum::typeEnums::NODE_RETIRE_ACK)
{       
}

bool NodeRetireAck::serialize(std::string & payload_string){
	network_messages::NodeRetireAck ob;
	
	network_messages::errorStatus* err_stat = ob.mutable_status();

	err_stat->set_code(this->error_status->get_code());
	err_stat->set_msg(this->error_status->get_msg());
	ob.set_node_id(this->node_id);
	
	return ob.SerializeToString(&payload_string);
}

bool NodeRetireAck::deserialize(const char * payload_string, uint32_t size){
	network_messages::NodeRetireAck ob;
	std::string s(payload_string, size);
	if (!ob.ParseFromString(s)){
		OSDLOG(DEBUG, "Deserialization failed")
		return false;;
	};

	network_messages::errorStatus err_stat = ob.status();
	this->error_status.reset(new ErrorStatus());
	this->error_status->set_code(err_stat.code());
	this->error_status->set_msg(err_stat.msg());

	this->node_id = ob.node_id();

	return true;
}


void NodeRetireAck::set_status(boost::shared_ptr<ErrorStatus> error_status){
	this->error_status = error_status;
}

boost::shared_ptr<ErrorStatus> NodeRetireAck::get_status(){
	return this->error_status;
}

void NodeRetireAck::set_node_id(std::string node_id){
	this->node_id = node_id;
}

std::string NodeRetireAck::get_node_id(){
	return this->node_id;
}

uint64_t NodeRetireAck::get_fd(){
        return this->fd;
}

void NodeRetireAck::set_fd(uint64_t fd){
        this->fd = fd;
}

const uint64_t NodeRetireAck::get_type(){
        return this->type;
}

StopServices::StopServices()
	:type(messageTypeEnum::typeEnums::STOP_SERVICES)
{
}

StopServices::StopServices(std::string service_id)
	: service_id(service_id), type(messageTypeEnum::typeEnums::STOP_SERVICES)
{
}

bool StopServices::serialize(std::string & payload_string){
	network_messages::StopServices ob;
	ob.set_service_id(this->service_id);
	return ob.SerializeToString(&payload_string);
}

bool StopServices::deserialize(const char * payload_string, uint32_t size){
	network_messages::StopServices ob;
	std::string s(payload_string, size);
	if (!ob.ParseFromString(s)){
		OSDLOG(DEBUG, "Deserialization failed")
		return false;;
	};
	this->service_id = ob.service_id();

	return true;
}

void StopServices::set_service_id(std::string service_id){
	this->service_id = service_id;
}

std::string StopServices::get_service_id(){
	return this->service_id;
}

uint64_t StopServices::get_fd(){
        return this->fd;
}

void StopServices::set_fd(uint64_t fd){
        this->fd = fd;
}

const uint64_t StopServices::get_type(){
        return this->type;
}

StopServicesAck::StopServicesAck()
        :type(messageTypeEnum::typeEnums::STOP_SERVICES_ACK)
{
}

StopServicesAck::StopServicesAck(boost::shared_ptr<ErrorStatus> error_status)
	 : error_status(error_status),
	   type(messageTypeEnum::typeEnums::STOP_SERVICES_ACK)
{
}

bool StopServicesAck::serialize(std::string & payload_string)
{
	network_messages::StopServicesAck ob;

	network_messages::errorStatus* err_stat = ob.mutable_status();
        err_stat->set_code(this->error_status->get_code());
        err_stat->set_msg(this->error_status->get_msg());

        return ob.SerializeToString(&payload_string);
}

bool StopServicesAck::deserialize(const char * payload_string, uint32_t size)
{
	network_messages::StopServicesAck ob;
	std::string s(payload_string, size);
	if (!ob.ParseFromString(s)){
		OSDLOG(DEBUG, "Deserialization failed")
		return false;;
	};

        network_messages::errorStatus err_stat = ob.status();
	this->error_status.reset(new ErrorStatus());
        this->error_status->set_code(err_stat.code());
        this->error_status->set_msg(err_stat.msg());

	return true;
}

void StopServicesAck::set_status(int32_t code, std::string msg){
	this->error_status->set_code(code);
	this->error_status->set_msg(msg);
}

boost::shared_ptr<ErrorStatus> StopServicesAck::get_status(){
	return this->error_status;
}

uint64_t StopServicesAck::get_fd(){
        return this->fd;
}            
             
void StopServicesAck::set_fd(uint64_t fd){
        this->fd = fd;
}            
             
const uint64_t StopServicesAck::get_type(){
        return this->type;
}

NodeSystemStopCli::NodeSystemStopCli()
	:type(messageTypeEnum::typeEnums::NODE_SYSTEM_STOP_CLI)
{
}

NodeSystemStopCli::NodeSystemStopCli(std::string node_id)
	:node_id(node_id),
	 type(messageTypeEnum::typeEnums::NODE_SYSTEM_STOP_CLI)
{
}

bool NodeSystemStopCli::serialize(std::string & payload_string){
	network_messages::NodeSystemStopCli ob;

        ob.set_node_id(this->node_id);
        return ob.SerializeToString(&payload_string);		
}

bool NodeSystemStopCli::deserialize(const char * payload_string, uint32_t size){
	network_messages::NodeSystemStopCli ob;
	std::string s(payload_string, size);
	if (!ob.ParseFromString(s)){
		OSDLOG(DEBUG, "Deserialization failed")
		return false;;
	};
        this->node_id = ob.node_id();

	return true;
}

void NodeSystemStopCli::set_node_id(std::string node_id){
	this->node_id = node_id;
}

std::string NodeSystemStopCli::get_node_id(){
	return this->node_id;
}

uint64_t NodeSystemStopCli::get_fd(){
        return this->fd;
}

void NodeSystemStopCli::set_fd(uint64_t fd){
        this->fd = fd;
}

const uint64_t NodeSystemStopCli::get_type(){
        return this->type;
}

LocalNodeStatus::LocalNodeStatus()
        :type(messageTypeEnum::typeEnums::LOCAL_NODE_STATUS)
{
}

LocalNodeStatus::LocalNodeStatus(std::string node_id)
        :node_id(node_id),
         type(messageTypeEnum::typeEnums::LOCAL_NODE_STATUS)
{       
}

bool LocalNodeStatus::serialize(std::string & payload_string){
        network_messages::LocalNodeStatus ob; 
        
        ob.set_node_id(this->node_id);
        return ob.SerializeToString(&payload_string);
}       

bool LocalNodeStatus::deserialize(const char * payload_string, uint32_t size){
        network_messages::LocalNodeStatus ob;
        std::string s(payload_string, size);
	if (!ob.ParseFromString(s)){
		OSDLOG(DEBUG, "Deserialization failed")
		return false;;
	}; 
        this->node_id = ob.node_id();

	return true;
}       

void LocalNodeStatus::set_node_id(std::string node_id){
        this->node_id = node_id;
}       

std::string LocalNodeStatus::get_node_id(){
        return this->node_id;
}       

uint64_t LocalNodeStatus::get_fd(){
        return this->fd;
}       

void LocalNodeStatus::set_fd(uint64_t fd){
        this->fd = fd;
}

const uint64_t LocalNodeStatus::get_type(){
        return this->type;
}

NodeStatus::NodeStatus()
        :type(messageTypeEnum::typeEnums::NODE_STATUS)
{
}

NodeStatus::NodeStatus(boost::shared_ptr<ServiceObject> node)
        :node(node), type(messageTypeEnum::typeEnums::NODE_STATUS)
{
}

bool NodeStatus::serialize(std::string & payload_string){
        network_messages::NodeStatus ob;

        network_messages::service_obj* node_obj = ob.mutable_node();
        node_obj->set_service_id(this->node->get_service_id());
        node_obj->set_ip(this->node->get_ip());
        node_obj->set_port(this->node->get_port());

        return ob.SerializeToString(&payload_string);
}

bool NodeStatus::deserialize(const char * payload_string, uint32_t size){
        network_messages::NodeStatus ob;
        std::string s(payload_string, size);
	if (!ob.ParseFromString(s)){
		OSDLOG(DEBUG, "Deserialization failed")
		return false;;
	};

        network_messages::service_obj node_obj = ob.node();
        this->node.reset(new ServiceObject());
        this->node->set_service_id(node_obj.service_id());
        this->node->set_ip(node_obj.ip());
        this->node->set_port(node_obj.port());

	return true;
}

void NodeStatus::set_node(boost::shared_ptr<ServiceObject> node){
        this->node = node;
}

boost::shared_ptr<ServiceObject> NodeStatus::get_node(){
        return this->node;
}

uint64_t NodeStatus::get_fd(){
        return this->fd;
}

void NodeStatus::set_fd(uint64_t fd){
        this->fd = fd;
}

const uint64_t NodeStatus::get_type(){
        return this->type;
}

NodeStatusAck::NodeStatusAck()
        :type(messageTypeEnum::typeEnums::NODE_STATUS_ACK)
{
}

NodeStatusAck::NodeStatusAck(network_messages::NodeStatusEnum node_status):
	node_status(node_status),
	type(messageTypeEnum::typeEnums::NODE_STATUS_ACK)
{
}

bool NodeStatusAck::serialize(std::string & payload_string)
{
        network_messages::NodeStatusAck ob;
	ob.set_status(this->node_status);
        return ob.SerializeToString(&payload_string);
}

bool NodeStatusAck::deserialize(const char * payload_string, uint32_t size)
{
        network_messages::NodeStatusAck ob;
        std::string s(payload_string, size);
	if (!ob.ParseFromString(s)){
		OSDLOG(DEBUG, "Deserialization failed")
		return false;;
	};

        this->node_status = ob.status();

	return true;
}

void NodeStatusAck::set_status(network_messages::NodeStatusEnum node_status)
{
        this->node_status = node_status;
}

network_messages::NodeStatusEnum NodeStatusAck::get_status()
{
        return this->node_status;
}

uint64_t NodeStatusAck::get_fd()
{
        return this->fd;
}

void NodeStatusAck::set_fd(uint64_t fd)
{
        this->fd = fd;
}

const uint64_t NodeStatusAck::get_type()
{
        return this->type;
}

NodeStopCli::NodeStopCli()
        :type(messageTypeEnum::typeEnums::NODE_STOP_CLI)
{
}       
        
NodeStopCli::NodeStopCli(std::string node_id)
        :node_id(node_id),
         type(messageTypeEnum::typeEnums::NODE_STOP_CLI)
{
}       
        
bool NodeStopCli::serialize(std::string & payload_string){
        network_messages::NodeStopCli ob; 

        ob.set_node_id(this->node_id);
        return ob.SerializeToString(&payload_string);
}       
        
bool NodeStopCli::deserialize(const char * payload_string, uint32_t size){
        network_messages::NodeStopCli ob; 
        std::string s(payload_string, size);
	if (!ob.ParseFromString(s)){
		OSDLOG(DEBUG, "Deserialization failed")
		return false;;
	};
        this->node_id = ob.node_id();

	return true;
}       

void NodeStopCli::set_node_id(std::string node_id){
        this->node_id = node_id;
}       

std::string NodeStopCli::get_node_id(){
        return this->node_id;
}       

uint64_t NodeStopCli::get_fd(){
        return this->fd;
}       

void NodeStopCli::set_fd(uint64_t fd){
        this->fd = fd;
}

const uint64_t NodeStopCli::get_type(){
        return this->type;
}

NodeStopCliAck::NodeStopCliAck()
        :type(messageTypeEnum::typeEnums::NODE_STOP_CLI_ACK)
{
}

NodeStopCliAck::NodeStopCliAck(boost::shared_ptr<ErrorStatus> error_status)
        :error_status(error_status),
         type(messageTypeEnum::typeEnums::NODE_STOP_CLI_ACK)
{
}

bool NodeStopCliAck::serialize(std::string & payload_string)
{
        network_messages::NodeStopCliAck ob;

        network_messages::errorStatus* err_stat = ob.mutable_status();
        err_stat->set_code(this->error_status->get_code());
        err_stat->set_msg(this->error_status->get_msg());

        return ob.SerializeToString(&payload_string);
}

bool NodeStopCliAck::deserialize(const char * payload_string, uint32_t size)
{
        network_messages::NodeStopCliAck ob;
        std::string s(payload_string, size);
	if (!ob.ParseFromString(s)){
		OSDLOG(DEBUG, "Deserialization failed")
		return false;;
	};

        network_messages::errorStatus err_stat = ob.status();
	this->error_status.reset(new ErrorStatus());
        this->error_status->set_code(err_stat.code());
        this->error_status->set_msg(err_stat.msg());

	return true;
}

void NodeStopCliAck::set_status(int32_t code, std::string msg){
        this->error_status->set_code(code);
        this->error_status->set_msg(msg);
}

boost::shared_ptr<ErrorStatus> NodeStopCliAck::get_status(){
        return this->error_status;
}

uint64_t NodeStopCliAck::get_fd(){
        return this->fd;
}

void NodeStopCliAck::set_fd(uint64_t fd){
        this->fd = fd;
}

const uint64_t NodeStopCliAck::get_type(){
        return this->type;
}

NodeFailover::NodeFailover()
	: type(messageTypeEnum::typeEnums::NODE_FAILOVER)
{
}

NodeFailover::NodeFailover(std::string node_id, boost::shared_ptr<ErrorStatus> error_status)
	: node_id(node_id), error_status(error_status),
	  type(messageTypeEnum::typeEnums::NODE_FAILOVER)
{
}

bool NodeFailover::serialize(std::string & payload_string)
{
        network_messages::NodeFailover ob;

        network_messages::errorStatus* err_stat = ob.mutable_status();
        err_stat->set_code(this->error_status->get_code());
        err_stat->set_msg(this->error_status->get_msg());
	ob.set_node_id(this->node_id);

        return ob.SerializeToString(&payload_string);
}

bool NodeFailover::deserialize(const char * payload_string, uint32_t size)
{
        network_messages::NodeFailover ob;
        std::string s(payload_string, size);
	if (!ob.ParseFromString(s)){
		OSDLOG(DEBUG, "Deserialization failed")
		return false;;
	};

        network_messages::errorStatus err_stat = ob.status();
	this->error_status.reset(new ErrorStatus());
        this->error_status->set_code(err_stat.code());
        this->error_status->set_msg(err_stat.msg());
	this->node_id = ob.node_id();

	return true;
}

void NodeFailover::set_status(int32_t code, std::string msg){
        this->error_status->set_code(code);
        this->error_status->set_msg(msg);
}

boost::shared_ptr<ErrorStatus> NodeFailover::get_status(){
        return this->error_status;
}

void NodeFailover::set_node_id(std::string node_id){
	this->node_id = node_id;
}

std::string NodeFailover::get_node_id(){
	return this->node_id;
}

uint64_t NodeFailover::get_fd(){
        return this->fd;
}

void NodeFailover::set_fd(uint64_t fd){
        this->fd = fd;
}

const uint64_t NodeFailover::get_type(){
        return this->type;
}

NodeFailoverAck::NodeFailoverAck()
        : type(messageTypeEnum::typeEnums::NODE_FAILOVER_ACK)
{
}

NodeFailoverAck::NodeFailoverAck(boost::shared_ptr<ErrorStatus> error_status)
        : error_status(error_status),
          type(messageTypeEnum::typeEnums::NODE_FAILOVER_ACK)
{
}

bool NodeFailoverAck::serialize(std::string & payload_string)
{
        network_messages::NodeFailoverAck ob;

        network_messages::errorStatus* err_stat = ob.mutable_status();
        err_stat->set_code(this->error_status->get_code());
        err_stat->set_msg(this->error_status->get_msg());

        return ob.SerializeToString(&payload_string);
}

bool NodeFailoverAck::deserialize(const char * payload_string, uint32_t size)
{
        network_messages::NodeFailoverAck ob;
        std::string s(payload_string, size);
	if (!ob.ParseFromString(s)){
		OSDLOG(DEBUG, "Deserialization failed")
		return false;;
	};

        network_messages::errorStatus err_stat = ob.status();
	this->error_status.reset(new ErrorStatus());
        this->error_status->set_code(err_stat.code());
        this->error_status->set_msg(err_stat.msg());

	return true;
}

void NodeFailoverAck::set_status(int32_t code, std::string msg){
        this->error_status->set_code(code);
        this->error_status->set_msg(msg);
}

boost::shared_ptr<ErrorStatus> NodeFailoverAck::get_status(){
        return this->error_status;
}

uint64_t NodeFailoverAck::get_fd(){
        return this->fd;
}

void NodeFailoverAck::set_fd(uint64_t fd){
        this->fd = fd;
}

const uint64_t NodeFailoverAck::get_type(){
        return this->type;
}

TakeGlOwnership::TakeGlOwnership()
	:type(messageTypeEnum::typeEnums::TAKE_GL_OWNERSHIP)
{
}

TakeGlOwnership::TakeGlOwnership(std::string old_gl_id, std::string new_gl_id)
	:old_gl_id(old_gl_id), new_gl_id(new_gl_id),
	 type(messageTypeEnum::typeEnums::TAKE_GL_OWNERSHIP)
{
}

bool TakeGlOwnership::serialize(std::string & payload_string){
        network_messages::TakeGlOwnership ob;

	ob.set_old_gl_id(this->old_gl_id);
	ob.set_new_gl_id(this->new_gl_id);

        return ob.SerializeToString(&payload_string);
}

bool TakeGlOwnership::deserialize(const char * payload_string, uint32_t size){
        network_messages::TakeGlOwnership ob;
        std::string s(payload_string, size);
	if (!ob.ParseFromString(s)){
		OSDLOG(DEBUG, "Deserialization failed")
		return false;;
	};
	
	this->old_gl_id = ob.old_gl_id();
	this->new_gl_id = ob.new_gl_id();

	return true;
}

void TakeGlOwnership::set_old_gl_id(std::string old_gl_id){
	this->old_gl_id = old_gl_id;
}

std::string TakeGlOwnership::get_old_gl_id(){
	return this->old_gl_id;
}

void TakeGlOwnership::set_new_gl_id(std::string new_gl_id){
	this->new_gl_id = new_gl_id;
}

std::string TakeGlOwnership::get_new_gl_id(){
	return this->new_gl_id;
}

uint64_t TakeGlOwnership::get_fd(){
        return this->fd;
}

void TakeGlOwnership::set_fd(uint64_t fd){
        this->fd = fd;
}

const uint64_t TakeGlOwnership::get_type(){
        return this->type;
}

TakeGlOwnershipAck::TakeGlOwnershipAck()
        :type(messageTypeEnum::typeEnums::TAKE_GL_OWNERSHIP_ACK)
{
	this->status.reset(new ErrorStatus());
}

TakeGlOwnershipAck::TakeGlOwnershipAck(std::string new_gl_id, boost::shared_ptr<ErrorStatus> status)
        :new_gl_id(new_gl_id), status(status),
         type(messageTypeEnum::typeEnums::TAKE_GL_OWNERSHIP_ACK)
{
}

bool TakeGlOwnershipAck::serialize(std::string & payload_string)
{
        network_messages::TakeGlOwnershipAck ob;
	
	network_messages::errorStatus* status = ob.mutable_status();
        status->set_code(this->status->get_code());
        status->set_msg(this->status->get_msg());
	
	ob.set_new_gl_id(this->new_gl_id);

        return ob.SerializeToString(&payload_string);
}

bool TakeGlOwnershipAck::deserialize(const char * payload_string, uint32_t size)
{
        network_messages::TakeGlOwnershipAck ob;
        std::string s(payload_string, size);
	if (!ob.ParseFromString(s))
	{
		OSDLOG(DEBUG, "Deserialization failed")
		return false;;
	};
	
	network_messages::errorStatus status = ob.status();
        this->status->set_code(status.code());
        this->status->set_msg(status.msg());

	this->new_gl_id = ob.new_gl_id();

	return true;
}

void TakeGlOwnershipAck::set_new_gl_id(std::string new_gl_id){
	this->new_gl_id = new_gl_id;
}

std::string TakeGlOwnershipAck::get_new_gl_id(){
        return this->new_gl_id;
}

void TakeGlOwnershipAck::set_status(boost::shared_ptr<ErrorStatus> status){
	this->status = status;
}

boost::shared_ptr<ErrorStatus> TakeGlOwnershipAck::get_status(){
	return this->status;
}

uint64_t TakeGlOwnershipAck::get_fd(){
        return this->fd;
}


void TakeGlOwnershipAck::set_fd(uint64_t fd){
        this->fd = fd;
}

const uint64_t TakeGlOwnershipAck::get_type(){
        return this->type;
}

TransferComponents::TransferComponents():
	type(messageTypeEnum::typeEnums::TRANSFER_COMPONENT)
{
}

TransferComponents::TransferComponents(
	std::map<recordStructure::ComponentInfoRecord, std::list<uint32_t> > service_map,
	std::map<std::string, std::string> header_dict
):
	type(messageTypeEnum::typeEnums::TRANSFER_COMPONENT),
	component_map(service_map),
	header_dict(header_dict)
{
}

std::map<std::string, std::string> TransferComponents::get_header_map()
{
	return this->header_dict;
}

std::map<recordStructure::ComponentInfoRecord, std::list<uint32_t> > TransferComponents::get_comp_map() const
{
	return this->component_map;
}

void TransferComponents::set_comp_map(std::map<recordStructure::ComponentInfoRecord, std::list<uint32_t> > service_map)
{
	this->component_map = service_map;
}

bool TransferComponents::serialize(std::string &payload_string)
{
	network_messages::TransferComp ob;

	std::map<recordStructure::ComponentInfoRecord, std::list<uint32_t> >::iterator it = this->component_map.begin();
	std::list<uint32_t>::iterator it1;

	for (; it != this->component_map.end(); ++it)
	{
		network_messages::TransferComp_pair * pair = ob.add_service_comp_list();
		network_messages::service_obj * dest_service_obj = pair->mutable_dest_service();
		dest_service_obj->set_service_id(it->first.get_service_id());
		dest_service_obj->set_ip(it->first.get_service_ip());
		dest_service_obj->set_port(it->first.get_service_port());
		for(it1 = it->second.begin(); it1 != it->second.end(); ++it1)
		{
			pair->add_component(*it1);
		}
	}
	return ob.SerializeToString(&payload_string);
}

bool TransferComponents::deserialize(const char * payload_string, uint32_t size)
{
	network_messages::TransferComp ob;

	std::string s(payload_string, size);
	if (!ob.ParseFromString(s)){
		OSDLOG(DEBUG, "Deserialization failed")
		return false;;
	}

	for (int i = 0; i < ob.service_comp_list_size(); ++i)
	{
		network_messages::TransferComp_pair pair = ob.service_comp_list(i);
		network_messages::service_obj serv_obj = pair.dest_service();

		recordStructure::ComponentInfoRecord dest_service_obj;
		dest_service_obj.set_service_id(serv_obj.service_id());
		dest_service_obj.set_service_ip(serv_obj.ip());
		dest_service_obj.set_service_port(serv_obj.port());
		std::list<uint32_t> li;
		for (int j = 0; j < pair.component_size(); ++j)
			li.push_back(pair.component(j));
		this->component_map.insert(std::make_pair(dest_service_obj, li));
	}

	return true;
}

const uint64_t TransferComponents::get_type()
{
        return this->type;
}

TransferComponentsAck::TransferComponentsAck(std::vector<std::pair<int32_t, bool> > comp_pair_list):
	type(messageTypeEnum::typeEnums::TRANSFER_COMPONENT_RESPONSE),
	comp_pair_list(comp_pair_list)
{
}

TransferComponentsAck::TransferComponentsAck():type(messageTypeEnum::typeEnums::TRANSFER_COMPONENT_RESPONSE)
{
}

bool TransferComponentsAck::serialize(std::string &payload_string)
{
	network_messages::TransferComponentsAck ob;
	std::vector<std::pair<int32_t, bool> >::iterator it = this->comp_pair_list.begin();

	for (; it != this->comp_pair_list.end(); ++it)
	{
		network_messages::TransferComponentsAck_pair * pair = ob.add_comp_status_list();

		pair->set_component((*it).first);
		pair->set_status((*it).second);
	}
	return ob.SerializeToString(&payload_string);

}

bool TransferComponentsAck::deserialize(const char * payload_string, uint32_t size)
{
	network_messages::TransferComponentsAck ob;

	std::string s(payload_string, size);
	if (!ob.ParseFromString(s)){
		OSDLOG(DEBUG, "Deserialization failed")
		return false;;
	};

	for (int i = 0; i < ob.comp_status_list_size(); ++i)
	{
		network_messages::TransferComponentsAck_pair pair = ob.comp_status_list(i);

		this->comp_pair_list.push_back(std::make_pair(pair.component(), pair.status()));
	}

	return true;
}

void TransferComponentsAck::set_comp_list(std::vector<std::pair<int32_t, bool> > comp_list)
{
	this->comp_pair_list = comp_list;
}

std::vector<std::pair<int32_t, bool> > TransferComponentsAck::get_comp_list() const
{
	return this->comp_pair_list;
}

const uint64_t TransferComponentsAck::get_type()
{
	return this->type;
}

BlockRequests::BlockRequests(
):
	type(messageTypeEnum::typeEnums::BLOCK_NEW_REQUESTS)
{
}

BlockRequests::BlockRequests(
	std::map<std::string, std::string> header_dict
):
	type(messageTypeEnum::typeEnums::BLOCK_NEW_REQUESTS)
{
	this->header_dict = header_dict;
}

bool BlockRequests::serialize(std::string & payload_string)
{
	return true;
}

bool BlockRequests::deserialize(const char * payload_string, uint32_t size)
{
	return true;
}

std::map<std::string, std::string> BlockRequests::get_header_map()
{
	return this->header_dict;
}

const uint64_t BlockRequests::get_type()
{
	return this->type;
}

BlockRequestsAck::BlockRequestsAck(
):
	type(messageTypeEnum::typeEnums::BLOCK_NEW_REQUESTS_ACK)
{
}

BlockRequestsAck::BlockRequestsAck(
	bool status
):
	type(messageTypeEnum::typeEnums::BLOCK_NEW_REQUESTS_ACK),
	status(status)
{
}

bool BlockRequestsAck::get_status() const
{
	return this->status;
}

void BlockRequestsAck::set_status(bool status)
{
	this->status = status;
}

bool BlockRequestsAck::serialize(std::string & payload_string)
{
	network_messages::BlockRequestAck ob;
	ob.set_status(this->status);
	return ob.SerializeToString(&payload_string);
}

bool BlockRequestsAck::deserialize(const char * payload_string, uint32_t size)
{
	this->status = true;
	return true;
	/*network_messages::BlockRequestAck ob;
	std::string s(payload_string, size);
	if (!ob.ParseFromString(s)){
		OSDLOG(DEBUG, "Deserialization failed")
		return false;;
	};

	return true;*/
}

const uint64_t BlockRequestsAck::get_type()
{
	return this->type;
}

GetVersionID::GetVersionID(std::string service_id):
	type(messageTypeEnum::typeEnums::GET_OBJECT_VERSION),
	service_id(service_id)
{
}

std::string GetVersionID::get_service_id()
{
	return this->service_id;
}

GetVersionID::GetVersionID():type(messageTypeEnum::typeEnums::GET_OBJECT_VERSION)
{
}

bool GetVersionID::serialize(std::string & payload_string)
{
	network_messages::GetObjectVersion ob;
	ob.set_service_id(this->service_id);

	return ob.SerializeToString(&payload_string);
}

bool GetVersionID::deserialize(const char * payload_string, uint32_t size)
{
	network_messages::GetObjectVersion ob;
	std::string s(payload_string, size);
	if (!ob.ParseFromString(s))
	{
		OSDLOG(DEBUG, "Deserialization failed");
		return false;
	}
	this->service_id = ob.service_id();
	return true;
}

const uint64_t GetVersionID::get_type()
{
	return this->type;
}

GetVersionIDAck::GetVersionIDAck(std::string service_id, uint64_t version, bool status):
	type(messageTypeEnum::typeEnums::GET_OBJECT_VERSION_ACK),
	service_id(service_id),
	version_id(version),
	status(status)
{
}

GetVersionIDAck::GetVersionIDAck():type(messageTypeEnum::typeEnums::GET_OBJECT_VERSION_ACK)
{
}

bool GetVersionIDAck::serialize(std::string &payload_string)
{
	network_messages::GetObjectVersionAck ob;

	ob.set_service_id(this->service_id);
	ob.set_object_version(this->version_id);
	ob.set_status(this->status);

	return ob.SerializeToString(&payload_string);
}

bool GetVersionIDAck::deserialize(const char * payload_string, uint32_t size)
{
	network_messages::GetObjectVersionAck ob;

	std::string s(payload_string, size);
	if (!ob.ParseFromString(s)){
		OSDLOG(DEBUG, "Deserialization failed");
		return false;
	}

	this->service_id = ob.service_id();
	this->version_id = ob.object_version();
	this->status = ob.status();

	return true;
}

std::string GetVersionIDAck::get_service_id()
{
	return this->service_id;
}

void GetVersionIDAck::set_version_id(uint64_t id)
{
	this->version_id = id;
}

uint64_t GetVersionIDAck::get_version_id()
{
	return this->version_id;
}

bool GetVersionIDAck::get_status() const
{
	return this->status;
}

void GetVersionIDAck::set_status(bool status)
{
	this->status = status;
}

const uint64_t GetVersionIDAck::get_type()
{
	return this->type;
}

NodeDeleteCli::NodeDeleteCli(boost::shared_ptr<ServiceObject> node_info):
	type(messageTypeEnum::typeEnums::NODE_DELETION),
	node_info(node_info)
{
}

NodeDeleteCli::NodeDeleteCli(): type(messageTypeEnum::typeEnums::NODE_DELETION)
{
	this->node_info.reset(new ServiceObject);
}

bool NodeDeleteCli::serialize(std::string &payload_string)
{
	network_messages::NodeDeletionCli ob;
        network_messages::service_obj* node_obj = ob.mutable_node();
	node_obj->set_service_id(this->node_info->get_service_id());
	node_obj->set_ip(this->node_info->get_ip());
	node_obj->set_port(this->node_info->get_port());
	return ob.SerializeToString(&payload_string);
}

bool NodeDeleteCli::deserialize(const char *payload_string, uint32_t size)
{
	network_messages::NodeDeletionCli ob;
	std::string s(payload_string, size);
	if (!ob.ParseFromString(s))
	{
		OSDLOG(DEBUG, "Deserialization failed");
		return false;
	}
	network_messages::service_obj node = ob.node();
	this->node_info->set_service_id(node.service_id());
	this->node_info->set_ip(node.ip());
	this->node_info->set_port(node.port());
	return true;
}

boost::shared_ptr<ServiceObject> NodeDeleteCli::get_node_serv_obj() const
{
	return this->node_info;
}

void NodeDeleteCli::set_serv_obj(boost::shared_ptr<ServiceObject> node_info)
{
	this->node_info = node_info;
}

const uint64_t NodeDeleteCli::get_type()
{
	return this->type;
}

NodeDeleteCliAck::NodeDeleteCliAck(
	std::string node_id,
	boost::shared_ptr<ErrorStatus> err
):
	type(messageTypeEnum::typeEnums::NODE_DELETION_ACK),
	node_id(node_id),
	err(err)
{
}

NodeDeleteCliAck::NodeDeleteCliAck():type(messageTypeEnum::typeEnums::NODE_DELETION_ACK)
{
}

bool NodeDeleteCliAck::serialize(std::string &payload_string)
{
	network_messages::NodeDeletionCliAck ob;
	ob.set_node_id(this->node_id);

	network_messages::errorStatus* err_obj = ob.mutable_status();
	err_obj->set_code(this->err->get_code());
	err_obj->set_msg(this->err->get_msg());
	return ob.SerializeToString(&payload_string);
}

bool NodeDeleteCliAck::deserialize(const char *payload_string, uint32_t size)
{
	return true;
}

const uint64_t NodeDeleteCliAck::get_type()
{
	return this->type;
}

NodeRejoinAfterRecovery::NodeRejoinAfterRecovery():type(messageTypeEnum::typeEnums::NODE_REJOIN_AFTER_RECOVERY)
{
}

NodeRejoinAfterRecovery::NodeRejoinAfterRecovery(std::string node_id, std::string node_ip, int32_t node_port):
	type(messageTypeEnum::typeEnums::NODE_REJOIN_AFTER_RECOVERY),
	node_id(node_id),
	node_ip(node_ip),
	node_port(node_port)
{
}

bool NodeRejoinAfterRecovery::serialize(std::string &payload_string)
{
	network_messages::NodeRejoinAfterRecovery ob;
	ob.set_node_id(this->node_id);
	ob.set_node_ip(this->node_ip);
	ob.set_node_port(this->node_port);
	return ob.SerializeToString(&payload_string);
}

bool NodeRejoinAfterRecovery::deserialize(const char *payload_string, uint32_t size)
{
	network_messages::NodeRejoinAfterRecovery ob;
	std::string s(payload_string, size);
	if (!ob.ParseFromString(s))
	{
		OSDLOG(DEBUG, "Deserialization failed");
		return false;
	}
	this->node_id = ob.node_id();
	this->node_ip = ob.node_ip();
	this->node_port = ob.node_port();
	return true;
}

std::string NodeRejoinAfterRecovery::get_node_ip() const
{
	return this->node_ip;
}

int32_t NodeRejoinAfterRecovery::get_node_port() const
{
	return this->node_port;
}

std::string NodeRejoinAfterRecovery::get_node_id() const
{
	return this->node_id;
}

const uint64_t NodeRejoinAfterRecovery::get_type()
{
	return this->type;
}

NodeRejoinAfterRecoveryAck::NodeRejoinAfterRecoveryAck():
	type(messageTypeEnum::typeEnums::NODE_REJOIN_AFTER_RECOVERY_ACK)
{
	this->error_status.reset(new ErrorStatus());
}

NodeRejoinAfterRecoveryAck::NodeRejoinAfterRecoveryAck(
	std::string node_id,
	boost::shared_ptr<ErrorStatus> err
):
	type(messageTypeEnum::typeEnums::NODE_REJOIN_AFTER_RECOVERY_ACK),
	node_id(node_id),
	error_status(err)
{
}

boost::shared_ptr<ErrorStatus> NodeRejoinAfterRecoveryAck::get_error_status() const
{
	return this->error_status;
}

std::string NodeRejoinAfterRecoveryAck::get_node_id() const
{
	return this->node_id;
}

const uint64_t NodeRejoinAfterRecoveryAck::get_type()
{
	return this->type;
}

bool NodeRejoinAfterRecoveryAck::serialize(std::string &payload_string)
{
	network_messages::NodeRejoinAfterRecoveryAck ob;
	ob.set_node_id(this->node_id);
	network_messages::errorStatus* err_obj = ob.mutable_status();
	err_obj->set_code(this->error_status->get_code());
	err_obj->set_msg(this->error_status->get_msg());
	return ob.SerializeToString(&payload_string);
}

bool NodeRejoinAfterRecoveryAck::deserialize(const char *payload_string, uint32_t size)
{
	network_messages::NodeRejoinAfterRecoveryAck ob;
	std::string s(payload_string, size);
	if (!ob.ParseFromString(s))
	{
		OSDLOG(DEBUG, "Deserialization failed");
		return false;
	}
	this->node_id = ob.node_id();
	network_messages::errorStatus status = ob.status();
	this->error_status->set_code(status.code());
	this->error_status->set_msg(status.msg());
	return true;
}

NodeStopLL::NodeStopLL():type(messageTypeEnum::typeEnums::NODE_STOP_LL)
{
}

NodeStopLL::NodeStopLL(boost::shared_ptr<ServiceObject> node):
	type(messageTypeEnum::typeEnums::NODE_STOP_LL),
	node_info(node)
{	
}

boost::shared_ptr<ServiceObject> NodeStopLL::get_node_info() const
{
	return this->node_info;
}

const uint64_t NodeStopLL::get_type()
{
	return this->type;
}

bool NodeStopLL::serialize(std::string &payload_string)
{
	network_messages::NodeStopLL ob;
	network_messages::service_obj* node_obj = ob.mutable_node();
	node_obj->set_service_id(this->node_info->get_service_id());
	node_obj->set_ip(this->node_info->get_ip());
	node_obj->set_port(this->node_info->get_port());
	return ob.SerializeToString(&payload_string);
}

bool NodeStopLL::deserialize(const char *payload_string, uint32_t size)
{
	network_messages::NodeStopLL ob;
	std::string s(payload_string, size);
	if (!ob.ParseFromString(s))
	{
		OSDLOG(DEBUG, "Deserialization failed");
		return false;
	}
	network_messages::service_obj node_obj = ob.node();
	this->node_info.reset(new ServiceObject());
	this->node_info->set_service_id(node_obj.service_id());
	this->node_info->set_ip(node_obj.ip());
	this->node_info->set_port(node_obj.port());
	return true;
}

NodeStopLLAck::NodeStopLLAck():type(messageTypeEnum::typeEnums::NODE_STOP_LL_ACK)
{
}

NodeStopLLAck::NodeStopLLAck(std::string node_id, boost::shared_ptr<ErrorStatus> err, int node_status):
	type(messageTypeEnum::typeEnums::NODE_STOP_LL_ACK),
	node_id(node_id),
	error_status(err),
	node_status(node_status)
{
}

const uint64_t NodeStopLLAck::get_type()
{
	return this->type;
}

int NodeStopLLAck::get_node_status() const
{
	return this->node_status;
}

bool NodeStopLLAck::serialize(std::string &payload_string)
{
	network_messages::NodeStopLLAck ob;
	ob.set_node_id(this->node_id);
	network_messages::errorStatus* err_obj = ob.mutable_status();
	err_obj->set_code(this->error_status->get_code());
	err_obj->set_msg(this->error_status->get_msg());
	ob.set_node_status(this->node_status);
	return ob.SerializeToString(&payload_string);
}

bool NodeStopLLAck::deserialize(const char *payload_string, uint32_t size)
{
	network_messages::NodeStopLLAck ob;
	std::string s(payload_string, size);
	if (!ob.ParseFromString(s))
	{
		OSDLOG(DEBUG, "Deserialization failed");
		return false;
	}
	this->node_id = ob.node_id();
	network_messages::errorStatus err_obj = ob.status();
	this->error_status.reset(new ErrorStatus);
	this->error_status->set_code(err_obj.code());
	this->error_status->set_msg(err_obj.msg());
	this->node_status = ob.node_status();
	return true;
}

boost::shared_ptr<ErrorStatus> NodeStopLLAck::get_error_status() const
{
	return this->error_status;
}

std::string NodeStopLLAck::get_node_id() const
{
	return this->node_id;
}

GetOsdClusterStatus::GetOsdClusterStatus():type(messageTypeEnum::typeEnums::GET_CLUSTER_STATUS)
{
}

std::string GetOsdClusterStatus::get_node_id() const
{
	return this->node_id;
}

const uint64_t GetOsdClusterStatus::get_type()
{
	return this->type;
}

bool GetOsdClusterStatus::serialize(std::string &payload_string)
{
	network_messages::GetClusterStatus ob;
	ob.set_service_id(this->node_id);
	return true;
}

bool GetOsdClusterStatus::deserialize(const char *payload_string, uint32_t size)
{
	network_messages::GetClusterStatus ob;
	std::string s(payload_string, size);
	if (!ob.ParseFromString(s))
	{
		OSDLOG(DEBUG, "Deserialization failed");
		return false;
	}
	this->node_id = ob.service_id();
	return true;
}

GetOsdClusterStatusAck::GetOsdClusterStatusAck(
	std::map<std::string, network_messages::NodeStatusEnum> node_to_status_map,
	boost::shared_ptr<ErrorStatus> error_status
)
:
	type(messageTypeEnum::typeEnums::GET_CLUSTER_STATUS_ACK),
	node_to_status_map(node_to_status_map),
	error_status(error_status)
{
}

const uint64_t GetOsdClusterStatusAck::get_type()
{
	return this->type;
}

bool GetOsdClusterStatusAck::serialize(std::string &payload_string)
{
	network_messages::GetClusterStatusAck ob;
	std::map<std::string, network_messages::NodeStatusEnum>::iterator it = node_to_status_map.begin();
	for(; it != node_to_status_map.end(); ++it)
	{
		network_messages::GetClusterStatusAck_pair * pair_obj = ob.add_node_status_list();
		pair_obj->set_node_id(it->first);
		pair_obj->set_status_enum(it->second);
	}
        network_messages::errorStatus* err_obj = ob.mutable_status();
	err_obj->set_code(this->error_status->get_code());
	err_obj->set_msg(this->error_status->get_msg());
	return ob.SerializeToString(&payload_string);
}

bool GetOsdClusterStatusAck::deserialize(const char *payload_string, uint32_t size)
{
	//Not required right now
	return true;
}

UpdateContainer::UpdateContainer(): type(messageTypeEnum::typeEnums::CONTAINER_UPDATE)
{
}

UpdateContainer::UpdateContainer(
	std::string meta_file_path,
	std::string operation
):
	type(messageTypeEnum::typeEnums::CONTAINER_UPDATE),
	meta_file_path(meta_file_path),
	operation(operation)
{
}

const uint64_t UpdateContainer::get_type()
{
	return this->type;
}

std::string UpdateContainer::get_meta_file_path()
{
	return this->meta_file_path;
}

std::string UpdateContainer::get_operation()
{
	return this->operation;
}

bool UpdateContainer::serialize(std::string & payload_string)
{
	network_messages::UpdateContainer ob;
	ob.set_meta_file_path(this->meta_file_path);
	ob.set_operation(this->operation);
	return ob.SerializeToString(&payload_string);
}

bool UpdateContainer::deserialize(const char *payload_string, uint32_t size)
{
	network_messages::UpdateContainer ob;
	std::string s(payload_string, size);
	if (!ob.ParseFromString(s))
	{
		OSDLOG(DEBUG, "Deserialization failed");
		return false;
	}

	this->meta_file_path = ob.meta_file_path();
	this->operation = ob.operation();
	return true;
}

ReleaseTransactionLock::ReleaseTransactionLock(): type(messageTypeEnum::typeEnums::RELEASE_TRANSACTION_LOCK)
{
}

ReleaseTransactionLock::ReleaseTransactionLock(
	std::string obj_hash,
	std::string operation
):
	type(messageTypeEnum::typeEnums::RELEASE_TRANSACTION_LOCK),
	obj_hash(obj_hash),
	operation(operation)
{
}

const uint64_t ReleaseTransactionLock::get_type()
{
	return this->type;
}

std::string ReleaseTransactionLock::get_object_hash()
{
	return this->obj_hash;
}

std::string ReleaseTransactionLock::get_operation()
{
	return this->operation;
}

bool ReleaseTransactionLock::serialize(std::string & payload_string)
{
	network_messages::ReleaseTransactionLock ob;
	ob.set_lock(this->obj_hash);
	ob.set_operation(this->operation);
	return ob.SerializeToString(&payload_string);
}

bool ReleaseTransactionLock::deserialize(const char *payload_string, uint32_t size)
{
	network_messages::ReleaseTransactionLock ob;
	std::string s(payload_string, size);
	if (!ob.ParseFromString(s))
	{
		OSDLOG(DEBUG, "Deserialization failed");
		return false;
	}

	this->obj_hash = ob.lock();
	this->operation = ob.operation();
	return true;
}

StatusAck::StatusAck(
):
	type(messageTypeEnum::typeEnums::STATUS_ACK),
	status(false)
{
}

const uint64_t StatusAck::get_type()
{
	return this->type;
}

bool StatusAck::get_result()
{
	return this->status;
}

void StatusAck::set_result(bool result)
{
	this->status = result;
}

bool StatusAck::serialize(std::string & payload_string)
{
        network_messages::StatusAck ob;
	ob.set_status(this->status);
        return ob.SerializeToString(&payload_string);
}

bool StatusAck::deserialize(const char *payload_string, uint32_t size)
{
	network_messages::StatusAck ob;
	std::string s(payload_string, size);
	if (!ob.ParseFromString(s))
	{
		OSDLOG(DEBUG, "Deserialization failed");
		return false;
	}

	this->status = ob.status();
	return true;
}

InitiateClusterRecovery::InitiateClusterRecovery():type(messageTypeEnum::typeEnums::INITIATE_CLUSTER_RECOVERY)
{
}

const uint64_t InitiateClusterRecovery::get_type()
{
	return messageTypeEnum::typeEnums::INITIATE_CLUSTER_RECOVERY;
}

bool InitiateClusterRecovery::serialize(std::string &payload_string)
{
	return true;
}

bool InitiateClusterRecovery::deserialize(const char *payload_string, uint32_t size)
{
	return true;
}

}// namespace comm_messages
