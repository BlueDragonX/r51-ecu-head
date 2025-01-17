#include "Event.h"

namespace R51 {
namespace {

size_t printHexByte(Print& p, uint8_t b) {
    if (b <= 0x0F) {
        p.print("0");
    }
    p.print(b, HEX);
    return 2;
}

}

Event::Event() : subsystem(0), id(0), scratch(nullptr) {
    memset(data, 0xFF, 6);
}

Event::Event(uint8_t subsystem, uint8_t id) :
        subsystem(subsystem), id(id), scratch(nullptr) {
    memset(data, 0xFF, 6);
}

Event::Event(SubSystem subsystem, uint8_t id) : Event((uint8_t)subsystem, id) {}

Event::Event(uint8_t subsystem, uint8_t id, const uint8_t* data, size_t size) :
        Event(subsystem, id) {
    for (uint8_t i = 0; i < size; i++) {
        this->data[i] = data[i];
    }
    for (uint8_t i = size; i < 6; i++) {
        this->data[i] = 0xFF;
    }
}

size_t Event::printTo(Print& p) const {
    size_t n = 0;
    n += printHexByte(p, subsystem);
    n += p.print(":");
    n += printHexByte(p, id);
    n += p.print("#");
    for (uint8_t i = 0; i < 6; i++) {
        n += printHexByte(p, data[i]);
        if (i < 5) {
            n += p.print(":");
        }
    }
    return n;
}

bool operator==(const Event& left, const Event& right) {
    return left.subsystem == right.subsystem && left.id == right.id &&
        memcmp(left.data, right.data, 6) == 0;
}

// Return true if the two system events are not equal
bool operator!=(const Event& left, const Event& right) {
    return left.subsystem != right.subsystem || left.id != right.id ||
        memcmp(left.data, right.data, 6) != 0;
}

}  // namespace R51
