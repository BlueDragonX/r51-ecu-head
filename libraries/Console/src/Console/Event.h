#ifndef _R51_CONSOLE_EVENT_H_
#define _R51_CONSOLE_EVENT_H_

#include <Arduino.h>
#include <Caster.h>
#include <Common.h>
#include "Command.h"
#include "Console.h"
#include "Error.h"

namespace R51::internal {

class EventSendRunCommand : public Command {
    public:
        Command* next(char*) override {
            return TooManyArgumentsCommand::get();
        }

        // Run the command. Parses and yields the event.
        void run(Console* console, char* arg, const Caster::Yield<Message>& yield) override;

    private:
        Event event_;
};

class EventSendCommand : public NotEnoughArgumentsCommand {
    public:
        Command* next(char*) override {
            return &run_;
        }

    private:
        EventSendRunCommand run_;
};

class EventCommand : public NotEnoughArgumentsCommand {
    public:
        Command* next(char* arg) override {
            if (strcmp(arg, "send") == 0)  {
                return &send_;
            }
            return NotFoundCommand::get();
        }

    private:
        EventSendCommand send_;
};

}  // namespace R51::internal

#endif  // _R51_CONSOLE_EVENT_H_
