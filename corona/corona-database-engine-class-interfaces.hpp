#pragma once

namespace corona {

	class class_interface : public shared_lockable, public class_interface_base
	{
	protected:
	public:

		virtual bool is_server_only(const std::string& _field_name) = 0;
		virtual void get_json(json& _dest) = 0;
		virtual void put_json(validation_error_collection& _errors, json& _src) = 0;

		virtual bool ready() {
			return false;
		}

		virtual std::string								get_class_name()  const = 0;
		virtual std::string								get_class_description()  const = 0;
		virtual std::string								get_base_class_name()  const = 0;
		virtual std::string                             get_class_display() const = 0;
		virtual std::string                             get_class_author() const = 0;
		virtual std::string                             get_class_version() const = 0;
		virtual std::map<std::string, bool>  const& get_descendants()  const = 0;
		virtual std::map<std::string, bool>  const& get_ancestors()  const = 0;
		virtual std::map<std::string, bool>& update_descendants() = 0;
		virtual std::map<std::string, bool>& update_ancestors() = 0;
		virtual std::vector<std::string>				get_parents() const = 0;
		virtual std::vector<std::string>				get_full_text_fields() const = 0;
		virtual std::vector<std::string>				get_field_display_order() const = 0;

		virtual json									get_parent_objects(std::string _user_name, json& _reference_object, corona_database_interface* _db) const = 0;

		// creates whatever table representation
		virtual std::shared_ptr<xtable_interface>		create_table(corona_database_interface* _db) = 0;
		virtual std::shared_ptr<xtable_interface>		get_table(corona_database_interface* _db) = 0;

		// creates a sql server table with an index table backing it.
		virtual std::shared_ptr<sql_table>				create_stable(corona_database_interface* _db) = 0;
		virtual std::shared_ptr<sql_table>				get_stable(corona_database_interface* _db) = 0;

		// creates a full_text_index 
		virtual std::shared_ptr<xtable>					create_full_text_index(corona_database_interface* _db) = 0;
		virtual std::shared_ptr<xtable>					get_full_text_index(corona_database_interface* _db) = 0;

		// creates a no-sql table with multiple optional indexes
		virtual std::shared_ptr<xtable>					create_xtable(corona_database_interface* _db) = 0;
		virtual std::shared_ptr<xtable>					get_xtable(corona_database_interface* _db) = 0;

		virtual std::shared_ptr<xtable>					alter_xtable(corona_database_interface* _db) = 0;
		virtual std::shared_ptr<xtable>					find_index(corona_database_interface* _db, json& _keys) const = 0;
		virtual	bool									open(activity* _context, json _existing_definition, int64_t _location) = 0;
		virtual	bool									update(activity* _context, json _changed_class) = 0;
		virtual	bool									create(activity* _context, json _changed_class) = 0;

		virtual	void									put_field(std::shared_ptr<field_interface>& _name) = 0;
		virtual std::shared_ptr<field_interface>		get_field(const std::string& _name)  const = 0;
		virtual std::shared_ptr<field_interface>		get_field_for_reference(class_interface* _class)  const = 0;
		virtual audio_function							get_audio(json _src, const std::string& _name) = 0;
		virtual std::shared_ptr<chest_field>		    get_chest(json _src, const std::string& _name) = 0;
		virtual void									put_chest(json& _dest, const std::string& _name, std::shared_ptr<chest_field>& _src) = 0;
		virtual void									put_chest(json& _dest, const std::string& _name, chest_field& _src) = 0;
		virtual std::shared_ptr<selection_field>		get_selection(json _src, const std::string& _name) = 0;
		virtual void									put_selection(json& _dest, const std::string& _name, selection_field& _src) = 0;
		virtual void									put_selection(json& _dest, const std::string& _name, std::shared_ptr<selection_field>& _src) = 0;
		virtual std::shared_ptr<pathDto>				get_path(json _src, const std::string& _name) = 0;
		virtual void									put_path(json& _dest, const std::string& _name, pathDto& _src) = 0;
		virtual void									put_path(json& _dest, const std::string& _name, std::shared_ptr<pathDto>& _src) = 0;
		virtual std::shared_ptr<generalBrushRequest>	get_brush(json _src, const std::string& _name) = 0;
		virtual void									put_brush(json& _dest, const std::string& _name, generalBrushRequest& _src) = 0;
		virtual void									put_brush(json& _dest, const std::string& _name, std::shared_ptr<generalBrushRequest>& _src) = 0;
		virtual std::shared_ptr<bitmapInstanceDto>		get_bitmap(json _src, const std::string& _name) = 0;
		virtual void									put_bitmap(json& _dest, const std::string& _name, bitmapInstanceDto& _src) = 0;
		virtual void									put_bitmap(json& _dest, const std::string& _name, std::shared_ptr<bitmapInstanceDto>& _src) = 0;
		virtual std::map<std::string, std::shared_ptr<field_interface>>& use_fields() = 0;
		virtual std::vector<std::shared_ptr<field_interface>> get_fields()  const = 0;
		virtual std::vector<std::shared_ptr<field_section>> get_fields_for_ux(corona_database_interface* _db)  const = 0;

		virtual std::shared_ptr<index_interface>		get_index(const std::string& _name)  const = 0;
		virtual std::vector<std::shared_ptr<index_interface>> get_indexes()  const = 0;

		virtual void	init_validation(corona_database_interface* _db, class_permissions _permissions) = 0;

