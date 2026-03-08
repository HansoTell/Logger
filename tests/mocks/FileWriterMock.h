#pragma once

#include "gmock/gmock.h"
#include <gmock/gmock.h>
#include "Logger.h"

class MOCKFileWriter : public Log::IFileWriter {
public:
    MOCK_METHOD(void, writeFile, (const std::string&), (override));
};

class FakeFileWriter : public Log::IFileWriter {
public:
    std::vector<std::string> messages;

    void writeFile(const std::string& msg) override {
        messages.push_back(msg);
    }
};
