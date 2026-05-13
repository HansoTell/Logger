#pragma once

#include <memory>
#include <string>
#include <iostream>
#include <fstream>
#include <ctime>
#include <mutex>
#include <thread>
#include <queue>
#include <condition_variable>
#include <atomic>
#include <string_view>
#include <cstring>
#include <system_error>
#include <chrono>
#include <filesystem>
#include <type_traits>

#define CURRENT_LOCATION_LOG ::Log::SourceLocation{__FILE__, __func__, __LINE__}

#define CREATE_LOGGER(file) Log::initLogger(std::make_unique<Log::AsyncLoggerCore>(std::make_unique<Log::AsyncFileWriter>(file)))
#define DESTROY_LOGGER() Log::destroyLogger()

#define SET_LOG_LEVEL(level) Log::pInstance->setLogLevel(level)

#ifndef NDEBUG
#define LOG_DEBUG(msg) Log::pInstance->log(Log::LogLevel::DEBUG, msg, CURRENT_LOCATION_LOG)
#define LOG_VDEBUG(...) Log::pInstance->var_Log(Log::LogLevel::DEBUG, CURRENT_LOCATION_LOG, __VA_ARGS__)
#else
#define LOG_DEBUG(msg) 
#define LOG_VDEBUG(...)
#endif

#define LOG_INFO(msg) Log::pInstance->log(Log::LogLevel::INFO, msg, CURRENT_LOCATION_LOG)
#define LOG_VINFO(...) Log::pInstance->var_Log(Log::LogLevel::INFO, CURRENT_LOCATION_LOG, __VA_ARGS__)
#define LOG_WARNING(msg) Log::pInstance->log(Log::LogLevel::WARNING, msg, CURRENT_LOCATION_LOG)
#define LOG_VWARNING(...) Log::pInstance->var_Log(Log::LogLevel::WARNING, CURRENT_LOCATION_LOG, __VA_ARGS__)
#define LOG_ERROR(msg) Log::pInstance->log(Log::LogLevel::ERROR, msg, CURRENT_LOCATION_LOG)
#define LOG_VERROR(...) Log::pInstance->var_Log(Log::LogLevel::ERROR, CURRENT_LOCATION_LOG, __VA_ARGS__)
#define LOG_CRITICAL(msg) Log::pInstance->log(Log::LogLevel::CRITICAL, msg, CURRENT_LOCATION_LOG)
#define LOG_VCRITICAL(...) Log::pInstance->var_Log(Log::LogLevel::CRITICAL, CURRENT_LOCATION_LOG, __VA_ARGS__)



#ifndef NDEBUG
#define DEBUG(msg) log(Log::LogLevel::DEBUG, msg, CURRENT_LOCATION_LOG)
#define VDEBUG(...) var_Log(Log::LogLevel::DEBUG, CURRENT_LOCATION_LOG, __VA_ARGS__)
#else
#define DEBUG(msg) 
#define VDEBUG(...)
#endif
#define INFO(msg) log(Log::LogLevel::INFO, msg, CURRENT_LOCATION_LOG)
#define VINFO(...) var_Log(Log::LogLevel::INFO, CURRENT_LOCATION_LOG, __VA_ARGS__)
#define WARNING(msg) log(Log::LogLevel::WARNING, msg, CURRENT_LOCATION_LOG)
#define VWARNING(...) var_Log(Log::LogLevel::WARNING, CURRENT_LOCATION_LOG, __VA_ARGS__)
#define ERROR(msg) log(Log::LogLevel::ERROR, msg, CURRENT_LOCATION_LOG)
#define VERROR(...) var_Log(Log::LogLevel::ERROR, CURRENT_LOCATION_LOG, __VA_ARGS__)
#define CRITICAL(msg) log(Log::LogLevel::CRITICAL, msg, CURRENT_LOCATION_LOG)
#define VCRITICAL(...) var_Log(Log::LogLevel::CRITICAL, CURRENT_LOCATION_LOG, __VA_ARGS__)

#define MAXLOGSIZE 5*1024*1024

namespace Log{
    template<typename T, typename = void>
    struct has_toLog : std::false_type {};

    template<typename T>
    struct has_toLog<T, std::void_t<decltype(std::declval<const T&>().toLog())>> : std::true_type {};

    template<typename T>
    inline constexpr bool has_toLog_v = has_toLog<T>::value;

    template<typename T>
    inline constexpr bool is_logable_v = std::is_arithmetic_v<T> || std::is_constructible_v<std::string, T> || has_toLog_v<T>;

    enum LogLevel{
        DEBUG = 0, INFO, WARNING, ERROR, CRITICAL
    };

