#ifndef _R51_CONSOLE_ROOT_H_
#define _R51_CONSOLE_ROOT_H_

#include <Arduino.h>
#include "Command.h"
#include "CAN.h"
#include "Error.h"
#include "Event.h"

namespace R51::internal {

// Root command.
class RootCommand : public Command {
    public:
        Command* next(char* arg) override {
            if (strcmp(arg, "can") == 0) {
                return &can_;
            } else if (strcmp(arg, "event") == 0) {
                return &event_;
            }
            return NotFoundCommand::get();
        }

        // Run the command. The command may yield messages or print information
        // to the user.
        void run(Stream* console, const Caster::Yield<Message>&) {
            console->println("console: command incomplete");
        }

    private:
        CANCommand can_;
        EventCommand event_;
};

}

#endif  // _R51_CONSOLE_ROOT_H_
