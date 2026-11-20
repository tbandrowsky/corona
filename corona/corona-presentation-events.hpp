/*
CORONA
C++ Low Code Performance Applications for Windows SDK
by Todd Bandrowky
(c) 2024 + All Rights Reserved


MIT License

About this File
Defines events 

Notes

For Future Consideration
*/


#pragma once

namespace corona {

	class control_base;

	class page;

	class page_event
	{
	public:
		std::shared_ptr<page> pg;
	};

	class page_select_event : public page_event
	{
	public:

	};

	class page_load_event : public page_event
	{
	public:

	};

	class page_unload_event : public page_event
	{
	public:

	};

	class control_event
	{
	public:
		int batch_id;
		control_base* control;
		comm_desktop_bus_interface* bus;
	};

	class command_event : public control_event
	{
	public:
		int command;
	};

	class mouse_event : public control_event
	{
	public:
		point absolute_point;
		point relative_point;

        virtual void get_json(json& _dest)
        {
            _dest.put_member_double("absolute_point_x", absolute_point.x);
            _dest.put_member_double("absolute_point_y", absolute_point.y);
            _dest.put_member_double("relative_point_x", relative_point.x);
            _dest.put_member_double("relative_point_y", relative_point.y);
        }
        virtual void put_json(json& _src)
        {
            absolute_point.x = _src["absolute_point_x"].as_double();
            absolute_point.y = _src["absolute_point_y"].as_double();
            relative_point.x = _src["relative_point_x"].as_double();
            relative_point.y = _src["relative_point_y"].as_double();
        }
	};

	class mouse_move_event : public mouse_event
	{

	};

	class mouse_wheel_event : public mouse_event
	{
	public:
		double delta;

		virtual void get_json(json& _dest)
		{
			mouse_event::get_json(_dest);
            _dest.put_member_double("delta", delta);
		}
		virtual void put_json(json& _src)
		{
            mouse_event::put_json(_src);
            delta = _src["delta"].as_double();
		}
	};

	class mouse_click_event : public mouse_event
	{

	};

	class mouse_left_click_event : public mouse_event
	{

	};

	class mouse_right_click_event : public mouse_event
	{

	};

	class notify_event : public control_event
	{

	};

	class key_event : public control_event
	{
	public:
		short key;

		virtual void get_json(json& _dest)
		{
			_dest.put_member_i64("key", key);
		}
		virtual void put_json(json& _src)
		{
			key = _src["key"].as_int64_t();
		}
	};

	class key_down_event : public key_event
	{
	public:

	};

	class key_up_event : public key_event
	{
	public:

	};

	enum class gamepad_button
	{
		A = 0,
		B = 1,
		X = 2,
		Y = 3,
		LeftShoulder = 4,
		RightShoulder = 5,
		Start = 6,
		Back = 7,
		LeftThumb = 8,
		RightThumb = 9,
        DpadUp = 10,
        DpadDown = 11,
        DpadLeft = 12,
        DpadRight = 13
    };

	enum class gamepad_trigger
	{
		LeftTrigger = 8,
		RightTrigger = 9
	};

	enum class gamepad_thumb
	{
		LeftThumb = 8,
		RightThumb = 9
	};

	class gamepad_event : public control_event
	{
	public:
		XINPUT_STATE state;

		virtual void get_json(json& _dest)
		{
			_dest.put_member_i64("packet", state.dwPacketNumber);
            _dest.put_member_i64("buttons", state.Gamepad.wButtons);
            _dest.put_member_i64("left_trigger", state.Gamepad.bLeftTrigger);
			_dest.put_member_i64("right_trigger", state.Gamepad.bRightTrigger);
			_dest.put_member_i64("left_thumb_x", state.Gamepad.sThumbLX);
			_dest.put_member_i64("left_thumb_y", state.Gamepad.sThumbLY);
			_dest.put_member_i64("right_thumb_x", state.Gamepad.sThumbRX);
			_dest.put_member_i64("right_thumb_y", state.Gamepad.sThumbRY);
		}

		virtual void put_json(json& _src)
		{
			state.dwPacketNumber = _src["packet"].as_int64_t();
            state.Gamepad.wButtons = _src["buttons"].as_int64_t();
            state.Gamepad.bLeftTrigger = (BYTE)_src["left_trigger"].as_int64_t();
            state.Gamepad.bRightTrigger = (BYTE)_src["right_trigger"].as_int64_t();
            state.Gamepad.sThumbLX = (SHORT)_src["left_thumb_x"].as_int64_t();
            state.Gamepad.sThumbLY = (SHORT)_src["left_thumb_y"].as_int64_t();
            state.Gamepad.sThumbRX = (SHORT)_src["right_thumb_x"].as_int64_t();
            state.Gamepad.sThumbRY = (SHORT)_src["right_thumb_y"].as_int64_t();
		}
	};

	class gamepad_button_event : public gamepad_event
	{
	public:
		gamepad_button button;
		virtual void get_json(json& _dest)
		{
			gamepad_event::get_json(_dest);
			_dest.put_member_i64("button", (int64_t)button);
		}
		virtual void put_json(json& _src)
		{
			gamepad_event::put_json(_src);
			button = (gamepad_button)_src["button"].as_int64_t();
		}

	};

	class gamepad_button_down_event : public gamepad_button_event
	{
	public:
	};

