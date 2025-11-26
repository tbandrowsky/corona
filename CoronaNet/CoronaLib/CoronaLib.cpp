#include "corona.hpp"
#include "CoronaLib.h"
#include <msclr\marshal_cppstd.h>

using namespace Newtonsoft::Json;

bool put_response_base(CoronaInterface::ICoronaBaseResponse^ baseResponse, corona::json result)
{
    bool success = false;

    std::string msg = result["message"];

    baseResponse->Success = (bool)result["success"];
    baseResponse->Message = gcnew System::String(msg.c_str());
    baseResponse->ExecutionTimeSeconds = (double)result["execution_time"];

    if (result.has_member("data")) {
        corona::json data = result["data"];
        std::string data_string = data.to_json();
        if (data.object()) {
            baseResponse->Data = JObject::Parse(gcnew System::String(data_string.c_str()));
            success = true;
        }
        else if (data.array()) {
            baseResponse->Data = JArray::Parse(gcnew System::String(data_string.c_str()));
            success = true;
        }
    }

    return success;
}

bool put_response(CoronaInterface::ILoginResult^ baseResponse, corona::json result)
{
    bool success_base = put_response_base(baseResponse, result);

    if (baseResponse->Data) {
        baseResponse->User = JsonConvert::DeserializeObject<CoronaInterface::SysUser^>(baseResponse->Data->ToString());
    }

    return success_base;
}

bool put_response(CoronaInterface::ICreateUserResponse^ baseResponse, corona::json result)
{
    bool success_base = put_response_base(baseResponse, result);

    return success_base;
}

bool put_response(CoronaInterface::IGetClassResponse^ baseResponse, corona::json result)
{
    bool success_base = put_response_base(baseResponse, result);

    baseResponse->CoronaClass = JsonConvert::DeserializeObject<CoronaInterface::CoronaClass^>(baseResponse->Data->ToString());

    return success_base;
}

bool put_response(CoronaInterface::IPutClassResponse^ baseResponse, corona::json result)
{
    bool success_base = put_response_base(baseResponse, result);

    baseResponse->CoronaClass = JsonConvert::DeserializeObject<CoronaInterface::CoronaClass^>(baseResponse->Data->ToString());

    return success_base;
}

bool put_response(CoronaInterface::IDeleteObjectsResponse^ baseResponse, corona::json result)
{
    bool success_base = put_response_base(baseResponse, result);

    return success_base;
}

bool put_response(CoronaInterface::IConfirmUserCodeResponse^ baseResponse, corona::json result)
{
    bool success_base = put_response_base(baseResponse, result);

    if (baseResponse->Data) {
        baseResponse->User = JsonConvert::DeserializeObject<CoronaInterface::SysUser^>(baseResponse->Data->ToString());
    }

    return success_base;
}

bool put_response(CoronaInterface::IGetObjectResponse^ baseResponse, corona::json result)
{
    bool success_base = put_response_base(baseResponse, result);

    return success_base;
}

bool put_response(CoronaInterface::IPutObjectsResponse^ baseResponse, corona::json result)
{
    bool success_base = put_response_base(baseResponse, result);

    return success_base;
}

bool put_response(CoronaInterface::IQueryClassResponse^ baseResponse, corona::json result)
{
    bool success_base = put_response_base(baseResponse, result);

    return success_base;
}

bool put_response(CoronaInterface::IQueryResponse^ baseResponse, corona::json result)
{
    bool success_base = put_response_base(baseResponse, result);

    return success_base;
}

