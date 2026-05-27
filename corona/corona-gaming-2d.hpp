#pragma once

namespace corona
{

	class game_sprite
	{
	public:
		std::string class_name;
		int64_t		object_id;
		rectangle   source_rectangle;
		std::string state;
		double		order;
		double		duration;

		std::shared_ptr<image_control> source_image;

		virtual void get_json(json& _dest)
		{
			json_parser jp;

			_dest.put_member("state", state);
			_dest.put_member("order", order);
			_dest.put_member("duration", duration);
			_dest.put_member("class_name", class_name);
            _dest.put_member("object_id", object_id);
			corona::get_json(_dest, source_rectangle);
		}

		virtual void put_json(json& _src)
		{
			state = _src["state"].as_string();
			order = _src["order"].as_double();
			duration = _src["duration"].as_double();
            class_name = _src["class_name"].as_string();
            object_id = _src["object_id"].as_int64_t();
			corona::put_json(source_rectangle, _src);
		}
	};

	class game_piece
	{
	public:
		std::string class_name;
		int64_t		object_id;
		std::string name;
		std::string image_name;
		std::string state;
		std::vector<std::shared_ptr<game_sprite>> sprites;
		std::shared_ptr<chest_field> inventory;

		DirectX::XMVECTOR position = {};
		DirectX::XMVECTOR velocity = {};
		DirectX::XMVECTOR acceleration = {};

		virtual void get_json(json& _dest)
		{
			json_parser jp;

            _dest.put_member("class_name", class_name);
            _dest.put_member_i64("object_id", object_id);
			_dest.put_member("image_name", image_name);
			_dest.put_member("state", state);
			_dest.put_member("name", name);
			_dest.put_member("dx", position.m128_f32[0]);
			_dest.put_member("dy", position.m128_f32[1]);
			_dest.put_member("dz", position.m128_f32[2]);
			_dest.put_member("ax", acceleration.m128_f32[0]);
			_dest.put_member("ay", acceleration.m128_f32[1]);
			_dest.put_member("az", acceleration.m128_f32[2]);
			_dest.put_member("cx", velocity.m128_f32[0]);
			_dest.put_member("cy", velocity.m128_f32[1]);
			_dest.put_member("cz", velocity.m128_f32[2]);

			json j = jp.create_array();
			for (auto& s : sprites) {
				json jsprite = jp.create_object();
				s->get_json(jsprite);
				j.push_back(jsprite);
			}
			_dest.put_member("sprites", j);

			json jinventory = jp.create_object();

			if (inventory) {
				inventory->get_json(jinventory);
			}
		}

		virtual void put_json(json& _src)
		{
			class_name = _src["class_name"].as_string();
            object_id = _src["object_id"].as_int64_t();
			image_name = _src["image_name"].as_string();
			state = _src["state"].as_string();
			name = _src["name"].as_string();
			position.m128_f32[0] = _src["dx"].as_double();
			position.m128_f32[1] = _src["dy"].as_double();
			position.m128_f32[2] = _src["dz"].as_double();
			acceleration.m128_f32[0] = _src["ax"].as_double();
			acceleration.m128_f32[1] = _src["ay"].as_double();
			acceleration.m128_f32[2] = _src["az"].as_double();
			velocity.m128_f32[0] = _src["cx"].as_double();
			velocity.m128_f32[1] = _src["cy"].as_double();
			velocity.m128_f32[2] = _src["cz"].as_double();

			json j = _src["sprites"];
			json aj = j.as_array();

			for (int i = 0; i < aj.size(); i++) {
				auto sprite = std::make_shared<game_sprite>();
				json aji = aj.get_element(i);
				if (aji.object()) {
					sprite->put_json(aji);
					sprites.push_back(sprite);
				}
			}

			inventory = std::make_shared<chest_field>();
			if (_src.has_member("inventory")) {
				json jinventory = _src["inventory"];
				inventory->put_json(jinventory);
			}
		}
	};

	class game_item : public game_piece
	{
	public:
		virtual void get_json(json& _dest)
		{
			game_piece::get_json(_dest);
            // add item specific properties here
		}

		virtual void put_json(json& _src)
		{	
			game_piece::put_json(_src);
            // add item specific properties here
		}
	};

	class game_player : public game_piece
	{
		std::string input_device;

	public:

		virtual void get_json(json& _dest)
		{
			game_piece::get_json(_dest);
			_dest.put_member("input_device", input_device);
		}

		virtual void put_json(json& _src)
		{
			game_piece::put_json(_src);
			input_device = _src["input_device"].as_string();
		}
	};

