#pragma once

namespace corona
{

	namespace game
	{


		struct collision_result
		{
			double				time_of_collision;
			intersection_side	collision_side;
		};

		class activity : public corona_object
		{
		public:
			std::string			name;
			std::string			description;
			bitmapInstanceDto	bitmap;
		};

		class essay : public activity
		{
		public:
			std::string         text;
		};

		class animation_frame : public corona_object
		{
		public:
			std::string			state;
			double				duration;
			double				time_point;
			rectangle			destination;
			DirectX::XMVECTOR	carry_point;

			animation_frame() = default;
			animation_frame(const animation_frame& _src) = default;
			animation_frame(animation_frame&& _src) = default;
			animation_frame& operator =(const animation_frame& _src) = default;
			animation_frame& operator =(animation_frame&& _src) = default;

			virtual void get_json(json& _dest)
			{
				json_parser jp;

				corona_object::get_json(_dest);
				_dest.put_member("state", state);
				_dest.put_member("duration", duration);
				_dest.put_member("time_point", time_point);
				_dest.put_member("destination", destination);
				_dest.put_member("carry_point", carry_point);
			}

			virtual void put_json(json& _src)
			{
				corona_object::get_json(_src);
				state = _src["state"].as_string();
				duration = _src["duration"].as_double();
				time_point = _src["time_point"].as_double();
				destination = _src["destination"].as_rectangle();
				carry_point = _src["carry_point"].as_vector();
			}
		};

		class bitmap_animation_frame : public animation_frame
		{
		public:
			bitmapInstanceDto bitmap;

            bitmap_animation_frame() = default;
            bitmap_animation_frame(const bitmap_animation_frame& _src) = default;
            bitmap_animation_frame(bitmap_animation_frame&& _src) = default;
            bitmap_animation_frame& operator =(const bitmap_animation_frame& _src) = default;
            bitmap_animation_frame& operator =(bitmap_animation_frame&& _src) = default;

            virtual void get_json(json& _dest)
			{
				json_parser jp;
				animation_frame::get_json(_dest);
				json jbitmap = jp.create_object();
				bitmap.get_json(jbitmap);
				_dest.put_member("bitmap", jbitmap);
            }

			virtual void put_json(json& _src)
			{
				animation_frame::put_json(_src);
				json jbitmap = _src["bitmap"];
				bitmap.put_json(jbitmap);
            }
		};

		class vector_animation_frame : public animation_frame
		{
		public:
			pathDto path;
            generalBrushRequest fill;
			generalBrushRequest stroke;
            double stroke_width = 1.0;

			vector_animation_frame() = default;
			vector_animation_frame(const vector_animation_frame& _src) = default;
			vector_animation_frame(vector_animation_frame&& _src) = default;
			vector_animation_frame& operator =(const vector_animation_frame& _src) = default;
			vector_animation_frame& operator =(vector_animation_frame&& _src) = default;

			virtual void get_json(json& _dest)
			{
				json_parser jp;
				animation_frame::get_json(_dest);
				json jpath = jp.create_object();
				path.get_json(jpath);
				_dest.put_member("path", jpath);
				json jfill = jp.create_object();
				fill.get_json(jfill);
				_dest.put_member("fill", jfill);
				json jstroke = jp.create_object();
				stroke.get_json(jstroke);
				_dest.put_member("stroke", jstroke);
				_dest.put_member("stroke_width", stroke_width);
			}

			virtual void put_json(json& _src)
			{
				animation_frame::put_json(_src);
				json jpath = _src["path"];
				path.put_json(jpath);
				json jfill = _src["fill"];
				fill.put_json(jfill);
				json jstroke = _src["stroke"];
				stroke.put_json(jstroke);
				stroke_width = _src["stroke_width"].as_double();
			}
		};


		class piece_animation : public corona_object
		{
		public:
			std::string state;
			double duration;
            rectangle destination;
			DirectX::XMVECTOR direction;
            audio_function sound;
            std::vector<std::shared_ptr<animation_frame>> frames;

			piece_animation() = default;
			piece_animation(const piece_animation& _src) = default;
			piece_animation(piece_animation&& _src) = default;
			piece_animation& operator =(const piece_animation& _src) = default;
			piece_animation& operator =(piece_animation&& _src) = default;

			virtual void get_json(json& _dest)
			{
                _dest.put_member("state", state);
                _dest.put_member("duration", duration);
                _dest.put_member("destination", destination);
                _dest.put_member("direction", direction);
                json_parser jp;
                json jframes = jp.create_array();
				for (auto& frame : frames) {
					json jframe = jp.create_object();
					frame->get_json(jframe);
					jframes.push_back(jframe);
                }
                _dest.put_member("frames", jframes);
			}

