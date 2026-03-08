#pragma once

#include "gmock/gmock.h"
#include <gmock/gmock.h>
#include <mutex>
#include "Logger.h"

class MOCKFileWriter : public Log::IFileWriter {
public:
    MOCK_METHOD(void, writeFile, (const std::string&), (override));
    MOCK_METHOD(void, flush, (), (override));
};

class FakeFileWriter : public Log::IFileWriter {
public:
    std::vector<std::string> messages;
    bool flushed = false;
    std::mutex mtx;

    void writeFile(const std::string& msg) override {
        std::lock_guard<std::mutex> _lock(mtx);
        messages.push_back(msg);
    }

    void flush() override { flushed = true; }
};