bool put_response(CoronaInterface::IEditObjectResponse^ baseResponse, corona::json result)
{
    bool success = put_response_base(baseResponse, result);

    if (baseResponse->Data) {
        baseResponse->SysObject = JsonConvert::DeserializeObject<CoronaInterface::SysObject^>(baseResponse->Data->ToString());
    }

    corona::json data = result["data"];

    corona::json object = data["object"];
    std::string object_contents = object.to_json_escaped();
    String^ sobject_contents = gcnew String(object_contents.c_str());
    // Fix: assign to the property, not the type
    baseResponse->SysObject = JsonConvert::DeserializeObject<CoronaInterface::SysObject^>(sobject_contents);

    corona::json classes = data["classes"];
    std::string classes_contents = classes.to_json_escaped();
    String^ sclasses_contents = gcnew String(classes_contents.c_str());
    baseResponse->Classes = JsonConvert::DeserializeObject<System::Collections::Generic::Dictionary<String^, CoronaInterface::CoronaClass^> ^>(sclasses_contents);

    corona::json child_classes = data["child_classes"];
    std::string child_classes_contents = child_classes.to_json_escaped();
    String^ schild_classes_contents = gcnew String(child_classes_contents.c_str());
    baseResponse->ChildClasses = JObject::Parse(schild_classes_contents);

    return success;
}

bool put_response(CoronaInterface::IRunObjectResponse^ baseResponse, corona::json result)
{
    bool success_base = put_response_base(baseResponse, result);

    corona::json data = result["data"];

    corona::json object = data["object"];
    std::string object_contents = object.to_json_escaped();
    String^ sobject_contents = gcnew String(object_contents.c_str());
    // Fix: assign to the property, not the type
    baseResponse->SysObject = JsonConvert::DeserializeObject<CoronaInterface::SysObject^>(sobject_contents);

    corona::json classes = data["classes"];
    std::string classes_contents = classes.to_json_escaped();
    String^ sclasses_contents = gcnew String(classes_contents.c_str());
    baseResponse->Classes = JsonConvert::DeserializeObject<System::Collections::Generic::Dictionary<String^, CoronaInterface::CoronaClass^>^>(sclasses_contents);

    corona::json child_classes = data["child_classes"];
    std::string child_classes_contents = child_classes.to_json_escaped();
    String^ schild_classes_contents = gcnew String(child_classes_contents.c_str());
    baseResponse->ChildClasses = JObject::Parse(schild_classes_contents);

    return success_base;
}

template <typename interface_type, typename response_type, typename request_type> response_type^ process_request(corona::corona_database* db, String^ token, request_type^ request, std::function<corona::json(corona::corona_database * _db, corona::json _request)> implementation) 
{
    if (db == nullptr) {
        throw gcnew System::ArgumentNullException("db");
    }
    if (request == nullptr) {
        throw gcnew System::ArgumentNullException("request");
    }
    if (!db) {
        throw gcnew System::InvalidOperationException("Database not open");
    }

    corona::json_parser jp;
    corona::json jrequest;

    String^ request_string_managed = request != nullptr ? JsonConvert::SerializeObject(request) : "";
    std::string request_string = request_string_managed != nullptr ? msclr::interop::marshal_as<std::string>(request_string_managed) : "";

    jrequest = jp.parse_object(request_string);
    jrequest.put_member("token", msclr::interop::marshal_as<std::string>(token != nullptr ? token : ""));

    corona::json response = implementation(db, jrequest);

    response_type^ netresult = gcnew response_type();

    put_response(netresult, response);
}

std::function<corona::json(corona::corona_database *_db, corona::json _request)> user_set_team_impl = [](corona::corona_database* _db, corona::json _request) {
    return _db->user_set_team(_request);
    };

std::function<corona::json(corona::corona_database* _db, corona::json _request)> create_user_impl = [](corona::corona_database* _db, corona::json _request) {
    return _db->create_user(_request);
    };

std::function<corona::json(corona::corona_database* _db, corona::json _request)> confirm_user_impl = [](corona::corona_database* _db, corona::json _request) {
    return _db->user_confirm_code(_request);
    };

std::function<corona::json(corona::corona_database* _db, corona::json _request)> get_class_impl = [](corona::corona_database* _db, corona::json _request) {
    return _db->get_class(_request);
    };

