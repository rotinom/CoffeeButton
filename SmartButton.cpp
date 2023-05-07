#include "SmartButton.h"
#include <Arduino.h>
#include "debounce.h"
#include <functional>



class SmartButton::impl {
    private:
        void println(const char * fmt, ...){
              va_list va;
              va_start (va, fmt);
              vsnprintf (_buffer, sizeof(_buffer), fmt, va);
              va_end (va);
              Serial.println(_buffer);
        }

    public:
        impl(uint8_t pin, uint8_t inputMode, bool inverted, uint32_t debounceDuration):
            _pin(pin),
            _inputMode(inputMode),
            button(pin, nullptr),
            _debounceDuration(debounceDuration),
            _lastSwitchValue(0),
            _longPressStart(0),
            _notifyLongPress(false),
            _pressedCallback(nullptr),
            _releasedCallback(nullptr),
            _longPressCallback(nullptr),
            _longPressReleasedCallback(nullptr),
            _inverted(inverted)
        {
            println("%s:%d - pin: %d; inputMode: %d; debounceDuration: %u", __FILE__, __LINE__, pin, inputMode, debounceDuration);

            println("%s:%d - _pin: %d; _inputMode: %d; _debounceDuration: %u", __FILE__, __LINE__, _pin, _inputMode, _debounceDuration);

            pinMode(pin, inputMode);
            button.setPushDebounceInterval(_debounceDuration);
            button.setReleaseDebounceInterval(_debounceDuration);
        }

        void tick() { 

            // Read the pin and normalize it to 0/1 based on inverted value
            int rawVal = digitalRead(_pin);
            // Serial.printf("rawVal: %ld;\n",  rawVal);
            uint8_t value = 0;
            if(_inverted) {
                // Serial.println("Inverted..");
                value = (rawVal == LOW) ? HIGH : LOW;
                // Serial.printf("value: %d\n", value);
            }
            else {
                // Serial.println("Not Inverted..");
                value = (rawVal == LOW) ? LOW : HIGH;
                // Serial.printf("value: %d\n", value);
            }

            if(!button.update(value)) {
              // return;
            }

            value = button.getState();
            // uint32_t now = millis();

            // println("%s:%d - now: %u; value: %d; _lastSwitchValue: %d", __FILE__, __LINE__, now, value, _lastSwitchValue);
            // if(value != _lastSwitchValue) {
            //     println("Setting _lastDebounceTime: %u;", now);
            //     _lastDebounceTime = now;
            //     println("_lastDebounceTime is: %u;", _lastDebounceTime);
            // }


            // println("%s:%d - now: %u; _lastDebounceTime: %u; _debounceDuration: %u", __FILE__, __LINE__, now, _lastDebounceTime, _debounceDuration);

            // // If we can trust the value, use it
            // if ((now - _lastDebounceTime) > _debounceDuration) {

                // println("%s:%d - LOW == _lastSwitchValue: %d; HIGH == value: %d", __FILE__, __LINE__, LOW == _lastSwitchValue, HIGH == value);


                // Rising edge
                if((LOW == _lastSwitchValue) && (HIGH == value))  {
                    // Serial.println("Pressed...");
                    _longPressStart = millis();
                    if(_pressedCallback) {
                        _pressedCallback();
                    }
                }

                // Falling edge
                else if((HIGH == _lastSwitchValue) && (LOW == value)) {
                    // Serial.println("Released...");
                    if(_notifyLongPress && _longPressReleasedCallback) {
                        _longPressReleasedCallback();
                    }
                    if(_releasedCallback) {
                        _releasedCallback();
                    }
                    _notifyLongPress = false;
                }

                // Check for long press
                else if((HIGH == _lastSwitchValue) && (HIGH == value))  {
                    // Serial.println("Long Press?");
                    if(millis() >= (_longPressStart + 1000)) {
                    //   Serial.println("Long Press!");
                      
                      if(_longPressCallback) {
                        _longPressCallback();
                      }
                      _notifyLongPress = true;
                    }
                }
            // }
            // else {
            //   println("Debouncing...");
            // }

            // println("%s:%d - _lastSwitchValue = %d", __FILE__, __LINE__, value);
            _lastSwitchValue = value;


            // delay(500);
            

            // // If the switch didn't change, do nothing 
            // if(value == _switchValue){
            //     return;
            // }

            // // Debounce if needed
            // uint64_t currentTimeMillis = millis();

            // // if(_lastSwitchUpdateMillis + _debounce_ms > currentTime) {
            // //     Serial.println("Debouncing a...");
            // //     return;
            // // }

            // // Rising edge
            // if(_switchValue == LOW && 
            //    value == HIGH) 
            // {
            //     Serial.println("Pressed...");
            //     if(_pressedCallback) {_pressedCallback.get();}
            // }

            // // Falling edge
            // else if(_switchValue == HIGH && 
            //         value == LOW) 
            // {
            //     Serial.println("Released...");
            //     if(_releasedCallback) {_releasedCallback.get();}
            // }

            // Serial.printf("lastTime = %lu; debounce_ms = %lu; currentTime = %lu\n",
            //                 _lastSwitchUpdateMillis,
            //                 _debounceMillis,
            //                 currentTimeMillis);

            // // Save current
            // _lastSwitchUpdateMillis = currentTimeMillis;
            // _switchValue = value;
        }

