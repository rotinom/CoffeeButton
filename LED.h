#pragma once
#include <memory>
#include "Arduino.h"

class LED {
    public:
        enum StateEnum {
            OFF = 0,
            ON = 255
        };

        LED(uint8_t pin, LED::StateEnum initialState = OFF);
        ~LED();

        void off();
        void on();
        void setState(LED::StateEnum state);
        void write(uint8_t value);

    private:
        class impl;
        std::unique_ptr<impl> p_impl;
};