std::function<corona::json(corona::corona_database* _db, corona::json _request)> put_class_impl = [](corona::corona_database* _db, corona::json _request) {
    return _db->put_class(_request);
    };

std::function<corona::json(corona::corona_database* _db, corona::json _request)> get_object_impl = [](corona::corona_database* _db, corona::json _request) {
    return _db->get_object(_request);
    };

std::function<corona::json(corona::corona_database* _db, corona::json _request)> edit_object_impl = [](corona::corona_database* _db, corona::json _request) {
    return _db->edit_object(_request);
    };

std::function<corona::json(corona::corona_database* _db, corona::json _request)> run_object_impl = [](corona::corona_database* _db, corona::json _request) {
    return _db->run_object(_request);
    };

std::function<corona::json(corona::corona_database* _db, corona::json _request)> put_objects_impl = [](corona::corona_database* _db, corona::json _request) {
    return _db->put_object(_request);
    };

std::function<corona::json(corona::corona_database* _db, corona::json _request)> delete_objects_impl = [](corona::corona_database* _db, corona::json _request) {
    return _db->delete_object(_request);
    };

std::function<corona::json(corona::corona_database* _db, corona::json _request)> query_class_impl = [](corona::corona_database* _db, corona::json _request) {
    return _db->query_class(_db->default_user, _request);
    };

std::function<corona::json(corona::corona_database* _db, corona::json _request)> query_impl = [](corona::corona_database* _db, corona::json _request) {
    return _db->query(_request);
    };

bool CoronaLib::CoronaDatabase::CreateDatabase(CoronaInterface::DatabaseConfiguration^ configuration)
{
    corona::json_parser jp;
    corona::json config_json = jp.create_object();

    String^ config_string_managed = configuration != nullptr ? JsonConvert::SerializeObject(configuration) : "";
    std::string config_string = configuration != nullptr ? msclr::interop::marshal_as<std::string>(config_string_managed) : "";
    
    config_json = jp.parse_object(config_string);
    m_database = new corona::corona_database();
    m_database->apply_config(config_json);
    corona::json jresponse = m_database->create_database();
    bool success = (bool)jresponse[corona::success_field];
    return success;
}

bool CoronaLib::CoronaDatabase::OpenDatabase(CoronaInterface::DatabaseConfiguration^ configuration)
{
    corona::json_parser jp;
    corona::json config_json = jp.create_object();

    String^ config_string_managed = configuration != nullptr ? JsonConvert::SerializeObject(configuration) : "";
    std::string config_string = configuration != nullptr ? msclr::interop::marshal_as<std::string>(config_string_managed) : "";

    config_json = jp.parse_object(config_string);
    m_database = new corona::corona_database();
    m_database->apply_config(config_json);
    auto result = m_database->open_database();
    return result >= -1;
}

CoronaInterface::ILoginResult^ CoronaLib::CoronaDatabase::LoginLocal(System::String^ username, System::String^ email)
{

    if (!m_database) {
        throw gcnew System::InvalidOperationException("Database not open");
    }

    std::string user_name = username != nullptr ? msclr::interop::marshal_as<std::string>(username) : "";
    std::string email_str = email != nullptr ? msclr::interop::marshal_as<std::string>(email) : "";
    std::string code_str = corona::application::get_machine_id();
    corona::json_parser jp;

    corona::json request = jp.create_object();
    corona::json data = jp.create_object();
    data.put_member("username", user_name);
    data.put_member("email", email_str);
    data.put_member("code", code_str);
    request.share_member("data", data);

    corona::json response = m_database->login_user_local(request);

    CoronaLib::LoginResult^ loginResult = gcnew CoronaLib::LoginResult();
    put_response(loginResult, response);
    this->token = loginResult->Token;
    return loginResult;
}

