#pragma once

namespace corona
{

    class xinput
    {
        XINPUT_STATE controllers[XUSER_MAX_COUNT];

    public:

        xinput()
        {
            ZeroMemory(controllers, sizeof(controllers));
        }

        void update()
        {
            for (DWORD i = 0; i < XUSER_MAX_COUNT; i++) {
                ZeroMemory(&controllers[i], sizeof(XINPUT_STATE));
                XInputGetState(i, &controllers[i]);
            }
        }

        const XINPUT_STATE& get_state(DWORD user_index) const
        {
            if (user_index >= XUSER_MAX_COUNT) {
                throw std::out_of_range("User index out of range");
            }
            return controllers[user_index];
        }

        json get_state_json() const
        {
            json_parser jp;
            json j = jp.create_array();
            for (DWORD i = 0; i < XUSER_MAX_COUNT; i++) {
               
                json jcontroller = jp.create_object();
                jcontroller.put_member_string(class_name_field, "xinput_controller_state");
                jcontroller.put_member("user_index", i);
                jcontroller.put_member("connected", controllers[i].dwPacketNumber != 0);

                int gpb = controllers[i].Gamepad.wButtons;

                if (gpb & XINPUT_GAMEPAD_DPAD_DOWN)  {
                    jcontroller.put_member("dpad_down", true);
                }
                if (gpb & XINPUT_GAMEPAD_DPAD_UP) {
                    jcontroller.put_member("dpad_up", true);
                }
                if (gpb & XINPUT_GAMEPAD_DPAD_LEFT) {
                    jcontroller.put_member("dpad_left", true);
                }
                if (gpb & XINPUT_GAMEPAD_DPAD_RIGHT) {
                    jcontroller.put_member("dpad_right", true);
                }
                if (gpb & XINPUT_GAMEPAD_START) {
                    jcontroller.put_member("start", true);
                }
                if (gpb & XINPUT_GAMEPAD_BACK) {
                    jcontroller.put_member("back", true);
                }
                if (gpb & XINPUT_GAMEPAD_LEFT_THUMB) {
                    jcontroller.put_member("left_thumb", true);
                }
                if  (gpb & XINPUT_GAMEPAD_RIGHT_THUMB) {
                    jcontroller.put_member("right_thumb", true);
                }
                if (gpb & XINPUT_GAMEPAD_LEFT_SHOULDER) {
                    jcontroller.put_member("left_shoulder", true);
                }
                if (gpb & XINPUT_GAMEPAD_RIGHT_SHOULDER) {
                    jcontroller.put_member("right_shoulder", true);
                }
                if (gpb & XINPUT_GAMEPAD_A) {
                    jcontroller.put_member("a", true);
                }
                if (gpb & XINPUT_GAMEPAD_B) {
                    jcontroller.put_member("b", true);
                }
                if (gpb & XINPUT_GAMEPAD_X) {
                    jcontroller.put_member("x", true);
                }
                if (gpb & XINPUT_GAMEPAD_Y) {
                    jcontroller.put_member("y", true);
                }

                jcontroller.put_member("buttons", controllers[i].Gamepad.wButtons);
                jcontroller.put_member("left_trigger", controllers[i].Gamepad.bLeftTrigger);
                jcontroller.put_member("right_trigger", controllers[i].Gamepad.bRightTrigger);
                jcontroller.put_member_double("thumb_lx", controllers[i].Gamepad.sThumbLX);
                jcontroller.put_member_double("thumb_ly", controllers[i].Gamepad.sThumbLY);
                jcontroller.put_member_double("thumb_rx", controllers[i].Gamepad.sThumbRX);
                jcontroller.put_member_double("thumb_ry", controllers[i].Gamepad.sThumbRY);
                j.push_back(jcontroller);
            }
            return j;
        }

    };

    point get_thumbstick(int dead_zone, int thumbX, int thumbY)
    {
        point pt;

        float LX = thumbX;
        float LY = thumbY;

        //determine how far the controller is pushed
        float magnitude = sqrt(LX * LX + LY * LY);

        float normalizedMagnitude = 0;

        //check if the controller is outside a circular dead zone
        if (magnitude > dead_zone)
        {
            //clip the magnitude at its expected maximum value
            if (magnitude > 32767) magnitude = 32767;

            //adjust magnitude relative to the end of the dead zone
            magnitude -= dead_zone;

            if (magnitude < 1) magnitude = 1;

            normalizedMagnitude = magnitude / (32767 - dead_zone);

            if (normalizedMagnitude < 1) normalizedMagnitude = 1;

            pt.x = LX * normalizedMagnitude;
            pt.y = LY * normalizedMagnitude;
        }

        return pt;
    }



}
