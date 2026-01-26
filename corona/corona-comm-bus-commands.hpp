#pragma once 

/*
CORONA
C++ Low Code Performance Applications for Windows SDK
by Todd Bandrowky
(c) 2024 + All Rights Reserved

MIT License

About this File
This is the application commands. commands can be sprinkled throughout the ui
to provide a declarative way to handle certain ui use cases. generally 
attached to buttons but can be wired up to just about anything.
conceptually similar to winui commands, but a tad more flexible.
thread / ui thread syncing handled for you.  most of the time.

Notes

For Future Consideration
*/

#pragma once


namespace corona
{

	void put_json(std::shared_ptr<corona_bus_command>& _dest, json _src);


	void get_json(json& _dest, const corona_instance& _src)
	{
		switch (_src)
		{
		case corona_instance::local:
			_dest.put_member("instance", std::string("local"));
			break;
		case corona_instance::remote:
			_dest.put_member("instance", std::string("remote"));
			break;
		}
	}

	void put_json(corona_instance& _dest, json& _src)
	{
		json member = _src["instance"];
		_dest = corona_instance::local;
		if (!member.empty())
		{
			std::string value = member.as_string();
			if (_stricmp(value.c_str(), "local") == 0) {
				_dest = corona_instance::local;
			}
			else if (_stricmp(value.c_str(), "remote") == 0) {
				_dest = corona_instance::remote;
			}
			else if (_stricmp(value.c_str(), "1") == 0) {
				_dest = corona_instance::local;
			}
			else if (_stricmp(value.c_str(), "0") == 0) {
				_dest = corona_instance::remote;
			}
		}
	}

	class corona_form_command : public corona_bus_command
	{
	public:

		corona_instance instance = corona_instance::local;

		std::string form_name;
		std::string success_message_field;
		std::string status_message_field;
		std::string execution_time_field;
		std::string error_table_field;

		corona_client_response response;

		std::shared_ptr<corona_bus_command> on_start;
		std::shared_ptr<corona_bus_command> on_success;
		std::shared_ptr<corona_bus_command> on_fail;

		corona_form_command() 
		{
			;
		}

		virtual json create_request(comm_bus_app_interface* _bus) {
			json_parser jp;

			return jp.create_object();
		}

		virtual corona_client_response execute_request(json request, comm_bus_app_interface* _bus)
		{
			corona_client_response response = {};
			return response;
		}

		virtual json handle_response(corona_client_response response, comm_bus_app_interface* _bus)
		{
			return response.data;
		}

		virtual corona_client_response& set_message(corona_client_response& _src, comm_bus_app_interface* _bus);

		virtual json execute(json context, comm_bus_app_interface* bus)
		{
			json obj = bus->get_form_data(form_name);

			if (obj.object()) {
                if (on_start) {
                    on_start->execute(context, bus);
                }	
				auto request = create_request(bus);

				if (request.object()) {
					bus->run([this, bus, request]()->void {
                        corona_client_response response = execute_request(request, bus);

						bus->run_ui([this, response, bus]()->void {
							handle_response(response, bus);
							});
					});
				}
			}
			else {
				log_warning("Could not find form '" + form_name + "'");
			}
			return obj;
		}

		virtual void get_json(json& _dest)
		{
			using namespace std::literals;

			json_parser jp;
			if (on_start) {
				json jon_start = jp.create_object();
				on_start->get_json(jon_start);
				_dest.put_member("on_start", jon_start);
			}
			if (on_success) {
				json jon_login_success = jp.create_object();
				on_success->get_json(jon_login_success);
				_dest.put_member("on_success", jon_login_success);
			}
			if (on_fail) {
				json jon_login_fail = jp.create_object();
				on_fail->get_json(jon_login_fail);
				_dest.put_member("on_fail", jon_login_fail);
			}
			_dest.put_member("form_name", form_name);
			_dest.put_member("success_message_field", success_message_field);
			_dest.put_member("status_message_field", status_message_field);
			_dest.put_member("error_table_field", error_table_field);
			_dest.put_member("execution_time_field", execution_time_field);
			
			corona::get_json(_dest, instance);
		}

		virtual void put_json(json& _src)
		{
			std::vector<std::string> missing;

			if (not _src.has_members(missing, {  })) {
				system_monitoring_interface::active_mon->log_warning("corona_command missing:");
				std::for_each(missing.begin(), missing.end(), [](const std::string& s) {
					system_monitoring_interface::active_mon->log_warning(s);
					});
				system_monitoring_interface::active_mon->log_information("the source json is:");
				system_monitoring_interface::active_mon->log_json<json>(_src, 2);
				return;
			}

			on_start = nullptr;

			json jon_start = _src["on_start"];
			if (jon_start.object()) {
				corona::put_json(on_start, jon_start);
			}
			json jon_login_success = _src["on_success"];
			if (jon_login_success.object()) {
				corona::put_json(on_success, jon_login_success);
			}
			json jon_login_fail = _src["on_fail"];
			if (jon_login_fail.object()) {
				corona::put_json(on_fail, jon_login_fail);
			}
			form_name = _src["form_name"].as_string();
			success_message_field = _src["success_message_field"].as_string();
			status_message_field = _src["status_message_field"].as_string();
			error_table_field = _src["error_table_field"].as_string();
			execution_time_field = _src["execution_time_field"].as_string();

			corona::put_json(instance, _src);

		}

	};

	class  corona_register_user_command : public corona_form_command
	{
	public:

