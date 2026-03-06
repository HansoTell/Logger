#pragma once

#include "gmock/gmock.h"
#include <gmock/gmock.h>
#include "Logger.h"

#define CREATE_MOCKLogger() Log::initLogger(std::make_unique<MOCKLoggerCore>())

class MOCKLoggerCore : public Log::ILoggerCore {
    MOCK_METHOD(const std::string&, getLogPath, (), (const, override));
    MOCK_METHOD(void, addToMessageQueue, (std::string&& logEntry), (override));
};



