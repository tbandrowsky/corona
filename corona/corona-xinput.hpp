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

    };

}
