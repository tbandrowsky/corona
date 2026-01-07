#pragma once


namespace corona
{
	class client_class_interface
	{

	};

	class corona_client_response
	{
	public:
		bool			success;
		std::string		message;
		double			execution_time;
		json			data;
		validation_error_collection errors;
		std::vector<std::shared_ptr<client_class_interface>> classes;

		void set(json& response)
		{
			success = response[success_field].as_bool();
			message = response[message_field].as_string();
			execution_time = response["execution_time_seconds"].as_double();
			data = response["data"];
			errors.clear();
			if (response.has_member("errors") and response["errors"].array())
			{
				auto error_array = response["errors"];
				for (int i = 0; i < error_array.size(); i++)
				{
					validation_error ve;
					json err = error_array.get_element(i);
					ve.put_json(err);
					errors.push_back(ve);
				}
            }
		}

		corona_client_response& operator = (json& response)
		{
			set(response);
			return *this;
		}

		corona_client_response& operator = (http_params& _params)
		{
			json_parser jp;
			json response;

			if (_params.response.response_body.is_safe_string()) {
				// read the response body
				response = jp.parse_object(_params.response.response_body.get_ptr());
			}
			else {
				response = jp.create_object();
			}

			set(response);
			return *this;
		}

	};

	class corona_client_interface
	{
	public:

		virtual corona_client_response register_user(json _user) = 0;
		virtual corona_client_response confirm_user(std::string user_name, std::string confirmation_code) = 0;
		virtual corona_client_response send_user(std::string user_name) = 0;
		virtual corona_client_response login(std::string _user_name, std::string _password) = 0;
		virtual corona_client_response login() = 0;
		virtual corona_client_response set_password(std::string user_name, std::string validation_code, std::string password1, std::string password2) = 0;
		virtual corona_client_response get_classes() = 0;
		virtual corona_client_response get_class(std::string class_name) = 0;
		virtual corona_client_response put_class(json _class_definition) = 0;
		virtual corona_client_response create_object(std::string _class_name) = 0;
		virtual corona_client_response edit_object(std::string _class_name, int64_t _object_id, bool _object_id_field) = 0;
		virtual corona_client_response get_object(std::string _class_name, int64_t _object_id, bool _include_children) = 0;
		virtual corona_client_response put_object(json _object) = 0;
		virtual corona_client_response run_object(json _object) = 0;
		virtual corona_client_response delete_object(std::string _class_name, int64_t _object_id) = 0;
		virtual corona_client_response query_objects(json _query) = 0;
	};
}
