//this is a dummy implementation.
#ifndef __MESSAGE_H_213__
#define __MESSAGE_H_213__

#include <list>
#include <utility>
#include <boost/shared_ptr.hpp>
#include <tr1/tuple>

#include "libraryUtils/record_structure.h"
#include "communication/message_binding.pb.h"
#include "communication/stubs.h"

//using namespace std::tr1;

namespace comm_messages
{

typedef boost::shared_ptr<ServiceObject> ptr_service_obj;
typedef boost::shared_ptr<std::list<uint32_t> > ptr_int32_list;
typedef std::map<ptr_service_obj, ptr_int32_list> serv_map_items;
typedef std::pair<uint32_t, ptr_service_obj> comp_pair;
typedef std::list<comp_pair> service_list;
typedef std::pair<uint32_t, bool> comp_status_pair;
typedef std::list<comp_status_pair> comp_status_list;

class Message
{
	public:
		Message(){};
		virtual ~Message(){};

		virtual const uint64_t get_type() = 0;
		virtual bool serialize(std::string &) = 0;
		virtual bool deserialize(const char *, uint32_t) = 0;
		virtual std::map<std::string, std::string> get_header_map(){
			std::map<std::string, std::string> map;
			return map;
		};
};

/*
class ErrorMessage: public Message
{
	public:
		ErrorMessage(uint64_t);
		~ErrorMessage(){};

		const uint64_t get_type();
		bool serialize(std::string &, uint32_t payload_string){};		//TODO
		bool deserialize(const char *, uint32_t payload_string){};	//TODO

	private:
		const uint64_t type;
		const uint64_t error_num;
};
*/

class HeartBeat: public Message
{

	public:
		HeartBeat();
		HeartBeat(std::string service_id, uint32_t);
		HeartBeat(std::string service_id, uint32_t, int);
		~HeartBeat(){};

		bool serialize(std::string &);
		bool deserialize(const char *, uint32_t);

		uint64_t get_fd();
		void set_fd(uint64_t);
		std::string get_service_id();
		void set_service_id(std::string);
		const uint64_t get_type();
		uint32_t get_sequence();
		std::string get_msg();
		int get_hfs_stat() const;
		void set_hfs_stat(int);
	private:
		std::string service_id;
		std::string msg;
		uint64_t fd;
		const uint64_t type;
		uint32_t sequence;
		int hfs_stat;
};

class HeartBeatAck: public Message
{

	public:
		HeartBeatAck();
		HeartBeatAck(uint32_t);
		HeartBeatAck(uint32_t, int);
		~HeartBeatAck(){};

		bool serialize(std::string &);
		bool deserialize(const char *, uint32_t);

		uint64_t get_fd();
		void set_fd(uint64_t);
		const uint64_t get_type();
		uint32_t get_sequence();
		int get_node_stat() const;
		void set_node_stat(int);
		std::string get_msg();
		

	private:
		std::string msg;
		uint64_t fd;
		const uint64_t type;
		uint32_t sequence;
		int node_stat;
};

class GetServiceComponent: public Message
{

	public:
		GetServiceComponent();
		GetServiceComponent(std::string service_id);
		~GetServiceComponent(){};

		bool serialize(std::string &);
		bool deserialize(const char *, uint32_t);

		uint64_t get_fd();
		void set_fd(uint64_t);
		std::string get_service_id();
		void set_service_id(std::string);
		const uint64_t get_type();

	private:
		std::string service_id;
		uint64_t fd;
		const uint64_t type;
};

class GetServiceComponentAck: public Message
{

	public:
		GetServiceComponentAck();
		GetServiceComponentAck(std::vector<uint32_t>, boost::shared_ptr<ErrorStatus> error_status);
		~GetServiceComponentAck(){};

		bool serialize(std::string &);
		bool deserialize(const char *, uint32_t);

		uint64_t get_fd();
		void set_fd(uint64_t);
		const uint64_t get_type();
		std::vector<uint32_t> get_component_list();
		void set_component_list(std::vector<uint32_t>);
		bool get_status();
		void set_status(bool);

	private:
		std::vector<uint32_t> component_list;
		boost::shared_ptr<ErrorStatus> error_status;
		uint64_t fd;
		const uint64_t type;
		bool status;
};

class GetGlobalMap: public Message
{

	public:
		GetGlobalMap();
		GetGlobalMap(std::string service_id);
		~GetGlobalMap(){};

