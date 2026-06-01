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
		double		mass;

		DirectX::XMVECTOR position = {};
		DirectX::XMVECTOR size = {};
		DirectX::XMVECTOR velocity = {};
		DirectX::XMVECTOR frame_velocity = {};
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
            _dest.put_member("mass", mass);

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
            frame_velocity = velocity;
			size = _src["size"].as_vector();
			mass = _src["mass"].as_double();

			if (fabs(mass) < 0.0001) {
				mass = 1.0;
			}

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

	class game_switch : public game_piece
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

	class game_shot : public game_piece
	{
		std::string originator;

	public:

		virtual void get_json(json& _dest)
		{
			game_piece::get_json(_dest);
			_dest.put_member("originator", originator);
		}

		virtual void put_json(json& _src)
		{
			game_piece::put_json(_src);
			originator = _src["originator"].as_string();
		}
	};

	class game_lootspot : public game_piece
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
					else if (class_name == "loot_spot")
					{
						auto loot_spot = std::make_shared<game_lootspot>();
						loot_spot->put_json(aji);
						pieces.push_back(loot_spot);
					}
					else if (class_name == "loot_box")
					{
						auto loot_box = std::make_shared<game_lootbox>();
						loot_box->put_json(aji);
						pieces.push_back(loot_box);
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

	struct collision_result
	{
		std::shared_ptr<game_piece> piece_1;
		std::shared_ptr<game_piece> piece_2;
		double time_of_collision;
        intersection_side collision_side;

        bool collided() { return piece_1.get() != nullptr && piece_2.get() != nullptr; }
    };

	class game_session : public job
	{
	public:
		std::string class_name;
		int64_t		object_id;
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
		
		rectangle get_piece_rectangle(game_piece* _piece, double _elapsed)
		{
			using namespace DirectX;

			XMVECTOR this_position_start = XMVectorAdd(_piece->position, XMVectorScale(_piece->velocity, static_cast<float>(_elapsed)));

            rectangle rect;
            rect.x = XMVectorGetX(this_position_start);
            rect.y = XMVectorGetY(this_position_start);
			rect.w = XMVectorGetX(_piece->size);
			rect.h = XMVectorGetY(_piece->size);
			return rect;
        }

		void init_piece(std::shared_ptr<game_map> _map, int _piece_index)
		{
			using namespace DirectX;

			auto _piece = _map->pieces[_piece_index];

			_piece->frame_velocity = _piece->velocity;
		}

		void reset_piece(std::shared_ptr<game_map> _map, int _piece_index)
		{
			using namespace DirectX;

			auto _piece = _map->pieces[_piece_index];

			_piece->velocity = _piece->frame_velocity;
		}

		void accelerate_piece(std::shared_ptr<game_map> _map, int _piece_index, double _elapsed_secs)
		{
			using namespace DirectX;

			auto _piece = _map->pieces[_piece_index];

			_piece->velocity = XMVectorAdd(_piece->velocity, XMVectorScale(_piece->acceleration, static_cast<float>(_elapsed_secs)));
		}

		void slide_piece(collision_result& collision)
		{
			using namespace DirectX;

			if (!collision.collided()) {
				return;
			}

			auto piece1 = collision.piece_1;
			auto piece2 = collision.piece_2;

			// Get masses
			float m1 = static_cast<float>(piece1->mass);
			float m2 = static_cast<float>(piece2->mass);

			// Get velocities
			XMVECTOR v1 = piece1->velocity;
			XMVECTOR v2 = piece2->velocity;

			// Calculate velocity difference
			XMVECTOR v_diff = XMVectorSubtract(v1, v2);

			// Calculate position difference (for collision normal)
			XMVECTOR p_diff = XMVectorSubtract(piece1->position, piece2->position);

			// Get length to figure out our total velocity
			XMVECTOR p_length = XMVector3Length(piece1->velocity);
			double l = XMVectorGetX(p_length);

            // And, that will be our slide velocity, which is the total velocity projected onto the collision normal
            if (collision.collision_side == intersection_side::intersection_side_top || collision.collision_side == intersection_side::intersection_side_bottom) {
                double xvl = XMVectorGetX(v1);
				if (xvl < 0) {
					l = -l;
                }
				// If we hit top or bottom, we want to slide along the x axis
				piece1->velocity = XMVectorSet(l, 0.0f, 0.0f, 0.0f);
			}
			else if (collision.collision_side == intersection_side::intersection_side_left || collision.collision_side == intersection_side::intersection_side_right) {
				double yvl = XMVectorGetY(v1);
				if (yvl < 0) {
					l = -l;
				}
				// If we hit left or right, we want to slide along the y axis
				piece1->velocity = XMVectorSet(0.0f, l, 0.0f, 0.0f);
			}
		}

		void recoil_piece(collision_result& collision)
		{
			using namespace DirectX;

			if (!collision.collided()) {
				return;
			}

			auto piece1 = collision.piece_1;
			auto piece2 = collision.piece_2;

			// Get masses
			float m1 = static_cast<float>(piece1->mass);
			float m2 = static_cast<float>(piece2->mass);

			// Get velocities
			XMVECTOR v1 = piece1->velocity;
			XMVECTOR v2 = piece2->velocity;

			// Calculate velocity difference
			XMVECTOR v_diff = XMVectorSubtract(v1, v2);

			// Calculate position difference (for collision normal)
			XMVECTOR p_diff = XMVectorSubtract(piece1->position, piece2->position);

			// Normalize position difference to get collision normal
			XMVECTOR normal = XMVector3Normalize(p_diff);

			// Calculate relative velocity along collision normal
			float v_rel_normal = XMVectorGetX(XMVector3Dot(v_diff, normal));

			// Only proceed if objects are moving towards each other
			if (v_rel_normal > 0) {
				return;
			}

			// Calculate impulse scalar for elastic collision
			float impulse = (2.0f * m2 * v_rel_normal) / (m1 + m2);

			// Update velocities based on elastic collision
			piece1->velocity = XMVectorSubtract(v1, XMVectorScale(normal, impulse));
			piece2->velocity = XMVectorAdd(v2, XMVectorScale(normal, (impulse * m1) / m2));

			// Set accelerations to zero after collision
			piece1->acceleration = zero_vector;
			piece2->acceleration = zero_vector;

			// Update frame velocities
			piece1->frame_velocity = piece1->velocity;
			piece2->frame_velocity = piece2->velocity;
		}

		void process_collision(collision_result& collision)
		{
			if (auto pplayer = std::dynamic_pointer_cast<game_player>(collision.piece_1)) 
			{
				if (auto target = std::dynamic_pointer_cast<game_player>(collision.piece_2)) 
				{
					// player hits another player
                    player_hits_player(collision, pplayer, target);
                }
                else if (auto pnpc = std::dynamic_pointer_cast<game_npc>(collision.piece_2))
				{
                    player_hits_npc(collision, pplayer, pnpc);
				}
				else if (auto plootbox = std::dynamic_pointer_cast<game_lootbox>(collision.piece_2)) 
				{
					player_hits_lootbox(collision, pplayer, plootbox);
				}
				else if (auto plootspot = std::dynamic_pointer_cast<game_lootspot>(collision.piece_2))
				{
					player_hits_lootspot(collision, pplayer, plootspot);
				}
				else if (auto pwall = std::dynamic_pointer_cast<game_wall>(collision.piece_2))
				{
					player_hits_wall(collision, pplayer, pwall);
				}
				else if (auto pdoor = std::dynamic_pointer_cast<game_door>(collision.piece_2)) 
				{
					player_hits_door(collision, pplayer, pdoor);
				}
			} 
			else if (auto pnpc = std::dynamic_pointer_cast<game_npc>(collision.piece_1)) 
			{
				if (auto target = std::dynamic_pointer_cast<game_player>(collision.piece_2))
				{
					// player hits another player
				}
				else if (auto pnpc = std::dynamic_pointer_cast<game_npc>(collision.piece_2))
				{
					// player hits npc
				}
				else if (auto plootbox = std::dynamic_pointer_cast<game_lootbox>(collision.piece_2))
				{
					// player hits lootbox
				}
				else if (auto plootspot = std::dynamic_pointer_cast<game_lootspot>(collision.piece_2))
				{
					// player hits lootspot
				}
				else if (auto pwall = std::dynamic_pointer_cast<game_wall>(collision.piece_2))
				{
					// player hits wall
				}
				else if (auto pdoor = std::dynamic_pointer_cast<game_door>(collision.piece_2))
				{
					// player hits door
				}

			} 
			else if (auto pshot = std::dynamic_pointer_cast<game_shot>(collision.piece_1)) 
			{

			} 
		}

		void player_hits_player(collision_result& collision, std::shared_ptr<game_player> player1, std::shared_ptr<game_player> player2)
		{
            recoil_piece(collision);
		}

		void player_hits_npc(collision_result& collision, std::shared_ptr<game_player> player1, std::shared_ptr<game_npc> player2)
		{
			recoil_piece(collision);
		}

		void player_hits_lootbox(collision_result& collision, std::shared_ptr<game_player> npc, std::shared_ptr<game_lootbox> lootbox)
		{

		}

		void player_hits_lootspot(collision_result& collision, std::shared_ptr<game_player> player, std::shared_ptr<game_lootspot> lootspot)
		{
			player->inventory->loot(*lootspot->inventory);
		}

		void player_hits_wall(collision_result& collision, std::shared_ptr<game_player> player, std::shared_ptr<game_wall> wall)
		{
			slide_piece(collision);
		}

		void player_hits_door(collision_result& collision, std::shared_ptr<game_player> npc, std::shared_ptr<game_door> lootbox)
		{
		}

		void npc_hits_player(collision_result& collision, std::shared_ptr<game_npc> player1, std::shared_ptr<game_player> player2)
		{
			recoil_piece(collision);
		}

		void npc_hits_npc(collision_result& collision, std::shared_ptr<game_npc> player1, std::shared_ptr<game_npc> player2)
		{
			recoil_piece(collision);
		}

		void npc_hits_lootbox(collision_result& collision, std::shared_ptr<game_npc> npc, std::shared_ptr<game_lootbox> lootbox)
		{

		}

		void npc_hits_lootspot(collision_result& collision, std::shared_ptr<game_npc> player, std::shared_ptr<game_lootspot> lootspot)
		{
			player->inventory->loot(*lootspot->inventory);
		}

		void npc_hits_wall(collision_result& collision, std::shared_ptr<game_npc> player, std::shared_ptr<game_wall> wall)
		{
			slide_piece(collision);
		}

		void npc_hits_door(collision_result& collision, std::shared_ptr<game_npc> npc, std::shared_ptr<game_door> lootbox)
		{
		}

		collision_result model_piece(std::shared_ptr<game_map> _map, int _piece_index, double _elapsed_secs)
		{
			using namespace DirectX;

            auto _piece = _map->pieces[_piece_index];

			collision_result collision = {};

			// can't be accelerated or accelerate anything if it's not moving or accelerating.
			// no kinetic energy
			if (XMVector3Equal(_piece->acceleration, zero_vector) &&
				XMVector3Equal(_piece->velocity, zero_vector)) {
				return collision;
			}

			bool caused_acceleration = false;

			collision.piece_1 = _piece;

            for (int i = _piece_index + 1; i < _map->pieces.size(); i++) 
			{
                auto& other = _map->pieces[i];
		
				// Check if other piece is a wall or solid object
				bool other_is_open = false;

                if (auto surface = std::dynamic_pointer_cast<game_surface>(other)) {
					other_is_open = false;
					// Apply surface mechanics to the piece's acceleration
					if (surface->mechanic == "slippery") {

					}
				}
				else if (auto wall = std::dynamic_pointer_cast<game_wall>(other)) {
					other_is_open = wall->passable;
				}
				else if (auto door = std::dynamic_pointer_cast<game_door>(other)) {
					other_is_open = door->open;
				}

				if (other_is_open) {
					continue; // Skip passable objects
				}

				double st = 0.0, et = _elapsed_secs, mt = et / 2.0;

                bool collision_detected = false;
				intersection_side collision_side;

				while (fabs(et - st) > 0.001) {
					rectangle piece_rect = get_piece_rectangle(_piece.get(), mt);
					rectangle other_rect = get_piece_rectangle(other.get(), mt);
					if (auto sides = rectangle_math::intersect(&piece_rect, &other_rect)) {
						collision_detected = true;
						et = mt; // Collision detected, search in the earlier half
						collision_side = sides;
					}
					else {
						st = mt; // No collision, search in the later half
					}
					mt = (st + et) / 2.0;
                }

				if (collision_detected) {
					if (collision.piece_2) {
						// We already have a collision, so we need to determine which one is sooner
						if (mt < collision.time_of_collision) {
							collision.piece_2 = other;
							collision.time_of_collision = mt;
                            collision.collision_side = collision_side;
						}
					}
					else {
						collision.piece_2 = other;
						collision.time_of_collision = mt;
                        collision.collision_side = collision_side;
					}
				}
			}

			return collision;
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

			double remaining = delta;
			double step_elapsed = 0.0;

            for (auto gm : maps) {
                if (gm->name == current_map) {

					for (int i = 0; i < gm->pieces.size(); i++) {
						init_piece(gm, i);
					}

					step_elapsed = remaining;
					while (remaining > 0.001) {

						collision_result closest_collision;

						for (int i = 0; i < gm->pieces.size(); i++) {
							reset_piece(gm, i);
							collision_result collision = model_piece(gm, i, step_elapsed);
							if (collision.collided()) {
								if (closest_collision.collided()) {
									if (collision.time_of_collision < closest_collision.time_of_collision) {
										closest_collision = collision;
									}
								}
								else {
									closest_collision = collision;
								}
							}
						}

                        if (closest_collision.collided()) {
							// move pieces to the point of collision
							for (int i = 0; i < gm->pieces.size(); i++) {
								accelerate_piece(gm, i, closest_collision.time_of_collision);
							}
							// resolve collision effects here and update accelerations accordingly
							// for example, if piece_1 is a player and piece_2 is a wall, we might want to stop the player's movement in the direction of the wall.
							remaining -= closest_collision.time_of_collision;

							process_collision(closest_collision);
						}
						else {
							// no more collisions, we can move all pieces for the remaining time
							for (int i = 0; i < gm->pieces.size(); i++) {
								accelerate_piece(gm, i, remaining);
							}
							remaining = 0;
						}
					}

					for (int i = 0; i < gm->pieces.size(); i++) {
						auto _piece = gm->pieces[i];
                        _piece->acceleration = zero_vector;
					}

				}
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
