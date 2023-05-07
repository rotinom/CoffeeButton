#include "LED.h"
#include <Arduino.h>
#include <limits>

class LED::impl {
    public:
        impl(uint8_t pin, LED::StateEnum initialState):
            _pin(pin)
        {
            pinMode(_pin, OUTPUT);
            digitalWrite(_pin, initialState);
        }

        ~impl(){}

        void off(){
            write(std::numeric_limits<uint8_t>::min());
        }

        void on(){
            write(std::numeric_limits<uint8_t>::max());
        }

        void setState(LED::StateEnum state){
            write(state);
        }

        void write(uint8_t value) {
            // Serial.printf("Setting LED - pin %u; value: %u\n", _pin, value);
            analogWrite(_pin, value);
        }

    private:
        uint8_t _pin;
};

// pimpl constructor
LED::LED(uint8_t pin, LED::StateEnum initialState):
    p_impl(new impl(pin, initialState))
{}

// Destructor has to be here for dumb reasons.  See:
// https://stackoverflow.com/questions/34072862/why-is-error-invalid-application-of-sizeof-to-an-incomplete-type-using-uniqu
LED::~LED(){}

// pimpl invokers
void LED::off() {p_impl -> off();}
void LED::on() {p_impl -> on();}
void LED::setState(LED::StateEnum state) {p_impl -> setState(state);}
void LED::write(uint8_t value) {p_impl -> write(value);}