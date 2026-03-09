#include <gtest/gtest.h>
#include <memory>
#include "Logger.h"

TEST(AsyncFileWriter, openFile){
    std::unique_ptr<Log::AsyncFileWriter> fw = std::make_unique<Log::AsyncFileWriter>("Test.log");

    EXPECT_TRUE(fw->isOpen());
}