        uint8_t getSwitchValue(){
            return _lastSwitchValue;
        }

        void setInverted(bool value){
            _inverted = value;
        }

        bool getInverted(){
            return _inverted;
        }

        void setPressedCallback(noArgsCallback_t callback){
            _pressedCallback = callback;
        }

        void setReleasedCallback(noArgsCallback_t callback){
            _releasedCallback = callback;
        }

        void setLongPressCallback(noArgsCallback_t callback){
            _longPressCallback = callback;
        }
        void setLongPressReleasedCallback(noArgsCallback_t callback){
            _longPressReleasedCallback = callback;
        }

    private:
        char _buffer[256];
        uint8_t  _pin;
        uint8_t  _inputMode;
        
        // uint8_t  _switchValue;
        uint8_t _lastSwitchValue;
        // uint64_t _lastSwitchUpdateMillis;

        Button button;

        uint32_t _debounceDuration;
        uint32_t _longPressStart;
        bool _notifyLongPress;
        // uint32_t _lastDebounceTime;


        bool _inverted = false;

        noArgsCallback_t _pressedCallback;
        noArgsCallback_t _releasedCallback;
        noArgsCallback_t _longPressCallback;
        noArgsCallback_t _longPressReleasedCallback; 
};

// pimpl constructor
SmartButton::SmartButton(uint8_t pin, uint8_t inputMode, bool inverted, uint32_t debounceMillis) :
    _impl(new impl(pin, inputMode, inverted, debounceMillis)) {

    // snprintf(_buffer, sizeof(buffer), "%s:%d - pin: %d; inputMode: %d; debounceMillis: %lu", __FILE__, __LINE__, pin, inputMode, debounceMillis);
    // Serial.println(buffer);
}

// Destructor has to be here for dumb reasons.  See:
// https://stackoverflow.com/questions/34072862/why-is-error-invalid-application-of-sizeof-to-an-incomplete-type-using-uniqu
SmartButton::~SmartButton(){}

void SmartButton::tick(){_impl->tick();}
uint8_t SmartButton::getSwitchValue(){return _impl->getSwitchValue();}

bool SmartButton::getInverted(){return _impl->getInverted();}
void SmartButton::setInverted(bool value){_impl->setInverted(value);}

void SmartButton::setPressedCallback(SmartButton::noArgsCallback_t callback){_impl->setPressedCallback(callback);}
void SmartButton::setReleasedCallback(SmartButton::noArgsCallback_t callback){_impl->setReleasedCallback(callback);}

void SmartButton::setLongPressCallback(SmartButton::noArgsCallback_t callback){_impl->setLongPressCallback(callback);}
void SmartButton::setLongPressReleasedCallback(SmartButton::noArgsCallback_t callback){_impl->setLongPressReleasedCallback(callback);}