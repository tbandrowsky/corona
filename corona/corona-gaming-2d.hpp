#pragma once
#include <algorithm>

namespace corona
{

	// corona_instance here is a placeholder,
	// eventually we want to define a game whose various pieces can 
    // come from either a local corona, or a remote corona, and the game engine will handle the differences.
	// or, we say all access is through local corona, so that, they can be stitched into a fabric.
	namespace game
	{
		class game;

		template <typename T> class item_holder
		{
		public:
			chest_item container;
            std::shared_ptr<T> item_object;

			operator bool() const {
				return item_object != nullptr;
            }
		};

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

			activity() {
				class_name = "activity";
			}
			activity(const activity& _src) = default;
			activity(activity&& _src) = default;
			activity& operator =(const activity& _src) = default;
			activity& operator =(activity&& _src) = default;
		};

		class essay : public activity
		{
		public:
			std::string         text;
		};

		class frame : public corona_object
		{
		public:
			std::string			state;
			double				duration;
			double				time_point;
			rectangle			draw_rectangle;
			DirectX::XMVECTOR	fire_point;
            audio_function      sound;

			frame() {
				class_name = "frame";
			}
			frame(const frame& _src) = default;
			frame(frame&& _src) = default;
			frame& operator =(const frame& _src) = default;
			frame& operator =(frame&& _src) = default;

			virtual void get_json(json& _dest)
			{
				json_parser jp;

				corona_object::get_json(_dest);
				_dest.put_member("state", state);
				_dest.put_member("duration", duration);
				_dest.put_member("time_point", time_point);
				_dest.put_member("draw_rectangle", draw_rectangle);
				_dest.put_member("fire_point", fire_point);
			}

			virtual void put_json(json& _src)
			{
				corona_object::get_json(_src);
				state = _src["state"].as_string();
				duration = _src["duration"].as_double();
				time_point = _src["time_point"].as_double();
				draw_rectangle = _src["draw_rectangle"].as_rectangle();
				fire_point = _src["fire_point"].as_vector();
                json jsound = _src["sound"];
                sound = audio_graph::from_json(jsound);
			}

			virtual void draw(direct2dContext& _context, point _location) 
			{

			}

            virtual void create_asset(direct2dContext& _context)
			{
				;
			}
		};