		std::string user_name_field = "";
		std::string email_field = "";
		std::string password1_field = "";
		std::string password2_field = "";
		std::string first_name_field = "";
		std::string last_name_field = "";
		std::string street_field = "";
		std::string city_field = "";
		std::string state_field = "";
		std::string zip_field = "";
		std::string phone_field = "";

		corona_register_user_command()
		{
			;
		}

		virtual std::string get_name()
		{
			return "register_user";
		}

		virtual json create_request(comm_bus_app_interface* _bus)
		{
			json_parser jp;

			json obj = _bus->get_form_data(form_name);
			json user_obj = jp.create_object();

			user_obj.put_member("user_name", obj[user_name_field].as_string());
			user_obj.put_member("email", obj[email_field].as_string());
			user_obj.put_member("password1", obj[password1_field].as_string());
			user_obj.put_member("password2", obj[password2_field].as_string());
			user_obj.put_member("first_name", obj[first_name_field].as_string());
			user_obj.put_member("street", obj[street_field].as_string());
			user_obj.put_member("city", obj[city_field].as_string());
			user_obj.put_member("state", obj[state_field].as_string());
			user_obj.put_member("phone", obj[phone_field].as_string());

            return user_obj;

		}

		virtual corona_client_response execute_request(json request, comm_bus_app_interface* _bus)
		{
			return _bus->register_user(instance, request);
		}

		virtual json handle_response(corona_client_response response, comm_bus_app_interface* _bus) {
			return response.data;
		}

		virtual void get_json(json& _dest)
		{
			using namespace std::literals;

			corona_form_command::get_json(_dest);

			_dest.put_member("class_name", "register_user"sv);
			_dest.put_member("user_name_field", user_name_field);
			_dest.put_member("email_field", email_field);
			_dest.put_member("password1_field", password1_field);
			_dest.put_member("password2_field", password2_field);
			_dest.put_member("first_name_field", first_name_field);
			_dest.put_member("last_name_field", last_name_field);
			_dest.put_member("street_field", street_field);
			_dest.put_member("zip_field", zip_field);
			_dest.put_member("city_field", city_field);
			_dest.put_member("phone_field", phone_field);
		}

		virtual void put_json(json& _src)
		{
			corona_form_command::put_json(_src);

			std::vector<std::string> missing;
			if (not _src.has_members(missing, { "user_name_field", "email_field", "password1_field", "password2_field"})) {
				system_monitoring_interface::active_mon->log_warning("register_user_command missing:");
				std::for_each(missing.begin(), missing.end(), [](const std::string& s) {
					system_monitoring_interface::active_mon->log_warning(s);
					});
				system_monitoring_interface::active_mon->log_information("the source json is:");
				system_monitoring_interface::active_mon->log_json<json>(_src, 2);
				return;
			}

			user_name_field = _src["user_name_field"].as_string();
			email_field = _src["email_field"].as_string();
			password1_field = _src["password1_field"].as_string();
			password2_field = _src["password2_field"].as_string();
			first_name_field = _src["first_name_field"].as_string();
			last_name_field = _src["last_name_field"].as_string();
			street_field = _src["password2_field"].as_string();
			city_field = _src["password2_field"].as_string();

		}

	};

	class  corona_confirm_user_command : public corona_form_command
	{
	public:

		std::string user_name_field = "";
		std::string validation_code_field = "";

		corona_confirm_user_command()
		{
			;
		}

		virtual std::string get_name()
		{
			return "confirm_user";
		}

		virtual json create_request(comm_bus_app_interface* _bus)
		{
			json_parser jp;

			json obj = _bus->get_form_data(form_name);

			return obj;
		}

		virtual corona_client_response execute_request(json request, comm_bus_app_interface* _bus)
		{
			return _bus->confirm_user(instance, request[user_name_field].as_string(), request[validation_code_field].as_string());
		}

		virtual json handle_response(corona_client_response response, comm_bus_app_interface* _bus) {
			return response.data;
		}

		virtual void get_json(json& _dest)
		{
			using namespace std::literals;

            corona_form_command::get_json(_dest);

			_dest.put_member("class_name", "confirm_user"sv);
			_dest.put_member("user_name_field", user_name_field);
			_dest.put_member("validation_code_field", validation_code_field);
		}

		virtual void put_json(json& _src)
		{
			std::vector<std::string> missing;

            corona_form_command::put_json(_src);

			if (not _src.has_members(missing, { "user_name_field", "validation_code_field" })) {
				system_monitoring_interface::active_mon->log_warning("confirm_user_command missing:");
				std::for_each(missing.begin(), missing.end(), [](const std::string& s) {
					system_monitoring_interface::active_mon->log_warning(s);
					});
				system_monitoring_interface::active_mon->log_information("the source json is:");
				system_monitoring_interface::active_mon->log_json<json>(_src, 2);
				return;
			}

			user_name_field = _src["user_name_field"].as_string();
			validation_code_field = _src["validation_code_field"].as_string();
		}

	};

	class  corona_send_user_command : public corona_form_command
	{
	public:

		std::string user_name_field = "";

		corona_send_user_command()
		{
			;
		}

		virtual std::string get_name()
		{
			return "send_user";
		}


		virtual json create_request(comm_bus_app_interface* _bus)
		{
			json_parser jp;

			json obj = _bus->get_form_data(form_name);

			return obj;
		}

		virtual corona_client_response execute_request(json request, comm_bus_app_interface* _bus)
		{
			return _bus->send_user(instance, request[user_name_field].as_string());
		}