CoronaInterface::ILoginResult^ CoronaLib::CoronaDatabase::LoginUser(System::String^ username, System::String^ password)
{
    if (!m_database) {
        throw gcnew System::InvalidOperationException("Database not open");
    }
    std::string user_name = username != nullptr ? msclr::interop::marshal_as<std::string>(username) : "";
    std::string pass_word = password != nullptr ? msclr::interop::marshal_as<std::string>(password) : "";

    corona::json_parser jp;

    corona::json request = jp.create_object();
    corona::json data = jp.create_object();
    data.put_member("username", user_name);
    data.put_member("password", pass_word);
    request.share_member("data", data);

    corona::json response = m_database->login_user(request);

    CoronaLib::LoginResult^ loginResult = gcnew CoronaLib::LoginResult();

    put_response(loginResult, response);
    token = loginResult->Token;

    return loginResult;
}

CoronaInterface::ILoginResult^ CoronaLib::CoronaDatabase::LoginUserSso(System::String^ username, System::String^ email, System::String^ code)
{
    if (!m_database) {
        throw gcnew System::InvalidOperationException("Database not open");
    }
    std::string user_name = username != nullptr ? msclr::interop::marshal_as<std::string>(username) : "";
    std::string email_str = email != nullptr ? msclr::interop::marshal_as<std::string>(email) : "";
    std::string code_str = code != nullptr ? msclr::interop::marshal_as<std::string>(code) : "";

    corona::json_parser jp;

    corona::json request = jp.create_object();
    corona::json data = jp.create_object();
    data.put_member("username", user_name);
    data.put_member("email", email_str);
    data.put_member("code", code_str);
    request.share_member("data", data);

    corona::json response = m_database->login_user_sso(request);

    CoronaLib::LoginResult^ loginResult = gcnew CoronaLib::LoginResult();

    put_response(loginResult, response);
    token = loginResult->Token;

    return loginResult;
}

CoronaInterface::ICreateUserResponse^ CoronaLib::CoronaDatabase::CreateUser(CoronaInterface::ICreateUserRequest^ request)
{
    if (!m_database) {
        throw gcnew System::InvalidOperationException("Database not open");
    }

    corona::json_parser jp;
    corona::json jrequest;

    String^ request_string_managed = request != nullptr ? JsonConvert::SerializeObject(request) : "";
    std::string request_string = request_string_managed != nullptr ? msclr::interop::marshal_as<std::string>(request_string_managed) : "";

    jrequest = jp.parse_object(request_string);
    jrequest.put_member("token", msclr::interop::marshal_as<std::string>(token != nullptr ? token : ""));

    corona::json response = m_database->create_user(jrequest);

    CoronaLib::CreateUserResponse^ loginResult = gcnew CoronaLib::CreateUserResponse();

    put_response_base(loginResult, response);
    return loginResult;
}

CoronaInterface::ISetSendCodeResponse^ CoronaLib::CoronaDatabase::SetSendCode(CoronaInterface::ISetSendCodeRequest^ request)
{
    CoronaLib::SetSendCodeResponse^ netresult = nullptr;

    return netresult;
}

CoronaInterface::IConfirmUserCodeResponse^ CoronaLib::CoronaDatabase::ConfirmUserCode(CoronaInterface::IConfirmUserCodeRequest^ request)
{
    CoronaLib::ConfirmUserCodeResponse^ netresult = process_request<CoronaInterface::IConfirmUserCodeResponse, CoronaLib::ConfirmUserCodeResponse, CoronaInterface::IConfirmUserCodeRequest>(
        m_database,
        token,
        request,
        confirm_user_impl
    );

    return netresult;
}

CoronaInterface::IGetClassResponse^ CoronaLib::CoronaDatabase::GetClass(CoronaInterface::IGetClassRequest^ request)
{
    CoronaLib::GetClassResponse^ netresult = process_request<CoronaInterface::IGetClassResponse, CoronaLib::GetClassResponse, CoronaInterface::IGetClassRequest>(
        m_database,
        token,
        request,
        get_class_impl
    );

    return netresult;
}