			virtual void put_json(json& _src)
			{
				state = _src["state"].as_string();
				duration = _src["duration"].as_double();
				destination = _src["destination"].as_rectangle();
				direction = _src["direction"].as_vector();
				json jframes = _src["frames"];
				frames.clear();

				if (jframes.array()) {
					for (int i = 0; i < jframes.size(); i++) {
						auto jframe = jframes.get_element(i);
						if (!jframe.object()) {
							continue;
						}
						std::shared_ptr<animation_frame> frame;
						if (jframe.has_member("bitmap")) {
							frame = std::make_shared<bitmap_animation_frame>();
						}
						else if (jframe.has_member("path")) {
							frame = std::make_shared<vector_animation_frame>();
						}
						else {
							frame = std::make_shared<animation_frame>();
						}
						frame->put_json(jframe);
						frames.push_back(frame);
					}
				}
                json jsound = _src["sound"];
                sound = audio_graph::from_json(jsound);
			}
		};

		using game_sprite_factory = corona_object_factory<game_sprite>;

		class game_piece : public corona_object
		{
		public:
			std::string name;
			std::string image_name;
			std::string state;
			std::vector<std::shared_ptr<game_sprite>> sprites;
			double		mass;
			double      full_hit_points;
			double      hit_points;
			std::shared_ptr<chest_field> inventory;

			DirectX::XMVECTOR position = {};
			DirectX::XMVECTOR facing = {};
			DirectX::XMVECTOR size = {};
			DirectX::XMVECTOR velocity = {};
			DirectX::XMVECTOR frame_velocity = {};
			DirectX::XMVECTOR acceleration = {};

			game_piece() = default;
			game_piece(const game_piece& _src) = default;
			game_piece(game_piece&& _src) = default;
			game_piece& operator =(const game_piece& _src) = default;
			game_piece& operator =(game_piece&& _src) = default;

			virtual void get_json(json& _dest)
			{
				json_parser jp;

				corona_object::get_json(_dest);

				_dest.put_member("image_name", image_name);
				_dest.put_member("state", state);
				_dest.put_member("name", name);
				_dest.put_member("position", position);
				_dest.put_member("acceleration", acceleration);
				_dest.put_member("velocity", velocity);
				_dest.put_member("size", size);
				_dest.put_member("mass", mass);
				_dest.put_member("full_hit_points", full_hit_points);
				_dest.put_member("hit_points", hit_points);	

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

			virtual void put_json(game_sprite_factory& _factory, json& _src)
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
				hit_points = _src["hit_points"].as_double();
				full_hit_points = _src["full_hit_points"].as_double();

				if (fabs(mass) < 0.0001) {
					mass = 1.0;
				}

				json j = _src["sprites"];
				sprites = _factory.create_array(j);

				inventory = std::make_shared<chest_field>();
				if (_src.has_member("inventory")) {
					json jinventory = _src["inventory"];
					inventory->put_json(jinventory);
				}
			}

			virtual void collide(collision_result& collision, std::shared_ptr<game_piece> _other)
			{
				; // default pieces do not react to collisions, but they can be overridden in derived classes
			}

			rectangle get_rectangle(double _elapsed)
			{
				using namespace DirectX;

				XMVECTOR this_position_start = XMVectorAdd(position, XMVectorScale(velocity, static_cast<float>(_elapsed)));

				rectangle rect;
				rect.x = XMVectorGetX(this_position_start);
				rect.y = XMVectorGetY(this_position_start);
				rect.w = XMVectorGetX(size);
				rect.h = XMVectorGetY(size);
				return rect;
			}

			void init_frame()
			{
				using namespace DirectX;

				frame_velocity = velocity;
			}

			void reset_frame()
			{
				using namespace DirectX;

				velocity = frame_velocity;
			}

			void accelerate(double _elapsed_secs)
			{
				using namespace DirectX;

				velocity = XMVectorAdd(velocity, XMVectorScale(acceleration, static_cast<float>(_elapsed_secs)));
			}

			void slide_piece(collision_result& collision, std::shared_ptr<game_piece> _other)
			{
				using namespace DirectX;

				auto piece1 = this;
				auto piece2 = _other;

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

			void recoil_piece(collision_result& collision, std::shared_ptr<game_piece> _other)
			{
				using namespace DirectX;

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
				piece1->acceleration = XMVectorZero();
				piece2->acceleration = XMVectorZero();

				// Update frame velocities
				piece1->frame_velocity = piece1->velocity;
				piece2->frame_velocity = piece2->velocity;
			}

		};

		using game_piece_factory = corona_object_factory<game_piece>;


		class game_bus 
		{
		public:
			game_piece_factory piece_factory;
			game_sprite_factory sprite_factory;
		};

		class game_effect : public game_piece
		{
		public:

			scheduled_lambda<game_effect*> scheduler;

			std::string		name;

			game_effect() = default;
			game_effect(const game_effect& _src) = default;
			game_effect(game_effect&& _src) = default;
			game_effect& operator =(const game_effect& _src) = default;
			game_effect& operator =(game_effect&& _src) = default;

			virtual void get_json(json& _dest)
			{
				game_piece::get_json(_dest);
				_dest.put_member_string(class_name_field, class_name);
				_dest.put_member_i64(object_id_field, object_id);

				corona::get_json(_dest, scheduler);

				_dest.put_member_string("name", name);
			}