		virtual json handle_response(corona_client_response response, comm_bus_app_interface* _bus) {
			return response.data;
		}

		virtual void get_json(json& _dest)
		{
			using namespace std::literals;

			corona_form_command::get_json(_dest);

			_dest.put_member("class_name", "send_user"sv);
			_dest.put_member("user_name_field", user_name_field);
		}

		virtual void put_json(json& _src)
		{

			corona_form_command::put_json(_src);

			std::vector<std::string> missing;
			if (not _src.has_members(missing, { "user_name_field"})) {
				system_monitoring_interface::active_mon->log_warning("send_user_command missing:");
				std::for_each(missing.begin(), missing.end(), [](const std::string& s) {
					system_monitoring_interface::active_mon->log_warning(s);
					});
				system_monitoring_interface::active_mon->log_information("the source json is:");
				system_monitoring_interface::active_mon->log_json<json>(_src, 2);
				return;
			}

			user_name_field = _src["user_name_field"].as_string();	
		}

	};

	class  corona_password_user_command : public corona_form_command
	{
	public:

		std::string user_name_field = "";
        std::string validation_code_field = "";
		std::string password1_field = "";
		std::string password2_field = "";

		corona_password_user_command()
		{
			;
		}

		virtual std::string get_name()
		{
			return "password_user";
		}

		virtual json create_request(comm_bus_app_interface* _bus)
		{
			json_parser jp;

			json obj = _bus->get_form_data(form_name);

			return obj;
		}

		virtual corona_client_response execute_request(json request, comm_bus_app_interface* _bus)
		{
			std::string user_name = request[user_name_field].as_string();
			std::string validation_code = request[validation_code_field].as_string();
			std::string password1 = request[password1_field].as_string();
			std::string password2 = request[password2_field].as_string();
			return _bus->set_password(instance, user_name, validation_code, password1, password2);
		}

		virtual json handle_response(corona_client_response response, comm_bus_app_interface* _bus) {
			return response.data;
		}

		virtual void get_json(json& _dest)
		{
			using namespace std::literals;

			corona_form_command::get_json(_dest);

			_dest.put_member("class_name", "password_user"sv);
			_dest.put_member("user_name_field", user_name_field);
			_dest.put_member("validation_code_field", validation_code_field);
			_dest.put_member("password1_field", password1_field);
			_dest.put_member("password2_field", password2_field);
		}

		virtual void put_json(json& _src)
		{

			corona_form_command::put_json(_src);

			std::vector<std::string> missing;
			if (not _src.has_members(missing, { "user_name_field", "validation_code_field", "password1_field", "password2_field"})) {
				system_monitoring_interface::active_mon->log_warning("set_user_password missing:");
				std::for_each(missing.begin(), missing.end(), [](const std::string& s) {
					system_monitoring_interface::active_mon->log_warning(s);
					});
				system_monitoring_interface::active_mon->log_information("the source json is:");
				system_monitoring_interface::active_mon->log_json<json>(_src, 2);
				return;
			}

			user_name_field = _src["user_name_field"].as_string();
			validation_code_field = _src["validation_code_field"].as_string();
			password1_field = _src["password1_field"].as_string();
			password2_field = _src["password2_field"].as_string();
		}

	};

	class  corona_login_command : public corona_form_command
	{
	public:

		std::string user_name_field = "";
		std::string user_password_field = "";

		corona_login_command()
		{
			;
		}

		virtual std::string get_name()
		{
			return "login_user";
		}

		virtual json create_request(comm_bus_app_interface* _bus)
		{
			json_parser jp;

			json obj = _bus->get_form_data(form_name);

			return obj;
		}

		virtual corona_client_response execute_request(json request, comm_bus_app_interface* _bus)
		{
			std::string user_name = request[user_name_field].as_string();
			std::string password = request[user_password_field].as_string();
			return _bus->login(instance, user_name, password);
		}

		virtual json handle_response(corona_client_response response, comm_bus_app_interface* _bus) {
			return response.data;
		}

		virtual void get_json(json& _dest)
		{
			using namespace std::literals;

			corona_form_command::get_json(_dest);

			_dest.put_member("class_name", "login_user"sv);
			_dest.put_member("user_name_field", user_name_field);
			_dest.put_member("user_password_field", user_password_field);

		}

		virtual void put_json(json& _src)
		{
			std::vector<std::string> missing;

			corona_form_command::put_json(_src);

			if (not _src.has_members(missing, { "form_name", "user_name_field", "user_password_field" })) {
				system_monitoring_interface::active_mon->log_warning("login_user missing:");
				std::for_each(missing.begin(), missing.end(), [](const std::string& s) {
					system_monitoring_interface::active_mon->log_warning(s);
					});
				system_monitoring_interface::active_mon->log_information("the source json is:");
				system_monitoring_interface::active_mon->log_json<json>(_src, 2);
				return;
			}

			user_name_field = _src["user_name_field"].as_string();
			user_password_field = _src["user_password_field"].as_string();

		}

	};

	class corona_class_page_map 
	{
	public:

		std::map<std::string, std::string> pages_by_class;

		virtual void get_json(json& _dest)
		{
			using namespace std::literals;
			_dest.put_member("class_name", "class_page_map"sv);
			json_parser jp;
            json jclasses = jp.create_object();
			for (auto &cd : pages_by_class) {
				jclasses.put_member(cd.first, cd.second);
			}
            _dest.put_member("sources", jclasses);
		}

