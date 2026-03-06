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

#define CURRENT_LOCATION_LOG ::Log::SourceLocation{__FILE__, __func__, __LINE__}

#define CREATE_LOGGER(file) Log::initLogger(file)
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
#define LOG_VINFO(...) Log::pInstance->var_log(Log::LogLevel::INFO, CURRENT_LOCATION_LOG, __VA_ARGS__)
#define LOG_WARNING(msg) Log::pInstance->log(Log::LogLevel::WARNING, msg, CURRENT_LOCATION_LOG)
#define LOG_VWARNING(...) Log::pInstance->var_log(Log::LogLevel::WARNING, CURRENT_LOCATION_LOG, __VA_ARGS__)
#define LOG_ERROR(msg) Log::pInstance->log(Log::LogLevel::ERROR, msg, CURRENT_LOCATION_LOG)
#define LOG_VERROR(...) Log::pInstance->var_log(Log::LogLevel::VERROR, CURRENT_LOCATION_LOG, __VA_ARGS__)
#define LOG_CRITICAL(msg) Log::pInstance->log(Log::LogLevel::CRITICAL, msg, CURRENT_LOCATION_LOG)
#define LOG_VCRITICAL(...) Log::pInstance->var_log(Log::LogLevel::CRITICAL, CURRENT_LOCATION_LOG, __VA_ARGS__)



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

//also trennen von thread logik und normaler logik
//veänderung bon dem to log ding keine ahnung
namespace Log{
    template<typename T, typename = void>
    struct has_toLog : std::false_type {};

    template<typename T>
    struct has_toLog<T, std::void_t<decltype(std::declval<const T&>().toLog())>> : std::true_type {};

    template<typename T>
    inline constexpr bool has_toLog_v = has_toLog<T>::value;

    template<typename T>
    inline constexpr bool is_logable_v = std::is_arithmetic_v<T> || std::is_convertible_v<T, std::string> || has_toLog_v<T>;

    enum LogLevel{
        DEBUG = 0, INFO, WARNING, ERROR, CRITICAL
    };


    struct SourceLocation {
        const char* File;
        const char* Function;
        int line;
    };

    inline std::ostream& operator<<(std::ostream& os, const SourceLocation& location){ return os << location.File << ":" << location.line << " " << location.Function; }

    class LoggerThread {
    public:
        LoggerThread(const std::string& file){
            m_logFile.open(file, std::ios::app);
            if(!m_logFile.is_open())
                std::cerr << "Failed to open Log file" << "\n";

            m_LogThread = std::thread ( [this](){ this->logThread(); } );
        }
        ~LoggerThread(){
            m_running = false;
            m_cv.notify_all();
            if( m_LogThread.joinable() )
                m_LogThread.join();

            if(m_logFile.is_open())
                m_logFile.close();
        }
    public:
        const std::string& getLogPath() const { return m_logPath; }

        void addToMessageQueue(std::string&& logEntry){
            std::lock_guard<std::mutex> __lock (m_queueMutex);
            m_MessageQueue.push(std::move(logEntry));
            m_cv.notify_one();
        }

    private:
        void logThread(){
            size_t logFileSize = std::filesystem::file_size(m_logPath);

            while( m_running || !m_MessageQueue.empty() ){
                std::unique_lock<std::mutex> __lock (m_queueMutex);
                m_cv.wait(__lock, [&]{
                    return !m_MessageQueue.empty() || !m_running;
                });

                while( !m_MessageQueue.empty() ){
                    std::string& message = m_MessageQueue.front();

                    if( logFileSize + message.size() > MAXLOGSIZE ){
                        m_logFile.flush();
                        if( changeLogFileIfNeeded() )
                            logFileSize = 0;
                    }

                    logFileSize+=message.size();

                    m_logFile << message;
                    m_MessageQueue.pop();
                }
            }
            m_logFile.flush();
        }

        bool changeLogFileIfNeeded(){

            if( !std::filesystem::exists(m_logPath) )
                return false;

            if( m_logFile.is_open() )
                m_logFile.close();

            auto now = std::chrono::system_clock::now();
            auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();

            char nsBuff[16];
            std::snprintf(nsBuff, sizeof(nsBuff), "%ld", ns);

            std::time_t currTime = std::chrono::system_clock::to_time_t(now);
            char timeBuffer[std::size("yyyy-mm-ddThh:mm:ssZ")];
            std::strftime(timeBuffer, std::size(timeBuffer), "%FT%TZ", std::gmtime(&currTime));
            
            const char* datFormat = ".log\0";
            const char* LogPath_cstr = m_logPath.c_str();

            const char* endOfDatName = strrchr(LogPath_cstr, '.');
            endOfDatName++;
            size_t nameSize = static_cast<size_t> (endOfDatName - LogPath_cstr);

            char newLogName[nameSize + strlen(timeBuffer) + strlen(nsBuff) + strlen(datFormat) + 1]; 

            std::strncpy(newLogName, LogPath_cstr, nameSize);
            newLogName[nameSize] = '\0';
            std::strcat(newLogName, timeBuffer);
            std::strcat(newLogName, nsBuff);
            std::strcat(newLogName, datFormat);

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

    private:
        std::ofstream m_logFile;
        std::string m_logPath;

        std::mutex m_queueMutex;
        std::queue<std::string> m_MessageQueue;
        std::condition_variable m_cv;
        std::thread m_LogThread;
        std::atomic<bool> m_running {true};

    };

    class Logger{
        public:
        Logger(const std::string& file) : m_LogThread(std::make_unique<LoggerThread>(file)){}
        ~Logger(){
            m_LogThread.reset(nullptr);
        }
        public:
        void setLogLevel(LogLevel level){ m_Loglevel = level; }

        const std::string& getLogPath() const { return m_LogThread->getLogPath(); }
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

            m_LogThread->addToMessageQueue(std::move(logEntry));
        }

        void log(LogLevel logLevel, const std::string_view message, SourceLocation location) {
            if(logLevel < m_Loglevel)
                return;

            char buff[32];
            currentTimetoString(buff);

            std::string logEntry = createDeafultEntry(buff, logLevel, location);
            logEntry.append(message.data()).append("\n");
            logEntry.shrink_to_fit();

            m_LogThread->addToMessageQueue(std::move(logEntry));
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
        std::enable_if_t<std::is_convertible_v<T, std::string>, void>
            appendToLog(std::string& Log, const T& message){ Log.append(std::string(message)); }
        template<typename T> 
        std::enable_if_t<!is_logable_v<T>, void>
        appendToLog(std::string& Log, const T&  message) = delete;

        private:
        std::unique_ptr<LoggerThread> m_LogThread;
        LogLevel m_Loglevel = LogLevel::INFO;
    };

    
    inline Logger* pInstance = nullptr; 

    inline void initLogger(const std::string& file){
        if( !pInstance )
            pInstance = new Logger(file); 
    }

    inline void destroyLogger() {
        if( !pInstance ){
            return;
        }
        delete pInstance; 
        pInstance = nullptr; 
    }
}