		bool serialize(std::string &);
		bool deserialize(const char *, uint32_t);

		uint64_t get_fd();
		void set_fd(uint64_t);
		std::string get_service_id();
		void set_service_id(std::string);
		const uint64_t get_type();

	private:
		std::string service_id;
		uint64_t fd;
		const uint64_t type;
};

class GlobalMapInfo: public Message
{

	public:
		GlobalMapInfo();
		GlobalMapInfo(
			std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> container_pair,
			std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> account_pair,
			std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> updater_pair,
			std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> object_pair,
			float gl_version
		);
		~GlobalMapInfo(){};

		bool serialize(std::string &);
		bool deserialize(const char *, uint32_t);

		uint64_t get_fd();
		void set_fd(uint64_t);
		const uint64_t get_type();
		bool get_status();
		void set_status(bool);
		/*void add_service_object_in_container(recordStructure::ComponentInfoRecord);
		void add_service_object_in_account(recordStructure::ComponentInfoRecord);
		void add_service_object_in_updater(recordStructure::ComponentInfoRecord);
		void add_service_object_in_objectService(recordStructure::ComponentInfoRecord);*/
		void set_version_in_container(float);
		void set_version_in_account(float);
		void set_version_in_updater(float);
		void set_version_in_object(float);
		float get_version_in_container();
		float get_version_in_account();
		float get_version_in_updater();
		float get_version_in_object();
		void set_version(float);
		float get_version();
		std::vector<recordStructure::ComponentInfoRecord> get_service_object_of_container();
		std::vector<recordStructure::ComponentInfoRecord> get_service_object_of_account();
		std::vector<recordStructure::ComponentInfoRecord> get_service_object_of_updater();
		std::vector<recordStructure::ComponentInfoRecord> get_service_object_of_objectService();

	private:
		std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> container_pair;
		std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> account_pair;
		std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> updater_pair;
		std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> object_pair;
		uint64_t fd;
		const uint64_t type;
		float version;
		bool status;
};

class OsdStartMonitoring: public Message
{
	public:
		OsdStartMonitoring();
		OsdStartMonitoring(std::string, uint32_t, std::string);
		~OsdStartMonitoring(){};

		bool serialize(std::string &);
		bool deserialize(const char *, uint32_t);

		uint64_t get_fd();
		void set_fd(uint64_t);
		std::string get_service_id();
		void set_service_id(std::string);
		std::string get_ip();
		void set_ip(std::string);
		uint32_t get_port();
		void set_port(uint32_t);
		virtual const uint64_t get_type();

	private:
		std::string service_id;
		uint32_t port;
		std::string ip;
		uint64_t fd;
		const uint64_t type;
};

class OsdStartMonitoringAck: public Message
{
	public:
		OsdStartMonitoringAck();
		OsdStartMonitoringAck(std::string, boost::shared_ptr<ErrorStatus> error_status);
		~OsdStartMonitoringAck(){};

		bool serialize(std::string &);
		bool deserialize(const char *, uint32_t);

		uint64_t get_fd();
		void set_fd(uint64_t);
		boost::shared_ptr<ErrorStatus> get_error_status();
		void set_error_status(boost::shared_ptr<ErrorStatus>);
		std::string get_service_id();
		void set_service_id(std::string);
		const uint64_t get_type();

	private:
		std::string service_id;
		boost::shared_ptr<ErrorStatus> error_status;
		uint64_t fd;
		const uint64_t type;
};

class LocalLeaderStartMonitoring: public Message
{
	public:
		LocalLeaderStartMonitoring();
		LocalLeaderStartMonitoring(std::string);
		~LocalLeaderStartMonitoring(){};

		bool serialize(std::string &);
		bool deserialize(const char *, uint32_t);

		uint64_t get_fd();
		void set_fd(uint64_t);
		std::string get_service_id();
		void set_service_id(std::string);
		const uint64_t get_type();

	private:
		std::string service_id;
		uint64_t fd;
		const uint64_t type;
};

class LocalLeaderStartMonitoringAck: public Message
{
	public:
		LocalLeaderStartMonitoringAck();
		LocalLeaderStartMonitoringAck(std::string);
		~LocalLeaderStartMonitoringAck(){};

		bool serialize(std::string &);
		bool deserialize(const char *, uint32_t);

