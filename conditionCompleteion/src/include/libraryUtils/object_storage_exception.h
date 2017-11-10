/*
 * $Id$
 *
 * object_storage_exception.h, Declaration of exceptions used for objectStorage Exception
 *
 * 
 *
 */

#ifndef __OSD_EXCEPTION_H__
#define __OSD_EXCEPTION_H__

#include "object_storage_log_setup.h"
#include <cool/types.h>
#include <cool/exceptions.h>

using namespace cool;

namespace OSDNodeStat
{
class NodeStatStatus
{
public:
	enum node_stat
	{
		NODE_NEW = 1,
		NODE_FAILED = 50,
		NODE_RECOVERED = 60 
	};
};
} //namespace of OSDNodeStat

namespace OSDException	 
{

#define OSD_EXCEPTION_THROW(cl, desc) \
	THROW(cl, desc) \

#define OSD_EXCEPTION_THROW_IF(cond, cl, desc)\
{\
	if (cond)\
	{\
		OSD_EXCEPTION_THROW(cl, desc);\
	}\
}

class OSDConnStatus
{
public:
	enum connectivity_stat
	{
		CONNECTED = 1,
		DISCONNECTED = 2
	};
};

class OSDHfsStatus
{
public:
	enum gfs_stat
	{
		READABILITY = 1,
		NOT_READABILITY = 2,
		WRITABILITY = 3,
		NOT_WRITABILITY = 4
	};	
};


class OSDServiceStatus
{
public:
	enum service_status
	{
		MTR = 1,
		MTF = 2,
		RUN = 3,	
		REG = 4,
		NDFO = 5,
		UREG = 6
	};

};


class OSDMsgTypeInMap
{
public:
	enum msg_type_in_map
	{
		STRM = 1,
		HRBT = 2,
		ENDM = 3
	};
};

class OSDErrorCode
{
public:
	typedef 
	enum error_code
	{
		INFO_FILE_MOUNT_CHECK_FAILED = 10,  // not using now
		LL_IS_NOT_REGISTERED = 11,	
		INFO_FILE_NOT_FOUND = 20,
		INFO_FILE_ALREADY_EXIST = 30,
		INFO_FILE_OPERATION_ERROR = 40,
		INFO_FILE_INTERNAL_ERROR = 50,
		INFO_FILE_DELETED = 55,
		CMD_ERR_BAD_CONFIG_ERROR = 60, // not using now
		INFO_FILE_MAX_META_COUNT_REACHED = 70,
		INFO_FILE_MAX_META_SIZE_REACHED = 71,
		INFO_FILE_MAX_ACL_SIZE_REACHED = 73,
		INFO_FILE_MAX_SYSTEM_META_SIZE_REACHED = 75,
		BULK_DELETE_TRANS_FAILED = 76,
                BULK_DELETE_CONTAINER_FAILED = 77,
                BULK_DELETE_TRANS_CONFLICT = 78,
		TRANSACTION_LOCK_DENIED = 80,
		TRANSACTION_RELEASE_FAILED = 81,
		ACCEPT_TRANS_DATA_FAILED = 82,
		COMMIT_TRANS_DATA_FAILED = 83,
		COMMIT_CONT_DATA_FAILED = 90,
		ACCEPT_CONT_DATA_FAILED = 91,
		FLUSH_ALL_DATA_ERROR = 92,
		START_FAILED = 93,
		TIMEOUT_EXIT_CODE  = 94,
		UNABLE_RECV_ACK_FROM_GL = 95,
		NODE_ADDITION_FAILED = 96,
		OSD_START_MONITORING_FAILED = 97,
		NODE_STOP_CLI_FAILED = 98,
		STOP_SERVICE_FAILED = 100,
		NODE_STOP_CLI_RETIRING_FAILED = 101
	} error_code;

	static inline string const get_error_string(OSDErrorCode::error_code error_code)
	{
		cool::oss ss;
		ss << error_code;
		return "OSD_CORE_LIB_ERR_CODE:" + ss.str() + " , ";
	}
};

class OSDSuccessCode
{
public:
	typedef 
	enum success_code
	{
		ELECTION_FAILURE = 90,
		RE_ELECTION_NOT_REQUIRED = 91,
		ELECTION_SUCCESS = 98,
		RECV_ACK_FROM_GL = 99,
		INFO_FILE_OPERATION_SUCCESS = 100,
		INFO_FILE_ACCEPTED = 200,	// use in container library in container-overwrite case
		BULK_DELETE_TRANS_ADDED = 201,
                BULK_DELETE_CONTAINER_SUCCESS = 202,
		TRANSACTION_LOCK_GRANTED = 300,
		TRANSACTION_LOCK_RELEASED = 301,
		ACCEPT_TRANS_DATA_SUCCESS = 302,
		COMMIT_TRANS_DATA_SUCCESS = 303,
		ACCEPT_CONT_DATA_SUCCESS = 400,
		COMMIT_CONT_DATA_SUCCESS = 401,
		FLUSH_ALL_DATA_SUCCESS = 402,
		START_SUCCESS = 403,
		NODE_ADDITION_SUCCESS = 97,
		OSD_START_MONITORING_SUCCESS = 96,
		NODE_STOP_CLI_SUCCESS = 95,
		STOP_SERVICE_SUCCESS = 94	
	} success_code;