			virtual void put_json(game_bus& _gbus, json& _src)
			{
				game_piece::put_json(_gbus.sprite_factory, _src);
				class_name = _src[class_name_field].as_string();
				object_id = _src[object_id_field].as_int64_t();

				corona::put_json(scheduler, _src);
			}
		};

		class game_player_spawn : public game_piece
		{
		public:

			game_player_spawn() = default;
			game_player_spawn(const game_player_spawn& _src) = default;
			game_player_spawn(game_player_spawn&& _src) = default;
			game_player_spawn& operator =(const game_player_spawn& _src) = default;
			game_player_spawn& operator =(game_player_spawn&& _src) = default;

			virtual void get_json(json& _dest)
			{
				game_piece::get_json(_dest);
			}

			virtual void put_json(game_bus& _gbus, json& _src)
			{
				game_piece::put_json(_gbus.sprite_factory, _src);
			}
		};

		class game_npc_spawn : public game_piece
		{
		public:

			scheduled_lambda<game_npc_spawn*> spawn_clock;
			std::vector<std::string> spawn_classes;

			game_npc_spawn() = default;
			game_npc_spawn(const game_npc_spawn& _src) = default;
			game_npc_spawn(game_npc_spawn&& _src) = default;
			game_npc_spawn& operator =(const game_npc_spawn& _src) = default;
			game_npc_spawn& operator =(game_npc_spawn&& _src) = default;

			virtual void get_json(json& _dest)
			{
				game_piece::get_json(_dest);
				json_parser jp;
				json j = jp.from_string_container(spawn_classes);
				_dest.put_member("spawn_classes", j);
			}

			virtual void put_json(game_bus& _gbus, json& _src)
			{
				game_piece::put_json(_gbus.sprite_factory, _src);
				json jsc = _src["spawn_classes"];
				json timer = _src["schedule"];

				spawn_classes = jsc.to_string_array();
			}

			std::string get_spawn_class()
			{
				std::string result;
				std::random_device rd; // Seed generator
				std::mt19937 gen(rd()); // Mersenne Twister engine
				std::uniform_int_distribution<> dist(0, spawn_classes.size() - 1);
				int index = dist(gen);
				result = spawn_classes[index];
			}
		};

		class game_light : public game_piece
		{
		public:

			game_light() = default;
			game_light(const game_light& _src) = default;
			game_light(game_light&& _src) = default;
			game_light& operator =(const game_light& _src) = default;
			game_light& operator =(game_light&& _src) = default;

			virtual void get_json(json& _dest)
			{
				game_piece::get_json(_dest);
			}

			virtual void put_json(game_bus& _gbus, json& _src)
			{
				game_piece::put_json(_gbus.sprite_factory, _src);
			}
		};


		class game_switch : public game_piece
		{
		public:

			game_switch() = default;
			game_switch(const game_switch& _src) = default;
			game_switch(game_switch&& _src) = default;
			game_switch& operator =(const game_switch& _src) = default;
			game_switch& operator =(game_switch&& _src) = default;

			virtual void get_json(json& _dest)
			{
				game_piece::get_json(_dest);
			}

			virtual void put_json(game_bus& _gbus, json& _src)
			{
				game_piece::put_json(_gbus.sprite_factory, _src);
			}
		};

		class game_lootspot : public game_piece
		{
		public:
			game_lootspot() = default;
			game_lootspot(const game_lootspot& _src) = default;
			game_lootspot(game_lootspot&& _src) = default;
			game_lootspot& operator =(const game_lootspot& _src) = default;
			game_lootspot& operator =(game_lootspot&& _src) = default;

			virtual void get_json(json& _dest)
			{
				game_piece::get_json(_dest);
			}

			virtual void put_json(game_bus& _gbus, json& _src)
			{
				game_piece::put_json(_gbus.sprite_factory, _src);
			}
		};

		class game_lootbox : public game_piece
		{
		public:
			game_lootbox() = default;
			game_lootbox(const game_lootbox& _src) = default;
			game_lootbox(game_lootbox&& _src) = default;
			game_lootbox& operator =(const game_lootbox& _src) = default;
			game_lootbox& operator =(game_lootbox&& _src) = default;

			virtual void get_json(json& _dest)
			{
				game_piece::get_json(_dest);
			}

			virtual void put_json(game_bus& _gbus, json& _src)
			{
				game_piece::put_json(_gbus.sprite_factory, _src);
			}
		};

		class game_wall : public game_piece
		{
		public:
			game_wall() = default;
			game_wall(const game_wall& _src) = default;
			game_wall(game_wall&& _src) = default;
			game_wall& operator =(const game_wall& _src) = default;
			game_wall& operator =(game_wall&& _src) = default;

			bool passable;

			virtual void get_json(json& _dest)
			{
				game_piece::get_json(_dest);
				_dest.put_member("passable", passable);
			}

			virtual void put_json(game_bus& _gbus, json& _src)
			{
				game_piece::put_json(_gbus.sprite_factory, _src);
				passable = _src["passable"].as_bool();
			}
		};