		uint64_t get_fd();
		void set_fd(uint64_t);
		std::string get_service_id();
		void set_service_id(std::string);
		const uint64_t get_type();
		void set_status(bool);
		bool get_status();

	private:
		std::string service_id;
		uint64_t fd;
		const uint64_t type;
		bool status;
};


class RecvProcStartMonitoring: public Message
{
	public:
		RecvProcStartMonitoring();
		RecvProcStartMonitoring(std::string);
		~RecvProcStartMonitoring(){};

		bool serialize(std::string &);
		bool deserialize(const char *, uint32_t);

		uint64_t get_fd();
		void set_fd(uint64_t);
		const uint64_t get_type();
		std::string get_proc_id();
		void set_proc_id(std::string);

	private:
		uint64_t fd;
		const uint64_t type;
		std::string proc_id;
};

class RecvProcStartMonitoringAck: public Message
{
	public:
		RecvProcStartMonitoringAck();
		RecvProcStartMonitoringAck(
			std::map<recordStructure::ComponentInfoRecord, std::list<uint32_t> >,
			recordStructure::ComponentInfoRecord source_node,
			bool status
			);
		~RecvProcStartMonitoringAck(){};

		bool serialize(std::string &);
		bool deserialize(const char *, uint32_t);

		uint64_t get_fd();
		bool get_status();
		void set_status(bool);
		void set_fd(uint64_t);
		const uint64_t get_type();
		recordStructure::ComponentInfoRecord get_source_service_object();
		void set_source_service_object(recordStructure::ComponentInfoRecord source_node_obj);
		std::map<recordStructure::ComponentInfoRecord, std::list<uint32_t> > get_service_map();
		void set_service_map(std::map<recordStructure::ComponentInfoRecord, std::list<uint32_t> > service_map);

	private:
		uint64_t fd;
		const uint64_t type;
		std::map<recordStructure::ComponentInfoRecord, std::list<uint32_t> > service_map;
		recordStructure::ComponentInfoRecord source_service;
		bool status;
};


class CompTransferInfo: public Message
{
	public:
		CompTransferInfo();
		CompTransferInfo(std::string, std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > comp_service_list);
		~CompTransferInfo(){};

		bool serialize(std::string &);
		bool deserialize(const char *, uint32_t);

		uint64_t get_fd();
		void set_fd(uint64_t);
		const uint64_t get_type();
		void set_service_id(std::string);
		std::string get_service_id();
		void set_comp_service_list(std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > comp_service_list);
		//service_list get_service_list();
		std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > get_comp_service_list();

	private:
		uint64_t fd;
		const uint64_t type;
		std::string service_id;
		//service_list comp_service_list;
		std::vector<std::pair<int32_t, recordStructure::ComponentInfoRecord> > comp_service_list;
};

class CompTransferFinalStat: public Message
{
	public:
		CompTransferFinalStat();
		CompTransferFinalStat(std::string, std::vector<std::pair<int32_t, bool> > comp_pair_list);
		~CompTransferFinalStat(){};

		bool serialize(std::string &);
		bool deserialize(const char *, uint32_t);

		uint64_t get_fd();
		void set_fd(uint64_t);
		const uint64_t get_type();
		std::vector<std::pair<int32_t, bool> > get_component_pair_list();
		void set_component_pair_list(std::vector<std::pair<int32_t, bool> > comp_pair_list);
		std::string get_service_id();
		void set_service_id(std::string);
		bool get_status();
		void set_status(bool);

	private:
		std::string service_id;
		uint64_t fd;
		const uint64_t type;
		std::vector<std::pair<int32_t, bool> > comp_pair_list;
		bool status;
};

class CompTransferInfoAck: public Message
{
	public:
		CompTransferInfoAck();
		CompTransferInfoAck(
			std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> container_pair,
			std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> account_pair,
			std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> updater_pair,
			std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> object_pair,
			float version
			);
		~CompTransferInfoAck(){};

		bool serialize(std::string &);
		bool deserialize(const char *, uint32_t);

		const uint64_t get_type();
		uint64_t get_fd();
		void set_fd(uint64_t);

	private:
		GlobalMapInfo gl_map_info_obj;
		const uint64_t type;
		uint64_t fd;
};

class CompTransferFinalStatAck: public Message
{
	public:
		CompTransferFinalStatAck();
		CompTransferFinalStatAck(
			std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> container_pair,
			std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> account_pair,
			std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> updater_pair,
			std::tr1::tuple<std::vector<recordStructure::ComponentInfoRecord>, float> object_pair,
			float version
			);
		~CompTransferFinalStatAck(){};