	static inline string const get_success_string(OSDSuccessCode::success_code success_code)
	{
		cool::oss ss;
		ss << success_code;
		return "OSD_CORE_LIB_SUCCESS_CODE:" + ss.str() + " , "; 
	}

};

class OSDCoreLibException : public cool::Exception
{
public:
	string getMessage() const
	{
		return " Object Storage Core Library Error: " + this->getDescription();		
	}
};

class OSDCoreLibBadConfigException : public cool::Exception
{
public:
	// overide the getMessage function from cool::Exception
	string getMessage() const
	{
		std::stringstream ss;
		ss << OSDErrorCode::CMD_ERR_BAD_CONFIG_ERROR;
		return "Runtime Error: OSD_CORE_LIB_ERR_CODE:" + ss.str() + " , " + this->getDescription();
	}
};

class MetaLimitException : public cool::Exception
{
public:
        // overide the getMessage function from cool::Exception
        string getMessage() const
        {
		return "Meta Limit check limit failed.";
        }
};

class MetaSizeException : public MetaLimitException
{
public:
        // overide the getMessage function from cool::Exception
        string getMessage() const
        {
                return "Meta Data size check failed.";
        }
};

class MetaCountException : public MetaLimitException
{
public:
        // overide the getMessage function from cool::Exception
        string getMessage() const
        {
                return "Meta Data count check failed.";
        }
};

class ACLMetaSizeException : public MetaLimitException
{
public:
        // overide the getMessage function from cool::Exception
        string getMessage() const
        {
                return "ACL meta data size check failed.";
        }
};

class SystemMetaSizeException : public MetaLimitException
{
public:
        // overide the getMessage function from cool::Exception
        string getMessage() const
        {
                return "System meta data size check failed.";
        }
};

class FileException: public cool::Exception
{
        std::string getMessage() const
        {
                return "File exception" + this->getDescription();
        }
};

class ReadFileException: public FileException
{
        std::string getMessage() const
        {
                return "Read file exception" + this->getDescription();
        }
};

class WriteFileException: public FileException
{
        std::string getMessage() const
        {
                return "Write file exception" + this->getDescription();
        }
};

class CreateFileException: public FileException
{
        std::string getMessage() const
        {
                return "Create file exception" + this->getDescription();
        }
};

class RenameFileException : public FileException
{
	std::string getMessage() const
	{
		return "Rename file exception" + this->getDescription();
	}

};

class FileExistException: public FileException
{
	std::string getMessage() const
	{
		return "file already exist exception" + this->getDescription();
	}
};

class OpenFileException: public FileException
{
        std::string getMessage() const
        {
                return "file open exception" + this->getDescription();
        }
};

class CloseFileException: public FileException
{
	std::string getMessage() const
	{
		return "file close exception" + this->getDescription();
	}
};

class SeekFileException: public FileException
{
        std::string getMessage() const
        {
                return "file seek exception" + this->getDescription();
        }
};



class SyncFileException: public FileException
{
	std::string getMessage() const
        {
                return "file sync exception" + this->getDescription();
        }
};

class FileNotFoundException: public FileException
{
	std::string getMessage() const
	{
		return "file not found exception" + this->getDescription();
	}
};


class FileMountException: public FileException
{
        std::string getMessage() const
        {
                return "file mount exception" + this->getDescription();
        }
};

class ThreadException: public cool::Exception
{
        std::string getMessage() const
        {
                return "Thread exception" + this->getDescription();
        }
};

class MaxThreadLimitReached: public ThreadException
{
        std::string getMessage() const
        {
                return "Maximum thread limit reached exception" + this->getDescription();
        }
};

class UnstopableException: public ThreadException
{
        std::string getMessage() const
        {
                return "Stop called on unstopable thread exception" + this->getDescription();
        }
};

class Alram: public cool::Exception
{
	std::string getMessage() const
	{
		return "Alarm exception" + this->getDescription();
	}	
};

class IdNotFoundException: public cool::Exception
{
	std::string getMessage() const
	{
		return "Not found" + this->getDescription();
	}
};

} // namespace OSDException

#endif // __OSD_EXCEPTION_H__
