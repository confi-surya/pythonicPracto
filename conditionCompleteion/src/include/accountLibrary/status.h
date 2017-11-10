#ifndef __STATUS_H__
#define __STATUS_H__

#include "accountLibrary/record_structure.h"
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

	uint16_t get_return_status(void)
	{
		return this->return_status;
	}
	void set_return_status(uint16_t status)
	{
		this->return_status = status;
	}

	~Status(){}
};

class AccountStatWithStatus:public Status
{
private:
	recordStructure::AccountStat account_stat;
public:
	AccountStatWithStatus(){}

	AccountStatWithStatus(uint16_t status) : Status(status)
	{
		;
	}
	
	AccountStatWithStatus(uint16_t status, recordStructure::AccountStat stat):Status(status)
	{
		this->account_stat = stat;
	}

	recordStructure::AccountStat get_account_stat(void)
	{
		return this->account_stat;
	}

	void set_account_stat(recordStructure::AccountStat stat)
	{
		account_stat = stat;
	}

	~AccountStatWithStatus(){}
};
 
class ListContainerWithStatus:public Status
{
private:
	std::list<recordStructure::ContainerRecord>container_record; 
public:
	ListContainerWithStatus(){}

	ListContainerWithStatus(uint16_t status):Status(status)
	{
		;
	}

	ListContainerWithStatus(uint16_t status,  std::list<recordStructure::ContainerRecord>record):Status(status)
	{
		this->container_record = record;
	}

	std::list<recordStructure::ContainerRecord> get_container_record(void)
	{
		return this->container_record;
	}
	
	void set_container_record(std::list<recordStructure::ContainerRecord>record)
	{
		container_record = record;
	}

	~ListContainerWithStatus(){}
 
};
}
#endif