		bool serialize(std::string &);
		bool deserialize(const char *, uint32_t);

		const uint64_t get_type();
		uint64_t get_fd();
		void set_fd(uint64_t);

	private:
		GlobalMapInfo gl_map_info_obj;
		const uint64_t type;
		uint64_t fd;
};


class NodeAdditionGl : public Message
{
        public:
                NodeAdditionGl();
                NodeAdditionGl(std::string service_id);
                ~NodeAdditionGl(){};

                bool serialize(std::string &);
                bool deserialize(const char *, uint32_t);

                uint64_t get_fd();
                void set_fd(uint64_t);
                std::string get_service_id();
                void set_service_id(std::string);
                const uint64_t get_type();
        private:
                std::string service_id;
                uint64_t fd;
                const uint64_t type;
};

class NodeAdditionGlAck : public Message
{
        public:
                NodeAdditionGlAck();
                NodeAdditionGlAck(std::list<boost::shared_ptr<ServiceObject> >, boost::shared_ptr<ErrorStatus>);
                ~NodeAdditionGlAck() {};

                bool serialize(std::string &);
                bool deserialize(const char *, uint32_t);

                void add_service_object(boost::shared_ptr<ServiceObject>);
                std::list<boost::shared_ptr<ServiceObject> >  get_service_objects();

                uint64_t get_fd();
                void set_fd(uint64_t);
                boost::shared_ptr<ErrorStatus> get_error_status();
                void set_error_status(boost::shared_ptr<ErrorStatus>);
                const uint64_t get_type();
        private:
                std::list<boost::shared_ptr<ServiceObject> > service_obj_list;
                boost::shared_ptr<ErrorStatus> error_status;
                uint64_t fd;
                const uint64_t type;


};

class NodeAdditionFinalAck : public Message
{
	public:
		NodeAdditionFinalAck(bool success);
		NodeAdditionFinalAck();
		~NodeAdditionFinalAck() {};
		bool serialize(std::string &);
		bool deserialize(const char *, uint32_t);
		bool get_status();
		const uint64_t get_type();
	private:
		const uint64_t type;
		bool success;
};

class NodeAdditionCli : public Message
{
        public:
                NodeAdditionCli();
                NodeAdditionCli(std::list<boost::shared_ptr<ServiceObject> >);
                ~NodeAdditionCli() {};

                bool serialize(std::string &);
                bool deserialize(const char *, uint32_t);

                uint64_t get_fd();
                void set_fd(uint64_t);
                void add_node_object(boost::shared_ptr<ServiceObject>);
                std::list<boost::shared_ptr<ServiceObject> > get_node_objects();
                const uint64_t get_type();
        private:
                std::list<boost::shared_ptr<ServiceObject> > node_list;
                uint64_t fd;
                const uint64_t type;
};

class NodeAdditionCliAck : public Message
{
        public:
                NodeAdditionCliAck();
                NodeAdditionCliAck(std::list<std::pair<boost::shared_ptr<ServiceObject>, boost::shared_ptr<ErrorStatus> > >);
                ~NodeAdditionCliAck() {};

                bool serialize(std::string &);
                bool deserialize(const char *, uint32_t);

                uint64_t get_fd();
                void set_fd(uint64_t);
                void add_node_in_node_status_list(boost::shared_ptr<ServiceObject>, boost::shared_ptr<ErrorStatus> );        
                std::list<std::pair<boost::shared_ptr<ServiceObject>, boost::shared_ptr<ErrorStatus> > > get_node_status_list();
                const uint64_t get_type();
        private:
                std::list<std::pair<boost::shared_ptr<ServiceObject>, boost::shared_ptr<ErrorStatus> > > node_status_list;
                uint64_t fd;
                const uint64_t type;
};

class NodeRetire : public Message
{
	public:
		NodeRetire();
		NodeRetire(boost::shared_ptr<ServiceObject> node);
		~NodeRetire() {};
		
		bool serialize(std::string &);
                bool deserialize(const char *, uint32_t);

