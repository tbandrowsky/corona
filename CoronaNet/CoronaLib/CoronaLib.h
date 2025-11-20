#pragma once

#include "corona.hpp"
#include <msclr\marshal_cppstd.h>

using namespace System;
using namespace Newtonsoft::Json::Linq;

namespace CoronaLib {

	public ref class BaseResponse : public CoronaInterface::ICoronaBaseResponse 
	{
	
		bool success;
		String^ message;
		JArray^ data_array;
		JObject^ data_object;
        double executionTimeSeconds;

	public:
		BaseResponse()
			: success(false),
			message(nullptr),
			executionTimeSeconds(0.0),
			data_object(nullptr),
			data_array(nullptr)
		{
		}

		virtual property bool Success		
		{
			bool get() {
				return success;
			}
			void set(bool value) {
				success = value;
			}
		}

		virtual property String^ Message
		{
			String ^ get() {
				return message;
			}
			void set(String ^ value) {
				message = value;
			}
		}

		virtual property double ExecutionTimeSeconds		
		{
			double get() {
				return ExecutionTimeSeconds;
			}
			void set(double value) {
				ExecutionTimeSeconds = value;
			}
		}

		virtual property JToken^ Data
		{
			JToken ^get() {
                if (data_array)
					return data_array;
				else if (data_object)
					return data_object;
				else
					return nullptr;
			}
			void set(JToken^ value) {

                if (value->Type == JTokenType::Array)
					data_array = (JArray^)value;
				else if (value->Type == JTokenType::Object)
					data_object = (JObject^)value;
			}
		}
	};
	public ref class LoginResult : public BaseResponse {
	
	};
	public ref class CreateUserResponse : public BaseResponse {

	};
	public ref class SetSendCodeResponse : public BaseResponse {

	};
	public ref class ConfirmUserCodeResponse : public BaseResponse {
	
	};
	public ref class GetClassResponse : public BaseResponse {

	};
	public ref class PutClassResponse : public BaseResponse	 {

	};
	public ref class GetObjectResponse : public BaseResponse {

	};
	public ref class EditObjectResponse : public BaseResponse {

	};
	public ref class RunObjectResponse : public BaseResponse {

	};
	public ref class PutObjectsResponse : public BaseResponse {

	};
	public ref class DeleteObjectsResponse : public BaseResponse {

	};
	public ref class QueryClassResponse : public BaseResponse {

	};
	public ref class QueryResponse : public BaseResponse {
	
	};

	public ref class CoronaDatabase : public CoronaInterface::ICoronaDatabase
	{

    private:
        corona::corona_database *m_database;
		bool put_response(CoronaInterface::ICoronaBaseResponse^ baseResponse, corona::json result);

	public:

		!CoronaDatabase() // finalizer
		{
			delete m_database;
			m_database = nullptr;
		}

	protected:

		~CoronaDatabase() // destructor calls finalizer
		{
			this->!CoronaDatabase();
		}

	public:
		virtual bool CreateDatabase(CoronaInterface::IDatabaseConfiguration^ configuration);
		virtual bool OpenDatabase(CoronaInterface::IDatabaseConfiguration^ configuration);
		virtual CoronaInterface::ILoginResult^ LoginLocal(System::String^ username, System::String^ password);
		virtual CoronaInterface::ILoginResult^ LoginUser(System::String^ username, System::String^ password);
		virtual CoronaInterface::ILoginResult^ LoginUserSso(System::String^ username, System::String^ password);
		virtual CoronaInterface::ICreateUserResponse^ CreateUser(CoronaInterface::ICreateUserRequest^ request);
		virtual CoronaInterface::ISetSendCodeResponse^ SetSendCode(CoronaInterface::ISetSendCodeRequest^ request);
		virtual CoronaInterface::IConfirmUserCodeResponse^ ConfirmUserCode(CoronaInterface::IConfirmUserCodeRequest^ request);
		virtual CoronaInterface::IGetClassResponse^ GetClass(CoronaInterface::IGetClassRequest^ request);
		virtual CoronaInterface::IPutClassResponse^ PutClass(CoronaInterface::IPutClassRequest^ request);
		virtual CoronaInterface::IGetObjectResponse^ GetObject(CoronaInterface::IGetObjectRequest^ request);
		virtual CoronaInterface::IEditObjectResponse^ EditObject(CoronaInterface::IEditObjectRequest^ request);
		virtual CoronaInterface::IRunObjectResponse^ EditObject(CoronaInterface::IRunObjectRequest^ request);
		virtual CoronaInterface::IPutObjectsResponse^ PutObjects(CoronaInterface::IPutObjectsRequest^ request);
		virtual CoronaInterface::IDeleteObjectsResponse^ DeleteObjects(CoronaInterface::IDeleteObjectsRequest^ request);
		virtual CoronaInterface::IQueryClassResponse^ QueryClass(CoronaInterface::IQueryClassRequest^ request);
		virtual CoronaInterface::IQueryResponse^ Query(CoronaInterface::IQueryRequest^ request);
	};
}