	class gamepad_button_up_event : public gamepad_button_event
	{
	public:
	};

	class gamepad_trigger_down_event : public gamepad_button_event
	{
	public:
	};

	class gamepad_trigger_up_event : public gamepad_button_event
	{
	public:
	};

	class gamepad_thumbstick_move_event : public gamepad_button_event
	{
	public:
		gamepad_thumb thumb;
		float x, y;

		virtual void get_json(json& _dest)
		{
			gamepad_event::get_json(_dest);
			_dest.put_member_i64("button", (int64_t)button);
		}
		virtual void put_json(json& _src)
		{
			gamepad_event::put_json(_src);
			button = (gamepad_button)_src["button"].as_int64_t();
		}
	};

	class key_press_event : public key_event
	{
	public:

	};

	class draw_event : public control_event
	{
	public:
		int state;
	};

	class item_changed_event : public control_event
	{
	public:
		std::string text_value;
	};

	class list_changed_event : public control_event
	{
	public:
		int state;

		std::string selected_text;
		int selected_index;
		relative_ptr_type selected_value;
	};

	class key_up_event_binding
	{
	public:
		std::function< void(key_up_event) > on_key_up;
	};

	class key_down_event_binding
	{
	public:
		std::function< void(key_down_event) > on_key_down;
	};
	 
	class key_press_event_binding
	{
	public:
		std::function< void(key_press_event) > on_key_press;
	};

	class mouse_move_event_binding
	{
	public:
		control_base *control;
		std::function< void(mouse_move_event) > on_mouse_move;
	};

	class mouse_wheel_event_binding
	{
	public:
		control_base* control;
		std::function< void(mouse_wheel_event) > on_mouse_wheel;
	};

	class mouse_click_event_binding
	{
	public:
		control_base* control;
		std::function< void(mouse_click_event) > on_mouse_click;
	};

	class mouse_left_click_event_binding
	{
	public:
		control_base* control;
		std::function< void(mouse_left_click_event) > on_mouse_left_click;
	};

	class mouse_right_click_event_binding
	{
	public:
		control_base* control;
		std::function< void(mouse_right_click_event) > on_mouse_right_click;
	};

	class gamepad_button_down_event_binding
	{
	public:
		control_base* control;
		std::function< void(gamepad_button_down_event) > on_button_down;
	};

	class gamepad_button_up_event_binding
	{
	public:
		control_base* control;
		std::function< void(gamepad_button_up_event) > on_button_up;
	};

	class gamepad_trigger_down_event_binding
	{
	public:
		control_base* control;
		std::function< void(gamepad_trigger_down_event) > on_trigger_down;
	};

	class gamepad_trigger_up_event_binding
	{
	public:
		control_base* control;
		std::function< void(gamepad_trigger_up_event) > on_trigger_up;
	};

	class gamepad_thumbstick_move_event_binding
	{
	public:
		control_base* control;
		std::function< void(gamepad_thumbstick_move_event) > on_thumbstick_move;
	};

	class draw_event_binding
	{
	public:
		control_base* control;
		std::function< void(draw_event) > on_draw;
	};

	class item_changed_event_binding
	{
	public:
		control_base* control;
		std::function< void(item_changed_event) > on_change;
	};

	class command_event_binding
	{
	public:
		control_base* control;
		std::function< void(command_event) > on_command;
	};

	class list_changed_event_binding
	{
	public:
		control_base* control;
		std::function< void(list_changed_event) > on_change;
	};

	class page_select_event_binding
	{
	public:
		control_base* control;
		std::function< void(page_select_event) > on_select;
	};

	class page_base {
	public:

		std::shared_ptr<control_base> root;
		std::string name;

		virtual void on_mouse_move(control_base *_base, std::function< void(mouse_move_event) >) = 0;
		virtual void on_mouse_click(control_base* _base, std::function< void(mouse_click_event) >) = 0;
		virtual void on_mouse_wheel(control_base* _base, std::function< void(mouse_wheel_event) >) = 0;
		virtual void on_mouse_left_click(control_base* _base, std::function< void(mouse_left_click_event) >) = 0;
		virtual void on_mouse_right_click(control_base* _base, std::function< void(mouse_right_click_event) >) = 0;
		virtual void on_item_changed(control_base* _base, std::function< void(item_changed_event) >) = 0;
		virtual void on_list_changed(control_base* _base, std::function< void(list_changed_event) >) = 0;
		virtual void on_command(control_base* _base, std::function< void(command_event) >) = 0;
		virtual void on_select(std::function< void(page_select_event) >) = 0;
        virtual void on_gamepad_button_down(control_base* _base, std::function< void(gamepad_button_down_event) >) = 0;
        virtual void on_gamepad_button_up(control_base* _base, std::function< void(gamepad_button_up_event) >) = 0;
        virtual void on_gamepad_trigger_down(control_base* _base, std::function< void(gamepad_trigger_down_event) >) = 0;
        virtual void on_gamepad_trigger_up(control_base* _base, std::function< void(gamepad_trigger_up_event) >) = 0;
        virtual void on_gamepad_thumbstick_move(control_base* _base, std::function< void(gamepad_thumbstick_move_event) >) = 0;
		virtual void clear_events(control_base* _base) = 0;
		virtual void refresh() = 0;
	};
}
