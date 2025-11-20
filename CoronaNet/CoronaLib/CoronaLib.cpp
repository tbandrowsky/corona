#include "corona.hpp"
#include "CoronaLib.h"
#include <msclr\marshal_cppstd.h>
#include <Newtonsoft.Json.h>

using namespace Newtonsoft::Json;

bool CoronaLib::CoronaDatabase::put_response(CoronaInterface::ICoronaBaseResponse^ baseResponse, corona::json result)
{
    baseResponse->Success = (bool)result["success"];
    baseResponse->Message = msclr::interop::marshal_as<System::String^>(result["message"]);
    baseResponse->ExecutionTimeSeconds = (double)result["execution_time"];

    if (result.has_member("data")) {
        corona::json data = result["data"];
        if (data.object()) {
            baseResponse->Data = JObject::Parse(msclr::interop::marshal_as<System::String^>(data.to_json()));
        }
        else if (data.array()) {
            baseResponse->Data = JArray::Parse(msclr::interop::marshal_as<System::String^>(data.to_json()));
        }
    }
}

bool CoronaLib::CoronaDatabase::CreateDatabase(CoronaInterface::IDatabaseConfiguration^ configuration)
{
    corona::json_parser jp;
    String^ config_string_managed = configuration != nullptr ? JsonConvert::SerializeObject(configuration) : "";
    std::string config_string = configuration != nullptr ? msclr::interop::marshal_as<std::string>(config_string_managed) : "";

    m_database = new corona::corona_database();


    m_database->create_database();
    return m_database != nullptr;
}

bool CoronaLib::CoronaDatabase::OpenDatabase(CoronaInterface::IDatabaseConfiguration^ configuration)
{
    m_database = new corona::corona_database();
}

CoronaInterface::ILoginResult^ CoronaLib::CoronaDatabase::LoginLocal(System::String^ username, System::String^ password)
{

    if (!m_database) {
        throw gcnew System::InvalidOperationException("Database not open");
    }
    // TODO: insert return statement here
}

CoronaInterface::ILoginResult^ CoronaLib::CoronaDatabase::LoginUser(System::String^ username, System::String^ password)
{
    if (!m_database) {
        throw gcnew System::InvalidOperationException("Database not open");
    }
    // TODO: insert return statement here
}

CoronaInterface::ILoginResult^ CoronaLib::CoronaDatabase::LoginUserSso(System::String^ username, System::String^ password)
{
    if (!m_database) {
        throw gcnew System::InvalidOperationException("Database not open");
    }
    // TODO: insert return statement here
}

CoronaInterface::ICreateUserResponse^ CoronaLib::CoronaDatabase::CreateUser(CoronaInterface::ICreateUserRequest^ request)
{
    if (!m_database) {
        throw gcnew System::InvalidOperationException("Database not open");
    }
    // TODO: insert return statement here
}

CoronaInterface::ISetSendCodeResponse^ CoronaLib::CoronaDatabase::SetSendCode(CoronaInterface::ISetSendCodeRequest^ request)
{
    if (!m_database) {
        throw gcnew System::InvalidOperationException("Database not open");
    }
    // TODO: insert return statement here
}

CoronaInterface::IConfirmUserCodeResponse^ CoronaLib::CoronaDatabase::ConfirmUserCode(CoronaInterface::IConfirmUserCodeRequest^ request)
{
    if (!m_database) {
        throw gcnew System::InvalidOperationException("Database not open");
    }
    // TODO: insert return statement here
}

CoronaInterface::IGetClassResponse^ CoronaLib::CoronaDatabase::GetClass(CoronaInterface::IGetClassRequest^ request)
{
    if (!m_database) {
        throw gcnew System::InvalidOperationException("Database not open");
    }
    // TODO: insert return statement here
}

CoronaInterface::IPutClassResponse^ CoronaLib::CoronaDatabase::PutClass(CoronaInterface::IPutClassRequest^ request)
{
    if (!m_database) {
        throw gcnew System::InvalidOperationException("Database not open");
    }
    // TODO: insert return statement here
}

CoronaInterface::IGetObjectResponse^ CoronaLib::CoronaDatabase::GetObject(CoronaInterface::IGetObjectRequest^ request)
{
    if (!m_database) {
        throw gcnew System::InvalidOperationException("Database not open");
    }
    // TODO: insert return statement here
}

CoronaInterface::IEditObjectResponse^ CoronaLib::CoronaDatabase::EditObject(CoronaInterface::IEditObjectRequest^ request)
{
    if (!m_database) {
        throw gcnew System::InvalidOperationException("Database not open");
    }
    // TODO: insert return statement here
}

CoronaInterface::IRunObjectResponse^ CoronaLib::CoronaDatabase::EditObject(CoronaInterface::IRunObjectRequest^ request)
{
    if (!m_database) {
        throw gcnew System::InvalidOperationException("Database not open");
    }
    // TODO: insert return statement here
}

CoronaInterface::IPutObjectsResponse^ CoronaLib::CoronaDatabase::PutObjects(CoronaInterface::IPutObjectsRequest^ request)
{
    if (!m_database) {
        throw gcnew System::InvalidOperationException("Database not open");
    }
    // TODO: insert return statement here
}

CoronaInterface::IDeleteObjectsResponse^ CoronaLib::CoronaDatabase::DeleteObjects(CoronaInterface::IDeleteObjectsRequest^ request)
{
    if (!m_database) {
        throw gcnew System::InvalidOperationException("Database not open");
    }
    // TODO: insert return statement here
}

CoronaInterface::IQueryClassResponse^ CoronaLib::CoronaDatabase::QueryClass(CoronaInterface::IQueryClassRequest^ request)
{
    if (!m_database) {
        throw gcnew System::InvalidOperationException("Database not open");
    }
    // TODO: insert return statement here
}

CoronaInterface::IQueryResponse^ CoronaLib::CoronaDatabase::Query(CoronaInterface::IQueryRequest^ request)
{
    if (!m_database) {
        throw gcnew System::InvalidOperationException("Database not open");
    }
    // TODO: insert return statement here
}