		class bitmap_frame : public frame
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
				frame::get_json(_dest);
				json jbitmap = jp.create_object();
				bitmap.get_json(jbitmap);
				_dest.put_member("bitmap", jbitmap);
            }

			virtual void put_json(json& _src)
			{
				frame::put_json(_src);
				json jbitmap = _src["bitmap"];
				bitmap.put_json(jbitmap);
            }

			virtual void draw(direct2dContext& _context, point _location)
			{
				bitmap.x = _location.x + draw_rectangle.x;
				bitmap.y = _location.y + draw_rectangle.y;
                if (draw_rectangle.w > 0 && draw_rectangle.h > 0) {
					bitmap.width = draw_rectangle.w;
					bitmap.height = draw_rectangle.h;
				}
				_context.drawBitmap(&bitmap);
			}

			virtual void create_asset(direct2dContext& _context)
			{

                if (_context.hasBitmap(bitmap.bitmapName)) {
					return;
				}

				if (bitmap.bitmapPath.size() == 0) {
                    bus->log_warning("bitmap_frame::create_asset: bitmapPath is empty, cannot create asset");
					return;
				}

				bitmapRequest request = {};
				request.filename = bitmap.bitmapPath;
				request.name = bitmap.bitmapName;
				request.cropEnabled = false;
				point pt = { bitmap.width, bitmap.height };
				request.sizes.push_back(pt);
			}
		};

		class vector_frame : public frame
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
				frame::get_json(_dest);
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
				frame::put_json(_src);
				json jpath = _src["path"];
				path.put_json(jpath);
				json jfill = _src["fill"];
				fill.put_json(jfill);
				json jstroke = _src["stroke"];
				stroke.put_json(jstroke);
				stroke_width = _src["stroke_width"].as_double();
			}

			virtual void draw(direct2dContext& _context, point _location)
			{
				pathImmediateDto pid;
                pid.position.x = _location.x + draw_rectangle.x;
                pid.position.y = _location.y + draw_rectangle.y;
                pid.path = path;
                pid.fillBrushName = fill.get_name();
				pid.borderBrushName = stroke.get_name();
				pid.rotation = 0;
                pid.strokeWidth = stroke_width;
                _context.drawPath(&pid);
			}

			virtual void create_asset(direct2dContext& _context)
			{
				_context.setBrush(&fill);
				_context.setBrush(&stroke);
			}


		};

		using frame_factory = corona_object_factory<frame>;

		class animation : public corona_object
		{
		public:
			std::string										state;
			double											duration;
            rectangle										draw_rectangle;
			DirectX::XMVECTOR								direction;
            audio_function									sound;
            std::vector<std::shared_ptr<frame>>	frames;
			animation_scheduler								schedule;
            int												current_frame_index;

			animation() {
                class_name = "animation";
				current_frame_index = 0;
			}

			animation(const animation& _src) = default;
			animation(animation&& _src) = default;
			animation& operator =(const animation& _src) = default;
			animation& operator =(animation&& _src) = default;


			virtual void draw(direct2dContext& _context, double _elapsed, point _location)
			{
				if (frames.size() == 0)
					return;

				int index = schedule.execute(_elapsed);

				index = index % frames.size();

				if (current_frame_index != index)
				{
					current_frame_index = index;
					bus->play_audio(sound, 1.0, -1.0);
				}

				auto frame = frames[index];

				frame->draw(_context, _location);
			}

			virtual void put_json(frame_factory& _factory, json& _src)
			{
				corona_object::put_json(_src);
				state = _src["state"].as_string();
				duration = _src["duration"].as_double();
				draw_rectangle = _src["draw_rectangle"].as_rectangle();
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

			virtual void get_json(json& _dest)
			{
				corona_object::get_json(_dest);
				_dest.put_member("state", state);
				_dest.put_member("duration", duration);
				_dest.put_member("draw_rectangle", draw_rectangle);
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

		};

		class piece_type : public corona_object
		{

		public:
			piece_type() {
				class_name = "piece";
			}
			piece_type(const piece_type& _src) = default;
			piece_type(piece_type&& _src) = default;
			piece_type& operator =(const piece_type& _src) = default;
			piece_type& operator =(piece_type&& _src) = default;

			std::string name;
			std::string description;
			std::string piece_class_name;
			std::vector<std::shared_ptr<animation>> animations;

			double		mass = 1.0;
			double      full_health = 1.0;
			double      health = 1.0;

			std::string state;

			std::shared_ptr<chest_field> inventory;

			virtual void get_json(json& _dest)
			{
				json_parser jp;

				corona_object::get_json(_dest);

				_dest.put_member("name", name);
				_dest.put_member("state", state);
				_dest.put_member("mass", mass);

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
				mass = _src["mass"].as_double();

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

			virtual void draw(direct2dContext& _context, double _elapsed, point _location)
			{
				for (auto animation : animations) {
					if (animation->state == state) {
						animation->draw(_context, _elapsed, _location);
					}
				}
			}

		};

		class piece_state
		{
		public:
			std::string state;
			double state_duration;
		};

		class piece : public corona_object
		{

		public:
			piece() {
				class_name = "piece";
				remove = false;
			}
			piece(const piece& _src) = default;
			piece(piece&& _src) = default;
			piece& operator =(const piece& _src) = default;
			piece& operator =(piece&& _src) = default;

			std::string name;
			std::string state;
			std::vector<std::shared_ptr<animation>> animations;
			DirectX::XMVECTOR position = {};
			DirectX::XMVECTOR size = {};
			DirectX::XMVECTOR velocity = {};
			DirectX::XMVECTOR facing = {};
			DirectX::XMVECTOR acceleration = {};

			std::shared_ptr<chest_field> inventory;
			object_reference owner;

			double		mass = 1.0;
			double      full_health = 1.0;
			double      health = 1.0;
			bool		remove = false;
			double		state_time = 0.0;



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
				_dest.put_member("mass", mass);
				_dest.put_member("health", health);
				_dest.put_member("full_health", full_health);
                _dest.put_member("owner", owner);

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

				remove = false;
				name = _src["name"].as_string();
				state = _src["state"].as_string();
				position = _src["position"].as_vector();
				size = _src["size"].as_vector();
				velocity = _src["velocity"].as_vector();
				facing = _src["facing"].as_vector();
				acceleration = DirectX::XMVectorZero();
				mass = _src["mass"].as_double();
				owner = _src["owner"].as_object_reference();
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

			virtual void draw(direct2dContext& _context, double _elapsed, point _location)
			{
                for (auto animation : animations) {
					if (animation->state == state) {
						animation->draw(_context, _elapsed, _location);
					}
				}
			}


			rectangle get_rectangle(double _elapsed) const
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

			virtual void move(double _elapsed)
			{
                velocity = DirectX::XMVectorAdd(velocity, DirectX::XMVectorScale(acceleration, static_cast<float>(_elapsed)));
                position = DirectX::XMVectorAdd(position, DirectX::XMVectorScale(velocity, static_cast<float>(_elapsed)));
			}

			// this gets called on every piece, to apply accelerations, make animation calculations, 
            // and other time-based calculations. The default implementation does nothing.
            // _elapsed_seconds is the time since the last call to run, in seconds.
			// this will be quite fractional.
			virtual void run(game* _game, piece *_user, double _elapsed);

			template <typename T>
			std::shared_ptr<T> get_item_ptr(game* _game, std::string _class_name, std::string _type_name)
			{
				std::shared_ptr<T> holder;
				if (inventory) {
					auto item = inventory->match(_class_name, _type_name);
					if (item) {
						holder = _game->factories.piece_factory(_game, item);
						return std::dynamic_pointer_cast<T>(item);
					}
				}
				return holder;
			}

            template <typename T>
			item_holder<T> get_item(game* _game, std::string _class_name, std::string _type_name)
			{
				item_holder<T> holder;
				if (inventory) {
                    auto item = inventory->match(_class_name, _type_name);
                    if (item) {
						holder.container = item;
                        holder.item_object = _game->factories.piece_factory(_game, item);
                        return std::dynamic_pointer_cast<T>(item);
                    }
                }
				return holder;
			}

			template <typename T>
			std::shared_ptr<T> get_item_ptr(game* _game, chest_item& _src)
			{
				std::shared_ptr<T> holder;
				if (inventory) {
					auto item = inventory->find(&_src);
					if (item) {
						holder = _game->factories.piece_factory.get_object<T>(item->reference, false);
					}
				}
				return holder;
			}

			template <typename T>
			item_holder<T> get_item(game* _game, chest_item& _src)
			{
				item_holder<T> holder;
				if (inventory) {
					auto item = inventory->find(&_src);
					if (item) {
                        holder.container = *item;
						holder.item_object = _game->factories.piece_factory.get_object<T>(item->reference, false);
					}
				}
				return holder;
			}

			bool remove_item(game* _game, corona_object* _src)
			{
				if (inventory) {
                    auto _src_ref = _src->to_reference();
					auto item = inventory->find(_src_ref);
					if (item) {
						inventory->remove_part(*item);
						return true;
					}
				}
				return false;
			}

			virtual void use(game* _game, piece* _piece);

		};

		using piece_factory = corona_object_factory<piece>;

        using general_factory = corona_object_factory<corona_object>;

		class collision_command_key
		{
		public:
			std::string moving_piece_class;
			std::string impact_piece_class;

			bool operator < (const collision_command_key& _other) const
			{
                return std::tie(moving_piece_class, impact_piece_class) < std::tie(_other.moving_piece_class, _other.impact_piece_class);
			}
		};

		class collision_command
		{
		public:

			collision_command() = default;
            collision_command(const collision_command& _src) = default;
            collision_command(collision_command&& _src) = default;
			collision_command& operator =(const collision_command& _src) = default;
            collision_command& operator =(collision_command&& _src) = default;

			virtual void run(game* _game, piece* _moving_piece, piece* _impact_piece)
			{

			}
		};

		class hit_collision_command : public collision_command
		{
		public:

			virtual void run(game* _game, piece* _moving_piece, piece* _impact_piece)
			{
				auto hit_piece = dynamic_cast<hit *>(_moving_piece);
				_game->factories.clone_piece(_impact_piece, _moving_piece, "effect", hit_piece->effect_type, 1);
				hit_piece->remove = true;
			}
		};

		class shot_collision_command: public collision_command
		{
		public:
			virtual void run(game* _game, piece* _moving_piece, piece* _impact_piece)
			{
				auto hit_piece = dynamic_cast<shot*>(_moving_piece);
				_game->factories.clone_piece(_impact_piece, _moving_piece, "effect", hit_piece->effect_type, 1);
				hit_piece->remove = true;
			}
        };

		class spell_collision_command : public collision_command
		{
		public:
			virtual void run(game* _game, piece* _moving_piece, piece* _impact_piece)
			{
				auto hit_piece = dynamic_cast<spell*>(_moving_piece);
				_game->factories.clone_piece(_impact_piece, _moving_piece, "effect", hit_piece->effect_type, 1);
				hit_piece->remove = true;
			}
		};

		class wall_collision_command : public collision_command
		{
		public:
			virtual void run(game* _game, piece* _moving_piece, piece* _impact_piece)
			{

			}
		};

		class surface_collision_command : public collision_command
		{
		public:
			virtual void run(game* _game, piece* _moving_piece, piece* _impact_piece)
			{

			}
		};

		class feature_collision_command : public collision_command
		{
		public:
			virtual void run(game* _game, piece* _moving_piece, piece* _impact_piece)
			{

			}
		};

		class collision_commands
		{
		public:

			collision_commands() = default;
			collision_commands(const collision_commands& _src) = default;
			collision_commands(collision_commands&& _src) = default;
			collision_commands& operator =(const collision_commands& _src) = default;
			collision_commands& operator =(collision_commands&& _src) = default;

            std::map<collision_command_key, std::shared_ptr<collision_command>> commands;

			void register_command(std::string _moving_piece_class, std::string _impact_piece_class, std::shared_ptr<collision_command> _command)
			{
				collision_command_key key;
				key.moving_piece_class = _moving_piece_class;
				key.impact_piece_class = _impact_piece_class;
				commands[key] = _command;
            }

		};

		class game_factory
		{
			comm_bus_app_interface* bus;

		public:
			piece_factory piece_factory;
			frame_factory frame_factory;
			general_factory general_factory;

			using object_action = std::function<void(game* _game, piece* _direct_object, piece* _actor)>;
			using object_interaction = std::function<void(game* _game, piece* _a, piece* _b)>;

			game_factory(comm_bus_app_interface* _bus) noexcept : bus(_bus), piece_factory(_bus), frame_factory(_bus), general_factory(_bus)
			{
				;
			}

			game_factory(const game_factory& _src) = default;
			game_factory(game_factory&& _src) = default;
			game_factory& operator =(const game_factory& _src) = default;
			game_factory& operator =(game_factory&& _src) = default;

			void init(corona_instance instance)
			{
				piece_factory.init(instance);
				frame_factory.init(instance);
			}
		};

		class piece_list : public corona_object
		{
		public:
			piece_list() {
				class_name = "piece_list";
			}
			piece_list(const piece_list& _src) = default;
			piece_list(piece_list&& _src) = default;
			piece_list& operator =(const piece_list& _src) = default;
			piece_list& operator =(piece_list&& _src) = default;

			std::string state;
            std::vector<std::shared_ptr<piece>> pieces;

			virtual void get_json(json& _dest)
			{
				json_parser jp;

				corona_object::get_json(_dest);
				_dest.put_member("state", state);
				json j = jp.create_array();
				for (const auto& p : pieces) {
                    json pj = jp.create_object();
                    p->get_json(pj);
                    j.push_back(pj);
				}
				_dest.put_member("pieces", j);
			}

			virtual void put_json(game_factory& _gbus, json& _src)
			{
				corona_object::put_json(_src);
				state = _src["state"].as_string();
                json jpieces = _src["pieces"];
				pieces = _gbus.piece_factory.create_array(jpieces);
			}
		};

		class feature : public piece
		{
		public:

			feature() {
				class_name = "feature";
			}
			feature(const feature& _src) = default;
			feature(feature&& _src) = default;
			feature& operator =(const feature& _src) = default;
			feature& operator =(feature&& _src) = default;

			function_scheduler<piece> scheduler;


			virtual void get_json(json& _dest)
			{
				json_parser jp;
				piece::get_json(_dest);
				json jtimer = jp.create_object();
				scheduler.get_json(jtimer);
                _dest.put_member("scheduler", jtimer);
			}

			virtual void put_json(game_factory& _gbus, json& _src)
			{
				piece::put_json(_gbus.frame_factory, _src);
				json timer = _src["scheduler"];
				scheduler.put_json(timer);
			}

			virtual void run(game* _game, double _elapsed)
			{
				scheduler.execute(_elapsed);
			}
		};

		class effect : public piece
		{
		public:

			effect() {
				class_name = "feature";
			}

			virtual void get_json(json& _dest)
			{
				piece::get_json(_dest);
			}

			virtual void put_json(game_factory& _gbus, json& _src)
			{
				piece::put_json(_gbus.frame_factory, _src);
			}

			// this gets called on every piece, to apply accelerations, make animation calculations, 
			// and other time-based calculations. The default implementation does nothing.
			// _elapsed_seconds is the time since the last call to run, in seconds.
			// this will be quite fractional.
			virtual void run(game* _game, piece* _user, double _elapsed);
		};

		class delivery : public piece
		{
		public:

			delivery() {
				class_name = "delivery";
			}

			virtual void get_json(json& _dest)
			{
				piece::get_json(_dest);
			}

			virtual void put_json(game_factory& _gbus, json& _src)
			{
				piece::put_json(_gbus.frame_factory, _src);
			}
		};

		class spawn : public feature
		{
		public:

			spawn() {
				class_name = "spawn";
			}
			spawn(const spawn& _src) = default;
			spawn(spawn&& _src) = default;
			spawn& operator =(const spawn& _src) = default;
			spawn& operator =(spawn&& _src) = default;

			virtual void get_json(json& _dest)
			{
				feature::get_json(_dest);
			}

			virtual void put_json(game_factory& _gbus, json& _src)
			{
				feature::put_json(_gbus, _src);
			}
		};

		class player_spawn : public spawn
		{
		public:

			player_spawn() {
				class_name = "player_spawn";
			}
			player_spawn(const player_spawn& _src) = default;
			player_spawn(player_spawn&& _src) = default;
			player_spawn& operator =(const player_spawn& _src) = default;
			player_spawn& operator =(player_spawn&& _src) = default;

			virtual void get_json(json& _dest)
			{
				spawn::get_json(_dest);
			}

			virtual void put_json(game_factory& _gbus, json& _src)
			{
				spawn::put_json(_gbus, _src);
			}
		};

		class npc_spawn : public spawn
		{
		public:

			function_scheduler<npc_spawn> scheduler;
			std::vector<std::string> spawn_classes;

			npc_spawn() {
				class_name = "npc_spawn";
			}
			npc_spawn(const npc_spawn& _src) = default;
			npc_spawn(npc_spawn&& _src) = default;
			npc_spawn& operator =(const npc_spawn& _src) = default;
			npc_spawn& operator =(npc_spawn&& _src) = default;

			virtual void get_json(json& _dest)
			{
				spawn::get_json(_dest);
				json_parser jp;
				json j = jp.vector_to_jarray(spawn_classes);
				_dest.put_member("spawn_classes", j);
                json jscheduler = jp.create_object();
				scheduler.get_json(jscheduler);
                _dest.put_member("task_scheduler", jscheduler);
			}

			virtual void put_json(game_factory& _gbus, json& _src)
			{
				spawn::put_json(_gbus, _src);
				json jsc = _src["spawn_classes"];
				json jtimer = _src["task_scheduler"];
                scheduler.put_json(jtimer);
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

            virtual void spawn(game* _game)
			{
				std::string spawn_class = get_spawn_class();
				if (spawn_class.size() > 0) {

					auto new_piece = _game->create_piece_of_type(spawn_class);
					if (new_piece) {
						new_piece->position = this->position;
                        new_piece->facing = this->facing;
						_game->game_map->pieces.push_back(new_piece);
					}
				}
			}

			virtual void run(game* _game, double _elapsed_seconds)
			{
                if (scheduler.execute(_elapsed_seconds)) {
					spawn(_game);
				}
			}

		};


		class lootbox : public feature
		{
		public:
			lootbox() {
				class_name = "loot_box";
			}
			lootbox(const lootbox& _src) = default;
			lootbox(lootbox&& _src) = default;
			lootbox& operator =(const lootbox& _src) = default;
			lootbox& operator =(lootbox&& _src) = default;

			virtual void get_json(json& _dest)
			{
				feature::get_json(_dest);
			}

			virtual void put_json(game_factory& _gbus, json& _src)
			{
				feature::put_json(_gbus, _src);
			}

		};

		class lootspot : public feature
		{
		public:
			lootspot() {
				class_name = "loot_spot";
			}
			lootspot(const lootspot& _src) = default;
			lootspot(lootspot&& _src) = default;
			lootspot& operator =(const lootspot& _src) = default;
			lootspot& operator =(lootspot&& _src) = default;

			virtual void get_json(json& _dest)
			{
				feature::get_json(_dest);
			}

			virtual void put_json(game_factory& _gbus, json& _src)
			{
				feature::put_json(_gbus, _src);
			}

		};


		class wall : public feature
		{
		public:
			wall() {
				class_name = "wall";
			}
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

			virtual void put_json(game_factory& _gbus, json& _src)
			{
				piece::put_json(_gbus.frame_factory, _src);
				passable = _src["passable"].as_bool();
			}
		};


		class switcher : public feature
		{
		public:

			switcher() {
				class_name = "switcher";
			}
			switcher(const switcher& _src) = default;
			switcher(switcher&& _src) = default;
			switcher& operator =(const switcher& _src) = default;
			switcher& operator =(switcher&& _src) = default;

			std::string state;
            std::vector<std::shared_ptr<piece_list>> sets;

			virtual void get_json(json& _dest)
			{
				feature::get_json(_dest);
			}

			virtual void put_json(game_factory& _gbus, json& _src)
			{
				feature::put_json(_gbus, _src);

                json jsets = _src["sets"];

				for (int i = 0; i < jsets.size(); i++) 
				{
                    json jset = jsets.get_element(i);
                    auto set = std::make_shared<piece_list>();
                    set->put_json(_gbus, jset);

                    sets.push_back(set);
                }
			}

			// the user uses the piece, which may change its state or cause it to be consumed. The default implementation does nothing and returns a copy of the piece.
			virtual void use(game* _game, piece* _piece)
			{
                if (state.empty()) {
					if (sets.size() > 0) {
						state = sets[0]->state;
					}
				}
				else {
					for (int i = 0; i < sets.size(); i++) {
						if (sets[i]->state == state) {
							int next_index = i;
							do {
								next_index = (i + 1) % sets.size();
                            } while (next_index != i && sets[next_index]->state == state);
							state = sets[next_index]->state;
							break;
						}
					}
				}
			}
		};

		class door : public feature
		{
		public:
			door() {
				class_name = "door";
				state = "closed";
			}
			door(const door& _src) = default;
			door(door&& _src) = default;
			door& operator =(const door& _src) = default;
			door& operator =(door&& _src) = default;

			virtual void get_json(json& _dest)
			{
                feature::get_json(_dest);
			}

			virtual void put_json(game_factory& _gbus, json& _src)
			{
				feature::put_json(_gbus, _src);
			}

			// the player uses the piece, and then, 
			// that changes the state.
			// the animation system picks this up while rendering
			virtual void use(game* _game, piece* _piece)
			{
				if (state == "open") {
					state = "closed";
				}
				else {
					state = "open";
				}
			}

		};

		class surface : public feature
		{
		public:
			surface() {
				class_name = "surface";
			}
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

			virtual void put_json(game_factory& _gbus, json& _src)
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
			decoration() {
				class_name = "decoration";
			}
			decoration(const decoration& _src) = default;
			decoration(decoration&& _src) = default;
			decoration& operator =(const decoration& _src) = default;
			decoration& operator =(decoration&& _src) = default;

			virtual void get_json(json& _dest)
			{

			}

			virtual void put_json(game_factory& _gbus, json& _src)
			{
				piece::put_json(_gbus.frame_factory, _src);
			}
		};

		class light : public feature
		{
		public:

			light() {
				class_name = "light";
			}
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

			virtual void put_json(game_factory& _gbus, json& _src)
			{
				piece::put_json(_gbus.frame_factory, _src);
                is_on = _src["is_on"].as_bool();
			}

			// the user uses the piece, which may change its state or cause it to be consumed. The default implementation does nothing and returns a copy of the piece.
			virtual void use(game* _game, piece* _piece)
			{
                is_on = !is_on;
			}

		};

		class spot_light : public light
		{
		public:

			spot_light() {
				class_name = "spot_light";
			}
			spot_light(const spot_light& _src) = default;
			spot_light(spot_light&& _src) = default;
			spot_light& operator =(const spot_light& _src) = default;
			spot_light& operator =(spot_light&& _src) = default;

			virtual void get_json(json& _dest)
			{
				light::get_json(_dest);
			}

			virtual void put_json(game_factory& _gbus, json& _src)
			{
				light::put_json(_gbus, _src);
			}

		};

		class globe_light : public light
		{
		public:

			globe_light() {
				class_name = "globe_light";
			}
			globe_light(const globe_light& _src) = default;
			globe_light(globe_light&& _src) = default;
			globe_light& operator =(const globe_light& _src) = default;
			globe_light& operator =(globe_light&& _src) = default;

			virtual void get_json(json& _dest)
			{
				feature::get_json(_dest);
			}

			virtual void put_json(game_factory& _gbus, json& _src)
			{
				feature::put_json(_gbus, _src);
			}
		};

		class camera : public feature
		{
		public:
			
			camera() {
				class_name = "camera";
			}
			camera(const camera& _src) = default;
			camera(camera&& _src) = default;
			camera& operator =(const camera& _src) = default;
			camera& operator =(camera&& _src) = default;

			virtual void get_json(json& _dest)
			{
				camera::get_json(_dest);
			}

			virtual void put_json(game_factory& _gbus, json& _src)
			{
				camera::put_json(_gbus, _src);
			}
		};

		class carryable : public piece
		{
		public:

			carryable() {
				class_name = "carryable";
			}
			carryable(const carryable& _src) = default;
			carryable(carryable&& _src) = default;
			carryable& operator =(const carryable& _src) = default;
			carryable& operator =(carryable&& _src) = default;

			virtual void get_json(json& _dest)
			{
				corona_object::get_json(_dest);
			}

			virtual void put_json(game_factory& _gbus, json& _src)
			{
				corona_object::put_json(_src);
			}

		};

		class tool : public carryable
		{
		public:

			tool() {
				class_name = "tool";
			}
			tool(const tool& _src) = default;
			tool(tool&& _src) = default; 
			tool& operator =(const tool& _src) = default;
			tool& operator =(tool&& _src) = default;

			virtual void get_json(json& _dest)
			{
				carryable::get_json(_dest);
			}

			virtual void put_json(game_factory& _gbus, json& _src)
			{
				carryable::put_json(_gbus, _src);
			}

		};

		class consumable : public carryable
		{
		public:

			consumable() {
				class_name = "consumable";
			}
			consumable(const consumable& _src) = default;
			consumable(consumable&& _src) = default;
			consumable& operator =(const consumable& _src) = default;
			consumable& operator =(consumable&& _src) = default;

			virtual void get_json(json& _dest)
			{
				carryable::get_json(_dest);
			}

			virtual void put_json(game_factory& _gbus, json& _src)
			{
				carryable::put_json(_gbus, _src);
			}
		};

		class shot : public piece
		{

		public:

			shot() {
				class_name = "shot";
			}
			shot(const shot& _src) = default;
			shot(shot&& _src) = default;
			shot& operator =(const shot& _src) = default;
			shot& operator =(shot&& _src) = default;

			std::string shot_type;
			std::string effect_type;

			virtual void get_json(json& _dest)
			{
				piece::get_json(_dest);
				_dest.put_member("shot_type", shot_type);
				_dest.put_member("effect_type", effect_type);
			}

			virtual void put_json(game_factory& _gbus, json& _src)
			{
				piece::put_json(_gbus.frame_factory, _src);
				shot_type = _src["shot_type"].as_string();
				effect_type = _src["effect_type"].as_string();
			}
		};

		class ammunition : public carryable
		{
		public:

			ammunition() {
				class_name = "ammunition";
			}
			ammunition(const ammunition& _src) = default;
			ammunition(ammunition&& _src) = default;
			ammunition& operator =(const ammunition& _src) = default;
			ammunition& operator =(ammunition&& _src) = default;

			std::string ammunition_type;
			std::string shot_type;
			double shot_muzzle_velocity;
			double shot_mass;
			double shot_impulse;

			virtual void get_json(json& _dest)
			{
				carryable::get_json(_dest);
				_dest.put_member("ammunition_type", ammunition_type);
				_dest.put_member("shot_type", shot_type);
				_dest.put_member("shot_muzzle_velocity", shot_muzzle_velocity);
				_dest.put_member("shot_mass", shot_mass);
				_dest.put_member("shot_impulse", shot_impulse);
			}

			virtual void put_json(game_factory& _gbus, json& _src)
			{
				carryable::put_json(_gbus, _src);
				ammunition_type = _src["ammunition_type"].as_string();
				shot_type = _src["shot_type"].as_string();
				shot_muzzle_velocity = _src["shot_muzzle_velocity"].as_double();
                shot_mass = _src["shot_mass"].as_double();
                shot_impulse = _src["shot_impulse"].as_double();
			}
		};

		class magazine : public carryable
		{
		public:

			magazine() {
				class_name = "magazine";
			}
			magazine(const magazine& _src) = default;
			magazine(magazine&& _src) = default;
			magazine& operator =(const magazine& _src) = default;
			magazine& operator =(magazine&& _src) = default;

			std::string magazine_type;
			double capacity;
			std::string ammunition_type;

			virtual void get_json(json& _dest)
			{
				json_parser jp;
				carryable::get_json(_dest);
				_dest.put_member("magazine_type", magazine_type);
                _dest.put_member("ammunition_type", ammunition_type);
			}

			virtual void put_json(game_factory& _gbus, json& _src)
			{
				json_parser jp;

				carryable::put_json(_gbus, _src);
				magazine_type = _src["magazine_type"].as_string();
				ammunition_type = _src["ammunition_type"].as_string();
			}

		};

		class firearm : public tool
		{
		public:

			firearm() {
				class_name = "firearm";
			}
			firearm(const firearm& _src) = default;
			firearm(firearm&& _src) = default;
			firearm& operator =(const firearm& _src) = default;
			firearm& operator =(firearm&& _src) = default;

			DirectX::XMVECTOR fire_point;
            std::string magazine_type;

			virtual void get_json(json& _dest)
			{
				carryable::get_json(_dest);

				_dest.put_member("magazine_type", magazine_type);
				_dest.put_member("fire_point", fire_point);
			}

			virtual void put_json(game_factory& _gbus, json& _src)
			{
				json_parser jp;
				carryable::put_json(_gbus, _src);

				magazine_type = _src["magazine_type"].as_string();
                fire_point = _src["fire_point"].as_vector();
			}

			// this gets called on every piece, to apply accelerations, make animation calculations, 
			// and other time-based calculations. The default implementation does nothing.
			// _elapsed_seconds is the time since the last call to run, in seconds.
			// this will be quite fractional.
			virtual void run(game* _game, piece* _user, double _elapsed);

		};

		class wand : public tool
		{
		public:

			wand() {
				class_name = "wand";
			}
			wand(const wand& _src) = default;
			wand(wand&& _src) = default;
			wand& operator =(const wand& _src) = default;
			wand& operator =(wand&& _src) = default;

			std::string spell_type;
            DirectX::XMVECTOR fire_point;

			virtual void get_json(json& _dest)
			{
				tool::get_json(_dest);
                _dest.put_member("spell_type", spell_type);
                _dest.put_member("fire_point", fire_point);
			}

			virtual void put_json(game_factory& _gbus, json& _src)
			{
				tool::put_json(_gbus, _src);
                spell_type = _src["spell_type"].as_string();
                fire_point = _src["fire_point"].as_vector();
			}
		};

		class spell : public carryable
		{
		public:

			spell() {
				class_name = "spell";
			}
			spell(const spell& _src) = default;
			spell(spell&& _src) = default;
			spell& operator =(const spell& _src) = default;
			spell& operator =(spell&& _src) = default;

			std::string spell_type;
			std::string effect_type;
			double spell_velocity;

			virtual void get_json(json& _dest)
			{
				carryable::get_json(_dest);
				_dest.put_member("effect_type", effect_type);
                _dest.put_member("spell_velocity", spell_velocity);
                _dest.put_member("spell_type", spell_type);
			}

			virtual void put_json(game_factory& _gbus, json& _src)
			{
				carryable::put_json(_gbus, _src);
				effect_type = _src["effect_type"].as_string();
				spell_velocity = _src["spell_velocity"].as_double();
                spell_type = _src["spell_type"].as_string();
			}

			virtual void run(game* _game, piece* _user, double _elapsed)
			{

			}

		};

		class stick : public tool
		{
		public:

			stick() {
				class_name = "stick";
			}
			stick(const stick& _src) = default;
			stick(stick&& _src) = default;
			stick& operator =(const stick& _src) = default;
			stick& operator =(stick&& _src) = default;

			std::string hit_type;

			virtual void get_json(json& _dest)
			{
				tool::get_json(_dest);
				_dest.put_member("hit_type", hit_type);
			}

			virtual void put_json(game_factory& _gbus, json& _src)
			{
				tool::put_json(_gbus, _src);
				hit_type = _src["hit_type"].as_string();
			}
		};

		class hit : public piece
		{
		public:

			hit() {
				class_name = "hit";
			}
			hit(const hit& _src) = default;
			hit(hit&& _src) = default;
			hit& operator =(const hit& _src) = default;
			hit& operator =(hit&& _src) = default;

			std::string hit_type;
			std::string effect_type;
			double hit_lifetime;


			virtual void get_json(json& _dest)
			{
				piece::get_json(_dest);
				_dest.put_member("effect_type", effect_type);
				_dest.put_member("hit_type", hit_type);
                _dest.put_member("hit_lifetime", hit_lifetime);
			}

			virtual void put_json(game_factory& _gbus, json& _src)
			{
				piece::put_json(_gbus.frame_factory, _src);
				effect_type = _src["effect_type"].as_string();
				hit_type = _src["hit_type"].as_string();
                hit_lifetime = _src["hit_lifetime"].as_double();
			}

			virtual void run(game* _game, piece* _user, double _elapsed)
			{
				
                hit_lifetime -= _elapsed;
                if (hit_lifetime <= 0) {
					remove = true;
				}
			}

		};

		class artifact : public tool
		{
		public:

			artifact() {
				class_name = "artifact";
			}
			artifact(const artifact& _src) = default;
			artifact(artifact&& _src) = default;
			artifact& operator =(const artifact& _src) = default;
			artifact& operator =(artifact&& _src) = default;

			virtual void get_json(json& _dest)
			{
				tool::get_json(_dest);
			}

			virtual void put_json(game_factory& _gbus, json& _src)
			{
				tool::put_json(_gbus, _src);
			}
		};

		class actor_type : public piece_type
		{
		public:

			actor_type() {
				class_name = "actor_type";
			}
			actor_type(const actor_type& _src) = default;
			actor_type(actor_type&& _src) = default;
			actor_type& operator =(const actor_type& _src) = default;
			actor_type& operator =(actor_type&& _src) = default;

			double crawling_velocity;
			double walking_velocity;
			double running_velocity;
			double throwing_velocity;

			virtual void get_json(json& _dest)
			{
				piece_type::get_json(_dest);
				_dest.put_member("crawling_velocity", crawling_velocity);
				_dest.put_member("walking_velocity", walking_velocity);
				_dest.put_member("running_velocity", running_velocity);
				_dest.put_member("throwing_velocity", throwing_velocity);
			}

			virtual void put_json(game_factory& _gbus, json& _src)
			{
				piece_type::put_json(_gbus.frame_factory, _src);
				crawling_velocity = _src["crawling_velocity"].as_double();
				walking_velocity = _src["walking_velocity"].as_double();
				running_velocity = _src["running_velocity"].as_double();
				throwing_velocity = _src["throwing_velocity"].as_double();
			}
		};

		class actor : public piece
		{
		public:

			actor() {
				class_name = "actor";
			}
			actor(const actor& _src) = default;
			actor(actor&& _src) = default;
			actor& operator =(const actor& _src) = default;
			actor& operator =(actor&& _src) = default;

			std::string								input_device;
			std::shared_ptr<selection_field>		selection;
			bool									ready;
			bool									dead;

			double stopped_velocity;
			double crawling_velocity;
			double walking_velocity;
			double running_velocity;
			double throwing_velocity;

			virtual void get_json(json& _dest)
			{
				piece::get_json(_dest);
				_dest.put_member("input_device", input_device);
				_dest.put_member_bool("ready", ready);
				_dest.put_member_bool("dead", dead);
				_dest.put_member("stopped_velocity", stopped_velocity);
				_dest.put_member("crawling_velocity", crawling_velocity);
				_dest.put_member("walking_velocity", walking_velocity);
				_dest.put_member("running_velocity", running_velocity);
				_dest.put_member("throwing_velocity", throwing_velocity);
			}

			virtual void put_json(game_factory& _gbus, json& _src)
			{
				piece::put_json(_gbus.frame_factory, _src);
				input_device = _src["input_device"].as_int();
				ready = _src["ready"].as_bool();
				dead = _src["dead"].as_bool();
				stopped_velocity = _src["stopped_velocity"].as_double();
				crawling_velocity = _src["crawling_velocity"].as_double();
				walking_velocity = _src["walking_velocity"].as_double();
				running_velocity = _src["running_velocity"].as_double();
				throwing_velocity = _src["throwing_velocity"].as_double();
			}

			// and now, we can extend the selection and the inventory

			virtual void select_next();
			virtual void select_previous();
			virtual void extend_selection(chest_item* _ci);
			virtual void clear_selection();

			virtual void move_left()
			{
				DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
				auto v = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(up, facing));
			}

			virtual void move_right()
			{
				DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
				auto v = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(up, facing));
				v = DirectX::XMVectorNegate(v);
			}

			virtual void move_foward()
			{
				auto v = facing;
			}

			virtual void move_back()
			{
				auto v = DirectX::XMVectorNegate(facing);
			}

			virtual void move(DirectX::XMVECTOR _direction)
			{
				auto v = DirectX::XMVector3Normalize(_direction);

				if (state == "stopped") 
				{
					state = "walking";
				}
				
				if (state == "crawling")
				{
					velocity = DirectX::XMVectorScale(v, crawling_velocity);
				}
				else if (state == "walking")
				{
					velocity = DirectX::XMVectorScale(v, walking_velocity);
				}
				else if (state == "running")
				{
					velocity = DirectX::XMVectorScale(v, running_velocity);
				}
			}

			void run()
			{
				state = "running";
			}

			void walk()
			{
				state = "walking";
			}

			void crawl()
			{
				state = "crawling";
			}

			void stop()
			{
				state = "stopped";
			}

			virtual void face(DirectX::XMVECTOR& _facing)
			{
				facing = DirectX::XMVector4Normalize(_facing);
			}
		};

		class player : public actor
		{
		public:
			player() {
				class_name = "player";
			}
			player(const player& _src) = default;
			player(player&& _src) = default;
			player& operator =(const player& _src) = default;
			player& operator =(player&& _src) = default;

			virtual void get_json(json& _dest)
			{
				actor::get_json(_dest);
			}

			virtual void put_json(game_factory& _gbus, json& _src)
			{
				actor::put_json(_gbus, _src);
			}
		};

		class npc : public actor
		{
		public:
			npc() {
				class_name = "npc";
			}
			npc(const npc& _src) = default;
			npc(npc&& _src) = default;
			npc& operator =(const npc& _src) = default;
			npc& operator =(npc&& _src) = default;

			virtual void get_json(json& _dest)
			{
				actor::get_json(_dest);
			}

			virtual void put_json(game_factory& _gbus, json& _src)
			{
				actor::put_json(_gbus, _src);
			}
		};

        class map : public piece_list
		{
		public:

			map() {
				class_name = "map";
			}
			map(const map& _src) = default;
			map(map&& _src) = default;
			map& operator =(const map& _src) = default;
			map& operator =(map&& _src) = default;

			std::string name;
			std::string description;

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
				piece_list::get_json(_dest);
				_dest.put_member("name", name);
                _dest.put_member("description", description);
			}

			virtual void put_json(game_factory& _gbus, json& _src)
			{
				piece_list::put_json(_gbus, _src);
				name = _src["name"].as_string();
                description = _src["description"].as_string();
			}
		};

		enum class game_state
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

		class game : public job, public corona_object, public game_interface
		{

		public:

			game(comm_bus_app_interface* _bus, json& _src) : factories(_bus), bus(_bus)
			{
				init();
				put_json(_src);
			}

			game(const game& _src) = default;
			game(game&& _src) = default;
			game& operator =(const game& _src) = default;
			game& operator =(game&& _src) = default;


			game_factory factories;
			corona::selection_commands::command_collection selections;
			collision_commands collisions;

			comm_bus_app_interface* bus;
			std::string			name;
			std::string			description;
			timer				frame_timer;
			lockable			map_locker;
			std::shared_ptr<map> game_map;
			game_state state;

			void init();

			std::shared_ptr<fire_command> fire_c = std::make_shared<fire_command>();
			std::shared_ptr<cast_command> cast_c = std::make_shared<cast_command>();
			std::shared_ptr<hit_command> hit_c = std::make_shared<hit_command>();
			std::shared_ptr<reload_command> reload_c = std::make_shared<reload_command>();
			std::shared_ptr<drop_command> drop_c = std::make_shared<drop_command>();

			std::shared_ptr<corona_bus_command> on_all_players_ready;
			std::shared_ptr<corona_bus_command> on_all_players_dead;
			std::map<std::string, std::shared_ptr<corona_bus_command>> on_player_use;
			std::map<std::string, std::shared_ptr<corona_bus_command>> on_player_shot;

			std::shared_ptr<piece> create_piece_of_type(std::string _piece_type_name);
			std::shared_ptr<piece> create_piece_of_class(std::string _class_name);
			std::shared_ptr<piece> clone_piece(piece* _dest_inventory, piece* _src_piece, std::string _class_name, std::string _piece_type, int _quantity);
			double fill_piece(piece* _dest, piece* _src, std::function<bool(const std::string&)> _type_names, double _level);
			double transfer_piece(piece* _dest, piece* _src, std::function<bool(const std::string&)> _type_names, double _quantity);
			double transfer_piece(piece* _dest, piece* _src, std::string _type_name, double _quantity);
			double copy_piece(piece* _dest, piece* _src, std::function<bool(const std::string&)> _type_names, double _quantity);
			double copy_piece(piece* _dest, piece* _src, std::string _type_name, double _quantity);

			void handle_gamepad_button_up(gamepad_button_up_event gpbd);
			void handle_gamepad_button_down(gamepad_button_down_event gpbd);
			void handle_gamepad_trigger_up(gamepad_trigger_up_event gptu);
			void handle_gamepad_trigger_down(gamepad_trigger_down_event gptd);
			void handle_gamepad_thumbstick_move(gamepad_thumbstick_move_event gpbd);

			void set_lobby();
			void set_active();
			void set_paused();
			void set_complete();
			void set_exit();
			void start_play(std::string input_name);
			void check_all_ready();
			void check_all_dead();
			virtual void get_json(json& _dest);
			virtual void put_json(json& _src);
			virtual job* get_next_job();

		private:


			double last_elapsed_seconds;

			DirectX::XMVECTOR zero_vector = {};

			std::shared_ptr<player> attach_player(std::string input_name);
			std::shared_ptr<player> attach_player(XINPUT_STATE& _input_state);

			// This has to be const because we want the assertion that this doesn't modify the map or the pieces,
			// and only finds their state
			collision_event model_piece(std::shared_ptr<map> _map, int _piece_index, double _elapsed_secs) const;
			collision_event find_closest_collision(double delta) const;

			void run_active(double delta);
			void run_complete(double delta);
			void run_lobby(double delta);
			void run_paused(double delta);
			void run_exit(double delta);

			virtual job_notify execute(job_queue* _callingQueue, DWORD _bytesTransferred, BOOL _success);

		};

		class player_command : public corona::selection_commands::command
		{


		public:

			template <typename T> T* get_item_ptr(game* _game, piece* _src, std::vector<corona::selection_commands::match>& _matches, std::shared_ptr<corona::selection_commands::requirement> _requirement)
			{
				if (!_src) return nullptr;

				auto m = get_match(_matches, _requirement);

				if (m)
				{
					auto item = _src->get_item<T>(_game, m.first());
					return item;
				}
				return nullptr;
			}

			virtual void run(game* _game, player* _actor) = 0;
		};

		class fire_command : public player_command
		{
		public:

			std::shared_ptr<corona::selection_commands::requirement> r_firearm = std::make_shared<corona::selection_commands::requirement>();

			fire_command()
			{
				using namespace corona::selection_commands;

				command_name = "fire";

				r_firearm = std::make_shared<requirement>();
				r_firearm->name = "firearm";
				r_firearm->class_name = "firearm";
				r_firearm->item_type = "";
				requirements.push_back(r_firearm);
			}

			virtual void run(game* _game, player* _actor)
			{
				using namespace corona::selection_commands;

				std::vector<match> found_matches;

				bool bm = matches(found_matches, *_actor->inventory.get(), *_actor->selection.get());
				if (!bm)
					return;

                auto _arm = get_item_ptr<firearm>(_game, _actor, found_matches, r_firearm);

				if (_arm) {

					auto _mag = _arm->get_item_ptr<magazine>(_game, "magazine", _arm->magazine_type);
					if (_mag) {
						auto _ammo = _mag->get_item<ammunition>(_game, "ammunition", _mag->ammunition_type);
						if (_ammo && _ammo.container.quantity > 1) {
							_ammo.container.quantity -= 1;
							auto _shot = _ammo.item_object->get_item_ptr<shot>(_game, "shot", _ammo.item_object->shot_type);
							if (_shot) {

								auto new_shot = std::make_shared<shot>(_shot);
								new_shot->position = DirectX::XMVectorAdd(_shot->position, _actor->position);
								new_shot->position = DirectX::XMVectorAdd(new_shot->position, _arm->fire_point);
								new_shot->facing = DirectX::XMVector2Normalize(_actor->facing);
                                new_shot->velocity = DirectX::XMVectorScale(new_shot->facing, _ammo.item_object->shot_muzzle_velocity);
                                _game->game_map->pieces.push_back(new_shot);

                                DirectX::XMVECTOR recoil = DirectX::XMVectorScale(new_shot->facing, -_ammo.item_object->shot_muzzle_velocity * _ammo.item_object->shot_mass / _actor->mass);
                                _actor->velocity = DirectX::XMVectorAdd(_actor->velocity, recoil);
							}
                        }
					}
                }
			}
		};

		class cast_command : public player_command
		{
		public:

			std::shared_ptr<corona::selection_commands::requirement> r_cast = std::make_shared<corona::selection_commands::requirement>();

			cast_command()
			{
				command_name = "cast";

				r_cast->name = "wand";
				r_cast->class_name = "wand";
				r_cast->item_type = "";
				requirements.push_back(r_cast);
			}

			virtual void run(game* _game, player* _actor)
			{
				using namespace corona::selection_commands;

				std::vector<match> found_matches;

				bool bm = matches(found_matches, *_actor->inventory.get(), *_actor->selection.get());
				if (!bm)
					return;

				auto _wand = get_item_ptr<wand>(_game, _actor, found_matches, r_cast);

				if (_wand) {
					auto _shot = _wand->get_item_ptr<spell>(_game, "spell", _wand->spell_type);
					if (_shot) {
						auto new_shot = std::make_shared<shot>(_shot);
						new_shot->position = DirectX::XMVectorAdd(_shot->position, _actor->position);
						new_shot->position = DirectX::XMVectorAdd(new_shot->position, _wand->fire_point);
						new_shot->facing = DirectX::XMVector2Normalize(_actor->facing);
						new_shot->velocity = DirectX::XMVectorScale(new_shot->facing, _shot->spell_velocity);
						_game->game_map->pieces.push_back(new_shot);
					}
				}
			}
		};

		class hit_command : public player_command
		{
		public:

			std::shared_ptr<corona::selection_commands::requirement> r_hit = std::make_shared<corona::selection_commands::requirement>();

			hit_command()
			{
				command_name = "hit";
				r_hit->name = "stick";
				r_hit->class_name = "stick";
				r_hit->item_type = "";
				requirements.push_back(r_hit);
			}

			virtual void run(game* _game, actor* _actor)
			{
				using namespace corona::selection_commands;

				requirements.clear();

				std::vector<match> found_matches;

				bool bm = matches(found_matches, *_actor->inventory.get(), *_actor->selection.get());
				if (!bm)
					return;

				auto _stick = get_item_ptr<stick>(_game, _actor, found_matches, r_hit);

				if (_stick) {
					auto _hit = _stick->get_item_ptr<hit>(_game, "hit", _stick->hit_type);
					if (_hit) {
						auto new_hit = std::make_shared<hit>(_hit);
						new_hit->position = DirectX::XMVectorAdd(_hit->position, _actor->position);
						new_hit->facing = DirectX::XMVector2Normalize(_actor->facing);
						new_hit->velocity = DirectX::XMVectorZero();
						// add the new hit to the game map
						_game->game_map->pieces.push_back(new_hit);
						_actor->state = "hit";
					}
				}
			}
		};

		class drop_command : public player_command
		{
		public:

			std::shared_ptr<corona::selection_commands::requirement> r_drop = std::make_shared<corona::selection_commands::requirement>();

			drop_command()
			{
				command_name = "drop";
				r_drop->name = "piece";
				r_drop->class_name = "piece";
				r_drop->item_type = "";
				requirements.push_back(r_drop);
			}

			virtual void run(game* _game, actor* _actor)
			{
				using namespace corona::selection_commands;

				requirements.clear();

				std::vector<match> found_matches;

				bool bm = matches(found_matches, *_actor->inventory.get(), *_actor->selection.get());
				if (!bm)
					return;

				auto target = _game->create_piece_of_type("lootspot");

				for (auto& fm : found_matches) {
					for (auto& ci : fm.matches) {
                        _actor->inventory->remove_part(ci);
                        target->inventory->add_part(ci);
					}
				}

                target->position = DirectX::XMVectorAdd(_actor->position, DirectX::XMVectorScale(_actor->facing, 1.0));
				target->velocity = DirectX::XMVectorScale(_actor->facing, 1.0);
                target->acceleration = DirectX::XMVectorZero();

                _game->game_map->pieces.push_back(target);
			}

		};

		class reload_command : public player_command
		{
		public:

			std::shared_ptr<corona::selection_commands::requirement> r_firearm = std::make_shared<corona::selection_commands::requirement>();
			std::shared_ptr<corona::selection_commands::requirement> r_magazine = std::make_shared<corona::selection_commands::requirement>();

			std::string loading_class;
			std::string loaded_class;

			reload_command(std::string _command_name, std::string _loading_class, std::string _loaded_class)
			{
				command_name = _command_name;
				loading_class = _loaded_class;
                loaded_class = _loaded_class;
				init();
			}

			reload_command()
			{
				command_name = "reload";
				loading_class = "firearm";
				loaded_class = "magazine";
				init();
            }

            reload_command(const reload_command& _src) = default;
            reload_command(reload_command&& _src) = default;
            reload_command& operator =(const reload_command& _src) = default;
            reload_command& operator =(reload_command&& _src) = default;

			void init()
			{
				requirements.clear();

				r_firearm->name = command_name;
				r_firearm->class_name = loading_class;
				r_firearm->item_type = "";
				requirements.push_back(r_firearm);

				r_magazine->name = command_name;
				r_magazine->class_name = loaded_class;
				r_magazine->item_type = "";
				requirements.push_back(r_magazine);
			}

			virtual void run(game* _game, actor* _actor)
			{
				using namespace corona::selection_commands;

				std::vector<match> found_matches;

				bool bm = matches(found_matches, *_actor->inventory.get(), *_actor->selection.get());
				if (!bm)
					return;

				auto _arm = get_item_ptr<firearm>(_game, _actor, found_matches, r_firearm);
				auto _mag = get_item_ptr<magazine>(_game, _actor, found_matches, r_magazine);

				if (_arm) {
					std::string mag_class_name;

                    // move the magazines out of the gun and into the actor's inventory
					_game->transfer_piece(_actor, _arm, loaded_class, 1.0);
					
					// we have the magazine that was selected,
                    _mag->owner = _actor->to_reference();

					// and the chest item for the magazine...
                    _actor->inventory->add_part(_mag->to_chest_item(1.0));
				}
			}
		};

		class build_reactant
		{
		public:
            std::string class_name;
			std::string type_name;
			double      quantity;
            bool	    consume;

			void get_json(json& _dest)
			{
				_dest.put_member_string("class_name", class_name);
                _dest.put_member_string("type_name", type_name);
				_dest.put_member_double("quantity", quantity);
				_dest.put_member_bool("consume", consume);	
			}

			void put_json(json& _src)
			{
				class_name = _src["class_name"].as_string();
                type_name = _src["type_name"].as_string();
                quantity = _src["quantity"].as_double();
                consume = _src["consume"].as_bool();
            }
		};

		class build_product
		{
		public:
			std::string class_name;
			std::string type_name;
			double      quantity;

			void get_json(json& _dest)
			{
				_dest.put_member_string("class_name", class_name);
				_dest.put_member_string("type_name", type_name);
				_dest.put_member_double("quantity", quantity);
			}

			void put_json(json& _src)
			{
				class_name = _src["class_name"].as_string();
				type_name = _src["type_name"].as_string();
				quantity = _src["quantity"].as_double();
			}
		};

		class build_command : public player_command
		{
		public:

			std::vector<build_reactant> reactants;
			std::vector<build_product> products;

			std::vector<std::shared_ptr<corona::selection_commands::requirement>> r_reactants;

			build_command(std::string& _command_name,
				std::vector<build_reactant>& _reactants,
				std::vector<build_product>& _products)
			{
				reactants = _reactants;
				products = _products;
				command_name = _command_name;
				init();
			}

			build_command()
			{
				command_name = "build";
				reactants = {};
				products = {};
				init();
			}

			build_command(const build_command& _src) = default;
			build_command(build_command&& _src) = default;
			build_command& operator =(const build_command& _src) = default;
			build_command& operator =(build_command&& _src) = default;

			void init()
			{
				
                // each reactant is a requirement, and the whole question is, 
				// do we have the things we need to make our stuff
				requirements.clear();

                for (auto& r : reactants) {
					std::shared_ptr<corona::selection_commands::requirement> scr = std::make_shared<corona::selection_commands::requirement>();
					scr->name = command_name;
					scr->class_name = r.class_name;
					scr->item_type = r.type_name;
                    scr->quantity = r.quantity;
					requirements.push_back(scr);
				}
			}

			virtual void run(game* _game, actor* _actor)
			{
				using namespace corona::selection_commands;

				std::vector<match> requirement_matches;

				bool bm = matches(requirement_matches, *_actor->inventory.get(), *_actor->selection.get());
				if (!bm)
					return;

				for (auto &fm : requirement_matches) {
                    for (auto& ci : fm.matches) {
						_actor->inventory->remove_part(ci);
					}
				}
			}
		};

		class world : public corona_object
		{
		public:
			std::string name;
			std::string description;
            std::shared_ptr<map> map;
            std::vector<std::shared_ptr<game>> games;

			world() {
				class_name = "world";
            }
            world(const world& _src) = default;
            world(world&& _src) = default;
            world& operator =(const world& _src) = default;
            world& operator =(world&& _src) = default;

			virtual void get_json(json& _dest)
			{
                _dest.put_member("name", name);
                _dest.put_member("description", description);
				json_parser jp;
                json jmap = jp.create_object();
                map->get_json(jmap);
                _dest.put_member("map", jmap);
			}

			virtual void put_json(game_factory& _factory, json& _src)
			{
				name = _src["name"].as_string();
				description = _src["description"].as_string();
				json jmap = _src["map"];
				map->put_json(_factory, jmap);
            }
		};

		class engine
		{
			comm_bus_app_interface *bus;
			corona_instance instance = corona_instance::local;
			std::vector<std::shared_ptr<game>> games;

		public:

			engine(comm_bus_app_interface* _db) : bus(_db)
			{
			}

			std::shared_ptr<game> new_game(json _world_key)
			{
				json_parser jp;

				// grab a copy of the mini_game,
				// and then copy it to a new session.

				json copy_plan = jp.create_object();

				json copy_from = jp.create_object();
				copy_from.copy_member("class_name", _world_key);
				copy_from.copy_member("object_id", _world_key);

				json copy_to = jp.create_object();
				copy_to.put_member_string("class_name", "game");

				json copy_transform = jp.create_object();
				copy_transform.put_member_string("class_name", "game");

				copy_plan.put_member("from", copy_from);
				copy_plan.put_member("to", copy_to);
				copy_plan.put_member("transform", copy_transform);

				auto ccr = bus->copy_object(instance, copy_plan);
				if (ccr.success) {
					json new_session = ccr.data;
					std::shared_ptr<game> session = std::make_shared<game>(bus, new_session);
					games.push_back(session);
					return session;
				}

				return nullptr;
			}

			std::shared_ptr<game> load_game(json _game_key)
			{
				json_parser jp;
				auto result = bus->get_object(instance, _game_key);
				if (result.success) {
					std::shared_ptr<game> session = std::make_shared<game>(bus, result.data);
					games.push_back(session);
					return session;
				}
				return nullptr;
			}

			void save_game(std::shared_ptr<game> _game)
			{
				json_parser jp;
				json jsession = jp.create_object();
				_game->get_json(jsession);
				bus->put_object(instance, jsession);
			}

			void close_game(std::shared_ptr<game> _game)
			{
				_game->set_exit();
				std::remove(games.begin(), games.end(), _game);
			}
		};

		std::shared_ptr<engine_interface> engine_factory::create_engine(system_monitoring_interface*_bus)
		{
			auto busorama = dynamic_cast<comm_bus_app_interface *>(_bus);
            auto ptr = std::make_shared<engine>(busorama);
            auto result = std::dynamic_pointer_cast<engine_interface>(ptr);
			return result;
		}

		// this gets called on every piece, to apply accelerations, make animation calculations, 
		// and other time-based calculations. The default implementation does nothing.
		// _elapsed_seconds is the time since the last call to run, in seconds.
		// this will be quite fractional.
		void piece::run(game* _game, piece* _user, double _elapsed)
		{

		}

		/// <summary>
		/// gets called when a piece is used by a player.  default impl does nothing, and use is like a blah verb.
		/// </summary>
		/// <param name="_game"></param>
		/// <param name="_user"></param>
		void piece::use(game* _game, piece* _user)
		{

		}

		void actor::select_next()
		{
			selection->clear();
			if (selection && inventory) {
				auto selected = selection->selections.begin();
				if (selected != std::end(selection->selections)) {
                    auto iter = inventory->items.upper_bound(selected->first);
					if (iter != std::end(inventory->items)) {
						selection->extend(iter->second.reference);
					}
				}
				else 
				{
                    auto default_item = inventory->items.begin();
					if (default_item != std::end(inventory->items)) {
						selection->extend(default_item->second.reference);
					}
				}
			}
		}

		void actor::select_previous()
		{
			selection->clear();
			if (selection && inventory) {
				auto selected = selection->selections.begin();
				if (selected != std::end(selection->selections)) {
					auto iter = inventory->items.lower_bound(selected->first);
					if (iter != std::end(inventory->items)) {
                        auto reverse_iter = std::make_reverse_iterator(iter);
						reverse_iter++;
                        if (reverse_iter != std::rend(inventory->items)) {
							selection->extend(reverse_iter->second.reference);
						}
					}
				}
				else
				{
					auto default_item = inventory->items.begin();
					if (default_item != std::end(inventory->items)) {
						selection->extend(default_item->second.reference);
					}
				}
			}
		}

		void actor::extend_selection(chest_item* _ci)
		{
			if (selection && _ci)
			{
				selection->extend(_ci->reference);
			}
		}

		void actor::clear_selection()
		{
			if (selection)
			{
				selection->clear();
			}
		}

		void piece_list::get_json(json& _dest)
		{
			json_parser jp;

			json j = jp.create_array();
			for (auto& s : pieces) {
				json jitem = jp.create_object();
				s->get_json(jitem);
				j.push_back(jitem);
			}
			_dest.put_member("pieces", j);
		}

		void piece_list::put_json(game_factory& _gbus, json& _src)
		{
			json j = _src["pieces"];
			pieces = _gbus.piece_factory.create_array(j);
		}

		std::shared_ptr<piece> game::clone_piece(piece* _dest_inventory, piece* _src_piece, std::string _class_name, std::string _piece_type, int _quantity)
		{
			std::shared_ptr<piece>  response;

			std::vector<chest_item> found_items;

			bool found_matches = _src_piece->inventory->match(found_items, _class_name, _piece_type, _quantity);

			if (found_matches) {

				for (auto& existing_reference : found_items)
				{
					// get the piece that is in the source inventory, and copy it to a new piece
					// that is, we are making a new piece, and putting it in the destination inventory

					auto existing_piece = factories.piece_factory.get_object(existing_reference.reference, true);

					if (existing_piece)
					{
						// this creates and saves the piece in the database
						auto new_piece = existing_piece->copy_as<piece>(corona_instance::local);

						// create a new chest item for the piece we made, 
						// that is, what's going into the inventory
						auto ci = new_piece->to_chest_item(_quantity);

						// then, we figure out, who dest inventory is, 
						// and that is the owner of our new piece
						new_piece->owner = _dest_inventory->to_reference();

						_dest_inventory->inventory->add_part(ci);
						_dest_inventory->save(corona_instance::local);
					}
				}
			}

			return response;
		}

		std::shared_ptr<piece> game::create_piece_of_type(std::string _piece_type_name )
		{		
			std::shared_ptr<piece> new_piece;
			json_parser jp;
			
			json filter = jp.create_object();
			filter.put_member_string("class_name", "piece_type");
            filter.put_member_string("name", _piece_type_name);
			auto response = bus->query_objects(corona_instance::local, filter);

			if (response.success) {
                auto found_pieces = factories.general_factory.create_array(response.data);
                if (found_pieces.size() > 0) {
					auto found_piece_type = std::dynamic_pointer_cast<piece_type>(found_pieces[0]);
					std::string instance_class_name = found_piece_type->piece_class_name;
					new_piece = factories.piece_factory.create_object(instance_class_name);
					if (new_piece) {
						new_piece->name = found_piece_type->name;
						new_piece->animations = found_piece_type->animations;
						new_piece->full_health = found_piece_type->full_health;
						new_piece->health = found_piece_type->full_health;
						new_piece->state = found_piece_type->state;
					}
				}
			}

			return new_piece;
		}

		std::shared_ptr<piece> game::create_piece_of_class(std::string _class_name)
		{
			std::shared_ptr<piece> new_piece;
			json_parser jp;

			auto new_piece = factories.piece_factory.create_object(_class_name);
			return new_piece;
		}

		double game::fill_piece(piece* _dest, piece* _src, std::function<bool(const std::string&)> _type_names, double _level)
		{
			double quantity_filled = 0;

			if (_level < 0 || !_dest->inventory || !_src->inventory) {
				return quantity_filled;
			}

			// Calculate current quantity on hand
			double quantity_on_hand = 0;
			for (const auto& [key, item] : _dest->inventory->items)
			{
				if (_type_names(item.item_type)) {
					quantity_on_hand += item.quantity;
				}
			}

			// Only fill if below the level
			if (quantity_on_hand < _level)
			{
				double fill_amount = _level - quantity_on_hand;

				// Transfer items from source to destination
				for (auto& [key, src_item] : _src->inventory->items)
				{
					if (_type_names(src_item.item_type) && fill_amount > 0) {
						chest_item transfer_item = src_item;
						double transfer_amount = std::min(fill_amount, src_item.quantity);

						transfer_item.quantity = transfer_amount;
						quantity_filled += transfer_amount;
						fill_amount -= transfer_amount;
						quantity_on_hand += transfer_amount;

						_dest->inventory->add_part(transfer_item);
						_src->inventory->remove_part(transfer_item);

						if (quantity_on_hand >= _level) {
							break;
						}
					}
				}

				// Save both pieces through the bus
				json_parser jp;
				json jdest = jp.create_object();
				json jsrc = jp.create_object();
				_dest->get_json(jdest);
				_src->get_json(jsrc);
				bus->put_object(corona_instance::local, jdest);
				bus->put_object(corona_instance::local, jsrc);
			}

			return quantity_filled;
		}

		double game::transfer_piece(piece* _dest, piece* _src, std::function<bool(const std::string&)> _type_names, double _quantity)
		{
			double quantity_transferred = 0;

			if (_quantity <= 0 || !_dest->inventory || !_src->inventory) {
				return quantity_transferred;
			}

			double remaining = _quantity;

			// Transfer items from source to destination
			for (auto& [key, src_item] : _src->inventory->items)
			{
				if (_type_names(src_item.item_type) && remaining > 0) {
					chest_item transfer_item = src_item;
					double transfer_amount = std::min(remaining, src_item.quantity);

					transfer_item.quantity = transfer_amount;
					quantity_transferred += transfer_amount;
					remaining -= transfer_amount;

					_dest->inventory->add_part(transfer_item);
					_src->inventory->remove_part(transfer_item);

					if (remaining <= 0) {
						break;
					}
				}
			}

			// Save both pieces through the bus if any transfer occurred
			if (quantity_transferred > 0) {
				json_parser jp;
				json jdest = jp.create_object();
				json jsrc = jp.create_object();
				_dest->get_json(jdest);
				_src->get_json(jsrc);
				bus->put_object(corona_instance::local, jdest);
				bus->put_object(corona_instance::local, jsrc);
			}

			return quantity_transferred;
		}

		double game::copy_piece(piece* _dest, piece* _src, std::function<bool(const std::string&)> _type_names, double _quantity)
		{
			double quantity_transferred = 0;

			if (_quantity <= 0 || !_dest->inventory || !_src->inventory) {
				return quantity_transferred;
			}

			double remaining = _quantity;

			// Transfer items from source to destination
			for (auto& [key, src_item] : _src->inventory->items)
			{
				if (_type_names(src_item.item_type) && remaining > 0) {
					auto new_piece = create_piece_of_type(src_item.item_type);
					break;
				}
			}

			// Save both pieces through the bus if any transfer occurred
			if (quantity_transferred > 0) {
				json_parser jp;
				json jdest = jp.create_object();
				json jsrc = jp.create_object();
				_dest->get_json(jdest);
				_src->get_json(jsrc);
				bus->put_object(corona_instance::local, jdest);
				bus->put_object(corona_instance::local, jsrc);
			}

			return quantity_transferred;
		}

		double game::transfer_piece(piece* _dest, piece* _src, std::string _type_name, double _quantity)
		{
			return transfer_piece(_dest, _src, [_type_name](const std::string _tn) {
				return _tn.empty() || _type_name == _tn;
				}, _quantity);
		}

		double game::copy_piece(piece* _dest, piece* _src, std::string _type_name, double _quantity)
		{
			return copy_piece(_dest, _src, [_type_name](const std::string _tn) {
				return _tn.empty() || _type_name == _tn;
				}, _quantity);
		}

		void game::init()
		{

			factories.frame_factory.register_class("frame", [](json& _src, comm_bus_app_interface* _bus) -> std::shared_ptr<frame> {
				return std::make_shared<frame>(_src);
				});
			factories.frame_factory.register_class("bitmap_frame", [](json& _src, comm_bus_app_interface* _bus) -> std::shared_ptr<bitmap_frame> {
				return std::make_shared<bitmap_frame>(_src);
				});
			factories.frame_factory.register_class("vector_frame", [](json& _src, comm_bus_app_interface* _bus) -> std::shared_ptr<vector_frame> {
				return std::make_shared<vector_frame>(_src);
				});

			factories.piece_factory.register_class("piece", [](json& _src, comm_bus_app_interface* _bus) -> std::shared_ptr<piece> {
				return std::make_shared<piece>(_src);
				});
			factories.piece_factory.register_class("actor", [](json& _src, comm_bus_app_interface* _bus) -> std::shared_ptr<actor> {
				return std::make_shared<actor>(_src);
				});
			factories.piece_factory.register_class("player", [](json& _src, comm_bus_app_interface* _bus) -> std::shared_ptr<player> {
				return std::make_shared<player>(_src);
				});
			factories.piece_factory.register_class("npc", [](json& _src, comm_bus_app_interface* _bus) -> std::shared_ptr<npc> {
				return std::make_shared<npc>(_src);
				});
			factories.piece_factory.register_class("feature", [](json& _src, comm_bus_app_interface* _bus) -> std::shared_ptr<feature> {
				return std::make_shared<feature>(_src);
				});
			factories.piece_factory.register_class("spawn", [](json& _src, comm_bus_app_interface* _bus) -> std::shared_ptr<feature> {
				return std::make_shared<spawn>(_src);
				});
			factories.piece_factory.register_class("player_spawn", [](json& _src, comm_bus_app_interface* _bus) -> std::shared_ptr<feature> {
				return std::make_shared<spawn>(_src);
				});
			factories.piece_factory.register_class("npc_spawn", [](json& _src, comm_bus_app_interface* _bus) -> std::shared_ptr<feature> {
				return std::make_shared<spawn>(_src);
				});
			factories.piece_factory.register_class("loot_box", [](json& _src, comm_bus_app_interface* _bus) -> std::shared_ptr<lootbox> {
				return std::make_shared<lootbox>(_src);
				});
			factories.piece_factory.register_class("loot_spot", [](json& _src, comm_bus_app_interface* _bus) -> std::shared_ptr<lootspot> {
				return std::make_shared<lootspot>(_src);
				});
			factories.piece_factory.register_class("wall", [](json& _src, comm_bus_app_interface* _bus) -> std::shared_ptr<wall> {
				return std::make_shared<wall>(_src);
				});
			factories.piece_factory.register_class("switcher", [](json& _src, comm_bus_app_interface* _bus) -> std::shared_ptr<switcher> {
				return std::make_shared<switcher>(_src);
				});
			factories.piece_factory.register_class("door", [](json& _src, comm_bus_app_interface* _bus) -> std::shared_ptr<door> {
				return std::make_shared<door>(_src);
				});
			factories.piece_factory.register_class("surface", [](json& _src, comm_bus_app_interface* _bus) -> std::shared_ptr<surface> {
				return std::make_shared<surface>(_src);
				});
			factories.piece_factory.register_class("decoration", [](json& _src, comm_bus_app_interface* _bus) -> std::shared_ptr<decoration> {
				return std::make_shared<decoration>(_src);
				});
			factories.piece_factory.register_class("light", [](json& _src, comm_bus_app_interface* _bus) -> std::shared_ptr<light> {
				return std::make_shared<light>(_src);
				});
			factories.piece_factory.register_class("spot_light", [](json& _src, comm_bus_app_interface* _bus) -> std::shared_ptr<spot_light> {
				return std::make_shared<spot_light>(_src);
				});
			factories.piece_factory.register_class("globe_light", [](json& _src, comm_bus_app_interface* _bus) -> std::shared_ptr<globe_light> {
				return std::make_shared<globe_light>(_src);
				});
			factories.piece_factory.register_class("camera", [](json& _src, comm_bus_app_interface* _bus) -> std::shared_ptr<camera> {
				return std::make_shared<camera>(_src);
				});
			factories.piece_factory.register_class("effect", [](json& _src, comm_bus_app_interface* _bus) -> std::shared_ptr<effect> {
				return std::make_shared<effect>(_src);
				});
			factories.piece_factory.register_class("delivery", [](json& _src, comm_bus_app_interface* _bus) -> std::shared_ptr<delivery> {
				return std::make_shared<delivery>(_src);
				});
			factories.piece_factory.register_class("carryable", [](json& _src, comm_bus_app_interface* _bus) -> std::shared_ptr<carryable> {
				return std::make_shared<carryable>(_src);
				});
			factories.piece_factory.register_class("consumable", [](json& _src, comm_bus_app_interface* _bus) -> std::shared_ptr<consumable> {
				return std::make_shared<consumable>(_src);
				});
			factories.piece_factory.register_class("tool", [](json& _src, comm_bus_app_interface* _bus) -> std::shared_ptr<tool> {
				return std::make_shared<tool>(_src);
				});
			factories.piece_factory.register_class("firearm", [](json& _src, comm_bus_app_interface* _bus) -> std::shared_ptr<firearm> {
				return std::make_shared<firearm>(_src);
				});
			factories.piece_factory.register_class("magazine", [](json& _src, comm_bus_app_interface* _bus) -> std::shared_ptr<magazine> {
				return std::make_shared<magazine>(_src);
				});
			factories.piece_factory.register_class("ammunition", [](json& _src, comm_bus_app_interface* _bus) -> std::shared_ptr<ammunition> {
				return std::make_shared<ammunition>(_src);
				});
			factories.piece_factory.register_class("shot", [](json& _src, comm_bus_app_interface* _bus) -> std::shared_ptr<shot> {
				return std::make_shared<shot>(_src);
				});
			factories.piece_factory.register_class("wand", [](json& _src, comm_bus_app_interface* _bus) -> std::shared_ptr<wand> {
				return std::make_shared<wand>(_src);
				});
			factories.piece_factory.register_class("spell", [](json& _src, comm_bus_app_interface* _bus) -> std::shared_ptr<spell> {
				return std::make_shared<spell>(_src);
				});
			factories.piece_factory.register_class("stick", [](json& _src, comm_bus_app_interface* _bus) -> std::shared_ptr<stick> {
				return std::make_shared<stick>(_src);
				});
			factories.piece_factory.register_class("artifact", [](json& _src, comm_bus_app_interface* _bus) -> std::shared_ptr<artifact> {
				return std::make_shared<artifact>(_src);
				});

			selections.put(fire_c);
			selections.put(cast_c);
			selections.put(hit_c);
			selections.put(reload_c);
			selections.put(drop_c);


            // we could add a build command, and will, once this shebang loads from a json more than it does.

			/// selections.put(std::make_shared<build_command>());

			// factories.init(corona_instance::local);

		}

		void game::handle_gamepad_button_up(gamepad_button_up_event gpbd)
		{
			auto player = attach_player(gpbd.state);
			switch (gpbd.button)
			{
			case gamepad_button::A:
				break;
			case gamepad_button::B:
				break;
			case gamepad_button::X:
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

		void game::handle_gamepad_button_down(gamepad_button_down_event gpbd)
		{
			auto player = attach_player(gpbd.state);
			switch (gpbd.button)
			{
			case gamepad_button::A:
				player->walk();
				break;
			case gamepad_button::B:
				player->run();
				break;
			case gamepad_button::X:
				player->crawl();
				break;
			case gamepad_button::Y:
				break;
			case gamepad_button::LeftShoulder:
				player->select_next();
				break;
			case gamepad_button::RightShoulder:
				player->select_previous();
				break;
			case gamepad_button::Back:
				break;
			case gamepad_button::Start:
				start_play(player->input_device);
				break;
			case gamepad_button::DpadUp:
				player->move_foward();
				break;
			case gamepad_button::DpadDown:
				player->move_back();
				break;
			case gamepad_button::DpadLeft:
				player->move_left();
				break;
			case gamepad_button::DpadRight:
				player->move_right();
				break;
			}
		}

		void game::handle_gamepad_trigger_up(gamepad_trigger_up_event gptu)
		{
			auto player = attach_player(gptu.state);
		}

		void game::handle_gamepad_trigger_down(gamepad_trigger_down_event gptd)
		{
			auto player = attach_player(gptd.state);

			std::vector<std::shared_ptr<corona::selection_commands::command>> commands = selections.get_commands_for_selection(*player->inventory.get(), *player->selection.get());

			if (gptd.trigger == gamepad_trigger::LeftTrigger) {

				auto command_range = std::find_if(commands.begin(), commands.end(), [](std::shared_ptr<corona::selection_commands::command>& _src) {
					return _src->requirements == 
					});

				if (commands.size()) {
					auto cmd = commands[0];
					std::shared_ptr<player_command> pcmd = std::dynamic_pointer_cast<player_command>(cmd);
					if (pcmd) {
						pcmd->run(this, player.get());
					}
				}
			}
		}

		void game::handle_gamepad_thumbstick_move(gamepad_thumbstick_move_event gpbd)
		{
			auto player = attach_player(gpbd.state);
		}

		std::shared_ptr<player> game::attach_player(std::string input_name)
		{
			// Search for existing player with this input device
			for (auto player : game_map->players()) {
				if (player->input_device == input_name) {
					return player;
				}
			}
			// Create new player and add to pieces only
			std::shared_ptr<player> new_player = std::make_shared<player>();
			new_player->name = input_name;
			new_player->ready = false;
			game_map->pieces.push_back(new_player);
			return new_player;
		}

		std::shared_ptr<player> game::attach_player(XINPUT_STATE& _input_state)
		{
			return attach_player(std::to_string(_input_state.dwPacketNumber));
		}

		// This has to be const because we want the assertion that this doesn't modify the map or the pieces,
		// and only finds their state
		collision_event game::model_piece(std::shared_ptr<map> _map, int _piece_index, double _elapsed_secs) const
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

		void game::run_lobby(double delta)
		{
			;
		}

		collision_event game::find_closest_collision(double delta) const
		{
			collision_event closest_collision;
			for (int i = 0; i < game_map->pieces.size(); i++) {
				auto pc = game_map->pieces[i];
				collision_event collision = model_piece(game_map, i, delta);
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

		void game::run_active(double delta)
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
					for (int i = 0; i < game_map->pieces.size(); i++) {
						auto pc = game_map->pieces[i];
						pc->move(closest_collision.collision.time_of_collision);
					}
					// resolve collision effects here and update accelerations accordingly
					// for example, if piece_1 is a player and piece_2 is a wall, we might want to stop the player's movement in the direction of the wall.
					remaining -= closest_collision.collision.time_of_collision;
				}
				else
				{
					// no more collisions, we can move all pieces for the remaining time
					for (int i = 0; i < game_map->pieces.size(); i++) {
						auto pc = game_map->pieces[i];
						pc->move(remaining);
					}
					remaining = 0;
				}
			}
		}

		void game::run_complete(double delta)
		{
			;
		}

		void game::run_paused(double delta)
		{
			;
		}

		void game::run_exit(double delta)
		{
			;
		}

		job_notify game::execute(job_queue* _callingQueue, DWORD _bytesTransferred, BOOL _success)
		{
			job_notify notify;
			json_parser jp;

			notify.shouldDelete = false;

			double current_elapsed_seconds = frame_timer.get_elapsed_seconds();
			double delta = current_elapsed_seconds - last_elapsed_seconds;

			last_elapsed_seconds = current_elapsed_seconds;

			switch (state) {
			case game_state::lobby:
				run_lobby(delta);
				break;
			case game_state::active:
				run_active(delta);
				break;
			case game_state::complete:
				run_complete(delta);
				break;
			case game_state::paused:
				run_paused(delta);
				break;
			case game_state::exit:
				run_exit(delta);
				break;
			}

			return notify;
		}


		void game::set_lobby()
		{
			// Remove all players from the pieces collection
			auto is_player = [](const auto& piece) {
				return std::dynamic_pointer_cast<player>(piece) != nullptr;
				};
			game_map->pieces.erase(
				std::remove_if(game_map->pieces.begin(), game_map->pieces.end(), is_player),
				game_map->pieces.end()
			);
			state = game_state::lobby;
		}

		void game::set_active()
		{
			state = game_state::active;
		}

		void game::set_paused()
		{
			state = game_state::paused;
		}

		void game::set_complete()
		{
			state = game_state::complete;
		}

		void game::set_exit()
		{
			state = game_state::exit;
		}

		void game::start_play(std::string input_name)
		{
			auto player = attach_player(input_name);

			if (state == game_state::lobby) {
				player->ready = !player->ready;
			}
			else if (state == game_state::active) {
				player->dead = false;
				set_paused();
			}
			else if (state == game_state::paused) {
				set_active();
			}
			else if (state == game_state::complete) {
				set_lobby();
			}
			else if (state == game_state::exit) {
				; // do nothing, we're exiting anyway
			}
		}

		void game::check_all_ready()
		{
			if (state == game_state::lobby) {
				auto players_view = game_map->players();
				bool all_ready = std::ranges::all_of(players_view, [](const auto& player) {
					return player->ready;
					});
				if (all_ready) {
					set_active();
				}
			}
		}

		void game::check_all_dead()
		{
			auto players_view = game_map->players();
			bool all_dead = std::ranges::all_of(players_view, [](const auto& player) {
				return player->dead;
				});
			if (all_dead) {
				set_complete();
			}
		}

		void game::get_json(json& _dest)
		{
			json_parser jp;
			_dest.put_member("name", name);
			_dest.put_member("description", description);
			json j = jp.create_array();
			json jmap = jp.create_object();
			game_map->get_json(jmap);
			j.push_back(jmap);
			_dest.put_member("map", j);

			switch (state) {
			case game_state::lobby:
				_dest.put_member_string("state", "lobby");
				break;
			case game_state::active:
				_dest.put_member_string("state", "active");
				break;
			case game_state::paused:
				_dest.put_member_string("state", "paused");
				break;
			case game_state::complete:
				_dest.put_member_string("state", "complete");
				break;
			case game_state::exit:
				_dest.put_member_string("state", "exit");
				break;
			}
		}

		void game::put_json(json& _src)
		{
			name = _src["name"].as_string();
			description = _src["description"].as_string();
			json j = _src["map"];
			game_map = std::make_shared<map>();
			std::string state_string = _src["state"].as_string();
		}

		job* game::get_next_job()
		{
			if (state != game_state::exit)
			{
				return this;
			}
			else
			{
				return nullptr;
			}
		}

	}

}
