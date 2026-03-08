#include <gtest/gtest.h>
#include <memory>

#include "FileWriterMock.h"
#include "Logger.h"


class AsyncLoggerCoreTest : public ::testing::Test {
protected:
    Log::AsyncLoggerCore* asyncCore;
    FakeFileWriter* fakeFileWriter;
    std::unique_ptr<Log::IFileWriter> ffw;


    void SetUp() override {
        ffw = std::make_unique<FakeFileWriter>();
        fakeFileWriter = reinterpret_cast<FakeFileWriter*>(ffw.get());

        asyncCore = new Log::AsyncLoggerCore(std::move(ffw));
    }
    void TearDown() override {
        delete asyncCore;
    }
};

TEST_F(AsyncLoggerCoreTest, singleMessageProccessed){
    asyncCore->write("Test");

    asyncCore->stop();


    EXPECT_EQ(fakeFileWriter->messages.size(), 1);
    EXPECT_EQ(fakeFileWriter->messages[0], "Test");
}

TEST_F(AsyncLoggerCoreTest, multiMessagesProccessed){
    for(int i = 0; i < 100; i++){
        asyncCore->write("Test");
    }
    asyncCore->stop();

    EXPECT_EQ(fakeFileWriter->messages.size(), 100);
}

TEST_F(AsyncLoggerCoreTest, coorektOrder){
    asyncCore->write("Test");
    asyncCore->write("async");

    asyncCore->stop();

    EXPECT_EQ(fakeFileWriter->messages[0], "Test");
    EXPECT_EQ(fakeFileWriter->messages[1], "async");

}
TEST_F(AsyncLoggerCoreTest, flushedCalled){
    asyncCore->write("Test");
    asyncCore->stop();

    EXPECT_TRUE(fakeFileWriter->flushed);
}


