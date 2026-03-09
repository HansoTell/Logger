#include "Logger.h"
#include <fstream>
#include <iostream>
#include <string>

enum ErrorCodes {
    eFileError = 0
};

struct Error {
    ErrorCodes e;
    std::string message;

    std::string toLog() const {
        std::string log = std::to_string(static_cast<int>(e));
        log += message;
        return log;
    }
};


int main(){
    std::ofstream ostr;

    CREATE_LOGGER("Log/Example.log");


    DESTROY_LOGGER();

    return 0;
}
