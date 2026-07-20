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
        json            cooked_data;

		void set(json& response)
		{
			success = response[success_field].as_bool();
			message = response[message_field].as_string();
			execution_time = response[seconds_field].as_double();
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
 		virtual corona_client_response run_method(json _object) = 0;
		virtual corona_client_response delete_object(std::string _class_name, int64_t _object_id) = 0;
		virtual corona_client_response query_objects(json _query) = 0;
		virtual corona_client_response copy_objects(json _copy) = 0;
	};
	
	class chest_item
	{
	public:

		object_reference	  reference;
		std::string			  item_type;
		double				  quantity;

		virtual void get_json(json& _dest)
		{
            std::string temp = reference;
			_dest.put_member("reference", temp);
			_dest.put_member("item_type", item_type);
			_dest.put_member_double("quantity", quantity);
		}

		virtual void put_json(json& _src)
		{
            std::string temp = _src["reference"].as_string();
			reference = temp;
			quantity = _src["quantity"].as_double();
            item_type = _src["item_type"].as_string();
		}

		bool operator == (const chest_item& _other) const
		{
			return reference.class_name == _other.reference.class_name and reference.object_id == _other.reference.object_id;
        }

		bool operator < (const chest_item& _other) const
		{
			if (reference.class_name < _other.reference.class_name) return true;
			if (reference.class_name > _other.reference.class_name) return false;
			return reference.object_id < _other.reference.object_id;
        }
	};

	class corona_object_interface
	{
	public:

		std::string  class_name;
		int64_t      object_id;

		std::string  created_by;
		date_time    created;

		std::string  updated_by;
		date_time    updated;

		corona_object_interface() = default;
		corona_object_interface(const corona_object_interface& _src) = default;
		corona_object_interface(corona_object_interface&& _src) = default;
		corona_object_interface& operator =(const corona_object_interface& _src) = default;
		corona_object_interface& operator =(corona_object_interface&& _src) = default;

		virtual void get_json(json& _dest) const = 0;
		virtual void put_json(json& _src) = 0;
		virtual std::string get_item_type() const = 0;
		virtual object_reference to_reference() const = 0;
		virtual chest_item to_chest_item(int _quantity) const = 0;
		virtual std::shared_ptr<corona_object_interface> clone() const = 0;

	};


	class selection_field_options_interface
	{
	public:
	};

	class audio_field_options_interface
	{
	public:
	};

	class chest_field_options_interface
	{
	public:
		std::string allowed_class_name;
		std::string allowed_item_type;
		std::string unit_name;
		int			max_items;
	};

	namespace game 
	{

		class game_interface
		{
		public:

			virtual std::shared_ptr<corona_object_interface> get_piece(object_reference& _reference, bool include_children) = 0;
			virtual void set_lobby() = 0;
			virtual void set_active() = 0;
			virtual void set_paused() = 0;
			virtual void set_complete() = 0;
			virtual void set_exit() = 0;
			virtual void start_play(std::string input_name) = 0;
			virtual void get_json(json& _dest) = 0;
			virtual void put_json(json& _src) = 0;
			virtual job* get_next_job() = 0;
		};

		class engine_interface
		{
		public:

			virtual std::shared_ptr<game_interface> new_game(json _game_key) = 0;
			virtual std::shared_ptr<game_interface> load_game(json _session_key) = 0;
			virtual void save_game(std::shared_ptr<game_interface> _session) = 0;
			virtual void close_game(std::shared_ptr<game_interface> _session) = 0;
		};

		class engine_factory
		{
			public:

            static std::shared_ptr<engine_interface> create_engine(system_monitoring_interface* _bus);
		};

	};

}
