#include "realdash.h"

#include <Arduino.h>
#include "binary.h"
#include "config.h"
#include "debug.h"


static const uint32_t kReceiveTimeout = 5000;

RealDashConnection::RealDashConnection() {
    stream_ = nullptr;
    reset();
}

void RealDashConnection::reset() {
    memset(checksum_buffer_, 0, 4);
    frame_type_66_ = false;
    frame44_checksum_ = 0;
    frame66_checksum_.reset();
    frame_size_ = 8;
    read_size_ = 0;
}

void RealDashConnection::begin(Stream* stream) {
    stream_ = stream;
#ifdef REALDASH_WAIT_FOR_SERIAL
    if (stream_ != nullptr) {
        while (!stream_) {
            INFO_MSG("realdash: serial not ready");
            delay(100);
        }
    }
#endif
}

void RealDashConnection::updateChecksum(byte b) {
    if (frame_type_66_) {
        frame66_checksum_.update(b);
    } else {
        frame44_checksum_ += b;
    }
}

bool RealDashConnection::read(uint32_t* id, uint8_t* len, byte* data) {
    if (stream_ == nullptr) {
        ERROR_MSG("realdash: not initialized");
        return false;
    }
    if (!stream_) {
        ERROR_MSG("realdash: not connected");
        return false;
    }
    if (readHeader() && readId(id) && readData(len, data) && validateChecksum()) {
        reset();
        return true;
    }
    return false;
}

bool RealDashConnection::readHeader() {
    byte b;
    while (stream_->available() && read_size_ < 4) {
        b = stream_->read();
        read_size_++;
        switch (read_size_) {
            case 1:
                if (b != 0x44 && b != 0x66) {
                    ERROR_MSG_VAL_FMT("realdash: unrecognized frame type ", b, HEX);
                    reset();
                    return false;
                }
                frame_type_66_ = (b == 0x66);
                break;
            case 2:
                if (b != 0x33) {
                    ERROR_MSG_VAL_FMT("realdash: invalid header byte 2 ", b, HEX);
                    reset();
                    return false;
                }
                break;
            case 3:
                if (b != 0x22) {
                    ERROR_MSG_VAL_FMT("realdash: invalid header byte 3 ", b, HEX);
                    reset();
                    return false;
                }
                break;
            case 4:
                if ((!frame_type_66_ && b != 0x11) || (frame_type_66_ && (b < 0x11 || b > 0x1F))) {
                    ERROR_MSG_VAL_FMT("realdash: invalid header byte 4 ", b, HEX);
                    reset();
                    return false;
                }
                frame_size_ = (b - 15) * 4;
                break;
            default:
                break;
        }
        updateChecksum(b);
    }
    return read_size_ >= 4;
}

bool RealDashConnection::readId(uint32_t* id) {
    uint32_t b;
    while (stream_->available() && read_size_ < 8) {
        b = stream_->read();
        read_size_++;
        updateChecksum(b);
        switch (read_size_-4) {
            case 1:
                *id = b;
                break;
            case 2:
                *id |= (b << 8);
                break;
            case 3:
                *id |= (b << 16);
                break;
            case 4:
                *id |= (b << 24);
                break;
        }
    }
    return read_size_ >= 8;
}

bool RealDashConnection::readData(uint8_t* len, byte* data) {
    while (stream_->available() && read_size_ - 8 < frame_size_) {
        data[read_size_-8] = stream_->read();
        updateChecksum(data[read_size_-8]);
        read_size_++;
    }
    *len = frame_size_;
    return read_size_ - 8 >= frame_size_;
}

bool RealDashConnection::validateChecksum() {
    if (frame_type_66_) {
        while (stream_->available() && read_size_ - 8 - frame_size_ < 4) {
            checksum_buffer_[read_size_ - 8 - frame_size_] = stream_->read();
            read_size_++;
        }
        if (read_size_ - 8 - frame_size_ < 4) {
            return false;
        }
        if (frame66_checksum_.finalize() != *((uint32_t*)checksum_buffer_)) {
            ERROR_MSG_VAL_FMT("realdash: frame 0x66 checksum error, wanted ", frame66_checksum_.finalize(), HEX);
            reset();
            return false;
        }
    } else {
        if (read_size_ - 8 - frame_size_ < 1) {
            if (!stream_->available()) {
                return false;
            }
            checksum_buffer_[0] = stream_->read();
            read_size_++;
        }
        if (frame44_checksum_ != checksum_buffer_[0]) {
            ERROR_MSG_VAL_FMT("realdash: frame 0x44 checksum error, wanted ", frame44_checksum_, HEX);
            reset();
            return false;
        }
    }
    return true;
}

void RealDashConnection::writeByte(const byte b) {
    stream_->write(b);
    write_checksum_.update(b);
}

