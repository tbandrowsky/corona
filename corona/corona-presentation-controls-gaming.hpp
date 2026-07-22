#pragma once

namespace corona
{

	class game_session_control :
		public draw_control
	{
		std::shared_ptr<corona::game::game> current_session;

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

        std::shared_ptr<corona::game::adventure_interface> get_session()
		{ 
			return std::dynamic_pointer_cast<corona::game::adventure_interface>(current_session); 
		}

		std::shared_ptr<corona::game::game_app_interface> set_session(std::shared_ptr<corona::game::game_app_interface>& _session)
		{ 
			if (current_session) {
				current_session->set_exit();
            }
			auto session = current_session;
			current_session = std::dynamic_pointer_cast<corona::game::game>(_session); 
			return session;
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

	json corona_start_game_command::handle_response(corona_client_response response, comm_bus_app_interface* _bus) {
		auto ctrl = _bus->find_control(form_name);
		game_session_control* session_control = dynamic_cast<game_session_control*>(ctrl);
		if (session_control) {
			session_control->set_session(session);
		}
		return response.data;
	}

	json corona_game_command::create_request(comm_bus_app_interface* _bus)
	{
		json_parser jp;
		json obj;

		auto ctrl = _bus->find_control(form_name);
		auto session_control = dynamic_cast<game_session_control*>(ctrl);
		auto temp = session_control->get_session();
		session = std::dynamic_pointer_cast<corona::game::game_app_interface>(temp);
		return obj;
	}

	corona_client_response corona_game_set_lobby::execute_request(json request, comm_bus_app_interface* _bus)
	{
		get_session()->set_lobby();
		return response;
	}

	corona_client_response corona_game_set_active::execute_request(json request, comm_bus_app_interface* _bus)
	{
		get_session()->set_active();
		return response;
	}

	corona_client_response corona_game_set_paused::execute_request(json request, comm_bus_app_interface* _bus)
	{
		get_session()->set_paused();
		return response;
	}

	corona_client_response corona_game_set_complete::execute_request(json request, comm_bus_app_interface* _bus)
	{
		get_session()->set_complete();
		return response;
	}

	corona_client_response corona_game_set_exit::execute_request(json request, comm_bus_app_interface* _bus)
	{
		get_session()->set_exit();
		return response;
	}

}
