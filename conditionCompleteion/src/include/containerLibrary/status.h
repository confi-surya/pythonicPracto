#ifndef __STATUS_H__
#define __STATUS_H__

#include "libraryUtils/record_structure.h"
#include <list>

namespace StatusAndResult
{

class Status
{
private:
	uint16_t return_status;
public:
	Status(){};

	Status(uint16_t status)
	{
		this->return_status = status;
	}

	void set_return_status(uint16_t status)
	{
		this->return_status = status;
	}

	uint16_t get_return_status()
	{
		return this->return_status;
	}

        ~Status(){}
};

class ContainerStatWithStatus:public Status
{

public:
	recordStructure::ContainerStat container_stat;
	
	ContainerStatWithStatus(){}

	ContainerStatWithStatus(uint16_t status):Status(status){}

	ContainerStatWithStatus(uint16_t status, recordStructure::ContainerStat stat):Status(status)
	{
		this->container_stat = stat;
	} 	

//TODO asrivastava, now it is not required.
	recordStructure::ContainerStat get_container_stat()
	{
		return this->container_stat;
	}
	~ContainerStatWithStatus(){}
};
 
class ListObjectWithStatus:public Status
{
public:
	ListObjectWithStatus(){}
	~ListObjectWithStatus(){}

	std::list<recordStructure::ObjectRecord> object_record;
 
};


}
#endif