		class game_door : public game_piece
		{
		public:
			game_door() = default;
			game_door(const game_door& _src) = default;
			game_door(game_door&& _src) = default;
			game_door& operator =(const game_door& _src) = default;
			game_door& operator =(game_door&& _src) = default;

			bool open;

			virtual void get_json(json& _dest)
			{
				_dest.put_member_bool("open", open);
			}

			virtual void put_json(game_bus& _gbus, json& _src)
			{
				game_piece::put_json(_gbus.sprite_factory, _src);
				open = _src["open"].as_bool();
			}
		};

		class game_surface : public game_piece
		{
		public:
			game_surface() = default;
			game_surface(const game_surface& _src) = default;
			game_surface(game_surface&& _src) = default;
			game_surface& operator =(const game_surface& _src) = default;
			game_surface& operator =(game_surface&& _src) = default;

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
			game_decoration() = default;
			game_decoration(const game_decoration& _src) = default;
			game_decoration(game_decoration&& _src) = default;
			game_decoration& operator =(const game_decoration& _src) = default;
			game_decoration& operator =(game_decoration&& _src) = default;

			virtual void get_json(json& _dest)
			{

			}

			virtual void put_json(game_bus& _gbus, json& _src)
			{
				game_piece::put_json(_gbus.sprite_factory, _src);
			}
		};

		class game_player : public game_piece
		{
		public:

			game_player() = default;
			game_player(const game_player& _src) = default;
			game_player(game_player&& _src) = default;
			game_player& operator =(const game_player& _src) = default;
			game_player& operator =(game_player&& _src) = default;

			std::string								input_device;
			std::shared_ptr<selection_field>		selection;
			bool									ready;
			bool									dead;

			virtual void get_json(json& _dest)
			{
				game_piece::get_json(_dest);
				_dest.put_member("input_device", input_device);
				_dest.put_member_bool("ready", ready);
				_dest.put_member_bool("dead", dead);
			}

			virtual void put_json(game_bus& _gbus, json& _src)
			{
				game_piece::put_json(_gbus.sprite_factory, _src);
				input_device = _src["input_device"].as_int();
				ready = _src["ready"].as_bool();
				dead = _src["dead"].as_bool();		
			}

			virtual void hit_player(collision_result& collision, std::shared_ptr<game_player>& pplayer)
			{
				recoil_piece(collision, pplayer);
			}

			virtual void hit_npc(collision_result& collision, std::shared_ptr<game_npc>& pnpc)
			{
				recoil_piece(collision, pnpc);
			}

			virtual void hit_lootbox(collision_result& collision, std::shared_ptr<game_lootbox>& plootbox)
			{
				recoil_piece(collision, plootbox);
			}

			virtual void hit_lootspot(collision_result& collision, std::shared_ptr<game_lootspot>& plootspot)
			{
				inventory->loot(*plootspot->inventory);
			}

			virtual void hit_switch(collision_result& collision, std::shared_ptr<game_switch>& pswitch)
			{

			}

			virtual void hit_wall(collision_result& collision, std::shared_ptr<game_wall>& pwall)
			{
				if (pwall->passable) {
					return;
				}
				slide_piece(collision, pwall);
			}

			virtual void hit_door(collision_result& collision, std::shared_ptr<game_door>& pdoor)
			{
				if (pdoor->open) {
					return;
				}
				recoil_piece(collision, pdoor);
			}

			virtual void collide(collision_result& collision, std::shared_ptr<game_piece> _other)
			{
				if (auto pplayer = std::dynamic_pointer_cast<game_player>(_other)) {
					hit_player(collision, pplayer);
				}
				else if (auto pnpc = std::dynamic_pointer_cast<game_npc>(_other)) {
					hit_npc(collision, pnpc);
				}
				else if (auto plootbox = std::dynamic_pointer_cast<game_lootbox>(_other)) {
					hit_lootbox(collision, plootbox);
				}
				else if (auto plootspot = std::dynamic_pointer_cast<game_lootspot>(_other)) {
					hit_lootspot(collision, plootspot);
				}
				else if (auto pswitch = std::dynamic_pointer_cast<game_switch>(_other)) {
					hit_switch(collision, pswitch);
				}
				else if (auto pwall = std::dynamic_pointer_cast<game_wall>(_other)) {
					hit_wall(collision, pwall);
				}
				else if (auto pdoor = std::dynamic_pointer_cast<game_door>(_other)) {
					hit_door(collision, pdoor);
				}
			}

			// and now, we can extend the selection and the inventory

			virtual void select_next()
			{
				if (selection && inventory) {
					auto selected = selection->get_first();
					if (selected) {
						auto* inventory_item = inventory->find_next(*selected);
						if (inventory_item) {
							selection->clear();
							selection->extend(*inventory_item);
						}
					}
					else {
						auto first_item = selected;
						if (first_item) {
							selection->extend(*first_item);
						}
					}
				}
			}

