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

		class piece_code : public corona_object
		{
		public:
			std::string piece_code;
            std::string piece_class;
			std::string piece_type;

			virtual void get_json(json& _dest) const
			{
				json_parser jp;

				corona_object::get_json(_dest);
				_dest.put_member("piece_code", piece_code);
				_dest.put_member("piece_class", piece_class);
				_dest.put_member("piece_type", piece_type);
			}

			virtual void put_json(json& _src)
			{
				corona_object::put_json(_src);
                piece_code = _src["piece_code"].as_string();
                piece_class = _src["piece_class"].as_string();
                piece_type = _src["piece_type"].as_string();
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

			virtual void get_json(json& _dest)
			{
				json_parser jp;

				corona_object::get_json(_dest);

                _dest.put_member("name", name);
                _dest.put_member("description", description);
				auto jbm = jp.create_object();
                bitmap.put_json(jbm);
                _dest.put_member("image", jbm);
			}

			virtual void put_json(json& _src)
			{
				corona_object::put_json(_src);

				json jbm = _src["image"];
				bitmap.put_json(jbm);
				name = _src["name"].as_string();
                description = _src["description"].as_string();
			}

		};

		class essay : public activity
		{
		public:
			std::string         text;

			essay() {
				class_name = "essay";
			}
			essay(const essay& _src) = default;
			essay(essay&& _src) = default;
			essay& operator =(const essay& _src) = default;
			essay& operator =(essay&& _src) = default;

			virtual void get_json(json& _dest)
			{
				json_parser jp;

				corona_object::get_json(_dest);
				_dest.put_member("name", name);
				_dest.put_member("description", description);
				auto jbm = jp.create_object();
				bitmap.put_json(jbm);
				_dest.put_member("image", jbm);
			}

			virtual void put_json(json& _src)
			{
				corona_object::put_json(_src);

				json jbm = _src["image"];
				bitmap.put_json(jbm);
				name = _src["name"].as_string();
				description = _src["description"].as_string();
			}

		};

		class timepoint_change : public corona_object
		{
		public:

			timepoint_change() {
				class_name = "timepoint_change";
			}
			timepoint_change(const timepoint_change& _src) = default;
			timepoint_change(timepoint_change&& _src) = default;
			timepoint_change& operator =(const timepoint_change& _src) = default;
			timepoint_change& operator =(timepoint_change&& _src) = default;

			virtual void get_json(json& _dest) const
			{
				json_parser jp;

				corona_object::get_json(_dest);
			}

			virtual void put_json(json& _src)
			{
				corona_object::put_json(_src);
			}
		};

		class value_change : public timepoint_change
		{
		public:

			std::string		frame_name;
			json_object	    new_value;

			value_change() {
				class_name = "value_change";
			}
			value_change(const value_change& _src) = default;
			value_change(value_change&& _src) = default;
			value_change& operator =(const value_change& _src) = default;
			value_change& operator =(value_change&& _src) = default;

			virtual void get_json(json& _dest) const
			{
				json_parser jp;

				timepoint_change::get_json(_dest);
				_dest.put_member_string("frame_name", frame_name);
                _dest.put_member("new_value", new_value);
			}

			virtual void put_json(json& _src)
			{
				corona_object::put_json(_src);

				new_value = to_object(_src["new_value"]);
                frame_name = _src["frame_name"].as_string();
			}

		};

		class audio_change : public timepoint_change
		{
		public:

			audio_function      sound;

			audio_change() {
				class_name = "audio_change";
			}
			audio_change(const audio_change& _src) = default;
			audio_change(audio_change&& _src) = default;
			audio_change& operator =(const audio_change& _src) = default;
			audio_change& operator =(audio_change&& _src) = default;

			virtual void get_json(json& _dest) const
			{
				json_parser jp;

				timepoint_change::get_json(_dest);
			}

			virtual void put_json(json& _src)
			{
				timepoint_change::put_json(_src);
				
                json jsound = _src["sound"];
                sound = audio_graph::from_json(jsound);
			}

		};

		class timepoint : public corona_object
		{
		public:

			double elapsed_seconds;
            std::vector<std::shared_ptr<timepoint_change>> changes;

			timepoint() {
				class_name = "timepoint";
			}
			timepoint(const timepoint& _src) = default;
			timepoint(timepoint&& _src) = default;
			timepoint& operator =(const timepoint& _src) = default;
			timepoint& operator =(timepoint&& _src) = default;

			virtual void get_json(json& _dest) const
			{
				json_parser jp;

				corona_object::get_json(_dest);

                _dest.put_member("elapsed_seconds", elapsed_seconds);
                json jchanges = jp.create_array();

				for (auto item : changes) {
                    json jitem = jp.create_object();
                    item->get_json(jitem);
                    jchanges.push_back(jitem);
				}
				_dest.put_member("changes", jchanges);
			}

			virtual void put_json(json& _src)
			{
				corona_object::put_json(_src);
				elapsed_seconds = _src["elapsed_seconds"].as_double();
				changes.clear();
                json jchanges = _src["changes"];
				if (jchanges.array()) {
					for (int i = 0; i < jchanges.size(); i++) {
						json jchange = jchanges.get_element(i);
						auto change = db->construct_as<timepoint_change>(jchange);
						change->put_json(jchange);
						changes.push_back(change);
					}
				}
			}

		};

		class frame : public corona_object
		{
		public:
			std::string			name;
            DirectX::XMVECTOR   position = {};
			double				rotation = 0.0;
			DirectX::XMVECTOR   pivot = {};
			double				total_seconds = 0.0;
			rectangle			view_box = {};

			corona_object_history history;

			frame() {
				class_name = "frame";
			}
			frame(const frame& _src) = default;
			frame(frame&& _src) = default;
			frame& operator =(const frame& _src) = default;
			frame& operator =(frame&& _src) = default;

			virtual void get_json(json& _dest) const	
			{
				json_parser jp;

				corona_object::get_json(_dest);
				_dest.put_member("name", name);
				_dest.put_member("position", position);
				_dest.put_member("rotation", rotation);
				_dest.put_member("pivot", pivot);
                _dest.put_member("view_box", view_box);
			}

			virtual void put_json(json& _src)
			{
				corona_object::put_json(_src);
				name = _src["name"].as_string();
				position = _src["position"].as_vector();
				rotation = _src["rotation"].as_double();
				pivot = _src["pivot"].as_vector();
				view_box = _src["view_box"].as_rectangle();
				history.set_original(_src);
				if (view_box.h > 0) {
					DebugBreak();
				}
				if (view_box.h == 0 || view_box.w == 0) {
					rectangle_points this_size;
					extent(this_size);
					double scale = this_size.scale_zero();
					view_box.w = scale;
					view_box.h = scale;
				}
			}

			virtual std::shared_ptr<corona_object_interface> clone() const
			{
				std::shared_ptr<frame> new_frame = std::make_shared<frame>(*this);
				return new_frame;
			}

			virtual void set_history()
			{
				history.clear();
				history.set_tween_fields({ "position", "rotation" });
			}

            virtual void put_change(double _elapsed_seconds, value_change& vc)
			{
				corona_object_timepoint cotp;
				cotp.elapsed_seconds = _elapsed_seconds;
				cotp.values = vc.new_value;				
				history.put_change(cotp);
			}

            virtual void set_time(double _elapsed_seconds)
			{
				total_seconds += _elapsed_seconds;
				json jstate = history.get_time(total_seconds);
				put_json(jstate);
			}

			virtual void draw_item(direct2dContext& _context)
			{
				;
			}

			virtual rectangle_points& extent(rectangle_points& _src)
			{
				return _src;
			}

			virtual void draw(direct2dContext& _context, rectangle* _location)
			{
				using namespace DirectX;

				try {

					if (!_location)
						return;
				
					D2D1_MATRIX_3X2_F existing;

					_context.getDeviceContext()->GetTransform(&existing);

					D2D1_SIZE_F image_scale;

					image_scale.width = _location->w / view_box.w;
					image_scale.height = _location->h / view_box.h;

					D2D1_POINT_2F scale_point = { 0, 0 };

					auto mscale = D2D1::Matrix3x2F::Scale(
						image_scale,
						scale_point
					);

					D2D1_POINT_2F rotation_point;

                    rotation_point.x = DirectX::XMVectorGetX(pivot);
					rotation_point.y = DirectX::XMVectorGetY(pivot);;

					auto mrotation = D2D1::Matrix3x2F::Rotation(
						rotation,
						rotation_point
					);

//					bus->log_information(std::format("{0} secs, {1} {2} degrees", total_seconds, name, rotation), __FILE__, __LINE__);				

					auto mtranslation = D2D1::Matrix3x2F::Translation(
						_location->x,
						_location->y
                    );

					auto mat = mrotation * mscale *  mtranslation;

					_context.getDeviceContext()->GetTransform(&existing);
					_context.getDeviceContext()->SetTransform(&mat);

					draw_item(_context);

					_context.getDeviceContext()->SetTransform(&existing);

				}
				catch (std::exception exc)
				{
					;
				}

			}

            virtual void create_assets(direct2dContext& _context)
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

            virtual void get_json(json& _dest) const	
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

			virtual void draw_item(direct2dContext& _context)
			{
				bitmap.x = 0;
				bitmap.y = 0;

				_context.drawBitmap(&bitmap);
			}

			virtual std::shared_ptr<corona_object_interface> clone() const
			{
				std::shared_ptr<bitmap_frame> new_frame = std::make_shared<bitmap_frame>(*this);
				return new_frame;
			}

			virtual rectangle_points& extent(rectangle_points& _src)
			{
				_src.extend(point{ 0, 0 });
                _src.extend(point{ bitmap.x, bitmap.y });
				return _src;

			}

			virtual void create_assets(direct2dContext& _context)
			{

                if (_context.hasBitmap(bitmap.bitmapName)) {
					return;
				}

				if (bitmap.bitmapPath.size() == 0) {
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

			virtual void get_json(json& _dest) const	
			{
				json_parser jp;
				frame::get_json(_dest);
				json jpath = jp.create_object();
				path.get_json(jpath);
				_dest.put_member("shape", jpath);
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
				json jpath = _src["shape"];
				path.put_json(jpath);
				json jfill = _src["fill"];
				fill.put_json(jfill);
				json jstroke = _src["stroke"];
				stroke.put_json(jstroke);
				stroke_width = _src["stroke_width"].as_double();
                if (path.name.empty()) {
					path.name = "frame_path_" + std::to_string(object_id);
				}
				if (!*fill.get_name()) {
					fill.set_name( "frame_fill_" + std::to_string(object_id) );
				}
				if (!*stroke.get_name()) {
					stroke.set_name( "frame_stroke_" + std::to_string(object_id));
				}
			}

			virtual void draw_item(direct2dContext& _context)
			{
				pathImmediateDto pid;
				pid.path = path.clone();
				pid.fillBrushName = fill.get_name();
				pid.borderBrushName = stroke.get_name();
				pid.rotation = 0;
				pid.strokeWidth = stroke_width;
				pid.closed = true;
				_context.drawPath(&pid);
			}

			virtual void create_assets(direct2dContext& _context)
			{
				_context.setBrush(&fill);
				_context.setBrush(&stroke);
			}

			virtual rectangle_points& extent(rectangle_points& _src)
			{
                path.extent(_src);
				return _src;
			}

			virtual std::shared_ptr<corona_object_interface> clone() const
			{
				std::shared_ptr<vector_frame> new_frame = std::make_shared<vector_frame>(*this);
				return new_frame;
			}

		};

		class animation : public corona_object
		{
		public:
            std::string										name;
            rectangle										destination;
			DirectX::XMVECTOR								direction;
            std::map<std::string, std::shared_ptr<frame>>	frames;
            std::map<double, std::shared_ptr<timepoint>>	timeline;
			double                                          total_animation_time;

			animation() {
                class_name = "animation";
				total_animation_time = (std::rand() % 100) *.01;
			}

			animation(const animation& _src) = default;
			animation(animation&& _src) = default;
			animation& operator =(const animation& _src) = default;
			animation& operator =(animation&& _src) = default;

			virtual void draw(direct2dContext& _context, rectangle* _location)
			{
				if (frames.size() == 0)
					return;			

				for (auto f : frames)
				{
					f.second->draw(_context, _location);
				}
			}

            virtual void set_time(double _elapsed)
			{
				total_animation_time += _elapsed;
				for (auto f : frames)
				{
					f.second->set_time(_elapsed);
				}
			}

			virtual void put_json(json& _src)
			{
				json_parser jp;

				corona_object::put_json(_src);

				name = _src["name"].as_string();
				destination = _src["destination"].as_rectangle();
				direction = _src["direction"].as_vector();
				
				json jframes = _src["frames"];
				frames.clear();

				if (jframes.array()) {
					for (int i = 0; i < jframes.size(); i++) {
						auto jframe = jframes.get_element(i);
						auto new_frame = db->construct_as<frame>(jframe);
						if (new_frame) {
							frames[new_frame->name] = new_frame;
						}
					}
				}

				json jtimeline = _src["timeline"];
				timeline.clear();

				double total_time = 0.0;
				if (jtimeline.array()) {
					for (int i = 0; i < jtimeline.size(); i++) {
						auto jchange = jtimeline.get_element(i);
						if (!jchange.object()) {
							continue;
						}
						auto new_change = db->construct_as<timepoint>(jchange);
						if (new_change) {
							new_change->put_json(jchange);
							timeline[new_change->elapsed_seconds] = new_change;
						}
					}
				}

				for (auto tl : timeline) {
					for (auto change : tl.second->changes) {
                        auto vc = std::dynamic_pointer_cast<value_change>(change);
                        if (vc) {
                            auto frames_it = frames.find(vc->frame_name);
                            if (frames_it != frames.end()) {
								json initial_values = jp.create_object();
								auto frame = frames_it->second;
								frame->put_change(tl.first, *vc);
							}
						}
					}
                }
			}

			virtual void get_json(json& _dest) const
			{
				corona_object::get_json(_dest);
				_dest.put_member("name", name);
				_dest.put_member("destination", destination);
				_dest.put_member("direction", direction);
				json_parser jp;
				json jframes = jp.create_array();
				for (auto& frame : frames) {
					json jframe = jp.create_object();
					frame.second->get_json(jframe);
					jframes.push_back(jframe);
				}
				_dest.put_member("frames", jframes);

                json jtimeline = jp.create_array();
				for (auto& tl : timeline) {
					json jtimepoint = jp.create_object();
					tl.second->get_json(jtimepoint);
					jtimeline.push_back(jtimepoint);
				}
                _dest.put_member("timeline", jtimeline);
			}

			virtual void create_assets(direct2dContext& _context)
			{
				for (auto f : frames)
				{
					f.second->create_assets(_context);
				}
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
			double		health = 1.0;
			double      full_health = 1.0;
			std::string state;
			json_object	defaults;

			std::shared_ptr<chest_field> inventory;

			virtual void get_json(json& _dest) const
			{
				json_parser jp;

				corona_object::get_json(_dest);

				_dest.put_member("name", name);
				_dest.put_member("description", description);
				_dest.put_member("piece_class_name", piece_class_name);

				json j = jp.create_array();
				for (auto& s : animations) {
					json jsprite = jp.create_object();
					s->get_json(jsprite);
					j.push_back(jsprite);
				}
				_dest.put_member("animations", j);

				_dest.put_member("mass", mass);
				_dest.put_member("health", health);
				_dest.put_member("full_health", full_health);
				_dest.put_member("state", state);
				_dest.put_member("defaults", defaults);

				json jinventory = jp.create_object();

				if (inventory) {
					inventory->get_json(jinventory);
				}

			}

			virtual void put_json(json& _src)
			{
				corona_object::put_json(_src);

				name = _src["name"].as_string();
				description = _src["description"].as_string();
				piece_class_name = _src["piece_class_name"].as_string();

				json janimations = _src["animations"];
				animations.clear();
				for (int i = 0; i < janimations.size(); i++) {
					auto janimation = janimations.get_element(i);
					if (!janimation.object()) {
						continue;
					}
					auto new_animation = db->construct_as<animation>(janimation);
                    if (new_animation) {
                        animations.push_back(new_animation);
                    }
				}

				mass = _src["mass"].as_double();
				state = _src["state"].as_string();
                health = _src["health"].as_double();
                full_health = _src["full_health"].as_double();
                state = _src["state"].as_string();
                defaults = _src["defaults"].as_object();

				if (fabs(mass) < 0.0001) {
					mass = 1.0;
				}

				inventory = std::make_shared<chest_field>();
				if (_src.has_member("inventory")) {
					json jinventory = _src["inventory"];
					inventory->put_json(jinventory);
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
			DirectX::XMVECTOR position = {};
			DirectX::XMVECTOR size = {};
			DirectX::XMVECTOR velocity = {};
			DirectX::XMVECTOR facing = {};

			std::shared_ptr<workflow_field> workflow;
			std::vector<std::shared_ptr<animation>> animations;

			object_reference owner;

			double		mass = 1.0;
			double      full_health = 1.0;
			double      health = 1.0;
			bool		remove = false;
			double		state_time = 0.0;

			template <typename T> bool is()
			{
				T* d = dynamic_cast<T*>(this);
				return d != nullptr;
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


			virtual void put_json(json& _src)
			{
				corona_object::put_json(_src);

				name = _src["name"].as_string();
				state = _src["state"].as_string();
				position = _src["position"].as_vector();
				size = _src["size"].as_vector();
				velocity = _src["velocity"].as_vector();
				facing = _src["facing"].as_vector();

				mass = _src["mass"].as_double();
				full_health = _src["full_health"].as_double();
				health = _src["health"].as_double();
				remove = _src["remove"].as_bool();
				state_time = _src["state_time"].as_double();

				json janimations = _src["animations"];
				animations.clear();
				for (int i = 0; i < janimations.size(); i++) {
					auto janimation = janimations.get_element(i);
					if (!janimation.object()) {
						continue;
					}
					auto new_animation = db->construct_as<animation>(janimation);
					if (new_animation) {
						animations.push_back(new_animation);
					}
				}

                json jworkflow = _src["workflow"];
				if (jworkflow.object()) {
					workflow = std::make_shared<workflow_field>(jworkflow);
				}
			}

			virtual void get_json(json& _dest) const
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
				_dest.put_member("full_health", full_health);
				_dest.put_member("health", health);
				_dest.put_member("remove", remove);
				_dest.put_member("owner", owner);
				_dest.put_member("state_time", state_time);

				json j = jp.create_array();
				for (auto& s : animations) {
					json jsprite = jp.create_object();
					s->get_json(jsprite);
					j.push_back(jsprite);
				}
				_dest.put_member("animations", j);

				if (workflow) {
					json jworkflow = jp.create_object();
					workflow->get_json(jworkflow);
					_dest.put_member("workflow", jworkflow);
				}
			}

			virtual void run(double _delta, double _elapsed)
			{
				position = DirectX::XMVectorAdd(position, DirectX::XMVectorScale(velocity, static_cast<float>(_delta)));
				for (auto anim : animations) {
                    anim->set_time(_delta);
				}
			}

			virtual void set_time(double _delta)
			{
				for (auto anim : animations) {
					anim->set_time(_delta);
				}
			}


			virtual void create_assets(direct2dContext& _context)
			{
				for (auto a : animations)
				{
					a->create_assets(_context);
				}
			}

			virtual void draw(direct2dContext& _context, rectangle* _location)
			{
				for (auto animation : animations) {
					if (animation->name == state) {
						animation->draw(_context, _location);
					}
				}
			}

		};


		using piece_collection = std::map<object_reference, std::shared_ptr<piece>>;
		using piece_collection_iterator = std::map<object_reference, std::shared_ptr<piece>>::iterator;

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
            std::map<object_reference, std::shared_ptr<piece>> pieces;

			virtual void get_json(json& _dest) const
			{
				json_parser jp;

				corona_object::get_json(_dest);
				_dest.put_member("state", state);
				json j = jp.create_array();
				for (const auto& [id, p] : pieces) {
                    json pj = jp.create_object();
                    p->get_json(pj);
                    j.push_back(pj);
				}
				_dest.put_member("pieces", j);
			}

			virtual void put_json(json& _src)
			{
				corona_object::put_json(_src);
				state = _src["state"].as_string();
                json jpieces = _src["pieces"];
				if (jpieces.array()) {
					pieces.clear();
					for (int i = 0; i < jpieces.size(); i++) {
						auto jpiece = jpieces.get_element(i);
						if (!jpiece.object()) {
							continue;
						}
						auto new_piece = db->construct_as<piece>(jpiece);
						new_piece->put_json(jpiece);
						pieces[new_piece->to_reference()] = new_piece;
                    }
				}
			}

			piece_collection_iterator begin() { return pieces.begin(); }
			piece_collection_iterator end() { return pieces.end(); }
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

			function_scheduler<piece *> scheduler;


			virtual void get_json(json& _dest) const
			{
				json_parser jp;
				piece::get_json(_dest);
				json jtimer = jp.create_object();
				scheduler.get_json(jtimer);
                _dest.put_member("scheduler", jtimer);
			}

			virtual void put_json(json& _src)
			{
				piece::put_json(_src);
				json timer = _src["scheduler"];
				scheduler.put_json(timer);
			}

		};

		class use_plate : public feature
		{
		public:

			use_plate() {
				class_name = "use_plate";
			}
			use_plate(const use_plate& _src) = default;
			use_plate(use_plate&& _src) = default;
			use_plate& operator =(const use_plate& _src) = default;
			use_plate& operator =(use_plate&& _src) = default;

			object_reference target_piece;

			virtual void get_json(json& _dest) const
			{
				json_parser jp;
				piece::get_json(_dest);
                _dest.put_member("target_piece", target_piece);
			}

			virtual void put_json(json& _src)
			{
				piece::put_json(_src);
                target_piece = _src["target_piece"].as_object_reference();
			}

		};

		class effect : public piece
		{
		public:

			effect() {
				class_name = "feature";
			}

			effect(const effect& _src) = default;
			effect(effect&& _src) = default;
			effect & operator=(const effect& _src) = default;
			effect & operator=(effect&& _src) = default;

			virtual void get_json(json& _dest) const
			{
				piece::get_json(_dest);
			}

			virtual void put_json(json& _src)
			{
				piece::put_json(_src);
			}
		};

		class wound_effect : public effect
		{
		public:
			wound_effect() {
				class_name = "wound_effect";
			}

			double hit_points_sec = 0.0;
			double duration_seconds = 0.0;
			double effect_remaining = 0.0;
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

			virtual void get_json(json& _dest) const	
			{
				feature::get_json(_dest);
			}

			virtual void put_json(json& _src)
			{
				feature::put_json(_src);
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

			virtual void get_json(json& _dest) const
			{
				spawn::get_json(_dest);
			}

			virtual void put_json(json& _src)
			{
				spawn::put_json(_src);
			}
		};

		class npc_spawn : public spawn
		{
		public:

			function_scheduler<npc_spawn *> scheduler;
			std::vector<std::string> spawn_classes;

			npc_spawn() {
				class_name = "npc_spawn";
			}
			npc_spawn(const npc_spawn& _src) = default;
			npc_spawn(npc_spawn&& _src) = default;
			npc_spawn& operator =(const npc_spawn& _src) = default;
			npc_spawn& operator =(npc_spawn&& _src) = default;

			virtual void get_json(json& _dest) const
			{
				spawn::get_json(_dest);
				json_parser jp;
				json j = jp.vector_to_jarray(spawn_classes);
				_dest.put_member("spawn_classes", j);
                json jscheduler = jp.create_object();
				scheduler.get_json(jscheduler);
                _dest.put_member("task_scheduler", jscheduler);
			}

			virtual void put_json(json& _src)
			{
				spawn::put_json(_src);
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
				return result;
			}

			virtual bool is_spawn_time(double _delta, double _elapsed)
			{
                spawn::run(_delta, _elapsed);
				return scheduler.execute(_delta);
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

			virtual void put_json(json& _src)
			{
				feature::put_json(_src);
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

			virtual void put_json(json& _src)
			{
				feature::put_json(_src);
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

			virtual void put_json(json& _src)
			{
				piece::put_json(_src);
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
            std::map<std::string, std::shared_ptr<piece_list>> sets;

			virtual void get_json(json& _dest)
			{
				feature::get_json(_dest);
			}

			virtual void put_json(json& _src)
			{
				feature::put_json(_src);

                json jsets = _src["sets"];

                auto members = jsets.get_members();

				for (const auto& member : members) {
					if (member.second.array()) {
                        for (int i = 0; i < member.second.size(); i++) 
						{
                            auto piece_json = member.second.get_element(i);
                            auto piece_ptr = std::make_shared<piece>();
                            piece_ptr->put_json(piece_json);
							auto key = piece_ptr->to_reference();
							sets[member.first]->pieces[key] = piece_ptr;
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

			virtual void put_json(json& _src)
			{
				feature::put_json(_src);
			}

			// the player uses the piece, and then, 
			// that changes the state.
			// the animation system picks this up while rendering
			virtual void use(piece* _piece)
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

			virtual void put_json(json& _src)
			{
				piece::put_json(_src);
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

			virtual void put_json(json& _src)
			{
				piece::put_json(_src);
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

			virtual void put_json(json& _src)
			{
				piece::put_json(_src);
                is_on = _src["is_on"].as_bool();
			}

			// the user uses the piece, which may change its state or cause it to be consumed. The default implementation does nothing and returns a copy of the piece.
			virtual void use(piece* _piece)
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

			virtual void put_json(json& _src)
			{
				light::put_json(_src);
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

			virtual void put_json(json& _src)
			{
				feature::put_json(_src);
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
				feature::get_json(_dest);
			}

			virtual void put_json(json& _src)
			{
				feature::put_json(_src);
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

			virtual void put_json(json& _src)
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

			virtual void put_json(json& _src)
			{
				carryable::put_json(_src);
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

			virtual void put_json(json& _src)
			{
				carryable::put_json(_src);
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

			virtual void put_json(json& _src)
			{
				piece::put_json(_src);
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

			virtual void get_json(json& _dest)
			{
				carryable::get_json(_dest);
				_dest.put_member("ammunition_type", ammunition_type);
				_dest.put_member("shot_type", shot_type);
				_dest.put_member("shot_muzzle_velocity", shot_muzzle_velocity);
				_dest.put_member("shot_mass", shot_mass);
			}

			virtual void put_json(json& _src)
			{
				carryable::put_json(_src);
				ammunition_type = _src["ammunition_type"].as_string();
				shot_type = _src["shot_type"].as_string();
				shot_muzzle_velocity = _src["shot_muzzle_velocity"].as_double();
                shot_mass = _src["shot_mass"].as_double();
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

			virtual void put_json(json& _src)
			{
				json_parser jp;

				carryable::put_json(_src);
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

			virtual void put_json(json& _src)
			{
				json_parser jp;
				carryable::put_json(_src);

				magazine_type = _src["magazine_type"].as_string();
                fire_point = _src["fire_point"].as_vector();
			}


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

			virtual void put_json(json& _src)
			{
				tool::put_json(_src);
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

			virtual void put_json(json& _src)
			{
				carryable::put_json(_src);
				effect_type = _src["effect_type"].as_string();
				spell_velocity = _src["spell_velocity"].as_double();
                spell_type = _src["spell_type"].as_string();
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

			virtual void put_json(json& _src)
			{
				tool::put_json(_src);
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

			virtual void put_json(json& _src)
			{
				piece::put_json(_src);
				effect_type = _src["effect_type"].as_string();
				hit_type = _src["hit_type"].as_string();
                hit_lifetime = _src["hit_lifetime"].as_double();
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

			virtual void put_json(json& _src)
			{
				tool::put_json(src);
			}
		};

		enum class actor_directions
		{
			up,
			down,
			left,
			right
		};

		enum class actor_statuses
		{
			waiting,
			playing,
			dead
		};

		enum class actor_actions
		{
			standing,
			walking,
			throwing,
			attacking
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

			bool									ready;
			bool									dead;

			actor_directions	actor_direction;
            actor_statuses		actor_status;
            actor_actions       actor_action;

			double stopped_velocity;
			double walking_velocity;
			double throwing_velocity;

			virtual void get_json(json& _dest)
			{
				piece::get_json(_dest);
				_dest.put_member_bool("ready", ready);
				_dest.put_member_bool("dead", dead);
				_dest.put_member("stopped_velocity", 0.0);
				_dest.put_member("walking_velocity", walking_velocity);
				_dest.put_member("throwing_velocity", throwing_velocity);
			}

			virtual void put_json(json& _src)
			{
				piece::put_json(_src);
				ready = _src["ready"].as_bool();
				dead = _src["dead"].as_bool();
				stopped_velocity = _src["stopped_velocity"].as_double();
				walking_velocity = _src["walking_velocity"].as_double();
				throwing_velocity = _src["throwing_velocity"].as_double();
			}

			virtual void select_next();
			virtual void select_previous();
			virtual void extend_selection(object_reference rf);
			virtual void clear_selection();

			virtual std::shared_ptr<piece> fire();
			virtual std::shared_ptr<piece> use();
			virtual std::shared_ptr<piece> hit();
			virtual std::shared_ptr<piece> cast();

			float get_travel_velocity()
			{
				switch (actor_action)
				{
					case actor_actions::standing:
                        return 0.0;
					case actor_actions::walking:
                        return walking_velocity;
					case actor_actions::throwing:
						return 0.0;
					case actor_actions::attacking:
						return 0.0;
				}
			}

			virtual void move_left()
			{
                float d = get_travel_velocity();
				DirectX::XMVECTOR left = DirectX::XMVectorSet(-d, 0.0f, 0.0f, 0.0f);				
                actor_direction = actor_directions::left;
			}

			virtual void move_right()
			{
				float d = get_travel_velocity();
				DirectX::XMVECTOR right = DirectX::XMVectorSet(d, 0.0f, 0.0f, 0.0f);
                actor_direction = actor_directions::right;
			}

			virtual void move_foward()
			{
				float d = get_travel_velocity();
				DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, d, 0.0f, 0.0f);
                actor_direction = actor_directions::up;
			}

			virtual void move_back()
			{
				float d = get_travel_velocity();
				DirectX::XMVECTOR down = DirectX::XMVectorSet(0.0f, -d, 0.0f, 0.0f);
				actor_direction = actor_directions::down;
			}

			void walk()
			{
				actor_action = actor_actions::walking;
			}

			void stop()
			{
				actor_action = actor_actions::standing;
			}

		};

		enum player_input {
			input_keymouse,
			input_gamepad0,
			input_gamepad1,
			input_gamepad2,
			input_gamepad3,
		};

		class player : public actor
		{
		public:

			bool active;
			player_input input_device;	

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

			virtual void put_json(json& _src)
			{
				actor::put_json(_src);
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

			virtual void put_json(json& _src)
			{
				actor::put_json(_src);
			}
		};

        class set : public corona_object
		{
		public:

			set() 
			{
				class_name = "set";
			}
			set(const set& _src) = default;
			set(set&& _src) = default;
			set& operator =(const set& _src) = default;
			set& operator =(set&& _src) = default;

			std::string name;
			std::string description;

			std::shared_ptr<piece_list> current;
			std::shared_ptr<piece_list> start;

			std::vector<std::string> tile_strings;
			DirectX::XMVECTOR tile_size;
            std::map<std::string, piece_code> tile_codes;

			// C++20 view functions - no separate storage needed
			template<std::derived_from<piece> T>
			auto get_pieces_of_type() const {
				return current->pieces
					| std::views::transform([](const auto& p) {
							return std::dynamic_pointer_cast<T>(p.second);
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
                corona_object::get_json(_dest);

				_dest.put_member("name", name);
                _dest.put_member("description", description);

				json_parser jp;
				json ts = jp.create_array(tile_strings);

                _dest.put_member("tile_strings", ts);
                _dest.put_member("tile_size", tile_size);

                json jabbreviations = jp.create_array();
                for (const auto& [key, value] : tile_codes) {
                    json jabbreviation = jp.create_object();
                    value.get_json(jabbreviation);
                    jabbreviations.push_back(jabbreviation);
                }
                _dest.put_member("tile_codes", jabbreviations);

				json piece_list_array = jp.create_array();

				if (current) {
					json jcurrent = jp.create_object();
					current->get_json(jcurrent);
					piece_list_array.push_back(jcurrent);
				}

				if (start) {
					json jstart = jp.create_object();
					start->get_json(jstart);
					piece_list_array.push_back(jstart);
				}
                _dest.put_member("contents", piece_list_array);
				
			}

			virtual void put_json(json& _src)
			{
                corona_object::put_json(_src);

				name = _src["name"].as_string();
                description = _src["description"].as_string();

                tile_strings = _src["tile_strings"].to_string_array();
                tile_size = _src["tile_size"].as_vector();
                tile_codes.clear();

				auto jtile_codes = _src["tile_codes"].as_array();
				for (int i = 0; i < jtile_codes.size(); i++) {
					piece_code tc;
					json j = jtile_codes.get_element(i);
					if (j.object()) {
						tc.put_json(j);
						tile_codes[tc.piece_code] = tc;
					}
				}

				current = std::make_shared<piece_list>();
                start = std::make_shared<piece_list>();

                json jcontents = _src["contents"];
				if (jcontents.array()) {
					for (int i = 0; i < jcontents.size(); i++) {
						json j = jcontents.get_element(i);
						if (j.object()) {
							auto pl = std::make_shared<piece_list>();
							pl->put_json(j);
                            if (pl->state == "start") {
								start = pl;
							}
							else if (pl->state == "current") {
								current = pl;
							}
						}
					}
				}
			}

			virtual void construct(adventure_interface& _gbus)
			{
                current->pieces.clear();

				if (tile_strings.size() > 0 && tile_codes.size() > 0)
				{
                    DirectX::XMVECTOR icurrent = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
					DirectX::XMVECTOR irow = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
					DirectX::XMVECTOR icol = DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
                    DirectX::XMVECTOR tile_position = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);

					for (auto& ts : tile_strings) {
						auto row = split(ts, ' ');
						for (auto pa : row) 
						{
							if (tile_codes.find(pa) != tile_codes.end()) {
								auto& pc = tile_codes[pa];

								json request = 
								json result  = db->create_object();
								
								auto p = _gbus.create_piece_of_type(pc.piece_type);
								if (p) {
									p->position = DirectX::XMVectorMultiply(icurrent, tile_size);
									p->size = tile_size;
                                    auto pn = std::dynamic_pointer_cast<piece>(p);
                                    current->pieces.insert_or_assign(pn->to_reference(), pn);
								}
							}
                            else 
							{
								auto p = _gbus.create_piece_of_type(pa);
								if (p) {
									p->position = DirectX::XMVectorMultiply(icurrent, tile_size);
									p->size = tile_size;
									auto pn = std::dynamic_pointer_cast<piece>(p);
									current->pieces.insert_or_assign(pn->to_reference(), pn);
								}
							}
							icurrent = DirectX::XMVectorAdd(icurrent, icol);							
						}
                        DirectX::XMVectorSetX(icurrent, 0.0f);
						icurrent = DirectX::XMVectorAdd(icurrent, irow);
					}
				}

                for (auto [k,p] : start->pieces) {
					auto np = p->copy(corona_instance::local);
                    auto np_cast = std::dynamic_pointer_cast<piece>(np);
					current->pieces.insert_or_assign(np_cast->to_reference(), np_cast);
				}
			}

            void set_time(double _elapsed)
			{
				if (current) {
					for (auto [k, p] : current->pieces) {
						p->set_time(_elapsed);
					}
				}
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

			template <typename T> std::shared_ptr<T> get_piece()
			{
				if (piece_1 && piece_1->is<T>()) {
					return std::dynamic_pointer_cast<T>(piece_1);
				}
				else if (piece_2 && piece_2->is<T>()) {
					return std::dynamic_pointer_cast<T>(piece_2);
				}
				return nullptr;
			}
		};

		class collision_command_key
		{
		public:
			std::string piece1_class;
			std::string piece2_class;

			bool operator < (const collision_command_key& _other) const
			{
				return std::tie(piece1_class, piece2_class) < std::tie(_other.piece1_class, _other.piece2_class);
			}
		};

		class game : public corona_object
		{
		public:
			std::string name;
			std::string description;
			std::shared_ptr<set> stage;
			std::vector<std::shared_ptr<adventure_app_interface>> games;

			game() {
				class_name = "game";
			}
			game(const game& _src) = default;
			game(game&& _src) = default;
			game& operator =(const game& _src) = default;
			game& operator =(game&& _src) = default;

			virtual void get_json(json& _dest)
			{
				_dest.put_member("name", name);
				_dest.put_member("description", description);
				json_parser jp;
				json jmap = jp.create_object();
				stage->get_json(jmap);
				_dest.put_member("stage", jmap);
			}

			virtual void put_json(game_factory& _factory, json& _src)
			{
				name = _src["name"].as_string();
				description = _src["description"].as_string();
				json jmap = _src["stage"];
				stage->put_json(_factory, jmap);
			}
		};

		class adventure_base : public job, public corona_object
		{

		public:

			adventure_base(comm_desktop_bus_interface* _bus, json& _src) 
			{
				class_name = "adventure";
			}

			adventure_base(const adventure_base& _src) = default;
			adventure_base(adventure_base&& _src) = default;
			adventure_base& operator =(const adventure_base& _src) = default;
			adventure_base& operator =(adventure_base&& _src) = default;

			virtual void handle_keydown(key_down_event evt) = 0;
			virtual void handle_gamepad_button_up(gamepad_button_up_event gpbd) = 0;
			virtual void handle_gamepad_button_down(gamepad_button_down_event gpbd) = 0;
			virtual void handle_gamepad_trigger_up(gamepad_trigger_up_event gptu) = 0;
			virtual void handle_gamepad_trigger_down(gamepad_trigger_down_event gptd) = 0;
			virtual void handle_gamepad_thumbstick_move(gamepad_thumbstick_move_event gpbd) = 0;

			virtual void set_lobby() = 0;
			virtual void set_active() = 0;
			virtual void set_paused() = 0;
			virtual void set_complete() = 0;
			virtual void set_exit() = 0;

			virtual job* get_next_job() = 0;

			virtual void set_time(double _elapsed) = 0;

			virtual std::shared_ptr<corona_object_interface> get_piece(object_reference& _reference, bool include_children) = 0;

			player_input to_player_input(XINPUT_STATE _input_state)
			{
				switch (_input_state.dwPacketNumber) {
				case 0:
					return player_input::input_gamepad0;
				case 1:
					return player_input::input_gamepad1;
				case 2:
					return player_input::input_gamepad2;
				case 3:
					return player_input::input_gamepad3;
				}
                return player_input::input_gamepad3;
			}


			virtual void create_stage() = 0;
			virtual void create_assets(direct2dContext& _src) = 0;
			virtual	void draw(direct2dContext& _src) = 0;



		};

		class top_down_adventure : public adventure_base
		{

		public:

			top_down_adventure(comm_desktop_bus_interface* _bus, json& _src) : adventure_base(_bus, _src)
			{
				class_name = "top_down_adventure";
				instance = corona_instance::local;
				init();
				put_json(_src);
			}

			top_down_adventure(const top_down_adventure& _src) = default;
			top_down_adventure(top_down_adventure&& _src) = default;
			top_down_adventure& operator =(const top_down_adventure& _src) = default;
			top_down_adventure& operator =(top_down_adventure&& _src) = default;

            void handle_keydown(key_down_event evt) override;
			void handle_gamepad_button_up(gamepad_button_up_event gpbd) override;
			void handle_gamepad_button_down(gamepad_button_down_event gpbd) override;
			void handle_gamepad_trigger_up(gamepad_trigger_up_event gptu) override;
			void handle_gamepad_trigger_down(gamepad_trigger_down_event gptd) override;
			void handle_gamepad_thumbstick_move(gamepad_thumbstick_move_event gpbd) override;

			void manage_play(player_input input_device);
            void move_left(player_input input_device);
			void move_right(player_input input_device);
			void move_up(player_input input_device);
			void move_down(player_input input_device);
			void fire(player_input input_device);
			void stop(player_input input_device);
			void select_next(player_input input_device);
			void select_previous(player_input input_device);
			void use(player_input input_device);

			virtual void set_lobby() override;
			virtual void set_active() override;
			virtual void set_paused() override;
			virtual void set_complete() override;
			virtual void set_exit() override;
			virtual void get_json(json& _dest) override;
			virtual void put_json(json& _src) override;
			virtual job* get_next_job() override;

            virtual void set_time(double _elapsed);

			virtual std::shared_ptr<corona_object_interface> get_piece(object_reference& _reference, bool include_children);
            

			void create_stage();
			void create_assets(direct2dContext& _src);
			void draw(direct2dContext& _src);


		private:

			void init();

			corona_instance instance;

			lockable adventure_locker;

			std::shared_ptr<corona_desktop_bus_command> on_all_players_ready;
			std::shared_ptr<corona_desktop_bus_command> on_all_players_dead;

			comm_bus_interface* bus;
			std::string				name;
			std::string				description;
			timer					frame_timer;
			lockable				set_locker;
			std::shared_ptr<set>	stage;
			game_state				state;

			void check_all_ready();
			void check_all_dead();

			double last_elapsed_seconds;
            double last_delta_seconds;

			DirectX::XMVECTOR zero_vector = {};

			std::shared_ptr<player> find_player(player_input input_device);

			// This has to be const because we want the assertion that this doesn't modify the map or the pieces,
			// and only finds their state
			collision_event model_piece(std::shared_ptr<set> _map, piece_collection_iterator _current, double _elapsed_secs) const;
			collision_event find_closest_collision(double delta) const;

			virtual void run_active(double delta, double elapsed);
			virtual void run_complete(double delta, double elapsed);
			virtual void run_lobby(double delta, double elapsed);
			virtual void run_paused(double delta, double elapsed);
			virtual void run_exit(double delta, double elapsed);

			virtual job_notify execute(job_queue* _callingQueue, DWORD _bytesTransferred, BOOL _success);
			virtual bool queued(job_queue* _queue);

		};


        class engine : public engine_interface
		{
			comm_desktop_bus_interface*bus;
			corona_instance instance = corona_instance::local;
			std::vector<std::shared_ptr<adventure_base>> adventures;

		public:

			engine(comm_desktop_bus_interface* _db) : bus(_db)
			{

			}

            engine(const engine& _src) = default;
            engine(engine&& _src) = default;
            engine& operator =(const engine& _src) = default;
            engine& operator =(engine&& _src) = default;

			virtual ~engine()
			{

			}

			std::shared_ptr<adventure_interface> new_adventure(json _world_key)
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
				copy_transform.put_member_string("class_name", "adventure");

				copy_plan.put_member("from", copy_from);
				copy_plan.put_member("to", copy_to);
				copy_plan.put_member("transform", copy_transform);

				auto ccr = bus->copy_object(instance, copy_plan);
				if (ccr.success) {
					json new_session = ccr.data;
					std::shared_ptr<top_down_adventure> session = std::make_shared<top_down_adventure>(bus, new_session);
					session->create_stage();
					adventures.push_back(session);
					return session;
				}

				return nullptr;
			}

			std::shared_ptr<adventure_interface> load_adventure(json _game_key)
			{
				json_parser jp;
				auto result = bus->get_object(instance, _game_key);
				if (result.success) {
					std::shared_ptr<top_down_adventure> session = std::make_shared<top_down_adventure>(bus, result.data);
					adventures.push_back(session);
					return session;
				}
				return nullptr;
			}

			void save_adventure(std::shared_ptr<adventure_interface> _game)
			{
				json_parser jp;
				json jsession = jp.create_object();
				_game->get_json(jsession);
				bus->put_object(instance, jsession);
			}

			void close_adventure(std::shared_ptr<adventure_interface> _game)
			{
				_game->set_exit();
				std::remove(adventures.begin(), adventures.end(), _game);
			}
		};

		std::shared_ptr<engine_interface> engine_factory::create_engine(system_monitoring_interface*_bus)
		{
			auto busorama = dynamic_cast<comm_desktop_bus_interface*>(_bus);
            auto ptr = std::make_shared<engine>(busorama);
            auto result = std::dynamic_pointer_cast<engine_interface>(ptr);
			return result;
		}

		void actor::select_next()
		{
			workflow->select_next();
		}

		void actor::select_previous()
		{
			workflow->select_previous();
		}

		void actor::extend_selection(object_reference rf)
		{
			workflow->extend(rf);
		}

		void actor::clear_selection()
		{
            workflow->clear();
		}

		void top_down_adventure::create_stage()
		{
			if (stage) 
			{
				stage->construct();
			}
		}

		void top_down_adventure::init()
		{
		}

		void top_down_adventure::move_left(player_input input_device)
		{
			auto player = find_player(input_device);

			if (player) {
				player->move_left();
			}
		}

		void top_down_adventure::move_right(player_input input_device)
		{
			auto player = find_player(input_device);

			if (player) {
				player->move_right();
			}
		}

		void top_down_adventure::move_up(player_input input_device)
		{
			auto player = find_player(input_device);

			if (player) {
				player->move_foward();
			}
		}

		void top_down_adventure::move_down(player_input input_device)
		{
			auto player = find_player(input_device);

			if (player) {
				player->move_back();
			}
		}

		void top_down_adventure::fire(player_input input_device)
		{
			auto player = find_player(input_device);

		}

        void top_down_adventure::use(player_input input_device)
        {
            auto player = find_player(input_device);
            std::vector<std::shared_ptr<corona::selection_commands::command>> commands = selection_rules.get_commands_for_selection(*player->inventory.get(), *player->selection.get());
            auto command_range = std::find_if(commands.begin(), commands.end(), [this](std::shared_ptr<corona::selection_commands::command>& _src) {
                return _src == drop_c;
                });
            if (command_range != std::end(commands)) {
                std::shared_ptr<player_command> pcmd = std::dynamic_pointer_cast<player_command>(*command_range);
                if (pcmd) {
                    pcmd->run(this, player.get());
                }
            }
        }

		void top_down_adventure::select_next(player_input input_device)
		{
			auto player = find_player(input_device);
		}

		void top_down_adventure::select_previous(player_input input_device)
		{
			auto player = find_player(input_device);
		}


		void top_down_adventure::manage_play(player_input input_device)
		{
			auto player = find_player(input_device);

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

		void top_down_adventure::handle_gamepad_button_up(gamepad_button_up_event gpbd)
		{
            scope_lock locker(adventure_locker);			
		}

		void top_down_adventure::handle_gamepad_button_down(gamepad_button_down_event gpbd)
		{
			scope_lock locker(adventure_locker);

			auto pi = to_player_input(gpbd.state);

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
				select_next(pi);
				break;
			case gamepad_button::RightShoulder:
				select_previous(pi);
				break;
			case gamepad_button::Back:
				break;
			case gamepad_button::Start:
				manage_play(pi);
				break;
			case gamepad_button::DpadUp:
                move_up(pi);
				break;
			case gamepad_button::DpadDown:
				move_down(pi);
				break;
			case gamepad_button::DpadLeft:
				move_left(pi);
				break;
			case gamepad_button::DpadRight:
				move_right(pi);
				break;
			}
		}

        void top_down_adventure::handle_keydown(key_down_event kde)
        {
            scope_lock locker(adventure_locker);
			auto pi = player_input::input_keymouse;

            switch (kde.key)
            {
            case VK_SPACE:
                fire(pi);
                break;
            case VK_RETURN:
                manage_play(pi);
                break;
            case VK_UP:
                move_up(pi);
                break;
            case VK_DOWN:
                move_down(pi);
                break;
            case VK_LEFT:
                move_left(pi);
                break;
            case VK_RIGHT:
                move_right(pi);
                break;
			}
        }

		void top_down_adventure::handle_gamepad_trigger_up(gamepad_trigger_up_event gptu)
		{
			scope_lock locker(adventure_locker);

			auto pi = to_player_input(gptu.state);
			auto p = find_player(pi);

		}

		void top_down_adventure::handle_gamepad_trigger_down(gamepad_trigger_down_event gptd)
		{
			scope_lock locker(adventure_locker);

			auto pi = to_player_input(gptd.state);

			if (gptd.trigger == gamepad_trigger::LeftTrigger) 
			{
				fire(pi);
			}
			else if (gptd.trigger == gamepad_trigger::RightTrigger) 
			{
				use(pi);
			}
		}

		void top_down_adventure::handle_gamepad_thumbstick_move(gamepad_thumbstick_move_event gpbd)
		{
			scope_lock locker(adventure_locker);

			auto pi = to_player_input(gpbd.state);
			auto p = find_player(pi);


			if (fabs(gpbd.x) < fabs(gpbd.y)) {
                if (gpbd.y > 0) {
					p->move_foward();
				}
				else if (gpbd.y < 0) {
					p->move_back();
				}
			}
			else 
			{
				if (gpbd.x > 0) {
					p->move_right();
				}
				else if (gpbd.x < 0) {
					p->move_left();
				}
			}
		}

		std::shared_ptr<player> top_down_adventure::find_player(player_input input_device)
		{
			scope_lock locker(adventure_locker);

			// Search for existing player with this input device
			for (auto pl : stage->players()) {
				if (pl->input_device == input_device) {
					return pl;
				}
			}
			// Create new player and add to pieces only
			auto npl = create_piece_of_class("player");
			auto plt = std::dynamic_pointer_cast<player>(npl);
			plt->input_device = input_device;
			plt->ready = false;
			stage->current->pieces.insert_or_assign(plt->to_reference(), plt);
			return plt;
		}

		// This has to be const because we want the assertion that this doesn't modify the map or the pieces,
		// and only finds their state
		collision_event top_down_adventure::model_piece(std::shared_ptr<set> _map, piece_collection_iterator _current, double _elapsed_secs) const
		{
			using namespace DirectX;

			auto _piece =  _current->second;

			collision_event event = {};
			collision_result collision = {};

			// can't be accelerated or accelerate anything if it's not moving or accelerating.
			// no kinetic energy
			if (XMVector3Equal(_piece->velocity, zero_vector)) {
				return event;
			}
			
			auto cpiece = _current;
			cpiece++;

			while (cpiece != _map->current->end())
			{
				auto& other = cpiece->second;

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

				cpiece++;
			}

			return event;
		}

		void top_down_adventure::run_lobby(double delta, double elapsed)
		{
			;
		}

		collision_event top_down_adventure::find_closest_collision(double delta) const
		{
			collision_event closest_collision;
			for (auto ipiece = stage->current->begin(); ipiece != stage->current->end(); ipiece++) {
				auto pc = ipiece->second;
				collision_event collision = model_piece(stage, ipiece, delta);
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

		void top_down_adventure::run_active(double delta, double elapsed)
		{
			// we have to resolve collision effects first, 

			// in the model, physical quantities are given in seconds, so, we can apply the ax, ay to terms
			// and that gives us simple linear models.
			// for other models, we can additionally scale the time so that seconds could be weeks,
			// months or years.

			double remaining = delta;
            double start = elapsed - delta;

			while (remaining > 0.001) {

				collision_event closest_collision = find_closest_collision(remaining);

				if (closest_collision.piece_1) {
					// move pieces to the point of collision

					remaining -= closest_collision.collision.time_of_collision;

					for (auto px = stage->current->begin(); px != stage->current->end(); px++) {
						px->second->run(closest_collision.collision.time_of_collision, start + closest_collision.collision.time_of_collision);
					}

					// resolve collision effects here and update accelerations accordingly
					// for example, if piece_1 is a player and piece_2 is a wall, we might want to stop the player's movement in the direction of the wall.




				}
				else
				{
					// no more collisions, we can move all pieces for the remaining time
					for (auto px = stage->current->begin(); px != stage->current->end(); px++) {
						px->second->run(remaining, elapsed);
					}
					remaining = 0;
				}
			}
		}

		void top_down_adventure::run_complete(double delta, double elapsed)
		{
			;
		}

		void top_down_adventure::run_paused(double delta, double elapsed)
		{
			;
		}

		void top_down_adventure::run_exit(double delta, double elapsed)
		{
			;
		}

		bool top_down_adventure::queued(job_queue* _callingQueue)
		{
			return true;
		}

		job_notify top_down_adventure::execute(job_queue* _callingQueue, DWORD _bytesTransferred, BOOL _success)
		{
			scope_lock locker(adventure_locker);

			job_notify notify;
			json_parser jp;

			notify.shouldDelete = false;

			double current_elapsed_seconds = frame_timer.get_elapsed_seconds();
			double delta = current_elapsed_seconds - last_elapsed_seconds;

			last_elapsed_seconds = current_elapsed_seconds;
			last_delta_seconds = delta;

            set_time(delta);

			if (delta <= 0.0) return notify;

			switch (state) {
			case game_state::lobby:
				run_lobby(delta, current_elapsed_seconds);
				break;
			case game_state::active:
				run_active(delta, current_elapsed_seconds);
				break;
			case game_state::complete:
				run_complete(delta, current_elapsed_seconds);
				break;
			case game_state::paused:
				run_paused(delta, current_elapsed_seconds);
				break;
			case game_state::exit:
				run_exit(delta, current_elapsed_seconds);
				break;
			}

			return notify;
		}

		void top_down_adventure::set_lobby()
		{
			state = game_state::lobby;
		}

		void top_down_adventure::set_active()
		{
			state = game_state::active;
		}

		void top_down_adventure::set_paused()
		{
			state = game_state::paused;
		}

		void top_down_adventure::set_complete()
		{
			state = game_state::complete;
		}

		void top_down_adventure::set_exit()
		{
			state = game_state::exit;
		}

		void top_down_adventure::check_all_ready()
		{
			if (state == game_state::lobby) {
				auto players_view = stage->players();
				bool all_ready = std::ranges::all_of(players_view, [](const auto& player) {
					return player->ready;
					});
				if (all_ready) {
					set_active();
				}
			}
		}

		void top_down_adventure::check_all_dead()
		{
			auto players_view = stage->players();
			bool all_dead = std::ranges::all_of(players_view, [](const auto& player) {
				return player->dead;
				});
			if (all_dead) {
				set_complete();
			}
		}

		void top_down_adventure::get_json(json& _dest)
		{
			json_parser jp;
			_dest.put_member("name", name);
			_dest.put_member("description", description);
			json j = jp.create_array();
			json jmap = jp.create_object();
			stage->get_json(jmap);
			j.push_back(jmap);
			_dest.put_member("stage", j);

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

		void top_down_adventure::put_json(json& _src)
		{
			name = _src["name"].as_string();
			description = _src["description"].as_string();
			json j = _src["stage"];
			stage = std::make_shared<set>();
			std::string state_string = _src["state"].as_string();
            if (state_string == "lobby") {
				state = game_state::lobby;
			}
			else if (state_string == "active") {
				state = game_state::active;
			}
			else if (state_string == "paused") {
				state = game_state::paused;
			}
			else if (state_string == "complete") {
				state = game_state::complete;
			}
			else if (state_string == "exit") {
				state = game_state::exit;
			}
		}

		job* top_down_adventure::get_next_job()
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

		void top_down_adventure::create_assets(direct2dContext& _src)
		{
			scope_lock locker(adventure_locker);

			for (auto &p : stage->current->pieces) {
				p.second->create_assets(_src);
			}
		}

		void top_down_adventure::draw(direct2dContext& _src)
		{
            scope_lock locker(adventure_locker);

			for (auto& p : stage->current->pieces) {
				p.second->draw(_src, nullptr);
			}
		}

		std::shared_ptr<piece> top_down_adventure::find(object_reference _piece_ref)
		{
			scope_lock locker(adventure_locker);

			auto pi = stage->current->pieces.find(_piece_ref);
			if (pi != std::end(stage->current->pieces)) {
				return std::dynamic_pointer_cast<piece>( pi->second);
			}
			return nullptr;
		}

		std::shared_ptr<piece> top_down_adventure::erase(object_reference _piece_ref)
		{
			scope_lock locker(adventure_locker);

			auto pi = stage->current->pieces.find(_piece_ref);
			if (pi != std::end(stage->current->pieces)) {
				auto rx = pi->second;
				stage->current->pieces.erase(pi);
				return std::dynamic_pointer_cast<piece>(rx);
			}
			return nullptr;
		}

		void top_down_adventure::put(std::shared_ptr<piece> _src)
		{
			scope_lock locker(adventure_locker);

			auto dc = std::dynamic_pointer_cast<piece>(_src);
			stage->current->pieces.insert_or_assign(dc->to_reference(), dc);
		}

		void top_down_adventure::set_time(double _elapsed_seconds)
		{
			scope_lock locker(adventure_locker);
            stage->set_time(_elapsed_seconds);
		}

	}


}