CoronaInterface::IPutClassResponse^ CoronaLib::CoronaDatabase::PutClass(CoronaInterface::IPutClassRequest^ request)
{
    CoronaLib::PutClassResponse^ netresult = process_request<CoronaInterface::IPutClassResponse, CoronaLib::PutClassResponse, CoronaInterface::IPutClassRequest>(
        m_database,
        token,
        request,
        put_class_impl
    );

    return netresult;
}

CoronaInterface::IGetObjectResponse^ CoronaLib::CoronaDatabase::GetObject(CoronaInterface::IGetObjectRequest^ request)
{
    CoronaLib::GetObjectResponse^ netresult = process_request<CoronaInterface::IGetObjectResponse, CoronaLib::GetObjectResponse, CoronaInterface::IGetObjectRequest>(
        m_database,
        token,
        request,
        get_object_impl
    );

    return netresult;
}

CoronaInterface::IEditObjectResponse^ CoronaLib::CoronaDatabase::EditObject(CoronaInterface::IEditObjectRequest^ request)
{
    CoronaLib::EditObjectResponse^ netresult = process_request<CoronaInterface::IEditObjectResponse, CoronaLib::EditObjectResponse, CoronaInterface::IEditObjectRequest>(
        m_database,
        token,
        request,
        edit_object_impl
    );

    return netresult;
}

CoronaInterface::IRunObjectResponse^ CoronaLib::CoronaDatabase::RunObject(CoronaInterface::IRunObjectRequest^ request)
{
    CoronaLib::RunObjectResponse^ netresult = process_request<CoronaInterface::IRunObjectResponse, CoronaLib::RunObjectResponse, CoronaInterface::IRunObjectRequest>(
        m_database,
        token,
        request,
        run_object_impl
    );

    return netresult;
}

CoronaInterface::IPutObjectsResponse^ CoronaLib::CoronaDatabase::PutObjects(CoronaInterface::IPutObjectsRequest^ request)
{
    CoronaLib::PutObjectsResponse^ netresult = process_request<CoronaInterface::IPutObjectsResponse, CoronaLib::PutObjectsResponse, CoronaInterface::IPutObjectsRequest>(
        m_database,
        token,
        request,
        put_objects_impl
    );

    return netresult;
}

CoronaInterface::IDeleteObjectsResponse^ CoronaLib::CoronaDatabase::DeleteObjects(CoronaInterface::IDeleteObjectsRequest^ request)
{
    CoronaLib::DeleteObjectsResponse^ netresult = process_request<CoronaInterface::IDeleteObjectsResponse, CoronaLib::DeleteObjectsResponse, CoronaInterface::IDeleteObjectsRequest>(
        m_database,
        token,
        request,
        delete_objects_impl
    );

    return netresult;
}

CoronaInterface::IQueryClassResponse^ CoronaLib::CoronaDatabase::QueryClass(CoronaInterface::IQueryClassRequest^ request)
{
    CoronaLib::QueryClassResponse^ netresult = process_request<CoronaInterface::IQueryClassResponse, CoronaLib::QueryClassResponse, CoronaInterface::IQueryClassRequest>(
        m_database,
        token,
        request,
        query_class_impl
    );
    return netresult;
}

CoronaInterface::IQueryResponse^ CoronaLib::CoronaDatabase::Query(CoronaInterface::IQueryRequest^ request)
{
    CoronaLib::QueryResponse^ netresult = process_request<CoronaInterface::IQueryResponse, CoronaLib::QueryResponse, CoronaInterface::IQueryRequest>(
        m_database,
        token,
        request,
        query_impl
    );
    return netresult;
}

CoronaInterface::ILoginResult^ CoronaLib::CoronaDatabase::UserSetTeam(CoronaInterface::IUserSetTeamRequest^ request)
{
    CoronaLib::LoginResult^ netresult = process_request<CoronaInterface::ILoginResult, CoronaLib::LoginResult, CoronaInterface::IUserSetTeamRequest>(
        m_database,
        token,
        request,
        user_set_team_impl
    );
    return netresult;
}