			virtual void select_previous()
			{
				if (selection && inventory) {
					auto selected = selection->get_first();
					if (selected) {
						auto* inventory_item = inventory->find_previous(*selected);
						if (inventory_item) {
							selection->clear();
							selection->extend(*inventory_item);
						}
					}
					else {
						auto first_item = selected;
						if (first_item) {
							selection->extend(*first_item);
						}
					}
				}
			}

			virtual void select_none()
			{
				if (selection) 
				{
					selection->clear();
				}
			}

			virtual void use_current()
			{
				if (selection) 
				{
					auto item = selection->get_first();
					if (item) {
						;
					}
				}
			}

			virtual void throw_current()
			{
				if (selection) 
				{

				}
			}

		};

		class game_shot : public game_piece
		{

		public:

			game_shot() = default;
			game_shot(const game_shot& _src) = default;
			game_shot(game_shot&& _src) = default;
			game_shot& operator =(const game_shot& _src) = default;
			game_shot& operator =(game_shot&& _src) = default;

			double      damage;
			std::string originator;

			virtual void get_json(json& _dest)
			{
				game_piece::get_json(_dest);
				_dest.put_member("originator", originator);
				_dest.put_member("damage", damage);
			}

			virtual void put_json(game_bus& _gbus, json& _src)
			{
				game_piece::put_json(_gbus.sprite_factory, _src);
				originator = _src["originator"].as_string();
				damage = _src["damage"].as_double();
			}

			virtual void hit_player(collision_result& collision, std::shared_ptr<game_player>& pplayer) = 0;
			virtual void hit_npc(collision_result& collision, std::shared_ptr<game_npc>& pnpc) = 0;
			virtual void hit_lootbox(collision_result& collision, std::shared_ptr<game_lootbox>& plootbox) = 0;
			virtual void hit_switch(collision_result& collision, std::shared_ptr<game_switch>& pswitch) = 0;
			virtual void hit_lootspot(collision_result& collision, std::shared_ptr<game_lootspot>& plootspot) = 0;
			virtual void hit_wall(collision_result& collision, std::shared_ptr<game_wall>& pwall) = 0;
			virtual void hit_door(collision_result& collision, std::shared_ptr<game_door>& pdoor) = 0;

		};

		class game_npc : public game_piece
		{
		public:
			game_npc() = default;
			game_npc(const game_npc& _src) = default;
			game_npc(game_npc&& _src) = default;
			game_npc& operator =(const game_npc& _src) = default;
			game_npc& operator =(game_npc&& _src) = default;

			virtual void get_json(json& _dest)
			{
				game_piece::get_json(_dest);
			}

			virtual void put_json(game_bus& _gbus, json& _src)
			{
				game_piece::put_json(_gbus.sprite_factory, _src);
			}

			virtual void hit_player(collision_result& collision, std::shared_ptr<game_player>& pplayer)
			{
				recoil_piece(collision, pplayer	);
			}
			virtual void hit_npc(collision_result& collision, std::shared_ptr<game_npc>& pnpc)
			{
				recoil_piece(collision, pnpc);
			}
			virtual void hit_lootbox(collision_result& collision, std::shared_ptr<game_lootbox>& plootbox)
			{
				recoil_piece(collision, plootbox);
			}
			virtual void hit_lootspot(collision_result& collision, std::shared_ptr<game_lootspot>& plootspot)
			{
				inventory->loot(*plootspot->inventory);
			}

			virtual void hit_switch(collision_result& collision, std::shared_ptr<game_switch>& pswitch)
			{

			}

			virtual void hit_wall(collision_result& collision, std::shared_ptr<game_wall>& pwall)
			{
				if (pwall->passable) {
					return;
				}
				slide_piece(collision, pwall);
			}
			virtual void hit_door(collision_result& collision, std::shared_ptr<game_door>& pdoor)
			{
				if (pdoor->open) {
					return;
				}
				recoil_piece(collision, pdoor);
			}

		};


		class game_map
		{
		public:

			game_map() = default;
			game_map(const game_map& _src) = default;
			game_map(game_map&& _src) = default;
			game_map& operator =(const game_map& _src) = default;
			game_map& operator =(game_map&& _src) = default;

			std::string class_name;
			int64_t object_id;
			std::string name;

			std::vector<std::shared_ptr<game_piece>> pieces;

			// C++20 view functions - no separate storage needed
			template<std::derived_from<game_piece> T>
			auto get_pieces_of_type() const {
				return pieces 
					| std::views::transform([](const auto& p) { 
						return std::dynamic_pointer_cast<T>(p); 
					  })
					| std::views::filter([](const auto& p) { return p != nullptr; });
			}

			// Convenience accessors for common types
			auto players() const { return get_pieces_of_type<game_player>(); }
			auto npcs() const { return get_pieces_of_type<game_npc>(); }
			auto walls() const { return get_pieces_of_type<game_wall>(); }
			auto player_spawns() const { return get_pieces_of_type<game_player_spawn>(); }
			auto npc_spawns() const { return get_pieces_of_type<game_npc_spawn>(); }
			auto lights() const { return get_pieces_of_type<game_light>(); }
			auto switches() const { return get_pieces_of_type<game_switch>(); }
			auto lootboxes() const { return get_pieces_of_type<game_lootbox>(); }
			auto lootspots() const { return get_pieces_of_type<game_lootspot>(); }
			auto doors() const { return get_pieces_of_type<game_door>(); }
			auto decorations() const { return get_pieces_of_type<game_decoration>(); }
			auto shots() const { return get_pieces_of_type<game_shot>(); }
			auto surfaces() const { return get_pieces_of_type<game_surface>(); }

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

