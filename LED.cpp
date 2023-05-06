#include "LED.h"
#include <Arduino.h>

class LED::impl {

    public:
        impl(uint8_t pin):
            pin(pin)
        {
            pinMode(pin, OUTPUT);
        }

        ~impl(){}

        void off(){
            digitalWrite(pin, LOW);
        }

        void on(){
            digitalWrite(pin, HIGH);
        }

        void setState(uint8_t state){
            digitalWrite(pin, state);
        }


    private:
        uint8_t pin;
};

// pimpl constructor
LED::LED(uint8_t pin):
    p_impl(new impl(pin))
{}

// Destructor has to be here for dumb reasons.  See:
// https://stackoverflow.com/questions/34072862/why-is-error-invalid-application-of-sizeof-to-an-incomplete-type-using-uniqu
LED::~LED(){}

// pimpl invokers
void LED::off() {p_impl -> off();}
void LED::on() {p_impl -> on();}
void LED::setState(uint8_t state) {p_impl -> setState(state);}