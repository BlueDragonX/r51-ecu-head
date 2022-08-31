#ifndef _R51_STEERING_H_
#define _R51_STEERING_H_

#include <AnalogMultiButton.h>
#include <Caster.h>
#include <Common.h>
#include <Faker.h>

namespace R51 {

enum class SteeringKeypadEvent : uint8_t {
    POWER = 0x00,
    MODE = 0x01,
    SEEK_UP = 0x02,
    SEEK_DOWN = 0x03,
    VOLUME_UP = 0x04,
    VOLUME_DOWN = 0x05,
};

// Steering wheel keypad. Sends key press events when steering wheel buttons
// are pressed and released.
class SteeringKeypad : public Caster::Node<Message> {
    public:
        // Construct a new steering switch keypad object. Switches are
        // connected to GPIO pins sw_a_pin and sw_b_pin.
        SteeringKeypad(int sw_a_pin, int sw_b_pin, Faker::Clock* clock = Faker::Clock::real(),
                Faker::GPIO* gpio = Faker::GPIO::real());

        // Noop. This node does not process messages.
        void handle(const Message&) override {}

        // Emit a state frame on keypad state change.
        void emit(const Caster::Yield<Message>& yield) override;

    private:
        AnalogMultiButton sw_a_;
        AnalogMultiButton sw_b_;
};

}  // namespace R51

#endif  // _R51_STEERING_H_