		virtual json	get_object(corona_database_interface* _db, int64_t _object_id, class_permissions _grant, bool& _exists) = 0;
		virtual json	put_objects(corona_database_interface* _db, json& _children, json& _src_objects, class_permissions _grant, validation_error_collection& validation_errors, int64_t& success_object_count, int64_t& fail_object_count) = 0;
		virtual json	get_objects(corona_database_interface* _db, json _key, bool _include_children, class_permissions _grant) = 0;
		virtual json	delete_objects(corona_database_interface* _db, json _key, bool _include_children, class_permissions _grant) = 0;
		virtual json    get_single_object(corona_database_interface* _db, json _key, bool _include_children, class_permissions _grant) = 0;
		virtual json    run_method(corona_database_interface* _db, std::string& _method_name, std::string& _token, std::string& _class_name, json& _target) = 0;
		virtual json	run_queries(corona_database_interface* _db, std::string& _token, std::string& _class_name, json& _target) = 0;

		virtual json	get_info(corona_database_interface* _db) = 0;
		virtual json	get_openapi_schema(corona_database_interface* _db) = 0;

		virtual bool	any(corona_database_interface* _db) = 0;
		virtual bool	any_descendants(corona_database_interface* _db) = 0;

		virtual int64_t get_next_object_id(corona_database_interface* _db) = 0;

		virtual int64_t get_object_count() = 0;
		virtual int64_t get_data_count() = 0;
	};

	using read_class_sp = read_locked_sp<class_interface>;
	using write_class_sp = write_locked_sp<class_interface>;

	class activity
	{
	public:
		corona_database_interface* db;

		std::map<std::string, write_class_sp> classes;
		validation_error_collection errors;

		class_interface* get_class(std::string _class_name);
		class_interface* create_class(std::string _class_name);
	};

	class corona_simulation_interface
	{
	public:

		virtual void on_frame(json& _commands) = 0;
	};

	class corona_database_interface
	{
	protected:

	public:

		corona_connections connections;
		std::string onboarding_email;
		std::string recovery_email;
		std::string schema_file_name;

		corona_database_interface()
		{

		}

		virtual json create_database() = 0;
		virtual relative_ptr_type open_database() = 0;

		virtual void apply_config(json _system_config) = 0;
		virtual void apply_config(json _system_config, json _server_config) = 0;
		virtual json apply_schema(json _schema) = 0;
		virtual json create_application(json _application_schema) = 0;
		virtual json update_application(json _application_schema, std::string _class_name) = 0;

		virtual std::string get_random_code(int _confirmation_code_digits = 8) = 0;

		virtual class_permissions get_class_permission(
			std::string _user_name,
			std::string _class_name) = 0;

		virtual void log_errors(validation_error_collection& _errors) = 0;

		virtual std::string get_directory() = 0;

		virtual std::shared_ptr<class_interface> read_get_class(const std::string& _class_name) = 0;
		virtual std::shared_ptr<class_interface> put_class_impl(activity* _activity, json& _class_definition) = 0;
		virtual std::shared_ptr<class_interface> get_class_impl(activity* _activity, std::string _class_name) = 0;

		virtual std::vector<std::string> get_class_descendants(const std::string& _class_name) = 0;

		virtual json query_class(std::string _user_name, json query_details) = 0;
		virtual void scrub_object(json& object_to_scrub) = 0;
		virtual json create_user(json create_user_request, bool _trusted_user, bool _system_user) = 0;
		virtual json login_user(json _login_request) = 0;
		virtual json login_user_sso(json _create_user_request) = 0;
		virtual json login_user_local(json _create_user_request) = 0;
		virtual json user_confirm_code(json _login_request) = 0;
		virtual json user_send_code(json _send_user_request) = 0;
		virtual json set_user_password(json _set_password_request) = 0;
		virtual json get_classes(json get_classes_request) = 0;
		virtual json get_class(json get_class_request) = 0;
		virtual json put_class(json put_class_request) = 0;
		virtual json alter_class(json alter_class_request) = 0;
		virtual json user_home(json user_home_request) = 0;
		virtual json user_set_team(json user_home_request) = 0;
		virtual json object_to_class(std::string _class_name) = 0;
		virtual json class_to_object(std::string _class_name) = 0;

		virtual json edit_object(json _edit_object_request) = 0;
		virtual json run_object(json _edit_object_request) = 0;
		virtual json run_method(json _run_method_request) = 0;
		virtual json create_object(json create_object_request) = 0;
		virtual json put_object(json put_object_request) = 0;
		virtual json get_object(json get_object_request) = 0;
		virtual json delete_object(json delete_object_request) = 0;
		virtual json query(json query_request) = 0;

		virtual json add_item_chest(json add_to_chest_request) = 0;
		virtual json remove_item_chest(json remove_from_chest_request) = 0;
		virtual json move_item_chest(json move_chest_request) = 0;

		virtual json select_object(std::string _class_name, int64_t _object_id, class_permissions _permissions) = 0;
		virtual json select_object(json _key, bool _include_children, class_permissions _permissions) = 0;
		virtual json select_single_object(json _key, bool _include_children, class_permissions _permissions) = 0;
		virtual read_class_sp read_lock_class(const std::string& _class_name) = 0;
		virtual write_class_sp write_lock_class(const std::string& _class_name) = 0;
		virtual json save_class(class_interface* _class_to_save) = 0;
		virtual bool check_message(json& _message, std::vector<std::string> _authorizations, std::string& _user_name, std::string& _token_authority) = 0;
		virtual json get_openapi_schema(std::string _user_name) = 0;

	};


}