		virtual void put_json(json& _src)
		{
			std::vector<std::string> missing;

			if (not _src.has_members(missing, { "sources" })) {
				system_monitoring_interface::active_mon->log_warning("sources missing:");
				std::for_each(missing.begin(), missing.end(), [](const std::string& s) {
					system_monitoring_interface::active_mon->log_warning(s);
					});
				system_monitoring_interface::active_mon->log_information("the source json is:");
				system_monitoring_interface::active_mon->log_json<json>(_src, 2);
				return;
			}

            json jclasses = _src["sources"];
            auto class_members = jclasses.get_members();
            for (auto cm : class_members) {
				pages_by_class[ cm.first ] = cm.second.as_string();
			}
		}
	};

	class  corona_get_classes_command : public corona_form_command
	{
	public:
		std::string	table_name = "";
		std::shared_ptr<corona_class_page_map> target;

		corona_get_classes_command()
		{
			;
		}

		virtual std::string get_name()
		{
			return "get_classes";
		}

		virtual json create_request(comm_bus_app_interface* _bus)
		{
			json_parser jp;

			json obj = _bus->get_form_data(form_name);

			return obj;
		}

		virtual corona_client_response execute_request(json request, comm_bus_app_interface* _bus)
		{
			corona_client_response response = _bus->get_classes(instance);
			return response;
		}

		virtual json handle_response(corona_client_response response, comm_bus_app_interface* _bus) {
			control_base* cb_table = {};

			if (not table_name.empty())
				cb_table = _bus->find_control(table_name);

			if (not cb_table) {
				comm_bus_app_interface::global_bus->log_warning(std::format("{0} table for search command not found", table_name), __FILE__, __LINE__);
			}

			json_parser jp;
			json results = jp.create_array();
			if (response.data.array()) {
				for (auto cls : response.data) {
					json item = jp.create_object();
					std::string cls_name = cls[class_name_field].as_string();
					std::string cls_description = cls["class_description"].as_string();
					std::string cls_base = cls[base_class_name_field].as_string();
					item.put_member(base_class_name_field, cls_base);
					item.put_member(class_name_field, cls_name);
					item.put_member("class_description", cls_description);
					results.push_back(item);
				}
			}
			cb_table->set_items(results);

			return response.data;
		}

		virtual void get_json(json& _dest)
		{
			using namespace std::literals;

			_dest.put_member("class_name", "get_classes"sv);
			_dest.put_member("table_name", table_name);
		}

		virtual void put_json(json& _src)
		{
			std::vector<std::string> missing;
			if (not _src.has_members(missing, { })) {
				system_monitoring_interface::active_mon->log_warning("get_classes_command missing:");
				std::for_each(missing.begin(), missing.end(), [](const std::string& s) {
					system_monitoring_interface::active_mon->log_warning(s);
					});
				system_monitoring_interface::active_mon->log_information("the source json is:");
				system_monitoring_interface::active_mon->log_json<json>(_src, 2);
				return;
			}

			table_name = _src["table_name"].as_string();
		}
	};

	class  corona_get_class_command : public corona_form_command
	{
	public:

		corona_get_class_command()
		{
			;
		}

		virtual json create_request(comm_bus_app_interface* _bus)
		{
			json_parser jp;

			json obj = _bus->get_form_data(form_name);

			return obj;
		}

		virtual corona_client_response execute_request(json request, comm_bus_app_interface* _bus)
		{
			std::string user_name = request[user_name_field].as_string();
			std::string password = request[user_password_field].as_string();
			return _bus->login(instance, user_name, password);
		}

		virtual json handle_response(corona_client_response response, comm_bus_app_interface* _bus) {
			return response.data;
		}

		virtual std::string get_name()
		{
			return "get_class";
		}

	};

	class  corona_put_class_command : public corona_form_command
	{
	public:
		std::shared_ptr<corona_class_page_map> source;

		corona_put_class_command()
		{
			;
		}

		virtual json create_request(comm_bus_app_interface* _bus)
		{
			json_parser jp;

			json obj = _bus->get_form_data(form_name);

			return obj;
		}

		virtual corona_client_response execute_request(json request, comm_bus_app_interface* _bus)
		{
			return _bus->put_class(instance, request);
		}

		virtual json handle_response(corona_client_response response, comm_bus_app_interface* _bus) {
			return response.data;
		}

		virtual std::string get_name()
		{
			return "put_class";
		}
	};

	class corona_create_object_command : public corona_form_command
	{
	public:
		std::string	create_class_name = "";
		std::string target;
		corona_client_response response;


		corona_create_object_command()
		{
			;
		}

		virtual std::string get_name()
		{
			return "create_object";
		}

		virtual json create_request(comm_bus_app_interface* _bus)
		{
			json_parser jp;

			json obj = _bus->get_form_data(form_name);

			return obj;
		}

		virtual corona_client_response execute_request(json request, comm_bus_app_interface* _bus)
		{
			auto response = _bus->create_object(instance, create_class_name);
			return response;
		}

		virtual json handle_response(corona_client_response response, comm_bus_app_interface* _bus) {
			return response.data;
		}

		virtual void get_json(json& _dest)
		{
			using namespace std::literals;

            corona_form_command::get_json(_dest);

			_dest.put_member("class_name", "create_object_frame"sv);
			_dest.put_member("create_class_name", create_class_name);
			_dest.put_member("target", target);
		}