    struct SourceLocation {
        const char* File;
        const char* Function;
        int line;
    };

    inline std::ostream& operator<<(std::ostream& os, const SourceLocation& location){ return os << location.File << ":" << location.line << " " << location.Function; }
    

    class IFileWriter {
    public:
        virtual ~IFileWriter() = default;
        virtual void writeFile( const std::string& logEntry ) = 0;
        virtual void flush() = 0;
    };

    class AsyncFileWriter : public IFileWriter {
    public:
    
        void writeFile( const std::string& logEntry ) override {
            size_t logFileSize = std::filesystem::file_size(m_logPath);
            if( logFileSize + logEntry.size() > MAXLOGSIZE ){
                flush();
                changeLogFileIfNeeded();
            }

            m_logFile << logEntry;
        }

        void flush() override { m_logFile.flush(); }

        //Test method
        bool isOpen() const { return m_logFile.is_open(); }
        
    public:
        AsyncFileWriter( std::string file ) : m_logPath(std::move(file)){

            m_logFile.open(m_logPath, std::ios::app);
            if(!m_logFile.is_open())
                std::cerr << "Failed to open Log file" << "\n";
        }
        AsyncFileWriter(const AsyncFileWriter& other) = delete;
        AsyncFileWriter(AsyncFileWriter&& other) = delete;
        ~AsyncFileWriter() {
            if(m_logFile.is_open())
                m_logFile.close();
        }
    private:
        bool changeLogFileIfNeeded() {

            if( !std::filesystem::exists(m_logPath) )
                return false;

            if( m_logFile.is_open() )
                m_logFile.close();

            char newLogName[1024]; 
            buildDatName(newLogName);

            std::string_view newLogName_c (newLogName);

            if( std::filesystem::exists(newLogName_c)){
                m_logFile.open(m_logPath, std::ios::app);

                if( !m_logFile.is_open() )
                    std::cerr << "Couldt open new Log File after rotating" << "\n";

                return false;
            }

            std::error_code ec;
            std::filesystem::rename(m_logPath, newLogName_c, ec);

            m_logFile.open(m_logPath, std::ios::app);

            if( !m_logFile.is_open() )
                std::cerr << "Couldt open new Log File after rotating" << "\n";

            if( ec ){
                std::cerr << "Failed to Rename File: " << ec.message() << "\n";
                
                return false;
            }

            return true;

        }
        void buildDatName(char* buf){
            const char* datFormat = ".log\0";
            const char* LogPath_cstr = m_logPath.c_str();

            const char* endOfDatName = strrchr(LogPath_cstr, '.');
            endOfDatName++;
            size_t nameSize = static_cast<size_t> (endOfDatName - LogPath_cstr);

            std::strncpy(buf, LogPath_cstr, nameSize);
            buf[nameSize] = '\0';
            concatTime(buf);
            std::strcat(buf, datFormat);
        }    

        void concatTime(char* buf){
            auto now = std::chrono::system_clock::now();
            auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();

            std::time_t currTime = std::chrono::system_clock::to_time_t(now);
            char timeBuffer[std::size("yyyy-mm-ddThh:mm:ssZ")];
            std::strftime(timeBuffer, std::size(timeBuffer), "%FT%TZ", std::gmtime(&currTime));

            char nsBuff[16];
            std::snprintf(nsBuff, sizeof(nsBuff), "%ld", ns);

            std::strcat(buf, timeBuffer);
            std::strcat(buf, nsBuff);
        }
    private:
        std::string m_logPath;
        std::ofstream m_logFile;
    };




    class ILoggerCore {
    public:
        virtual ~ILoggerCore() = default;
        virtual void write( std::string&& logEntry ) = 0;
    };

    class AsyncLoggerCore : public ILoggerCore{
    public:
        AsyncLoggerCore( std::unique_ptr<IFileWriter> fileWriter ) : m_FileWriter(std::move(fileWriter)){ start(); }
        ~AsyncLoggerCore(){ 
            stop(); 

            m_FileWriter.reset(nullptr);
        }
    public:
        void write(std::string&& logEntry) override{
            std::lock_guard<std::mutex> __lock (m_queueMutex);
            m_MessageQueue.push(std::move(logEntry));
            m_cv.notify_one();
        }

        void start() { m_LogThread = std::thread ( [this](){ this->logThread(); } ); }
        void stop() {
            {
                std::lock_guard<std::mutex> __lock (m_queueMutex);
                m_running = false;
            }
            m_cv.notify_all();
            if( m_LogThread.joinable() )
                m_LogThread.join();
        }