	class game_lootbox : public game_piece
	{
	public:
		virtual void get_json(json& _dest)
		{
			game_piece::get_json(_dest);
		}

		virtual void put_json(json& _src)
		{
			game_piece::put_json(_src);
		}
	};

	class game_npc : public game_piece
	{
	public:
		virtual void get_json(json& _dest)
		{
			game_piece::get_json(_dest);
		}

		virtual void put_json(json& _src)
		{
			game_piece::put_json(_src);
		}
	};

	class game_wall : public game_piece
	{
	public:

		bool passable;

		virtual void get_json(json& _dest)
		{
			game_piece::get_json(_dest);
			_dest.put_member("passable", passable);
		}

		virtual void put_json(json& _src)
		{
			game_piece::put_json(_src);
			passable = _src["passable"].as_bool();
		}
	};

	class game_door : public game_piece
	{
	public:

		bool open;

		virtual void get_json(json& _dest)
		{
			_dest.put_member_bool("open", open);
		}

		virtual void put_json(json& _src)
		{
			game_piece::put_json(_src);
			open = _src["open"].as_bool();
		}
	};

	class game_surface : public game_piece
	{
	public:
		std::string mechanic;
		double acceleration_multiplier;
		double friction_multiplier;

		virtual void get_json(json& _dest)
		{
			_dest.put_member("mechanic", mechanic);
			_dest.put_member("acceleration_multiplier", acceleration_multiplier);
			_dest.put_member("friction_multiplier", friction_multiplier);
		}

		virtual void put_json(json& _src)
		{
			mechanic = _src["mechanic"].as_string();
			acceleration_multiplier = _src["acceleration_multiplier"].as_double();
			friction_multiplier = _src["friction_multiplier"].as_double();
		}
	};

	class game_decoration : public game_piece
	{
	public:
		virtual void get_json(json& _dest)
		{
		}

		virtual void put_json(json& _src)
		{
		}
	};

	class game_map
	{
	public:
		std::string class_name;
		int64_t object_id;
		std::string name;
		std::vector<std::shared_ptr<game_piece>> pieces;

		virtual void get_json(json& _dest)
		{
			json_parser jp;

			_dest.put_member("pieces", name);

			json j = jp.create_array();
			for (auto& s : pieces) {
				json jitem = jp.create_object();
				s->get_json(jitem);
				j.push_back(jitem);
			}
			_dest.put_member("pieces", j);
		}

		virtual void put_json(json& _src)
		{

			name = _src["name"].as_string();

			json j = _src["pieces"];
			json aj = j.as_array();

			for (int i = 0; i < aj.size(); i++) {
				json aji = aj.get_element(i);
				if (aji.object()) {
					std::string class_name = aji["class_name"].as_string();

					if (class_name == "piece")
					{
						auto piece = std::make_shared<game_piece>();
						piece->put_json(aji);
						pieces.push_back(piece);
					}
					else if (class_name == "item")
					{
						auto item = std::make_shared<game_item>();
						item->put_json(aji);
						pieces.push_back(item);
					}
					else if (class_name == "player")
					{
						auto player = std::make_shared<game_player>();
						player->put_json(aji);
						pieces.push_back(player);
					}
					else if (class_name == "lootbox")
					{
						auto lootbox = std::make_shared<game_lootbox>();
						lootbox->put_json(aji);
						pieces.push_back(lootbox);
					}
					else if (class_name == "npc")
					{
						auto npc = std::make_shared<game_npc>();
						npc->put_json(aji);
						pieces.push_back(npc);
					}
					else if (class_name == "wall")
					{
						auto wall = std::make_shared<game_wall>();
						wall->put_json(aji);
						pieces.push_back(wall);
					}
					else if (class_name == "door")
					{
						auto door = std::make_shared<game_door>();
						door->put_json(aji);
						pieces.push_back(door);
					}
					else if (class_name == "surface")
					{
						auto surface = std::make_shared<game_surface>();
						surface->put_json(aji);
						pieces.push_back(surface);
					}
					else if (class_name == "decoration")
					{
						auto decoration = std::make_shared<game_decoration>();
						decoration->put_json(aji);
						pieces.push_back(decoration);
					}
				}
			}
		}

	};

	class game_session : public job
	{
	public:
		std::string class_name;
		int64_t object_id;
		std::string name;
        std::string description;
		std::string image;
		std::string current_map;
		timer		frame_timer;
        xinput		input;

		bool game_launched = true;
		bool game_running = false;
		bool game_over = false;
		bool game_complete = false;

		double last_elapsed_seconds;

		std::vector<std::shared_ptr<game_map>> maps;
		DirectX::XMVECTOR zero_vector = {};

