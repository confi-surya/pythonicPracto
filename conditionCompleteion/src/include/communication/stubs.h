#ifndef __STUBS_H__
#define __STUBS_H__


#include <iostream>
#include <list>

class ServiceObject{
	public:
		ServiceObject();
		~ServiceObject();

		void set_service_id(std::string);
		void set_ip(std::string);
		void set_port(uint64_t);

		std::string get_service_id();
		std::string get_ip();
		uint64_t get_port();

	private:
		std::string service_id;
		std::string ip;
		uint64_t port;
};

class ErrorStatus{
        public:
                ErrorStatus();
                ErrorStatus(int32_t code, std::string msg);
		~ErrorStatus();

                void set_code(int32_t code);
                void set_msg(std::string msg);

                int32_t get_code();
                std::string get_msg();

        private:
                int32_t code;
                std::string msg;

};

class LocalLeaderNodeStatus{
	public:
		typedef 
		enum LL_NODE_STATUS
		{
			RUNNING_SERVICES = 10,
			STOPPING_SERVICES = 20,
			STOPPED_SERVICES = 30
		} LL_NODE_STATUS ;
		LocalLeaderNodeStatus();
		LocalLeaderNodeStatus(LL_NODE_STATUS );
		~LocalLeaderNodeStatus();
		void set_local_node_status(LL_NODE_STATUS );
		LL_NODE_STATUS get_local_node_status();
	private: 
		LL_NODE_STATUS local_node_status;
};

class GlobalNodeStatus{
	public:
		typedef
                enum GL_NODE_STATUS
                {
                        RUNNING_SERVICES = 10,
                        STOPPING_SERVICES = 20,
                        STOPPED_SERVICES = 30,
			FAILED_SERVICES = 40,
			RECOVERED_SERVICES = 50,
			RETIRING_SERVICES = 60,
			RETIRED_SERVICES = 70
                } GL_NODE_STATUS ;
		GlobalNodeStatus();
		GlobalNodeStatus(GL_NODE_STATUS );
		~GlobalNodeStatus();
                void set_global_node_status(GL_NODE_STATUS );
                GL_NODE_STATUS get_global_node_status();
        private:
                GL_NODE_STATUS global_node_status;
};
		
		
#endif
