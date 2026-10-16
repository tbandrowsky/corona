/*
CORONA
C++ Low Code Performance Applications for Windows SDK
by Todd Bandrowky
(c) 2024 + All Rights Reserved

MIT License

About this File
This is the core database engine for the corona database server.  

*/

#pragma once

/*********************************************** 

Journal

3/14/2024 - Objectives
Create and list teams and users, 
Team permission checks for all objects,
Create 

3/15/2024 - log
Create and edit 

3/28/2024 - 
API close to complete, rounding out for testing.

4/6/2024 -
A good chunk of this is coded, but I am dealing 
with getting the headers straightened out from 
the great module disaster.

9/18/2024 - rewrote most of the above because the design was stupid.
9/25/2024 - factored classes and indexes out to help with concurrency.
10/28/2024 - now with xtable implementation for faster and more efficient storage,
			and querying that works increasingly well.
8/24/2025 - now with xtables, sign on ceremony, 20k puts / sec.  

***********************************************/


namespace corona
{

	
	class corona_database_header_data
	{
	public:
		int64_t object_id;
	};

	class progress_fence
	{
	public:
		std::atomic<int> *calls_in_progress;

		progress_fence(std::atomic<int>* _src) {
			calls_in_progress = _src;
			calls_in_progress->fetch_add(1);
		}
		~progress_fence() {
			calls_in_progress->fetch_sub(1);
		}
	};

	class corona_database : public corona_database_interface
	{
		shared_lockable allocation_lock,
						class_lock,
						database_lock,
						key_lock;
		
        bool			shutdown_in_progress = false;
        std::atomic<int>	calls_in_progress = 0;

		json schema;

		std::map<class_method_key, json_function_function> functions;

		crypto crypter;

        const std::string auth_general = "auth-general"; // this is the general user, which is used for general operations
        const std::string auth_system = "auth-system"; // this is the system user, which is used for system operations
		const std::string auth_self = "auth-self"; // this is the self user, which is used for the case when a user wants his own record

		std::string data_path;
		std::string config_path;

		long import_batch_size = 2500;

		/*
		* authorizations in tokens, methods and progressions
		* 
		create_user->login_user
		login_user->send_confirmation_code, send_password_reset_code
		send_login_confirmation_code->receive_login_confirmation_code
		receive_login_confirmation_code->user - connected
		send_password_reset_code->receive_reset_password_code
		receive_reset_password_code->user - connected

		connected can:
		edit_object
		get_classes
		get_class
		put_class
		query_class
		create_object
		put_object
		get_object
		copy_object
		delete_object
		*/

		std::shared_ptr<xtable> classes;
		bool trace_check_class = false;

		using corona_method_implementation = std::function<json(json obj, std::string user_name, std::string user_auth, validation_error_collection& _errors)>;

