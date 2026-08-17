#pragma once

namespace corona
{

	class adventure_control :
		public draw_control
	{
		std::shared_ptr<corona::game::adventure> current_session;

	public:

		adventure_control() { 
			; 
		}

		adventure_control(const adventure_control& _src) = default;

		adventure_control(control_base* _parent, int _id) : draw_control(_parent, _id) 
		{ 
			; 
		}

		virtual void on_subscribe(presentation_base* _presentation, page_base* _page)
		{
			draw_control::on_subscribe(_presentation, _page);

			_page->on_gamepad_button_down(id, [this](gamepad_button_down_event gpbd) {
				if (current_session) {
                    current_session->handle_gamepad_button_down(gpbd);
				}
			});

			_page->on_gamepad_button_up(id, [this](gamepad_button_up_event gpbu) {
				if (current_session) {
					current_session->handle_gamepad_button_up(gpbu);
				}
			});

			_page->on_gamepad_trigger_up(id, [this](gamepad_trigger_up_event gptu) {
				if (current_session) {
					current_session->handle_gamepad_trigger_up(gptu);
				}
			});

			_page->on_gamepad_trigger_down(id, [this](gamepad_trigger_down_event gptd) {
				if (current_session) {
					current_session->handle_gamepad_trigger_down(gptd);
				}
			});

			_page->on_gamepad_thumbstick_move(id, [this](gamepad_thumbstick_move_event gptm) {
				if (current_session) {
					current_session->handle_gamepad_thumbstick_move(gptm);
				}
			});
		}

		virtual ~adventure_control() { ; }

		virtual std::shared_ptr<control_base> clone()
		{
			auto tv = std::make_shared<adventure_control>(*this);
			tv->current_session = current_session;
			return tv;
		}

		virtual void get_json(json& _dest)
		{
			json_parser jp;
			draw_control::get_json(_dest);
		}

		virtual void put_json(json& _src)
		{
			draw_control::put_json(_src);
		}

        std::shared_ptr<corona::game::adventure_interface> get_session()
		{ 
			return std::dynamic_pointer_cast<corona::game::adventure_interface>(current_session); 
		}

		std::shared_ptr<corona::game::adventure_app_interface> set_session(std::shared_ptr<corona::game::adventure_app_interface>& _session)
		{ 
			if (current_session) {
				current_session->set_exit();
            }
			auto session = current_session;
			current_session = std::dynamic_pointer_cast<corona::game::adventure>(_session); 
			return session;
		}

		virtual void on_update(double _time)
		{
			if (current_session) {
				current_session->set_time(_time);
            }
		}

	private:

		void init()
		{
			on_draw = [this](std::shared_ptr<direct2dContext>& _context, draw_control*) {
				if (current_session) {
					current_session->draw(*_context);
                }
			};

			on_create = [this](std::shared_ptr<direct2dContext>& _context, draw_control*) {
				if (current_session) {
					current_session->create_assets(*_context);
				}
			};
		}
	};

	class corona_frame_rectangle
	{
	public:
		rectangle rect;
		std::shared_ptr<game::frame> object;
	};

	class corona_animation_rectangle
	{
	public:
		rectangle rect;
		std::shared_ptr<game::animation> object;
	};

	class animations_control : public draw_control
	{
		json data;
		std::vector<std::shared_ptr<game::animation>>	animations;
		std::vector<corona_animation_rectangle>			animation_rectangles;
		std::vector<corona_frame_rectangle>				frame_rectangles;
		corona_animation_rectangle						current_animation;
		corona_frame_rectangle							current_frame;
		double elapsed_seconds = 0.0;

	public:

		animations_control(const animations_control& _src) = default;

		animations_control() 
		{
			init();
		}

		animations_control(control_base* _parent, int _id) 
			: draw_control(_parent, _id)
		{ 
			init();
        }

		virtual ~animations_control() { ; }

		virtual void get_json(json& _dest)
		{
			json_parser jp;
			draw_control::get_json(_dest);
			if (!json_field_name.empty()) {
				json j = jp.create_array();
				for (auto& s : animations) {
					json jsprite = jp.create_object();
					s->get_json(jsprite);
					j.push_back(jsprite);
				}
				_dest.put_member(json_field_name, j);
			}
		}

		virtual void put_json(json& _src);

		virtual std::shared_ptr<control_base> clone()
		{
			auto tv = std::make_shared<animations_control>(*this);
			return tv;
		}

		virtual void on_update(double _time)
		{
			elapsed_seconds = _time;
            for (auto anim : animations) {
				anim->set_time(_time);
			}
			for (auto child : children) {
				child->on_update(_time);
			}
		}

