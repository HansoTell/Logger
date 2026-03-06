#include "gmock/gmock.h"
#include <gtest/gtest.h>
#include "Logger.h"

#include <LoggerCoreMock.h>
#include <memory>
#include <string>
#include <string_view>

using ::testing::_;
using ::testing::AllOf;
using ::testing::HasSubstr;

class LoggerTest : public ::testing::Test {
protected:
    Log::Logger* logger;
    std::unique_ptr<MOCKLoggerCore> pCore;
    MOCKLoggerCore* core;

    void SetUp() override{
        pCore = std::make_unique<MOCKLoggerCore>();
        core = pCore.get();

        logger = new Log::Logger(std::move(pCore));
    }

    void TearDown() override {
        delete logger;
    }
};


TEST(LoggerCreation, CreationTest){
    CREATE_MOCKLogger();

    ASSERT_NE(Log::pInstance, nullptr);
}
TEST(LoggerCrearion, CreationDoubleCall){
    CREATE_MOCKLogger();

    auto* pInstanceCopy = Log::pInstance;

    CREATE_MOCKLogger();

    EXPECT_EQ(pInstanceCopy, Log::pInstance);
}

TEST(LogLevel, setLogLevel){
    CREATE_MOCKLogger();

    EXPECT_EQ(Log::pInstance->getLogLevel(), Log::LogLevel::INFO);

    SET_LOG_LEVEL(Log::LogLevel::DEBUG);

    EXPECT_EQ(Log::pInstance->getLogLevel(), Log::LogLevel::DEBUG);
}

TEST_F(LoggerTest, LogUnderLoglevel){
    logger->setLogLevel(Log::LogLevel::ERROR);

    EXPECT_CALL(*core, addToMessageQueue(_)).Times(0);

    logger->INFO("Test");
}

TEST_F(LoggerTest, LogSuccess){
    EXPECT_CALL(*core, addToMessageQueue(AllOf(
        HasSubstr("INFO"),
        HasSubstr("Test")
    ))).Times(1);

    logger->INFO("Test");
}

TEST_F(LoggerTest, VLogUnderLogLevel){
    logger->setLogLevel(Log::LogLevel::ERROR);

    EXPECT_CALL(*core, addToMessageQueue(_)).Times(0);

    logger->VINFO("Test");
}
struct TestToLog{
    int a;
    float g;

    std::string toLog() const { return "ToLog"; }
};
struct strConvert {
    int a;
    float b;
    strConvert() : a(3), b(0.5f) {}
    strConvert(strConvert&& other) : a(other.a), b(other.b) { other.a = 0; other.b = 0.0f; }
    explicit operator std::string() const { return "string convert"; }
};

TEST_F(LoggerTest, VLogSuccess){
    EXPECT_CALL(*core, addToMessageQueue(AllOf(
        HasSubstr("INFO"),
        HasSubstr("Test"),
        HasSubstr("string"),
        HasSubstr("string"),
        HasSubstr("1"),
        HasSubstr("string convert"),
        HasSubstr("ToLog")
    ))).Times(1);

    std::string s = "view";
    std::string str = "string";
    std::string_view view (s);
    TestToLog toLog { 2, 0.4f };
    strConvert strConvert;
    logger->VINFO("Test", view, str, 1, strConvert, toLog);
}

TEST_F(LoggerTest, UseAfterVLog){
    EXPECT_CALL(*core, addToMessageQueue(_)).Times(1);

    strConvert a;

    logger->VINFO(a);

    EXPECT_EQ(a.a, 3);
    EXPECT_EQ(a.b, 0.5f);
}



