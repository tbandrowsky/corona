#pragma once

namespace corona
{

	// corona_instance here is a placeholder,
	// eventually we want to define a game whose various pieces can 
    // come from either a local corona, or a remote corona, and the game engine will handle the differences.
	// or, we say all access is through local corona, so that, they can be stitched into a fabric.
	namespace game
	{
		class game;

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
			std::string state;
			std::vector<std::shared_ptr<animation>> animations;

			std::shared_ptr<chest_field> inventory;

			double		mass = 1.0;
			double      full_health = 1.0;
			double      health = 1.0;

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


		class piece : public corona_object
		{

		public:
			piece() {
				class_name = "piece";
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

			double		mass = 1.0;
			double      full_health = 1.0;
			double      health = 1.0;

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
				_dest.put_member("health", health);
				_dest.put_member("full_health", full_health);

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

			virtual void draw(direct2dContext& _context, double _elapsed, point _location)
			{
                for (auto animation : animations) {
					if (animation->state == state) {
						animation->draw(_context, _elapsed, _location);
					}
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

            // the user uses the piece, which may change its state or cause it to be consumed. The default implementation does nothing and returns a copy of the piece.
			virtual void use(game* _game, piece *_user);

			// the user uses the piece, with a consumable piece
			virtual void load(game* _game, piece* _user, chest_item& _item);

			// the user uses the piece, with a consumable piece
			virtual void unload(game* _game, piece* _user, chest_item& _item);

			// the user drops the selection
            virtual void drop(game* _game, chest_item& _item);

			// the user picks up the object
			virtual void pickup(game* _game, piece* _user, piece *_pickup);

			// this gets called on every piece, to apply accelerations, make animation calculations, 
            // and other time-based calculations. The default implementation does nothing.
            // _elapsed_seconds is the time since the last call to run, in seconds.
			// this will be quite fractional.
			virtual void run(game* _game, piece *_user, double _elapsed);


		};

		using piece_factory = corona_object_factory<piece>;

        using general_factory = corona_object_factory<corona_object>;

		class game_factory
		{
		public:
			piece_factory piece_factory;
			frame_factory frame_factory;
			general_factory general_factory;

			using object_action = std::function<void(game* _game, piece* _direct_object, piece* _actor)>;
			using object_interaction = std::function<void(game* _game, piece* _a, piece* _b)>;

			game_factory(comm_bus_app_interface* bus) noexcept : piece_factory(bus), frame_factory(bus), general_factory(bus)
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

			std::shared_ptr<piece> create_piece(std::string _class_name);

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

			virtual void load(game* _game, piece* _user, piece* _consumable);

			// the user drops the selection
			virtual void drop(game* _game, piece* _user);

			// the user picks up the obect
			virtual void pickup(game* _game, piece* _user, piece* _pickup);

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
					json_parser jp;
                    json spawn_class = jp.create_object();
					spawn_class.put_member(class_name_field, spawn_class);
					auto new_piece = _game->factories.piece_factory.create_object(spawn_class);
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
                            _game->switch_changed(this, state);
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
				_game->door_changed(this, state);
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
				_game->light_changed(this, state);
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

			virtual void get_json(json& _dest)
			{
				piece::get_json(_dest);
				_dest.put_member("shot_type", shot_type);
			}

			virtual void put_json(game_factory& _gbus, json& _src)
			{
				piece::put_json(_gbus.frame_factory, _src);
				shot_type = _src["shot_type"].as_string();
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
			std::string shot_class;
			double shot_muzzle_velocity;
			double shot_mass;
			double shot_impulse;

			virtual void get_json(json& _dest)
			{
				carryable::get_json(_dest);
				_dest.put_member("ammunition_type", ammunition_type);
				_dest.put_member("shot_class", shot_class);
				_dest.put_member("shot_muzzle_velocity", shot_muzzle_velocity);
				_dest.put_member("shot_mass", shot_mass);
				_dest.put_member("shot_impulse", shot_impulse);
			}

			virtual void put_json(game_factory& _gbus, json& _src)
			{
				carryable::put_json(_gbus, _src);
				ammunition_type = _src["ammunition_type"].as_string();
				shot_class = _src["shot_class"].as_string();
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
			std::map<std::string,bool> ammunition_types;

			virtual void get_json(json& _dest)
			{
				carryable::get_json(_dest);
				json_parser jp;
				_dest.put_member("magazine_type", magazine_type);
				json jammo_types = jp.vector_to_jarray(ammunition_types);
                _dest.put_member("ammunition_types", jammo_types);
			}

			virtual void put_json(game_factory& _gbus, json& _src)
			{
				json_parser jp;

				carryable::put_json(_gbus, _src);

				magazine_type = _src["magazine_type"].as_string();
                json jammo_types = _src["ammunition_types"];
				ammunition_types = jp.jarray_to_string_map(jammo_types);
			}

			// now we load the ammunition
			virtual void load(game* _game, piece* _user, piece* _consumable)
			{
				if (auto ammo = dynamic_cast<ammunition*>(_consumable)) {
					std::vector<std::string> class_names = { "ammunition" };
					auto found = _user->inventory->find_first_any(class_names);
					if (found)
					{
						// we found a magazine in the firearm, so we will remove it and put it back in the player's inventory
						inventory->remove_part(*found);
						_user->inventory->add_part(*found);
					}
				}
			}

            // discharges the magazine, which means it removes one round of ammunition from the magazine and returns it as a chest_item. If the magazine is empty, it returns nullptr.
			virtual chest_item *discharge(game* _game, piece* _piece)
			{
				chest_item* discharged = nullptr;
				if (inventory) {
					if (auto *cf = inventory->use(1)) {
						discharged = cf;
					}
				}
                return discharged;
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

            std::map<std::string, bool> magazine_types;

			virtual void get_json(json& _dest)
			{
				carryable::get_json(_dest);
				json_parser jp;
				json jmagazine_types = jp.map_to_jarray(magazine_types);
				_dest.put_member("magazine_types", jmagazine_types);
			}

			virtual void put_json(game_factory& _gbus, json& _src)
			{
				json_parser jp;
				carryable::put_json(_gbus, _src);

				json jmagazine_types = _src["magazine_types"];
				magazine_types = jp.jarray_to_string_map(jmagazine_types);
			}

            virtual void load(game* _game, piece* _piece, magazine* _magazine)
			{
				if (inventory) {
					if (magazine_types.contains(_magazine->magazine_type)) {
						auto ci = _magazine->make_chest_item(1);
						inventory->add_part(ci);
					}
				}
			}

			virtual void use(game* _game, piece* _piece)
			{
				if (inventory) {
                    // see if we have any magazines that are compatible with this firearm
                    // these are the ones in the firearm inventory (that is, the magazine is in the firearm, not in the player inventory)
					auto compatible_magazines = _game->factories.piece_factory.get_inventory(*inventory);
					for (auto& mag : compatible_magazines) {
						if (magazine_types.contains( mag->magazine_type )) {
							// we have a compatible magazine, so we can fire the firearm
							// we will consume one round of ammunition from the magazine
                            // and we can discharge the firearm, which reduces the inventory in the magazine by one
                            auto discharged = mag->discharge(_game, _piece);
							if (discharged) {
								// now the shot is fired.  So now we can create the shot corresponding 
								// to the ammunition.
								auto round = _game->factories.piece_factory.get_object<ammunition>(corona_instance::local, discharged->part_class, discharged->part_id, false);
								if (round) {
									auto new_shot = _game->factories.piece_factory.create_object(round->shot_class);
									if (new_shot) {
										new_shot->position = _piece->position;
										new_shot->facing = _piece->facing;
                                        new_shot->mass = round->shot_mass;
										DirectX::XMVECTOR normalized_facing = DirectX::XMVector3Normalize(_piece->facing);
										auto scaled = DirectX::XMVectorScale(normalized_facing, static_cast<float>(round->shot_muzzle_velocity));
                                        auto new_velocity = DirectX::XMVectorAdd(_piece->velocity, scaled);
                                        new_shot->velocity = new_velocity;
										_game->game_map->pieces.push_back(new_shot);
										double recoil_acceleration = - (new_shot->mass * round->shot_muzzle_velocity) / (_piece->mass * round->shot_impulse);
										auto recoil_vector = DirectX::XMVectorScale(normalized_facing, static_cast<float>(recoil_acceleration);
                                        _piece->acceleration = DirectX::XMVectorAdd(_piece->acceleration, recoil_vector);
									}
								}
							}
						}
					}
                }
			}

			// now we load the ammunition 
			virtual void load(game* _game, piece* _user, chest_item& _item)
			{
                auto ammo = _game->factories.piece_factory.get_object<ammunition>(corona_instance::local, _item.part_class, _item.part_id, false);
				if (ammo) {
					std::vector<std::string> class_names = { "ammunition" };
					auto found = _user->inventory->find_first_any(class_names);
					if (found)
					{
						inventory->add_part(*found);
					}
				}
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

			virtual void get_json(json& _dest)
			{
				tool::get_json(_dest);
			}

			virtual void put_json(game_factory& _gbus, json& _src)
			{
				tool::put_json(_gbus, _src);
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

			virtual void get_json(json& _dest)
			{
				carryable::get_json(_dest);
			}

			virtual void put_json(game_factory& _gbus, json& _src)
			{
				carryable::put_json(_gbus, _src);
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

			virtual void get_json(json& _dest)
			{
				tool::get_json(_dest);
			}

			virtual void put_json(game_factory& _gbus, json& _src)
			{
				tool::put_json(_gbus, _src);
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

		class selection_analysis
		{
		public:
			tool* selected_tool;
			magazine* selected_magazine;
			ammunition* selected_ammunition;
			wand* selected_wand;
			spell* selected_spell;
			piece* selected_piece;
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

			virtual void get_json(json& _dest)
			{
				piece::get_json(_dest);
				_dest.put_member("input_device", input_device);
				_dest.put_member_bool("ready", ready);
				_dest.put_member_bool("dead", dead);
			}

			virtual void put_json(game_factory& _gbus, json& _src)
			{
				piece::put_json(_gbus.frame_factory, _src);
				input_device = _src["input_device"].as_int();
				ready = _src["ready"].as_bool();
				dead = _src["dead"].as_bool();
			}

			selection_analysis analyze_selected(game* _game);

			// and now, we can extend the selection and the inventory

			virtual void select_next();
			virtual void select_previous();
			virtual void extend_selection(chest_item* _ci);
			virtual void clear_selection();
			virtual void use_selection(game* _game);
			virtual void throw_selection(game* _game);
			virtual void drop_selection(game* _game);
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

		class state_event
		{
		public:
			game_state state;

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

		class game : public job, public corona_object, public game_interface
		{

		public:

			game(comm_bus_app_interface *_bus, json& _src) : factories(_bus), bus(_bus)
			{
				init();
				put_json(_src);
			}

            game(const game& _src) = default;
            game(game&& _src) = default;
            game& operator =(const game& _src) = default;
            game& operator =(game&& _src) = default;


			game_factory factories;
			comm_bus_app_interface *bus;
			std::string			name;
			std::string			description;
			timer				frame_timer;
			lockable			map_locker;
			std::map<game_state, std::shared_ptr<state_event>> event_handlers;
			std::shared_ptr<map> game_map;
			std::shared_ptr<state_event> handlers;
			game_state state;

            std::shared_ptr<corona_bus_command> on_all_players_ready;
			std::shared_ptr<corona_bus_command> on_all_players_dead;
			std::map<std::string, std::shared_ptr<corona_bus_command>> on_player_use;
			std::map<std::string, std::shared_ptr<corona_bus_command>> on_player_shot;

			void init()
			{
				factories.frame_factory.register_class("frame", [](json& _src, comm_bus_app_interface *_bus) -> std::shared_ptr<frame> {
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

				factories.init(corona_instance::local);
			}

			void set_lobby()
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
                handlers = event_handlers[state];
			}

			void set_active()
			{
				state = game_state::active;
				handlers = event_handlers[state];
			}

			void set_paused()
			{
				state = game_state::paused;
				handlers = event_handlers[state];
			}

			void set_complete()
			{
				state = game_state::complete;
				handlers = event_handlers[state];
			}

			void set_exit()
			{
				state = game_state::exit;
				handlers = event_handlers[state];
			}

			void start_play(std::string input_name)
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

			void check_all_ready()
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

			void check_all_dead()
			{
				auto players_view = game_map->players();
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

			corona_client_response displace(std::string input_name, DirectX::XMVECTOR d)
			{
				corona_client_response response;
				auto player = attach_player(input_name);
				if (player) {
					response.success = true;
					player->displace(d);
				}
				return response;
			}

			corona_client_response clear_selection(std::string input_name)
			{
				corona_client_response response;
				auto player = attach_player(input_name);
				if (player) {
					response.success = true;
					player->clear_selection();
				}
				return response;
			}

			corona_client_response extend_selection(std::string input_name, chest_item *ci)
			{
				corona_client_response response;
				auto player = attach_player(input_name);
				if (player) {
					response.success = true;
					player->extend_selection(ci);
				}
				return response;
			}

			corona_client_response throw_selection(std::string input_name)
			{
				corona_client_response response;
				auto player = attach_player(input_name);
				if (player) {
					response.success = true;
					player->throw_selection(this);
				}
				return response;
			}

			corona_client_response drop_selection(std::string input_name)
			{
				corona_client_response response;
				auto player = attach_player(input_name);
				if (player) {
					response.success = true;
					player->drop_selection(this);
				}
				return response;
			}

			corona_client_response use_selection(std::string input_name)
			{
				corona_client_response response;
				auto player = attach_player(input_name);
				if (player) {
					response.success = true;
					player->use_selection(this);
				}
				return response;
			}

			corona_client_response select_next(std::string input_name)
			{
				corona_client_response response;
				auto player = attach_player(input_name);
				if (player) {
					response.success = true;
					player->select_next();
				}
				return response;

			}

			corona_client_response select_previous(std::string input_name)
			{
				corona_client_response response;
				auto player = attach_player(input_name);
				if (player) {
					response.success = true;
					player->select_previous();
				}
				return response;
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

			virtual void get_json(json& _dest)
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

			virtual void put_json(json& _src)
			{
				name = _src["name"].as_string();
				description = _src["description"].as_string();
				json j = _src["map"];
				game_map = std::make_shared<map>();
				std::string state_string = _src["state"].as_string();
			}

			job* get_next_job()
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

		private:


			double last_elapsed_seconds;

			DirectX::XMVECTOR zero_vector = {};

			std::shared_ptr<player> attach_player(std::string input_name)
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


			collision_event model_piece(std::shared_ptr<map> _map, int _piece_index, double _elapsed_secs)
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
				map = std::make_shared<map>();
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

		void piece::use(game* _game, piece* _piece)
		{

				
		}
		
		// the user uses the piece, with a consumable piece
		void piece::load(game* _game, piece* _user, chest_item& _item)
		{
			;
		}

		// the user uses the piece, with a consumable piece
		void piece::unload(game* _game, piece* _user, chest_item& _item)
		{

		}

		// the user drops the selection
		void piece::drop(game* _game, chest_item& _item)
		{

		}

		// the user picks up the obect
		void piece::pickup(game* _game, piece* _user, piece* _pickup)
		{

		}

		// this gets called on every piece, to apply accelerations, make animation calculations, 
		// and other time-based calculations. The default implementation does nothing.
		// _elapsed_seconds is the time since the last call to run, in seconds.
		// this will be quite fractional.
		void run(game* _game, piece* _user, double _elapsed)
		{

		}

		void actor::select_next()
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

		void actor::select_previous()
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

		void actor::extend_selection(chest_item* _ci)
		{
			if (selection && _ci)
			{
				selection->extend(*_ci);
			}
		}

		void actor::clear_selection()
		{
			if (selection)
			{
				selection->clear();
			}
		}

		selection_analysis actor::analyze_selected(game* _game)
		{
			selection_analysis sa;

			sa = {};

			if (selection)
			{

				for (auto itm : selection->selections)
				{
					auto piece = _game->factories.piece_factory.get_object(corona_instance::local, itm.part_class, itm.part_id, true);

					if (sa.selected_piece == nullptr)
					{
						sa.selected_piece = piece.get();
					}

					if (std::is_base_of_v<tool, decltype(*piece)>) {
						sa.selected_tool = dynamic_cast<tool*>(piece.get());
					}
					else if (std::is_base_of_v<consumable, decltype(*piece)>) {
						sa.selected_magazine = dynamic_cast<magazine*>(piece.get());
					}
					else if (std::is_base_of_v<ammunition, decltype(*piece)>) {
						sa.selected_ammunition = dynamic_cast<ammunition*>(piece.get());
					}
					else if (std::is_base_of_v<wand, decltype(*piece)>) {
						sa.selected_wand = dynamic_cast<wand*>(piece.get());
					}
					else if (std::is_base_of_v<spell, decltype(*piece)>) {
						sa.selected_spell = dynamic_cast<spell*>(piece.get());
					}
				}
			}

			return sa;
		}

		void actor::use_selection(game* _game)
		{
            selection_analysis sa = analyze_selected();

            if (sa.selected_tool) {
				sa.selected_tool->use(_game, this);
			}
			else if (sa.selected_wand) {
				sa.selected_wand->use(_game, this);
			}
			else if (sa.selected_magazine) {
				sa.selected_magazine->use(_game, this);
			}
			else if (sa.selected_ammunition) {
				sa.selected_ammunition->use(_game, this);
			}
			else if (sa.selected_spell) {
				sa.selected_spell->use(_game, this);
			}
		}

		void actor::throw_selection(game* _game)
		{
			if (selection)
			{
				for (auto itm : selection->selections)
				{
					_game->factories.piece_factory.get_object(corona_instance::local, itm.part_class, itm.part_id, true);
				}
			}
		}

		void actor::drop_selection(game* _game)
		{
			if (selection)
			{
				for (auto itm : selection->selections)
				{
					_game->factories.piece_factory.get_object(corona_instance::local, itm.part_class, itm.part_id, true);
				}
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

		void firearm::use(game* _game, piece* _piece)
		{
			;
		}


	}

}