    private:
        void logThread(){
            while( m_running || !m_MessageQueue.empty() ){
                std::unique_lock<std::mutex> __lock (m_queueMutex);
                m_cv.wait(__lock, [&]{
                    return !m_MessageQueue.empty() || !m_running;
                });

                while( !m_MessageQueue.empty() ){
                    std::string msg = std::move(m_MessageQueue.front());
                    m_MessageQueue.pop();

                    __lock.unlock();

                    m_FileWriter->writeFile(msg);

                    __lock.lock();
                }
            }
            m_FileWriter->flush();
        }
    private:
        std::unique_ptr<IFileWriter>m_FileWriter;

        std::mutex m_queueMutex;
        std::queue<std::string> m_MessageQueue;
        std::condition_variable m_cv;
        std::thread m_LogThread;
        std::atomic<bool> m_running {true};
    };




    class Logger{
        public:
        Logger(std::unique_ptr<ILoggerCore> core) : m_LogThread(std::move(core)){}
        ~Logger(){
            m_LogThread.reset(nullptr);
        }
        public:
        void setLogLevel(LogLevel level){ m_Loglevel = level; }

        LogLevel getLogLevel() const { return m_Loglevel; }

        template<typename ... Args> 
        void var_Log(LogLevel logLevel, SourceLocation location, Args&&... args){
            if( logLevel < m_Loglevel )
                return;

            char buff[32];            
            currentTimetoString(buff);

            std::string logEntry = createDeafultEntry(buff, logLevel, location);

            (addMessageToString(logEntry, std::forward<Args>(args)), ...);

            logEntry.append("\n");
            logEntry.shrink_to_fit();

            m_LogThread->write(std::move(logEntry));
        }

        void log(LogLevel logLevel, const std::string_view message, SourceLocation location) {
            if(logLevel < m_Loglevel)
                return;

            char buff[32];
            currentTimetoString(buff);

            std::string logEntry = createDeafultEntry(buff, logLevel, location);
            logEntry.append(message.data()).append("\n");
            logEntry.shrink_to_fit();

            m_LogThread->write(std::move(logEntry));
        }

        private:
        std::string levelToString(LogLevel logLevel) const {
            switch ( logLevel ){
                case DEBUG: return "DEBUG"; 
                case INFO: return "INFO";
                case ERROR: return "ERROR";
                case WARNING: return "WARNING";
                case CRITICAL: return "CRITICAL";
                default: return "NON TYPE";
            }
        }

        void currentTimetoString(char* dest){
            std::time_t logTime = std::time(nullptr);
            std::strncpy(dest, std::ctime(&logTime), 24);
            dest[24] = '\0';
        }

        std::string createDeafultEntry(const char* time, LogLevel logLevel, const SourceLocation& location){
            std::string logEntry;
            logEntry.reserve(256);
            logEntry.append("[").append(time).append("] ")
                    .append(levelToString(logLevel))
                    .append(": [").append(location.File).append(":").append(std::to_string(location.line)).append(" ").append(location.Function).append("]");
            return logEntry;
        }

        template<typename T>
        void addMessageToString(std::string& string, T&& message){ 
            appendToLog(string, message);
            string.append(" ");
        }

        void appendToLog(std::string& Log, const char* message){ Log.append(message); }
        void appendToLog(std::string& Log, std::string_view message) { Log.append(message); }
        void appendToLog(std::string& Log, const std::string& message) {Log.append(message); }
        template<typename T>
        std::enable_if_t<std::is_arithmetic_v<T>, void> 
            appendToLog(std::string& Log, const T& message){ Log.append(std::to_string(message)); }
        template<typename T>
        auto appendToLog(std::string& Log, const T& message) -> decltype(message.toLog(), void()) { Log.append(message.toLog()); }
        template<typename T>
        std::enable_if_t<std::is_constructible_v<std::string, T>, void>
            appendToLog(std::string& Log, const T& message){ Log.append(std::string(message)); }
        template<typename T> 
        std::enable_if_t<!is_logable_v<T>, void>
        appendToLog(std::string& Log, const T&  message) = delete;

        private:
        std::unique_ptr<ILoggerCore> m_LogThread;
        LogLevel m_Loglevel = LogLevel::INFO;
    };

    
    inline Logger* pInstance = nullptr; 

    inline void initLogger(std::unique_ptr<ILoggerCore> core){
        if( !pInstance )
            pInstance = new Logger(std::move(core)); 
    }

    inline void destroyLogger() {
        if( !pInstance ){
            return;
        }
        delete pInstance; 
        pInstance = nullptr; 
    }
}
