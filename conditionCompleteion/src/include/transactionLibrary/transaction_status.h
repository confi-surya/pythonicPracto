namespace TransactionResponse{

	class TransactionResponse{
		private:
			uint16_t status;
		public:
			TransactionResponse(){}
			TransactionResponse(uint16_t status){
				this->status = status;
			}

			uint16_t get_status(){
				return this->status; 
			}

			~TransactionResponse(){}
	}

	
}