		virtual void put_json(json& _src)
		{
			corona_form_command::put_json(_src);

			std::vector<std::string> missing;
			if (not _src.has_members(missing, { "create_class_name", "form_name" })) {
				system_monitoring_interface::active_mon->log_warning("create_object_command missing:");
				std::for_each(missing.begin(), missing.end(), [](const std::string& s) {
					system_monitoring_interface::active_mon->log_warning(s);
					});
				system_monitoring_interface::active_mon->log_information("the source json is:");
				system_monitoring_interface::active_mon->log_json<json>(_src, 2);
				return;
			}

			create_class_name = _src["create_class_name"].as_string();
			target = _src["target"].as_string();

		}

	};

	class corona_list_select_object_command : public corona_form_command
	{
	public:
		std::string		table_name = "";
		std::string     target_frame = "";

		std::shared_ptr<corona_class_page_map> sources;

		corona_instance instance = corona_instance::local;

		corona_list_select_object_command()
		{
			;
		}

		virtual std::string get_name()
		{
			return "list_select_object";
		}

		virtual json create_request(comm_bus_app_interface* _bus)
		{
			json_parser jp;

			control_base* cb = _bus->find_control(table_name);
			if (cb) {
				json key_data = cb->get_selected_object();
				return key_data;
			}
			return jp.create_object();
		}

		virtual corona_client_response execute_request(json request, comm_bus_app_interface* _bus)
		{
			auto response = _bus->edit_object(instance, request);
			return response;
		}

		virtual json handle_response(corona_client_response response, comm_bus_app_interface* _bus) {
			if (response.success) {
				std::string class_name = response.data["object"][class_name_field].as_string();
				auto source_it = sources->pages_by_class.find(class_name);
				if (source_it != sources->pages_by_class.end()) {
					std::string source_form = source_it->second;
					_bus->select_frame(target_frame, source_form, response.data);
				}
				response.message = "No target destination found for class '" + class_name;
				response.success = false;
				return response.data;
			}
		}

		virtual void get_json(json& _dest)
		{
			using namespace std::literals;

			_dest.put_member("class_name", "list_select_object"sv);
			_dest.put_member("table_name", table_name);
			_dest.put_member("target_frame", target_frame);
			if (sources) {
				sources->get_json(_dest);
            }
			corona::get_json(_dest, instance);
		}

		virtual void put_json(json& _src)
		{
			std::vector<std::string> missing;
			if (not _src.has_members(missing, { "sources", "table_name"})) {
				system_monitoring_interface::active_mon->log_warning("list_select_object missing:");
				std::for_each(missing.begin(), missing.end(), [](const std::string& s) {
					system_monitoring_interface::active_mon->log_warning(s);
					});
				system_monitoring_interface::active_mon->log_information("the source json is:");
				system_monitoring_interface::active_mon->log_json<json>(_src, 2);
				return;
			}

            target_frame = _src["target_frame"].as_string();
			table_name = _src["table_name"].as_string();
            if (_src.has_member("sources")) {
				sources = std::make_shared<corona_class_page_map>();
				sources->put_json(_src);
			}
			corona::put_json(instance, _src);
		}
	};

	class corona_save_object_command : public corona_form_command
	{
	public:

		corona_instance instance = corona_instance::local;

		corona_save_object_command()
		{
			;
		}

		virtual std::string get_name()
		{
			return "save_object";
		}

		virtual json create_request(comm_bus_app_interface* _bus)
		{
			json_parser jp;

			control_base* cb = _bus->find_control(form_name);
			if (cb) {
				json key_data = cb->get_data();
				return key_data;
			}
			return jp.create_object();
		}

		virtual corona_client_response execute_request(json request, comm_bus_app_interface* _bus)
		{
			auto response = _bus->put_object(instance, request);
			return response;
		}

		virtual json handle_response(corona_client_response response, comm_bus_app_interface* _bus) {
			return response.data;
		}

		virtual void get_json(json& _dest)
		{
			using namespace std::literals;

			corona_form_command::get_json(_dest);

			_dest.put_member("class_name", "save_object"sv);
			_dest.put_member("form_name", form_name);
			corona::get_json(_dest, instance);

		}

		virtual void put_json(json& _src)
		{

			corona_form_command::put_json(_src);

			form_name = _src["form_name"].as_string();
			std::vector<std::string> missing;
			if (not _src.has_members(missing, { "form_name" })) {
				system_monitoring_interface::active_mon->log_warning("save_object_command missing:");
				std::for_each(missing.begin(), missing.end(), [](const std::string& s) {
					system_monitoring_interface::active_mon->log_warning(s);
					});
				system_monitoring_interface::active_mon->log_information("the source json is:");
				system_monitoring_interface::active_mon->log_json<json>(_src, 2);
				return;
			}
			corona::put_json(instance, _src	);
		}

	};

	class corona_load_object_command : public corona_form_command
	{
	public:
		std::string control_name = "";
		json		object_data;
		corona_instance instance = corona_instance::local;

		corona_load_object_command()
		{
			;
		}

		virtual std::string get_name()
		{
			return "load_object";
		}

		virtual json create_request(comm_bus_app_interface* _bus)
		{
			json_parser jp;

			json obj = _bus->get_form_data(form_name);

			return obj;
		}

		virtual corona_client_response execute_request(json request, comm_bus_app_interface* _bus)
		{
			auto response = _bus->get_object(instance, request);
			return response;
		}

		virtual json handle_response(corona_client_response response, comm_bus_app_interface* _bus) {
			return response.data;
		}


