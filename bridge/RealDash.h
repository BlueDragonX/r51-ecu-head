#ifndef _R51_BRIDGE_REALDASH_
#define _R51_BRIDGE_REALDASH_

#include <Canny.h>
#include <Caster.h>
#include <Common.h>

// Caster node for communicating with RealDash. This converts Event messages to
// a format that's compatible with RealDash.
class RealDashAdapter : public Caster::Node<R51::Message> {
    public:
        // Construct a new RealDash node that communicates over the provided
        // CAN connection. Events are sent and received using the provided
        // can_id.
        //
        // If heartbeat_id is set then that a frame with that ID and a counter
        // in the first byte  is sent to RealDash every interval of
        // heartbeat_ms.
        RealDashAdapter(Canny::Connection* connection, uint32_t frame_id,
                uint32_t heartbeat_id = 0, uint32_t heartbeat_ms = 500) :
            connection_(connection), frame_id_(frame_id), hb_id_(heartbeat_id),
            hb_counter_(0), hb_ticker_(heartbeat_ms), frame_(8) {}

        // Encode and send an Event message to RealDash.
        void handle(const R51::Message& msg) override;

        // Yield received Events from RealDash.
        void emit(const Caster::Yield<R51::Message>& yield) override;

    private:
        Canny::Connection* connection_;
        uint32_t frame_id_;
        uint32_t hb_id_;
        uint8_t hb_counter_;
        R51::Ticker hb_ticker_;
        Canny::Frame frame_;
        R51::Event event_;
};

#endif  // _R51_BRIDGE_REALDASH_