			virtual void put_json(game_bus& _gbus, json& _src)
			{
				name = _src["name"].as_string();
				json j = _src["pieces"];

				pieces = _gbus.piece_factory.create_array(j);
			}

		};

		enum class game_session_state
		{
			lobby,
			active,
			paused,
			complete,
			exit
		};

		class collision_event
		{
		public:
			std::shared_ptr<game_piece> piece_1;
			std::shared_ptr<game_piece> piece_2;
			collision_result collision;
		};

		class game_state_event
		{
		public:
			game_session_state state;

			std::shared_ptr<corona_bus_command> A_down;
			std::shared_ptr<corona_bus_command> B_down;
			std::shared_ptr<corona_bus_command> X_down;
			std::shared_ptr<corona_bus_command> Y_down;
			std::shared_ptr<corona_bus_command> LeftTrigger_down;
			std::shared_ptr<corona_bus_command> RightTrigger_down;
			std::shared_ptr<corona_bus_command> LeftShoulder_down;
			std::shared_ptr<corona_bus_command> RightShoulder_down;
			std::shared_ptr<corona_bus_command> Dpad_up;
			std::shared_ptr<corona_bus_command> Dpad_down;
			std::shared_ptr<corona_bus_command> Dpad_right;
			std::shared_ptr<corona_bus_command> Dpad_left;
			std::shared_ptr<corona_bus_command> Left_Thumb_move;
			std::shared_ptr<corona_bus_command> Right_Thumb_move;
			std::shared_ptr<corona_bus_command> On_All_Players_Ready;
			std::shared_ptr<corona_bus_command> On_All_Players_Dead;
		};

		class game_session : public job, public corona_object
		{
			std::shared_ptr<comm_bus_app_interface> bus;
			int64_t				object_id;
			std::string			name;
			std::string			description;
			timer				frame_timer;
			lockable			map_locker;
			game_session_state	game_state;

			double last_elapsed_seconds;

			std::shared_ptr<game_map> map;

			DirectX::XMVECTOR zero_vector = {};

			std::shared_ptr<game_player> attach_player(std::string input_name)
			{
				// Search for existing player with this input device
				for (auto player : map->players()) {
					if (player->input_device == input_name) {
						return player;
					}
				}
				// Create new player and add to pieces only
				std::shared_ptr<game_player> new_player = std::make_shared<game_player>();
				new_player->name = input_name;
				new_player->ready = false;
				map->pieces.push_back(new_player);
				return new_player;
			}

			std::shared_ptr<game_player> attach_player(XINPUT_STATE& _input_state)
			{
				// Search for existing player with this input device
				for (auto player : map->players()) {
					if (player->input_device == _input_state.dwPacketNumber) {
						return player;
					}
				}

				// Create new player and add to pieces only
				std::shared_ptr<game_player> new_player = std::make_shared<game_player>();
				new_player->input_device = _input_state.dwPacketNumber;
				new_player->name = "Player " + std::to_string(_input_state.dwPacketNumber);
				new_player->ready = false;
				map->pieces.push_back(new_player);
				return new_player;
			}

			void fire_player_event(std::shared_ptr<corona_bus_command> _command, std::string input_name)
			{
				if (input_name.empty())
				{
					return;
				}

				_command->set_parameter("input_name", input_name);
				bus->run_command(0, _command);
			}

			std::map<game_session_state, game_state_event> state_events;

		public:

			game_session()
			{
				;
			}

			game_session(std::shared_ptr<comm_bus_app_interface> _bus, json& _src)
			{
				put_json(_src);
			}


			corona_client_response clear_selection(std::string input_name)
			{

			}

			corona_client_response extend_selection(std::string input_name)
			{

			}

			corona_client_response throw_selection(std::string input_name)
			{

			}

			corona_client_response drop_selection(std::string input_name)
			{

			}

			corona_client_response use_selection(std::string input_name)
			{

			}

			corona_client_response select_next(std::string input_name)
			{

			}

			corona_client_response select_previous(std::string input_name)
			{

			}

			corona_client_response add_pieces(json _pieces)
			{

			}

			corona_client_response remove_pieces(json _pieces)
			{

			}

			corona_client_response purchase_pieces(std::string input_name, json _for_sale, json _price)
			{

			}

			void set_lobby()
			{
				// Remove all players from the pieces collection
				auto is_player = [](const auto& piece) {
					return std::dynamic_pointer_cast<game_player>(piece) != nullptr;
					};
				map->pieces.erase(
					std::remove_if(map->pieces.begin(), map->pieces.end(), is_player),
					map->pieces.end()
				);
				game_state = game_session_state::lobby;
			}

