#pragma once

#include <string>

class ICommand {
public:
    virtual ~ICommand() = default;

    virtual void execute() = 0;
    virtual void undo() = 0;
    virtual void redo() { execute(); }
     virtual std::string getDescription() const = 0;
};
