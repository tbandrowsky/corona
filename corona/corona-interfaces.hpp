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

		std::string part_class;
		int64_t		part_id;
		double      quantity;
        json	    lot_data;

		virtual void get_json(json& _dest)
		{
			_dest.put_member_string("part_class", part_class);
			_dest.put_member_i64("part_id", part_id);
			_dest.put_member_double("quantity", quantity);
            _dest.put_member("lot_data", lot_data);
		}

		virtual void put_json(json& _src)
		{
			part_class = _src["part_class"].as_string();
			part_id = _src["part_id"].as_int64_t();
			quantity = _src["quantity"].as_double();
            lot_data = _src["lot_data"];
		}

	};

	namespace game 
	{

		class game_interface
		{
		public:
			virtual void set_lobby() = 0;
			virtual void set_active() = 0;
			virtual void set_paused() = 0;
			virtual void set_complete() = 0;
			virtual void set_exit() = 0;
			virtual void start_play(std::string input_name) = 0;
			virtual void check_all_ready() = 0;
			virtual void check_all_dead() = 0;
			virtual corona_client_response accelerate(std::string input_name, DirectX::XMVECTOR a) = 0;
			virtual corona_client_response displace(std::string input_name, DirectX::XMVECTOR d) = 0;
			virtual corona_client_response clear_selection(std::string input_name) = 0;
			virtual corona_client_response extend_selection(std::string input_name, chest_item* ci) = 0;
			virtual corona_client_response throw_selection(std::string input_name) = 0;
			virtual corona_client_response drop_selection(std::string input_name) = 0;
			virtual corona_client_response use_selection(std::string input_name) = 0;
			virtual corona_client_response select_next(std::string input_name) = 0;
			virtual corona_client_response select_previous(std::string input_name) = 0;
			virtual corona_client_response add_pieces(json _pieces) = 0;
			virtual corona_client_response remove_pieces(json _pieces) = 0;
			virtual corona_client_response purchase_pieces(std::string input_name, json _for_sale, json _price) = 0;
			virtual void get_json(json& _dest) = 0;
			virtual void put_json(json& _src) = 0;
			virtual job* get_next_job() = 0;
		};

		class engine_interface
		{
		public:

			std::shared_ptr<game_interface> new_game(json _game_key);
			std::shared_ptr<game_interface> load_game(json _session_key);
			void save_game(std::shared_ptr<game_interface> _session);
			void close_game(std::shared_ptr<game_interface> _session);
		};

		class engine_factory
		{
			public:

            static std::shared_ptr<engine_interface> create_engine(system_monitoring_interface* _bus);
		};

	};

}