                uint64_t get_fd();
                void set_fd(uint64_t);
		void set_node(boost::shared_ptr<ServiceObject> node);
		boost::shared_ptr<ServiceObject> get_node();
		const uint64_t get_type();
	private:
		boost::shared_ptr<ServiceObject> node;
		uint64_t fd;
                const uint64_t type;
};

class NodeRetireAck : public Message
{
        public:
                NodeRetireAck();
                NodeRetireAck(std::string node_id, boost::shared_ptr<ErrorStatus> error_status);
                ~NodeRetireAck() {};

                bool serialize(std::string &);
                bool deserialize(const char *, uint32_t);

                uint64_t get_fd();
                void set_fd(uint64_t);
                void set_status(boost::shared_ptr<ErrorStatus> error_status);
                boost::shared_ptr<ErrorStatus> get_status();
		void set_node_id(std::string node_id);
		std::string get_node_id();
                const uint64_t get_type();
        private:
		std::string node_id;
                boost::shared_ptr<ErrorStatus> error_status;
                uint64_t fd;
                const uint64_t type;
};

class StopServices : public Message
{
	public:
		StopServices();
		StopServices(std::string service_id);
		~StopServices() {};
	
		bool serialize(std::string &);
                bool deserialize(const char *, uint32_t);

		uint64_t get_fd();
                void set_fd(uint64_t);
		void set_service_id(std::string service_id);
                std::string get_service_id();
                const uint64_t get_type();
	private:
		std::string service_id;
		uint64_t fd;
                const uint64_t type;
};

class StopServicesAck : public Message
{
	public:
		StopServicesAck();
		StopServicesAck(boost::shared_ptr<ErrorStatus> error_status);
		~StopServicesAck() {};

		bool serialize(std::string &);
                bool deserialize(const char *, uint32_t);

		uint64_t get_fd();
                void set_fd(uint64_t);
		void set_status(int32_t code, std::string msg);
		boost::shared_ptr<ErrorStatus> get_status();
		const uint64_t get_type();
	private:
		boost::shared_ptr<ErrorStatus> error_status;
                uint64_t fd;
                const uint64_t type;
};

class NodeSystemStopCli : public Message
{
	public:
		NodeSystemStopCli();
		NodeSystemStopCli(std::string node_id);
		~NodeSystemStopCli() {};

		bool serialize(std::string &);
		bool deserialize(const char *, uint32_t);

                uint64_t get_fd();
                void set_fd(uint64_t);
		void set_node_id(std::string node_id);
		std::string get_node_id();
		const uint64_t get_type();
	private:
		std::string node_id;
		uint64_t fd;
                const uint64_t type;
};

class LocalNodeStatus : public Message
{
        public:
                LocalNodeStatus();
                LocalNodeStatus(std::string node_id);
                ~LocalNodeStatus() {};

                bool serialize(std::string &);
                bool deserialize(const char *, uint32_t);

                uint64_t get_fd();
                void set_fd(uint64_t);
                void set_node_id(std::string node_id);
                std::string get_node_id();
                const uint64_t get_type();
        private:
                std::string node_id;
                uint64_t fd;
                const uint64_t type;
};

class NodeStatus : public Message
{
	public:
		NodeStatus();
		NodeStatus(boost::shared_ptr<ServiceObject> node);
		~NodeStatus() {};
	
		bool serialize(std::string &);
	        bool deserialize(const char *, uint32_t);

                uint64_t get_fd();
                void set_fd(uint64_t);
		void set_node(boost::shared_ptr<ServiceObject> node);
		boost::shared_ptr<ServiceObject> get_node();
		const uint64_t get_type();
	private:
		boost::shared_ptr<ServiceObject> node;
		uint64_t fd;
                const uint64_t type;
};
	

class NodeStatusAck : public Message
{
        public:
                NodeStatusAck();
                NodeStatusAck(network_messages::NodeStatusEnum node_status);
                ~NodeStatusAck() {};

                bool serialize(std::string &);
                bool deserialize(const char *, uint32_t);

                uint64_t get_fd();
                void set_fd(uint64_t);
                void set_status(network_messages::NodeStatusEnum node_status);
                network_messages::NodeStatusEnum get_status();
                const uint64_t get_type();
        private:
                network_messages::NodeStatusEnum node_status;
                uint64_t fd;
                const uint64_t type;
};

class NodeStopCli : public Message
{
        public:
                NodeStopCli();
                NodeStopCli(std::string node_id);
                ~NodeStopCli() {}; 

