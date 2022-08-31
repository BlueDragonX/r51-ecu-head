#ifndef _R51_VEHICLE_ECM_H_
#define _R51_VEHICLE_ECM_H_

#include <Arduino.h>
#include <Caster.h>
#include <Common.h>
#include <Faker.h>

namespace R51 {

enum class ECMEvent : uint8_t {
    ENGINE_TEMP_STATE = 0x00,
    REQUEST = 0x01,
};

// Track reported coolant temperature from the ECM via the 0x551 CAN frame.
class EngineTempState : public Caster::Node<Message> {
    public:
        EngineTempState(uint32_t tick_ms = 0, Faker::Clock* clock = Faker::Clock::real()) :
            changed_(false), event_((uint8_t)SubSystem::ECM, (uint8_t)ECMEvent::ENGINE_TEMP_STATE, {0x00}), ticker_(tick_ms, clock) {}

        // Handle ECM 0x551 state frames. Returns true if the state changed as
        // a result of handling the frame.
        void handle(const Message& msg) override;

        // Yield an ENGINE_TEMP_STATE frame on change or tick.
        void emit(const Caster::Yield<Message>& yield) override;

    private:
        bool changed_;
        Event event_;
        Ticker ticker_;
};

}  // namespace R51

#endif  // _R51_VEHICLE_ECM_H_