		json corona_function(const std::string& _function_name, json request_message, corona_method_implementation fn)
		{
			progress_fence progress(&calls_in_progress);

			timer method_timer;
			json_parser jp;
			json response;

			date_time start_time = date_time::now();
			timer tx;
			system_monitoring_interface::active_mon->log_function_start(_function_name, "start", start_time, __FILE__, __LINE__);

			try {

				validation_error_collection errors;
				std::string user_name, user_auth;

				if (not check_message(request_message, { auth_general }, user_name, user_auth))
				{
					response = create_response(request_message, false, "Denied", jp.create_object(), errors, method_timer.get_elapsed_seconds());
					system_monitoring_interface::active_mon->log_function_stop(_function_name, "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
					return response;
				}

				json data = request_message[data_field];
				if (data.object())
				{
					json result = fn(data, user_name, user_auth, errors);
					response = create_response(request_message, true, "Ok", result, errors, method_timer.get_elapsed_seconds());
				}
				else
				{
					response = create_response(request_message, false, "Not found", data, errors, method_timer.get_elapsed_seconds());
				}
				system_monitoring_interface::active_mon->log_function_stop(_function_name, "complete", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);

			}
			catch (std::exception exc)
			{
				system_monitoring_interface::active_mon->log_exception(exc, __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_function_stop(_function_name, "exception", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
			}
		}

	public:

		virtual void log_errors(validation_error_collection& _errors) override
		{
			for (auto &err : _errors) {
				std::string msg = std::format("{}.{}: {}", err.class_name, err.field_name, err.message);
				system_monitoring_interface::active_mon->log_warning(msg);
				std::string msg2 = std::format("Code Location: @({},{}) , Occurrences:{}", err.filename, err.line_number, err.count);
				system_monitoring_interface::active_mon->log_warning(msg2);
			}
		}

		virtual std::string get_directory()
		{
			return data_path;
		}

		void log_error_array(json& put_result)
		{
			auto result_items = put_result[data_field];
			if (result_items.object()) {
				result_items.for_each_member([](const std::string& _member_name, json _member) {
					if (_member.array()) {
						_member.for_each_element([](json& _item) {
							if (!_item[success_field].as_bool()) {
								std::string msg = std::format("{0}:", _item[message_field].as_string());
								system_monitoring_interface::active_mon->log_warning(msg);
								if (_item.has_member("errors"))
								{
									json errors = _item["errors"];
									errors.for_each_element([](json& _msg) {
										std::string msg = std::format("{0}.{1} {2}", _msg[class_name_field].as_string(), _msg["field_name"].as_string(), _msg[message_field].as_string());
										system_monitoring_interface::active_mon->log_information(msg);
										});
								}
							}
							});
					}
					});
			}
			else {
				system_monitoring_interface::active_mon->log_warning("No result items found in put_result", __FILE__, __LINE__);
			}
		}


		virtual std::shared_ptr<class_interface> get_class_impl(activity* _activity, std::string _class_name)
		{
			std::shared_ptr<class_interface> cd;
			json_parser jp;

			class_cache.try_get(_class_name, cd);

			if (not cd) {
				std::shared_ptr<class_implementation> cdimp = std::make_shared<class_implementation>();
				json key = jp.create_object();
				key.put_member(class_name_field, _class_name);
				json class_def = classes->select(nullptr, key, [&key](json& _target)-> json {
					if (key.compare(_target) == 0) {
						return _target;
					}
					json jx;
					return jx;
					});
				if (class_def.array() and class_def.size()>0) {
					activity get_activity;
					get_activity.db = this;
					try {
						cdimp->open(&get_activity, class_def.get_first_element(), -1);
						class_cache.insert(_class_name, cdimp);
						auto perms = get_system_permission();
						cdimp->init_validation(this, perms);
						return cdimp;
					}
                    catch (std::exception& ex) {
                        system_monitoring_interface::active_mon->log_warning(std::format("Exception {0} on opening class {1}", ex.what(), _class_name), __FILE__, __LINE__);
                        return nullptr;
                    }
				}
			}
			return cd;
		}

		virtual std::shared_ptr<class_interface> put_class_impl(activity* _activity, json& _class_definition)
		{
			std::string class_name = _class_definition[class_name_field].as_string();
			auto ci = get_class_impl(_activity, class_name);
			if (ci) {
				activity activio;
				activio.db = this;
				ci->update(&activio, _class_definition);
			}
			else 
			{
				activity activio;
				activio.db = this;
				ci = std::make_shared<class_implementation>();
				bool created = ci->create(&activio, _class_definition);
				if (!created) {
                    log_warning("Failed to create class " + class_name);
					log_errors(activio.errors);
                }
			}
			auto perms = get_system_permission();
			ci->init_validation(this, perms);
            save_class(ci.get());
			class_cache.insert(class_name, ci);
			return ci;
		}

		json create_database()
		{
			json result;
			timer method_timer;

			json created_classes;
			json_parser jp;

			date_time start_time = date_time::now();
			timer tx;

			using namespace std::literals;

			system_monitoring_interface::active_mon->log_job_start("create_database", "start", start_time, __FILE__, __LINE__);

			if (corona_db_read_only) {
				throw std::logic_error("Can't create a read only database, create one, then open it read only");
			}
		
			std::shared_ptr<xtable_header> class_data_header = std::make_shared<xtable_header>();

			class_data_header->key_members.columns[1] = { field_types::ft_string, 1, "class_name" };
			class_data_header->object_members.columns[2] = { field_types::ft_string, 2, "class_description" };
			class_data_header->object_members.columns[3] = { field_types::ft_string, 3, "base_class_name" };
			class_data_header->object_members.columns[4] = { field_types::ft_array, 4, "parents" };
			
			class_data_header->object_members.columns[5] = { field_types::ft_object, 5, "fields" };
			class_data_header->object_members.columns[6] = { field_types::ft_object, 6, "indexes" };
			class_data_header->object_members.columns[7] = { field_types::ft_array, 7, "ancestors" };
			class_data_header->object_members.columns[8] = { field_types::ft_array, 8, "descendants" };
			class_data_header->object_members.columns[9] = { field_types::ft_object, 9, "sql" };
			class_data_header->object_members.columns[13] = { field_types::ft_string, 10, "class_author" };
			class_data_header->object_members.columns[14] = { field_types::ft_string, 11, "class_version" };
			class_data_header->object_members.columns[17] = { field_types::ft_array, 12, "full_text" };
			class_data_header->object_members.columns[18] = { field_types::ft_array, 13, "field_order" };

			std::filesystem::path class_table = data_path;
            class_table /= "classes.coronatbl";
            std::string file_name = class_table.string();
			
			classes = std::make_shared<xtable>(file_name, class_data_header);
			classes->commit();

			created_classes = jp.create_object();

			json response =  create_class(R"(
{	
	"class_name" : "sys_object",
	"class_description" : "Object",
	"fields" : {			
			"object_id" : { 
				"field_type":"int64",
				"read_only": true,	
				"label": "Object Id"
			},
			"class_name" : { 
				"field_type":"string",
				"read_only": true,	
				"label": "Class"
			},
			"created" : {
				"field_type":"datetime",
				"read_only": true,	
				"label": "Created"
			},
			"created_by" : {
				"field_type":"string",
				"read_only": true,	
				"label": "Created By"
			},
			"updated": {
				"field_type":"datetime",
				"read_only": true,	
				"label": "Updated"
			},
 			"updated_by" : {
				"field_type":"string",
				"read_only": true,	
				"label": "Updated By"
			},
			"team" : {
				"field_type":"string",
				"read_only": true,	
				"label": "Team"
			}
	},
	"field_order" : ["object_id", "class_name", "created", "created_by", "updated", "updated_by", "team"]	
}
)");

			if (response[success_field].as_bool() == false) {
				system_monitoring_interface::active_mon->log_warning("create_class put failed", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json<json>(response);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			json test =  classes->get(R"({"class_name":"sys_object"})"_jobject);
			if (test.empty() or test.error()) {
				system_monitoring_interface::active_mon->log_warning("could not find class sys_object after creation.", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			created_classes.put_member("sys_object", true);


			response = create_class(R"(
{	
	"class_name" : "sys_field",
	"class_description" : "Base Field",
	"base_class_name" : "sys_object",
	"parents" :[ "sys_class" ],
	"fields" : {			
			"field_name" : { 
				"field_type":"string",
				"label": "Field Name",
				"required": true
			},
			"field_type" : { 
				"field_type":"string",
				"read_only": true,	
				"label": "Field Type",
				"required": true,
				"enumeration": ["string", "number", "int64", "datetime", "boolean", "array", "object", "selection", "chest", "path", "brush", "bitmap"]
			},
			"required":"bool",
			"format":"string",
			"server_only": "bool",
			"read_only": "bool",
			"input_mask":"string",
			"label":"string",
			"description":"string",
			"tab_index":"string"
	},
	"field_order" : ["field_name", "field_type", "label", "description", "required", "server_only", "read_only", "input_mask"]	
}
)");

			if (response[success_field].as_bool() == false) {
				system_monitoring_interface::active_mon->log_warning("create_class put failed", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json<json>(response);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			test = classes->get(R"({"class_name":"sys_field"})"_jobject);
			if (test.empty() or test.error()) {
				system_monitoring_interface::active_mon->log_warning("could not find class sys_field after creation.", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			created_classes.put_member("sys_field", true);


			response = create_class(R"(
{	
	"class_name" : "sys_index",
	"class_description" : "Base Index",
	"base_class_name" : "sys_object",
	"parents" :[ "sys_class" ],
	"fields" : {			
			"index_name" : { 
				"field_type":"string",
				"label": "Index Name",
				"required": true
			},
			"index_keys" :[ "string" ]
	},
	"field_order" : ["index_name", "index_keys" ]	
}
)");

			if (response[success_field].as_bool() == false) {
				system_monitoring_interface::active_mon->log_warning("create_class put failed", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json<json>(response);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			test = classes->get(R"({"class_name":"sys_field"})"_jobject);
			if (test.empty() or test.error()) {
				system_monitoring_interface::active_mon->log_warning("could not find class sys_field after creation.", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			created_classes.put_member("sys_field", true);


			response = create_class(R"(
{	
	"class_name" : "sys_object_field",
	"class_description" : "Object Field",
	"base_class_name" : "sys_field",
	"parents" :[ "sys_class" ],
	"fields" : {			
		"relations":"object",
		"fundamental_type":"string"
	},
	"field_order" : ["relations", "fundamental_type"]	
}
)");

			if (response[success_field].as_bool() == false) {
				system_monitoring_interface::active_mon->log_warning("create_class put failed", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json<json>(response);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			test = classes->get(R"({"class_name":"sys_object_field"})"_jobject);
			if (test.empty() or test.error()) {
				system_monitoring_interface::active_mon->log_warning("could not find class sys_object_field after creation.", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			created_classes.put_member("sys_object_field", true);

			response = create_class(R"(
{	
	"class_name" : "sys_array_field",
	"class_description" : "Array Field",
	"base_class_name" : "sys_field",
	"parents" :[ "sys_class" ],
	"fields" : {	
		"relations":"object",
		"fundamental_type":"string"
	},
	"field_order" : ["relations", "fundamental_type"]	
}
)");

			if (response[success_field].as_bool() == false) {
				system_monitoring_interface::active_mon->log_warning("create_class put failed", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json<json>(response);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			test = classes->get(R"({"class_name":"sys_array_field"})"_jobject);
			if (test.empty() or test.error()) {
				system_monitoring_interface::active_mon->log_warning("could not find class sys_array_field after creation.", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			created_classes.put_member("sys_array_field", true);

			response = create_class(R"(
{	
	"class_name" : "sys_chest_field",
	"class_description" : "Chest Field",
	"base_class_name" : "sys_field",
	"parents" :[ "sys_class" ],
	"fields" : {			
		"allowed_class_name":"string",
		"allowed_item_type":"string",
		"unit_name":"string",
		"max_items":"int64"
	},
	"field_order" : ["allowed_class_name", "allowed_item_type", "unit_name", "max_items"]	
}
)");

			if (response[success_field].as_bool() == false) {
				system_monitoring_interface::active_mon->log_warning("create_class put failed", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json<json>(response);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			test = classes->get(R"({"class_name":"sys_chest_field"})"_jobject);
			if (test.empty() or test.error()) {
				system_monitoring_interface::active_mon->log_warning("could not find class sys_chest_field after creation.", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			created_classes.put_member("sys_chest_field", true);

			response = create_class(R"(
{	
	"class_name" : "sys_double_field",
	"class_description" : "Double Field",
	"base_class_name" : "sys_field",
	"parents" :[ "sys_class" ],
	"fields" : {	
			"max_value":"double",
			"min_value":"double"
	},
	"field_order" : ["field_name", "field_type"]	
}
)");

			if (response[success_field].as_bool() == false) {
				system_monitoring_interface::active_mon->log_warning("create_class put failed", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json<json>(response);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			test = classes->get(R"({"class_name":"sys_double_field"})"_jobject);
			if (test.empty() or test.error()) {
				system_monitoring_interface::active_mon->log_warning("could not find class sys_double_field after creation.", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			created_classes.put_member("sys_double_field", true);


			response = create_class(R"(
{	
	"class_name" : "sys_int64_field",
	"class_description" : "Int64 Field",
	"base_class_name" : "sys_field",
	"parents" :[ "sys_class" ],
	"fields" : {			
		"min_value":"int64",
		"max_value":"int64"
	},
	"field_order" : ["min_value", "max_value"]	
}
)");

			if (response[success_field].as_bool() == false) {
				system_monitoring_interface::active_mon->log_warning("create_class put failed", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json<json>(response);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			test = classes->get(R"({"class_name":"sys_int64_field"})"_jobject);
			if (test.empty() or test.error()) {
				system_monitoring_interface::active_mon->log_warning("could not find class sys_int64_field after creation.", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			created_classes.put_member("sys_int64_field", true);


			response = create_class(R"(
{	
	"class_name" : "sys_reference_field",
	"class_description" : "Reference Field",
	"base_class_name" : "sys_field",
	"parents" :[ "sys_class" ],
	"fields" : {			
		"reference_class":"string"
	},
	"field_order" : ["reference_class"]	
}
)");

			if (response[success_field].as_bool() == false) {
				system_monitoring_interface::active_mon->log_warning("create_class put failed", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json<json>(response);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			test = classes->get(R"({"class_name":"sys_reference_field"})"_jobject);
			if (test.empty() or test.error()) {
				system_monitoring_interface::active_mon->log_warning("could not find class sys_reference_field after creation.", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			created_classes.put_member("sys_reference_field", true);


			response = create_class(R"(
{	
	"class_name" : "sys_string_field",
	"class_description" : "String Field",
	"base_class_name" : "sys_field",
	"parents" :[ "sys_class" ],
	"fields" : {			
		"match_pattern":"string",
		"allowed_values":["string"],
		"min_length":"int64",
		"max_length":"int64"
	},
	"field_order" : ["match_pattern", "allowed_values", "min_length", "max_length"]	
}
)");

			if (response[success_field].as_bool() == false) {
				system_monitoring_interface::active_mon->log_warning("create_class put failed", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json<json>(response);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			test = classes->get(R"({"class_name":"sys_string_field"})"_jobject);
			if (test.empty() or test.error()) {
				system_monitoring_interface::active_mon->log_warning("could not find class sys_string_field after creation.", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			created_classes.put_member("sys_string_field", true);



			response = create_class(R"(
{	
	"class_name" : "sys_bool_field",
	"class_description" : "Bool Field",
	"base_class_name" : "sys_field",
	"parents" :[ "sys_class" ],
	"fields" : {			

	},
	"field_order" : []	
}
)");

			if (response[success_field].as_bool() == false) {
				system_monitoring_interface::active_mon->log_warning("create_class put failed", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json<json>(response);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			test = classes->get(R"({"class_name":"sys_bool_field"})"_jobject);
			if (test.empty() or test.error()) {
				system_monitoring_interface::active_mon->log_warning("could not find class sys_bool_field after creation.", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			created_classes.put_member("sys_bool_field", true);



			response = create_class(R"(
{	
	"class_name" : "sys_datetime_field",
	"class_description" : "Datetime Field",
	"base_class_name" : "sys_field",
	"parents" :[ "sys_class" ],
	"fields" : {			
		"min_value":"datetime",
		"max_value":"datetime"
	},
	"field_order" : ["min_value", "max_value"]	
}
)");

			if (response[success_field].as_bool() == false) {
				system_monitoring_interface::active_mon->log_warning("create_class put failed", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json<json>(response);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			test = classes->get(R"({"class_name":"sys_datetime_field"})"_jobject);
			if (test.empty() or test.error()) {
				system_monitoring_interface::active_mon->log_warning("could not find class sys_datetime_field after creation.", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			created_classes.put_member("sys_datetime_field", true);




			response = create_class(R"(
{	
	"class_name" : "sys_query_field",
	"class_description" : "Query Field",
	"base_class_name" : "sys_field",
	"parents" :[ "sys_class" ],
	"fields" : {			
		"query_body":"object"
	},
	"field_order" : ["field_name", "field_type"]	
}
)");

			if (response[success_field].as_bool() == false) {
				system_monitoring_interface::active_mon->log_warning("create_class put failed", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json<json>(response);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			test = classes->get(R"({"class_name":"sys_query_field"})"_jobject);
			if (test.empty() or test.error()) {
				system_monitoring_interface::active_mon->log_warning("could not find class sys_query_field after creation.", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			created_classes.put_member("sys_query_field", true);


			response = create_class(R"(
{	
	"class_name" : "sys_vector_field",
	"class_description" : "Vector Field",
	"base_class_name" : "sys_field",
	"parents" :[ "sys_class" ],
	"fields" : {			

	},
	"field_order" : []	
}
)");

			if (response[success_field].as_bool() == false) {
				system_monitoring_interface::active_mon->log_warning("create_class put failed", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json<json>(response);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			test = classes->get(R"({"class_name":"sys_vector_field"})"_jobject);
			if (test.empty() or test.error()) {
				system_monitoring_interface::active_mon->log_warning("could not find class sys_vector_field after creation.", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			created_classes.put_member("sys_vector_field", true);



			response = create_class(R"(
{	
	"class_name" : "sys_rectangle_field",
	"class_description" : "Rectangle Field",
	"base_class_name" : "sys_field",
	"parents" :[ "sys_class" ],
	"fields" : {			

	},
	"field_order" : []	
}
)");

			if (response[success_field].as_bool() == false) {
				system_monitoring_interface::active_mon->log_warning("create_class put failed", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json<json>(response);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			test = classes->get(R"({"class_name":"sys_vector_field"})"_jobject);
			if (test.empty() or test.error()) {
				system_monitoring_interface::active_mon->log_warning("could not find class sys_vector_field after creation.", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			created_classes.put_member("sys_rectangle_field", true);



			response = create_class(R"(
{	
	"class_name" : "sys_path_field",
	"class_description" : "Path Field",
	"base_class_name" : "sys_field",
	"parents" :[ "sys_class" ],
	"fields" : {			

	},
	"field_order" : []	
}
)");

			if (response[success_field].as_bool() == false) {
				system_monitoring_interface::active_mon->log_warning("create_class put failed", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json<json>(response);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			test = classes->get(R"({"class_name":"sys_path_field"})"_jobject);
			if (test.empty() or test.error()) {
				system_monitoring_interface::active_mon->log_warning("could not find class sys_path_field after creation.", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			created_classes.put_member("sys_path_field", true);



			response = create_class(R"(
{	
	"class_name" : "sys_brush_field",
	"class_description" : "Brush Field",
	"base_class_name" : "sys_field",
	"parents" :[ "sys_class" ],
	"fields" : {			

	},
	"field_order" : []	
}
)");

			if (response[success_field].as_bool() == false) {
				system_monitoring_interface::active_mon->log_warning("create_class put failed", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json<json>(response);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			test = classes->get(R"({"class_name":"sys_brush_field"})"_jobject);
			if (test.empty() or test.error()) {
				system_monitoring_interface::active_mon->log_warning("could not find class sys_brush_field after creation.", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			created_classes.put_member("sys_brush_field", true);



			response = create_class(R"(
{	
	"class_name" : "sys_bitmap_field",
	"class_description" : "Bitmap Field",
	"base_class_name" : "sys_field",
	"parents" :[ "sys_class" ],
	"fields" : {			

	},
	"field_order" : []	
}
)");

			if (response[success_field].as_bool() == false) {
				system_monitoring_interface::active_mon->log_warning("create_class put failed", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json<json>(response);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			test = classes->get(R"({"class_name":"sys_bitmap_field"})"_jobject);
			if (test.empty() or test.error()) {
				system_monitoring_interface::active_mon->log_warning("could not find class sys_bitmap_field after creation.", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			created_classes.put_member("sys_bitmap_field", true);



			response = create_class(R"(
{	
	"class_name" : "sys_audio_field",
	"class_description" : "Audio Field",
	"base_class_name" : "sys_field",
	"parents" :[ "sys_class" ],
	"fields" : {			

	},
	"field_order" : []	
}
)");

			if (response[success_field].as_bool() == false) {
				system_monitoring_interface::active_mon->log_warning("create_class put failed", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json<json>(response);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			test = classes->get(R"({"class_name":"sys_audio_field"})"_jobject);
			if (test.empty() or test.error()) {
				system_monitoring_interface::active_mon->log_warning("could not find class sys_audio_field after creation.", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			created_classes.put_member("sys_audio_field", true);




			response = create_class(R"(
{	
	"class_name" : "sys_selection_field",
	"class_description" : "Selection Field",
	"base_class_name" : "sys_field",
	"parents" :[ "sys_class" ],
	"fields" : {			

	},
	"field_order" : []	
}
)");

			if (response[success_field].as_bool() == false) {
				system_monitoring_interface::active_mon->log_warning("create_class put failed", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json<json>(response);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			test = classes->get(R"({"class_name":"sys_selection_field"})"_jobject);
			if (test.empty() or test.error()) {
				system_monitoring_interface::active_mon->log_warning("could not find class sys_selection_field after creation.", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			created_classes.put_member("sys_selection_field", true);



			response = create_class(R"(
{	
	"class_name" : "sys_class",
	"class_description" : "Class Template",
	"base_class_name" : "sys_object",
	"fields" : {			
		"base_name": { 
			"field_type":"string",
			"label": "Base Class Name",
			"required": true
		},		
		"name" : { 
			"field_type":"string",
			"label": "Class Name",
			"required": true
		},
		"description" : { 
			"field_type":"string",
			"label": "Class Description",
			"required": true
		},
		"author": { 
			"field_type":"string",
			"label": "Class Author",
			"required": true
		},
		"version": { 
			"field_type":"string",
			"label": "Version",
			"required": true
		},
		"full_text":[ "string"],
		"fields" : [ "sys_field"],
		"indexes" : [ "sys_index"],
		"sql" : "object",
		"field_order" : [ "string"]
	},
	"field_order" : []	
}
)");

			if (response[success_field].as_bool() == false) {
				system_monitoring_interface::active_mon->log_warning("create_class put failed", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json<json>(response);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			test = classes->get(R"({"class_name":"sys_selection_field"})"_jobject);
			if (test.empty() or test.error()) {
				system_monitoring_interface::active_mon->log_warning("could not find class sys_selection_field after creation.", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			created_classes.put_member("sys_selection_field", true);


			response = create_class(R"(
{	
	"class_name" : "sys_error",
	"class_description" : "Error",
	"base_class_name" : "sys_object",
	"fields" : {			
			"system" : {
				"field_type":"string",
				"read_only": true,
				"label": "System Origin"
			},
			"level": {
				"field_type":"string",
				"read_only": true,
				"label": "Error Level"
			},
			"message" :{
				"field_type":"string",
				"read_only": true,
				"label": "Message"
			},
			"body": {
				"field_type":"string",
				"read_only": true,
				"label": "Details"
			},
			"file" : {
				"field_type":"string",
				"read_only": true,
				"label": "Source File"
			},
			"line": {
				"field_type":"number",
				"read_only": true,
				"label": "Source Line #"
			},
	},
	"field_order" : ["system", "level", "message", "body", "file", "line"]
}
)");

			if (response[success_field].as_bool() == false) {
				system_monitoring_interface::active_mon->log_warning("create_class put failed", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json<json>(response);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			test = classes->get(R"({"class_name":"sys_error"})"_jobject);
			if (test.empty() or test.error()) {
				system_monitoring_interface::active_mon->log_warning("could not find class sys_error after creation.", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			created_classes.put_member("sys_error", true);


			response = create_class(R"(
{	
	"class_name" : "sys_server",
	"class_description" : "Servers",
	"base_class_name" : "sys_object",
	"fields" : {
			"server_name" : {
				"field_type":"string",
				"read_only": true,
				"label": "Server Name"
			},
			"server_description" : {
				"field_type":"string",
				"read_only": true,
				"label": "Description",
			},
			"server_url" : {
				"field_type":"string",
				"read_only": true,
				"label": "Url",
			},
			"server_version": {
				"field_type":"string",
				"read_only": true,
				"label": "Version"
			}
	}
}
)");

			if (response[success_field].as_bool() == false) {
				system_monitoring_interface::active_mon->log_warning("sys_server put failed", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json<json>(response);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			test = classes->get(R"({"class_name":"sys_server"})"_jobject);
			if (test.empty() or test.error()) {
				system_monitoring_interface::active_mon->log_warning("could not find class sys_server after creation.", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			created_classes.put_member("sys_server", true);



			response = create_class(R"(
{	
	"class_name" : "sys_command",
	"class_description" : "Command",
	"base_class_name" : "sys_object",
	"fields" : {
	}
}
)");

			if (response[success_field].as_bool() == false) {
				system_monitoring_interface::active_mon->log_warning("create_class put failed", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json<json>(response);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			test = classes->get(R"({"class_name":"sys_command"})"_jobject);
			if (test.empty() or test.error()) {
				system_monitoring_interface::active_mon->log_warning("could not find class sys_command after creation.", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			created_classes.put_member("sys_command", true);



			response = create_class(R"(
{
	"class_name" : "sys_grant",
	"base_class_name" : "sys_object",
	"class_description" : "Grant",
	"parents" : [ "sys_team" ],
	"fields" : {
			"grant_classes" : "[string]",
			"get" : {
				"field_type":"string",
				"field_name":"get",
				"enum" : [ "any", "none", "own", "team", "teamorown" ]
			},
			"put" : {
				"field_type":"string",
				"field_name":"put",
				"enum" : [ "any", "none", "own", "team", "teamorown" ]
			},
			"delete" : {
				"field_type":"string",
				"field_name":"delete",
				"enum" : [ "any", "none", "own", "team", "teamorown" ]
			},
			"alter" : {
				"field_type":"string",
				"field_name":"alter",
				"enum" : [ "any", "none", "own", "team", "teamorown" ]
			},
			"derive" : {
				"field_type":"string",
				"field_name":"derive",
				"enum" : [ "any", "none", "own", "team", "teamorown" ]
			}
			
	},
	"field_order" : ["grant_classes", "get", "put", "delete", "alter", "derive"]
}
)");

			if (response[success_field].as_bool() == false) {
				system_monitoring_interface::active_mon->log_warning("create_class sys_grant put failed", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json<json>(response);
				std::cout << response.to_json() << std::endl;
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			test = classes->get(R"({"class_name":"sys_grant"})"_jobject);
			if (test.empty() or test.error()) {
				system_monitoring_interface::active_mon->log_warning("could not find class sys_grant after creation.", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			created_classes.put_member("sys_grant", true);



			response = create_class(R"(
{
	"class_name" : "sys_team",
	"base_class_name" : "sys_object",
	"class_description" : "Team",
	"fields" : {
			"team_name" :  {
				"field_name:" : "team_name",
				"field_type" : "string",
				"label":"Team Name"
			},
			"team_description" :  {
				"field_name:" : "team_description",
				"field_type" : "string",		
				"label":"Team Description"
			},
			"team_domain" : {
				"field_name:" : "team_domain",
				"field_type" : "string",		
				"format:" : "regexp",
				"label":"Team Domain"
			},
			"permissions" : "[ sys_grant ]",
			"allowed_teams" : "[ string ]",
			"items" : "[ sys_item ]"
	},
	"field_order" : ["team_name", "team_description", "team_domain", "permissions", "allowed_teams", "items"],
	"indexes" : {
        "sys_team_name": {
          "index_keys": [ "team_name" ]
        }
	}
}
)");

			if (response[success_field].as_bool() == false) {
				system_monitoring_interface::active_mon->log_warning("create_class sys_team put failed", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json<json>(response);
				std::cout << response.to_json() << std::endl;
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			test = classes->get(R"({"class_name":"sys_team"})"_jobject);
			if (test.empty() or test.error()) {
				system_monitoring_interface::active_mon->log_warning("could not find class sys_team after creation.", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			created_classes.put_member("sys_team", true);

			response = create_class(R"(
{
	"class_name" : "sys_dataset",
	"base_class_name" : "sys_object",
	"class_description" : "DataSet",
	"parents" : [ "sys_schema" ],
	"fields" : {
			"dataset_name" : {
				"field_name:" : "dataset_name",
				"field_type" : "string",		
				"label":"Name"
			},
			"dataset_description" : {
				"field_name:" : "dataset_description",
				"field_type" : "string",		
				"label":"Description"
			},
			"dataset_version" : {
				"field_name:" : "dataset_version",
				"field_type" : "string",		
				"label":"Version"
			},
			"dataset_author" : {
				"field_name:" : "dataset_author",
				"field_type" : "string",		
				"label":"Authors"
			},
			"dataset_source" : {
				"field_name:" : "dataset_source",
				"field_type" : "string",		
				"label":"Source"
			},
			"completed" : {
				"field_name:" : "completed",
				"field_type" : "datetime",		
				"label":"Completed"
			},
			"run_on_change": {
				"field_name:" : "run_on_change",
				"field_type" : "string",		
				"label":"Run On Change"
			},	
			"bytes_read" : {
				"field_name:" : "bytes_read",
				"field_type" : "int64",		
				"label":"Bytes Read"
			},
			"bytes_total" : {
				"field_name:" : "bytes_total",
				"field_type" : "int64",		
				"label":"Bytes Total"
			},
			"objects" : "array",
			"import" : "object"
	},
	"indexes" : {
		"sys_dataset_dataset_name": {
		  "index_keys": [ "dataset_name", "dataset_version" ]
		}	
	},
	"field_order" : ["dataset_name", "dataset_description", "dataset_version", "dataset_author", "dataset_source", "completed", "run_on_change", "bytes_read", "bytes_total", "objects", "import"]
}
)");

			if (response[success_field].as_bool() == false) {
				system_monitoring_interface::active_mon->log_warning("create_class sys_dataset put failed", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json<json>(response);
				std::cout << response.to_json() << std::endl;
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			test = classes->get(R"({"class_name":"sys_dataset"})"_jobject);
			if (test.empty() or test.is_member("class_name", "SysParseError")) {
				system_monitoring_interface::active_mon->log_warning("could not find class sys_dataset after creation.", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			created_classes.put_member("sys_dataset", true);

			response =  create_class(R"(
{
	"class_name" : "sys_schema",
	"base_class_name" : "sys_object",
	"class_description" : "Schema",
	"fields" : {		
			"schema_name" : {
				"field_name:" : "schema_name",
				"field_type" : "string",		
				"label":"Name"
			},
			"schema_description" : {
				"field_name:" : "schema_description",
				"field_type" : "string",		
				"label":"Description"
			},
			"schema_version" : {
				"field_name:" : "schema_version",
				"field_type" : "string",		
				"label":"Version"
			},
			"schema_authors" : [ "string" ],			
			"classes" : "[object]",
			"users" : "[object]",
			"datasets" : [ "sys_dataset" ]
		}
	}
}
)");

			created_classes.put_member("sys_schema", true);

			if (response[success_field].as_bool() == false) {
				system_monitoring_interface::active_mon->log_warning("create_class put failed", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json<json>(response);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			test = classes->get(R"({"class_name":"sys_schema"})"_jobject);
			if (test.empty() or test.error()) {
				system_monitoring_interface::active_mon->log_warning("could not find class sys_schema after creation.", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);

				return result;
			}

			response = create_class(R"(
{	
	"base_class_name" : "sys_object",
	"class_name" : "sys_item",
	"class_description" : "Item",
	"parents": [ "sys_user", "sys_item", "sys_team" ],
	"fields" : {			
			
        }
	}
}
)");

			if (response[success_field].as_bool() == false) {
				system_monitoring_interface::active_mon->log_warning("create_class sys_item put failed", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json<json>(response);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			test = classes->get(R"({"class_name":"sys_item"})"_jobject);
			if (test.empty() or test.error()) {
				system_monitoring_interface::active_mon->log_warning("could not find class sys_item after creation.", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);

				return result;
			}
			created_classes.put_member("sys_item", true);

			response =  create_class(R"(
{	
	"base_class_name" : "sys_object",
	"class_name" : "sys_user",
	"class_description" : "User",
	"fields" : {			
			"first_name" : {
				"field_type":"string",
				"field_name":"first_name",
				"label": "First Name",
				"required" : true,
				"max_length" : 50,
				"match_pattern": "[a-zA-Z0-9/s]+"
			},
			"last_name" : {
				"field_type":"string",
				"field_name":"last_name",
				"label": "Last Name",
				"required" : true,
				"max_length" : 50,
				"match_pattern": "[a-zA-Z0-9/s]+"
			},
			"user_name" : {
				"field_type":"string",
				"field_name":"user_name",
				"label": "User Name",	
				"required" : true,
				"max_length" : 100
			},
			"email" : {
				"field_type":"string",
				"field_name":"email",
				"label": "E-Mail",
				"required" : true,
				"max_length" : 100,
				"match_pattern": "(/w+)(/.|_)?(/w*)@(/w+)(/.(/w+))+"
			},
			"mobile" : {
				"field_type":"string",
				"field_name":"mobile",
				"label": "Mobile",
				"format":"tel",
				"required" : true,
				"max_length" : 15
			},
			"street1" : {
				"field_type":"string",
				"field_name":"street1",
				"format":"street",
				"label": "Street Address",
				"required" : true,
				"max_length" : 100,
				"match_pattern": "[a-zA-Z0-9_/-/s]+"
			},
			"street2" : {
				"field_type":"string",
				"field_name":"street2",
				"required" : true,
				"label": "Street Address 2",
				"max_length" : 100,
				"match_pattern": "[a-zA-Z0-9_/-/s]+"
			},
			"city" : {
				"field_type":"string",
				"field_name":"city",
				"required" : true,
				"label": "City",
				"max_length" : 100,
				"match_pattern": "[a-zA-Z0-9_/-/s]+"
			},
			"state" : {
				"field_type":"string",
				"field_name":"state",
				"required" : true,
				"max_length" : 50,
				"label": "State",
				"match_pattern": "[a-zA-Z0-9_/-/s]+"
			},
			"zip" : {
				"field_type":"string",
				"field_name":"zip",
				"required" : true,
				"max_length" : 15,
				"match_pattern": "^/d{5}(?:[-/s]/d{4})?$",
				"label": "Zip"
			},
			"password" : { 
				"field_type":"string",
				"field_name":"password",	
				"is_server_only": true,
				"read_only" : true,	
			},
			"confirmed_code": { 
				"field_type":"bool",
				"field_name":"confirmed_code",	
				"is_server_only": true,
				"read_only" : true,	
			},
			"validation_code" : { 
				"field_type":"string",
				"field_name":"validation_code",	
				"is_server_only": true,
				"read_only" : true,	
			},
			"home_team_name" :{ 
				"field_type":"string",
				"field_name":"home_team_name",
				"read_only" : true,	
			},
			"team_name" :{ 
				"field_type":"string",
				"field_name":"team_name",
				"read_only" : true
			},
			"picture" :{ 
				"field_type":"string",
				"field_name":"picture"
			}
        }
	}
}
)");

			if (response[success_field].as_bool() == false) {
				system_monitoring_interface::active_mon->log_warning("create_class sys_user put failed", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json<json>(response);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			test = classes->get(R"({"class_name":"sys_user"})"_jobject);
			if (test.empty() or test.error()) {
				system_monitoring_interface::active_mon->log_warning("could not find class sys_user after creation.", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);

				return result;
			}
			created_classes.put_member("sys_user", true);

			json gc = jp.create_object();
			json gcr = create_system_request( gc );

			json classes_array_response =  get_classes(gcr);
			json classes_array = classes_array_response[data_field];
			json classes_grouped = classes_array.group([](json& _item) -> std::string {
				return _item[class_name_field].as_string();
				});

			auto members = created_classes.get_members();
			json missing_classes = jp.create_array();
			for (auto created_class : members) {
				const std::string& created_class_name = created_class.first;
				if (not classes_grouped.has_member(created_class_name)) {
					missing_classes.push_back(created_class_name);
				}
			}

			if (missing_classes.size() > 0) {
				system_monitoring_interface::active_mon->log_warning("system classes not saved", __FILE__, __LINE__);

				for (auto mc : missing_classes) {
					system_monitoring_interface::active_mon->log_information(mc.as_string(), __FILE__, __LINE__);
				}

				for (auto mc : classes_array) {
					system_monitoring_interface::active_mon->log_json(mc);
				}

				system_monitoring_interface::active_mon->log_job_stop("create_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			json new_user_request;
			json new_user_data;

			new_user_data = jp.create_object();
			new_user_data.put_member(class_name_field, "sys_user"sv);
			new_user_data.put_member(user_name_field, default_user);
			new_user_data.put_member(user_email_field, default_email_address);
			new_user_data.put_member(user_first_name_field, "first"sv);
			new_user_data.put_member(user_last_name_field, "last"sv);
			new_user_data.put_member("password1", default_password);
			new_user_data.put_member("password2", default_password);
			new_user_data.put_member(user_street1_field, "street1"sv);
			new_user_data.put_member(user_street2_field, "street2"sv);
			new_user_data.put_member(user_city_field, "bowling green"sv);
			new_user_data.put_member(user_state_field, "ky"sv);
			new_user_data.put_member(user_zip_field, "42101"sv);
			new_user_data.put_member(user_mobile_field, "2158675309"sv);

			new_user_request = create_system_request(new_user_data);
			json new_user_result =  create_user(new_user_request, true, true);
			bool success = new_user_result[success_field].as_bool();
			validation_error_collection errors;

			if (success) {
				json new_user = new_user_result[data_field];
				response = create_response(new_user_request, true, "Database Created", new_user, errors, method_timer.get_elapsed_seconds());
			}
			else 
			{
				system_monitoring_interface::active_mon->log_warning("system user create failed", __FILE__, __LINE__);
				log_errors(errors);
				json temp = jp.create_object();
				response = create_response(new_user_request, false, "Database user create failed.", temp, errors, method_timer.get_elapsed_seconds());
			}
			classes->commit();
			classes = nullptr;
			class_table = data_path;
			class_table /= "classes.coronatbl";
			file_name = class_table.string();
			classes = std::make_shared<xtable>(file_name, false);

			system_monitoring_interface::active_mon->log_job_stop("create_database", "completed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
			return response;
		}

private:

		virtual json add_item_chest_impl(json data, std::string user_name, std::string user_auth, validation_error_collection& _errors)
		{
			json_parser jp;
			json result = jp.create_object();
			if (data.object())
			{
				std::string target_class_name = data[class_name_field].as_string();
				int64_t target_object_id = data[object_id_field].as_int64_t();
				std::string chest_name = data[chest_name_field].as_string();
                json item_data = data["item_data"];
				auto edit_class = read_lock_class(target_class_name);

				if (edit_class) {
					auto permissions = get_class_permission(user_name, target_class_name);
					json key = data.extract({ class_name_field, object_id_field });
					json obj = edit_class->get_single_object(this, key, true, permissions);

					auto chest = edit_class->get_chest(obj, chest_name);

					item_data = item_data.as_array();

					for (int i = 0; i < item_data.size(); i++) {
						json id = item_data.get_element(i);
						chest_item ci;
						ci.put_json(item_data);
						chest->add_part(ci);
					}

					json new_chest = jp.create_object();
					chest->put_json(new_chest);
					obj.put_member(chest_name, new_chest);
					json child_objects = jp.create_array();
					json objects = jp.create_array();
					objects.push_back(obj);
					int64_t success_objects = 0;
					int64_t fail_objects = 0;
					result = edit_class->put_objects(this, child_objects, obj, permissions, _errors, success_objects, fail_objects);
				}
			}
			return result;
		}

		virtual json remove_item_chest_impl(json data, std::string user_name, std::string user_auth, validation_error_collection& _errors)
		{
			json_parser jp;
			json result = jp.create_object();
			if (data.object())
			{
				std::string target_class_name = data[class_name_field].as_string();
				int64_t target_object_id = data[object_id_field].as_int64_t();
				std::string chest_name = data[chest_name_field].as_string();
				json item_data = data["item_data"];

				auto edit_class = read_lock_class(target_class_name);

				if (edit_class) {
					auto permissions = get_class_permission(user_name, target_class_name);
					json key = data.extract({ class_name_field, object_id_field });
					json obj = edit_class->get_single_object(this, key, true, permissions);
					auto chest = edit_class->get_chest(obj, chest_name);

					item_data = item_data.as_array();

					for (int i = 0; i < item_data.size(); i++) {
						json id = item_data.get_element(i);
						chest_item ci;
						ci.put_json(item_data);
						chest->remove_part(ci);
					}

					json new_chest = jp.create_object();
					chest->put_json(new_chest);
					obj.put_member(chest_name, new_chest);
					json child_objects = jp.create_array();
					json objects = jp.create_array();
					objects.push_back(obj);
					int64_t success_objects = 0;
					int64_t fail_objects = 0;
					result = edit_class->put_objects(this, child_objects, obj, permissions, _errors, success_objects, fail_objects);
				}
			}

			return result;
		}

		virtual json move_item_chest_impl(json data, std::string user_name, std::string user_auth, validation_error_collection& _errors)
		{
			json_parser jp;
			json result = jp.create_object();
			if (data.object())
			{
                json item_data = data["item_data"];
                json from = data["from"];
                json to = data["to"];
                from.put_member("item_data"	, item_data);
				to.put_member("item_data", item_data);
                remove_item_chest_impl(from, user_name, user_auth, _errors);
                add_item_chest_impl(to, user_name, user_auth, _errors);
			}

			return result;
		}

		json create_class(std::string _text)
		{
			json_parser jp;

			json jclass_def = jp.parse_object(_text);

			json sys_request = create_system_request(jclass_def);

			json response = put_class(sys_request);

			if (response.error() or response.empty() or response[success_field].as_bool() == false)
			{
				system_monitoring_interface::active_mon->log_warning("Error creating class", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json(response);
			}

			return response;
		}

		virtual std::vector<std::string> get_class_descendants(const std::string& _class_name)
		{
            std::map<std::string, bool> visited;
			get_class_descendants(visited, _class_name);
			std::vector<std::string> results;
            for (auto pair : visited) {
				results.push_back(pair.first);
			}
			return results;
		}

		virtual void get_class_descendants(std::map<std::string, bool>& _visited, const std::string& _class_name)
		{
			_visited[_class_name] = true;
			auto classdef = read_lock_class(_class_name);
			if (classdef) {
				auto descendants = classdef->get_descendants();
				for (auto desc : descendants) {
					if (!_visited.contains(desc.first)) {
                        get_class_descendants(_visited, desc.first);
					}
				}
			}
		}

		json check_object(json object_load, std::string _user_name, validation_error_collection& validation_errors, std::string _authorization)
		{
			timer method_timer;
			json_parser jp;
			date_time current_date = date_time::now();
			using namespace std::literals;
			json response; 			

			response = jp.create_object();

			json object_definition,
				object_list,
				result_list;

			if (object_load.array()) 
			{
				object_list = object_load;
			}
			else if (object_load.object()) 
			{
				object_list = jp.create_array();
				object_list.push_back(object_load);
			}
			else
			{
				response.put_member(success_field, false);
				response.put_member(message_field, "not an object"sv);
				return response;
			}

			json classes_group = object_list.group([](json _item) -> std::string {
				return _item[class_name_field].as_string();
				});

			auto class_list = classes_group.get_members();


			for (auto class_pair : class_list)
			{
				if (class_pair.first.empty()) {
					response.put_member(success_field, false);
					response.put_member(message_field, "empty class name"sv);
					validation_error ve;
					ve.count = classes_group[class_pair.first].size();
					ve.filename = get_file_name(__FILE__);
					ve.class_name = "(empty)";
					ve.message = "Missing classname";
					ve.line_number = __LINE__;
					validation_errors.push_back(ve);
					continue;
				}
			}


			for (auto class_pair : class_list)
			{
				std::string class_name = class_pair.first;

				json class_object_list = classes_group[class_name];

				class_permissions permission;

				if (_authorization != auth_system) 
				{
					permission = get_class_permission(_user_name, class_pair.first);

					if (permission.put_grant == class_grants::grant_none) {
						response.put_member(success_field, false);
						response.put_member(message_field, "check_object denied"sv);
						validation_error ve;
						ve.count = classes_group[class_name].size();
						ve.filename = __FILE__;
						ve.class_name = class_pair.first;
						ve.message = "Check object denied";
						ve.line_number = __LINE__;
						validation_errors.push_back(ve);
						return response;
					}
				}
				else {
					permission.user_name = _user_name;
                    permission.put_grant = class_grants::grant_any;	
				}

				// move_member here is to prevent something stupid happening because this is in a loop 
				// and the scope of result_list is outside of it.
				// fixing it is out of scope for this particular thing.
				// TODO: fix result_list
				classes_group.move_member(class_name, class_object_list);
			}

			response.put_member(success_field, true);
			response.put_member(message_field, "Ok"sv);
			response.share_member(data_field, classes_group);
			return response;
		}

		std::string pass_phrase;
		std::string pass_iv;

		std::string get_pass_phrase()
		{
			if (pass_phrase.empty()) {
				pass_phrase = get_random_code(16);
            }
            return pass_phrase;
		}

		std::string get_iv()
		{
			if (pass_iv.empty()) {
				pass_iv = get_random_code(8);
			}
			return pass_iv;
		}

		json create_response(std::string _user_name, std::string _authorization, bool _success, std::string _message, json _data, validation_error_collection& _errors, double _seconds)
		{
			json_parser jp;

			json payload = jp.create_object();
			json token = jp.create_object();
			token.put_member(user_name_field, _user_name);
			token.put_member(authorization_field, _authorization);
			date_time expiration = date_time::utc_now() + this->token_life;
			token.put_member(token_expires_field, expiration);
			std::string cipher_text = crypter.encrypt(token, get_pass_phrase(), get_iv());
			token.put_member(signature_field, cipher_text);

			std::string token_string = token.to_json_typed();
			std::string base64_token_string = base64_encode(token_string );

			if (_success) {
				payload.put_member(token_field, base64_token_string);
			}
			payload.put_member(success_field, _success);
			payload.put_member(message_field, _message);
			payload.put_member(data_field, _data);
			payload.put_member(seconds_field, _seconds);

			if (_errors.size()) {
				json errors_array = jp.create_array();
				for (auto& ve : _errors) {
					json jve = jp.create_object();
					ve.get_json(jve);
					errors_array.push_back(jve);
				}
				payload.share_member("errors", errors_array);
			}


			return payload;
		}

		json create_user_response(json _request, bool _success, std::string _message, json _data, validation_error_collection& _errors, double _seconds)
		{
			json_parser jp;
			json payload;
			json errors_array = jp.create_array();

			if (_errors.size()) {
				for (auto& ve : _errors) {
					json jve = jp.create_object();
					ve.get_json(jve);
					errors_array.push_back(jve);
				}
			}
			payload = create_user_response(_request, _success, _message, _data, errors_array, _seconds);

			return payload;
		}

		json create_user_response(json _request, bool _success, std::string _message, json _data, json _errors, double _seconds)
		{
			json_parser jp;
			json payload = jp.create_object();

			payload.put_member(success_field, _success);
			payload.put_member(message_field, _message);
			payload.put_member(data_field, _data);
			payload.put_member_double(seconds_field, _seconds);

			if (_errors.size()) {
				payload.put_member("errors", _errors);
			}

			return payload;
		}

		json create_response(json _request, bool _success, std::string _message, json _data, validation_error_collection& _errors, double _seconds, int64_t success_objects = -1, int64_t failed_objects = -1, int64_t total_objects = -1)
		{
			json_parser jp;
			json payload = jp.create_object();
			json token = jp.create_object();
			json src_token = _request[token_field];
			
			token.copy_member(user_name_field, src_token);
			token.copy_member(authorization_field, src_token);
			date_time expiration = date_time::utc_now() + this->token_life;
			token.put_member(token_expires_field, expiration);
			std::string cipher_text = crypter.encrypt(token, get_pass_phrase(), get_iv());
			token.put_member(signature_field, cipher_text);

			std::string token_string = token.to_json_typed();
			std::string base64_token_string = base64_encode(token_string);

			payload.put_member(token_field, base64_token_string);
			payload.put_member(success_field, _success);
			payload.put_member(message_field, _message);
			payload.put_member("success_objects", success_objects < 0 ? 1 : success_objects);
			payload.put_member("failed_objects", failed_objects < 0 ? 1 : failed_objects);
			payload.put_member("total_objects", total_objects < 0 ? 1 : total_objects);

			payload.share_member(data_field, _data);
			if (_errors.size()) {
				json errors_array = jp.create_array();
				for (auto& ve : _errors) {
					json jve = jp.create_object();
					ve.get_json(jve);
					errors_array.push_back(jve);
				}
				payload.share_member("errors", errors_array);
			}
			payload.put_member_double(seconds_field, _seconds);
			return payload;
		}

		json check_token(std::string _encoded_token, std::vector<std::string> _authorizations)
		{
			json_parser jp;
			json empty;
			std::string decoded_token = base64_decode(_encoded_token);
			json token;

			try 
			{
				token = jp.parse_object(decoded_token);
			}
			catch (std::exception)
			{
				return empty;
			}

			if (not token.object())
			{
				return empty;
			}

			if (not token.has_member(signature_field))
			{
				return empty;
			}

			std::string signature = token[signature_field].as_string();
			token.erase_member(signature_field);

			std::string cipher_text = crypter.encrypt(token, get_pass_phrase(), get_iv());

			if (cipher_text != signature) {
				return empty;
			}

			token.put_member(signature_field, signature);

			date_time current = date_time::utc_now();
			date_time expiration = token[token_expires_field].as_date_time();

			if (current > expiration)
			{
				return empty;
			}

			if (cipher_text != signature)
			{
				return empty;
			}

			if (not token.has_member(authorization_field))
			{
				return empty;
			}

			std::string authorization = token[authorization_field].as_string();
			std::string user = token[user_name_field].as_string();

			if (authorization == auth_system) {
                return token;
			}

			for (auto _authorization : _authorizations)
			{
				if (authorization == _authorization) /* perhaps a tad loose used to be  and user == default_user*/
				{
					return token;
				}
			}
			return empty;
		}

		public:

		virtual bool check_message(json& _message, std::vector<std::string> _authorizations, std::string& _user_name, std::string& _token_authorization) override
		{
			std::string token = _message[token_field].as_string();

			json result = check_token(token, _authorizations);
			bool is_ok = not result.empty();

			if (is_ok) {
				_user_name = result[user_name_field].as_string();
                _token_authorization = result[authorization_field].as_string();
			}

			return is_ok;
		}

		virtual json get_openapi_schema(std::string user_name) override
		{
			json schema;
			json_parser jp;

			if (user_name.empty()) {
				user_name = default_user;
            }

			json all_classes = classes->select(nullptr, R"({"class_name":"sys_class"})"_jobject, [](json& _item) -> json {
                return _item;
				});

			json result_list = jp.create_array();
			if (all_classes.array()) {
				for (auto cls : all_classes) {
					std::string class_name = cls[class_name_field].as_string();
					auto permission = get_class_permission(user_name, class_name);
					if (permission.get_grant != class_grants::grant_none)
					{
						result_list.push_back(cls);
					}
                }
			}

			schema = jp.create_object();
			for (auto items : result_list)
			{
				std::string class_name = items[class_name_field].as_string();
				auto ptr = read_lock_class(class_name);
				if (ptr) {
					json class_schema = ptr->get_openapi_schema(this);
					schema.share_member(class_name, class_schema);
				}
			}

			return schema;
		}

		protected:

		json select_object(std::string _class_name, int64_t _object_id, class_permissions _permissions)
		{
			json_parser jp;
			json obj = jp.create_array();

			read_class_sp classd = read_lock_class(_class_name);
			if (not classd)
				return obj;

			auto perm = get_class_permission(_permissions.user_name, _class_name);
			bool exists = false;
			json oneobj= classd->get_object(this, _object_id, perm, exists);
			if (oneobj.object())
                obj.push_back(oneobj);

			return obj;
		}


		json select_object(json _key, bool _children, class_permissions _permission)
		{
			json_parser jp;
			json obj = jp.create_array();

			_key.set_natural_order();

			std::string class_name = _key[class_name_field].as_string();

			read_class_sp classd = read_lock_class(class_name);
			if (not classd)
				return obj;

			auto perm = get_class_permission(_permission.user_name, class_name);

			obj = classd->get_objects(this, _key, _children, perm);

			return obj;
		}

		json select_single_object(json _key, bool _children, class_permissions _permission)
		{
			json result = select_object(_key, _children, _permission);
			result = result.get_first_element();
			return result;
		}

		thread_safe_map<std::string, json> user_cache;

		void put_user(json _user)
		{
			json_parser jp;

			_user.erase_member("current_team");
			_user.erase_member("home_team");
			_user.erase_member("allowed_teams");
			_user.erase_member("create_options");
			json children = jp.create_array();
			json items = jp.create_array();
			items.push_back(_user);
			json request = create_system_request(items);
			json put_result = put_object(request);
			if (!put_result[success_field].as_bool()) {
	
			}
            user_cache.erase(_user[user_name_field].as_string());
		}

		void put_error(std::string _system, std::string _message, json& _body, std::string _file, int _line)
		{
			json_parser jp;
			json error = jp.create_object();
			error.put_member(class_name_field, std::string("sys_error"));
			error.put_member("system", _system);
			error.put_member("message", _message);
			error.put_member("file", _file);
			error.put_member("line", _line);
			error.share_member("body", _body);
            json create_object_request = create_system_request(error);
			json response = put_object(error);
			if (response[success_field].as_bool()==false) {
				system_monitoring_interface::active_mon->log_warning("put_error failed", __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json<json>(response);
			}
		}

		json get_user(std::string _user_name, bool _run_teams = true)
		{
			json_parser jp;
			json value;

            if (user_cache.try_get(_user_name, value)) {
				return value;
			}

			json key = jp.create_object();
			key.put_member(user_name_field, _user_name);

			auto classd = read_get_class("sys_user");
			if (not classd)
				return jp.create_array();

            auto permission = get_system_permission();

			json users = classd->get_objects(this, key, true, permission);

			json user = users.get_first_element();
			if (user.object()) {

				std::map<std::string, bool> allowed_teams;
				json jall_allowed_teams = jp.create_array();

				std::string team_name = user["team_name"].as_string();
				if (not team_name.empty()) {
					if (_run_teams) {
						json team_data = run_team(team_name, permission);
						if (team_data.object()) {
							user.share_member("current_team", team_data);
							json jallowed_teams = team_data["allowed_teams"];
							if (jallowed_teams.array()) {
								for (json jteam_name : jallowed_teams) {
									std::string atm = jteam_name.as_string();
									allowed_teams[atm] = true;
									jall_allowed_teams.push_back(atm);
								}
							}
						}
					}
				}
				team_name = user["home_team_name"].as_string();
				if (not team_name.empty()) {
					json team_data = run_team(team_name, permission);
					if (team_data.object()) {
						user.share_member("home_team", team_data);
						json jallowed_teams = team_data["allowed_teams"];
						if (jallowed_teams.array()) {
							for (json jteam_name : jallowed_teams) {
								std::string atm = jteam_name.as_string();
								if (not allowed_teams.contains(atm)) {
									allowed_teams[atm] = true;
									jall_allowed_teams.push_back(atm);
								}
							}
						}
					}
				}
				user.share_member("allowed_teams", jall_allowed_teams);
				user_cache.insert(_user_name, user);
			}

			return user;
		}

		json get_team_by_email(std::string _domain, class_permissions _permission)
		{
			json_parser jp;

			json query_details = jp.create_object();
            query_details.put_member("class_name", std::string("sys_team"));	
			json all_teams = query_class(default_user, query_details);
			json found_teams = jp.create_array();

			if (all_teams[success_field].as_bool()) {
                all_teams = all_teams[data_field];
				if (all_teams.array()) {
					for (int ix = 0; ix < all_teams.size(); ix++) {
						json team = all_teams.get_element(ix);
						std::string team_name = team["team_name"].as_string();
						std::string team_domain = team["team_domain"].as_string();
						if (team_domain.size() == 0 || team_domain == "none")
							continue;
						if (team_domain == "any")
						{
							json actual_team = get_team(team_name, _permission);
							found_teams.push_back(actual_team);
						}
						else 
						{
							try {
								
								if (_domain.ends_with(team_domain)) {
									json actual_team = get_team(team_name, _permission);
									found_teams.push_back(actual_team);
								}
								else {
									system_monitoring_interface::active_mon->log_information(std::format("Not matched domain '{0}' for team_domain '{1}' for '{2}'", _domain, team_domain, team_name), __FILE__, __LINE__);
								}
							}
							catch (std::exception exc)
							{
								system_monitoring_interface::active_mon->log_warning(std::format("Invalid regexp '{0}' for team_domain '{1}' for '{2}'", _domain, team_domain, team_name), __FILE__, __LINE__);
							}
						}
					}
				}
			}

			return found_teams;
		}

		json get_team(std::string _team_name, class_permissions _permission)
		{
			json_parser jp;

			json key = jp.create_object();
            key.put_member("team_name", _team_name);
			json query_details = jp.create_object();
			query_details.put_member("class_name", std::string("sys_team"));
			query_details.put_member("filter", key);
			json all_teams = query_class(default_user, query_details);
			json teams = jp.create_array();
			if (all_teams[success_field].as_bool()) {
				all_teams = all_teams[data_field];
				if (all_teams.array()) {
					for (int ix = 0; ix < all_teams.size(); ix++) {
						json team = all_teams.get_element(ix);
						json key = team.extract({ class_name_field, object_id_field });
						std::string names = team["team_name"].as_string();
						if (names != _team_name)
							continue;
                        std::string team_class_name = key[class_name_field].as_string();
						auto classd = read_lock_class(team_class_name);
						if (classd) {
							json full_teams = classd->get_objects(this, key, true, _permission);
                            json full_team = full_teams.get_first_element();
//                          system_monitoring_interface::active_mon->log_warning(std::format("Found team '{0}'", names), __FILE__, __LINE__);
//							system_monitoring_interface::active_mon->log_json(full_team);
							teams.push_back(full_team);
						}
					}
				}
			}
			return teams.get_first_element();
		}

		json run_team(std::string _team_name, class_permissions _permission)
		{
			json_parser jp;

            json team = get_team(_team_name, _permission);
			if (team.object()) 
			{
				int64_t team_id = team[object_id_field].as_int64_t();

				json permissions = team["permissions"];

				json leaf_classes = jp.create_array();
				json root_classes = jp.create_array();
				if (permissions.array()) 
				{
					for (auto grant : permissions) 
					{
						json granted_all = jp.create_object();
						auto class_list = grant["grant_classes"];
						if (class_list.array()) 
						{
							for (auto cls : class_list) 
							{
								std::string class_name = cls.as_string();
								json descendants = jp.create_array();

								auto tempdescendants = get_class_descendants(class_name);

                                for (auto tempdesc : tempdescendants) {
									descendants.push_back(tempdesc);
								}

								granted_all.put_member(class_name, descendants);
							}
						}
						grant.put_member("all_granted_classes", granted_all);
						grant.put_member("leaf_classes", leaf_classes);
						grant.put_member("root_classes", root_classes);
					}
				}

			}
			return team;
		}

		json get_schema(std::string schema_name, std::string schema_version, class_permissions _permission)
		{
			json_parser jp;
			json key = jp.create_object();

			key.put_member("schema_name", schema_name);
			key.put_member("schema_version", schema_version);

			auto classd = read_get_class("sys_schema");
			if (not classd) {
				return jp.create_array();
			}
			json data = classd->get_objects(this, key, true, _permission);

			return data.get_first_element();
		}

		json get_dataset(std::string dataset_name, std::string dataset_version, class_permissions _permission)
		{
			json_parser jp;
			json key = jp.create_object();
			key.put_member("dataset_name", dataset_name);
			key.put_member("dataset_version", dataset_version);

			auto classd = read_get_class("sys_dataset");
			if (not classd) {
				return json();
			}
			json data = classd->get_objects(this, key, true, _permission);

			return data.get_first_element();
		}

		virtual class_permissions get_system_permission()
		{
			class_permissions grants;
			grants.user_name = default_user;
            grants.team_name = "systemonlyfans";
			grants.alter_grant = class_grants::grant_any;
			grants.put_grant = class_grants::grant_any;
			grants.get_grant = class_grants::grant_any;
			grants.delete_grant = class_grants::grant_any;
			return grants;
		}

        thread_safe_map<std::string, class_permissions> permissions_cache;

		virtual class_permissions get_team_permissions(std::string user_name, std::string team_name, std::string _class_name)
		{

			class_permissions value;

            if (permissions_cache.try_get(team_name, value)) {
                value.user_name = user_name;
				return value;
			}

			auto sys_perm = get_system_permission();
			json jteam = get_team(team_name, sys_perm);

			class_permissions grants;

			grants.user_name = user_name;
            grants.team_name = team_name;
			grants.alter_grant = class_grants::grant_none;
			grants.put_grant = class_grants::grant_own;
			grants.get_grant = class_grants::grant_own;
			grants.delete_grant = class_grants::grant_none;

			if (jteam.object()) {
				json jpermissions = jteam["permissions"];
				if (jpermissions.array()) {
					for (auto jperm : jpermissions) {
						json class_array = jperm["grant_classes"];
						std::vector<std::string> granted_classes;
						if (class_array.array())
						{
							for (auto jcls : class_array) {
								granted_classes.push_back(jcls.as_string());
							}
						}
						else {
							granted_classes.push_back(class_array.as_string());
						}
						bool granted = false;
                        std::map<std::string, bool> visited;
						for (std::string& jclass : granted_classes) {
							auto permclass = read_lock_class(jclass);
							if (permclass) {

								get_class_descendants(visited, jclass);

								if (visited.contains(_class_name)) {
									std::string permission = jperm[class_permission_get].as_string();
									if (permission == "any")
										grants.get_grant = class_grants::grant_any;
									else if (permission == "own")
										grants.get_grant = class_grants::grant_own;
									else if (permission == "teamorown")
										grants.get_grant = class_grants::grant_teamorown;

									permission = jperm[class_permission_put].as_string();
									if (permission == "any")
										grants.put_grant = class_grants::grant_any;
									else if (permission == "own")
										grants.put_grant = class_grants::grant_own;
									else if (permission == "teamorown")
										grants.put_grant = class_grants::grant_teamorown;

									permission = jperm[class_permission_delete].as_string();
									if (permission == "any")
										grants.delete_grant = class_grants::grant_any;
									else if (permission == "own")
										grants.delete_grant = class_grants::grant_own;
									else if (permission == "teamorown")
										grants.delete_grant = class_grants::grant_teamorown;

									permission = jperm[class_permission_alter].as_string();
									if (permission == "any")
										grants.alter_grant = class_grants::grant_any;
									else if (permission == "own")
										grants.alter_grant = class_grants::grant_own;
									else if (permission == "teamorown")
										grants.alter_grant = class_grants::grant_teamorown;

									granted = true;
									break;
								}
							}
						}
					}
				}
			}

            permissions_cache.insert(team_name, grants);
			return grants;
		}

		virtual class_permissions get_class_permission(
			std::string _user_name,
			std::string _class_name) override
		{

			class_permissions grants;

			grants.user_name = _user_name;
			grants.alter_grant = class_grants::grant_none;
			grants.put_grant = class_grants::grant_own;
			grants.get_grant = class_grants::grant_own;
			grants.delete_grant = class_grants::grant_none;

			// extract the user key from the token and get the user object
			if (_user_name == default_user) 
			{
				grants = get_system_permission();
				return grants;
			}

			json user =  get_user(_user_name, false);

			if (not user.empty()) 
			{			
				auto home_grants = grants;
				std::string team_name = user["home_team_name"].as_string();
				home_grants.team_name = team_name;
				home_grants = get_team_permissions(_user_name, team_name, _class_name);

				auto team_grants = grants;
				team_name = user["team_name"].as_string();
				team_grants.team_name = team_name;
				team_grants = get_team_permissions(_user_name, team_name, _class_name);

				grants = home_grants | team_grants | grants;
				grants.team_name = team_name;
				grants.user_name = _user_name;
			}
			else 
			{
                grants.alter_grant = class_grants::grant_none;
                grants.delete_grant = class_grants::grant_none;
                grants.get_grant = class_grants::grant_none;
                grants.put_grant = class_grants::grant_none;	
				grants.team_name = std::format("permissions for user '{0}' not found", _user_name);
				system_monitoring_interface::active_mon->log_warning(grants.team_name, __FILE__, __LINE__);
			}

			if (_class_name == "sys_user") 
			{
				grants.put_grant = class_grants::grant_own;
			}

			return grants;
		}

		thread_safe_map<std::string, std::shared_ptr<class_interface>> class_cache;

		int64_t maximum_record_cache_size_bytes = giga_to_bytes(1);

		bool send_user_confirmation(json user_info, std::string email_template)
		{
			bool success = false;
			try {
				std::string new_code = get_random_code();

				user_info.put_member("validation_code", new_code);
				user_info.put_member("confirmed_code", 0);
				auto sys_perm = get_system_permission();
				put_user(user_info);

				sendgrid_client sc_client;
				sc_client.sender_email = sendgrid_sender_email;
				sc_client.sender_name = sendgrid_sender;
				sc_client.api_key = connections.get_connection("sendgrid");

				if (sc_client.api_key.empty()) {
                    json_parser jp;
					json blank = jp.create_object();
					system_monitoring_interface::active_mon->log_warning("Sendgrid configuration is missing Api Key.  Be sure to set the CONNECT_SENDGRID environment variable.", __FILE__, __LINE__);
					put_error("SendGrid", "Sendgrid configuration is missing Api Key.  Be sure to set the CONNECT_SENDGRID environment variable.", blank, __FILE__, __LINE__);
				}

				if (email_template.empty()) {
					email_template = R"(<html><body><h2>$EMAIL_TITLE$</h2><p>Username is $USERNAME$</p><p>Validation code <span style="background:grey;border:1px solid black;padding 8px;">$CODE$</p></body></html>)";
				}

				std::string user_name = user_info[user_name_field].as_string();

				std::string email_body = replace(email_template, "$CODE$", new_code);
				email_body = replace(email_body, "$USERNAME$", user_name);
				email_body = replace(email_body, "$EMAIL_TITLE$", user_confirmation_title);
				auto sg_response = sc_client.send_email(user_info, user_confirmation_title, email_body, "text/html");
				if (sg_response.response.http_status_code > 299 or sg_response.response.http_status_code < 200) {
					system_monitoring_interface::active_mon->log_warning(std::format("Send email {} to {}", user_confirmation_title, user_name), __FILE__, __LINE__);
					if (sg_response.response.response_body.is_safe_string()) {
						system_monitoring_interface::active_mon->log_warning(sg_response.response.response_body.get_ptr(), __FILE__, __LINE__);
					}
					else {
						system_monitoring_interface::active_mon->log_warning("Response body is not a string", __FILE__, __LINE__);
					}
				}
				success = true;
			}
			catch (std::exception exc)
			{
				system_monitoring_interface::active_mon->log_warning(exc.what(), __FILE__, __LINE__);
			}
			return success;
		}


	public:

		std::string default_user;
		std::string default_password;
		std::string default_email_address;
		std::string default_guest_team;
		std::string default_onboard_email_filename;
		std::string default_recovery_email_filename;
		std::string default_onboard_email;
		std::string default_recovery_email;

		time_span token_life;

		std::string default_api_title;
		std::string default_api_description;
		std::string default_api_version;
		std::string default_api_author;

		std::string sendgrid_sender;
		std::string sendgrid_sender_email;
		std::string user_confirmation_title;
		bool database_shutdown = false;
		std::string styles_file_name;
		std::string pages_file_name;

		json system_config;
		json server_config;

		// constructing and opening a database


		corona_database()
		{
			data_path = "/";
			config_path = "/";
			token_life = time_span(1, time_models::days);

			default_user = "system";
			default_password = "TestDefaultPassword123456!";
			default_email_address = "system@example.com";
			default_guest_team = "Guests";
			default_api_title = "API";
			default_api_description = "API Description";
			default_api_version = "1.0";
			default_api_author = "System";
			shutdown_in_progress = false;
			database_shutdown = false;
		}

        corona_database(std::string _data_path, std::string _config_path) : corona_database()
		{
            data_path = _data_path;
            config_path = _config_path;
			database_shutdown = false;
		}

		void shutdown()
		{
			if (database_shutdown) {
				return;
            }
			shutdown_in_progress = true;
			while (calls_in_progress.load() != 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
            corona::global_job_queue->waitForEmptyQueue();
            shutdown_in_progress = false;
			database_shutdown = true;
		}

		virtual ~corona_database()
		{
			shutdown();
		}

		virtual void apply_config(json _system_config)
		{
			progress_fence progress(&calls_in_progress);
			if (shutdown_in_progress) {
				return;
            }

			json_parser jp;
			json system_config;

			system_config = jp.create_object();
			server_config = jp.create_object();

            system_config.copy_member("Connections", _system_config);
			system_config.copy_member("SendGrid", _system_config);
            json servers = _system_config["Servers"];

			if (servers.array() && servers.size() > 0) {
                server_config = servers.get_element(0);
			}
			apply_config(system_config, server_config);
		}

		virtual void apply_config(json _system_config, json _server_config)
		{
			progress_fence progress(&calls_in_progress);

			if (shutdown_in_progress) {
				return;
			}

			date_time start;
			start = date_time::now();
			timer tx;
			system_monitoring_interface::active_mon->log_job_start("apply_config", "start", start, __FILE__, __LINE__);


			if (_system_config.has_member("SendGrid"))
			{
				json send_grid = _system_config["SendGrid"];
				std::string send_grid_api_key = send_grid["ApiKey"].as_string();
				sendgrid_sender = send_grid["SenderName"].as_string();
				sendgrid_sender_email = send_grid["SenderEmail"].as_string();
				user_confirmation_title = send_grid["UserConfirmationTitle"].as_string();
				this->connections.set_connection("sendgrid", send_grid_api_key);
			}

			if (_system_config.has_member("Connections"))
			{
				json connections = _system_config["Connections"];
				for (auto connection : connections) {
                    std::string conn_name = connection["name"].as_string();
                    std::string conn_value = connection["connection"].as_string();
					this->connections.set_connection(conn_name, conn_value);
				}
			}

			json server = _server_config;
			default_user = server[sys_user_name_field].as_string();
			if (default_user.empty()) 			
			{
				default_user = "system";
            }

			std::string defaultpw = server[sys_user_password_field].as_string();

			if (not defaultpw.empty()) {
				default_password = server[sys_user_password_field].as_string();
			}

			default_email_address = server[sys_user_email_field].as_string();
			if (default_email_address.empty()) {
				default_email_address = std::format("dummy@example.com", default_user);
			}

			default_guest_team = server[sys_default_team_field].as_string();
			default_api_title = server[sys_default_api_title_field].as_string();
			default_api_description = server[sys_default_api_description_field].as_string();
			default_api_version = server[sys_default_api_version_field].as_string();
			default_api_author = server[sys_default_api_author_field].as_string();
            default_onboard_email_filename = server[sys_default_onboard_email_template].as_string();
			default_recovery_email_filename = server[sys_default_recovery_email_template].as_string();

			std::filesystem::path schema_path(config_path);
			schema_file_name = server["schema_filename"].as_string();

            if (not schema_file_name.empty()) {
				std::filesystem::path schema_file_path(schema_file_name);
				if (not schema_file_path.is_absolute()) {
					schema_file_path = schema_path / schema_file_path;
					schema_file_name = schema_file_path.string();
				}
			}

			styles_file_name = _server_config["styles_filename"].as_string();
			pages_file_name = _server_config["pages_filename"].as_string();

			if (not (default_onboard_email_filename.empty() or default_recovery_email_filename.empty()))
			{
				std::filesystem::path onboard_file(default_onboard_email_filename);
				std::filesystem::path recovery_file(default_recovery_email_filename);

				if (not onboard_file.is_absolute()) {
					onboard_file = schema_path / onboard_file;
				}
				if (not recovery_file.is_absolute()) {
					recovery_file = schema_path / recovery_file;
				}

				std::string obf = onboard_file.string();
				std::string rf = recovery_file.string();
				if (std::filesystem::exists(obf)) {
					default_onboard_email = read_all_string(obf);
				}
				if (std::filesystem::exists(rf)) {
					default_recovery_email = read_all_string(rf);
				}
			}

			if (server.has_member(sys_record_cache_field)) {
				maximum_record_cache_size_bytes = server[sys_record_cache_field].as_int64_t();
			}

			system_monitoring_interface::active_mon->log_job_stop("apply_config", "complete", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);

		}

        static const int max_code_length = 31;

		virtual std::string get_random_code(int _confirmation_code_digits = 8)
		{
			std::string s_confirmation_code = "";
			int confirmation_code_digits = _confirmation_code_digits;
			if (confirmation_code_digits > max_code_length) {
                confirmation_code_digits = max_code_length;
			} else if (max_code_length < 0) {
				confirmation_code_digits = 8;
            }
			char confirmation_code[max_code_length + 1] = {};

			int rc = 0;
			int lc = 0;
			int i = 0;
			int random_number;

			std::random_device rd; // Seed generator
			std::mt19937 gen(rd()); // Mersenne Twister engine
			std::uniform_int_distribution<> dist('A', 'A' + 26); // Range [1, 100]

			while (i < confirmation_code_digits and i < sizeof(confirmation_code))
			{
				do
				{
					random_number = dist(gen);
					rc = random_number % 26 + 'A';
					confirmation_code[i] = rc;
				} while (rc == lc);
				lc = rc;
				i++;
			}
			confirmation_code[i] = 0;
			s_confirmation_code = confirmation_code;
			return s_confirmation_code;
		}

		virtual std::shared_ptr<class_interface> read_get_class(const std::string& _class_name) 
		{
			progress_fence progress(&calls_in_progress);

			std::shared_ptr<class_interface> cd;

			activity act;
			act.db = this;
			cd = get_class_impl(&act, _class_name);

			return cd;
		}

		virtual read_class_sp read_lock_class(const std::string& _class_name) override
		{
			progress_fence progress(&calls_in_progress);

			std::shared_ptr<class_interface> cd = read_get_class(_class_name);
			return read_class_sp(cd);
		}

		virtual write_class_sp write_lock_class(const std::string& _class_name) override
		{
			progress_fence progress(&calls_in_progress);

			std::shared_ptr<class_interface> cd = read_get_class(_class_name);
			return write_class_sp(cd);
		}

		virtual json save_class(class_interface *_class_to_save)
		{
			progress_fence progress(&calls_in_progress);

			json_parser jp;
			json class_def;

			class_def = jp.create_object();
			_class_to_save->get_json(class_def);
            classes->put(class_def);
			classes->commit();
			return class_def;		
		}

        virtual json generate_card(read_class_sp& _class_definition)
		{
			json_parser jp;
			json jempty = jp.create_object();
			return jempty;
		}

		/*
		

								{
				  "class_name": "command_button",
				  "image": "day.png",
				  "box": "button_box",
				  "margin": "button_margin",
				  "text": "today",
				  "defaults_field": "common",
				  "on_click": {
					"class_name": "query",
					"form_name": "search_row",
					"table_name": "result_table",
					"detail_frame": "frame_selected",
					"query": {
					  "from": [
						{
						  "class_name": "task",
						  "name": "objects",
						  "filter": {
							"full_text": "$this.full_text"
						  }
						}
					  ],
					  "stages": [
						{
						  "class_name": "filter",
						  "source": "objects",
						  "name": "filtered",
						  "condition": {
							"class_name": "all",
							"conditions": [
							  {
								"class_name": "gte",
								"valuepath": "task_date",
								"value": "$this.common.dates.today.start"
																"value": "$this.common.dates.this_week.start"
							  },
							  {
								"class_name": "any",
								"conditions": [
								  {
									"class_name": "eq",
									"valuepath": "status",
									"value": "open"
								  },
								  {
									"class_name": "eq",
									"valuepath": "status",
									"value": "in progress"
								  },
								  {
									"class_name": "eq",
									"valuepath": "status",
									"value": "on hold"
								  },
								  {
									"class_name": "eq",
									"valuepath": "status",
									"value": "blocked"
								  }
								]
							  }
							]
						  }
						},
						{
						  "class_name": "result",
						  "source": "filtered",
						  "name": "output"
						}
					  ]
					}
				  }
				},

		*/

        virtual json read_json(validation_error_collection& _errors, std::string _file_name)
		{
			json_parser jp;
			json result = jp.from_file(_file_name);
			if (result[class_name_field].as_string() == parse_error_class) {
				json errs = result["errors"].as_array();
                for (int i = 0; i < errs.size(); i++) {
					json err = errs.get_element(i);
					validation_error ve;
                    ve.filename = _file_name;
                    ve.line_number = err["line"].as_int();
                    ve.message = err["error"].as_string();
					ve.field_name = err["topic"].as_string();
					ve.class_name = _file_name;
					_errors.push_back(ve);
				}
			}
			if (result.empty()) {
				validation_error ve;
				ve.filename = _file_name;
				ve.line_number = 0;
				ve.message = std::format("File {0} is empty or not found", _file_name);
				ve.field_name = "";
				ve.class_name = _file_name;
				_errors.push_back(ve);
            }
			return result;
		}

		virtual json update_application(json _application_schema, std::string _class_name) override
		{
			validation_error_collection errors;

			std::string version_string = _application_schema["schema_version"].as_string();
            std::vector<std::string> version_parts = split(version_string, '.');

            auto& last_string = version_parts.back();
            int version_number = std::strtod(last_string.c_str(), nullptr);
			version_number++;
            std::string new_version_string = join(version_parts, ".");
            _application_schema.put_member("schema_version", new_version_string);

			json_parser jp;
			json result_pages = jp.create_object();

			json field_mappings = _application_schema["fields"];
			json class_mappings = _application_schema["classes"];
			json team_mappings = _application_schema["teams"];
			json object_list = _application_schema["objects"];

			json result = jp.create_object();
			json pages = jp.create_array();
			json card_sources = jp.create_object();
			json form_sources = jp.create_object();

			json section_box = field_mappings[".section"]["box"];

			// config path always ends with a separator

			// read the templates from the files.  everything is json object and we're just manipulating them
			// in a C++ equivalent of what node.js might do.

			json chart_template = read_json(errors, this->config_path + "source_templates\\chart.json");

			json card_template = read_json(errors, this->config_path + "source_templates\\card.json");

			json chest_item_card = read_json(errors, this->config_path + "source_templates\\card_chest_item.json");
			json card_container_template = read_json(errors, this->config_path + "source_templates\\card_container.json");
			json object_template = read_json(errors, this->config_path + "source_templates\\object.json");
			json object_details = read_json(errors, this->config_path + "source_templates\\object_details.json");
			json object_container_template = read_json(errors, this->config_path + "source_templates\\object_container.json");

			json tab_edit_template = read_json(errors, this->config_path + "source_templates\\tab_edit.json");
			json tab_list_template = read_json(errors, this->config_path + "source_templates\\tab_list.json");
			json tab_inventory_template = read_json(errors, this->config_path + "source_templates\\tab_inventory.json");
			json tab_custom_template = read_json(errors, this->config_path + "source_templates\\tab_custom.json");
			json tab_container_template = read_json(errors, this->config_path + "source_templates\\tab_container.json");
			json home_template = read_json(errors, this->config_path + "source_templates\\home.json");
			json home_page = home_template.clone();

			home_page.apply_abbreviations({
				{ "$corporate_name", _application_schema["application_company"] },
				{ "$title_name",  _application_schema["application_name"] }
				});

			json pages_template = read_json(errors, this->config_path + "source_templates\\pages.json");
			json styles_template = read_json(errors, this->config_path + "source_templates\\styles.json");

			json search_template = read_json(errors, this->config_path + "source_templates\\search.json");
			json search_group_template = read_json(errors, this->config_path + "source_templates\\search_group.json");
			json search_command_class_template = read_json(errors, this->config_path + "source_templates\\search_command_class.json");
			json create_command_class_template = read_json(errors, this->config_path + "source_templates\\create_command_class.json");
			json chart_command_class_template = read_json(errors, this->config_path + "source_templates\\chart_command_class.json");

			json pages_array = pages_template["pages"];
			json abbreviations = pages_template["abbreviations"];

			pages.push_back(chest_item_card);
			pages.push_back(card_container_template);
			pages.push_back(object_container_template);
			pages.push_back(tab_container_template);
			pages.push_back(object_details);
			pages.push_back(home_page);

			std::map<std::string, bool> card_fields;

			if (errors.size() > 0) {
				log_errors(errors);
				return jp.create_object();
			}

			json ja = _application_schema["card_fields"];
			if (ja.array()) {
				for (int i = 0; i < ja.size(); i++) {
					std::string field_name = ja.get_element(i).as_string();
					card_fields[field_name] = true;
				}
			}

			// we're going to go through the list of classes here, applying ux maps for each class, and generating pages based on the templates for each class.  
			// The ux maps will determine which fields are included in the generated pages, and the templates will determine the layout and structure of the pages.
			// The generated pages will then be added to the pages array, and the pages template will be updated to include the new pages.  
			// Finally, the pages template and styles template will be saved to the config path.

			std::string class_name = _class_name;

			auto classd = read_lock_class(class_name);
			if (classd) {

				json class_ux_map;

				class_ux_map = class_mappings[class_name];

				if (class_ux_map.is_string() && class_ux_map.as_string() == "default")
				{
					class_ux_map = create_ux_map(errors, classd, card_fields, field_mappings);
				}

				if (!class_ux_map.empty())
				{
					// Generate card page
					json card_pages = generate_card_pages(errors, card_template, classd, class_ux_map, field_mappings);
					if (!card_pages.empty()) {
						card_sources.put_member(class_name, std::format("card_{}", class_name));
						pages.push_back_array(card_pages);
					}

					// Generate object details page
					json object_pages = generate_object_pages(errors, object_template, classd, class_ux_map, field_mappings, tab_list_template, tab_custom_template, tab_inventory_template, tab_edit_template, create_command_class_template, form_sources, section_box);
					if (!object_pages.empty()) {
						form_sources.put_member(class_name, std::format("object_{}", class_name));
						pages.push_back_array(object_pages);
					}

					if (errors.size() > 0) {
						log_warning("UX NOT CREATED");
						log_errors(errors);
						return jp.create_object();
					}

                    std::filesystem::path pages_path(this->config_path + "pages");
                    std::filesystem::create_directory(pages_path);

					for (int i = 0; i < pages.size(); i++)
					{
						json page = pages.get_element(i);
						std::string page_name = page["page_name"].as_string();
						std::string filename = pages_path.string() + page_name + ".json";

						json page_import = jp.create_object();
						page_import.put_member_string("class_name", "import");
						page_import.put_member("file_name", filename);

						pages_array.push_back(page_import);

						std::string page_file_path = this->config_path + filename;
						page.save(page_file_path);

						std::string override_path = this->config_path + "overrides\\" + page_name + ".json";
						if (std::filesystem::exists(override_path)) {
							json override_page = read_json(errors, override_path);
							if (not override_page.empty()) {
								override_page.save(page_file_path);
							}
						}
					}

					result_pages.put_member("pages", pages_array);
				}			
			}

			return result_pages;
		}

		virtual json create_application(json _application_schema) override
		{

			validation_error_collection errors;

			json_parser jp;

			json field_mappings = _application_schema["fields"];
			json class_mappings = _application_schema["classes"];
			json team_mappings = _application_schema["teams"];
			json object_list = _application_schema["objects"];

			json result = jp.create_object();
			json pages = jp.create_array();
			json card_sources = jp.create_object();
			json form_sources = jp.create_object();

			json section_box = field_mappings[".section"]["box"];

			// and objects

			if (object_list.array()) {
				for (int j = 0; j < object_list.size(); j++) {
					json object_definition = object_list.get_element(j).clone();
					json put_object_request = create_system_request(object_definition);
					json create_result = put_object(put_object_request);
					if (not create_result[success_field].as_bool()) {
						system_monitoring_interface::active_mon->log_warning(create_result[message_field].as_string());
						log_error_array(create_result);
					}
					else {
						json result = create_result[data_field];
						for (auto class_result : result.get_members()) {
							json items = class_result.second;
							if (items.array()) {
								for (auto item : items) {
									if (not item[success_field].as_bool()) {
										system_monitoring_interface::active_mon->log_warning(item[message_field].as_string(), __FILE__, __LINE__);
									}
									else {
										json item_data = item[data_field];
										std::string new_class_name = item_data[class_name_field].as_string();
										int64_t object_id = item_data[object_id_field].as_int64_t();
										std::string object_created = std::format("object {0} {1} saved", new_class_name, object_id);
										system_monitoring_interface::active_mon->log_information(object_created);
									}
								}
							}
						}
					}
				}
			}

            // get the list of classes from the database

			json class_list = classes->select(nullptr, jp.create_object(), [](json& _item)->json {
				return _item;
				});

			// config path always ends with a separator

			// read the templates from the files.  everything is json object and we're just manipulating them
			// in a C++ equivalent of what node.js might do.

            json chart_template = read_json(errors, this->config_path + "source_templates\\chart.json");
			
			json card_template = read_json(errors, this->config_path + "source_templates\\card.json");

            json chest_item_card = read_json(errors, this->config_path + "source_templates\\card_chest_item.json");
			json card_container_template = read_json(errors, this->config_path + "source_templates\\card_container.json");
			json object_template = read_json(errors, this->config_path + "source_templates\\object.json");
			json object_details = read_json(errors, this->config_path + "source_templates\\object_details.json");
			json object_container_template = read_json(errors, this->config_path + "source_templates\\object_container.json");

			json tab_edit_template = read_json(errors, this->config_path + "source_templates\\tab_edit.json");
			json tab_list_template = read_json(errors, this->config_path + "source_templates\\tab_list.json");
			json tab_inventory_template = read_json(errors, this->config_path + "source_templates\\tab_inventory.json");
			json tab_custom_template = read_json(errors, this->config_path + "source_templates\\tab_custom.json");
			json tab_container_template = read_json(errors, this->config_path + "source_templates\\tab_container.json");
			json home_template = read_json(errors, this->config_path + "source_templates\\home.json");
			json home_page = home_template.clone();

			home_page.apply_abbreviations({
				{ "$corporate_name", _application_schema["application_corporate_title"] },
				{ "$title_name",  _application_schema["application_title"] },
				{ "$subtitle_name",  _application_schema["application_subtitle"] }
                });

			json pages_template = read_json(errors, this->config_path + "source_templates\\pages.json");
			json styles_template = read_json(errors, this->config_path + "source_templates\\styles.json");

            json search_template = read_json(errors, this->config_path + "source_templates\\search.json");
            json search_group_template = read_json(errors, this->config_path + "source_templates\\search_group.json");
			json search_command_class_template = read_json(errors, this->config_path + "source_templates\\search_command_class.json");
			json create_command_class_template = read_json(errors, this->config_path + "source_templates\\create_command_class.json");
			json chart_command_class_template = read_json(errors, this->config_path + "source_templates\\chart_command_class.json");

			json pages_array = pages_template["pages"];
            json abbreviations = pages_template["abbreviations"];

            pages.push_back(chest_item_card);
			pages.push_back(card_container_template);
			pages.push_back(object_container_template);
			pages.push_back(tab_container_template);
			pages.push_back(object_details);
			pages.push_back(home_page);

			std::map<std::string, bool> card_fields;

			if (errors.size() > 0) {
				log_errors(errors);
				return jp.create_object();
			}

            json ja = _application_schema["card_fields"];
			if (ja.array()) {
				for (int i = 0; i < ja.size(); i++) {
					std::string field_name = ja.get_element(i).as_string();
					card_fields[field_name] = true;
				}
            }

            // we're going to go through the list of classes here, applying ux maps for each class, and generating pages based on the templates for each class.  
			// The ux maps will determine which fields are included in the generated pages, and the templates will determine the layout and structure of the pages.
            // The generated pages will then be added to the pages array, and the pages template will be updated to include the new pages.  
			// Finally, the pages template and styles template will be saved to the config path.

			if (class_list.array()) {
				for (auto& item : *class_list.array_impl()) {
					std::shared_ptr<json_object> jo = std::dynamic_pointer_cast<json_object>(item);
					if (jo && jo->has_member(class_name_field)) {
						std::string class_name = (*jo)[class_name_field]->to_string();

						auto classd = read_lock_class(class_name);
						if (!classd) continue;

						json class_ux_map;

						class_ux_map = class_mappings[class_name];

                        if (class_ux_map.is_string() && class_ux_map.as_string() == "default") 
						{
							class_ux_map = create_ux_map(errors, classd, card_fields, field_mappings);
						}

						if (class_ux_map.empty()) 
						{
							continue;
						}

						// Generate card page
						json card_pages = generate_card_pages(errors, card_template, classd, class_ux_map, field_mappings);
						if (!card_pages.empty()) {
                            card_sources.put_member(class_name, std::format("card_{}", class_name));
							pages.push_back_array(card_pages);
						}

						// Generate object details page
						json object_pages = generate_object_pages(errors, object_template, classd, class_ux_map, field_mappings, tab_list_template, tab_custom_template, tab_inventory_template, tab_edit_template, create_command_class_template, form_sources, section_box);
						if (!object_pages.empty()) {
							form_sources.put_member(class_name, std::format("object_{}", class_name));
							pages.push_back_array(object_pages);
						}

						if (errors.size() > 0) {
							log_warning("UX NOT CREATED");
							log_errors(errors);
							return jp.create_object();
						}
					}
				}
			}

			// here we build the search pages.  each team gets a search page.

			if (team_mappings.object()) {
				auto tms = team_mappings.get_members();

				for (auto tm : tms) {
					json team_page = search_template.clone();

					std::string team_name = tm.first;
					json team_spec = tm.second;
					json search_groups = team_spec["search_groups"];
					team_page.put_member("page_name", "home_" + team_name);
                    json ux_search_groups = jp.create_array();

					if (search_groups.array()) {


						for (int isg = 0; isg < search_groups.size(); isg++) {
							json sg = search_groups.get_element(isg);
							auto sg_classes = sg["classes"].to_string_array();
							auto sg_title = sg["title"].as_string();
							json charts = sg["charts"];

							json search_group = search_group_template.clone();
							json search_group_commands = jp.create_array();

							std::string sg_list_title_text = sg["list_title_text"].as_string();
							std::string sg_edit_title_text = sg["edit_title_text"].as_string();
							std::string sg_section_title_text = sg["section_title_text"].as_string();

							json sg_new_objects = sg["new_objects"];
							std::string sg_search_commands_control = sg["commands_control"].as_string();
							std::string chart_name = sg["chart_name"].as_string();

							json chart_pages = chart_template.clone();

							chart_pages.apply_abbreviations(
								{
									{ "$chart_name", jp.from_string(chart_name) },
									{ "$page_name", jp.from_string(chart_name) },
									{ "$chart_commands", jp.create_array() }
								});

							pages.push_back(chart_pages);

							std::set<std::string> new_object_classes;

							if (sg_new_objects.array()) {

								for (int i = 0; i < sg_new_objects.size(); i++) {
									auto option = sg_new_objects.get_element(i).as_string();
									new_object_classes.insert(option);
								}
							}

							json search_create_commands = jp.create_array();

							for (auto sg_class : new_object_classes) {

								// this is for the classes you can create in the search group.  like the options panel

								auto sgclassd = read_lock_class(sg_class);
								if (!sgclassd) {
									log_warning("class " + sg_class + " not found", __FILE__, __LINE__);
									return jp.create_object();
								}

								json create_command = create_command_class_template.clone();

								create_command.apply_abbreviations({
									{ "$create_button_class", jp.from_string(sgclassd->get_class_name()) },
									{ "$create_button_name", jp.from_string("create_" + sgclassd->get_class_name() + "_button") },
									{ "$create_button_image", jp.from_string(std::format("{}.png", sgclassd->get_class_name())) },
									{ "$create_button_text", jp.from_string(sgclassd->get_class_name()) },
									{ "$create_button_message", jp.from_string("new " + sgclassd->get_class_name()) },
									{ "$class_edit_page", form_sources[sgclassd->get_class_name()] }
									});
								search_create_commands.push_back(create_command);
							}

							if (charts.array()) {

								// these are the charts that get tacked on.
								for (int i = 0; i < charts.size(); i++) {

									json chart_command = chart_command_class_template.clone();
									json chart_spec = charts.get_element(i);

									std::string chart_class = chart_spec["chart_class"].as_string();
									std::string chart_button_name = chart_spec["chart_button_name"].as_string();
									std::string chart_button_text = chart_spec["chart_button_text"].as_string();
									std::string chart_button_image = chart_spec["chart_button_image"].as_string();

									json chart_actual = chart_spec["chart"];
									json chart_group_by = chart_spec["group_by"];
									json chart_cross_tab = chart_spec["cross_tab"];

									chart_command.apply_abbreviations({
										{ "$chart_button_name", jp.from_string(chart_button_name) },
										{ "$chart_button_text", jp.from_string(chart_button_text) },
										{ "$chart_button_image", jp.from_string(chart_button_image) },
										{ "$chart_name", jp.from_string(chart_name) },
										{ "$chart_group_by", chart_group_by },
										{ "$chart_cross_tab", chart_cross_tab },
										{ "$chart", chart_actual }
										});

									search_create_commands.push_back(chart_command);
								}

							}

                            for (auto sg_class : sg_classes) {

								json search_command = search_command_class_template.clone();

								auto sgclassd = read_lock_class(sg_class);
								if (!sgclassd) {
									log_warning("class " + sg_class + " not found", __FILE__, __LINE__);
									return jp.create_object();
								}

								// this is the guy that decorates the main search button that is next to the search text
								json update_filter = jp.create_object();
								update_filter.put_member(class_name_field, sg_class);

								search_command.apply_abbreviations({
	{ "$search_button_class", jp.from_string(sgclassd->get_class_name()) },
	{ "$search_button_name", jp.from_string("query_" + sgclassd->get_class_name() + "_button") },
	{ "$search_button_image", jp.from_string(std::format("{}.png", sgclassd->get_class_name())) },
	{ "$search_button_text", jp.from_string(sgclassd->get_class_name()) },
	{ "$section_title_text", jp.from_string(sg_section_title_text) },
	{ "$list_title_text", jp.from_string(sg_list_title_text) },
	{ "$edit_title_text", jp.from_string(sg_edit_title_text) },
	{ "$create_commands", search_create_commands },
	{ "$commands_control", jp.from_string(sg_search_commands_control) },
	{ "$update_filter", update_filter }
									});

                                search_group_commands.push_back(search_command);
							}

							search_group.apply_abbreviations({
	{ "$search_group_name", jp.from_string(sg_title) },
	{ "$search_group_commands", search_group_commands }
								});


							ux_search_groups.push_back(search_group);
						}
						team_page.apply_abbreviations({
							{ "$search_groups", ux_search_groups }
						});

					}

					pages.push_back(team_page);
				}
			}

			// now we're going to write the pages
			// while building references to the other pages in the master page.

			for (int i = 0; i < pages.size(); i++) 
			{
				json page = pages.get_element(i);
				std::string page_name = page["page_name"].as_string();
				std::string filename = "pages\\" + page_name + ".json";

				json page_import = jp.create_object();
				page_import.put_member_string("class_name", "import");
				page_import.put_member("file_name", filename);

				pages_array.push_back(page_import);

                std::string page_file_path = this->config_path + filename;
                page.save(page_file_path);

				std::string override_path = this->config_path + "overrides\\" + page_name + ".json";
                if (std::filesystem::exists(override_path)) {
                    json override_page = read_json(errors, override_path);
                    if (not override_page.empty()) {
                        override_page.save(page_file_path);
                    }
                }
			}

            abbreviations.put_member("card_sources", card_sources);
            abbreviations.put_member("form_sources", form_sources);

			std::string styles_file_path = this->config_path + styles_file_name;
            styles_template.save(styles_file_path);
			std::string pages_file_path = this->config_path + pages_file_name;
            pages_template.save(pages_file_path);

            result.put_member("pages_template", pages_template);
			result.put_member("styles_template", styles_template);
			result.put_member("pages", pages);

			return result;
		}

private:


	/*
	
	    "card": {
          "title": "description",
          "fields": [
            {
              "field_name": "name",
              "expression": ""
            }
          ],
          "commands": []
        },
        "tab_edit": {
          "fields": [
            {
              "field_name": "name"
            }
          ],
          "commands": []
        },
        "page": {
          "tabs": [
            {
              "name": "My Tab",
              "page_name": "tab_list",
              "member_name": "member",
              "list": true
            }
          ]
        }
*/

	json create_ux_map(validation_error_collection& vec, read_class_sp& classd, std::map<std::string, bool>& _card_fields, json& field_mappings)
	{
		json_parser jp;
		json ux_map = jp.create_object();

		json card = jp.create_object();
 		json tab_edit = jp.create_object();
		json page = jp.create_object();
        json tabs = jp.create_array();
		json default_tab = jp.create_object();
		json object_tab = jp.create_object();

		ux_map.put_member("card", card);
        ux_map.put_member("tab_edit", tab_edit);
        ux_map.put_member("page", page);

        auto fields = classd->get_fields();

        json card_fields = jp.create_array();
		json tab_edit_fields = jp.create_array();

        card.put_member("title", classd->get_class_name());

		auto field_sections = classd->get_fields_for_ux(this);

		for (auto& section : field_sections)
		{
			int section_fields_count = 0;

			for (auto& field : section->section_fields)
			{
				std::string field_class = field->get_field_class();
				std::string tab_name = "tab_" + classd->get_class_name() + "_" + field->get_field_name();
				json field_mapping = get_field_mapping(classd, field->get_field_name(), field_mappings);

				std::string field_ux_class = "";
				std::string field_tab_type = "";
				layout_rect field_box = {};

				if (field_mapping.object()) {
					field_tab_type = field_mapping["tab_type"].as_string();
					field_ux_class = field_mapping["class_name"].as_string();
					json jbox = field_mapping["box"];
					put_json(field_box, jbox);
				}

				if (field_tab_type == "edit") {
					section_fields_count++;
				}

			}

			if (section_fields_count > 0) {
				json tab_field = jp.create_object();
				 tab_field.put_member_string("field_name", ".section");
				tab_field.put_member_string("field_label", section->section_description);
				tab_field.put_member_string("field_class", section->section_name);
				tab_edit_fields.push_back(tab_field);
			}

			for (auto& field : section->section_fields)
			{
				std::string field_class = field->get_field_class();

				std::string tab_name = "tab_" + classd->get_class_name() + "_" + field->get_field_name();

				json field_mapping = get_field_mapping(classd, field->get_field_name(), field_mappings);

				std::string field_ux_class = "";
				std::string field_tab_type = "";
				layout_rect field_box = {};
                json jfield_box = jp.create_object();
				bool default_tab = field_mapping["default_tab"].as_bool();

				if (field_mapping.object()) {
					field_tab_type = field_mapping["tab_type"].as_string();
					field_ux_class = field_mapping["class_name"].as_string();
					jfield_box = field_mapping["box"];
					put_json(field_box, jfield_box);
				}

				if (field_tab_type == "custom" || field_tab_type == "inventory") {
					json tab = jp.create_object();
					tab.put_member_string("page_name", tab_name);
					tab.put_member_string("name", field->get_label());
					tab.put_member_string("member_name", field->get_field_name());
					tab.put_member_string("ux_class", field_ux_class);
					tab.put_member_string("tab_type", field_tab_type);
					tab.put_member_bool("default_tab", default_tab);
					tabs.push_back(tab);
				}
				else if (field_tab_type == "edit")
				{
					json tab_field = jp.create_object();
					tab_field.put_member_string("field_name", field->get_field_name());
					tab_field.put_member_string("field_label", field->get_label());
					tab_field.put_member_string("field_class", field->get_field_class());
                    tab_field.put_member_string("field_ux_class", field_ux_class);
                    tab_field.put_member("field_box", jfield_box);
					tab_edit_fields.push_back(tab_field);
				}
				else if (field->get_field_type() == field_types::ft_object) {
					if (!field->is_relational_children()) {
						json tab = jp.create_object();
						tab.put_member_string("page_name", tab_name);
						tab.put_member_string("name", field->get_label());
						tab.put_member_string("member_name", field->get_field_name());
						tab.put_member_string("ux_class", "scintilla");
						tab.put_member_string("tab_type", field_tab_type);
						tab.put_member_bool("default_tab", default_tab);
						tabs.push_back(tab);
					}
					else
					{
						std::shared_ptr<child_relations_interface> relations;
						relations = field->get_relations();

						std::shared_ptr<child_relation_interface> relation;
						if (relations) {
							relation = relations->get_relation(field_class);
						}

						json tab = jp.create_object();

						auto allowed_classes = field->get_allowed_classes();
						if (allowed_classes.size() > 0) {
							tab.put_member_string("ux_class", allowed_classes[0]);
						}
						if (relation) {
							json jcopy = relation->get_copy_assignment();
							tab.put_member("filter", jcopy);
						}

						tab.put_member_string("page_name", tab_name);
						tab.put_member_string("name", field->get_label());
						tab.put_member_string("member_name", field->get_field_name());
						tab.put_member_string("tab_type", "object");
						tab.put_member_bool("default_tab", default_tab);
						tabs.push_back(tab);
					}
				}
				else if (field->get_field_type() == field_types::ft_array) {
					json tab = jp.create_object();
					tab.put_member_string("page_name", tab_name);
					tab.put_member_string("name", field->get_label());
					tab.put_member_string("member_name", field->get_field_name());

					std::shared_ptr<child_relations_interface> relations;
					relations = field->get_relations();

					std::shared_ptr<child_relation_interface> relation;
					if (relations) {
						relation = relations->get_relation(field_class);
					}

					auto allowed_classes = field->get_allowed_classes();
					if (allowed_classes.size() > 0) {
						tab.put_member_string("ux_class", allowed_classes[0]);
					}
					if (relation) {
						json jcopy = relation->get_copy_assignment();
						tab.put_member("filter", jcopy);
					}
					tab.put_member_string("tab_type", "list");
					tab.put_member_bool("default_tab", default_tab);
					tabs.push_back(tab);
				}

				if (_card_fields.find(field->get_field_name()) != _card_fields.end()) {
					json new_card_field = jp.create_object();
					new_card_field.put_member("field_name", field->get_field_name());
					new_card_field.put_member("expression", field->get_field_name());
					card_fields.push_back(new_card_field);
				}

			}
		}


		std::string tab_name = "tab_" + classd->get_class_name() + "_fields";
		default_tab.put_member_string("page_name", tab_name);
		default_tab.put_member_string("name", classd->get_class_name());
		default_tab.put_member_string("member_name", ".");
		default_tab.put_member_string("tab_type", "edit");
		tabs.push_front(default_tab);

		page.put_member("tabs", tabs);
        card.put_member("fields", card_fields);
        tab_edit.put_member("fields", tab_edit_fields);

		return ux_map;
    }

	json generate_card_pages(validation_error_collection& vec, json& card_template, read_class_sp& classd, json& class_mappings, json& field_mappings)
		{
			json_parser jp;

            json pages = jp.create_array();

			json tab_edit_page = card_template.clone();

			const std::string class_name = classd->get_class_name();

			tab_edit_page.put_member("page_name", "card_" + class_name);

			// Set icon
			auto parameters = tab_edit_page.find_member("using.parameters");
			if (parameters.object()) {
				parameters.put_member("$icon_image_file", class_name + ".png");

				// Populate card contents
				auto contents = parameters["$card_contents"];
				if (contents.array()) {

					// Check if this field should be displayed on the card
					auto cfm = class_mappings["card"]["fields"];

                    if (not cfm.array()) {
						validation_error ve;
                        ve.class_name = class_name;
						ve.count = 1;
                        ve.message = "card.fields must be supplied for " + class_name;
                        ve.filename = __FILE__;
                        ve.line_number = __LINE__;
						
						return pages;
					}

					for (int i = 0; i < cfm.size(); i++) {
						auto field = cfm.get_element(i);

						json content = jp.create_object();
						content.put_member_string("class_name", "paragraph");
						content.put_member_string("box", "card_text_box");
						content.put_member("json_field_name", field["field_name"].as_string());
                        std::string expression = "{" + field["expression"].as_string() + "}";
						content.put_member_string("prompt", expression);
						content.put_member_string("text", expression);
						contents.push_back(content);
					}
				}

				pages.push_back(tab_edit_page);

			}


			return pages;
		}

        json get_field_mapping(read_class_sp& classd, const std::string& field_name, json& field_mappings)
		{
			std::string key = "." + field_name;
			json empty;

			if (field_mappings.has_member(key)) {
				return field_mappings[key];
			}

            key = classd->get_class_name() + "." + field_name;

			if (field_mappings.has_member(key)) {
				return field_mappings[key];
			}

			auto ancestors = classd->get_ancestors();
            for (auto ans : ancestors) {
				std::string ancestor_key = ans.first + "." + field_name;
				if (field_mappings.has_member(ancestor_key)) {
					return field_mappings[ancestor_key];
				}
			}

            auto field = classd->get_field(field_name);
			if (field) {
				auto ft = field->get_field_type();
                key = field_type_names[ft];

				if (field_mappings.has_member(key)) {
					return field_mappings[key];
				}
			}

			return empty;
		}

		json generate_tab_edit_page(validation_error_collection& _errors, json& tab_template, 
			read_class_sp& classd, json& class_mappings, json& field_mappings)
		{
			json_parser jp;

			json tab_contents = jp.create_array();

			json tab_edit_page = tab_template.clone();

			tab_edit_page.put_member("page_name", "tab_" + classd->get_class_name() + "_fields");

			// Set icon
			auto parameters = tab_edit_page.find_member("using.parameters");
			if (parameters.object()) {
				parameters.put_member("$icon_image_file", classd->get_class_name() + ".png");
			}

			// Populate card contents
			auto cfm = class_mappings["tab_edit"]["fields"];

            auto fmb = field_mappings[".section"]["box"];

			if (!cfm.array()) {
				validation_error ve;
				ve.class_name = classd->get_class_name();
                ve.filename = __FILE__;
				ve.line_number = __LINE__;
				ve.count = 1;
                ve.message = "tab_edit.fields must be supplied for " + classd->get_class_name();
				_errors.push_back(ve);
				return jp.create_object();
			}

			bool default_set = true;

			for (int i = 0; i < cfm.size(); i++) {
				auto field = cfm.get_element(i);

                std::string field_name = field["field_name"].as_string();
				std::string field_label = field["field_label"].as_string();

				if (field_name == ".section") {
					json content = jp.create_object();
					content.put_member_string("class_name", "chaptersubtitle");
					content.put_member("box", fmb);
					content.put_member_string("prompt", field_label);
					tab_contents.push_back(content);
					continue;
				}

				json field_box = field["field_box"];

				std::string field_class_name = field["field_ux_class"].as_string();

				json content = jp.create_object();
				content.put_member_string("class_name", field_class_name);
				content.put_member("box", field_box);
				content.put_member_string("prompt", field_label);
				content.put_member_string("name", field_name);
				content.put_member("json_field_name", field_name);
                content.put_member_bool("default_focus", default_set);
				default_set = false;
				tab_contents.push_back(content);
			}

            parameters.put_member("$tab_contents", tab_contents);

			return tab_edit_page;
		}

		json generate_tab_inventory_page(json& tab_inventory_template, std::string _control_class, read_class_sp& classd, const std::string& class_name, const std::string& member_name, json& create_command_class_template, json& form_sources)
		{
			json_parser jp;
			json tab_list_page = tab_inventory_template.clone();

			// Set icon
			auto parameters = tab_list_page.find_member("using.parameters");
			if (parameters.object()) {
				parameters.put_member("$icon_image_file", class_name + ".png");

				json tab_create_commands = jp.create_array();

				// in a future release we can have inventory manipulation commands here.

				std::string tab_list_table_name = "ti_" + class_name + "_" + member_name + "_table";

				json abbrevations = jp.create_object();
				abbrevations.put_member("$tab_inventory_commands", tab_create_commands);
				abbrevations.put_member("$tab_inventory_class", _control_class);
				abbrevations.put_member_string("$tab_inventory_commands_name", "ti_" + class_name + "_" + member_name + "_commands");
				abbrevations.put_member_string("$tab_inventory_table_name", tab_list_table_name);
				abbrevations.put_member_string("$json_field_name", member_name);
				parameters.apply_abbreviations(abbrevations);
			}

			return tab_list_page;
		}

		json generate_tab_custom_page(json& tab_custom_template, std::string _control_class, read_class_sp& classd, const std::string& class_name, const std::string& member_name, json& create_command_class_template, json& form_sources)
		{
			json_parser jp;
			json tab_custom_page = tab_custom_template.clone();

			// Set icon
			auto parameters = tab_custom_page.find_member("using.parameters");
			if (parameters.object()) {
				parameters.put_member("$icon_image_file", class_name + ".png");

				json tab_create_commands = jp.create_array();

				std::string tab_custom_table_name = "tc_" + class_name + "_" + member_name + "_table";

				auto field = classd->get_field(member_name);

				json abbreviations = jp.create_object();
				abbreviations.put_member("$tab_custom_class", _control_class);
				abbreviations.put_member_string("$tab_custom_table_name", tab_custom_table_name);
				abbreviations.put_member_string("$json_field_name", member_name);
				parameters.apply_abbreviations(abbreviations);
			}

			return tab_custom_page;
		}

		json generate_tab_list_page(json& tab_list_template, read_class_sp& classd, const std::string& class_name, const std::string& member_name, json& create_command_class_template, json& form_sources)
		{
			json_parser jp;
			json tab_list_page = tab_list_template.clone();

			// Set icon
			auto parameters = tab_list_page.find_member("using.parameters");
			if (parameters.object()) {
				parameters.put_member("$icon_image_file", class_name + ".png");

                json tab_create_commands = jp.create_array();

				std::string tab_list_table_name = "tl_" + class_name + "_" + member_name + "_table";

                auto field = classd->get_field(member_name);
				for (auto allowed_class : field->get_allowed_classes()) {
					json new_item = create_command_class_template.clone();

                    json update_filter = jp.create_object();				
					new_item.apply_abbreviations({
							{ "$create_button_class", jp.from_string(allowed_class) },
							{ "$create_button_name", jp.from_string("create_" + allowed_class + "_button") },
							{ "$create_button_image", jp.from_string(std::format("{}.png", allowed_class)) },
							{ "$create_button_text", jp.from_string(allowed_class) },
							{ "$create_button_message", jp.from_string("new " + allowed_class) },
							{ "$class_edit_page", form_sources[allowed_class] }
					});

					json click_command = new_item["on_click"];
					click_command.put_member_string("create_class_name", allowed_class);
					click_command.put_member_string("constructor_frame", "frame_selected");

					// so now, we need to get the class definition from the child, so we can get the field
					// name where we need to stick our reference to the parent in the child 

                    auto target_parent_class = read_lock_class(allowed_class);
					if (target_parent_class) {
						auto ref_target = target_parent_class->get_field_for_reference(classd.get());
						if (ref_target) {
							json jcopy = jp.create_object();
							jcopy.put_member_string(ref_target->get_field_name(), reference_field);
							click_command.put_member("constructor_copy", jcopy);
						}
						else
						{
							log_warning("could not find reference target for " + class_name + "." + member_name + " in " + target_parent_class->get_class_name(), __FILE__, __LINE__);
						}
					} 
					else
					{
						log_warning("could not find class " + allowed_class + " referred to in " + class_name + "." + member_name, __FILE__, __LINE__);
					}

					std::string form = std::format("object_{}", allowed_class);//form_sources[class_name].as_string();
					click_command.put_member_string("source_frame", form);
					click_command.put_member_string("target_frame", "frame_selected");
					new_item.put_member_string("message", "Create " + allowed_class);

                    tab_create_commands.push_back(new_item);
				}

				json abbrevations = jp.create_object();
                abbrevations.put_member("$tab_list_commands", tab_create_commands);
				abbrevations.put_member_string("$tab_list_commands_name", "tl_" + class_name + "_" + member_name + "_commands");
				abbrevations.put_member_string("$tab_list_table", tab_list_table_name);
				abbrevations.put_member_string("$json_field_name", member_name);
				abbrevations.put_member_string("$empty_text", "No " + member_name);
                parameters.apply_abbreviations(abbrevations);
			}

			return tab_list_page;
		}

		json generate_object_pages(validation_error_collection& _errors, 
			json& object_template,
			read_class_sp& classd,
			json& class_mappings, 
			json& field_mappings, 
			json& tab_list_template, 
			json& tab_custom_template, 
			json& tab_inventory_template, 
			json& tab_edit_template,
			json& create_command_class_template, 
			json& form_sources,
			json& section_box)
		{
			json_parser jp;

			json result = jp.create_array();
			json object_page = object_template.clone();

            std::string class_name = classd->get_class_name();

			object_page.put_member("page_name", "object_" + class_name);

			auto src_tabs = class_mappings["page"]["tabs"];
            auto dest_tabs = object_page.find_member("using.parameters.$details_contents.tabs");
			auto parameters = object_page.find_member("using.parameters");
            parameters.put_member("$icon_image_file", class_name + ".png");

			if (!src_tabs.array()) {
				return result;
			}

			if (!dest_tabs.array()) {
				return result;
			}

			for (int i = 0; i < src_tabs.size(); i++) {
				auto src_tab = src_tabs.get_element(i);

				json new_tab = src_tab.clone();

				std::string member_name = src_tab["member_name"].as_string();

				json tab_page;

                std::string page_name = new_tab["page_name"].as_string();
                std::string tab_type = new_tab["tab_type"].as_string();
				std::string ux_class = new_tab["ux_class"].as_string();

				if (tab_type == "inventory") {
					tab_page = generate_tab_inventory_page(tab_inventory_template, ux_class, classd, class_name, member_name, create_command_class_template, form_sources);
				}
				else if (tab_type == "custom") {
					tab_page = generate_tab_custom_page(tab_custom_template, ux_class, classd, class_name, member_name, create_command_class_template, form_sources);
				}
				else if (tab_type == "object") {
					tab_page = generate_tab_edit_page(_errors, tab_edit_template, classd, class_mappings, field_mappings );
				}
				else if (tab_type == "list") {
					tab_page = generate_tab_list_page(tab_list_template, classd, class_name, member_name, create_command_class_template, form_sources);
				}
				else if (tab_type == "edit") {
					tab_page = generate_tab_edit_page(_errors, tab_edit_template, classd, class_mappings, field_mappings );
				}

				if (tab_page.object()) {
					tab_page.put_member("page_name", page_name);
					result.push_back_array(tab_page);

					new_tab.erase_member("tab_type");
					new_tab.erase_member("ux_type");
					new_tab.put_member("page_name", page_name);

					if (dest_tabs.array()) {
						bool default_tab = src_tab["default_tab"].as_bool();
                        if (default_tab) {
                            dest_tabs.push_front(new_tab);
						}
						else {
							dest_tabs.push_back(new_tab);
						}
					}
				}
				else {
					//DebugBreak();
				}
			}

			

            result.push_back(object_page);

			return result;
		}

		public:


		virtual json apply_schema()
		{
			progress_fence progress(&calls_in_progress);

			json_parser jp;
			std::string schema = read_all_string(schema_file_name);
            if (schema.empty()) {
				system_monitoring_interface::active_mon->log_warning(std::format("Could not read schema '{}'", schema_file_name), __FILE__, __LINE__);
				return jp.create_object();
			}
			json jschema = jp.parse_object(schema);
            if (jschema.error()) {
				system_monitoring_interface::active_mon->log_warning("Could not parse schema file", __FILE__, __LINE__);
                for (auto& err : jp.parse_errors) {
					system_monitoring_interface::active_mon->log_warning(err.error, schema_file_name.c_str(), err.line);
				}
				return jschema;
			}

			json result = apply_schema(jschema);
			return result;
		}

		virtual json process_data_set(json jschema, std::string _dataset_name)
		{
			json_parser jp;

			auto sys_perm = get_system_permission();
			timer tx;

			if (jschema.has_member(_dataset_name))
			{
				date_time start_section = date_time::now();
				timer txsect;
				system_monitoring_interface::active_mon->log_job_section_start("", "Datasets", start_section, __FILE__, __LINE__);
				using namespace std::literals;
				json dataset_array = jschema[_dataset_name];
				int64_t dataset_count = 1;
				if (dataset_array.array())
				{
					dataset_count = dataset_array.size();
					for (int i = 0; i < dataset_array.size(); i++)
					{
						date_time start_dataset = date_time::now();
						timer txs;

						if (shutdown_in_progress) {
							break;
						}

						json new_dataset = dataset_array.get_element(i);
						new_dataset.put_member(class_name_field, "sys_dataset"sv);
						object_reference parent;
						parent.object_id = jschema[object_id_field].as_int64_t();
                        parent.class_name = jschema[class_name_field].as_string();
						new_dataset.put_member("sys_schema", parent);
						std::string dataset_name = new_dataset["dataset_name"].as_string();
						std::string dataset_version = new_dataset["dataset_version"].as_string();

						system_monitoring_interface::active_mon->log_job_section_start("DataSet", dataset_name + " Start", start_dataset, __FILE__, __LINE__);

						bool run_on_change = new_dataset["run_on_change"].as_bool();
						json existing_dataset = get_dataset(dataset_name, dataset_version, sys_perm);

						if (!run_on_change and existing_dataset.object() and existing_dataset.has_member("completed")) {
							system_monitoring_interface::active_mon->log_job_section_stop("DataSet", dataset_name + " Already Done", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
							new_dataset.erase_member("objects");
							continue;
						}

						if (new_dataset.object()) {

							if (new_dataset.has_member("import"))
							{
								new_dataset.put_member_i64("sys_schema", jschema[object_id_field].as_int64_t());
								json import_spec = new_dataset["import"];
								std::vector<std::string> missing;

								if (not import_spec.has_members(missing, { "target_class", "type" })) {
									system_monitoring_interface::active_mon->log_warning("Import missing:");
									std::for_each(missing.begin(), missing.end(), [](const std::string& s) {
										system_monitoring_interface::active_mon->log_warning(s);
										});
									system_monitoring_interface::active_mon->log_information("the source json is:");
									system_monitoring_interface::active_mon->log_json<json>(import_spec, 2);
									continue;
								}

								std::string target_class = import_spec["target_class"].as_string();
								std::string target_class_plural = target_class;
								if (target_class_plural.ends_with("s")) {
									target_class_plural += "es";
								}
								else {
									target_class_plural += "s";
								}
								std::string import_type = import_spec["type"].as_string();
								date_time import_datatime = import_spec["import_datatime"].as_date_time();

								if (import_type == "csv") {

									if (not import_spec.has_members(missing, { "filename", "delimiter" })) {
										system_monitoring_interface::active_mon->log_warning("Import CSV missing:");
										std::for_each(missing.begin(), missing.end(), [](const std::string& s) {
											system_monitoring_interface::active_mon->log_warning(s);
											});
										system_monitoring_interface::active_mon->log_information("the source json is:");
										system_monitoring_interface::active_mon->log_json<json>(import_spec, 2);
										continue;
									}

									std::string filename = import_spec["filename"].as_string();

									std::string delimiter = import_spec["delimiter"].as_string();
									if (filename.empty() or delimiter.empty()) {
										system_monitoring_interface::active_mon->log_warning("filename and delimiter can't be blank.");
										continue;
									}

									std::filesystem::path import_path(filename);
									if (not import_path.is_absolute()) {
										import_path = config_path / import_path;
									}
									filename = import_path.string();

									if (delimiter == "tab") {
										delimiter = "\t";
									}

									json pivot = import_spec["pivot"];
									json pivot_object;
									std::vector<std::string> pivot_object_field_names;
									std::string pivot_attribute_field_name;
									std::string pivot_value_field_name;
									bool pivot_lower_case = true;

									if (pivot.object())
									{
										pivot_attribute_field_name = pivot["attribute_field"].as_string();
										pivot_value_field_name = pivot["value_field"].as_string();
										pivot_lower_case = pivot["lower_case_attribute"].as_bool();

										if (pivot_attribute_field_name.empty() or pivot_value_field_name.empty())
										{
											system_monitoring_interface::active_mon->log_warning("pivot import attribute_field and value_field must be strings.");
											pivot.clear();
											continue;
										}

										pivot_object_field_names = pivot["object_fields"].to_string_array();
										if (pivot_object_field_names.size() == 0)
										{
											system_monitoring_interface::active_mon->log_warning("pivot import object_fields must be [ string, .. ].");
											pivot.clear();
											continue;
										}

										pivot_object = jp.create_object();
									}

									if (std::filesystem::exists(filename))
									{

										auto file_modified = std::filesystem::last_write_time(filename);
										const auto system_file_modified = std::chrono::clock_cast<std::chrono::system_clock>(file_modified);
										const auto file_modified_time = std::chrono::system_clock::to_time_t(system_file_modified);
										bool pivot_object_ready = false;
										date_time file_modified_dt = date_time(file_modified_time);

										if (file_modified_dt > import_datatime || !existing_dataset.has_member("completed")) {

											int64_t file_size = 0;
											int64_t bytes_processed = 0;
											int64_t last_bytes_processed = 0;

											if (existing_dataset.object() && existing_dataset.has_member("bytes_read")) {
												last_bytes_processed = existing_dataset["bytes_read"].as_int64_t();
											}

											try
											{
												file_size = std::filesystem::file_size(filename);
											}
											catch (std::exception exc)
											{

											}

											json column_map = import_spec["column_map"];

											FILE* fp = nullptr;
											int error_code = fopen_s(&fp, filename.c_str(), "rt");

											if (fp) {
												// Buffer to store each line of the file.
												char line[32768];
												json datomatic = jp.create_array();

												// create template object
												json codata = jp.create_object();
												codata.put_member(class_name_field, target_class);
												json cor = create_system_request(codata);
												json new_object_response = create_object(cor);

												if (new_object_response[success_field].as_bool()) {
													json new_object_template = new_object_response[data_field];

													// Read each line from the file and store it in the 'line' buffer.
													int64_t total_row_count = 0;
													while (fgets(line, sizeof(line) - 1, fp)) {

														if (shutdown_in_progress) {
															system_monitoring_interface::active_mon->log_warning("Shutdown in progress, import aborted.", __FILE__, __LINE__);
															break;
														}

														char* line_start = line;
														if (line_start[0] == '\ufeff') {
															line_start = &line_start[3];
														}

														// Print each line to the standard output.
														json new_object = new_object_template.clone();
														new_object.erase_member(object_id_field);
														int64_t bytes_in_line = (int64_t)strlen(line_start);
														bytes_processed += bytes_in_line;

														if (bytes_processed < last_bytes_processed) {
															continue;
														}

														if (pivot.empty())
														{
															json extra = jp.create_object();
															jp.parse_delimited_string(new_object, column_map, line_start, delimiter, extra);
															datomatic.push_back(new_object);
														}
														else
														{
															json extra = jp.create_object();
															jp.parse_delimited_string(new_object, column_map, line_start, delimiter, extra);

															json pivot_keys = new_object.extract(pivot_object_field_names);

															if (!pivot_object_ready) {
																pivot_object = pivot_keys.clone();
																pivot_object_ready = true;
																pivot_object.put_member(class_name_field, target_class);
															}
															else if (pivot_keys.compare(pivot_object) != 0)
															{
																datomatic.push_back(pivot_object);
																pivot_object = pivot_keys.clone();
																pivot_object.put_member(class_name_field, target_class);
															}

															std::string attribute_field_name = extra[pivot_attribute_field_name].as_string();
															std::string attribute_value = extra[pivot_value_field_name].as_string();
															if (pivot_lower_case) {
																std::transform(attribute_field_name.begin(), attribute_field_name.end(), attribute_field_name.begin(),
																	[](unsigned char c) { return std::tolower(c); });
															}
															pivot_object.put_member(attribute_field_name, attribute_value);
														}

														if (datomatic.size() > import_batch_size) {
															int batch_size = (int)datomatic.size();
															total_row_count += datomatic.size();
															json request(datomatic);
															json cor = create_system_request(request);

															put_object_sync(cor, [this, file_size, bytes_processed, total_row_count, batch_size, &target_class_plural](json& put_result, double _exec_time) {
																double x = batch_size / _exec_time;
																std::string msg = put_result["message"].as_string();
																if (file_size) {
																	msg = msg + std::format(", {0:.2f}%, {1:.2f} / sec, {2} {3}", (double)bytes_processed / (double)file_size * 100.0, x, total_row_count, target_class_plural);
																}
																system_monitoring_interface::active_mon->log_activity(msg, _exec_time, batch_size, __FILE__, __LINE__);
																if (put_result[success_field].as_bool()) {

																}
																else
																{
																	log_error_array(put_result);
																}
																});

															new_dataset.put_member_i64("bytes_read", bytes_processed);
															new_dataset.put_member_i64("bytes_total", file_size);
															import_spec.put_member("import_datatime", file_modified_dt);
															new_dataset.share_member("import", import_spec);
															json dataset_request = create_system_request(new_dataset);
															json result = put_object(dataset_request);


															datomatic = jp.create_array();
														}
													}

													if (datomatic.size() > 0) {
														int batch_size = (int)datomatic.size();
														total_row_count += batch_size;
														json request(datomatic);
														json cor = create_system_request(request);
														put_object_sync(cor, [this, total_row_count, batch_size](json& put_result, double _exec_time) {
															if (put_result[success_field].as_bool()) {
																double x = batch_size / _exec_time;
																std::string msg = std::format("{0} objects, {1:.2f} / sec, {2} rows total", batch_size, x, total_row_count);
																system_monitoring_interface::active_mon->log_activity(msg, _exec_time, __FILE__, __LINE__);
																system_monitoring_interface::active_mon->log_activity(put_result["message"].as_string(), _exec_time, __FILE__, __LINE__);
															}
															else
															{
																log_error_array(put_result);
															}
															});
														new_dataset.put_member_i64("bytes_read", bytes_processed);
														new_dataset.put_member_i64("bytes_total", file_size);
														import_spec.put_member("import_datatime", file_modified_dt);
														new_dataset.share_member("import", import_spec);
														json dataset_request = create_system_request(new_dataset);
														json result = put_object(dataset_request);

														datomatic = jp.create_array();
													}

												}

												// Close the file stream once all lines have been read.
												fclose(fp);
												import_spec.put_member("import_datatime", file_modified_dt);
												new_dataset.share_member("import", import_spec);
												if (bytes_processed >= file_size) {
													date_time completed_date = date_time::now();
													new_dataset.put_member("completed", completed_date);
												}
												json dataset_request = create_system_request(new_dataset);
												new_dataset.erase_member("objects");
												json result = put_object(dataset_request);
												if (result[success_field].as_bool()) {
													std::string msg = std::format("imported from {0}", filename);
													system_monitoring_interface::active_mon->log_information(msg, __FILE__, __LINE__);
												}
												else {
													system_monitoring_interface::active_mon->log_warning(result[message_field].as_string(), __FILE__, __LINE__);
												}
											}
											else {
												char error_buffer[256] = {};
												strerror_s(
													error_buffer,
													std::size(error_buffer),
													error_code
												);
												std::string msg = std::format("could not open file {0}:{1}", filename, error_buffer);
												system_monitoring_interface::active_mon->log_warning(msg, __FILE__, __LINE__);
												char directory_name[MAX_PATH] = {};
												char* result = _getcwd(directory_name, std::size(directory_name));
												if (result) {
													msg = std::format("cwd is {0}", result);
													system_monitoring_interface::active_mon->log_information(msg, __FILE__, __LINE__);
												}
											}
										}
									}
									else
									{
										std::string msg = std::format("file {0} doesn't exist", filename);
										system_monitoring_interface::active_mon->log_warning(msg, __FILE__, __LINE__);
									}
								}
							}

							if (new_dataset.has_member("objects")) {
								json object_list = new_dataset["objects"];
								if (object_list.array()) {
									for (int j = 0; j < object_list.size(); j++) {
										json object_definition = object_list.get_element(j).clone();
										json put_object_request = create_system_request(object_definition);
										json create_result = put_object(put_object_request);
										if (not create_result[success_field].as_bool()) {
											system_monitoring_interface::active_mon->log_warning(create_result[message_field].as_string());
										}
										else {
											json result = create_result[data_field];
											for (auto class_result : result.get_members()) {
												json items = class_result.second;
												if (items.array()) {
													for (auto item : items) {
														if (not item[success_field].as_bool()) {
															system_monitoring_interface::active_mon->log_warning(item[message_field].as_string(), __FILE__, __LINE__);
														}
														else {
															json item_data = item[data_field];
															std::string new_class_name = item_data[class_name_field].as_string();
															int64_t object_id = item_data[object_id_field].as_int64_t();
															std::string object_created = std::format("object {0} {1} saved", new_class_name, object_id);
															system_monitoring_interface::active_mon->log_information(object_created);
														}
													}
												}
											}
										}
									}
								}
							}
							if (!shutdown_in_progress) {
								date_time completed_date = date_time::now();
								new_dataset.put_member("completed", completed_date);
							}
							new_dataset.erase_member("objects");
							json put_script_request = create_system_request(new_dataset);
							json save_script_result = put_object(put_script_request);
							if (not save_script_result[success_field].as_bool()) {
								system_monitoring_interface::active_mon->log_warning(save_script_result[message_field].as_string());
							}
							else
								system_monitoring_interface::active_mon->log_information(save_script_result[message_field].as_string());
						}

						system_monitoring_interface::active_mon->log_job_section_stop("DataSet", dataset_name + " Finished", txs.get_elapsed_seconds(), 1, __FILE__, __LINE__);
					}
					system_monitoring_interface::active_mon->log_job_section_stop("DataSets", "", txsect.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				}
			}

			return jp.create_object();
		}

		virtual json apply_schema(json _schema)
		{
			progress_fence progress(&calls_in_progress);

			if (shutdown_in_progress) {
				json_parser jp;
				json error = jp.create_object();
				error.put_member(success_field, false);
				error.put_member(message_field, std::string("Shutdown in progress"));
				return error;
            }

			date_time start_schema = date_time::now();
			timer tx;
			system_monitoring_interface::active_mon->log_job_start("apply_schema", "Applying schema file", start_schema, __FILE__, __LINE__);
			using namespace std::literals;

			if (_schema.error()) {
				json errs = _schema["errors"];
				if (errs.array()) {
					for (int i = 0; i < errs.size(); i++)
					{
						json erri = errs.get_element(i);
						std::string message = std::format("{} @{}", erri["error"].as_string(), erri["line"].as_string());
						log_warning(message);
					}
				}
				return _schema;
			}

			if (not _schema.has_member("schema_name"))
			{
				system_monitoring_interface::active_mon->log_warning("Schema doesn't have a schema name");
			}

			if (not _schema.has_member("schema_version"))
			{
				system_monitoring_interface::active_mon->log_warning("Schema doesn't have a schema version");
			}

			if (not _schema.has_member("schema_author"))
			{
				system_monitoring_interface::active_mon->log_warning("Schema doesn't have a schema author");
			}

			auto sys_perm = get_system_permission();

			json_parser jp;

			json schema_key = jp.create_object();
			schema_key.copy_member("schema_name", _schema);
			schema_key.copy_member("schema_version", _schema);
			schema_key.put_member(class_name_field, "sys_schema"sv);
			schema_key.set_compare_order({ "schema_name", "schema_version" });

			json jschema_array =  select_object(schema_key, false, sys_perm);
            json jschema = jschema_array.get_first_element();

			if (jschema.object()) 
			{
                jschema.merge(_schema);

				std::string schema_msg = std::format("Schema '{0}' already applied", schema_key["schema_name"].as_string());
				system_monitoring_interface::active_mon->log_information(schema_msg, __FILE__, __LINE__);

				json error = jp.create_object();
				error.put_member(success_field, false);
				error.put_member(message_field, std::string("Schema already applied"));
				return error;
			}
			else 
			{
                json create_object_body = jp.create_object();
				create_object_body.put_member(class_name_field, "sys_schema"sv);
                json create_object_request = create_system_request(create_object_body);
                json result = create_object(create_object_request);
				if (result.has_member(data_field)) 
				{
					json new_schema = result[data_field];
                    new_schema.merge(_schema);
					jschema = new_schema;
				}
			}

            if (jschema.has_member("pre_dataset")) {                
                process_data_set(jschema, "pre_dataset");
            }

			if (jschema.has_member("classes"))
			{
				date_time start_section = date_time::now();
				timer txsect;
				system_monitoring_interface::active_mon->log_job_section_start("", "Classes", start_section, __FILE__, __LINE__);

				json class_array = jschema["classes"];
				int64_t class_count = 1;
				if (class_array.array())
				{
					class_count = class_array.size();
					std::map<std::string, bool> visited_classes;

					for (int i = 0; i < class_array.size(); i++)
					{
						date_time start_class = date_time::now();
						timer txc;

						json class_definition = class_array.get_element(i).clone();

						try {

							if constexpr (false) {
								if (class_definition[class_name_field].as_string() == "company")
								{
									DebugBreak();
								}
							}

							std::string new_class_name = class_definition["class_name"].as_string();
							std::string new_class_author = class_definition["class_author"].as_string();
							std::string new_class_version = class_definition["class_version"].as_string();

							if (visited_classes.contains(new_class_name)) {
								system_monitoring_interface::active_mon->log_warning(new_class_name + " already visited", __FILE__, __LINE__);
								system_monitoring_interface::active_mon->log_job_section_stop("", new_class_name + " already visited", txc.get_elapsed_seconds(), 1, __FILE__, __LINE__);
								continue;
							}

                            visited_classes[new_class_name] = true;

                            auto existing_class = read_lock_class(new_class_name);

							if (existing_class) {
                                if (existing_class->get_class_version() == new_class_version and
                                    existing_class->get_class_author() == new_class_author)
                                {
                                    system_monitoring_interface::active_mon->log_job_section_stop("", new_class_name + " up to date", txc.get_elapsed_seconds(), 1, __FILE__, __LINE__);
                                    continue;
                                }
							}

							json put_class_request = create_system_request(class_definition);
							json class_result =  put_class(put_class_request);

							if (class_result.error()) 
							{
								system_monitoring_interface::active_mon->log_warning(class_result["message"].as_string(), __FILE__, __LINE__);
								continue;
							}

							system_monitoring_interface::active_mon->log_job_section_stop("", new_class_name + " complete", txc.get_elapsed_seconds(), 1, __FILE__, __LINE__);
						}
						catch (std::exception exc)
						{
							system_monitoring_interface::active_mon->log_exception(exc);
							continue;
						}
					}
				}
				system_monitoring_interface::active_mon->log_job_section_stop("", "Classes", txsect.get_elapsed_seconds(), class_count, __FILE__, __LINE__);
			}
			else
			{
				system_monitoring_interface::active_mon->log_warning("classes not found in schema");
			}

			class_cache.clear();

			if (jschema.has_member("users"))
			{
				date_time start_section = date_time::now();
				timer txsect;
				system_monitoring_interface::active_mon->log_job_section_start("", "Users", start_section, __FILE__, __LINE__);
				json user_array = jschema["users"];
				int64_t user_count = 1;
				if (user_array.array())
				{
					user_count = user_array.size();
					for (int i = 0; i < user_array.size(); i++)
					{
						date_time start_user = date_time::now();
						timer txu;

						json user_definition = user_array.get_element(i);
						system_monitoring_interface::active_mon->log_function_start("put user", user_definition[user_name_field].as_string(), start_user, __FILE__, __LINE__);
						json put_user_request = create_system_request(user_definition);
						create_user(put_user_request);
					    system_monitoring_interface::active_mon->log_function_stop("put user", user_definition[user_name_field].as_string(), txu.get_elapsed_seconds(), 1, __FILE__, __LINE__);
					}
				}
				system_monitoring_interface::active_mon->log_job_section_stop("", "Users", txsect.get_elapsed_seconds(), user_count, __FILE__, __LINE__);
			}

			if (jschema.has_member("post_dataset")) {
				process_data_set(jschema, "post_dataset");
			}

			if (jschema.has_member("dataset")) {
				process_data_set(jschema, "dataset");
			}

			if (jschema.has_member("application")) {
				try {
					json japplication = jschema["application"];
					create_application(japplication);
				}
                catch (std::exception exc) {
					system_monitoring_interface::active_mon->log_exception(exc);
				}
			}

			json put_schema_request = create_system_request(jschema);
			// in corona, creating an object doesn't actually persist anything 
			// but a change in identifier.  It's a clean way of just getting the 
			// "new chumpy" item for ya.  
			json put_schema_result =  put_object(put_schema_request);
			if (put_schema_result[success_field].as_bool()) {
				std::string message = put_schema_result[message_field].as_string();
				system_monitoring_interface::active_mon->log_information(message, __FILE__, __LINE__);
			}
			else 
			{
				system_monitoring_interface::active_mon->log_warning(put_schema_result[message_field].as_string(), __FILE__, __LINE__);
			}

			system_monitoring_interface::active_mon->log_job_stop("apply_schema", "schema applied", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);

			json temp = R"({ "success" : true, "message" : "Everything Ok, situation normal."})"_jobject;

			return temp;
		}

		virtual relative_ptr_type open_database()
		{
			progress_fence progress(&calls_in_progress);

			write_scope_lock my_lock(database_lock);
			json_parser jp;

			timer method_timer;
			date_time start_schema = date_time::now();
			timer tx;
			system_monitoring_interface::active_mon->log_job_start("open_database", "Open database", start_schema, __FILE__, __LINE__);

			activity act;
			act.db = this;

			json class_definition = jp.parse_object(class_definition_string);
			if (class_definition.error())
				throw std::exception("Class Definition Parse Error");

			std::filesystem::path class_table = data_path;
			class_table /= "classes.coronatbl";
			std::string file_name = class_table.string();

			classes = std::make_shared<xtable>(file_name, corona_db_read_only);

            std::vector<std::string> check_classes = { "sys_object", "sys_user", "sys_team", "sys_grant", "sys_server", "sys_error", "sys_schema", "sys_dataset" };

			for (auto check_class : check_classes) {
                std::string class_key = std::format(R"({{"class_name":"{0}"}})", check_class);
                json key = jp.parse_object(class_key);
				json test = classes->get(key);
				if (test.empty() or test.error()) {
					system_monitoring_interface::active_mon->log_warning("could not find class '" + check_class + "' after open.", __FILE__, __LINE__);
					system_monitoring_interface::active_mon->log_job_stop("open_database", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
					abort();
				}
			}

			system_monitoring_interface::active_mon->log_job_stop("open_database", "Open database", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);

			return true;
		}

		class password_metrics
		{
		public:
			int punctuation_count = 0;
			int digit_count = 0;
			int alpha_count = 0;
			int char_count = 0;

			password_metrics(const std::string& _check)
			{
				for (auto c : _check)
				{
					if (std::ispunct(c) or c == ' ') {
						punctuation_count++;
					}
					else if (std::isdigit(c)) {
						digit_count++;
					}
					else if (std::isalpha(c))
					{
						alpha_count++;
					}

					char_count++;
				}

			}

			struct password_policy {
				int min_length = 10;
				int min_punctuation = 1;
				int min_digits = 1;
                int min_alpha = 1;
			};

			std::string is_stupid()
			{
				password_policy policy;

				std::string plural = "s";

				std::string corrections = "";
				int c;
				if ((c = policy.min_punctuation - punctuation_count) > 0) {
                    std::string x = c > 1 ? "s" : "";
					corrections += std::format( "Add {0} character{1}", c, x);
                }
				if ((c = policy.min_punctuation - punctuation_count) > 0) {
					std::string x = c > 1 ? "s" : "";
					corrections += std::format("Add {0} punctuation marks, like '!'", policy.min_length - char_count);
				}
				if ((c = policy.min_digits - digit_count) > 0) {
					std::string x = c > 1 ? "s" : "";
					corrections += std::format("Add {0} digits, like '0123'", policy.min_length - char_count);
				}
				if ((c = policy.min_alpha - alpha_count) > 0) {
					std::string x = c > 1 ? "s" : "";
					corrections += std::format("Add {0} letters, like 'Abc'", policy.min_alpha - alpha_count);
				}

				return corrections;
			}
		};

		/// <summary>
		/// Removes references to fields in object_to_scrub that are marked server only.
		/// This prevents clients from updating server only data,
		/// and also guards against leaking them.
		/// </summary>
		/// <param name="object_to_scrub"></param>
		virtual void remove_object_ids(json& object_to_scrub)
		{
			if (auto obj_impl = object_to_scrub.object_impl()) {
				auto member_set = object_to_scrub.get_members();
				for (auto member : member_set)
				{
					json child(member.second);
					if (child.object() or child.array()) {
						scrub_object(child);
					}
                    else if (member.first == object_id_field) {
						obj_impl->erase(member.first);
					}
				}
			}
			else if (auto array_impl = object_to_scrub.array_impl()) {
				for (int i = 0; i < array_impl->size(); i++) {
					json item((*array_impl)[i]);
					if (item.object()) {
						scrub_object(item);
					}
				}
			}
		}

		/// <summary>
		/// Removes references to fields in object_to_scrub that are marked server only.
		/// This prevents clients from updating server only data,
		/// and also guards against leaking them.
		/// </summary>
		/// <param name="object_to_scrub"></param>
		virtual void scrub_object(json& object_to_scrub)
		{
			
			if (auto obj_impl = object_to_scrub.object_impl()) {
				std::string class_name = object_to_scrub[class_name_field].as_string();
				auto rsp = read_lock_class(class_name);
                auto member_set = object_to_scrub.get_members();
				for (auto member : member_set)
				{
					if (rsp and not rsp->is_server_only(member.first)) {
						obj_impl->erase(member.first);
					}
                    json child(member.second);
					if (child.object() or child.array()) {
						scrub_object(child);
					}
				}
			}
			else if (auto array_impl = object_to_scrub.array_impl()) {
				for (int i = 0; i < array_impl->size(); i++) {
					json item((*array_impl)[i]);
					if (item.object()) {
						scrub_object(item);
					}
				}
			}
		}

		date_time last_checked;
		json google_keys;

		//https://www.googleapis.com/oauth2/v3/certs

		json check_jwt_token(std::string _token)
		{

			json_parser jp;
			json result;

			std::vector<std::string> parts = split(_token, '.');

			date_time current = date_time::now();

			if (google_keys.empty() or current.hour() != last_checked.hour()) {
				write_scope_lock check_lock(key_lock);

				last_checked = current;
				http_client google_api;
				http_params google_response = google_api.get("www.googleapis.com", 443, "/oauth2/v3/certs");
				system_monitoring_interface::active_mon->log_warning("Google auth keys", __FILE__, __LINE__);
				if (google_response.response.response_body.is_safe_string()) 
				{
					char* body = google_response.response.response_body.get_ptr();
					system_monitoring_interface::active_mon->log_warning(body, __FILE__, __LINE__);
					google_keys = jp.parse_object(body);
				}
				else 
				{
					system_monitoring_interface::active_mon->log_warning("Response body is not a string", __FILE__, __LINE__);
				}
			}

			if (google_keys.array()) {
				std::string header = base64_decode(parts[0]);
				json jheader = jp.parse_object(header);

				std::string payload = base64_decode(parts[1]);
				json jpayload = jp.parse_object(payload);

				std::string kid = jheader["kid"].as_string();
				json public_key;
                for (auto key : google_keys) {
					std::string jkid = key["kid"].as_string();
                    if (jkid == kid) {
                        public_key = key;
                        break;
                    }
                }
                std::string plain_text = parts[0] + "." + parts[1];
				std::string cipher_text = crypter.encrypt(plain_text, public_key);
				std::string cipher64 = base64_encode(cipher_text);
				result = jpayload;
			}

			return result;
		}


		virtual json login_user_local(json _sso_user_request) override
		{
			progress_fence progress(&calls_in_progress);

			timer method_timer;
			json_parser jp;
			json response;

			validation_error_collection errors;
			read_scope_lock my_lock(database_lock);
			date_time start_time = date_time::now();
			timer tx;

			system_monitoring_interface::active_mon->log_function_start("login_user_local", "start", start_time, __FILE__, __LINE__);

			json data = _sso_user_request[data_field];
			std::string access_code = data["access_code"].as_string();
			std::string user_name = data[user_name_field].as_string();
			std::string user_email = data[user_email_field].as_string();

			if (user_name.empty()) {
				user_name = default_user;
			}
			if (user_email.empty()) {
				user_email = default_email_address;
			}

			if (access_code.empty())
			{
				validation_error_collection errors;
				validation_error err;
				err.class_name = "sys_user";
				err.field_name = "access_code";
				err.filename = get_file_name(__FILE__);
				err.line_number = __LINE__;
				err.message = "Invalid access_code (sso integration botched)";
				errors.push_back(err);
				response = create_user_response(_sso_user_request, false, "access_code is required to validate.", data, errors, method_timer.get_elapsed_seconds());
				return response;
			}

			std::string machine_id = application::get_machine_id();
			if (machine_id != access_code)
			{
				validation_error_collection errors;
				validation_error err;
				err.class_name = "sys_user";
				err.field_name = "access_code";
				err.filename = get_file_name(__FILE__);
				err.line_number = __LINE__;
				err.message = "Invalid access_code (machine id mismatch)";
				errors.push_back(err);
				response = create_user_response(_sso_user_request, false, "access_code is invalid for this machine.", data, errors, method_timer.get_elapsed_seconds());
				return response;
            }

			auto sys_perm = get_system_permission();

			json existing_user = get_user(user_name, false);

			if (not existing_user.object())
			{
				std::string user_password1;
				user_password1 = get_random_code() + get_random_code() + get_random_code();
				std::string hashed_pw = crypter.hash(user_password1);

				existing_user = jp.create_object();
				existing_user.put_member(class_name_field, std::string("sys_user"));
				existing_user.put_member(user_name_field, user_name);
				existing_user.put_member(user_email_field, user_email);
				existing_user.put_member(user_password_field, hashed_pw);
				existing_user.put_member(user_first_name_field, std::string("firstname"));
				existing_user.put_member(user_last_name_field, std::string("lastname"));
				existing_user.put_member(user_street1_field, std::string("5 Watchful Road"));
				existing_user.put_member(user_street2_field, std::string("Room One 101"));
				existing_user.put_member(user_city_field, std::string("London"));
				existing_user.put_member(user_state_field, std::string("Oceana"));
				existing_user.put_member(user_zip_field, std::string("12345"));
				existing_user.put_member(user_mobile_field, std::string("5558675309"));
				existing_user.put_member("confirmed_code", 1);
				json teams = get_team_by_email(user_email, sys_perm);

				if (teams.empty())
				{
					system_monitoring_interface::active_mon->log_warning(std::format("No team found for user '{}', email '{}'", user_name, user_email), __FILE__, __LINE__);
                }

				for (json team : teams)
				{
					existing_user.put_member("home_team_name", team["team_name"].as_string());
					existing_user.put_member("team_name", team["team_name"].as_string());
				}

				apply_user_team(existing_user);
				json create_object_request = create_system_request(existing_user, default_user);
				json user_result = put_object(create_object_request);
				// we have to confirm if this guy did his job.
				json jerrors = user_result["errors"];
				if (user_result[success_field].as_bool() && (!jerrors.array() || jerrors.size() == 0))
				{
					existing_user = get_user(user_name);
				}
				else
				{
					std::string message = user_result[message_field].as_string();
					system_monitoring_interface::active_mon->log_warning(std::format("Could not create user '{}': {}", user_name, message), __FILE__, __LINE__);
					system_monitoring_interface::active_mon->log_json(user_result);
					response = create_user_response(_sso_user_request, false, "User not created", existing_user, jerrors, method_timer.get_elapsed_seconds());
					system_monitoring_interface::active_mon->log_function_stop("login_user_local", "complete", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
					return response;
				}
			}
			else if (!existing_user.has_member("team_name") || !existing_user.has_member("home_team_name")) {
				json teams = get_team_by_email(user_email, sys_perm);
				for (json team : teams)
				{
					existing_user.put_member("home_team_name", team["team_name"].as_string());
					existing_user.put_member("team_name", team["team_name"].as_string());
				}
				apply_user_team(existing_user);
				put_user(existing_user);
				existing_user = get_user(user_name);
			}

			response = create_response(user_name, auth_system, true, "Ok", existing_user, errors, method_timer.get_elapsed_seconds());
			system_monitoring_interface::active_mon->log_function_stop("login_user_local", "complete", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);

			return response;
		}

		virtual json login_user_sso(json _sso_user_request) override
		{
			timer method_timer;
			json_parser jp;
			json response;

			validation_error_collection errors;
			read_scope_lock my_lock(database_lock);
			date_time start_time = date_time::now();
			timer tx;

			system_monitoring_interface::active_mon->log_function_start("login_user_sso", "start", start_time, __FILE__, __LINE__);
			
			json data = _sso_user_request[data_field];
            std::string access_code = data["code"].as_string();
			std::string user_name = data[user_name_field].as_string();
			std::string user_email = data[user_email_field].as_string();

			if (access_code.empty())
			{
				validation_error_collection errors;
				validation_error err;
				err.class_name = "sys_user";
				err.field_name = "access_code";
				err.filename = get_file_name(__FILE__);
				err.line_number = __LINE__;
				err.message = "Invalid access_code (sso integration botched)";
				errors.push_back(err);				
				response = create_user_response(_sso_user_request, false, "access_code is required to validate.", data, errors, method_timer.get_elapsed_seconds());
				return response;
			}

			std::string client_secret = connections.get_connection("googleclientsecret");
			std::string client_id = connections.get_connection("googleclientid");

			/*
			POST /token HTTP/1.1
Host: oauth2.googleapis.com
Content-Type: application/x-www-form-urlencoded

code=4/P7q7W91a-oMsCeLvIaQm6bTrgtp7&
client_id=your_client_id&
client_secret=your_client_secret&
redirect_uri=https%3A//oauth2.example.com/code&
grant_type=authorization_code
			*/

			json jpost_body = jp.create_object();
            jpost_body.put_member("code", access_code);
			jpost_body.put_member("client_id", client_id);
			jpost_body.put_member("client_secret", client_secret);
			jpost_body.put_member("redirect_uri", std::string("postmessage"));
			jpost_body.put_member("grant_type", std::string("authorization_code"));

			http_client google_api;
			http_params google_response1 = google_api.post("oauth2.googleapis.com", 443, "token", jpost_body);

			if (not google_response1.response.response_body.is_safe_string())
			{
				validation_error_collection errors;
				validation_error err;
				err.class_name = "sys_user";
				err.field_name = "access_token";
				err.filename = get_file_name(__FILE__);
				err.line_number = __LINE__;
				err.message = "Could not convert Google code to token";
				errors.push_back(err);
				system_monitoring_interface::active_mon->log_function_stop("login_user_sso", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				response = create_user_response(_sso_user_request, false, "could not convert google code.", data, errors, method_timer.get_elapsed_seconds());
				return response;
			}

			json token_body = jp.parse_object(google_response1.response.response_body.get_ptr());
            std::string access_token = token_body["access_token"].as_string();

            std::string header = std::format("Authorization: Bearer {}\n", access_token);

			http_params google_response = google_api.get("www.googleapis.com", 443, "/oauth2/v3/userinfo", header.c_str());
			system_monitoring_interface::active_mon->log_warning("Google auth keys", __FILE__, __LINE__);

            if (not google_response.response.response_body.is_safe_string())
			{
                validation_error_collection errors;
                validation_error err;
                err.class_name = "sys_user";
                err.field_name = "access_token";
                err.filename = get_file_name(__FILE__);
                err.line_number = __LINE__;
                err.message = "Google didn't like the sign on";
                errors.push_back(err);
                system_monitoring_interface::active_mon->log_function_stop("login_user_sso", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
                response = create_user_response(_sso_user_request, false, "access_token is invalid.", data, errors, method_timer.get_elapsed_seconds());
                return response;
			}

            json user_body = jp.parse_object(google_response.response.response_body.get_ptr());

            user_name = user_body["email"].as_string();
            user_email = user_body["email"].as_string();

            std::string user_first = user_body["given_name"].as_string();
            std::string user_last = user_body["family_name"].as_string();
            std::string user_picture = user_body["picture"].as_string();

			data.put_member(user_name_field, user_name);
			data.put_member(user_email_field, user_email);

			auto sys_perm = get_system_permission();

			json existing_user = get_user(user_name, false);

			if (not existing_user.object())
			{
				std::string user_password1;
				user_password1 = get_random_code() + get_random_code() + get_random_code();
				std::string hashed_pw = crypter.hash(user_password1);

				existing_user = jp.create_object();
				existing_user.put_member(class_name_field, std::string("sys_user"));
				existing_user.put_member(user_name_field, user_name);
				existing_user.put_member(user_email_field, user_email);
				existing_user.put_member(user_password_field, hashed_pw);
				existing_user.put_member(user_first_name_field, user_first);
				existing_user.put_member(user_last_name_field, user_last);
				existing_user.put_member("picture", user_picture);
				existing_user.put_member("confirmed_code", 1);
				json teams = get_team_by_email(user_email, sys_perm);
				for (json team : teams)
				{
					existing_user.put_member("home_team_name", team["team_name"].as_string());
					existing_user.put_member("team_name", team["team_name"].as_string());
				}

				apply_user_team(existing_user);

				json create_object_request = create_system_request(existing_user, default_user);
				json user_result = put_object(create_object_request);
				// we have to confirm if this guy did his job.
				json jerrors = user_result["errors"];
				if (user_result[success_field].as_bool()) {
					existing_user = get_user(user_name);
				} 				
				else 				
				{
					std::string message = user_result[message_field].as_string();
					system_monitoring_interface::active_mon->log_warning(std::format("Could not create user '{}': {}", user_name, message), __FILE__, __LINE__);
					system_monitoring_interface::active_mon->log_json(user_result);
					response = create_user_response(_sso_user_request, false, "User not created", existing_user, jerrors, method_timer.get_elapsed_seconds());
					system_monitoring_interface::active_mon->log_function_stop("login_user_sso", "complete", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
					return response;
				}
			}
			else {
				json teams = get_team_by_email(user_email, sys_perm);
				for (json team : teams)
				{
					existing_user.put_member("home_team_name",team["team_name"].as_string());
					existing_user.put_member("team_name", team["team_name"].as_string());
				}
				apply_user_team(existing_user);
				put_user(existing_user);
				existing_user = get_user(user_name);
			}

			response = create_response(user_name, auth_general, true, "Ok", existing_user, errors, method_timer.get_elapsed_seconds());
			system_monitoring_interface::active_mon->log_function_stop("login_user_sso", "complete", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);

			return response;
		}

		virtual json create_user(json create_user_request, bool _trusted_user = false, bool _system_user = false)
		{
			progress_fence progress(&calls_in_progress);

			timer method_timer;
			json_parser jp;
			json response;

			read_scope_lock my_lock(database_lock);

			date_time start_time = date_time::now();
			timer tx;

			system_monitoring_interface::active_mon->log_function_start("create_user", "start", start_time, __FILE__, __LINE__);

			json data = create_user_request[data_field];

			std::string user_name = data[user_name_field].as_string();
			std::string user_email = data[user_email_field].as_string();

			if (user_name.empty() and not user_email.empty()) {
				user_name = user_email;
			}
			else if (not user_name.empty() and user_email.empty()) {
				user_email = user_name;
            }
			else if (user_name.empty() and user_email.empty()) {
				validation_error_collection errors;
				validation_error err;
                err.class_name = "sys_user";
                err.field_name = user_name_field;	
                err.filename = get_file_name(__FILE__);
                err.line_number = __LINE__;
                err.message = "email/username is required";
                errors.push_back(err);
				err.field_name = user_email_field;
				errors.push_back(err);
				system_monitoring_interface::active_mon->log_function_stop("create_user", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				response = create_user_response(create_user_request, false, "An email is required.", data, errors, method_timer.get_elapsed_seconds());
				return response;
			}

			data.put_member(user_name_field, user_name);
			data.put_member(user_email_field, user_email);
			std::string user_password1 = data["password1"].as_string();
			std::string user_password2 = data["password2"].as_string();
			std::string user_class = "sys_user";	

			if (user_password1 != user_password2)
			{
				validation_error_collection errors;
				validation_error err;
                err.class_name = "sys_user";
				err.field_name = "password2";
				err.filename = get_file_name(__FILE__);
                err.line_number = __LINE__;
                err.message = "Passwords don't match";
				errors.push_back(err);
				system_monitoring_interface::active_mon->log_function_stop("create_user", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				response = create_user_response(create_user_request, false, "Passwords don't match", data, errors, method_timer.get_elapsed_seconds());
				return response;
			}
			// password complexity check

			password_metrics pm1(user_password1);
            std::string reason = pm1.is_stupid();

			if (not reason.empty())
			{
				validation_error_collection errors;
				validation_error err;
				err.class_name = "sys_user";
				err.field_name = "password1";
				err.filename = get_file_name(__FILE__);
				err.line_number = __LINE__;
				err.message = "Password too simple";
				errors.push_back(err);
				err.class_name = "sys_user";
				err.field_name = "password2";
				err.filename = get_file_name(__FILE__);
				err.line_number = __LINE__;
				err.message = "Password too simple";
				errors.push_back(err);

				system_monitoring_interface::active_mon->log_function_stop("create_user", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				response = create_user_response(create_user_request, false, std::format("Password too simple:{0}", reason), data, errors, method_timer.get_elapsed_seconds());
				return response;
			}

			bool user_exists = true;
			int attempt_count = 0;

			auto sys_perm = get_system_permission();

			json existing_user = get_user(user_name, false);

			if (existing_user.object())
			{
				json_parser jp;
                json existing_errors = jp.create_array();
				response = create_user_response(create_user_request, false, "User already exists.", existing_user, existing_errors, method_timer.get_elapsed_seconds());
				return response;
			}

			std::string hashed_pw = crypter.hash(user_password1);

			json create_user_params = jp.create_object();
			create_user_params.put_member(class_name_field, user_class);
			create_user_params.put_member(user_name_field, user_name);
			create_user_params.put_member(user_email_field, user_email);
			create_user_params.put_member(user_password_field, hashed_pw);
			create_user_params.copy_member(user_first_name_field, data);
			create_user_params.copy_member(user_last_name_field, data);
			create_user_params.copy_member(user_email_field, data);
			create_user_params.copy_member(user_mobile_field, data);
			create_user_params.copy_member(user_street1_field, data);
			create_user_params.copy_member(user_street2_field, data);
			create_user_params.copy_member(user_city_field, data);
			create_user_params.copy_member(user_state_field, data);
			create_user_params.copy_member(user_zip_field, data);

			// TODO: SYSTEM USER CONFIRMATION POLICY
			// the default system user doesn't need a confirmation process
			// this may change or at least be allowed as an option 
			// if in the future you really wanted to lock this down.

			create_user_params.put_member("confirmed_code", _system_user ? 1 : 0);

			std::string new_code = get_random_code();
			create_user_params.put_member("validation_code", new_code);

			json create_object_request = create_system_request(create_user_params);
			json user_result =  put_object(create_object_request);
			// we have to confirm if this guy did his job.
			json jerrors = user_result["errors"];
			if (user_result[success_field].as_bool()) {
				json new_user_wrapper = user_result[data_field]["sys_user"].get_element(0);
				new_user_wrapper = new_user_wrapper[data_field];
				if (not _system_user)
				{
					send_user_confirmation(new_user_wrapper, default_onboard_email);
				}
				new_user_wrapper.erase_member("password");
				new_user_wrapper.erase_member("confirmed_code");
				new_user_wrapper.erase_member("validation_code");
				response = create_user_response(create_user_request, true, "User created", new_user_wrapper, jerrors, method_timer.get_elapsed_seconds());
			}
			else
			{
                std::string message = user_result[message_field].as_string();
				system_monitoring_interface::active_mon->log_warning(std::format("Could not create user '{}': {}", user_name, message), __FILE__, __LINE__);
				system_monitoring_interface::active_mon->log_json(user_result);
				response = create_user_response(create_user_request, false, "User not created", create_user_params, jerrors, method_timer.get_elapsed_seconds());
			}

			system_monitoring_interface::active_mon->log_function_stop("create_user", "complete", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);

			return response;
		}


		virtual json user_send_code(json validation_code_request) override
		{
			timer method_timer;
			json_parser jp;
			json response;

			date_time start_time = date_time::now();
			timer tx;

			auto sys_perm = get_system_permission();

			system_monitoring_interface::active_mon->log_function_start("send_validation_code", "start", start_time, __FILE__, __LINE__);

			std::string user_name = validation_code_request[user_name_field].as_string();

			if (user_name.empty()) {
				json data = validation_code_request[data_field];
			
				if (data.object()) {
					user_name = data[user_name_field].as_string();
				}
			}

			if (user_name.empty()) {
				validation_error_collection errors;
				validation_error err;
				err.class_name = "sys_user";
				err.field_name = user_name_field;
				err.filename = get_file_name(__FILE__);
				err.line_number = __LINE__;
				err.message = "User required";
				errors.push_back(err);
				system_monitoring_interface::active_mon->log_function_stop("send_validation_code", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				response = create_user_response(validation_code_request, false, "user_name (typically an email address) is required", validation_code_request[data_field], errors, tx.get_elapsed_seconds());
				return response;
			}

			json user_info = get_user(user_name);

			std::string message = "Code not sent.";

			if (user_info.object()) {
				message = "Code sent";
				send_user_confirmation(user_info, recovery_email);
			}
			else 
			{
				validation_error_collection errors;
				validation_error err;
				err.class_name = "sys_user";
				err.field_name = user_name_field;
				err.filename = get_file_name(__FILE__);
				err.line_number = __LINE__;
				err.message = std::format("User '{}' not found", user_name);
				errors.push_back(err);
				message = "User not found.";
				system_monitoring_interface::active_mon->log_function_stop("send_validation_code", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				response = create_user_response(validation_code_request, false, message, validation_code_request[data_field], errors, tx.get_elapsed_seconds());
                return response;
			}

			json errors = jp.create_array();
			response = create_user_response(validation_code_request, true, message, validation_code_request[data_field], errors, tx.get_elapsed_seconds());

			system_monitoring_interface::active_mon->log_function_stop("send_validation_code", "complete", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);

			return response;
		}

		virtual void apply_user_team(json& user, json& team)
		{
			progress_fence progress(&calls_in_progress);

			json_parser jp;
			std::string user_name = user["user_name"].as_string();
		}

		virtual void apply_user_team(json user)
		{
			json_parser jp;
            json team = get_team(user["team_name"].as_string(), get_system_permission());
			if (not team.empty())
				apply_user_team(user, team);

			team = get_team(user["home_team_name"].as_string(), get_system_permission());
			if (not team.empty())
				apply_user_team(user, team);
		}

		// this allows a user to login
		virtual json user_confirm_code(json _confirm_request)
		{
			timer method_timer;
			json_parser jp;
			json response;

			date_time start_time = date_time::now();
			timer tx;

			read_scope_lock my_lock(database_lock);
			validation_error_collection errors;

			system_monitoring_interface::active_mon->log_function_start("confirm", "start", start_time, __FILE__, __LINE__);

			json data;
			std::string user_name = _confirm_request[user_name_field].as_string();
			std::string user_code = _confirm_request["validation_code"].as_string();
			if (user_name.empty()) {
				data = _confirm_request[data_field];
				if (data.object()) {
					user_name = data[user_name_field].as_string();
					user_code = data["validation_code"].as_string();
				}
            }

			auto sys_perm = get_system_permission();

			json user = get_user(user_name);

			if (user.empty()) {

				validation_error_collection errors;
				validation_error err;
				err.class_name = "sys_user";
				err.field_name = "user_name";
				err.filename = get_file_name(__FILE__);
				err.line_number = __LINE__;
				err.message = std::format("User '{}' not found", user_name);
				errors.push_back(err);

				response = create_user_response(_confirm_request, false, err.message, data, errors, method_timer.get_elapsed_seconds());
				return response;
			}

			std::string sys_code = user["validation_code"].as_string();

			if (sys_code == user_code)
			{
				user.put_member("confirmed_code", 1);
				std::string email = user[user_email_field].as_string();
				json teams = get_team_by_email(email, sys_perm);
				for (json team : teams) 
				{
					user.put_member("home_team_name", team["team_name"].as_string());
					user.put_member("team_name", team["team_name"].as_string());
				}
				apply_user_team(user);
				put_user(user);
				user = get_user(user_name);

				response = create_response(user_name, auth_general, true, "Ok", user, errors, method_timer.get_elapsed_seconds());
			}
			else
			{
				validation_error_collection errors;
				validation_error err;
				err.class_name = "sys_user";
				err.field_name = "validation_code";
				err.filename = get_file_name(__FILE__);
				err.line_number = __LINE__;
				err.message = "Incorrect validation code.";
				errors.push_back(err);

				response = create_user_response(_confirm_request, false, "Incorrect validation code.  Check your e-mail or send another one.", jp.create_object(), errors, method_timer.get_elapsed_seconds());
			}

			system_monitoring_interface::active_mon->log_function_stop("confirm", "complete", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);

			return response;
		}

		virtual json user_home(json _user_home_request)
		{
			progress_fence progress(&calls_in_progress);

			timer method_timer;
			json_parser jp;

			json result;
			json result_list;

			date_time start_time = date_time::now();
			timer tx;

			read_scope_lock my_lock(database_lock);
			validation_error_collection errors;

			system_monitoring_interface::active_mon->log_function_start("user_home", "start", start_time, __FILE__, __LINE__);

			std::string user_name, user_auth;

			if (not check_message(_user_home_request, { auth_self }, user_name, user_auth))
			{
				result = create_response(_user_home_request, false, "User home denied", jp.create_object(), errors, method_timer.get_elapsed_seconds());
				return result;
			}

            json user_details = get_user(user_name);

			system_monitoring_interface::active_mon->log_function_stop("user_home", "complete", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
            validation_error_collection empty_errors;
			result = create_response(_user_home_request, true, "Ok", user_details, empty_errors, method_timer.get_elapsed_seconds());

			return result;

		}

		// and user set password
		virtual json set_user_password(json _password_request)
		{
			timer method_timer;
			json_parser jp;
			json response;

			date_time start_time = date_time::now();
			timer tx;

			system_monitoring_interface::active_mon->log_function_start("confirm", "start", start_time, __FILE__, __LINE__);

			read_scope_lock my_lock(database_lock);
			validation_error_collection errors;

			json data = _password_request[data_field];
			std::string user_name, user_auth;
			std::string user_code = data["validation_code"].as_string();
			std::string requested_user_name = data[user_name_field].as_string();

			auto sys_perm = get_system_permission();

			json user = get_user(requested_user_name);

			if (user.empty()) {

				validation_error_collection errors;
				validation_error err;
				err.class_name = "sys_user";
				err.field_name = "user_name";
				err.filename = get_file_name(__FILE__);
				err.line_number = __LINE__;
				err.message = "Incorrect user_name.";
				errors.push_back(err);

				response = create_user_response(_password_request, false, "User not found", data, errors, method_timer.get_elapsed_seconds());
				return response;
			}

			std::string sys_code = user["validation_code"].as_string();

			if (user_code.size()>0 and sys_code.size()>0 and sys_code == user_code)
			{
				user.put_member("validation_code", std::string(""));
				user_name = requested_user_name;

			}
			else if (not check_message(_password_request, { auth_general }, user_name, user_auth))
			{
				validation_error_collection errors;
				validation_error err;
				err.class_name = "sys_user";
				err.field_name = "user_name";
				err.filename = get_file_name(__FILE__);
				err.line_number = __LINE__;
				err.message = "Denied.";
				errors.push_back(err);

				response = create_user_response(_password_request, false, "Set password denied", jp.create_object(), errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("confirm", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return response;
			}

			system_monitoring_interface::active_mon->log_function_start("confirm", "start", start_time, __FILE__, __LINE__);

			if (requested_user_name != user_name)
			{
				response = create_response(_password_request, false, "Set Password denied.", jp.create_object(), errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("confirm", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return response;
			}

			std::string user_password1 = data["password1"].as_string();
			std::string user_password2 = data["password2"].as_string();

			if (user_password1 != user_password2)
			{
				validation_error_collection errors;
				validation_error err;
				err.class_name = "sys_user";
				err.field_name = "password2";
				err.filename = get_file_name(__FILE__);
				err.line_number = __LINE__;
				err.message = "Passwords don't match.";
				errors.push_back(err);

				response = create_user_response(_password_request, false, "Passwords do not match.", jp.create_object(), errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("confirm", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return response;
			}

			password_metrics pm1(user_password1);

            std::string reason = pm1.is_stupid();
			if (not reason.empty())
			{
				validation_error_collection errors;
				validation_error err;
				err.class_name = "sys_user";
				err.field_name = "password1";
				err.filename = get_file_name(__FILE__);
				err.line_number = __LINE__;
				err.message = "Password too simple.";
				errors.push_back(err);


				response = create_user_response(_password_request, false, std::format( "Password fails complexity test: {0}", reason), jp.create_object(), errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("confirm", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return response;
			}

			std::string encrypted_password = crypter.hash(user_password1);
			user.put_member("password", encrypted_password);

			put_user(user);
			json jerrors;
			response = create_user_response(_password_request, true, "Ok", data, jerrors, method_timer.get_elapsed_seconds());

			system_monitoring_interface::active_mon->log_function_stop("confirm", "complete", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);

			return response;
		}

		virtual json user_set_team(json _user_set_team_request)
		{
			timer method_timer;
			json_parser jp;

			json result;
			json result_list;

			date_time start_time = date_time::now();
			timer tx;

			read_scope_lock my_lock(database_lock);
			validation_error_collection errors;

			system_monitoring_interface::active_mon->log_function_start("user_set_team", "start", start_time, __FILE__, __LINE__);

			std::string user_name, user_auth;

			if (not check_message(_user_set_team_request, { auth_general }, user_name, user_auth))
			{
				result = create_response(_user_set_team_request, false, "User set team denied", jp.create_object(), errors, method_timer.get_elapsed_seconds());
				return result;
			}

			bool team_set = false;
			std::string ok_message;
			json user_details = get_user(user_name);
			if (user_details.object()) {
				json jteam = user_details["home_team"];
				if (jteam.object()) {
					json jallowed_teams = jteam["allowed_teams"];
					if (auto ata = jallowed_teams.array_impl()) {
						if (std::any_of(ata->begin(), ata->end(), [&](json _item) {
							return _item.as_string() == _user_set_team_request["team_name"].as_string();
							})) {
							user_details.put_member("team_name", _user_set_team_request["team_name"].as_string());
							team_set = true;
                            ok_message = "Selected " + _user_set_team_request["team_name"].as_string();
						}
					}
				}
			}

			system_monitoring_interface::active_mon->log_function_stop("user_set_team", "complete", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);

			validation_error_collection empty_errors;

			if (team_set) {
				apply_user_team(user_details);
				put_user(user_details);
                user_details = get_user(user_name);
				result = create_response(_user_set_team_request, true, ok_message, user_details, empty_errors, method_timer.get_elapsed_seconds());
			}
			else
			{
				// clear user details to avoid any kind of leak of info
				user_details = jp.create_object();
				result = create_response(_user_set_team_request, false, "False", user_details, empty_errors, method_timer.get_elapsed_seconds());
			}

			return result;

		}

		// this starts a login attempt
		virtual json login_user(json _login_request)
		{
			progress_fence progress(&calls_in_progress);

			timer method_timer;
			json_parser jp;
			json response;

			validation_error_collection errors;

			date_time start_time = date_time::now();
			timer tx;

			read_scope_lock my_lock(database_lock);

			system_monitoring_interface::active_mon->log_function_start("login_user", "start", start_time, __FILE__, __LINE__);

			json data = _login_request;
			std::string user_name = data[user_name_field].as_string();
			std::string user_password = data["password"].as_string();
			std::string hashed_user_password;

			std::string hashed_pw = crypter.hash(user_password);

			json user = get_user(user_name);
			std::string pw = user["password"].as_string();

			if (pw == hashed_pw)
			{
				bool confirm = user["confirmed_code"].as_bool();

				json navigation_options = jp.create_object();

				if (user_name == default_user and default_user.size() > 0) 
				{
					json result = user.clone();
					result.erase_member("password");
					result.erase_member("confirmed_code");
					result.erase_member("validation_code");

					response = create_response(user_name, auth_system, true, "Ok", result, errors, method_timer.get_elapsed_seconds());
				}
				else if (confirm)
				{
					json result = user.clone();
					result.erase_member("password");
					result.erase_member("confirmed_code");
					result.erase_member("validation_code");

					response = create_response(user_name, auth_general, true, "Ok", result, errors, method_timer.get_elapsed_seconds());
				}
				else
				{
					response = create_response(user_name, auth_general, false, "Need confirmation", data, errors, method_timer.get_elapsed_seconds());
				}
			}
			else
			{
				response = create_response(_login_request, false, "Login Failed", jp.create_object(), errors, method_timer.get_elapsed_seconds());
			}
			system_monitoring_interface::active_mon->log_function_stop("login_user", "complete", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);

			return response;
		}

		virtual json run_object(json _run_object_request)
		{
			std::string user_name, user_auth;
			json_parser jp;
			json response;

			date_time start_time = date_time::now();
			timer tx;
			validation_error_collection errors;

			if (not check_message(_run_object_request, { auth_general }, user_name, user_auth))
			{
				response = create_response(_run_object_request, false, "run object denied", jp.create_object(), errors, tx.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("run_object", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return response;
			}

			json result = put_object(_run_object_request);
			if (result.error())
			{
				system_monitoring_interface::active_mon->log_function_stop("run_object", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			json edit_request_data = _run_object_request[data_field];
			if (edit_request_data.array())
				edit_request_data = edit_request_data.get_element(0);
			std::string class_name = edit_request_data[class_name_field].as_string();
			int64_t object_id = edit_request_data[object_id_field].as_int64_t();
			json edit_request = create_request(user_name, auth_general, edit_request_data);
            edit_request.put_member("include_children", true);
			result = edit_object(edit_request);
			if (result.error())
			{
				system_monitoring_interface::active_mon->log_function_stop("run_object", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}
			return result;
		}

		virtual json run_method(json _run_method_request)
		{
			std::string user_name, user_auth;
			json_parser jp;
			json response;

			date_time start_time = date_time::now();
			timer tx;
			validation_error_collection errors;

			if (not check_message(_run_method_request, { auth_general }, user_name, user_auth))
			{
				response = create_response(_run_method_request, false, "run method denied", jp.create_object(), errors, tx.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("run_method", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return response;
			}

            std::string token = _run_method_request[token_field].as_string();

			json edit_request_data = _run_method_request[data_field];
			if (edit_request_data.array())
				edit_request_data = edit_request_data.get_element(0);
			std::string class_name = edit_request_data[class_name_field].as_string();
			int64_t object_id = edit_request_data[object_id_field].as_int64_t();
            std::string method_path = edit_request_data[method_path_field].as_string();

			class_permissions perms = get_class_permission(user_name, class_name);
			class_permissions sys_perms = get_system_permission();

			json user = get_user(user_name);
			auto edit_class = read_lock_class(class_name);

			json result = jp.create_array();

			if (edit_class) {

				if (object_id > 0) {

					json key = jp.create_object();
					key.put_member(class_name_field, class_name);
                    key.put_member_i64(object_id_field, object_id);
                    json object = edit_class->get_single_object(this, key, true, perms);
					result = edit_class->run_method(this, method_path, token, class_name, object);
				}
			}

            response = create_response(_run_method_request, true, "Ok", result, errors, tx.get_elapsed_seconds());

			return response;
		}

		virtual json edit_object(json _edit_object_request) override
		{
			timer method_timer;
			json_parser jp;
			json result;
			json jedit_object;

			read_scope_lock my_lock(database_lock);

			date_time start_time = date_time::now();
			timer tx;
			std::string user_name, user_auth;

			json edit_request_data = _edit_object_request[data_field];

			bool include_children = _edit_object_request["include_children"].as_bool();
			validation_error_collection errors;

			if (not check_message(_edit_object_request, { auth_general }, user_name, user_auth))
			{
				result = create_response(_edit_object_request, false, "edit object denied", jp.create_object(), errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("edit_object", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			if (edit_request_data.empty())
			{
				result = create_response(_edit_object_request, false, "edit object bad request", jp.create_object(), errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("edit_object", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			system_monitoring_interface::active_mon->log_function_start("edit_object", "start", start_time, __FILE__, __LINE__);

			json token = _edit_object_request[token_field];
			json key = edit_request_data.extract({ class_name_field, object_id_field });
			int64_t object_id = key[object_id_field].as_int64_t();
			std::string class_name = key[class_name_field].as_string();

			class_permissions perms = get_class_permission(user_name, class_name);
			class_permissions sys_perms = get_system_permission();

			json user = get_user(user_name);
			auto edit_class = read_lock_class(class_name);

			if (edit_class) {

				if (object_id > 0) {
					jedit_object = edit_class->get_single_object(this, key, true, perms);
				}
				else {
					json create_object_body = jp.create_object();
					json create_object_data = jp.create_object();
					create_object_data.put_member(class_name_field, class_name);
					json create_object_request = create_system_request(create_object_data);
					json result = create_object(create_object_request);
					jedit_object = result[data_field];
				}

				result = jp.create_object();
				bool attempted = false;
				json jclasses = jp.create_object();

				if (not jedit_object.empty() and include_children)
				{
					std::string token = _edit_object_request[token_field].as_string();
					auto jchild_classes = edit_class->run_queries(this, token, class_name, jedit_object);
					result.share_member("child_classes", jchild_classes);
				}

				result.share_member("object", jedit_object);

				while (edit_class)
				{
					auto current_class_name = edit_class->get_class_name();
					json jedit_class = jp.create_object();
					edit_class->get_json(jedit_class);
					jclasses.share_member(current_class_name, jedit_class);
                    std::string edit_class_base = edit_class->get_base_class_name();
					if (edit_class_base.empty()) {
						edit_class.reset();
					}
					else 
					{
                        edit_class = read_lock_class(edit_class_base);
					}
				}
				result.share_member("classes", jclasses);

			}
			else 
			{
				system_monitoring_interface::active_mon->log_function_stop("edit_object", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return create_response(_edit_object_request, false, "Invalid class:" + class_name, jp.create_object(), errors, method_timer.get_elapsed_seconds());
			}

			system_monitoring_interface::active_mon->log_function_stop("edit_object", "success", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
			return create_response(_edit_object_request, true, "Ok", result, errors, method_timer.get_elapsed_seconds());
		}

		virtual json get_classes(json get_classes_request)
		{
			timer method_timer;
			json_parser jp;

			json result;
			json result_list;
			json object_list;
			json data;

			date_time start_time = date_time::now();
			timer tx;

			read_scope_lock my_lock(database_lock);
			validation_error_collection errors;

			system_monitoring_interface::active_mon->log_function_start("get_classes", "start", start_time, __FILE__, __LINE__);

			std::string user_name, user_auth;

			if (not check_message(get_classes_request, { auth_general }, user_name, user_auth))
			{
				result = create_response(get_classes_request, false, "get_classes denied", jp.create_object(), errors, method_timer.get_elapsed_seconds());
				return result;
			}

			object_list = classes->select(nullptr, jp.create_object(), [](json& _item)->json {
				return _item;
				});
            result_list = jp.create_array();

			// put on the afterburners.  my ai said this was faster...
			if (object_list.array()) {
				for (auto& item : *object_list.array_impl()) {
                    std::shared_ptr<json_object> jo = std::dynamic_pointer_cast<json_object>(item);	
					if (jo && jo->has_member(class_name_field)) {
						std::string class_name = (*jo)[class_name_field]->to_string();
						auto permission = get_class_permission(user_name, class_name);

						if (permission.get_grant != class_grants::grant_none)
						{
							auto classd = read_lock_class(class_name);
							json item = jp.create_object();
							if (classd) {
								classd->get_json(item);
//								json class_info = classd->get_info(this);
//								item.share_member("info", class_info);
                                result_list.push_back(item);
							}
						}
					}
				}
			}

			data = jp.create_object();
			data.share_member("class", result_list);

			system_monitoring_interface::active_mon->log_function_stop("get_classes", "complete", tx.get_elapsed_seconds(), result_list.size(), __FILE__, __LINE__);
			result = create_response(get_classes_request, true, "Ok", result_list, errors, method_timer.get_elapsed_seconds());

			return result;
		}

		virtual json get_class(json get_class_request)
		{
			progress_fence progress(&calls_in_progress);

			timer method_timer;
			json_parser jp;
			json result;

			date_time start_time = date_time::now();
			timer tx;

			read_scope_lock my_lock(database_lock);

			system_monitoring_interface::active_mon->log_function_start("get_class", "start", start_time, __FILE__, __LINE__);

			std::vector<std::string> missing_elements;
			validation_error_collection errors;


			if (not get_class_request.has_members(missing_elements, { token_field, class_name_field })) {
				std::string error_message;
				error_message = "get_class missing elements:";
				std::string comma = "";
				for (auto m : missing_elements) {
					error_message.append(comma);
					error_message.append(m);
				}
				json response = create_response(get_class_request, false, error_message, jp.create_object(), errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("get_class", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return response;
			}

			std::string user_name, user_auth;

			if (not check_message(get_class_request, { auth_general }, user_name, user_auth))
			{
				result = create_response(get_class_request, false, "get_class denied", jp.create_object(), errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("get_class", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			std::string class_name = get_class_request[class_name_field].as_string();

			auto permission = get_class_permission(user_name, class_name);

			json key = jp.create_object(class_name_field, class_name);
			key.set_natural_order();

			if (permission.get_grant != class_grants::grant_none) {


				auto classd = read_lock_class(class_name);
				if (classd) {
					json class_definition = jp.create_object();
					classd->get_json(class_definition);

//					json class_info = classd->get_info(this);

					result = jp.create_object();
					result.share_member("class", class_definition);
	//				result.share_member("info", class_info);

					result = create_response(get_class_request, true, "Ok", result, errors, method_timer.get_elapsed_seconds());
					system_monitoring_interface::active_mon->log_function_stop("get_class", "complete", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				}
				else {
					result = create_response(get_class_request, false, "Missing Class", key, errors, method_timer.get_elapsed_seconds());
					system_monitoring_interface::active_mon->log_function_stop("get_class", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				}
			}
			else {
				result = create_response(get_class_request, false, "get_class denied", key, errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("get_class", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
			}
			return result;
		}

		virtual json put_class(json put_class_request)
		{
			timer method_timer;
			json result;
			json_parser jp;

			validation_error_collection errors;

			date_time start_time = date_time::now();
			timer tx;

			std::string pc_name = "put_class";
			std::string pc_msg = put_class_request[data_field][class_name_field].as_string();
			std::string pc_start = pc_msg + " start";
			std::string pc_stop = pc_msg + " stop";
			std::string pc_failed = pc_msg + " failed";

			system_monitoring_interface::active_mon->log_function_start(pc_name, pc_start, start_time, __FILE__, __LINE__);

			std::vector<std::string> missing_elements;
			if (not put_class_request.has_members(missing_elements, { token_field })) {
				std::string error_message;
				error_message = "create_class missing elements:";
				std::string comma = "";
				for (auto m : missing_elements) {
					error_message.append(comma);
					error_message.append(m);
				}
				json response = create_response(put_class_request, false, error_message, jp.create_object(), errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop(pc_name, pc_failed, tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return response;
			}

			std::string user_name, user_auth;

			if (not check_message(put_class_request, { auth_general }, user_name, user_auth))
			{
				result = create_response(put_class_request, false, "put_class denied", jp.create_object(), errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop(pc_name, pc_failed, tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);

				return result;
			}

			json token = put_class_request[token_field];
			json jclass_definition = put_class_request[data_field];
			std::string class_name = jclass_definition[class_name_field].as_string();

			if (jclass_definition.error())
			{
				result = create_response(put_class_request, false, "Invalid class", jclass_definition, errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop(pc_name, pc_failed, tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			if (class_name.empty()) {
				result = create_response(put_class_request, false, "No class name", jclass_definition, errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop(pc_name, pc_failed, tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
			}

			auto permission = get_class_permission(
				user_name,
				class_name);

			if (permission.alter_grant == class_grants::grant_none) {
				result = create_response(put_class_request, false, "put_class denied", jclass_definition, errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop(pc_name, pc_failed, tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);

				return result;
			}

			activity pcactivity;
			pcactivity.db = this;

			auto pclass = put_class_impl(&pcactivity, jclass_definition);

			if (pclass) 
			{
				bool update_class = !put_class_request["is_class_update"].as_bool();
				if (update_class) {
					class_to_object(permission, class_name);
				}
				result = create_response(put_class_request, true, "Ok", jclass_definition, pcactivity.errors, method_timer.get_elapsed_seconds());
			}
			else 
			{
                log_errors(pcactivity.errors);
				result = create_response(put_class_request, false, "errors", jclass_definition, pcactivity.errors, method_timer.get_elapsed_seconds());
			}

			system_monitoring_interface::active_mon->log_function_stop(pc_name, pc_stop, tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
			return result;
		}

		virtual json object_to_class(class_permissions _permission, std::string _class_name) 
		{
			json_parser jp;
			json result;

            // get the class for the sys_class table,
			// so we can read our class record from it
			auto classd = read_get_class("sys_class");
			if (not classd)
				return result;

			json key = jp.create_object();
			key.put_member("name", _class_name);

			json class_object = classd->get_objects(this, key, true, _permission);

            if (class_object.array() and class_object.size() > 0) {
                json class_object = class_object.get_element(0);
                json new_class = jp.create_object();
				new_class.put_member("class_name", class_object["name"].as_string());
				new_class.put_member("class_description", class_object["description"].as_string());
				new_class.put_member("class_author", class_object["author"].as_string());
				new_class.put_member("class_version", class_object["version"].as_string());
				new_class.put_member("full_text", class_object["full_text"].value());
				new_class.put_member("fields", class_object["fields"].value());
				new_class.put_member("indexes", class_object["indexes"].value());
				new_class.put_member("sql", class_object["sql"].value());
				new_class.put_member("field_order", class_object["field_order"].value());

                json put_class_request = create_request(_permission.user_name, auth_general, new_class);
				put_class_request.put_member("is_class_update", true);
				result = put_class(put_class_request);
            }

			return result;
		}

		virtual json class_to_object(class_permissions _permission, std::string _class_name)
		{
			json_parser jp;
			json result;

			// get the class definition, so we can update the object to it
			auto classs = read_get_class(_class_name);
			if (not classs)
				return result;

            json class_definition = jp.create_object();
			classs->get_json(class_definition);

			// we want to find if this class has a matching definition
			auto classd = read_get_class("sys_class");
			if (not classd)
				return result;

			json key = jp.create_object();
			key.put_member("name", _class_name);

			json class_object = classd->get_objects(this, key, true, _permission);
            if (class_object.array() and class_object.size() > 0) {
                json class_object = class_object.get_element(0);
                class_object.put_member("description", class_definition["class_description"].as_string());
                class_object.put_member("author", class_definition["class_author"].as_string());
                class_object.put_member("version", class_definition["class_version"].as_string());
                class_object.put_member("full_text", class_definition["full_text"].value());
                class_object.put_member("fields", class_definition["fields"].value());
                class_object.put_member("indexes", class_definition["indexes"].value());
                class_object.put_member("sql", class_definition["sql"].value());
                class_object.put_member("field_order", class_definition["field_order"].value());				
                json put_object_request = create_request(_permission.user_name, auth_general, class_object);
				put_object_request.put_member("is_class_update", true);
                put_object(put_object_request);
            }
		}

        /// this method is used to alter a class definition.  
		/// it will take an alter class request, with a new field definition, and new index definitions, 
		/// get the existing class, then put the modifications in.
		/// This method can then be used by a UX to allow a simpler way of self modifying applications
		/// in flight.
		virtual json alter_class(json _alter_class_request) override
		{
			json result;
			json_parser jp;

			std::string user_name, user_auth;
			
			timer method_timer;
			timer tx;
			validation_error_collection errors;

			std::string pc_name = "alter_class";
			std::string pc_msg = _alter_class_request[data_field][class_name_field].as_string();
			std::string pc_start = pc_msg + " start";
			std::string pc_stop = pc_msg + " stop";
			std::string pc_failed = pc_msg + " failed";

			json_object data = corona::to_object(_alter_class_request[data_field]);

			std::string target_class = data[class_name_field]->as_string();
			json_array new_fields = data["fields"];
			json_array new_indexes = data["indexes"];
	
			if (not check_message(_alter_class_request, { auth_general }, user_name, user_auth))
			{
				result = create_response(_alter_class_request, false, "alter_class denied", jp.create_object(), errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop(pc_name, pc_failed, tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);

				return result;
			}

			if (auto classd = read_lock_class(target_class)) {
                json class_definition = jp.create_object();
				classd->get_json(class_definition);
				json_object classdef = corona::to_object(class_definition);
                json_object fields = classdef["fields"];
                json_object indexes = classdef["indexes"];	

                for (auto& field : new_fields) {
                    json_object jfield = corona::to_object(field);
                    std::string field_name = jfield["field_name"]->as_string();
                    if (not field_name.empty()) {
                        fields[field_name] = field;
                    }
                }

				for (auto& index : new_indexes) {
					json_object jindex = corona::to_object(index);
					std::string index_name = jindex["index_name"]->as_string();
					if (not index_name.empty()) {
						indexes[index_name] = index;
					}
				}

                json put_class_request = create_request(user_name, auth_general, classdef);

                result = put_class(put_class_request);
			}
			else 
			{
				result = create_response(_alter_class_request, false, "alter_class denied", jp.create_object(), errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop(pc_name, pc_failed, tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
			}

			return result;
		}

		

		virtual json query_class(std::string _user_name, json query_details) override
		{
			timer method_timer;
			json_parser jp;
			json response;

			date_time start_time = date_time::now();
			timer tx;
			validation_error_collection errors;

			bool include_children = query_details["include_children"].as_bool();

			json base_class_name = query_details[class_name_field];
			if (base_class_name.empty()) {
				response = create_response(_user_name, auth_general, false, "class_name not specified", query_details, errors, method_timer.get_elapsed_seconds());
				return response;
			}

			auto permission = get_class_permission(_user_name, base_class_name.as_string());
			if (permission.get_grant == class_grants::grant_none)
			{
				response = create_response(_user_name, auth_general, false, "query_class denied", query_details, errors, method_timer.get_elapsed_seconds());
				return response;
			}

			auto class_def = read_lock_class(base_class_name.as_string());

			json object_list = jp.create_array();
			if (not class_def) {
				response = create_response(_user_name, auth_general, false, "class not found", query_details, errors, method_timer.get_elapsed_seconds());
				return response;
			}

			json filter = query_details["filter"];
			if (filter.empty()) {
				filter = jp.create_object();
			}

            auto class_descendants = get_class_descendants(base_class_name.as_string());
			for (auto class_pair : class_descendants)
			{
				read_class_sp classd = read_lock_class(class_pair);
				if (classd) {
					json objects = classd->get_objects(this, filter, include_children, permission);

					if (objects.array()) {
						for (auto obj : objects) {
							if (permission.get_grant & class_grants::grant_any) {
								object_list.push_back(obj);
							}
							else if ((permission.get_grant & class_grants::grant_own) && obj["created_by"].as_string() == permission.user_name) {

								object_list.push_back(obj);
							}
							else if ((permission.get_grant & class_grants::grant_team) && obj["current_team"].as_string() == permission.team_name) {

								object_list.push_back(obj);
							}
						}
					}
				}
			}

			response = create_response(_user_name, auth_general, true, "completed", object_list, errors, method_timer.get_elapsed_seconds());

			return response;
		}

		virtual json query(json query_request)
		{
			progress_fence progress(&calls_in_progress);

			timer tx;
			read_scope_lock my_lock(database_lock);
			json_parser jp;
			json jx; // not creating an object, leaving it empty.  should work with empty objects
			// or with an object that has no members.
			json response;

			std::string user_name, user_auth;
			validation_error_collection errors;

			if (not check_message(query_request, { auth_general }, user_name, user_auth))
			{
				response = create_response(query_request, false, "query denied", jp.create_object(), errors, tx.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("query", "denied", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return response;
			}

			query_context context;

			context.froms_preloaded = true;

			json from_classes = query_request["from"];
			if (from_classes.array())
			{
				json stages = query_request["stages"];

				if (not stages.array() or stages.size() == 0)
				{
					response = create_response(query_request, false, "query has no stages", jp.create_object(), errors, tx.get_elapsed_seconds());
					system_monitoring_interface::active_mon->log_function_stop("query", "query has no stages", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
					return response;
				}

				for (auto from_class : from_classes)
				{
					std::string from_class_name = from_class[class_name_field].as_string();
					std::string from_name = from_class["name"].as_string();
					if (from_name.empty())
					{
						response = create_response(query_request, false, "from with no name", jp.create_object(), errors, tx.get_elapsed_seconds());
						system_monitoring_interface::active_mon->log_function_stop("query", "from with no name", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
						return response;
					}
					if (from_class_name.empty())
					{
						response = create_response(query_request, false, "from with no class", jp.create_object(), errors, tx.get_elapsed_seconds());
						system_monitoring_interface::active_mon->log_function_stop("query", "from with no class", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
						return response;
					}
					auto cd = read_lock_class(from_class_name);
					if (not cd) {
						std::string message = std::format("from class '{0}' not found", from_class_name);
						response = create_response(query_request, false, message, jp.create_object(), errors, tx.get_elapsed_seconds());
						system_monitoring_interface::active_mon->log_function_stop("query", message, tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
						return response;
					}
				}

				query_from fj(user_name, from_classes);

				fj.run(&context, this);

				if (context.is_error()) 
				{
					auto query_errors = context.get_errors();
					log_errors(query_errors);
					response = create_response(query_request, false, "query failed", jp.create_object(), query_errors, tx.get_elapsed_seconds());
					system_monitoring_interface::active_mon->log_function_stop("query", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
					return response;
				}

				// so now, we've loaded up our context, we can extract the stages
				context.put_json(query_request);

				// and, we can run the thing.
				json query_results = context.run();
				if (context.is_error()) {
					auto query_errors = context.get_errors();
					log_errors(query_errors);
                    std::string msg = query_errors.begin() != query_errors.end() ? query_errors.begin()->message : "query failed with unknown error";
					response = create_response(query_request, false, msg, query_results, query_errors, tx.get_elapsed_seconds());
					system_monitoring_interface::active_mon->log_function_stop("query", msg, tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				}
				else 
				{
					auto query_errors = context.get_errors();
					log_errors(query_errors);
					response = create_response(query_request, true, "completed", query_results, query_errors, tx.get_elapsed_seconds());
					system_monitoring_interface::active_mon->log_function_stop("query", "complete", tx.get_elapsed_seconds(), query_results.size(), __FILE__, __LINE__);
				}
			}
			else 
			{
				response = create_response(query_request, true, "query has no froms", jp.create_object(), errors, tx.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("query", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
			}
			return response;
		}


		virtual json create_object(json create_object_request)
		{
			timer method_timer;
			json_parser jp;
			using namespace std::literals;
			date_time start_time = date_time::now();
			timer tx;
			read_scope_lock my_lock(database_lock);
			system_monitoring_interface::active_mon->log_function_start("create_object", "start", start_time, __FILE__, __LINE__);

			json token = create_object_request[token_field];
			json data = create_object_request[data_field];
			std::string class_name = data[class_name_field].as_string();
			json response;

			std::string user_name, user_auth;
			validation_error_collection errors;

			if (not check_message(create_object_request, { auth_general }, user_name, user_auth))
			{
				response = create_response(create_object_request, false, "create_object denied", jp.create_object(), errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("create_object", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return response;
			}

			auto permission =  get_class_permission(user_name, class_name);
			if (permission.put_grant == class_grants::grant_none) {
				validation_error ve;
				ve.message = "put access denied";
				ve.class_name = class_name;
				ve.field_name = "n/a";
				ve.filename = __FILE__;
				ve.line_number = __LINE__;
				errors.push_back(ve);
				json result = create_response(create_object_request, false, "create_object denied", data, errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("create_object", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			auto class_def = read_lock_class(class_name);

			if (class_def) {

				if (not class_def->ready()) {
					validation_error ve;
					ve.message = "class not ready or failed definition (check base as well)";
					ve.class_name = class_name;
					ve.field_name = "n/a";
					ve.filename = __FILE__;
					ve.line_number = __LINE__;
					errors.push_back(ve);
					json result = create_response(create_object_request, false, "create_object failed", data, errors, method_timer.get_elapsed_seconds());
					system_monitoring_interface::active_mon->log_function_stop("create_object", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
					return result;
				}

				json new_object = jp.create_object();
				new_object.put_member(class_name_field, class_name);

				auto fields = class_def->get_fields();

				for (auto& field : fields)
				{
					std::string field_name = field->get_field_name();

					if (field_name == "created_by" or field_name == "created" or field_name == "updated_by" or field_name == "updated")
						continue;

					if (data.has_member(field_name))
					{
						new_object.copy_member(field_name, data);
						continue;
					}

					if (field_name == class_name_field) {
						new_object.put_member(field_name, class_name);
					}
					else if (field->get_field_type() == field_types::ft_object)
					{
						new_object.put_member_object(field_name);
					}
					else if (field->get_field_type() == field_types::ft_array or field->get_field_type() == field_types::ft_query)
					{
						new_object.put_member_array(field_name);
					}
					else if (field->get_field_type() == field_types::ft_double)
					{
						new_object.put_member(field_name, 0.0);
					}
					else if (field->get_field_type() == field_types::ft_string)
					{
						new_object.put_member(field_name, ""sv);
					}
					else if (field->get_field_type() == field_types::ft_int64)
					{
						new_object.put_member_i64(field_name, 0);
					}
					else if (field->get_field_type() == field_types::ft_datetime)
					{
						date_time dt;
						new_object.put_member(field_name, dt);
					}
					else if (field->get_field_type() == field_types::ft_function)
					{
						auto key = std::make_tuple(class_name, field_name);
						if (functions.contains(key)) {
							new_object.put_member_function(field_name, functions[key]);
						}
						else 
						{
							std::string err_message = std::format("function {0} {1} not defined", class_name, field_name);
							new_object.put_member(field_name, err_message);
						}
					}
				}
				int64_t new_id = class_def->get_next_object_id(this);
				new_object.put_member_i64("object_id", new_id);

                date_time cxnow = date_time::now();
                new_object.put_member("created_by", user_name);
				new_object.put_member("updated_by", user_name);
				new_object.put_member("created", cxnow);
				new_object.put_member("updated", cxnow);

				response = create_response(create_object_request, true, "Object created", new_object, errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("create_object", "complete", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
			}
			else {
				std::string msg = std::format("create_object failed because the class '{0}' was never found.", class_name);
				system_monitoring_interface::active_mon->log_warning(msg);
				response = create_response(create_object_request, false, "Couldn't find class", create_object_request, errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("create_object", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
			}

			return response;
			
		}

		virtual json put_object_nl(json put_object_request)
		{
			progress_fence progress(&calls_in_progress);

			timer method_timer;
			json_parser jp;
			json token;
			json object_definition;
			json result;

			try {

				date_time start_time = date_time::now();
				timer tx;
				validation_error_collection errors;

				system_monitoring_interface::active_mon->log_function_start("put_object", "start", start_time, __FILE__, __LINE__);

				object_definition = put_object_request[data_field];
				std::string user_name;
				int64_t total_objects = 0;

				if (object_definition.object()) {
                    total_objects = 1;
				}
				else if (object_definition.array()) {
                    total_objects = object_definition.size();
				}

				std::string authority = auth_general;
				std::string token_authority = auth_general;

				if (not check_message(put_object_request, { authority }, user_name, token_authority))
				{
					result = create_response(put_object_request, false, "Put object denied", jp.create_object(), errors, method_timer.get_elapsed_seconds());
					system_monitoring_interface::active_mon->log_function_stop("put_object", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
					return result;
				}

				result = check_object(object_definition, user_name, errors, token_authority);

				json grouped_by_class_name = result[data_field];

				std::string message = result["message"].as_string();

				int64_t success_objects = 0;
				int64_t failed_objects = 0;

				if (result[success_field].as_bool())
				{
					auto classes_and_data = grouped_by_class_name.get_members();

					json child_objects = jp.create_array();

					for (auto class_pair : classes_and_data)
					{
						auto cd = read_lock_class(class_pair.first);
						if (cd) {
							auto perms = get_class_permission(user_name, class_pair.first);
                            if (class_pair.first.starts_with("sys_")) {
								system_monitoring_interface::active_mon->log_information("put " + class_pair.first, __FILE__, __LINE__);
							}
							json put_results = cd->put_objects(this, child_objects, class_pair.second, perms, errors, success_objects, failed_objects);

                            grouped_by_class_name.put_member(class_pair.first, put_results);
                            auto ancestors = cd->get_ancestors();
                            if (cd->get_class_name
								() == "sys_team" || ancestors.find("sys_team") != ancestors.end()) {
								permissions_cache.clear();
							}
                            if (cd->get_class_name() == "sys_user" || ancestors.find("sys_user") != ancestors.end()) {
                                permissions_cache.clear();
                            }
						}
					}


					if (child_objects.size() > 0) {
						put_object_request.put_member(data_field, child_objects);
						result = put_object_nl(put_object_request);
					}

					for (auto class_pair : classes_and_data) {
						bool update_class = !put_object_request["is_class_update"].as_bool();

						// avoid recursision if this is a class update (put_class->put_object and put_object->put_class)
						// the object here is a sys_class, and we're going to update the class definition, 
						if (update_class && class_pair.first == "sys_class") {
							auto perms = get_class_permission(user_name, class_pair.first);
							object_to_class(perms, class_pair.first);
						}
					}
						
					result = create_response(put_object_request, true, message, grouped_by_class_name, errors, method_timer.get_elapsed_seconds(), success_objects, failed_objects, total_objects);
					log_errors(errors);
				}
				else
				{
					result = create_response(put_object_request, false, message, grouped_by_class_name, errors, method_timer.get_elapsed_seconds());
					log_errors(errors);
				}
				system_monitoring_interface::active_mon->log_function_stop("put_object", "complete", tx.get_elapsed_seconds(), total_objects, __FILE__, __LINE__);
			}
			catch (std::exception exc)
			{
                result = jp.create_object();
                result.put_member(success_field, false);
                result.put_member(message_field, "Exception: " + (std::string)exc.what());
			}
			return result;
		}

		virtual int64_t commit()
		{
			
			return 1;
		}

		virtual json put_object(json put_object_request)
		{
			read_scope_lock my_lock(database_lock);

			return put_object_nl(put_object_request);
		}

		virtual void put_object_async(json put_object_request, std::function<void(json& result, double exec_time)> on_complete)
		{
			global_job_queue->submit_job([this, put_object_request, on_complete]() {
				timer tx;
				// this is a lambda that will run in the job queue
				// it will call the put_object_nl method, which will
				// lock the database and do the work.
				read_scope_lock my_lock(database_lock);
				json result = this->put_object_nl(put_object_request);
				if (on_complete)
				{
					on_complete(result, tx.get_elapsed_seconds());
				}
			}, NULL);
		}

		virtual void put_object_sync(json put_object_request, std::function<void(json& result, double exec_time)> on_complete)
		{
			timer tx;
				// lock the database and do the work.
			read_scope_lock my_lock(database_lock);
			json result = this->put_object_nl(put_object_request);
			if (on_complete)
			{
				on_complete(result, tx.get_elapsed_seconds());
			}
		}

		virtual json get_object(json get_object_request)
		{
			progress_fence progress(&calls_in_progress);

			timer method_timer;
			json_parser jp;
			json result;
			read_scope_lock my_lock(database_lock);
			date_time start_time = date_time::now();
			timer tx;

			system_monitoring_interface::active_mon->log_function_start("get_object", "start", start_time, __FILE__, __LINE__);

			std::string user_name, user_auth;
			json object_key = get_object_request.extract({ class_name_field, object_id_field });

			validation_error_collection errors;
			if (not check_message(get_object_request, { auth_general }, user_name, user_auth))
			{
				result = create_response(get_object_request, false, "Denied", jp.create_object(), errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("get_object", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			std::string class_name = object_key[class_name_field].as_string();

			auto permission =  get_class_permission(user_name, class_name);
			if (permission.get_grant == class_grants::grant_none) {
				json result = create_response(get_object_request, false, "Denied", jp.create_object(), errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("get_object", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return result;
			}

			bool include_children = get_object_request["include_children"].as_bool();
			
			json obj =  select_single_object(object_key, include_children, permission);
			if (obj.object()) 
			{
				if (include_children)
				{
					auto edit_class = read_lock_class(class_name);
					if (edit_class) {
						std::string token = get_object_request[token_field].as_string();
						edit_class->run_queries(this, token, class_name, obj);
					}
				}

				result = create_response(get_object_request, true, "Ok", obj, errors, method_timer.get_elapsed_seconds());
			}
			else 
			{
				result = create_response(get_object_request, false, "Not found", object_key, errors, method_timer.get_elapsed_seconds());
			}
			system_monitoring_interface::active_mon->log_function_stop("get_object", "complete", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);

			return result;
		}


		virtual json delete_object(json delete_object_request)
		{
			progress_fence progress(&calls_in_progress);

			timer method_timer;
			json response;
			json_parser jp;
			read_scope_lock my_lock(database_lock);
			date_time start_time = date_time::now();			
			timer tx;

			validation_error_collection errors;

			system_monitoring_interface::active_mon->log_function_start("delete_object", "start", start_time, __FILE__, __LINE__);

			std::string user_name, user_auth;
			json object_key = delete_object_request.extract({ class_name_field, object_id_field });

			if (not check_message(delete_object_request, { auth_general }, user_name, user_auth))
			{
				response = create_response(delete_object_request, false, "Denied", jp.create_object(), errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("delete_object", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return response;
			}

			std::string class_name = object_key[class_name_field].as_string();

			auto permission = get_class_permission(user_name, class_name);
			if (permission.delete_grant == class_grants::grant_none) {
				response = create_response(delete_object_request, false, "Denied", jp.create_object(), errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("get_object", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return response;
			}

			auto cd = read_lock_class(class_name);
			if (cd) {

				cd->delete_objects(this, object_key, true, permission);

				response = create_response(delete_object_request, true, "Ok", jp.create_object(), errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("delete_object", "complete", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
			}
			else 
			{
				response = create_response(delete_object_request, true, "class not found", jp.create_object(), errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("delete_object", "complete", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
			}


			return response;
		}

		virtual json copy_object(json copy_request)
		{
			progress_fence progress(&calls_in_progress);

			timer method_timer;
			json_parser jp;
			read_scope_lock my_lock(database_lock);
			json response;

			date_time start_time = date_time::now();
			timer tx;
			system_monitoring_interface::active_mon->log_function_start("copy_object", "start", start_time, __FILE__, __LINE__);

			validation_error_collection errors;
			std::string user_name, user_auth;

			if (not check_message(copy_request, { auth_general }, user_name, user_auth))
			{
				response = create_response(copy_request, false, "Denied", jp.create_object(), errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("copy_object", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return response;
			}

			json source_spec = copy_request["from"];
			std::string source_class = source_spec[class_name_field].as_string();
			std::string source_path = source_spec["path"].as_string();
			json source_key = source_spec.extract({ class_name_field, object_id_field });

			json dest_spec = copy_request["to"];
			std::string dest_class = dest_spec[class_name_field].as_string();
			std::string dest_path = dest_spec["path"].as_string();
			json dest_key = dest_spec.extract({ class_name_field, object_id_field });

			json transform_spec = copy_request["transform"];
			std::string transform_class = transform_spec[class_name_field].as_string();

			class_permissions src_permission, dest_permission, trans_permission;

			src_permission = get_class_permission(user_name, source_class);
			if (src_permission.get_grant == class_grants::grant_none) {
				response = create_response(copy_request, false, "Denied", source_spec, errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("copy_object", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return response;
			}

			dest_permission = get_class_permission(user_name, dest_class);
			if (dest_permission.put_grant == class_grants::grant_none) {
				response = create_response(copy_request, false, "Denied", dest_spec, errors, method_timer.get_elapsed_seconds());
				system_monitoring_interface::active_mon->log_function_stop("copy_object", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
				return response;
			}

			if (not transform_class.empty()) {
				trans_permission = get_class_permission(user_name, transform_class);
				if (trans_permission.get_grant == class_grants::grant_none) {
					response = create_response(copy_request, false, "Denied", transform_spec, errors, method_timer.get_elapsed_seconds());
					system_monitoring_interface::active_mon->log_function_stop("copy_object", "failed", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
					return response;
				}
			}

			// load the complete object
			json object_source_array = select_object(source_key, true, src_permission);
			json object_source = object_source_array.get_first_element();

			json object_to_copy_result;
			json source_object;

			if (not object_source.empty())
			{
				// if the source object is not empty, then, walk the source path to extract the child object we want
				object_to_copy_result = object_source.query(source_path);
				source_object = object_to_copy_result["value"];

				// if that worked, then let's go for the copy
				if (source_object.object()) 
				{

					// transforms this to a new object....
					// because this is a deep copy.
                    remove_object_ids(source_object);

					json new_object;
					// we transform the object into a new class while copying, if there is a 
					// transform specification

					if (not transform_class.empty()) {
						// so we create a new object of the class
						json transform_key = transform_spec.extract({ class_name_field });
						json cor = create_request(user_name, auth_general, transform_spec);
						json new_object_result = create_object(cor);

						// then we can copy the fields over
						if (new_object_result[success_field].as_bool()) {
							new_object = new_object_result[data_field];
							auto new_object_fields = new_object.get_members();
							for (auto nof : new_object_fields) {
								if (source_object.has_member(nof.first)) {
									new_object.copy_member(nof.first, source_object);
								}
							}
						}
					}

                    // ok, two cases.  Either the destination is a new object, in which case we just put the new object there, or the destination is an existing object, in which case we need to merge the new object into the existing one.  We can tell which case it is by whether the dest_path is empty or not.  If it's empty, then we are putting the new object at the destination key.  If it's not empty, then we are merging into an existing object.
					// or, the destionation is a member of an existing object.

					if (dest_key["object_id"].as_int64_t() == 0)
					{
						json por = create_request(user_name, auth_general, new_object);
						response = put_object(por);
					}
					else 
					{
						//
						json object_dest = select_single_object(dest_key, true, dest_permission);

						// now we are going to put this object in our destination
						json object_dest_result = object_dest.query(dest_path);

						if (object_dest_result.object()) {
							json update_obj;
							json target = object_dest_result["target"];
							std::string name = object_dest_result["name"].as_string();
							update_obj = object_dest_result["object"];
							if (target.array())
							{
								target.push_back(new_object);
							}
							else if (target.object())
							{
								target.put_member(name, new_object);
							}
							json por = create_request(user_name, auth_general, update_obj);
							response = put_object(por);
						}
						else 
						{
							response = create_response(copy_request, false, "could not find dest object", object_dest, errors, method_timer.get_elapsed_seconds());;
						}
					}
				}
				else 				
				{
					response = create_response(copy_request, false, "could not find source object", object_source, errors, method_timer.get_elapsed_seconds());;
				}
			}
			else {
				response = create_response(copy_request, false, "source object not specified", object_source, errors, method_timer.get_elapsed_seconds());;;
			}

			system_monitoring_interface::active_mon->log_function_stop("copy_object", "complete", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);
			return response;
		}

		virtual json add_item_chest(json add_to_chest_request) override
		{
			return corona_function("add_item_chest", add_to_chest_request,
				[this](json data, std::string user_name, std::string user_auth, validation_error_collection& errors) {
					return this->add_item_chest_impl(data, user_name, user_auth, errors);
				});
		}

		virtual json remove_item_chest(json remove_from_chest_request) override
		{
			return corona_function("remove_item_chest", remove_from_chest_request,
				[this](json data, std::string user_name, std::string user_auth, validation_error_collection& errors) {
					return this->remove_item_chest_impl(data, user_name, user_auth, errors);
				});
		}

		virtual json move_item_chest(json move_chest_request) override
		{
			return corona_function("move_item_chest", move_chest_request,
				[this](json data, std::string user_name, std::string user_auth, validation_error_collection& errors) {
					return this->move_item_chest_impl(data, user_name, user_auth, errors);
				});
		}


		private:

		json create_system_request(json _data, std::string _override_user = "")
		{
			progress_fence progress(&calls_in_progress);

			json_parser jp;

			json payload;

			payload = jp.create_object();

			json token = jp.create_object();
            std::string user_name = _data[user_name_field].as_string();
            if (not _override_user.empty()) {
				user_name = _override_user;
			}
			if (user_name.empty()) {
				user_name = default_user;
            }
			token.put_member(user_name_field, user_name);
			token.put_member(authorization_field, auth_system);
			date_time expiration = date_time::utc_now() + this->token_life;
			token.put_member(token_expires_field, expiration);
			std::string cipher_text = crypter.encrypt(token, get_pass_phrase(), get_iv());
			token.put_member(signature_field, cipher_text);

			std::string token_string = token.to_json_typed();
			std::string base64_token_string = base64_encode(token_string);

			payload.put_member(token_field, base64_token_string);

			payload.put_member(data_field, _data);

			return payload;
		}


		json create_request(std::string _user_name, std::string _authorization, json _data)
		{
			progress_fence progress(&calls_in_progress);

			json_parser jp;

			json payload;
			
			payload = jp.create_object();
			json token = jp.create_object();
			token.put_member(user_name_field, _user_name);
			token.put_member(authorization_field, _authorization);
			date_time expiration = date_time::utc_now() + this->token_life;
			token.put_member(token_expires_field, expiration);
			std::string cipher_text = crypter.encrypt(token, get_pass_phrase(), get_iv());
			token.put_member(signature_field, cipher_text);

			std::string token_string = token.to_json_typed();
			std::string base64_token_string = base64_encode(token_string);

			payload.put_member(token_field, base64_token_string);
			payload.share_member(data_field, _data);

			return payload;
		}


		json create_get_request(std::string _user_name, std::string _authorization, std::string _class_name, int64_t _object_id)
		{
			json_parser jp;

			json payload;
			payload = jp.create_object();

			json token = jp.create_object();
			token.put_member(user_name_field, _user_name);
			token.put_member(authorization_field, _authorization);
			date_time expiration = date_time::utc_now() + this->token_life;
			token.put_member(token_expires_field, expiration);
			std::string cipher_text = crypter.encrypt(token, get_pass_phrase(), get_iv());
			token.put_member(signature_field, cipher_text);

			std::string token_string = token.to_json_typed();
			std::string base64_token_string = base64_encode(token_string);

			payload.put_member(token_field, base64_token_string);
			payload.put_member(class_name_field, _class_name);
			payload.put_member_i64(object_id_field, _object_id);

			return payload;
		}

	};

	compiled_object_view::compiled_object_view(corona_database_interface *_db, class_interface* _class_definition)
	{

		database = _db;
		class_definition = _class_definition;

		auto& field_definitions = class_definition->use_fields();

		for (auto& _class_field : field_definitions) 
		{
			compiled_field_view cfv;
			cfv.field_definition = _class_field.second;
			cfv.field_value = nullptr;
			cfv.index_required = false;
			auto indexes = _class_definition->get_indexes();

			for (auto index : indexes) {
				std::vector<std::string>& keys = index->get_index_keys();
                for (auto& key : keys) {
					if (key == cfv.field_definition->get_field_name()) {
						cfv.index_required = true;
					}
				}
			}

			fields.push_back(cfv);
		}
	}

	bool compiled_object_view::check(json& _object, validation_error_collection& errors)
	{
		bool succeeded = true;

		auto oimpl = _object.object_impl();
		auto ovalue = _object.value();
		last_object = ovalue;

		for (auto& fld : fields) 
		{
			auto it = oimpl->at(fld.field_definition->get_field_name());
			if (it) {
				
				fld.field_value = it;
				if (fld.field_value) {
					auto obj_type = fld.field_value->get_field_type();
					auto member_type = fld.field_definition->get_field_type();

					if (member_type == field_types::ft_query) {
						continue;
					}

					fld.field_value = it;
					if (member_type != obj_type) {
						_object.change_member_type(fld.field_definition->get_field_name(), member_type);
						fld.field_value = oimpl->at(fld.field_definition->get_field_name());
					}
					json test_value(fld.field_value);
					bool accept_result = fld.field_definition->accepts(database, errors, class_definition->get_class_name(), fld.field_definition->get_field_name(), test_value);

					if (accept_result == false)
					{
						succeeded = false;
					}
				}
			}
			else 
			{
				fld.field_value = nullptr;
				if (!fld.index_required) {
					auto options = fld.field_definition->get_options();
					if (options && options->is_required()) {
						validation_error ve;
						ve.message = "required field missing from object";
						ve.class_name = class_definition->get_class_name();
						ve.field_name = fld.field_definition->get_field_name();
						ve.filename = __FILE__;
						ve.line_number = __LINE__;
						errors.push_back(ve);
						succeeded = false;
					}
				}
				else 
				{
					succeeded = true;
				}
			}
        }

		return succeeded;
	}

	////

	class_interface* activity::get_class(std::string _class_name)
	{
		auto fi = classes.find(_class_name);

		if (fi != classes.end()) {
			return fi->second.get();
		}

		return nullptr;
	}

	class_interface* activity::create_class(std::string _class_name)
	{
		auto fi = classes.find(_class_name);

		if (fi != classes.end()) {
			return fi->second.get();
		}

		std::shared_ptr<class_implementation> class_impl = std::make_shared<class_implementation>();
		std::shared_ptr<class_interface> iclass = std::dynamic_pointer_cast<class_interface>(class_impl);
		classes.insert_or_assign(_class_name, iclass);
		return get_class(_class_name);
	}


	bool test_database_engine(json& _proof, std::shared_ptr<application> _app)
	{
		bool success = true;
		using namespace std::literals;
		date_time st = date_time::now();
		timer tx;
		system_monitoring_interface::active_mon->log_function_start("table proof", "start", st, __FILE__, __LINE__);

		std::shared_ptr<file> f = _app->create_file_ptr(FOLDERID_Documents, "ig_test.cdb");

		json_parser jp;
		json proof_assertion = jp.create_object();

		proof_assertion.put_member("test_name", "database"sv);

		json dependencies = jp.parse_object(R"( 
{ 
	"create" : [  "table.get", "table.put", "table.create", "table.select" ],
	"login" : [  "table.get", "table.put",  "table.select" ],
    "class" : [ "table.get", "table.put", "table.create", "table.select" ],
	"class_index" : [ "table.get", "table.put", "table.create", "table.select" ],
	"create" : [ "table.get", "table.put", "table.select" ],
	"put" : [ "table.get", "table.put", "table.select" ],
	"put_index" : [ "table.get", "table.put", "table.select" ],
	"get" : [ "table.get" ],
	"get_index" : [ "table.get", "table.select" ],
	"delete" : [ "table.erase" ],
	"team_class" : [ "table.get", "table.put", "table.create", "table.select" ],
	"team_get" : [ "table.get", "table.select" ],
	"team_put" : [ "table.get", "table.put", "table.select" ],
	"team_delete" : [ "table.get", "table.put", "table.create", "table.select" ]
}
)");

		bool login_success;

		date_time start_schema = date_time::now();
		system_monitoring_interface::active_mon->log_job_start("test_database_engine", "start", start_schema, __FILE__, __LINE__);

		corona_database db("test", "test");

		proof_assertion.put_member("dependencies", dependencies);

		json system_config = jp.create_object();
		json server_config = jp.create_object();
		json servers = jp.create_array();

		server_config.put_member(sys_user_name_field, "todd"sv);
		server_config.put_member(sys_user_password_field, "randomite"sv);
		server_config.put_member(sys_user_email_field, "todd.bandrowsky@gmail.com"sv);
		server_config.put_member(sys_default_team_field, "GuestTeam"sv);

		// Add any other required server config fields here, e.g.:
		// server_config.put_member("schema_filename", "revolution_schema.json");
		// server_config.put_member("database_threads", 4);
		// server_config.put_member("database_recreate", true);

        servers.push_back(server_config);
		system_config.put_member("Servers", servers);

		// Now apply the config to the database
		db.apply_config(system_config, server_config);
        json create_result = db.create_database();
		relative_ptr_type database_location	= create_result.as_int64_t();

		login_success = true;

		json login_positive_request = R"(
{
	"user_name" : "todd",
	"password" : "randomite"
}
)"_jobject;

		json login_negative_request = R"(
{
	"user_name" : "todd",
	"password" : "reachio"
}
)"_jobject;

		json login_result;

		login_result = db.login_user(login_negative_request);

		if (login_result[success_field].as_bool()) {
			login_success = false;
			system_monitoring_interface::active_mon->log_warning("able to login with bad account", __FILE__, __LINE__);
		}

		login_result = db.login_user(login_positive_request);

		if (not login_result[success_field].as_bool()) {
			login_success = false;
			system_monitoring_interface::active_mon->log_warning("can't with good account", __FILE__, __LINE__);
		}


		system_monitoring_interface::active_mon->log_job_stop("test_database_engine", "complete", tx.get_elapsed_seconds(), 1, __FILE__, __LINE__);

		return success;
	}

	bool collection_tests()
	{

		try {

			application app;

			std::shared_ptr<dynamic_box> box = std::make_shared<dynamic_box>();
			box->init(1 << 21);

			jschema schema;
			relative_ptr_type schema_id;

			schema = jschema::create_schema(box, 20, true, schema_id);

			relative_ptr_type quantity_field_id = null_row;
			relative_ptr_type last_name_field_id = null_row;
			relative_ptr_type first_name_field_id = null_row;
			relative_ptr_type birthday_field_id = null_row;
			relative_ptr_type count_field_id = null_row;
			relative_ptr_type title_field_id = null_row;
			relative_ptr_type institution_field_id = null_row;

			schema.bind_field("quantity", quantity_field_id);
			schema.bind_field("lastName", last_name_field_id);
			schema.bind_field("firstName", first_name_field_id);
			schema.bind_field("birthday", birthday_field_id);
			schema.bind_field("count", count_field_id);
			schema.bind_field("title", title_field_id);
			schema.bind_field("institutionName", institution_field_id);

			jcollection_ref ref;
			ref.data = std::make_shared<dynamic_box>(1 << 20);
			ref.max_objects = 50;
			ref.collection_size_bytes = 1 << 19;

			init_collection_id(ref.collection_id);

			jcollection people = schema.create_collection(&ref);

			put_class_request person;

			person.class_name = "person";
			person.class_description = "a person";
			person.member_fields = { last_name_field_id, first_name_field_id, birthday_field_id, count_field_id, quantity_field_id };
			relative_ptr_type person_class_id = schema.put_class(person);

			if (person_class_id == null_row)
			{
				std::cout << __LINE__ << ":class create failed failed" << std::endl;
				return false;
			}

			int birthdaystart = 1941;
			int countstart = 12;
			double quantitystart = 10.22;
			int increment = 5;

			relative_ptr_type people_object_id;

			auto sl = people.create_object(person_class_id, people_object_id);
			auto last_name = sl.get_string(0);
			auto first_name = sl.get_string(1);
			auto birthday = sl.get_time(2);
			auto count = sl.get_int64(3);
			auto qty = sl.get_double(4);
			last_name = "last 1";
			first_name = "first 1";
			birthday = birthdaystart + increment * 0;
			count = countstart + increment * 0;
			qty = quantitystart + increment * 0;

			sl = people.create_object(person_class_id, people_object_id);
			last_name = sl.get_string(0);
			first_name = sl.get_string(1);
			birthday = sl.get_time(2);
			count = sl.get_int64(3);
			qty = sl.get_double(4);
			last_name = "last 2";
			first_name = "first 2";
			birthday = birthdaystart + increment * 1;
			count = countstart + increment * 1;
			qty = quantitystart + increment * 1;

			sl = people.create_object(person_class_id, people_object_id);
			last_name = sl.get_string(0);
			first_name = sl.get_string(1);
			birthday = sl.get_time(2);
			count = sl.get_int64(3);
			qty = sl.get_double(4);
			last_name = "last 3";
			first_name = "first 3";
			birthday = birthdaystart + increment * 2;
			count = countstart + increment * 2;
			qty = quantitystart + increment * 2;

			sl = people.create_object(person_class_id, people_object_id);
			last_name = sl.get_string(0);
			first_name = sl.get_string(1);
			birthday = sl.get_time(2);
			count = sl.get_int64(3);
			qty = sl.get_double(4);
			last_name = "last 4";
			first_name = "first 4";
			birthday = birthdaystart + increment * 3;
			count = countstart + increment * 3;
			qty = quantitystart + increment * 3;

			sl = people.create_object(person_class_id, people_object_id);
			last_name = sl.get_string(0);
			first_name = sl.get_string(1);
			birthday = sl.get_time(2);
			count = sl.get_int64(3);
			qty = sl.get_double(4);
			last_name = "last 5 really long test 12345 abcde 67890 fghij 12345 klmno 67890 pqrst";
			first_name = "first 5 really long test 12345 abcde 67890 fghij 12345 klmno 67890 pqrst";
			birthday = birthdaystart + increment * 4;
			count = countstart + increment * 4;
			qty = quantitystart + increment * 4;

			int inc_count = 0;

			for (auto sl : people)
			{
				last_name = sl.item.get_string(0);
				if (not last_name.starts_with("last")) {
					std::cout << __LINE__ << ":last name failed" << std::endl;
					return false;
				}
				first_name = sl.item.get_string(1);
				if (not first_name.starts_with("first")) {
					std::cout << __LINE__ << ":first name failed" << std::endl;
					return false;
				}
				birthday = sl.item.get_time(2);
				count = sl.item.get_int64(3);
				qty = sl.item.get_double(4);

				if (birthday != birthdaystart + increment * inc_count) {
					std::cout << __LINE__ << ":birthday failed" << std::endl;
					return false;
				}

				if (count != countstart + increment * inc_count) {
					std::cout << __LINE__ << ":count failed" << std::endl;
					return false;
				}

				if (qty != quantitystart + increment * inc_count) {

					std::cout << __LINE__ << ":qty failed" << std::endl;
					return false;
				}

				inc_count++;
			}

			return true;
		}
		catch (std::exception exc)
		{
			system_monitoring_interface::active_mon->log_exception(exc);
			return false;
		}

	}

	bool array_tests()
	{

		try {
			std::shared_ptr<dynamic_box> box = std::make_shared<dynamic_box>();
			box->init(1 << 21);
			application app;

			jschema schema;
			relative_ptr_type schema_id;

			schema = jschema::create_schema(box, 50, true, schema_id);

			put_class_request sprite_frame_request;

			sprite_frame_request.class_name = "spriteframe";
			sprite_frame_request.class_description = "sprite frame";
			sprite_frame_request.member_fields = { "shortName", "object_x", "object_y", "object_w", "object_h", "color" };
			relative_ptr_type sprite_frame_class_id = schema.put_class(sprite_frame_request);

			if (sprite_frame_class_id == null_row) {
				std::cout << __LINE__ << ":class create failed failed" << std::endl;
				return false;
			}

			put_class_request sprite_class_request;
			sprite_class_request.class_name = "sprite";
			sprite_class_request.class_description = "sprite";
			sprite_class_request.member_fields = { "shortName", "object_x", "object_y", "object_w", "object_h" , member_field(sprite_frame_class_id) };
			relative_ptr_type sprite_class_id = schema.put_class(sprite_class_request);

			if (sprite_class_id == null_row) {
				std::cout << __LINE__ << ":class create failed failed" << std::endl;
				return false;
			}

			collection_id_type colid;

			init_collection_id(colid);

			relative_ptr_type classesb[2] = { sprite_class_id, null_row };

			jcollection_ref ref;
			ref.data = std::make_shared<dynamic_box>(1 << 20);
			ref.max_objects = 50;
			ref.collection_size_bytes = 1 << 19;

			init_collection_id(ref.collection_id);

			jcollection sprites = schema.create_collection(&ref);

			for (int i = 0; i < 10; i++) {

				relative_ptr_type new_sprite_id;
				auto slice = sprites.create_object(sprite_class_id, new_sprite_id);
				auto image_name = slice.get_string(0);


				auto bx = slice.get_int32("object_x");
				auto by = slice.get_int32("object_y");
				auto bw = slice.get_int32("object_w");
				auto bh = slice.get_int32("object_h");

				rectangle image_rect;

				image_name = std::format("{} #{}", "image", i);
				bx = 0;
				by = 0;
				bw = 1000;
				bh = 1000;

#if _DETAIL
				std::cout << "before:" << image_name << std::endl;
				std::cout << image_rect->w << " " << image_rect->h << " " << image_rect->x << " " << image_rect->y << std::endl;
#endif

			}

			int scount = 0;

			for (auto slice : sprites)
			{
				auto image_name = slice.item.get_string(0);
				auto bx = slice.item.get_int32("object_x");
				auto by = slice.item.get_int32("object_y");
				auto bw = slice.item.get_int32("object_w");
				auto bh = slice.item.get_int32("object_h");

#if _DETAIL
				std::cout << image_name << std::endl;
				std::cout << image_rect->w << " " << image_rect->h << " " << image_rect->x << " " << image_rect->y << std::endl;
#endif

				if (bw != 1000 or bh != 1000) {

					std::cout << __LINE__ << ":array failed" << std::endl;
					return false;
				}

			}

			return true;
		}
		catch (std::exception& exc)
		{
			system_monitoring_interface::active_mon->log_exception(exc);
			return false;
		}
	}

}