			void set_active()
			{
				game_state = game_session_state::active;
			}

			void set_paused()
			{
				game_state = game_session_state::paused;
			}

			void set_complete()
			{
				game_state = game_session_state::complete;
			}

			void set_exit()
			{
				game_state = game_session_state::exit;
			}

			void start_play(std::string input_name)
			{
				auto player = attach_player(input_name);

				if (game_state == game_session_state::lobby) {
					player->ready = !player->ready;
					start_game_if_all_ready();
				}
				else if (game_state == game_session_state::active) {
					player->dead = false;
					set_paused();
				}
				else if (game_state == game_session_state::paused) {
					set_active();
				}
				else if (game_state == game_session_state::complete) {
					set_lobby();
				}
				else if (game_state == game_session_state::exit) {
					; // do nothing, we're exiting anyway
				}
				break;
			}

			void check_all_ready()
			{
				if (game_state == game_session_state::lobby) {
					auto players_view = map->players();
					bool all_ready = std::ranges::all_of(players_view, [](const auto& player) {
						return player->ready;
					});
					if (all_ready) {
						set_active();
					}
				}
			}

			void check_all_dead()
			{
				auto players_view = map->players();
				bool all_dead = std::ranges::all_of(players_view, [](const auto& player) {
					return player->dead;
					});
				if (all_dead) { 
					set_complete();
				}
			}

			void handle_gamepad_button_up(gamepad_button_up_event gpbd)
			{
				auto player = attach_player(gpbd.state); 
			}

			void handle_gamepad_button_down(gamepad_button_down_event gpbd) 
			{
				auto player = attach_player(gpbd.state);
				switch (gpbd.button) 
				{
				case gamepad_button::A:
					break;
				case gamepad_button::B:
					break;
				case gamepad_button::X:  // It's good to learn to not hit the X button.
					player->dead = true;
					break;
				case gamepad_button::Y:
					break;
				case gamepad_button::LeftShoulder:
					break;
				case gamepad_button::RightShoulder:
					break;
				case gamepad_button::Back:
					player->dead = false;
					break;
				case gamepad_button::Start:
				case gamepad_button::DpadUp:
					player->acceleration = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
					break;
				case gamepad_button::DpadDown:
					player->acceleration = DirectX::XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f);
					break;
				case gamepad_button::DpadLeft:
					player->acceleration = DirectX::XMVectorSet(-1.0f, 0.0f, 0.0f, 0.0f);
					break;
				case gamepad_button::DpadRight:
					player->acceleration = DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
					break;
				}
			}

			void handle_gamepad_trigger_up(gamepad_trigger_up_event gptu) 
			{
				auto player = attach_player(gptu.state);
			}

			void handle_gamepad_trigger_down(gamepad_trigger_down_event gptd) 
			{
			}

			void handle_gamepad_thumbstick_move(gamepad_thumbstick_move_event gpbd) 
			{
				auto player = attach_player(gpbd.state);
				player->acceleration = DirectX::XMVectorSet(gpbd.x, gpbd.y, 0.0f, 0.0f);
			}

			virtual void get_json(json& _dest)
			{
				json_parser jp;
				_dest.put_member("name", name);
				_dest.put_member("description", description);
				json j = jp.create_array();
				json jmap = jp.create_object();
				map->get_json(jmap);
				j.push_back(jmap);
				_dest.put_member("map", j);

				switch (game_state) {
				case game_session_state::lobby:
					_dest.put_member_string("state", "lobby");
					break;
				case game_session_state::active:
					_dest.put_member_string("state", "active");
					break;
				case game_session_state::paused:
					_dest.put_member_string("state", "paused");
					break;
				case game_session_state::complete:
					_dest.put_member_string("state", "complete");
					break;
				case game_session_state::exit:
					_dest.put_member_string("state", "exit");
					break;
				}
			}

			virtual void put_json(json& _src)
			{
				name = _src["name"].as_string();
				description = _src["description"].as_string();
				json j = _src["map"];
				map = std::make_shared<game_map>();
				std::string state_string = _src["state"].as_string();
			}

			job* get_next_job()
			{
				if (game_state != game_session_state::exit)
				{
					return this;
				}
				else
				{
					return nullptr;
				}
			}

		private:


