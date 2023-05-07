#pragma once
#include <memory>

class SmartButton {

    public:
        SmartButton(uint8_t pin, uint8_t inputMode, uint32_t debounceMillis = 100);
        ~SmartButton();

        void tick();
        uint8_t getSwitchValue();

        void setInverted(bool value);
        bool getInverted();

        typedef void(*noArgsCallback_t)();

        void setPressedCallback(noArgsCallback_t callback);
        void setReleasedCallback(noArgsCallback_t callback);

        void setLongPressCallback(noArgsCallback_t callback);
        void setLongPressReleasedCallback(noArgsCallback_t callback);

    private:
        class impl;
        std::unique_ptr<impl> _impl;
};
