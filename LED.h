#pragma once
#include <memory>

class LED {
    public:
        LED(uint8_t pin);
        ~LED();

        void off();
        void on();
        void setState(uint8_t state);

    private:
        class impl;
        std::unique_ptr<impl> p_impl;
};
