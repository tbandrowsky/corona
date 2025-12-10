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

		CoronaInterface::SysUser^ m_user;
	public:

		virtual property CoronaInterface::SysUser^ User {
			CoronaInterface::SysUser^ get() {
				return m_user;
			}
			void set(CoronaInterface::SysUser^ value) {
				m_user = value;
			}
		}

	};

	public ref class CreateUserResponse : public BaseResponse, public CoronaInterface::ICreateUserResponse {

	};

	public ref class SetSendCodeResponse : public BaseResponse, public CoronaInterface::ISetSendCodeResponse {

	};

	public ref class ConfirmUserCodeResponse : public BaseResponse, public CoronaInterface::IConfirmUserCodeResponse {
		CoronaInterface::SysUser^ m_user;
	public:

		virtual property CoronaInterface::SysUser^ User {
			CoronaInterface::SysUser^ get() {
				return m_user;
			}
			void set(CoronaInterface::SysUser^ value) {
				m_user = value;
			}
		}
	};

	public ref class GetClassResponse : public BaseResponse, public CoronaInterface::IGetClassResponse {
		CoronaInterface::CoronaClass^ m_class;
	public:

		virtual property CoronaInterface::CoronaClass^ CoronaClass {
			CoronaInterface::CoronaClass^ get() {
				return m_class;
			}
			void set(CoronaInterface::CoronaClass^ value) {
				m_class = value;
			}
		}
	};

	public ref class PutClassResponse : public BaseResponse, public CoronaInterface::IPutClassResponse {
		CoronaInterface::CoronaClass^ m_class;
	public:

		virtual property CoronaInterface::CoronaClass^ CoronaClass {
			CoronaInterface::CoronaClass^ get() {
				return m_class;
			}
			void set(CoronaInterface::CoronaClass^ value) {
				m_class = value;
			}
		}

	};

	public ref class GetObjectResponse : public BaseResponse, public CoronaInterface::IGetObjectResponse {

	};

	public ref class EditObjectResponse : public BaseResponse, public CoronaInterface::IEditObjectResponse {

		CoronaInterface::SysObject^ m_sysObject;
		System::Collections::Generic::Dictionary<String^, CoronaInterface::CoronaClass^>^ m_classes;
		JObject^ m_childClasses;

		CoronaInterface::SysUser^ m_class;
	public:

		virtual property CoronaInterface::SysObject^ SysObject {
			CoronaInterface::SysObject^ get() {
				return m_sysObject;
			}
			void set(CoronaInterface::SysObject^ value) {
				m_sysObject = value;
			}
		}

		virtual property System::Collections::Generic::Dictionary<String^, CoronaInterface::CoronaClass^>^ Classes {
			System::Collections::Generic::Dictionary<String^, CoronaInterface::CoronaClass^>^ get() {
				return m_classes;
			}
			void set(System::Collections::Generic::Dictionary<String^, CoronaInterface::CoronaClass^>^ value) {
				m_classes = value;
			}
		}

		virtual property JObject^ ChildClasses {
			JObject^ get() {
				return m_childClasses;
			}
			void set(JObject^ value) {
				m_childClasses = value;
			}
		}
	};

	public ref class RunObjectResponse : public BaseResponse, public CoronaInterface::IRunObjectResponse {

		CoronaInterface::SysObject^ m_sysObject;
		System::Collections::Generic::Dictionary<String^, CoronaInterface::CoronaClass^>^ m_classes;
		JObject^ m_childClasses;

		CoronaInterface::SysUser^ m_class;

	public:

		virtual property CoronaInterface::SysObject^ SysObject {
			CoronaInterface::SysObject^ get() {
				return m_sysObject;
			}
			void set(CoronaInterface::SysObject^ value) {
				m_sysObject = value;
			}
		}

		virtual property System::Collections::Generic::Dictionary<String^, CoronaInterface::CoronaClass^>^ Classes {
			System::Collections::Generic::Dictionary<String^, CoronaInterface::CoronaClass^>^ get() {
				return m_classes;
			}
			void set(System::Collections::Generic::Dictionary<String^, CoronaInterface::CoronaClass^>^ value) {
				m_classes = value;
			}
		}

		virtual property JObject^ ChildClasses {
			JObject^ get() {
				return m_childClasses;
			}
			void set(JObject^ value) {
				m_childClasses = value;
			}
		}

	};
	public ref class PutObjectsResponse : public BaseResponse, public CoronaInterface::IPutObjectsResponse {

	};
	public ref class DeleteObjectsResponse : public BaseResponse, public CoronaInterface::IDeleteObjectsResponse {

	};
	public ref class QueryClassResponse : public BaseResponse, public CoronaInterface::IQueryClassResponse {

	};
	public ref class QueryResponse : public BaseResponse, public CoronaInterface::IQueryResponse {

	};

	public ref class CoronaSystem : public CoronaInterface::ICoronaSystem
	{
	public:
		CoronaSystem()
		{
			corona::system_monitoring_interface::start(); // this will create the global log queue.
			corona::system_monitoring_interface::active_mon = corona::system_monitoring_interface::global_mon;
			corona::init_xtables();
		}

		virtual property CoronaInterface::ISystemMonitoring^ SystemMonitoring {
			CoronaInterface::ISystemMonitoring^ get() {
				return corona::system_monitoring_interface::active_mon->net_reporting.get();
			}
			void set(CoronaInterface::ISystemMonitoring^ value) {
				corona::system_monitoring_interface::active_mon->net_reporting=value;
			}
		}


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

		CoronaDatabase()
		{
			corona::system_monitoring_interface::start();
			if (!corona::global_job_queue) {
				corona::global_job_queue = std::make_unique<corona::job_queue>();
				corona::global_job_queue->start(0);
			}
		}

	protected:

		~CoronaDatabase() // destructor calls finalizer
		{
			this->!CoronaDatabase();
		}

	public:
		virtual bool CreateDatabase(CoronaInterface::DatabaseConfiguration^ configuration);
		virtual bool OpenDatabase(CoronaInterface::DatabaseConfiguration^ configuration);
        virtual void ApplySchema(System::String^ schema_file_name);
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
