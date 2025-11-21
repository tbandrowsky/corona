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
		String^ token;
		JArray^ data_array;
		JObject^ data_object;
        double executionTimeSeconds;

	public:
		BaseResponse()
			: success(false),
			message(nullptr),
			executionTimeSeconds(0.0),
			data_object(nullptr),
			data_array(nullptr),
			token(nullptr)
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

		virtual property String^ Token
		{
			String^ get() {
				return token;
			}
			void set(String^ value) {
				token = value;
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

	public ref class LoginResult : public BaseResponse, public CoronaInterface::ILoginResult {
	
	};
	public ref class CreateUserResponse : public BaseResponse, public CoronaInterface::ICreateUserResponse {

	};
	public ref class SetSendCodeResponse : public BaseResponse, public CoronaInterface::ISetSendCodeResponse {

	};
	public ref class ConfirmUserCodeResponse : public BaseResponse, public CoronaInterface::IConfirmUserCodeResponse {
	
	};
	public ref class GetClassResponse : public BaseResponse, public CoronaInterface::IGetClassResponse {

	};
	public ref class PutClassResponse : public BaseResponse, public CoronaInterface::IPutClassResponse {

	};
	public ref class GetObjectResponse : public BaseResponse, public CoronaInterface::IGetObjectResponse {

	};
	public ref class EditObjectResponse : public BaseResponse, public CoronaInterface::IEditObjectResponse {

	};
	public ref class RunObjectResponse : public BaseResponse, public CoronaInterface::IRunObjectResponse {

	};
	public ref class PutObjectsResponse : public BaseResponse, public CoronaInterface::IPutObjectsResponse {

	};
	public ref class DeleteObjectsResponse : public BaseResponse, public CoronaInterface::IDeleteObjectsResponse {

	};
	public ref class QueryClassResponse : public BaseResponse, public CoronaInterface::IQueryClassResponse {

	};
	public ref class QueryResponse : public BaseResponse, public CoronaInterface::IQueryResponse {

	};

	public ref class CoronaDatabase : public CoronaInterface::ICoronaDatabase
	{

    private:

        corona::corona_database *m_database;
		String^ token;

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
		virtual CoronaInterface::ILoginResult^ LoginLocal(System::String^ username, System::String^ email);
		virtual CoronaInterface::ILoginResult^ LoginUser(System::String^ username, System::String^ password);
		virtual CoronaInterface::ILoginResult^ LoginUserSso(System::String^ username, System::String^ email, System::String^ code);
		virtual CoronaInterface::ICreateUserResponse^ CreateUser(CoronaInterface::ICreateUserRequest^ request);
		virtual CoronaInterface::ISetSendCodeResponse^ SetSendCode(CoronaInterface::ISetSendCodeRequest^ request);
		virtual CoronaInterface::IConfirmUserCodeResponse^ ConfirmUserCode(CoronaInterface::IConfirmUserCodeRequest^ request);
		virtual CoronaInterface::IGetClassResponse^ GetClass(CoronaInterface::IGetClassRequest^ request);
		virtual CoronaInterface::IPutClassResponse^ PutClass(CoronaInterface::IPutClassRequest^ request);
		virtual CoronaInterface::IGetObjectResponse^ GetObject(CoronaInterface::IGetObjectRequest^ request);
		virtual CoronaInterface::IEditObjectResponse^ EditObject(CoronaInterface::IEditObjectRequest^ request);
		virtual CoronaInterface::IRunObjectResponse^ RunObject(CoronaInterface::IRunObjectRequest^ request);
		virtual CoronaInterface::ILoginResult^ UserSetTeam(CoronaInterface::IUserSetTeamRequest^ request);
		virtual CoronaInterface::IPutObjectsResponse^ PutObjects(CoronaInterface::IPutObjectsRequest^ request);
		virtual CoronaInterface::IDeleteObjectsResponse^ DeleteObjects(CoronaInterface::IDeleteObjectsRequest^ request);
		virtual CoronaInterface::IQueryClassResponse^ QueryClass(CoronaInterface::IQueryClassRequest^ request);
		virtual CoronaInterface::IQueryResponse^ Query(CoronaInterface::IQueryRequest^ request);
	};
}