void RealDashConnection::writeBytes(const byte* b, uint8_t len) {
    for (int i = 0; i < len; i++) {
        writeByte(b[i]);
    }
}

bool RealDashConnection::write(uint32_t id, uint8_t len, byte* data) {
    if (stream_ == nullptr) {
        ERROR_MSG("realdash: not initialized");
        return false;
    }
    if (!stream_) {
        ERROR_MSG("realdash: not connected");
        return false;
    }
    if (data == nullptr) {
        len = 0;
    }
    if (len > 64 || len % 4 != 0) {
        ERROR_MSG_VAL("realdash: frame write error, invalid length ", len);
        return false;
    }

    write_checksum_.reset();

    byte size = len / 4 + 15;
    writeByte(0x66);
    writeByte(0x33);
    writeByte(0x22);
    writeByte(size);
    writeBytes((const byte*)&id, 4);
    if (data != nullptr) {
        writeBytes(data, len);
    }
    for (int i = 0; i < 8-len; i++) {
        writeByte(0);
    }
    uint32_t checksum = write_checksum_.finalize();
    stream_->write((const byte*)&checksum, 4);
    stream_->flush();
    return true;
}

void RealDashClimate::connect(Connection* realdash, ClimateController* climate) {

    if (repeat_ < 1) {
        repeat_ = 1;
    }
    realdash_ = realdash;
    climate_ = climate;
    last_write_ = 0;
    last_read_ = millis();      // for control state timeouts
    write_count_ = repeat_;     // force a write on start
    memset(frame5400_, 0, 8);
    memset(frame5401_, 0, 8);
}

void RealDashClimate::setClimateActive(bool value) {
    setBit(frame5400_, 0, 0, value);
    write_count_ = 0;
}

void RealDashClimate::setClimateAuto(bool value) {
    setBit(frame5400_, 0, 1, value);
    write_count_ = 0;
}

void RealDashClimate::setClimateAc(bool value) {
    setBit(frame5400_, 0, 2, value);
    write_count_ = 0;
}

void RealDashClimate::setClimateDual(bool value) {
    setBit(frame5400_, 0, 3, value);
    write_count_ = 0;
}

void RealDashClimate::setClimateFace(bool value) {
    setBit(frame5400_, 0, 4, value);
    write_count_ = 0;
}

void RealDashClimate::setClimateFeet(bool value) {
    setBit(frame5400_, 0, 5, value);
    write_count_ = 0;
}

void RealDashClimate::setClimateRecirculate(bool value) {
    setBit(frame5400_, 0, 7, value);
    write_count_ = 0;
}

void RealDashClimate::setClimateFrontDefrost(bool value) {
    setBit(frame5400_, 0, 6, value);
    write_count_ = 0;
}

void RealDashClimate::setClimateRearDefrost(bool value) {
    setBit(frame5400_, 4, 0, value);
    write_count_ = 0;
}

void RealDashClimate::setClimateFanSpeed(uint8_t value) {
    frame5400_[1] = value;
    write_count_ = 0;
}

void RealDashClimate::setClimateDriverTemp(uint8_t value) {
    frame5400_[2] = value;
    write_count_ = 0;
}

void RealDashClimate::setClimatePassengerTemp(uint8_t value) {
    frame5400_[3] = value;
    write_count_ = 0;
}

void RealDashClimate::setClimateOutsideTemp(uint8_t value) {
    frame5400_[7] = value;
    write_count_ = 0;
}

void RealDashClimate::push() {
    if (realdash_ == nullptr) {
        return;
    }
    if (write_count_ < repeat_ || millis() - last_write_ >= 500) {
        if (write_count_ == 0) {
            INFO_MSG_FRAME("realdash: send ", 0x5400, 8, frame5400_);
        }
        if (write_count_ < repeat_) {
            write_count_++;
        }
        realdash_->write(0x5400, 8, frame5400_);
        last_write_ = millis();
    }
}

