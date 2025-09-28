#pragma once

#ifndef CORONA_CORONA_CLIENT_HPP
#define CORONA_CORONA_CLIENT_HPP

/*

Now that you wrote the open api wrapper, and the class generator from it, you can just 
generate all this stuff... or just use C# on the client end.

*/

namespace corona
{


	class corona_client : public corona_client_interface
	{
	public:
		std::string host;
		int port;
		std::string base_path;
		std::string authorization_header;

		corona_client()
		{
			;
		}

		virtual ~corona_client()
		{
			;
		}

		virtual corona_client_response register_user(json _user) override
		{
			corona_client_response result;

			json_parser jp;
			http_client cc;

			authorization_header = "";

			std::string header = "Content-Type: application/json\r\n";

			json payload = jp.create_object();
			payload.put_member("data", _user);

			std::string path = base_path + "/corona/login/createuser";

			http_params params = cc.post(host.c_str(), port, path.c_str(), payload, header.c_str());

			result = params;

			return result;
		}

		virtual corona_client_response confirm_user(std::string user_name, std::string confirmation_code) override
		{
			json_parser jp;

			corona_client_response result;

			http_client cc;

			authorization_header = "";

			std::string header = "Content-Type: application/json\r\n";

			json payload = jp.create_object();
			json data = jp.create_object();
			data.put_member("user_name", user_name);
			data.put_member("validation_code", confirmation_code);

			std::string path = base_path + "corona/login/confirmuser";

			http_params params = cc.post(host.c_str(), port, path.c_str(), payload, header.c_str());

			result = params;

			return result;

		}

		virtual corona_client_response send_user(std::string user_name) override
		{
			corona_client_response result;

			json_parser jp;
			http_client cc;

			authorization_header = "";

			std::string header = "Content-Type: application/json\r\n";

			json payload = jp.create_object();
			json data = jp.create_object();
			data.put_member("user_name", user_name);
			payload.put_member("data", data);

			std::string path = base_path + "/corona/login/senduser";

			http_params params = cc.post(host.c_str(), port, path.c_str(), payload, header.c_str());

			result = params;

			return result;

		}

		std::string user_name;
		std::string user_password;
		std::string token;

		virtual corona_client_response login(std::string _user_name, std::string _password) override
		{
			corona_client_response result;

			user_name = _user_name;
			user_password = _password;

			result = login();

			return result;
		}

		virtual corona_client_response login() override
		{
			corona_client_response result;

			json_parser jp;
			http_client cc;

			std::string header = "Content-Type: application/json\r\n" + authorization_header;

			json payload = jp.create_object();
			json data = jp.create_object();
			data.put_member("user_name", user_name);
			data.put_member("password", user_password);
			payload.put_member("data", data);

			std::string path = base_path + "/corona/login/loginuser";

			http_params params = cc.post(host.c_str(), port, path.c_str(), payload, header.c_str());

			result = params;

			if (result.success) 
			{
				authorization_header = "Authorization: Bearer " + result.data[token_field];
			}
			else 
			{
				authorization_header = "";
			}

			return result;
		}

		virtual corona_client_response set_password(std::string user_name, std::string validation_code, std::string password1, std::string password2) override
		{
			corona_client_response result;

			json_parser jp;
			http_client cc;

			std::string header = "Content-Type: application/json\r\n" + authorization_header;

			json payload = jp.create_object();
			json data = jp.create_object();
			data.put_member("user_name", user_name);
			data.put_member("validation_code", validation_code);
			data.put_member("password1", password1);
			data.put_member("password2", password2);
			payload.put_member("data", data);

			std::string path = base_path + "/corona/login/passworduser";

			http_params params = cc.post(host.c_str(), port, path.c_str(), payload, header.c_str());

			result = params;

			return result;
		}

		virtual corona_client_response get_classes() override
		{
			corona_client_response result;

			json_parser jp;
			http_client cc;

			std::string header = "Content-Type: application/json\r\n" + authorization_header;

			json payload = jp.create_object();
			std::string path = base_path + "/corona/classes/get";

			http_params params = cc.post(host.c_str(), port, path.c_str(), payload, header.c_str());

			result = params;

			return result;
		}

		virtual corona_client_response get_class(std::string class_name) override
		{
			corona_client_response result;

			json_parser jp;
			http_client cc;

			std::string header = "Content-Type: application/json\r\n" + authorization_header;

			json payload = jp.create_object();
			payload.put_member("class_name", class_name);

			std::string path = base_path + "/corona/classes/get/details/";

			http_params params = cc.post(host.c_str(), port, path.c_str(), payload, header.c_str());

			result = params;

			return result;
		}