		virtual void get_json(json& _dest)
		{
			using namespace std::literals;

			_dest.put_member("class_name", "load_object"sv);
			_dest.put_member("control_name", control_name);
			_dest.put_member("data", object_data);
			corona::get_json(_dest, instance);

		}

		virtual void put_json(json& _src)
		{
			std::vector<std::string> missing;
			if (not _src.has_members(missing, { "control_name", "data" })) {
				system_monitoring_interface::active_mon->log_warning("load_object_command missing:");
				std::for_each(missing.begin(), missing.end(), [](const std::string& s) {
					system_monitoring_interface::active_mon->log_warning(s);
					});
				system_monitoring_interface::active_mon->log_information("the source json is:");
				system_monitoring_interface::active_mon->log_json<json>(_src, 2);
				return;
			}
			control_name = _src["control_name"].as_string();
			object_data = _src["data"];
			corona::put_json(instance, _src);
		}

	};

	class corona_delete_object_command : public corona_form_command
	{
	public:
		std::string		control_name = "";
		corona_instance instance = corona_instance::local;

		corona_delete_object_command()
		{
			;
		}

		virtual std::string get_name()
		{
			return "delete_object";
		}

		virtual json create_request(comm_bus_app_interface* _bus)
		{
			json_parser jp;

			json obj = _bus->get_form_data(form_name);

			return obj;
		}

		virtual corona_client_response execute_request(json request, comm_bus_app_interface* _bus)
		{
			auto response = _bus->delete_object(instance, request);
			return response;
		}

		virtual json handle_response(corona_client_response response, comm_bus_app_interface* _bus) {
			return response.data;
		}

		virtual void get_json(json& _dest)
		{
			using namespace std::literals;
			_dest.put_member("class_name", "delete_object"sv);
			_dest.put_member("control_name", control_name);
			corona::get_json(_dest, instance);

		}

		virtual void put_json(json& _src)
		{
			std::vector<std::string> missing;
			if (not _src.has_members(missing, { "class_name", "control_name" })) {
				system_monitoring_interface::active_mon->log_warning("delete_object_command missing:");
				std::for_each(missing.begin(), missing.end(), [](const std::string& s) {
					system_monitoring_interface::active_mon->log_warning(s);
					});
				system_monitoring_interface::active_mon->log_information("the source json is:");
				system_monitoring_interface::active_mon->log_json<json>(_src, 2);
				return;
			}

			control_name = _src["control_name"].as_string();
			corona::put_json(instance, _src);
		}

	};

	class corona_run_object_command : public corona_form_command
	{
	public:
		std::string			search_class_name = "";
		std::string			form_name = "";
		std::string			table_name = "";
		query_context		qctx;
		corona_instance instance = corona_instance::local;

		corona_run_object_command()
		{
			;
		}

		virtual std::string get_name()
		{
			return "run_object";
		}


		virtual json create_request(comm_bus_app_interface* _bus)
		{
			json_parser jp;

			json obj = _bus->get_form_data(form_name);

			return obj;
		}

		virtual corona_client_response execute_request(json request, comm_bus_app_interface* _bus)
		{
			auto response = _bus->run_object(instance, request);
			return response;
		}

		virtual json handle_response(corona_client_response response, comm_bus_app_interface* _bus) {
			return response.data;
		}


		virtual void get_json(json& _dest)
		{
			using namespace std::literals;

			_dest.put_member("class_name", "search_objects"sv);
			_dest.put_member("search_class_name", search_class_name);
			_dest.put_member("form_name", form_name);
			_dest.put_member("table_name", table_name);
			json_parser jp;
			json jctx = jp.create_object();
			qctx.get_json(jctx);
			_dest.put_member("query", jctx);
			corona::get_json(_dest, instance);

		}

		virtual void put_json(json& _src)
		{
			std::vector<std::string> missing;

			if (not _src.has_members(missing, { "form_name", "table_name", "search_class_name" })) {
				system_monitoring_interface::active_mon->log_warning("search_objects_command missing:");
				std::for_each(missing.begin(), missing.end(), [](const std::string& s) {
					system_monitoring_interface::active_mon->log_warning(s);
					});
				system_monitoring_interface::active_mon->log_information("the source json is:");
				system_monitoring_interface::active_mon->log_json<json>(_src, 2);
				return;
			}

			form_name = _src["form_name"].as_string();
			table_name = _src["table_name"].as_string();
			search_class_name = _src["search_class_name"].as_string();
			json jctx = _src["query"];
			qctx.put_json(jctx);
			corona::put_json(instance, _src);
		}

	};

	class corona_query_command : public corona_form_command
	{
	public:
		std::string			table_name = "";
		json				query_body;
		corona_instance instance = corona_instance::local;

		corona_query_command()
		{
			;
		}

		virtual std::string get_name()
		{
			return "query_object";
		}


		virtual json create_request(comm_bus_app_interface* _bus)
		{
			json_parser jp;
			control_base* cb_form = {};
			json object_data;

			if (not form_name.empty())
				cb_form = _bus->find_control(form_name);

			if (not cb_form) {
				comm_bus_app_interface::global_bus->log_warning(std::format("{0} form for search command not found", form_name), __FILE__, __LINE__);
			}

			if (cb_form)
			{
				object_data = cb_form->get_data();
			}

			return object_data;
		}