		void init()
		{
			set_origin(0.0_px, 0.0_px);
			set_size(1.0_container, 1.2_fontgr);

			on_create = [](std::shared_ptr<direct2dContext>& _context, draw_control* _src)
				{
					animations_control* t = dynamic_cast<animations_control*>(_src);
					if (t) {
						t->set_default_styles();
                        for (auto anim : t->animations) {
							anim->create_assets(*_context);
						}
					}
				};

			on_draw = [](std::shared_ptr<direct2dContext>& _context, draw_control* _src) {
				animations_control* t = dynamic_cast<animations_control*>(_src);

				auto draw_bounds = _src->get_inner_bounds();

                for (auto& anim_rect : t->animation_rectangles) {
					if (anim_rect.object) {
                        auto rect = anim_rect.rect;
                        rect.x += draw_bounds.x;
                        rect.y += draw_bounds.y;
						DirectX::XMVECTOR location = to_point(rect);
						anim_rect.object->draw(*_context, location);
					}
				}

				for (auto& frame_rect : t->frame_rectangles) {
                    if (frame_rect.object) {
						auto rect = frame_rect.rect;
                        rect.x += draw_bounds.x;
                        rect.y += draw_bounds.y;
						DirectX::XMVECTOR location = to_point(rect);
						frame_rect.object->draw(*_context, location);
					}
				}

			};
		}

		virtual void set_default_styles()
		{
			;
		}

		virtual void arrange(control_base* _parent, rectangle* _ctx) override
		{
			draw_control::arrange(_parent, _ctx);

			double num_frames_x = 4;
			double num_frames_y = 2;
			double num_animations_x = 2;
			double num_animations_y = 4;
			
			point total_size = rectangle_math::size(_ctx);
            point animation_select_area_size = { total_size.x * 0.3, total_size.y };
			point animation_select_size = { animation_select_area_size.x / num_animations_x, animation_select_area_size.y / num_animations_y };
			point animation_area_size = { total_size.x * 0.7, total_size.y * 0.6 };
            point frame_area_size = { animation_area_size.x, total_size.y * 0.4 };
			point frame_select_size = { frame_area_size.x / num_frames_x, total_size.y / num_frames_y };

			point base;
			current_animation.rect.x = inner_bounds.x + animation_select_area_size.x;
			current_animation.rect.y = inner_bounds.y;
            current_animation.rect.w = animation_area_size.x;
            current_animation.rect.h = animation_area_size.y;

			if (current_animation.object) {
                auto iter = current_animation.object->frames.begin();
				for (int x = 0; x < num_frames_x; x++) {
					for (int y = 0; y < num_frames_y; y++) {
                        if (iter != std::end(current_animation.object->frames)) {
							rectangle r;
							r.x = x * frame_select_size.x;
							r.y = y * frame_select_size.y;
							r.w = frame_select_size.x;
							r.h = frame_select_size.y;
							frame_rectangles.push_back({ r, current_frame.object });
							iter++;
						}
						else {
							break;
						}
					}
                }
			}
            auto iter = animations.begin();
            for (int x = 0; x < num_animations_x; x++) {
				for (int y = 0; y < num_animations_y; y++) {
					if (iter != animations.end()) {
						rectangle r;
						r.x = x * animation_select_size.x;
						r.y = y * animation_select_size.y;
						r.w = animation_select_size.x;
						r.h = animation_select_size.y;
						animation_rectangles.push_back({ r, *iter });
						iter++;
					}
					else {
						break;
					}
				}
			}
		}

		virtual json get_data() override
		{
			if (json_field_name.empty()) {
				return data;
			}
			else if (data.object() && data.has_member(json_field_name)) {
				return data[json_field_name];
			}
			return data;
		}

		virtual json set_data(json _data) override
		{
			data = _data;
			if (data.array()) {
				set_items(data);
			} else if (data.object() && data.has_member(json_field_name)) {
				set_items(data[json_field_name]);
            }
			return data;
		}

		virtual bool set_items(json _data);

		virtual double get_font_size() { return view_style ? view_style->text_style.fontSize : 14; }
	};

	json corona_start_game_command::handle_response(corona_client_response response, comm_desktop_bus_interface* _bus) {
		auto ctrl = _bus->find_control(form_name);
		adventure_control* session_control = dynamic_cast<adventure_control*>(ctrl);
		if (session_control) {
			session_control->set_session(session);
		}
		return response.data;
	}

	json corona_game_command::create_request(comm_desktop_bus_interface* _bus)
	{
		json_parser jp;
		json obj;

		auto ctrl = _bus->find_control(form_name);
		auto session_control = dynamic_cast<adventure_control*>(ctrl);
		auto temp = session_control->get_session();
		session = std::dynamic_pointer_cast<corona::game::adventure_app_interface>(temp);
		return obj;
	}

	corona_client_response corona_game_set_lobby::execute_request(json request, comm_desktop_bus_interface* _bus)
	{
		get_session()->set_lobby();
		return response;
	}

	corona_client_response corona_game_set_active::execute_request(json request, comm_desktop_bus_interface* _bus)
	{
		get_session()->set_active();
		return response;
	}

	corona_client_response corona_game_set_paused::execute_request(json request, comm_desktop_bus_interface* _bus)
	{
		get_session()->set_paused();
		return response;
	}

	corona_client_response corona_game_set_complete::execute_request(json request, comm_desktop_bus_interface* _bus)
	{
		get_session()->set_complete();
		return response;
	}

	corona_client_response corona_game_set_exit::execute_request(json request, comm_desktop_bus_interface* _bus)
	{
		get_session()->set_exit();
		return response;
	}

}