void RealDashClimate::receive(uint32_t id, uint8_t len, byte* data) {
    if (climate_ == nullptr || id != 0x5401 || len != 8) {
        return;
    }

    D(bool changed = false;)

    // reset internal state if we haven't received a frame recently
    if (millis() - last_read_ > kReceiveTimeout) {
        INFO_MSG("realdash: climate state reset, control frame timeout exceeded");
        memset(frame5401_, 0, 8);
        D(changed = true;)
    }
    last_read_ = millis();

    // check if any bits have flipped
    if (xorBits(frame5401_, data, 0, 0)) {
        climate_->climateClickOff();
        D(changed = true;)
    }
    if (xorBits(frame5401_, data, 0, 1)) {
        climate_->climateClickAuto();
        D(changed = true;)
    }
    if (xorBits(frame5401_, data, 0, 2)) {
        climate_->climateClickAc();
        D(changed = true;)
    }
    if (xorBits(frame5401_, data, 0, 3)) {
        climate_->climateClickDual();
        D(changed = true;)
    }
    if (xorBits(frame5401_, data, 0, 4)) {
        climate_->climateClickMode();
        D(changed = true;)
    }
    if (xorBits(frame5401_, data, 0, 6)) {
        climate_->climateClickFrontDefrost();
        D(changed = true;)
    }
    if (xorBits(frame5401_, data, 0, 7)) {
        climate_->climateClickRecirculate();
        D(changed = true;)
    }
    if (xorBits(frame5401_, data, 1, 0)) {
        climate_->climateClickFanSpeedUp();
        D(changed = true;)
    }
    if (xorBits(frame5401_, data, 1, 1)) {
        climate_->climateClickFanSpeedDown();
        D(changed = true;)
    }
    if (xorBits(frame5401_, data, 1, 2)) {
        climate_->climateClickDriverTempUp();
        D(changed = true;)
    }
    if (xorBits(frame5401_, data, 1, 3)) {
        climate_->climateClickDriverTempDown();
        D(changed = true;)
    }
    if (xorBits(frame5401_, data, 1, 4)) {
        climate_->climateClickPassengerTempUp();
        D(changed = true;)
    }
    if (xorBits(frame5401_, data, 1, 5)) {
        climate_->climateClickPassengerTempDown();
        D(changed = true;)
    }
    if (xorBits(frame5401_, data, 4, 0)) {
        climate_->climateClickRearDefrost();
        D(changed = true;)
    }

    // update the stored frame
    memcpy(frame5401_, data, 8);

    D({
        if (changed) {
            INFO_MSG_FRAME("realdash: receive ", 0x5401, 8, frame5401_);
        }
    })
}

void RealDashSettings::connect(Connection* realdash, SettingsController* settings) {
    if (repeat_ < 1) {
        repeat_ = 1;
    }
    realdash_ = realdash;
    settings_ = settings;
    last_read_ = millis();      // for control state timeouts
    write_count_ = repeat_;     // force a write on start
    memset(frame5700_, 0, 8);
    memset(frame5701_, 0, 8);
}

void RealDashSettings::setAutoInteriorIllumination(bool value) {
    setBit(frame5700_, 0, 0, value);
    write_count_ = 0;
}

void RealDashSettings::setAutoHeadlightSensitivity(uint8_t value) {
    if (value > 3) {
        value = 3;
    }
    frame5700_[1] &= ~0x03;
    frame5700_[1] |= value;
    write_count_ = 0;
}

void RealDashSettings::setAutoHeadlightOffDelay(AutoHeadlightOffDelay value) {
    frame5700_[1] &= ~0xF0;
    frame5700_[1] |= value << 4;
    write_count_ = 0;
}

void RealDashSettings::setSpeedSensingWiperInterval(bool value) {
    setBit(frame5700_, 0, 2, value);
    write_count_ = 0;
}

void RealDashSettings::setRemoteKeyResponseHorn(bool value) {
    setBit(frame5700_, 3, 0, value);
    write_count_ = 0;
}

void RealDashSettings::setRemoteKeyResponseLights(RemoteKeyResponseLights value) {
    frame5700_[3] &= ~0x30;
    frame5700_[3] |= value << 2;
    write_count_ = 0;
}

void RealDashSettings::setAutoReLockTime(AutoReLockTime value) {
    frame5700_[2] &= ~0xF0;
    frame5700_[2] |= value << 4;
    write_count_ = 0;
}

void RealDashSettings::setSelectiveDoorUnlock(bool value) {
    setBit(frame5700_, 2, 0, value);
    write_count_ = 0;
}

void RealDashSettings::setSlideDriverSeatBackOnExit(bool value) {
    setBit(frame5700_, 0, 1, value);
    write_count_ = 0;
}

void RealDashSettings::push() {
    if (realdash_ == nullptr) {
        return;
    }
    if (write_count_ < repeat_) {
        if (write_count_ == 0) {
            INFO_MSG_FRAME("realdash: send ", 0x5700, 8, frame5700_);
        }
        if (write_count_ < repeat_) {
            write_count_++;
        }
        realdash_->write(0x5700, 8, frame5700_);
    }
}

