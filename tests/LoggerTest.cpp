#include <gtest/gtest.h>

#include "Logger.h"


TEST(LoggerCreation, CreationTest){
    CREATE_LOGGER("Test.log");

    ASSERT_NE(Log::pInstance, nullptr);
    EXPECT_EQ(Log::pInstance->getLogPath(), "Test.log");
}
TEST(LoggerCrearion, CreationDoubleCall){
    CREATE_LOGGER("Test.log");

    auto* pInstanceCopy = Log::pInstance;

    CREATE_LOGGER("NOTNew.log");

    EXPECT_EQ(pInstanceCopy, Log::pInstance);
    EXPECT_NE(Log::pInstance->getLogPath(), "NOTNew.log");
    EXPECT_EQ(Log::pInstance->getLogPath(), "Test.log");
}

TEST(LogLevel, setLogLevel){
    CREATE_LOGGER("Test.log");

    EXPECT_EQ(Log::pInstance->getLogLevel(), Log::LogLevel::INFO);

    SET_LOG_LEVEL(Log::LogLevel::DEBUG);

    EXPECT_EQ(Log::pInstance->getLogLevel(), Log::LogLevel::DEBUG);
}



