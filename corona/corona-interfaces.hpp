#pragma once


namespace corona
{
	class corona_bus_command : public json_serializable
	{
	public:

	};

	class corona_server_bus_command : public corona_bus_command
	{
	public:

	};

    class corona_desktop_bus_command : public corona_bus_command
	{
	public:
		json data;
		json parameters;

		corona_desktop_bus_command()
		{
			json_parser jp;
			parameters = jp.create_object();
		}

		virtual json execute(int _batch_id, comm_desktop_bus_interface* _bus) = 0;
		virtual json execute_sync(int _batch_id, comm_desktop_bus_interface* _bus) = 0;

		virtual void set_parameter(std::string _name, std::string _value)
		{
			parameters.put_member_string(_name, _value);
		}

		virtual void get_json(json& _dest)
		{}

		virtual void put_json(json& _src)
		{}

		virtual std::string get_name()
		{
			return std::string("command");
		}
	};

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
		virtual corona_client_response alter_class(json _class_definition) = 0;
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

		virtual void get_json(json& _dest) const
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
			return reference == _other.reference;
        }

		bool operator < (const chest_item& _other) const
		{
			return reference < _other.reference;
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
		virtual void get_json(json& _dest) const = 0;
		virtual void put_json(json& _src) = 0;
	};

	class audio_field_options_interface
	{
	public:
		virtual void get_json(json& _dest) const = 0;
		virtual void put_json(json& _src) = 0;
	};

	class chest_field_options_interface
	{
	public:
		std::string allowed_class_name;
		std::string allowed_item_type;
		std::string unit_name;
		int			max_items;

		virtual void get_json(json& _dest) const = 0;
		virtual void put_json(json& _src) = 0;
	};


	class text_template_base {
	public:
		virtual void set_text_template(std::string _template) = 0;
	};

	namespace workflow
	{

		class symbol
		{
		public:
			std::string			  item_class;
			std::string			  item_type;
			double				  quantity;

			symbol() = default;
			symbol(const symbol& _src) = default;
			symbol(symbol&& _src) = default;
			symbol& operator = (const symbol& _src) = default;
			symbol& operator = (symbol&& _src) = default;

			bool compare(const chest_item& ct) const
			{
				return (ct.reference.class_name == item_class && ct.item_type == item_type && ct.quantity >= quantity);
			}

			void get_json(json& _dest) const
			{
				json_parser jp;
				_dest = jp.create_object();
				_dest.put_member("item_class", item_class);
				_dest.put_member("item_type", item_type);
				_dest.put_member("quantity", quantity);
			}

			void put_json(json& _src)
			{
				if (_src.object()) {
					item_class = _src.get_member("item_class").as_string();
					item_type = _src.get_member("item_type").as_string();
					quantity = _src.get_member("quantity").as_double();
				}
			}
		};

		class production
		{
		public:
			std::string							rule_name;
			// terminals and non-terminals that must be present in the inventory to execute this command,
			// that is, if the inventory fits the grammar then the command can be produced.
			std::vector<symbol>					required_symbols;
			std::vector<symbol>					produced_symbols; // the symbol that is produced by this command	
			std::shared_ptr<corona_desktop_bus_command> command; // the command that is executed

			production() = default;
			production(const production& _src) = default;
			production(production&& _src) = default;
			production& operator = (const production& _src) = default;
			production& operator = (production&& _src) = default;

			bool compare(const chest_field& cf) const
			{
				for (auto& ri : required_symbols) {
					bool found = false;
					for (auto& item : cf.items) {
						if (ri.compare(item)) {
							found = true;
							break;
						}
					}
					if (!found) return false;
				}
				return true;
			}

			bool apply(chest_field& cf)
			{
				bool r = false;
				if (r = compare(cf)) {
					for (auto& ri : required_symbols) {
						chest_item ci;
						ci.item_type = ri.item_type;
						ci.reference.class_name = ri.item_class;
						ci.quantity = ri.quantity;
						cf.remove_part(ci);
					}
				}
				return r;
			}

			void get_json(json& _dest) const;
			void put_json(json& _src);

		};

		class workflow_field_options_interface {
		public:
			std::shared_ptr<chest_field_options_interface> chest_options;
			std::shared_ptr<selection_field_options_interface> selection_options;
			std::vector<std::shared_ptr<workflow::production>>	language;

			virtual void get_json(json& _dest) const = 0;
			virtual void put_json(json& _src) = 0;
		};

	}


	namespace game 
	{

		class adventure_interface
		{
		public:

			virtual std::shared_ptr<corona_object_interface> get_piece(object_reference& _reference, bool include_children) = 0;
			virtual void set_lobby() = 0;
			virtual void set_active() = 0;
			virtual void set_paused() = 0;
			virtual void set_complete() = 0;
			virtual void set_exit() = 0;
			virtual void get_json(json& _dest) = 0;
			virtual void put_json(json& _src) = 0;
			virtual job* get_next_job() = 0;
		};

		class engine_interface
		{
		public:

			virtual std::shared_ptr<adventure_interface> new_adventure(json _game_key) = 0;
			virtual std::shared_ptr<adventure_interface> load_adventure(json _session_key) = 0;
			virtual void save_adventure(std::shared_ptr<adventure_interface> _session) = 0;
			virtual void close_adventure(std::shared_ptr<adventure_interface> _session) = 0;
		};

		class engine_factory
		{
			public:

            static std::shared_ptr<engine_interface> create_engine(system_monitoring_interface* _bus);
		};

	};

}
