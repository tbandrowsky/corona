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
		DirectX::XMVECTOR size = {};
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
			_dest.put_member("position", position);
			_dest.put_member("acceleration", acceleration);
			_dest.put_member("velocity", velocity);
			_dest.put_member("size", size);

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
			position = _src["position"].as_vector();
			acceleration = _src["acceleration"].as_vector();
			velocity = _src["velocity"].as_vector();
			size = _src["size"].as_vector();

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
		double mass;
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
            mass = _src["mass"].as_double();

			if (fabs(mass) < 0.0001) {
				mass = 1.0;
            }

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
			using namespace DirectX;

			// can't be accelerated or accelerate anything if it's not moving or accelerating.
			// no kinetic energy
			if (XMVector3Equal(_piece->acceleration, zero_vector) &&
				XMVector3Equal(_piece->velocity, zero_vector)) {
				return false;
			}

			bool caused_acceleration = false;

			// Apply acceleration to velocity (velocity += acceleration * delta_time)
			// Note: delta_time should be passed in, but using a small timestep for now
			XMVECTOR velocity_change = XMVectorScale(_piece->acceleration, static_cast<float>(last_elapsed_seconds));
			_piece->velocity = XMVectorAdd(_piece->velocity, velocity_change);

			// Calculate predicted position for this frame
			XMVECTOR displacement = XMVectorScale(_piece->velocity, static_cast<float>(last_elapsed_seconds));
			XMVECTOR predicted_position = XMVectorAdd(_piece->position, displacement);

			// Get current piece bounds
			rectangle piece_rect = rectangle_math::from_vector(predicted_position, _piece->size);

			// Assume mass is proportional to area (width * height)
			// You may want to add an explicit mass field to game_piece
			float piece_mass = XMVectorGetX(_piece->size) * XMVectorGetY(_piece->size);
			if (piece_mass < 1.0f) piece_mass = 1.0f; // Minimum mass

			// Check collision with all other pieces
			for (auto& other : _map->pieces)
			{
				if (other == _piece) continue;

				// Check if other piece is a wall or solid object
				bool other_is_solid = false;
				bool other_is_passable = true;

				if (auto wall = std::dynamic_pointer_cast<game_wall>(other)) {
					other_is_solid = true;
					other_is_passable = wall->passable;
				}
				else if (auto door = std::dynamic_pointer_cast<game_door>(other)) {
					other_is_solid = true;
					other_is_passable = door->open;
				}

				if (other_is_solid && other_is_passable) {
					continue; // Skip passable objects
				}

				// Get other piece bounds
				rectangle other_rect = rectangle_math::from_vector(other->position, other->size);

				// Check for collision (AABB intersection)
				if (piece_rect.intersects(other_rect))
				{
					caused_acceleration = true;

					// Calculate collision normal (direction from other to piece)
					XMVECTOR collision_vector = XMVectorSubtract(_piece->position, other->position);
					float length = XMVectorGetX(XMVector3Length(collision_vector));
					XMVECTOR collision_normal = length > 0.001f ?
						XMVectorScale(collision_vector, 1.0f / length) :
						XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);

					// Calculate relative velocity
					XMVECTOR relative_velocity = XMVectorSubtract(_piece->velocity, other->velocity);
					float velocity_along_normal = XMVectorGetX(XMVector3Dot(relative_velocity, collision_normal));

					// Don't resolve if velocities are separating
					if (velocity_along_normal > 0)
						continue;

					// Calculate other piece mass
					float other_mass = XMVectorGetX(other->size) * XMVectorGetY(other->size);
					if (other_mass < 1.0f) other_mass = 1.0f;

					// If other is solid and immovable (wall), treat as infinite mass
					if (other_is_solid && !other_is_passable) {
						other_mass = piece_mass * 1000.0f; // Effectively infinite
					}

					// Calculate restitution (bounciness) - 0 = inelastic, 1 = perfectly elastic
					float restitution = 0.5f; // Semi-elastic collision

					// Calculate impulse scalar
					float impulse_scalar = -(1.0f + restitution) * velocity_along_normal;
					impulse_scalar /= (1.0f / piece_mass + 1.0f / other_mass);

					// Apply impulse to both objects
					XMVECTOR impulse = XMVectorScale(collision_normal, impulse_scalar);

					// Update velocities based on impulse
					_piece->velocity = XMVectorAdd(_piece->velocity,
						XMVectorScale(impulse, 1.0f / piece_mass));

					if (!other_is_solid || other_is_passable) {
						// Only update velocity of movable objects
						other->velocity = XMVectorSubtract(other->velocity,
							XMVectorScale(impulse, 1.0f / other_mass));

						// Apply acceleration to the other piece
						other->acceleration = XMVectorAdd(other->acceleration,
							XMVectorScale(impulse, 0.1f / other_mass)); // Scaled for game feel
					}

					// Separate the objects to prevent overlap
					float penetration_depth =
						(XMVectorGetX(_piece->size) + XMVectorGetX(other->size)) * 0.5f - length;

					if (penetration_depth > 0) {
						float separation_ratio = piece_mass / (piece_mass + other_mass);
						XMVECTOR separation = XMVectorScale(collision_normal, penetration_depth);

						_piece->position = XMVectorAdd(_piece->position,
							XMVectorScale(separation, separation_ratio));

						if (!other_is_solid || other_is_passable) {
							other->position = XMVectorSubtract(other->position,
								XMVectorScale(separation, 1.0f - separation_ratio));
						}
					}

					// Apply friction/damping to simulate energy loss
					float friction_coefficient = 0.98f;
					_piece->velocity = XMVectorScale(_piece->velocity, friction_coefficient);

					if (!other_is_solid || other_is_passable) {
						other->velocity = XMVectorScale(other->velocity, friction_coefficient);
					}
				}
			}

			// Update position based on final velocity (if no collision blocked it)
			if (!caused_acceleration) {
				_piece->position = predicted_position;
			}

			// Apply friction when no collision (air resistance, surface friction, etc.)
			float ambient_friction = 0.99f;
			_piece->velocity = XMVectorScale(_piece->velocity, ambient_friction);
			_piece->acceleration = XMVectorScale(_piece->acceleration, 0.95f); // Decay acceleration over time

			return caused_acceleration;
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