		virtual corona_client_response execute_request(json request, comm_bus_app_interface* _bus)
		{
			corona_client_response response;
			json_parser jp;
			if (request.object()) {

				json this_query_body = query_body.clone();
				this_query_body.put_member("include_children", false);

				json froms = this_query_body["from"];
				if (froms.array()) {
					// this adds us as an input source
					json new_from = jp.create_object();
					json thisobj = request.clone();
					new_from.put_member(data_field, thisobj);
					new_from.put_member(class_name_field, std::string("sys_object"));
					new_from.put_member("name", std::string("this"));
					auto arr = froms.array_impl();
					arr->elements.insert(arr->elements.begin(), new_from.value());
				}
				response = _bus->query(instance, this_query_body);
			}
			return response;
		}

		virtual json handle_response(corona_client_response response, comm_bus_app_interface* _bus) {
			control_base* cb_table = {};
			if (not table_name.empty())
				cb_table = _bus->find_control(table_name);
			if (not cb_table) {
				comm_bus_app_interface::global_bus->log_warning(std::format("{0} table for search command not found", table_name), __FILE__, __LINE__);
			}
			if (response.success) {
				if (cb_table) {
					cb_table->set_items(response.data);
				}
			}
			return response.data;
		}

		virtual void get_json(json& _dest)
		{
			using namespace std::literals;

			_dest.put_member("class_name", "query"sv);
			_dest.put_member("form_name", form_name);
			_dest.put_member("table_name", table_name);
			_dest.put_member("query", query_body);
			corona::get_json(_dest, instance);

		}

		virtual void put_json(json& _src)
		{
			std::vector<std::string> missing;

			if (not _src.has_members(missing, { "form_name", "table_name", "query"})) {
				system_monitoring_interface::active_mon->log_warning("search_objects_command missing:");
				std::for_each(missing.begin(), missing.end(), [](const std::string& s) {
					system_monitoring_interface::active_mon->log_warning(s);
					});
				system_monitoring_interface::active_mon->log_information("the source json is:");
				system_monitoring_interface::active_mon->log_json<json>(_src, 2);
				return;
			}

			form_name = _src["form_name"].as_string();
			table_name = _src["table_name"].as_string();
			query_body = _src["query"];
			corona::put_json(instance, _src);
		}

	};

	class corona_select_frame_command : public corona_form_command
	{
	public:
		std::string		target_frame;
		std::string		source_frame;

		corona_select_frame_command()
		{
			;
		}

		virtual std::string get_name()
		{
			return "select_frame";
		}

		virtual json execute(json context, comm_bus_app_interface* bus)
		{
			json_parser jp;
			json obj = jp.create_object();

			bus->select_frame(target_frame, source_frame, obj);

			return obj;
		}

		virtual void get_json(json& _dest)
		{
			using namespace std::literals;

			_dest.put_member("class_name", "select_frame"sv);
			_dest.put_member("target_frame", target_frame);
			_dest.put_member("source_frame", source_frame);

		}

		virtual void put_json(json& _src)
		{
			std::vector<std::string> missing;
			if (not _src.has_members(missing, { "target_frame" })) {
				system_monitoring_interface::active_mon->log_warning("select_frame_command missing:");
				std::for_each(missing.begin(), missing.end(), [](const std::string& s) {
					system_monitoring_interface::active_mon->log_warning(s);
					});
				system_monitoring_interface::active_mon->log_information("the source json is:");
				system_monitoring_interface::active_mon->log_json<json>(_src, 2);
				return;
			}

			target_frame = _src["target_frame"].as_string();
			source_frame = _src["source_frame"].as_string();
		}
	};

	class corona_script_command: public corona_form_command
	{
	public:
		std::string		control_name = "";
		std::vector<std::shared_ptr<corona_bus_command>> commands;

		corona_script_command()
		{
			;
		}

		virtual std::string get_name()
		{
			return "script";
		}

		virtual json execute(json context,  comm_bus_app_interface* bus)
		{
			json obj;
			for (auto comm : commands) {
				obj = comm->execute(context, bus);
			}
			return obj;
		}

		virtual void get_json(json& _dest)
		{
			using namespace std::literals;

			_dest.put_member("class_name", "script"sv);
			json_parser jp;
			json jcommand_array = jp.create_array();

			for (auto comm : commands)
			{
				json jcomm = jp.create_object();
				comm->get_json(jcomm);
				jcommand_array.push_back(jcomm);
			}

			_dest.put_member("control_name", control_name);
			_dest.put_member("commands", jcommand_array);

		}

		virtual void put_json(json& _src)
		{
			std::vector<std::string> missing;

			if (not _src.has_members(missing, { "control_name", "commands" })) {
				system_monitoring_interface::active_mon->log_warning("script missing:");

				std::for_each(missing.begin(), missing.end(), [](const std::string& s) {
					system_monitoring_interface::active_mon->log_warning(s);
					});
				system_monitoring_interface::active_mon->log_information("the source json is:");
				system_monitoring_interface::active_mon->log_json<json>(_src, 2);
				return;
			}

			control_name = _src["control_name"].as_string();
			commands.clear();

			json_parser jp;
			json jcommand_array = _src["commands"];
			if (jcommand_array.array()) {
				for (auto jcomm : jcommand_array)
				{
					std::shared_ptr<corona_bus_command> comm;
					corona::put_json(comm, jcomm);
					if (not comm)
						break;
					commands.push_back(comm);
				}
			}
		}
	};

	class corona_set_property_command : public corona_form_command
	{
	public:
		std::string		control_name;
		std::string		property_name;
		std::string		value;

		// this is defined in corona-presentation-builder.  
		// should have done this more cleanly with interfaces, but
		// this gets the job done.

		corona_set_property_command()
		{
			;
		}

