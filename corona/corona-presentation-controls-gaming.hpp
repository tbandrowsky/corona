#pragma once

namespace corona
{

	class game_session_control :
		public draw_control
	{
		std::shared_ptr<corona::game::game> current_session;
		std::map<std::string, std::shared_ptr<image_control>> images;

	public:

		game_session_control() { 
			; 
		}

		game_session_control(const game_session_control& _src) = default;

		game_session_control(control_base* _parent, int _id) : draw_control(_parent, _id) 
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

		virtual ~game_session_control() { ; }

		virtual std::shared_ptr<control_base> clone()
		{
			auto tv = std::make_shared<game_session_control>(*this);
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

		virtual void create_game_assets(std::shared_ptr<direct2dContext>& _context)
		{

			for (auto& [name, image] : images) {
				image->create(_context, host);
            }
		}

		virtual void draw_game_frame(std::shared_ptr<direct2dContext>& _context)
		{
		}

        std::shared_ptr<corona::game::game_interface> get_session()
		{ 
			return std::dynamic_pointer_cast<corona::game::game_interface>(current_session); 
		}

		std::shared_ptr<corona::game::game_interface> set_session(std::shared_ptr<corona::game::game>& _session)
		{ 
			if (current_session) {
				current_session->state = corona::game::game_state::exit;
            }
			auto session = current_session;
			current_session = _session; 
			return session;
		}
	};

}