			collision_event model_piece(std::shared_ptr<game_map> _map, int _piece_index, double _elapsed_secs)
			{
				using namespace DirectX;

				auto _piece = _map->pieces[_piece_index];

				collision_event event = {};
				collision_result collision = {};

				// can't be accelerated or accelerate anything if it's not moving or accelerating.
				// no kinetic energy
				if (XMVector3Equal(_piece->acceleration, zero_vector) &&
					XMVector3Equal(_piece->velocity, zero_vector)) {
					return event;
				}

				for (int i = _piece_index + 1; i < _map->pieces.size(); i++) 
				{
					auto& other = _map->pieces[i];

					double st = 0.0, et = _elapsed_secs, mt = et / 2.0;

					bool collision_detected = false;
					intersection_side collision_side;

					while (fabs(et - st) > 0.001) {
						rectangle piece_rect = _piece->get_rectangle(mt);
						rectangle other_rect = other->get_rectangle(mt);
						if (auto sides = rectangle_math::intersect(&piece_rect, &other_rect)) {
							collision_detected = true;
							et = mt; // Collision detected, search in the earlier half
							collision_side = sides;
						}
						else 
						{
							st = mt; // No collision, search in the later half
						}
						mt = (st + et) / 2.0;
					}

					if (collision_detected) {

					
						// We already have a collision, so we need to determine which one is sooner
						if (mt < collision.time_of_collision) {
							collision.time_of_collision = mt;
							collision.collision_side = collision_side;
							event.collision = collision;
							event.piece_1 = _piece;
							event.piece_2 = other;
						}
					}
				}

				return event;
			}

			void run_lobby(double delta)
			{
				;
			}

			collision_event find_closest_collision(double delta)
			{
				collision_event closest_collision;
				for (int i = 0; i < map->pieces.size(); i++) {
					auto pc = map->pieces[i];
					pc->reset_frame();
					collision_event collision = model_piece(map, i, delta);
					if (collision.piece_1) {
						if (closest_collision.piece_1) {
							if (collision.collision.time_of_collision < closest_collision.collision.time_of_collision) {
								closest_collision = collision;
							}
						}
						else {
							closest_collision = collision;
						}
					}
				}
				return closest_collision;
			}

			void run_active(double delta)
			{
				// we have to resolve collision effects first, because, 
				// that gives us new accelerations.

				// in the model, physical quantities are given in seconds, so, we can apply the ax, ay to terms
				// and that gives us simple linear models.
				// for other models, we can additionally scale the time so that seconds could be weeks,
				// months or years.

				double remaining = delta;

				while (remaining > 0.001) {

					collision_event closest_collision = find_closest_collision(remaining);

					if (closest_collision.piece_1) {
						// move pieces to the point of collision
						for (int i = 0; i < map->pieces.size(); i++) {
							auto pc = map->pieces[i];
							pc->accelerate(closest_collision.collision.time_of_collision);
						}
						// resolve collision effects here and update accelerations accordingly
						// for example, if piece_1 is a player and piece_2 is a wall, we might want to stop the player's movement in the direction of the wall.
						remaining -= closest_collision.collision.time_of_collision;

						closest_collision.piece_1->collide(closest_collision.collision, closest_collision.piece_2);
					}
					else
					{
						// no more collisions, we can move all pieces for the remaining time
						for (int i = 0; i < map->pieces.size(); i++) {
							auto pc = map->pieces[i];
							pc->accelerate(remaining);
						}
						remaining = 0;
					}
				}

				for (int i = 0; i < map->pieces.size(); i++) {
					auto _piece = map->pieces[i];
					_piece->acceleration = zero_vector;
				};
			}

			void run_complete(double delta)
			{
				;
			}

			void run_paused(double delta)
			{
				;
			}

			void run_exit(double delta)
			{
				;
			}

			virtual job_notify execute(job_queue* _callingQueue, DWORD _bytesTransferred, BOOL _success)
			{
				job_notify notify;
				json_parser jp;

				notify.shouldDelete = false;

				double current_elapsed_seconds = frame_timer.get_elapsed_seconds();
				double delta = current_elapsed_seconds - last_elapsed_seconds;

				last_elapsed_seconds = current_elapsed_seconds;

				switch (game_state) {
				case game_session_state::lobby:
					run_lobby(delta);
					break;
				case game_session_state::active:
					run_active(delta);
					break;
				case game_session_state::complete:
					run_complete(delta);
					break;
				case game_session_state::paused:
					run_paused(delta);
					break;
				case game_session_state::exit:
					run_exit(delta);
					break;
				}

				return notify;
			}

		};

		class game_engine
		{
			std::shared_ptr<comm_bus_app_interface> bus;
			corona_instance instance = corona_instance::local;
			std::vector<std::shared_ptr<game_session>> sessions;

		public:

			game_engine(std::shared_ptr<comm_bus_app_interface> _db) : bus(_db)
			{
			}


			std::shared_ptr<game_session> new_game_session(json _game_key)
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
					sessions.push_back(session);
					return session;
				}

				return nullptr;
			}

			std::shared_ptr<game_session> load_game_session(json _session_key)
			{
				json_parser jp;
				auto result = bus->get_object(instance, _session_key);
				if (result.success) {
					std::shared_ptr<game_session> session = std::make_shared<game_session>();
					session->put_json(result.data);
					sessions.push_back(session);
					return session;
				}
				return nullptr;
			}

			void save_game_session(std::shared_ptr<game_session> _session)
			{
				json_parser jp;
				json jsession = jp.create_object();
				_session->get_json(jsession);
				bus->put_object(instance, jsession);
			}

			void close_game_session(std::shared_ptr<game_session> _session)
			{
				_session->set_exit();
				std::remove(sessions.begin(), sessions.end(), _session);
			}
		};

	}

}