		virtual std::string get_name()
		{
			return "set_property";
		}

		virtual json execute(json context,  comm_bus_app_interface* bus);
		virtual void get_json(json& _dest)
		{
			using namespace std::literals;

			_dest.put_member("class_name", "set_property"sv);
			_dest.put_member("control_name", control_name);
			_dest.put_member("property_name", property_name);
			_dest.put_member("value", value);
		}

		virtual void put_json(json& _src)
		{
			std::vector<std::string> missing;

			if (not _src.has_members(missing, { "control_name", "property_name"})) {
				system_monitoring_interface::active_mon->log_warning("set_property missing:");

				std::for_each(missing.begin(), missing.end(), [](const std::string& s) {
					system_monitoring_interface::active_mon->log_warning(s);
					});
				system_monitoring_interface::active_mon->log_information("the source json is:");
				system_monitoring_interface::active_mon->log_json<json>(_src, 2);
				return;
			}

			control_name = _src["control_name"].as_string();
			property_name = _src["property_name"].as_string();
			value = _src["value"].as_string();
		}
	};

	void get_json(json& _dest, std::shared_ptr<corona_bus_command>& _src)
	{
		json_parser jp;
		_dest = jp.create_object();
		_src->get_json(_dest);
	}

	void put_json(std::shared_ptr<corona_bus_command>& _dest, json _src)
	{
		std::vector<std::string> missing;

		if (_src.empty())
			return;

		if (not _src.has_members(missing, { "class_name" })) {
			system_monitoring_interface::active_mon->log_warning("command object missing class_name.");
			if (_src.size()) {
				system_monitoring_interface::active_mon->log_information("the source json is:");
				system_monitoring_interface::active_mon->log_json<json>(_src, 2);
			}
			return;
		}

		if (_src.has_member("class_name"))
		{
			std::string class_name = _src["class_name"].as_string();

			if (class_name == "script")
			{
				_dest = std::make_shared<corona_script_command>();
				_dest->put_json(_src);
			}
			else if (class_name == "set_property")
			{
				_dest = std::make_shared<corona_set_property_command>();
				_dest->put_json(_src);
			}
			else if (class_name == "select_frame")
			{
				_dest = std::make_shared<corona_select_frame_command>();
				_dest->put_json(_src);
			}
			else if (class_name == "list_select_object")
			{
				_dest = std::make_shared<corona_list_select_object_command>();
				_dest->put_json(_src);
			}
			else if (class_name == "register_user")
			{
				_dest = std::make_shared<corona_register_user_command>();
				_dest->put_json(_src);
			}
			else if (class_name == "send_user")
			{
				_dest = std::make_shared<corona_send_user_command>();
				_dest->put_json(_src);
			}
			else if (class_name == "password_user")
			{
				_dest = std::make_shared<corona_password_user_command>();
				_dest->put_json(_src);
			}
			else if (class_name == "confirm_user")
			{
				_dest = std::make_shared<corona_confirm_user_command>();
				_dest->put_json(_src);
			}
			else if (class_name == "login_user")
			{
				_dest = std::make_shared<corona_login_command>();
				_dest->put_json(_src);
			}
			else if (class_name == "get_classes")
			{
				_dest = std::make_shared<corona_get_classes_command>();
				_dest->put_json(_src);
			}
			else if (class_name == "get_class")
			{
				_dest = std::make_shared<corona_get_class_command>();
				_dest->put_json(_src);
			}
			else if (class_name == "put_class")
			{
				_dest = std::make_shared<corona_put_class_command>();
				_dest->put_json(_src);
			}
			else if (class_name == "create_object")
			{
				_dest = std::make_shared<corona_create_object_command>();
				_dest->put_json(_src);
			}
			else if (class_name == "save_object")
			{
				_dest = std::make_shared<corona_save_object_command>();
				_dest->put_json(_src);
			}
			else if (class_name == "load_object")
			{
				_dest = std::make_shared<corona_load_object_command>();
				_dest->put_json(_src);
			}
			else if (class_name == "delete_object")
			{
				_dest = std::make_shared<corona_delete_object_command>();
				_dest->put_json(_src);
			}
			else if (class_name == "query")
			{
				_dest = std::make_shared<corona_query_command>();
				_dest->put_json(_src);
			}
		}
	}

	corona_client_response& corona_form_command::set_message(corona_client_response& _src, comm_bus_app_interface* _bus)
	{
		auto dest = std::make_shared<corona_set_property_command>();

		dest->property_name = "text";

		dest->control_name = success_message_field.empty() ? "call_success_message" : status_message_field;
		dest->value = _src.success ? "Success" : "Failure";
		dest->execute(_src.data, _bus);

		dest->control_name = status_message_field.empty() ? "call_status_message" : status_message_field;
		dest->value = _src.message;
		dest->execute(_src.data, _bus);

		dest->control_name = execution_time_field.empty() ? "call_execution_time" : status_message_field;
		dest->value = std::format("{0} secs", _src.execution_time );
		dest->execute(_src.data, _bus);

		std::string table_field = error_table_field.empty() ? "call_error_table" : error_table_field;

		if (not table_field.empty()) {
			auto cb_table = _bus->find_control(table_field);

			if (cb_table) {
				json_parser jp;
				json results = jp.create_array();
				for (auto err : _src.errors) {
					json jerr = jp.create_object();
					err.get_json(jerr);
					results.push_back(jerr);
				}
				cb_table->set_items(results);
			}
		}

		return _src;
	}

}

