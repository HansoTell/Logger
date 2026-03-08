#pragma once

#include "gmock/gmock.h"
#include <gmock/gmock.h>
#include "Logger.h"

#define CREATE_MOCKLogger() Log::initLogger(std::make_unique<MOCKLoggerCore>())

class MOCKLoggerCore : public Log::ILoggerCore {
public:
    MOCK_METHOD(void, write, (std::string&&), (override));
};