void RealDashSettings::receive(uint32_t id, uint8_t len, byte* data) {
    if (settings_ == nullptr || id != 0x5701 || len != 8) {
        return;
    }

    D(bool changed = false;)

    // reset internal state if we haven't received a frame recently
    if (millis() - last_read_ > kReceiveTimeout) {
        INFO_MSG("realdash: settings state reset, control frame timeout exceeded");
        memset(frame5701_, 0, 8);
        D(changed = true;)
    }
    last_read_ = millis();

    // check if any bits have flipped
    if (xorBits(frame5701_, data, 0, 0)) {
        settings_->toggleAutoInteriorIllumination();
        D(changed = true;)
    }
    if (xorBits(frame5701_, data, 0, 1)) {
        settings_->toggleSlideDriverSeatBackOnExit();
        D(changed = true;)
    }
    if (xorBits(frame5701_, data, 0, 2)) {
        settings_->toggleSpeedSensingWiperInterval();
        D(changed = true;)
    }
    if (xorBits(frame5701_, data, 1, 0)) {
        settings_->nextAutoHeadlightSensitivity();
        D(changed = true;)
    }
    if (xorBits(frame5701_, data, 1, 1)) {
        settings_->prevAutoHeadlightSensitivity();
        D(changed = true;)
    }
    if (xorBits(frame5701_, data, 1, 4)) {
        settings_->nextAutoHeadlightOffDelay();
        D(changed = true;)
    }
    if (xorBits(frame5701_, data, 1, 5)) {
        settings_->prevAutoHeadlightOffDelay();
        D(changed = true;)
    }
    if (xorBits(frame5701_, data, 2, 0)) {
        settings_->toggleSelectiveDoorUnlock();
        D(changed = true;)
    }
    if (xorBits(frame5701_, data, 2, 4)) {
        settings_->nextAutoReLockTime();
        D(changed = true;)
    }
    if (xorBits(frame5701_, data, 2, 5)) {
        settings_->prevAutoReLockTime();
        D(changed = true;)
    }
    if (xorBits(frame5701_, data, 3, 0)) {
        settings_->toggleRemoteKeyResponseHorn();
        D(changed = true;)
    }
    if (xorBits(frame5701_, data, 3, 2)) {
        settings_->nextRemoteKeyResponseLights();
        D(changed = true;)
    }
    if (xorBits(frame5701_, data, 3, 3)) {
        settings_->prevRemoteKeyResponseLights();
        D(changed = true;)
    }
    if (xorBits(frame5701_, data, 7, 0)) {
        settings_->retrieveSettings();
        D(changed = true;)
    }
    if (xorBits(frame5701_, data, 7, 7)) {
        settings_->resetSettingsToDefault();
        D(changed = true;)
    }

    // update the stored frame
    memcpy(frame5701_, data, 8);

    D({
        if (changed) {
            INFO_MSG_FRAME("realdash: receive ", 0x5701, 8, frame5701_);
        }
    })
}

void RealDashKeypad::connect(Connection* realdash) {
    if (repeat_ < 1) {
        repeat_ = 1;
    }
    realdash_ = realdash;
    last_write_ = 0;
    write_count_ = repeat_;     // force a write on start
    memset(frame5800_, 0, 8);
}

void RealDashKeypad::press(KeypadController::Button button) {
    switch (button) {
        case KeypadController::POWER:
            setBit(frame5800_, 0, 0, 1);
            break;
        case KeypadController::MODE:
            setBit(frame5800_, 0, 1, 1);
            break;
        case KeypadController::VOLUME_UP:
            setBit(frame5800_, 0, 2, 1);
            break;
        case KeypadController::VOLUME_DOWN:
            setBit(frame5800_, 0, 3, 1);
            break;
        case KeypadController::SEEK_UP:
            setBit(frame5800_, 0, 4, 1);
            break;
        case KeypadController::SEEK_DOWN:
            setBit(frame5800_, 0, 5, 1);
            break;
        default:
            break;
    }
    last_write_ = 0;
}

void RealDashKeypad::release(KeypadController::Button button) {
    switch (button) {
        case KeypadController::POWER:
            setBit(frame5800_, 0, 0, 0);
            break;
        case KeypadController::MODE:
            setBit(frame5800_, 0, 1, 0);
            break;
        case KeypadController::VOLUME_UP:
            setBit(frame5800_, 0, 2, 0);
            break;
        case KeypadController::VOLUME_DOWN:
            setBit(frame5800_, 0, 3, 0);
            break;
        case KeypadController::SEEK_UP:
            setBit(frame5800_, 0, 4, 0);
            break;
        case KeypadController::SEEK_DOWN:
            setBit(frame5800_, 0, 5, 0);
            break;
        default:
            break;
    }
    last_write_ = 0;
}

void RealDashKeypad::push() {
    if (realdash_ == nullptr) {
        return;
    }
    if (write_count_ < repeat_ || millis() - last_write_ >= 500) {
        if (write_count_ == 0) {
            INFO_MSG_FRAME("realdash: send ", 0x5800, 8, frame5800_);
        }
        if (write_count_ < repeat_) {
            write_count_++;
        }
        realdash_->write(0x5800, 8, frame5800_);
        last_write_ = millis();
    }
}
