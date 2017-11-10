#include "communication/stubs.h"

ServiceObject::ServiceObject()
{
}

ServiceObject::~ServiceObject()
{
}

void ServiceObject::set_service_id(std::string service_id)
{
	this->service_id = service_id;
}

void ServiceObject::set_ip(std::string ip){
	this->ip = ip;
}

void ServiceObject::set_port(uint64_t port){
	this->port = port;
}

std::string ServiceObject::get_service_id()
{
	return this->service_id;
}

std::string ServiceObject::get_ip(){
	return this->ip;
}

uint64_t ServiceObject::get_port(){
	return this->port;
}

ErrorStatus::ErrorStatus()
{
}

ErrorStatus::ErrorStatus(int32_t code, std::string msg) :
        code(code),msg(msg)
{
}

ErrorStatus::~ErrorStatus()
{
}

void ErrorStatus::set_code(int32_t code)
{
        this->code = code;
}

void ErrorStatus::set_msg(std::string msg)
{
        this->msg = msg;
}

int32_t ErrorStatus::get_code()
{
        return this->code;
}

std::string ErrorStatus::get_msg()
{
        return this->msg;
}

LocalLeaderNodeStatus::LocalLeaderNodeStatus()
{
}

LocalLeaderNodeStatus::LocalLeaderNodeStatus(LL_NODE_STATUS local_node_status)
{
	this->local_node_status = local_node_status;
}

LocalLeaderNodeStatus::~LocalLeaderNodeStatus()
{
}

void LocalLeaderNodeStatus::set_local_node_status(LocalLeaderNodeStatus::LL_NODE_STATUS local_node_status)
{
	this->local_node_status = local_node_status;
}

LocalLeaderNodeStatus::LL_NODE_STATUS LocalLeaderNodeStatus::get_local_node_status()
{
	return this->local_node_status;
}

GlobalNodeStatus::GlobalNodeStatus()
{
}

GlobalNodeStatus::GlobalNodeStatus(GL_NODE_STATUS global_node_status)
{
        this->global_node_status = global_node_status;
}

GlobalNodeStatus::~GlobalNodeStatus()
{
}

void GlobalNodeStatus::set_global_node_status(GlobalNodeStatus::GL_NODE_STATUS global_node_status)
{
        this->global_node_status = global_node_status;
}

GlobalNodeStatus::GL_NODE_STATUS GlobalNodeStatus::get_global_node_status()
{
        return this->global_node_status;
}
