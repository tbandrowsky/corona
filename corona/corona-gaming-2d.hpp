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

		class bitmap_frame : public animation_frame
		{
		public:
			bitmapInstanceDto bitmap;

			bitmap_frame() {
				class_name = "bitmap_frame";
			}
            bitmap_frame(const bitmap_frame& _src) = default;
            bitmap_frame(bitmap_frame&& _src) = default;
            bitmap_frame& operator =(const bitmap_frame& _src) = default;
            bitmap_frame& operator =(bitmap_frame&& _src) = default;

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

		class vector_frame : public animation_frame
		{
		public:
			pathDto path;
            generalBrushRequest fill;
			generalBrushRequest stroke;
            double stroke_width = 1.0;

			vector_frame() {
                class_name = "vector_frame";
			}
			vector_frame(const vector_frame& _src) = default;
			vector_frame(vector_frame&& _src) = default;
			vector_frame& operator =(const vector_frame& _src) = default;
			vector_frame& operator =(vector_frame&& _src) = default;

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

		using frame_factory = corona_object_factory<animation_frame>;

		class animation : public corona_object
		{
		public:
			std::string state;
			double duration;
            rectangle destination;
			DirectX::XMVECTOR direction;
            audio_function sound;
            std::vector<std::shared_ptr<animation_frame>> frames;

			animation() {
                class_name = "animation";
			}

			animation(const animation& _src) = default;
			animation(animation&& _src) = default;
			animation& operator =(const animation& _src) = default;
			animation& operator =(animation&& _src) = default;

			virtual void get_json(json& _dest)
			{
                corona_object::get_json(_dest);
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

			virtual void put_json(frame_factory& _factory, json& _src)
			{
                corona_object::put_json(_src);
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
                        auto frame = _factory.create_object(jframe);
						frame->put_json(jframe);
						frames.push_back(frame);
					}
				}
                json jsound = _src["sound"];
                sound = audio_graph::from_json(jsound);
			}
		};

		class piece : public corona_object
		{
		public:
			std::string name;
			std::string state;
			std::vector<std::shared_ptr<animation>> animations;
			DirectX::XMVECTOR position = {};
			DirectX::XMVECTOR size = {};
			DirectX::XMVECTOR velocity = {};
			DirectX::XMVECTOR facing = {};
			DirectX::XMVECTOR acceleration = {};

			double		mass;
			double      full_hit_points;
			double      hit_points;
			std::shared_ptr<chest_field> inventory;

			piece() = default;
			piece(const piece& _src) = default;
			piece(piece&& _src) = default;
			piece& operator =(const piece& _src) = default;
			piece& operator =(piece&& _src) = default;

			virtual void get_json(json& _dest)
			{
				json_parser jp;

				corona_object::get_json(_dest);

				_dest.put_member("name", name);
				_dest.put_member("state", state);
				_dest.put_member("position", position);
				_dest.put_member("size", size);
				_dest.put_member("velocity", velocity);
				_dest.put_member("facing", facing);
				_dest.put_member("acceleration", acceleration);
				_dest.put_member("mass", mass);
				_dest.put_member("full_hit_points", full_hit_points);
				_dest.put_member("hit_points", hit_points);	

				json j = jp.create_array();
				for (auto& s : animations) {
					json jsprite = jp.create_object();
					s->get_json(jsprite);
					j.push_back(jsprite);
				}
				_dest.put_member("animations", j);

				json jinventory = jp.create_object();

				if (inventory) {
					inventory->get_json(jinventory);
				}

			}

			virtual void put_json(frame_factory& _factory, json& _src)
			{
                corona_object::put_json(_src);

				name = _src["name"].as_string();
				state = _src["state"].as_string();
				position = _src["position"].as_vector();
				size = _src["size"].as_vector();
				velocity = _src["velocity"].as_vector();
				facing = _src["facing"].as_vector();
				acceleration = _src["acceleration"].as_vector();
				mass = _src["mass"].as_double();
				hit_points = _src["hit_points"].as_double();
				full_hit_points = _src["full_hit_points"].as_double();

				if (fabs(mass) < 0.0001) {
					mass = 1.0;
				}

				json janimations = _src["animations"];
				animations.clear();
                for (int i = 0; i < janimations.size(); i++) {
					auto janimation = janimations.get_element(i);
					if (!janimation.object()) {
						continue;
					}
					auto new_animation = std::make_shared<animation>();
					new_animation->put_json(_factory, janimation);
					animations.push_back(new_animation);
				}

				inventory = std::make_shared<chest_field>();
				if (_src.has_member("inventory")) {
					json jinventory = _src["inventory"];
					inventory->put_json(jinventory);
				}
			}

			virtual void collide(collision_result& collision, std::shared_ptr<piece> _other)
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

			virtual void accelerate(DirectX::XMVECTOR _acceleration)
			{
				using namespace DirectX;
				acceleration = XMVectorAdd(acceleration, _acceleration);
			}

			virtual void displace(DirectX::XMVECTOR _displacement)
			{
				using namespace DirectX;
				velocity = XMVectorAdd(velocity, _displacement);
			}

			virtual std::shared_ptr<piece> use()
			{

			}

			void slide_piece(collision_result& collision, std::shared_ptr<piece> _other)
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

			void recoil_piece(collision_result& collision, std::shared_ptr<piece> _other)
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
			}

		};

		using piece_factory = corona_object_factory<piece>;

		class game_bus 
		{
		public:
			piece_factory piece_factory;
			frame_factory frame_factory;
		};

		class feature : public piece
		{
		public:

			feature() = default;
			feature(const feature& _src) = default;
			feature(feature&& _src) = default;
			feature& operator =(const feature& _src) = default;
			feature& operator =(feature&& _src) = default;

			virtual void get_json(json& _dest)
			{
				piece::get_json(_dest);
			}

			virtual void put_json(game_bus& _gbus, json& _src)
			{
				piece::put_json(_gbus.frame_factory, _src);
			}
		};

		class spawn : public feature
		{
		public:

			spawn() = default;
			spawn(const spawn& _src) = default;
			spawn(spawn&& _src) = default;
			spawn& operator =(const spawn& _src) = default;
			spawn& operator =(spawn&& _src) = default;

			virtual void get_json(json& _dest)
			{
				feature::get_json(_dest);
			}

			virtual void put_json(game_bus& _gbus, json& _src)
			{
				feature::put_json(_gbus, _src);
			}
		};

		class player_spawn : public spawn
		{
		public:

			player_spawn() = default;
			player_spawn(const player_spawn& _src) = default;
			player_spawn(player_spawn&& _src) = default;
			player_spawn& operator =(const player_spawn& _src) = default;
			player_spawn& operator =(player_spawn&& _src) = default;

			virtual void get_json(json& _dest)
			{
				spawn::get_json(_dest);
			}

			virtual void put_json(game_bus& _gbus, json& _src)
			{
				spawn::put_json(_gbus, _src);
			}
		};

		class npc_spawn : public spawn
		{
		public:

			scheduled_lambda<npc_spawn*> spawn_clock;
			std::vector<std::string> spawn_classes;

			npc_spawn() = default;
			npc_spawn(const npc_spawn& _src) = default;
			npc_spawn(npc_spawn&& _src) = default;
			npc_spawn& operator =(const npc_spawn& _src) = default;
			npc_spawn& operator =(npc_spawn&& _src) = default;

			virtual void get_json(json& _dest)
			{
				spawn::get_json(_dest);
				json_parser jp;
				json j = jp.from_string_container(spawn_classes);
				_dest.put_member("spawn_classes", j);
			}

			virtual void put_json(game_bus& _gbus, json& _src)
			{
				spawn::put_json(_gbus, _src);
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

		class feature : public piece
		{
		public:
			feature() = default;
			feature(const feature& _src) = default;
			feature(feature&& _src) = default;
			feature& operator =(const feature& _src) = default;
			feature& operator =(feature&& _src) = default;

			virtual void get_json(json& _dest)
			{
				piece::get_json(_dest);
			}

			virtual void put_json(game_bus& _gbus, json& _src)
			{
				piece::put_json(_gbus.frame_factory, _src);
			}
		};

		class lootbox : public feature
		{
		public:
			lootbox() = default;
			lootbox(const lootbox& _src) = default;
			lootbox(lootbox&& _src) = default;
			lootbox& operator =(const lootbox& _src) = default;
			lootbox& operator =(lootbox&& _src) = default;

			virtual void get_json(json& _dest)
			{
				feature::get_json(_dest);
			}

			virtual void put_json(game_bus& _gbus, json& _src)
			{
				feature::put_json(_gbus, _src);
			}
		};

		class lootspot : public feature
		{
		public:
			lootspot() = default;
			lootspot(const lootspot& _src) = default;
			lootspot(lootspot&& _src) = default;
			lootspot& operator =(const lootspot& _src) = default;
			lootspot& operator =(lootspot&& _src) = default;

			virtual void get_json(json& _dest)
			{
				feature::get_json(_dest);
			}

			virtual void put_json(game_bus& _gbus, json& _src)
			{
				feature::put_json(_gbus, _src);
			}
		};


		class wall : public feature
		{
		public:
			wall() = default;
			wall(const wall& _src) = default;
			wall(wall&& _src) = default;
			wall& operator =(const wall& _src) = default;
			wall& operator =(wall&& _src) = default;

			bool passable;

			virtual void get_json(json& _dest)
			{
				piece::get_json(_dest);
				_dest.put_member("passable", passable);
			}

			virtual void put_json(game_bus& _gbus, json& _src)
			{
				piece::put_json(_gbus.frame_factory, _src);
				passable = _src["passable"].as_bool();
			}
		};

		class switcher : public feature
		{
		public:
			switcher() = default;
			switcher(const switcher& _src) = default;
			switcher(switcher&& _src) = default;
			switcher& operator =(const switcher& _src) = default;
			switcher& operator =(switcher&& _src) = default;

			virtual void get_json(json& _dest)
			{
			}

			virtual void put_json(game_bus& _gbus, json& _src)
			{
				piece::put_json(_gbus.frame_factory, _src);
			}
		};

		class door : public feature
		{
		public:
			door() = default;
			door(const door& _src) = default;
			door(door&& _src) = default;
			door& operator =(const door& _src) = default;
			door& operator =(door&& _src) = default;

			virtual void get_json(json& _dest)
			{
			}

			virtual void put_json(game_bus& _gbus, json& _src)
			{
				piece::put_json(_gbus.frame_factory, _src);
			}
		};

		class surface : public feature
		{
		public:
			surface() = default;
			surface(const surface& _src) = default;
			surface(surface&& _src) = default;
			surface& operator =(const surface& _src) = default;
			surface& operator =(surface&& _src) = default;

			std::string mechanic;
			double acceleration_multiplier;
			double friction_multiplier;

			virtual void get_json(json& _dest)
			{
				_dest.put_member("mechanic", mechanic);
				_dest.put_member("acceleration_multiplier", acceleration_multiplier);
				_dest.put_member("friction_multiplier", friction_multiplier);
			}

			virtual void put_json(game_bus& _gbus, json& _src)
			{
				piece::put_json(_gbus.frame_factory, _src);
				mechanic = _src["mechanic"].as_string();
				acceleration_multiplier = _src["acceleration_multiplier"].as_double();
				friction_multiplier = _src["friction_multiplier"].as_double();
			}
		};

		class decoration : public feature
		{
		public:
			decoration() = default;
			decoration(const decoration& _src) = default;
			decoration(decoration&& _src) = default;
			decoration& operator =(const decoration& _src) = default;
			decoration& operator =(decoration&& _src) = default;

			virtual void get_json(json& _dest)
			{

			}

			virtual void put_json(game_bus& _gbus, json& _src)
			{
				piece::put_json(_gbus.frame_factory, _src);
			}
		};

		class light : public feature
		{
		public:

			light() = default;
			light(const light& _src) = default;
			light(light&& _src) = default;
			light& operator =(const light& _src) = default;
			light& operator =(light&& _src) = default;

			bool is_on = true;

			virtual void get_json(json& _dest)
			{
				piece::get_json(_dest);
                _dest.put_member_bool("is_on", is_on);
			}

			virtual void put_json(game_bus& _gbus, json& _src)
			{
				piece::put_json(_gbus.frame_factory, _src);
                is_on = _src["is_on"].as_bool();
			}

		};

		class spot_light : public light
		{
		public:

			spot_light() = default;
			spot_light(const spot_light& _src) = default;
			spot_light(spot_light&& _src) = default;
			spot_light& operator =(const spot_light& _src) = default;
			spot_light& operator =(spot_light&& _src) = default;

			virtual void get_json(json& _dest)
			{
				light::get_json(_dest);
			}

			virtual void put_json(game_bus& _gbus, json& _src)
			{
				light::put_json(_gbus.frame_factory, _src);
			}

		};

		class globe_light : public light
		{
		public:

			globe_light() = default;
			globe_light(const globe_light& _src) = default;
			globe_light(globe_light&& _src) = default;
			globe_light& operator =(const globe_light& _src) = default;
			globe_light& operator =(globe_light&& _src) = default;

			virtual void get_json(json& _dest)
			{
				feature::get_json(_dest);
			}

			virtual void put_json(game_bus& _gbus, json& _src)
			{
				feature::put_json(_gbus, _src);
			}
		};

		class camera : public feature
		{
		public:
			
			camera() = default;
			camera(const camera& _src) = default;
			camera(camera&& _src) = default;
			camera& operator =(const camera& _src) = default;
			camera& operator =(camera&& _src) = default;

			virtual void get_json(json& _dest)
			{
				camera::get_json(_dest);
			}

			virtual void put_json(game_bus& _gbus, json& _src)
			{
				camera::put_json(_gbus, _src);
			}
		};

		class carryable : public piece
		{
		public:

			carryable() = default;
			carryable(const carryable& _src) = default;
			carryable(carryable&& _src) = default;
			carryable& operator =(const carryable& _src) = default;
			carryable& operator =(carryable&& _src) = default;

			virtual void get_json(json& _dest)
			{
				corona_object::get_json(_dest);
			}

			virtual void put_json(game_bus& _gbus, json& _src)
			{
				corona_object::put_json(_src);
			}
		};

		class tool : public carryable
		{
		public:

			tool() = default;
			tool(const tool& _src) = default;
			tool(tool&& _src) = default;
			tool& operator =(const tool& _src) = default;
			tool& operator =(tool&& _src) = default;

			virtual void get_json(json& _dest)
			{
				carryable::get_json(_dest);
			}

			virtual void put_json(game_bus& _gbus, json& _src)
			{
				carryable::put_json(_gbus, _src);
			}
		};

		class consumable : public carryable
		{
		public:

			consumable() = default;
			consumable(const consumable& _src) = default;
			consumable(consumable&& _src) = default;
			consumable& operator =(const consumable& _src) = default;
			consumable& operator =(consumable&& _src) = default;

			virtual void get_json(json& _dest)
			{
				carryable::get_json(_dest);
			}

			virtual void put_json(game_bus& _gbus, json& _src)
			{
				carryable::put_json(_gbus, _src);
			}
		};

		class firearm : public tool
		{
		public:

			firearm() = default;
			firearm(const firearm& _src) = default;
			firearm(firearm&& _src) = default;
			firearm& operator =(const firearm& _src) = default;
			firearm& operator =(firearm&& _src) = default;

			virtual void get_json(json& _dest)
			{
				tool::get_json(_dest);
			}

			virtual void put_json(game_bus& _gbus, json& _src)
			{
				tool::put_json(_gbus, _src);
			}
		};

		class magazine : public carryable
		{
		public:

			magazine() = default;
			magazine(const magazine& _src) = default;
			magazine(magazine&& _src) = default;
			magazine& operator =(const magazine& _src) = default;
			magazine& operator =(magazine&& _src) = default;

			virtual void get_json(json& _dest)
			{
				carryable::get_json(_dest);
			}

			virtual void put_json(game_bus& _gbus, json& _src)
			{
				carryable::put_json(_gbus, _src);
			}
		};

		class ammunition : public carryable
		{
		public:

			ammunition() = default;
			ammunition(const ammunition& _src) = default;
			ammunition(ammunition&& _src) = default;
			ammunition& operator =(const ammunition& _src) = default;
			ammunition& operator =(ammunition&& _src) = default;

			virtual void get_json(json& _dest)
			{
				carryable::get_json(_dest);
			}

			virtual void put_json(game_bus& _gbus, json& _src)
			{
				carryable::put_json(_gbus, _src);
			}
		};

		class shot : public piece
		{

		public:

			shot() = default;
			shot(const shot& _src) = default;
			shot(shot&& _src) = default;
			shot& operator =(const shot& _src) = default;
			shot& operator =(shot&& _src) = default;

			virtual void get_json(json& _dest)
			{
				piece::get_json(_dest);
			}

			virtual void put_json(game_bus& _gbus, json& _src)
			{
                piece::put_json(_gbus.frame_factory, _src);
			}
		};

		class wand : public tool
		{
		public:

			wand() = default;
			wand(const wand& _src) = default;
			wand(wand&& _src) = default;
			wand& operator =(const wand& _src) = default;
			wand& operator =(wand&& _src) = default;

			virtual void get_json(json& _dest)
			{
				tool::get_json(_dest);
			}

			virtual void put_json(game_bus& _gbus, json& _src)
			{
				tool::put_json(_gbus, _src);
			}
		};

		class spell : public carryable
		{
		public:

			spell() = default;
			spell(const spell& _src) = default;
			spell(spell&& _src) = default;
			spell& operator =(const spell& _src) = default;
			spell& operator =(spell&& _src) = default;

			virtual void get_json(json& _dest)
			{
				carryable::get_json(_dest);
			}

			virtual void put_json(game_bus& _gbus, json& _src)
			{
				carryable::put_json(_gbus, _src);
			}
		};

		class stick : public tool
		{
		public:

			stick() = default;
			stick(const stick& _src) = default;
			stick(stick&& _src) = default;
			stick& operator =(const stick& _src) = default;
			stick& operator =(stick&& _src) = default;

			virtual void get_json(json& _dest)
			{
				tool::get_json(_dest);
			}

			virtual void put_json(game_bus& _gbus, json& _src)
			{
				tool::put_json(_gbus, _src);
			}
		};

		class artifact : public tool
		{
		public:

			artifact() = default;
			artifact(const artifact& _src) = default;
			artifact(artifact&& _src) = default;
			artifact& operator =(const artifact& _src) = default;
			artifact& operator =(artifact&& _src) = default;

			virtual void get_json(json& _dest)
			{
				tool::get_json(_dest);
			}

			virtual void put_json(game_bus& _gbus, json& _src)
			{
				tool::put_json(_gbus, _src);
			}
		};

		class actor : public piece
		{
		public:

			actor() = default;
			actor(const actor& _src) = default;
			actor(actor&& _src) = default;
			actor& operator =(const actor& _src) = default;
			actor& operator =(actor&& _src) = default;

			std::string								input_device;
			std::shared_ptr<selection_field>		selection;
			bool									ready;
			bool									dead;

			virtual void get_json(json& _dest)
			{
				piece::get_json(_dest);
				_dest.put_member("input_device", input_device);
				_dest.put_member_bool("ready", ready);
				_dest.put_member_bool("dead", dead);
			}

			virtual void put_json(game_bus& _gbus, json& _src)
			{
				piece::put_json(_gbus.frame_factory, _src);
				input_device = _src["input_device"].as_int();
				ready = _src["ready"].as_bool();
				dead = _src["dead"].as_bool();
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


		class player : public actor
		{
		public:
			player() = default;
			player(const player& _src) = default;
			player(player&& _src) = default;
			player& operator =(const player& _src) = default;
			player& operator =(player&& _src) = default;

			virtual void get_json(json& _dest)
			{
				actor::get_json(_dest);
			}

			virtual void put_json(game_bus& _gbus, json& _src)
			{
				actor::put_json(_gbus, _src);
			}
		};

		class npc : public actor
		{
		public:
			npc() = default;
			npc(const npc& _src) = default;
			npc(npc&& _src) = default;
			npc& operator =(const npc& _src) = default;
			npc& operator =(npc&& _src) = default;

			virtual void get_json(json& _dest)
			{
				actor::get_json(_dest);
			}

			virtual void put_json(game_bus& _gbus, json& _src)
			{
				actor::put_json(_gbus, _src);
			}
		};


		class map
		{
		public:

			map() = default;
			map(const map& _src) = default;
			map(map&& _src) = default;
			map& operator =(const map& _src) = default;
			map& operator =(map&& _src) = default;

			std::string class_name;
			int64_t object_id;
			std::string name;

			std::vector<std::shared_ptr<piece>> pieces;

			// C++20 view functions - no separate storage needed
			template<std::derived_from<piece> T>
			auto get_pieces_of_type() const {
				return pieces 
					| std::views::transform([](const auto& p) { 
						return std::dynamic_pointer_cast<T>(p); 
					  })
					| std::views::filter([](const auto& p) { return p != nullptr; });
			}

			// Convenience accessors for common types
			auto players() const { return get_pieces_of_type<player>(); }
			auto npcs() const { return get_pieces_of_type<npc>(); }
			auto walls() const { return get_pieces_of_type<wall>(); }
			auto player_spawns() const { return get_pieces_of_type<player_spawn>(); }
			auto npc_spawns() const { return get_pieces_of_type<npc_spawn>(); }
			auto lights() const { return get_pieces_of_type<light>(); }
			auto switches() const { return get_pieces_of_type<switcher>(); }
			auto lootboxes() const { return get_pieces_of_type<lootbox>(); }
			auto lootspots() const { return get_pieces_of_type<lootspot>(); }
			auto doors() const { return get_pieces_of_type<door>(); }
			auto decorations() const { return get_pieces_of_type<decoration>(); }
			auto shots() const { return get_pieces_of_type<shot>(); }
			auto surfaces() const { return get_pieces_of_type<surface>(); }

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

		enum class world_state
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
			std::shared_ptr<piece> piece_1;
			std::shared_ptr<piece> piece_2;
			collision_result collision;
		};

		class state_event
		{
		public:
			world_state state;

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

		class game : public job, public corona_object
		{

		public:

			game()
			{
				;
			}

			game(std::shared_ptr<comm_bus_app_interface> _bus, json& _src)
			{
				put_json(_src);
			}

			std::shared_ptr<comm_bus_app_interface> bus;
			std::string			name;
			std::string			description;
			timer				frame_timer;
			lockable			map_locker;
			std::map<world_state, std::shared_ptr<state_event>> event_handlers;
			std::shared_ptr<map> map;
			std::shared_ptr<state_event> handlers;
			world_state state;

			void set_lobby()
			{
				// Remove all players from the pieces collection
				auto is_player = [](const auto& piece) {
					return std::dynamic_pointer_cast<player>(piece) != nullptr;
					};
				map->pieces.erase(
					std::remove_if(map->pieces.begin(), map->pieces.end(), is_player),
					map->pieces.end()
				);
				state = world_state::lobby;
                handlers = event_handlers[state];
			}

			void set_active()
			{
				state = world_state::active;
				handlers = event_handlers[state];
			}

			void set_paused()
			{
				state = world_state::paused;
				handlers = event_handlers[state];
			}

			void set_complete()
			{
				state = world_state::complete;
				handlers = event_handlers[state];
			}

			void set_exit()
			{
				state = world_state::exit;
				handlers = event_handlers[state];
			}

			void start_play(std::string input_name)
			{
				auto player = attach_player(input_name);

				if (state == world_state::lobby) {
					player->ready = !player->ready;
				}
				else if (state == world_state::active) {
					player->dead = false;
					set_paused();
				}
				else if (state == world_state::paused) {
					set_active();
				}
				else if (state == world_state::complete) {
					set_lobby();
				}
				else if (state == world_state::exit) {
					; // do nothing, we're exiting anyway
				}
			}

			void check_all_ready()
			{
				if (state == world_state::lobby) {
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

			corona_client_response accelerate(std::string input_name, DirectX::XMVECTOR a)
			{
				corona_client_response response;
				auto player = attach_player(input_name);
				if (player) {
					response.success = true;
					player->accelerate(a);
				}
				return response;
			}

			corona_client_response velocity(std::string input_name, DirectX::XMVECTOR v)
			{
				corona_client_response response;
				auto player = attach_player(input_name);
				if (player) {
					response.success = true;
					player->velocity(a);
				}
				return response;
			}

			corona_client_response clear_selection(std::string input_name)
			{
				corona_client_response response;
				auto player = attach_player(input_name);
				if (player) {
					response.success = true;
					player->accelerate(a);
				}
				return response;
			}
			corona_client_response clear_selection(std::string input_name)
			{

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

			corona_client_response purchase_pieces(std::string input_name, json _for_sale, json _price)
			{

			}

			void handle_gamepad_button_up(gamepad_button_up_event gpbd)
			{
				auto player = attach_player(gpbd.state); 
				switch (gpbd.button)
				{
				case gamepad_button::A:
					break;
				case gamepad_button::B:
					break;
				case gamepad_button::X:  // It's good to learn to not hit the X button.
					break;
				case gamepad_button::Y:
					break;
				case gamepad_button::LeftShoulder:
					break;
				case gamepad_button::RightShoulder:
					break;
				case gamepad_button::Back:
					break;
				case gamepad_button::Start:
					break;
				case gamepad_button::DpadUp:
					break;
				case gamepad_button::DpadDown:
					break;
				case gamepad_button::DpadLeft:
					break;
				case gamepad_button::DpadRight:
					break;
				}
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
					break;
				case gamepad_button::Y:
					break;
				case gamepad_button::LeftShoulder:
					break;
				case gamepad_button::RightShoulder:
					break;
				case gamepad_button::Back:
					break;
				case gamepad_button::Start:
					break;
				case gamepad_button::DpadUp:
					break;
				case gamepad_button::DpadDown:
					break;
				case gamepad_button::DpadLeft:
					break;
				case gamepad_button::DpadRight:
					break;
				}
			}

			void handle_gamepad_trigger_up(gamepad_trigger_up_event gptu) 
			{
				auto player = attach_player(gptu.state);
			}

			void handle_gamepad_trigger_down(gamepad_trigger_down_event gptd) 
			{
				auto player = attach_player(gptd.state);
			}

			void handle_gamepad_thumbstick_move(gamepad_thumbstick_move_event gpbd) 
			{
				auto player = attach_player(gpbd.state);
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
				if (state != world_state::exit)
				{
					return this;
				}
				else
				{
					return nullptr;
				}
			}

		private:


			double last_elapsed_seconds;

			DirectX::XMVECTOR zero_vector = {};

			std::shared_ptr<player> attach_player(std::string input_name)
			{
				// Search for existing player with this input device
				for (auto player : map->players()) {
					if (player->input_device == input_name) {
						return player;
					}
				}
				// Create new player and add to pieces only
				std::shared_ptr<player> new_player = std::make_shared<player>();
				new_player->name = input_name;
				new_player->ready = false;
				map->pieces.push_back(new_player);
				return new_player;
			}

			std::shared_ptr<player> attach_player(XINPUT_STATE& _input_state)
			{
                return attach_player(std::to_string(_input_state.dwPacketNumber));
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

		class engine
		{
			std::shared_ptr<comm_bus_app_interface> bus;
			corona_instance instance = corona_instance::local;
			std::vector<std::shared_ptr<game>> games;

		public:

			engine(std::shared_ptr<comm_bus_app_interface> _db) : bus(_db)
			{
			}


			std::shared_ptr<game> new_game(json _game_key)
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

			std::shared_ptr<game> load_game(json _session_key)
			{
				json_parser jp;
				auto result = bus->get_object(instance, _session_key);
				if (result.success) {
					std::shared_ptr<session> session = std::make_shared<session>();
					session->put_json(result.data);
					sessions.push_back(session);
					return session;
				}
				return nullptr;
			}

			void save_game(std::shared_ptr<session> _session)
			{
				json_parser jp;
				json jsession = jp.create_object();
				_session->get_json(jsession);
				bus->put_object(instance, jsession);
			}

			void close_game(std::shared_ptr<session> _session)
			{
				_session->set_exit();
				std::remove(sessions.begin(), sessions.end(), _session);
			}
		};

	}

}