                bool serialize(std::string &);
                bool deserialize(const char *, uint32_t);

                uint64_t get_fd();
                void set_fd(uint64_t);
                void set_node_id(std::string node_id);
                std::string get_node_id();
                const uint64_t get_type();
        private:
                std::string node_id;
                uint64_t fd;
                const uint64_t type;
};

class NodeStopCliAck : public Message
{
        public:
                NodeStopCliAck();
                NodeStopCliAck(boost::shared_ptr<ErrorStatus> error_status);
                ~NodeStopCliAck() {};

                bool serialize(std::string &);
                bool deserialize(const char *, uint32_t);

                uint64_t get_fd();
                void set_fd(uint64_t);
                void set_status(int32_t code, std::string msg);
                boost::shared_ptr<ErrorStatus> get_status();
                const uint64_t get_type();
        private:
                boost::shared_ptr<ErrorStatus> error_status;
                uint64_t fd;
                const uint64_t type;
};

class NodeFailover : public Message
{
	public:
		NodeFailover();
		NodeFailover(std::string node_id , boost::shared_ptr<ErrorStatus> error_status);
		~NodeFailover() {};

		bool serialize(std::string &);
                bool deserialize(const char *, uint32_t);

                uint64_t get_fd();
                void set_fd(uint64_t);
		void set_status(int32_t code, std::string msg);
                boost::shared_ptr<ErrorStatus> get_status();
		void set_node_id(std::string node_id);
		std::string get_node_id();
		const uint64_t get_type();
	private:
		std::string node_id;
                boost::shared_ptr<ErrorStatus> error_status;
                uint64_t fd;
                const uint64_t type;
};

class NodeFailoverAck : public Message
{
        public:
                NodeFailoverAck();
                NodeFailoverAck(boost::shared_ptr<ErrorStatus> error_status);
                ~NodeFailoverAck() {};

                bool serialize(std::string &);
                bool deserialize(const char *, uint32_t);

                uint64_t get_fd();
                void set_fd(uint64_t);
                void set_status(int32_t code, std::string msg);
                boost::shared_ptr<ErrorStatus> get_status();
                const uint64_t get_type();
        private:
                boost::shared_ptr<ErrorStatus> error_status;
                uint64_t fd;
                const uint64_t type;
};

class TakeGlOwnership : public Message
{
	public:
		TakeGlOwnership();
		TakeGlOwnership(std::string old_gl_id, std::string new_gl_id);
		~TakeGlOwnership() {};

		bool serialize(std::string &);
                bool deserialize(const char *, uint32_t);

                uint64_t get_fd();
                void set_fd(uint64_t);
		void set_old_gl_id(std::string old_gl_id);
		std::string get_old_gl_id();
		void set_new_gl_id(std::string new_gl_id);
                std::string get_new_gl_id();
		const uint64_t get_type();
	private:
		std::string old_gl_id;
		std::string new_gl_id;
		uint64_t fd;
                const uint64_t type;
};

class TakeGlOwnershipAck : public Message
{
        public:
                TakeGlOwnershipAck();
                TakeGlOwnershipAck(std::string new_gl_id, boost::shared_ptr<ErrorStatus> status);
                ~TakeGlOwnershipAck() {};

                bool serialize(std::string &);
                bool deserialize(const char *, uint32_t);

                uint64_t get_fd();
                void set_fd(uint64_t);
		void set_new_gl_id(std::string new_gl_id);
                std::string get_new_gl_id();
		void set_status(boost::shared_ptr<ErrorStatus> status);
		boost::shared_ptr<ErrorStatus> get_status();
		const uint64_t get_type();
	private:
		std::string new_gl_id;
		boost::shared_ptr<ErrorStatus> status;
		uint64_t fd;
                const uint64_t type;
};
		
class StopProxy : public Message
{
	public:
		StopProxy();
		~StopProxy() {};

		bool serialize(std::string &);
		bool deserialize(const char *, uint32_t);

	private:
		const uint64_t type;
};

class StopProxyAck : public Message
{
	public:
		StopProxyAck();
		~StopProxyAck() {};

		bool serialize(std::string &);
		bool deserialize(const char *, uint32_t);

