#include <gtest/gtest.h>
#include <memory>
#include "Logger.h"

TEST(AsyncFileWriter, openFile){
    std::unique_ptr<Log::FileWriter> fw = std::make_unique<Log::FileWriter>("Test.log");

    EXPECT_TRUE(fw->isOpen());
}