		game_session()
		{
			;
		}

		game_session(json& _src)
		{
			put_json(_src);
		}

		virtual void get_json(json& _dest)
		{
			json_parser jp;
			_dest.put_member("name", name);
			_dest.put_member("description", description);
			_dest.put_member("image", image);
			_dest.put_member("current_map", current_map);
			json j = jp.create_array();
			for (auto& s : maps) {
				json jmap = jp.create_object();
				s->get_json(jmap);
				j.push_back(jmap);
			}
			_dest.put_member("maps", j);
		}

		virtual void put_json(json& _src)
		{
			name = _src["name"].as_string();
			description = _src["description"].as_string();
			image = _src["image"].as_string();
			current_map = _src["current_map"].as_string();
			json j = _src["maps"];
			json aj = j.as_array();
			for (int i = 0; i < aj.size(); i++) {
				json aji = aj.get_element(i);
				if (aji.object()) {
					auto map = std::make_shared<game_map>();
					map->put_json(aji);
					maps.push_back(map);
				}
			}
        }

		job* get_next_job()
		{
			if (!game_over) {
				return this;
			}
			else {
				return nullptr;
			}
		}

		/// <summary>
		/// applies the acceleration of a piece,
		/// returning true, if, the piece accelerated something else.
		/// think of this as like a packet
		/// </summary>
		/// <param name="_piece"></param>
		/// <returns></returns>
		bool accelerate_piece(std::shared_ptr<game_map> _map, std::shared_ptr<game_piece> _piece)
		{
            // can't be accelerated or accelerate anything if it's not moving or accelerating.
			// no kinetic energy
			if (DirectX::XMVector3Equal(_piece->acceleration, zero_vector) &&
				DirectX::XMVector3Equal(_piece->velocity, zero_vector)) {
				return false;
            }

            DirectX::XMVECTOR new_position = DirectX::XMVectorAdd(_piece->position, _piece->velocity);

            // now do we hit something as we are moving
			for (auto p : _map->pieces) 
			{
				if (p != _piece) {
					
				}
			}
		}

		virtual job_notify execute(job_queue* _callingQueue, DWORD _bytesTransferred, BOOL _success)
		{
            job_notify notify;

			notify.shouldDelete = false;

			if (game_launched) {
                last_elapsed_seconds = frame_timer.get_elapsed_seconds();
				return;
			}

            double current_elapsed_seconds = frame_timer.get_elapsed_seconds();
			double delta = current_elapsed_seconds - last_elapsed_seconds;

			// we have to resolve collision effects first, because, 
			// that gives us new accelerations.

			// in the model, physical quantities are given in seconds, so, we can apply the ax, ay to terms
			// and that gives us simple linear models.
			// for other models, we can additionally scale the time so that seconds could be weeks,
			// months or years.

            for (auto gm : maps) {
			}

			return notify;
		}

	};

	class gaming_engine
	{
		std::shared_ptr<comm_bus_app_interface> bus;
		corona_instance instance = corona_instance::local;

	public:

        gaming_engine(std::shared_ptr<comm_bus_app_interface> _db) : bus(_db)
		{
		}

		std::shared_ptr<game_session> new_session(json _game_key)
		{
			json_parser jp;

			// grab a copy of the mini_game,
			// and then copy it to a new session.

			json copy_plan = jp.create_object();

			json copy_from = jp.create_object();
            copy_from.copy_member("class_name", _game_key);
			copy_from.copy_member("object_id", _game_key);

			json copy_to = jp.create_object();
			copy_to.put_member_string("class_name", "session");

			json copy_transform = jp.create_object();
			copy_transform.put_member_string("class_name", "session");

			copy_plan.put_member("from", copy_from);
			copy_plan.put_member("to", copy_to);
			copy_plan.put_member("transform", copy_transform);

			auto ccr = bus->copy_object(instance, copy_plan);
			if (ccr.success) {
				json new_session = ccr.data;
                std::shared_ptr<game_session> session = std::make_shared<game_session>();
                session->put_json(new_session);
				return session;
			}
			return nullptr;
		}

		std::shared_ptr<game_session> load_session(json _session_key)
		{
			json_parser jp;
			auto result = bus->get_object(instance, _session_key);
			if (result.success) {
				std::shared_ptr<game_session> session = std::make_shared<game_session>();
				session->put_json(result.data);
				return session;
			}
			return nullptr;
		}

		void save_session(std::shared_ptr<game_session> _session)
		{
			json_parser jp;
			json jsession = jp.create_object();
			_session->get_json(jsession);
			bus->put_object(instance, jsession);
		}
	};

}