		boost::shared_ptr<ErrorStatus> get_status();
		void set_status(int32_t code, std::string msg);
	private:
		const uint64_t type;
		boost::shared_ptr<ErrorStatus> error_status;
};

class TransferComponents : public Message
{
	public:
		TransferComponents(
			std::map<recordStructure::ComponentInfoRecord, std::list<uint32_t> > service_map,
			std::map<std::string, std::string> header_dict
		);
		TransferComponents();
		~TransferComponents() {};

		bool serialize(std::string &);
		bool deserialize(const char *, uint32_t);

		std::map<recordStructure::ComponentInfoRecord, std::list<uint32_t> > get_comp_map() const;
		void set_comp_map(std::map<recordStructure::ComponentInfoRecord, std::list<uint32_t> > service_map);
		std::map<std::string, std::string> get_header_map();
		const uint64_t get_type();

	private:
		const uint64_t type;
		std::map<recordStructure::ComponentInfoRecord, std::list<uint32_t> > component_map;
		std::map<std::string, std::string> header_dict;
};


class TransferComponentsAck : public Message
{
	public:
		TransferComponentsAck(std::vector<std::pair<int32_t, bool> > comp_pair_list);
		TransferComponentsAck();
		~TransferComponentsAck() {};

		bool serialize(std::string &);
		bool deserialize(const char *, uint32_t);

		std::vector<std::pair<int32_t, bool> > get_comp_list() const;
		void set_comp_list(std::vector<std::pair<int32_t, bool> > comp_list);
		const uint64_t get_type();

	private:
		const uint64_t type;
		std::vector<std::pair<int32_t, bool> > comp_pair_list;
};

class BlockRequests : public Message
{
	public:
		BlockRequests();
		BlockRequests(std::map<std::string, std::string> dict);
		~BlockRequests() {};

		bool serialize(std::string &);
		bool deserialize(const char *, uint32_t);
		const uint64_t get_type();

