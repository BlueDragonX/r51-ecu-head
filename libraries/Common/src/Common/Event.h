#ifndef _R51_COMMON_EVENT_H_
#define _R51_COMMON_EVENT_H_

// Convenience macro for defining getters and setters on child events.
#define EVENT_PROPERTY(type, name, getter, setter) \
    type name() const { return (getter); } \
    bool name(type value) { \
        if ((getter) == value) { \
            return false; \
        } \
        (setter); \
        return true; \
    }

#include <Arduino.h>
#include <Canny.h>

namespace R51 {

// Sub-system identifier. This allows a physical to host multiple subsystems.
enum class SubSystem : uint8_t {
    CONTROLLER      = 0x00, // Common ECU functionality.
    ECM             = 0x04, // Events relating to engine control.
    IPDM            = 0x06, // Under hood and exterior power control.
    BCM             = 0x07, // Interior lightings, etc.
    TIRE            = 0x08, // Tire monitoring.
    CLIMATE         = 0x0A, // Climate control.
    SETTINGS        = 0x0B, // Vehicle settings management.
    BLUETOOTH       = 0x0D, // vehicle bluetooth connectivity.
    SCREEN          = 0x0E, // Vehicle infotainment screen.
    AUDIO           = 0x0F, // Audio controls.
    POWER           = 0x10, // Generic power control.
    KEYPAD          = 0x11, // Generic keypad control.
};

enum class ControllerEvent : uint8_t {
    REQUEST_CMD = 0x00, // Request state from the controller. Payload is the
                        // subsystem and state ID to retrieve or 0xFFFF for
                        // all states the controller owns.
};

struct Event : public Printable {
    // Subsystem identifier. This is the subsystem the event belongs to.
    uint8_t subsystem;
    // ID of the event. This uniquely identifies an event within a subsystem.
    uint8_t id;
    // Event data. Empty bytes are padded with 0xFF.
    uint8_t data[6];

    // Construct an empty system event. SubSystem and ID are set to 0x00.
    Event();

    // Construct a specific event with empty data.
    Event(uint8_t subsystem, uint8_t id);
    Event(SubSystem subsystem, uint8_t id);

    // Construct a specific event containing the provided data. The
    // size is set to the size of the provided data array. This array
    // should not exceed 5 bytes.
    Event(uint8_t subsystem, uint8_t id, const uint8_t* data, size_t size);
    template <size_t N> 
    Event(uint8_t subsystem, uint8_t id, const uint8_t (&data)[N]);
    template <size_t N> 
    Event(SubSystem subsystem, uint8_t id, const uint8_t (&data)[N]);

    // Print the system event.
    size_t printTo(Print& p) const override;
};

// Event class for the CONTROLLER:REQUEST_CMD event.
class RequestCommand : public Event {
    public:
        RequestCommand() :
            Event(SubSystem::CONTROLLER,
                (uint8_t)ControllerEvent::REQUEST_CMD,
                {0xFF, 0xFF}) {}

        RequestCommand(SubSystem request_subsystem) :
            Event(SubSystem::CONTROLLER,
                (uint8_t)ControllerEvent::REQUEST_CMD,
                {(uint8_t)request_subsystem, 0xFF}) {}

        RequestCommand(SubSystem request_subsystem, uint8_t request_id) :
            Event(SubSystem::CONTROLLER,
                (uint8_t)ControllerEvent::REQUEST_CMD,
                {(uint8_t)request_subsystem, request_id}) {}

        EVENT_PROPERTY(SubSystem, request_subsystem,
                (SubSystem)data[0], data[0] = (uint8_t)value);
        EVENT_PROPERTY(uint8_t, request_id, data[1], data[1] = value);

        static bool match(const Event& event, SubSystem subsystem, uint8_t id) {
            if (event.subsystem != (uint8_t)SubSystem::CONTROLLER ||
                    event.id != (uint8_t)ControllerEvent::REQUEST_CMD) {
                return false;
            }
            return (event.data[0] == 0xFF || event.data[0] == (uint8_t)subsystem) &&
                (event.data[1] == 0xFF || event.data[1] == id);
        }
};

// Return true if the two system events are equal
bool operator==(const Event& left, const Event& right);

// Return true if the two system events are not equal
bool operator!=(const Event& left, const Event& right);

template <size_t N>
Event::Event(uint8_t subsystem, uint8_t id, const uint8_t (&data)[N]) :
        Event(subsystem, id, data, N) {}

template <size_t N>
Event::Event(SubSystem subsystem, uint8_t id, const uint8_t (&data)[N]) :
        Event((uint8_t)subsystem, id, data, N) {}

}  // namespace R51

#endif  // _R51_COMMON_EVENT_H_