		virtual corona_client_response put_class(json _class_definition) override
		{
			corona_client_response result;
			json_parser jp;
			http_client cc;

			std::string header = "Content-Type: application/json\r\n" + authorization_header;

			json payload = jp.create_object();
			payload.put_member("data", _class_definition);

			std::string path = base_path + "/corona/classes/put/";

			http_params params = cc.post(host.c_str(), port, path.c_str(), payload, header.c_str());

			result = params;

			return result;
		}

		virtual corona_client_response create_object(std::string _class_name) override
		{
			corona_client_response result;
			json_parser jp;
			http_client cc;

			std::string header = "Content-Type: application/json\r\n" + authorization_header;

			json payload = jp.create_object();
			json data = jp.create_object();

			data.put_member(class_name_field, _class_name);

			payload.put_member("data", data);

			std::string path = base_path + "/corona/objects/create/";

			http_params params = cc.post(host.c_str(), port, path.c_str(), payload, header.c_str());

			result = params;

			return result;
		}


		virtual corona_client_response edit_object(std::string _class_name, int64_t _object_id, bool _object_id_field) override
		{
			corona_client_response result;
			json_parser jp;
			http_client cc;

			std::string header = "Content-Type: application/json\r\n" + authorization_header;

			json payload = jp.create_object();
			json data = jp.create_object();

			data.put_member(class_name_field, _class_name);
			data.put_member(object_id_field, _object_id);
			data.put_member("include_children", true);

			payload.put_member("data", data);

			std::string path = base_path + "/corona/objects/get/";

			http_params params = cc.post(host.c_str(), port, path.c_str(), payload, header.c_str());

			result = params;

			return result;
		}

		virtual corona_client_response get_object(std::string _class_name, int64_t _object_id, bool _include_children) override
		{
			corona_client_response result;
			json_parser jp;
			http_client cc;

			std::string header = "Content-Type: application/json\r\n" + authorization_header;

			json payload = jp.create_object();
			json data = jp.create_object();

			data.put_member(class_name_field, _class_name);
			data.put_member(object_id_field, _object_id);
			data.put_member("include_children", _include_children);

			payload.put_member("data", data);

			std::string path = base_path + "/corona/objects/get/";

			http_params params = cc.post(host.c_str(), port, path.c_str(), payload, header.c_str());

			result = params;

			return result;
		}

		virtual corona_client_response put_object(json _object) override
		{
			corona_client_response result;
			json_parser jp;
			http_client cc;

			std::string header = "Content-Type: application/json\r\n" + authorization_header;

			json payload = jp.create_object();
			payload.put_member("data", _object);

			std::string path = base_path + "/corona/objects/put/";

			http_params params = cc.post(host.c_str(), port, path.c_str(), payload, header.c_str());

			result = params;

			return result;
		}

		virtual corona_client_response run_object(json _object) override
		{
			corona_client_response result;
			json_parser jp;
			http_client cc;

			std::string header = "Content-Type: application/json\r\n" + authorization_header;

			json payload = jp.create_object();
			payload.put_member("data", _object);

			std::string path = base_path + "/corona/objects/run/";

			http_params params = cc.post(host.c_str(), port, path.c_str(), payload, header.c_str());

			result = params;

			return result;
		}

		virtual corona_client_response delete_object(std::string _class_name, int64_t _object_id) override
		{
			corona_client_response result;
			json_parser jp;
			http_client cc;

			std::string header = "Content-Type: application/json\r\n" + authorization_header;

			json payload = jp.create_object();
			json data = jp.create_object();

			data.put_member(class_name_field, _class_name);
			data.put_member(object_id_field, _object_id);

			payload.put_member("data", data);

			std::string path = base_path + "/corona/objects/delete/";

			http_params params = cc.post(host.c_str(), port, path.c_str(), payload, header.c_str());

			result = params;

			return result;
		}

		virtual corona_client_response query_objects(json _query) override
		{
			corona_client_response result;

			json_parser jp;
			http_client cc;

			std::string header = "Content-Type: application/json\r\n" + authorization_header;

			std::string path = base_path + "/corona/objects/query/";

			http_params params = cc.post(host.c_str(), port, path.c_str(), _query, header.c_str());

			result = params;

			return result;
		}

	};

}

#endif
