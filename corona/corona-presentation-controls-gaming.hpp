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

		virtual void on_draw(std::shared_ptr<direct2dContext>& _context, draw_control*) {
			if (current_session) {
				current_session->draw(*_context);
			}
		};

		virtual void on_create(std::shared_ptr<direct2dContext>& _context, draw_control*) {
			if (current_session) {
				current_session->create_assets(*_context);
			}
		};

	private:

		void init()
		{
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
		json_object data;
		std::vector<std::shared_ptr<game::animation>>	animations;
		std::vector<corona_animation_rectangle>			animation_rectangles;
		std::vector<corona_frame_rectangle>				frame_rectangles;
		corona_animation_rectangle						current_animation;
		corona_frame_rectangle							current_frame;
		double elapsed_seconds = 0.0;
		generalBrushRequest								animation_border;
		generalBrushRequest								frame_border;

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

		virtual void on_draw(std::shared_ptr<direct2dContext>& _context, draw_control*)
		{

			animation_border = solidBrushRequest("animation_border", "00C000");
			frame_border = solidBrushRequest("frame_border", "850095");

			_context->setBrush(&animation_border);
			_context->setBrush(&frame_border);

			std::string border_name = animation_border.get_name();

			for (auto& anim_rect : animation_rectangles) {
				if (anim_rect.object) {
					auto rect = anim_rect.rect;
					DirectX::XMVECTOR location = to_point(rect);
					_context->drawRectangle(&rect, border_name, 2, "");
				}
			}

			border_name = frame_border.get_name();

			for (auto& frame_rect : frame_rectangles) {
				if (frame_rect.object) {
					auto rect = frame_rect.rect;
					DirectX::XMVECTOR location = to_point(rect);
					_context->drawRectangle(&rect, border_name, 8, "");
				}
			}

			for (auto& anim_rect : animation_rectangles) {
				if (anim_rect.object) {
					auto rect = anim_rect.rect;
					anim_rect.object->draw(*_context, &rect);
				}
			}

			for (auto& frame_rect : frame_rectangles) {
				if (frame_rect.object) {
					auto rect = frame_rect.rect;
					frame_rect.object->draw(*_context, &rect);
				}
			}
		}

		virtual void on_create(std::shared_ptr<direct2dContext>& _context, draw_control*)
		{
			set_default_styles();
			for (auto anim : animations) {
				anim->create_assets(*_context);
			}
		}

		void init()
		{
			set_origin(0.0_px, 0.0_px);
			set_size(1.0_container, 1.2_fontgr);
			animation_border = solidBrushRequest("animation_border", "400040");
			frame_border = solidBrushRequest("frame_border", "400035");
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

			double animation_width = total_size.x * 0.3;
			double animation_height = animation_width;
			double frame_width = animation_width * .75;
			double frame_height = animation_height * .75;
	
            rectangle animation_list_rect = { _ctx->x, _ctx->y, animation_width, total_size.y };
			rectangle frame_list_rect = { _ctx->x + animation_width, _ctx->y + total_size.y - frame_height, total_size.x - animation_width, frame_height };
            rectangle current_animation_rect = { _ctx->x + animation_width, _ctx->y, total_size.x - animation_width, total_size.y - frame_height };

			point base;
			current_animation.rect = current_animation_rect;

			auto ianim = animations.begin();

			rectangle r;

			r.x = animation_list_rect.x;
			r.y = animation_list_rect.y;
			r.w = animation_width;
			r.h = animation_height;

			while (ianim != std::end(animations))
			{
				animation_rectangles.push_back({ r, *ianim });

				if (!current_animation.object) {
					current_animation.object = *ianim;
				}

				r.y += animation_height;
				ianim++;
			}

			r.x = frame_list_rect.x;
			r.y = frame_list_rect.y;
			r.w = frame_width;
			r.h = frame_height;

			if (current_animation.object) {
                auto iframe = current_animation.object->frames.begin();

                while (iframe != std::end(current_animation.object->frames)) {
					corona_frame_rectangle fr;
					fr.rect = r;
					fr.object = iframe->second;
					frame_rectangles.push_back(fr);
					if (!current_frame.object) {
						current_frame.object = iframe->second;
					}
                    r.x += frame_width;
					iframe++;
				}
			}
		}

		virtual json_object get_data() override
		{
			return data;
		}

		virtual json_object set_data(json_object _data) override
		{
			data = _data;
			if (data.has_member(json_field_name)) {
				json_array items_array = data[json_field_name];
				set_items(items_array);
            }
			return data;
		}

		virtual bool set_items(json_array _data) override;

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