		std::map<std::string, std::string> get_header_map();
	private:
		const uint64_t type;
		bool status;
		std::map<std::string, std::string> header_dict;
};

class BlockRequestsAck : public Message
{
	public:
		BlockRequestsAck();
		BlockRequestsAck(bool status);
		~BlockRequestsAck() {};
		bool serialize(std::string &);
		bool deserialize(const char *, uint32_t);
		const uint64_t get_type();
		bool get_status() const;
		void set_status(bool);
	private:
		const uint64_t type;
		bool status;
};

class GetVersionID : public Message
{
	public:
		GetVersionID();
		GetVersionID(std::string service_id);
		~GetVersionID() {};
		bool serialize(std::string &);
		bool deserialize(const char *, uint32_t);
		const uint64_t get_type();
		std::string get_service_id();
	private:
		const uint64_t type;
		std::string service_id;
		
};

class GetVersionIDAck : public Message
{
	public:
		GetVersionIDAck();
		GetVersionIDAck(std::string service_id, uint64_t version, bool status);
		~GetVersionIDAck() {};
		bool serialize(std::string &);
		bool deserialize(const char *, uint32_t);
		const uint64_t get_type();
		uint64_t get_version_id();
		void set_version_id(uint64_t version_id);
		bool get_status() const;
		void set_status(bool);
		std::string get_service_id();
	private:
		const uint64_t type;
		std::string service_id;
		uint64_t version_id;
		bool status;
};

class NodeDeleteCli : public Message
{
	public:
		NodeDeleteCli();
		NodeDeleteCli(boost::shared_ptr<ServiceObject> node_info);
		~NodeDeleteCli() {};
		bool serialize(std::string &);
		bool deserialize(const char *, uint32_t);
		const uint64_t get_type();
		boost::shared_ptr<ServiceObject> get_node_serv_obj() const;
		void set_serv_obj(boost::shared_ptr<ServiceObject>);
	private:
		const uint64_t type;
		boost::shared_ptr<ServiceObject> node_info;

};

class NodeDeleteCliAck : public Message
{
	public:
		NodeDeleteCliAck();
		NodeDeleteCliAck(std::string node_id, boost::shared_ptr<ErrorStatus> err);
		~NodeDeleteCliAck() {};
		bool serialize(std::string &);
		bool deserialize(const char *, uint32_t);
		const uint64_t get_type();
	private:
		const uint64_t type;
		std::string node_id;
		boost::shared_ptr<ErrorStatus> err;
};

class NodeRejoinAfterRecovery : public Message
{
	public:
		NodeRejoinAfterRecovery();
		NodeRejoinAfterRecovery(std::string node_id, std::string node_ip, int32_t node_port);
		std::string get_node_id() const;
		std::string get_node_ip() const;
		int32_t get_node_port() const;
		const uint64_t get_type();
		bool serialize(std::string &);
		bool deserialize(const char *, uint32_t);
	private:
		const uint64_t type;
		std::string node_id;
		std::string node_ip;
		int32_t node_port;
};

class NodeRejoinAfterRecoveryAck : public Message
{
	public:
		NodeRejoinAfterRecoveryAck();
		NodeRejoinAfterRecoveryAck(std::string node_id, boost::shared_ptr<ErrorStatus> err);
		const uint64_t get_type();
		bool serialize(std::string &);
		bool deserialize(const char *, uint32_t);
		boost::shared_ptr<ErrorStatus> get_error_status() const;
		std::string get_node_id() const;
	private:
		const uint64_t type;
		std::string node_id;
		boost::shared_ptr<ErrorStatus> error_status;
};

class NodeStopLL : public Message
{
	public:
		NodeStopLL();
		NodeStopLL(boost::shared_ptr<ServiceObject> node);
		boost::shared_ptr<ServiceObject> get_node_info() const;
		const uint64_t get_type();
		bool serialize(std::string &);
		bool deserialize(const char *, uint32_t);
	private:
		const uint64_t type;
		boost::shared_ptr<ServiceObject> node_info;
};

class NodeStopLLAck : public Message
{
	public:
		NodeStopLLAck();
		NodeStopLLAck(std::string node_id, boost::shared_ptr<ErrorStatus> err, int node_status);
		const uint64_t get_type();
		bool serialize(std::string &);
		bool deserialize(const char *, uint32_t);
		boost::shared_ptr<ErrorStatus> get_error_status() const;
		std::string get_node_id() const;
		int get_node_status() const;
	private:
		const uint64_t type;
		std::string node_id;
		boost::shared_ptr<ErrorStatus> error_status;
		int node_status;
};

class GetOsdClusterStatus : public Message
{
	public:
		GetOsdClusterStatus();
		std::string get_node_id() const;
		const uint64_t get_type();
		bool serialize(std::string &);
		bool deserialize(const char *, uint32_t);
	private:
		const uint64_t type;
		std::string node_id;
};

class GetOsdClusterStatusAck : public Message
{
	public:
		GetOsdClusterStatusAck();
		GetOsdClusterStatusAck(
			std::map<std::string, network_messages::NodeStatusEnum> node_to_status_map,
			boost::shared_ptr<ErrorStatus> error_status
		);
		const uint64_t get_type();
		bool serialize(std::string &);
		bool deserialize(const char *, uint32_t);
		boost::shared_ptr<ErrorStatus> get_error_status() const;
	private:
		const uint64_t type;
		std::map<std::string, network_messages::NodeStatusEnum> node_to_status_map;
		boost::shared_ptr<ErrorStatus> error_status;
};

class UpdateContainer : public Message
{
	public:
		UpdateContainer();
		UpdateContainer(
			std::string,
			std::string
		);
		const uint64_t get_type();
		bool serialize(std::string &);
		bool deserialize(const char *, uint32_t);
		std::string get_meta_file_path();
		std::string get_operation();

	private:
		const uint64_t type;
		std::string meta_file_path;
		std::string operation;
};

class ReleaseTransactionLock: public Message
{
	public:
		ReleaseTransactionLock();
		ReleaseTransactionLock(
			std::string,
			std::string
		);
		const uint64_t get_type();
		bool serialize(std::string &);
		bool deserialize(const char *, uint32_t);
		std::string get_operation();
		std::string get_object_hash();

	private:
		const uint64_t type;
		std::string obj_hash;
		std::string operation;
};

class StatusAck: public Message
{
	public:
		StatusAck();
		const uint64_t get_type();
		bool serialize(std::string &);
		bool deserialize(const char *, uint32_t);
		bool get_result();
		void set_result(bool);

	private:
		const uint64_t type;
		bool status;
};

class InitiateClusterRecovery : public Message
{
	public:
		InitiateClusterRecovery();
		~InitiateClusterRecovery() {}
		const uint64_t get_type();
		bool serialize(std::string &);
		bool deserialize(const char *, uint32_t);
	private:
		const uint64_t type;
};

}// namespace comm_messages

#endif
