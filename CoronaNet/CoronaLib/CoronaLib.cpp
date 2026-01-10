#include "corona.hpp"
#include "CoronaLib.h"
#include <msclr\marshal_cppstd.h>

using namespace Newtonsoft::Json;
using namespace System::Collections::Generic;
using namespace System::Dynamic;
using namespace System;

Object^ expand( corona::json result )
{
    auto expando = gcnew ExpandoObject();
    auto dict = (IDictionary<String^, Object^> ^)expando;

    if (result.object()) {

        for (auto& member : result.object_impl()->members) {
            std::string member_name = member.first;
            corona::json member_value = member.second;
            String^ key = gcnew String(member_name.c_str());
            if (member_value.object()) {
                auto exp = expand(member_value);
                dict->Add(key, exp);
            }
            else if (member_value.array()) {
                auto array = gcnew List<Object^>(member_value.size());
                for (int i = 0; i < member_value.size(); i++) {
                    auto var = member_value.get_element(i);
                    auto exp2 = expand(var);
                    array->Add(exp2);
                }
                dict->Add(key, array);
            }
            else {
                auto exp = expand(member_value);
                dict->Add(key, exp);
            }
        }
    }
    else if (result.array())
    {
        auto array = gcnew List<Object^>(result.size());
        for (auto vvar : result.array_impl()->elements) {
            auto exp2 = expand(vvar);
            array->Add(exp2);
        }
        return array;
    }
    else if (result.value())
    {
        switch (result.get_field_type()) {
            case corona::field_types::ft_bool:
                return gcnew System::Boolean(result.value_impl()->to_bool());
            case corona::field_types::ft_int64:
                return gcnew System::Int64(result.value_impl()->to_int64());
            case corona::field_types::ft_double:
                return gcnew System::Double(result.value_impl()->to_double());
            case corona::field_types::ft_datetime:
            {
                auto datetime =  result.value_impl()->to_datetime();
                SYSTEMTIME st = (SYSTEMTIME)datetime;  
                return gcnew System::DateTime(st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
            }
            case corona::field_types::ft_string:
                return gcnew System::String(result.value_impl()->to_string().c_str());
        }
    }

    return expando;
}

bool put_response_base(CoronaInterface::ICoronaBaseResponse^ baseResponse, corona::json result)
{
    bool success = false;

    try 
    {    
        ExpandoObject^ expando = gcnew ExpandoObject();

        std::string msg = result["message"].as_string();

        baseResponse->Success = result["success"].as_bool();
        baseResponse->Message = gcnew System::String(msg.c_str());
        baseResponse->ExecutionTimeSeconds = result["execution_time"].as_double();

        if (result.has_member("data")) {
            corona::json data = result["data"];
            baseResponse->Data = expand(data);
        }

        if (result.has_member("token"))
        {
            std::string token = result["token"].as_string();
            baseResponse->Token = gcnew System::String(token.c_str());
        }

        if (result.has_member("errors"))
        {
            corona::json errors = result["errors"];
            baseResponse->Errors = gcnew System::Collections::Generic::List<CoronaInterface::CoronaError^>();
            if (errors.array()) {
                for (auto error : errors) {
                    std::string error_contents = error.to_json();
                    baseResponse->Errors->Add(JsonConvert::DeserializeObject<CoronaInterface::CoronaError^>(gcnew System::String(error_contents.c_str())));
                }
            }
        }

        success = true;
    }
    catch (std::exception& ex) {
        std::string err_msg = ex.what();
        baseResponse->Success = false;
        baseResponse->Message = gcnew System::String(err_msg.c_str());
    }
    catch (...) {
        baseResponse->Success = false;
        baseResponse->Message = gcnew System::String("Unknown error in put_response_base");
    }

    return success;
}

bool put_response(CoronaInterface::ILoginResult^ baseResponse, corona::json result)
{
    bool success_base = put_response_base(baseResponse, result);

    try {

        if (baseResponse->Data != nullptr) {
            std::string class_data = result["data"].as_string() ;
            String^ sclass_data = gcnew String(class_data.c_str());
            baseResponse->User = JsonConvert::DeserializeObject<CoronaInterface::SysUser^>(sclass_data);
        }

    }
    catch (std::exception& ex) {
        std::string err_msg = ex.what();
        baseResponse->Success = false;
        baseResponse->Message = gcnew System::String(err_msg.c_str());
    }
    catch (System::Exception^ ex) {
        baseResponse->Success = false;
        baseResponse->Message = ex->Message;
    }
    catch (...) {
        baseResponse->Success = false;
        baseResponse->Message = gcnew System::String("Unknown error in put_response LoginResult");
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

    try
    {
        if (baseResponse->Data != nullptr) {
            std::string class_data = result["data"]["class"].as_string();
            String^ sclass_data = gcnew String(class_data.c_str());
            baseResponse->CoronaClass = JsonConvert::DeserializeObject<CoronaInterface::CoronaClass^>(sclass_data);
        }
    }
    catch (std::exception& ex) {
        std::string err_msg = ex.what();
        baseResponse->Success = false;
        baseResponse->Message = gcnew System::String(err_msg.c_str());
    }
    catch (...) {
        baseResponse->Success = false;
        baseResponse->Message = gcnew System::String("Unknown error in put_response LoginResult");
    }

    return success_base;
}

bool put_response(CoronaInterface::IPutClassResponse^ baseResponse, corona::json result)
{
    bool success_base = put_response_base(baseResponse, result);

    try {
        if (baseResponse->Data != nullptr) {
            baseResponse->CoronaClass = JsonConvert::DeserializeObject<CoronaInterface::CoronaClass^>(baseResponse->Data->ToString());
        }
    }
    catch (std::exception& ex) {
        std::string err_msg = ex.what();
        baseResponse->Success = false;
        baseResponse->Message = gcnew System::String(err_msg.c_str());
    }
    catch (...) {
        baseResponse->Success = false;
        baseResponse->Message = gcnew System::String("Unknown error in put_response PutClassResponse");
    }

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

    try {
        if (baseResponse->Data != nullptr) {
            baseResponse->User = JsonConvert::DeserializeObject<CoronaInterface::SysUser^>(baseResponse->Data->ToString());
        }
    }
    catch (std::exception& ex) {
        std::string err_msg = ex.what();
        baseResponse->Success = false;
        baseResponse->Message = gcnew System::String(err_msg.c_str());
    }
    catch (...) {
        baseResponse->Success = false;
        baseResponse->Message = gcnew System::String("Unknown error in put_response ConfirmUserCodeResponse");
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
    bool success = false;


    try {

        ExpandoObject^ expando = gcnew ExpandoObject();

        std::string msg = result["message"].as_string();

        baseResponse->Success = result["success"].as_bool();
        baseResponse->Message = gcnew System::String(msg.c_str());
        baseResponse->ExecutionTimeSeconds = result["execution_time"].as_double() ;

        if (result.has_member("data")) {
            corona::json data = result["data"];
            corona::json classes = data["classes"];
            corona::json objects = data["object"];
            baseResponse->Data = expand(objects);
            std::string object_contents = objects.to_json();
            String^ sobject_contents = gcnew String(object_contents.c_str());
            // Fix: assign to the property, not the type
            baseResponse->SysObject = JsonConvert::DeserializeObject<CoronaInterface::SysObject^>(sobject_contents);
            std::string classes_contents = classes.to_json();
            String^ sclasses_contents = gcnew String(classes_contents.c_str());
            baseResponse->Classes = JsonConvert::DeserializeObject<System::Collections::Generic::Dictionary<String^, CoronaInterface::CoronaClass^>^>(sclasses_contents);
        }

        if (result.has_member("token"))
        {
            std::string token = result["token"].as_string();
            baseResponse->Token = gcnew System::String(token.c_str());
        }

        if (result.has_member("errors"))
        {
            corona::json errors = result["errors"];
            baseResponse->Errors = gcnew System::Collections::Generic::List<CoronaInterface::CoronaError^>();
            if (errors.array()) {
                for (auto error : errors) {
                    std::string error_contents = error.to_json();
                    baseResponse->Errors->Add(JsonConvert::DeserializeObject<CoronaInterface::CoronaError^>(gcnew System::String(error_contents.c_str())));
                }
            }
        }

    }
    catch (std::exception& ex) {
        std::string err_msg = ex.what();
        baseResponse->Success = false;
        baseResponse->Message = gcnew System::String(err_msg.c_str());
    }
    catch (...) {
        baseResponse->Success = false;
        baseResponse->Message = gcnew System::String("Unknown error in put_response EditObjectResponse");
    }
  
    return success;
}

bool put_response(CoronaInterface::IRunObjectResponse^ baseResponse, corona::json result)
{
    bool success = false;

    try
    {

        ExpandoObject^ expando = gcnew ExpandoObject();

        std::string msg = result["message"].as_string();

        baseResponse->Success = result["success"].as_bool();
        baseResponse->Message = gcnew System::String(msg.c_str());
        baseResponse->ExecutionTimeSeconds = result["execution_time"].as_double();

        if (result.has_member("data")) {
            corona::json data = result["data"];
            corona::json classes = data["classes"];
            corona::json objects = data["object"];
            baseResponse->Data = expand(objects);
            std::string object_contents = objects.to_json();
            String^ sobject_contents = gcnew String(object_contents.c_str());
            // Fix: assign to the property, not the type
            baseResponse->SysObject = JsonConvert::DeserializeObject<CoronaInterface::SysObject^>(sobject_contents);
            std::string classes_contents = classes.to_json();
            String^ sclasses_contents = gcnew String(classes_contents.c_str());
            baseResponse->Classes = JsonConvert::DeserializeObject<System::Collections::Generic::Dictionary<String^, CoronaInterface::CoronaClass^>^>(sclasses_contents);
        }

        if (result.has_member("token"))
        {
            std::string token = result["token"].as_string();
            baseResponse->Token = gcnew System::String(token.c_str());
        }

        if (result.has_member("errors"))
        {
            corona::json errors = result["errors"];
            baseResponse->Errors = gcnew System::Collections::Generic::List<CoronaInterface::CoronaError^>();
            if (errors.array()) {
                for (auto error : errors) {
                    std::string error_contents = error.to_json();
                    baseResponse->Errors->Add(JsonConvert::DeserializeObject<CoronaInterface::CoronaError^>(gcnew System::String(error_contents.c_str())));
                }
            }
        }

    }
    catch (std::exception& ex) {
        std::string err_msg = ex.what();
        baseResponse->Success = false;
        baseResponse->Message = gcnew System::String(err_msg.c_str());
    }
    catch (...) {
        baseResponse->Success = false;
        baseResponse->Message = gcnew System::String("Unknown error in put_response RunObjectResponse");
    }

    return success;
}

template <typename interface_type, typename response_type, typename request_type> response_type^ process_request(corona::corona_database* db, String^ token, request_type^ request, std::function<corona::json(corona::corona_database * _db, corona::json _request)> implementation) 
{
    response_type^ netresult = gcnew response_type();

    try
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

        put_response(netresult, response);
    }
    catch (std::exception& ex) 
    {
        netresult->Success = false;
        netresult->Message = gcnew System::String(ex.what());
    }

    return netresult;
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
    bool success = false;
    try {
        corona::json_parser jp;
        corona::json config_json = jp.create_object();

        String^ config_string_managed = configuration != nullptr ? JsonConvert::SerializeObject(configuration) : "";
        std::string config_string = configuration != nullptr ? msclr::interop::marshal_as<std::string>(config_string_managed) : "";

        config_json = jp.parse_object(config_string);
        if (jp.parse_errors.size() > 0) {
            for (auto& err : jp.parse_errors) {
                corona::system_monitoring_interface::active_mon->log_warning(err.error, "", err.line);
            }
        }
        std::string database_path = msclr::interop::marshal_as<std::string>(configuration->DatabasePath);
        std::string schema_path = msclr::interop::marshal_as<std::string>(configuration->SchemaPath);

        m_database = new corona::corona_database(database_path, schema_path);
        m_database->apply_config(config_json);
        corona::json jresponse = m_database->create_database();
        success = jresponse[corona::success_field].as_bool();
        if (success) {
            m_database->apply_schema();
        }
    }
    catch (const std::exception& ex) {
        corona::system_monitoring_interface::active_mon->log_warning(ex.what(), "", 0);
    }
    return success;
}

bool CoronaLib::CoronaDatabase::OpenDatabase(CoronaInterface::DatabaseConfiguration^ configuration)
{
    bool success = false;

    try {

        corona::json_parser jp;
        corona::json config_json = jp.create_object();

        String^ config_string_managed = configuration != nullptr ? JsonConvert::SerializeObject(configuration) : "";
        std::string config_string = configuration != nullptr ? msclr::interop::marshal_as<std::string>(config_string_managed) : "";

        config_json = jp.parse_object(config_string);
        std::string database_path = msclr::interop::marshal_as<std::string>(configuration->DatabasePath);
        std::string schema_path = msclr::interop::marshal_as<std::string>(configuration->SchemaPath);
        m_database = new corona::corona_database(database_path, schema_path);
        m_database->apply_config(config_json);
        auto result = m_database->open_database();
        if (result >= -1) {
            m_database->apply_schema();
        }
        else {
            result = CreateDatabase(configuration);
        }
        success = (result >= -1);
    }
    catch (const std::exception& ex) {
        corona::system_monitoring_interface::active_mon->log_warning(ex.what(), "", 0);
    }

    return success;
}

void CoronaLib::CoronaDatabase::ApplySchema(String^ configuration)
{
    //m_database->apply_schema();
}

CoronaInterface::ILoginResult^ CoronaLib::CoronaDatabase::LoginLocal(System::String^ username, System::String^ email)
{
    CoronaLib::LoginResult^ loginResult = gcnew CoronaLib::LoginResult();

    try
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
        data.put_member("user_name", user_name);
        data.put_member("email", email_str);
        data.put_member("access_code", code_str);
        request.share_member("data", data);

        corona::json response = m_database->login_user_local(request);

        put_response(loginResult, response);
        this->token = loginResult->Token;

    }
    catch (std::exception& ex) 
    {
        loginResult->Success = false;
        loginResult->Message = gcnew System::String(ex.what());
    }

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
    data.put_member("user_name", user_name);
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
    data.put_member("user_name", user_name);
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

public ref class database_query_class
{
public:
    CoronaLib::CoronaDatabase^ db;
    CoronaInterface::IQueryClassRequest^ request;

    CoronaInterface::IQueryClassResponse^ RunQuery()
    {
        return db->QueryClass(request);
    }
};

Task<CoronaInterface::IQueryClassResponse^> ^CoronaLib::CoronaDatabase::QueryClassAsync(CoronaInterface::IQueryClassRequest^ request)
{
    auto dqc = gcnew database_query_class();
    dqc->db = this;
    dqc->request = request;

    auto new_query = gcnew Func<CoronaInterface::IQueryClassResponse^>(dqc, &database_query_class::RunQuery);
    auto v = Task::Run<CoronaInterface::IQueryClassResponse^>(new_query);
    return v;
}

public ref class database_query
{
public:
    CoronaLib::CoronaDatabase^ db;
    CoronaInterface::IQueryRequest^ request;

    CoronaInterface::IQueryResponse^ RunQuery()
    {
        return db->Query(request);
    }
};

Task<CoronaInterface::IQueryResponse^>^ CoronaLib::CoronaDatabase::QueryAsync(CoronaInterface::IQueryRequest^ request)
{
    auto dq = gcnew database_query();
    dq->db = this;
    dq->request = request;

    auto new_query = gcnew Func<CoronaInterface::IQueryResponse^>(dq, &database_query::RunQuery);
    auto v = Task::Run<CoronaInterface::IQueryResponse^>(new_query);
    return v;
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
