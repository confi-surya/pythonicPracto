template <typename T>
class MessageResult{
	public:
		MessageResult(T *, bool, uint64_t, std::string);
		MessageResult();
		~MessageResult();

		T * get_message_object();
		bool get_status();
		void set_status(bool);
		uint64_t get_return_code();
		std::string get_msg();
		void set_message_object(T *);
		void set_return_code(uint64_t);
		void set_msg(std::string);

	private:
		T * msg;
		bool status;
		uint64_t ret_code;
		std::string error_msg;
};


template <typename T>
MessageResult<T>::MessageResult(
	T * msg,
	bool status,
	uint64_t ret_code,
	std::string error_msg
):
	msg(msg),
	status(status),
	ret_code(ret_code),
	error_msg(error_msg)
{
}

template <typename T>
MessageResult<T>::MessageResult()
{
}

template <typename T>
MessageResult<T>::~MessageResult()
{
}

template <typename T>
T * MessageResult<T>::get_message_object()
{
	return this->msg;
}

template <typename T>
bool MessageResult<T>::get_status()
{
	return this->status;
}

template <typename T>
void MessageResult<T>::set_status(bool status)
{
	this->status = status;
}
template <typename T>
uint64_t MessageResult<T>::get_return_code()
{
	return this->ret_code;
}

template <typename T>
std::string MessageResult<T>::get_msg()
{
	return this->error_msg;
}

template <typename T>
void MessageResult<T>::set_msg(std::string msg)
{
	this->error_msg = msg;
}

template <typename T>
void MessageResult<T>::set_message_object(T * ob)
{
	this->msg = ob;
}

template <typename T>
void MessageResult<T>::set_return_code(uint64_t ret)
{
	this->ret_code = ret; 
}
