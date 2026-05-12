/*
 * Logger.h — Simple file logger for debugging
 *
 * Writes timestamped log messages to RDR2ChromaSync.log
 * in the game directory (next to the .asi file).
 */

#pragma once

#include <windows.h>
#include <fstream>
#include <string>
#include <ctime>
#include <sstream>
#include <mutex>

class Logger
{
public:
    static Logger& Instance()
    {
        static Logger instance;
        return instance;
    }

    void Init(HMODULE hModule)
    {
        // Get the directory where the .asi file lives
        char path[MAX_PATH];
        GetModuleFileNameA(hModule, path, MAX_PATH);

        std::string dllPath(path);
        std::string dir = dllPath.substr(0, dllPath.find_last_of("\\/"));
        m_logPath = dir + "\\RDR2ChromaSync.log";

        // Open log file (overwrite on each game launch)
        m_file.open(m_logPath, std::ios::out | std::ios::trunc);
        if (m_file.is_open())
        {
            Write("INFO", "=== RDR2ChromaSync v1.0 started ===");
            Write("INFO", "Log file: " + m_logPath);
        }
    }

    void Info(const std::string& msg)    { Write("INFO", msg); }
    void Error(const std::string& msg)   { Write("ERROR", msg); }
    void Debug(const std::string& msg)   { Write("DEBUG", msg); }

    void Close()
    {
        if (m_file.is_open())
        {
            Write("INFO", "=== RDR2ChromaSync shutting down ===");
            m_file.close();
        }
    }

private:
    Logger() {}

    void Write(const char* level, const std::string& msg)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_file.is_open()) return;

        // Timestamp
        SYSTEMTIME st;
        GetLocalTime(&st);

        char timestamp[64];
        sprintf_s(timestamp, "%02d:%02d:%02d.%03d",
            st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

        m_file << "[" << timestamp << "] [" << level << "] " << msg << std::endl;
        m_file.flush();  // Flush immediately so we see logs even on crash
    }

    std::ofstream m_file;
    std::string m_logPath;
    std::mutex m_mutex;
};

// Convenience macros
#define LOG_INFO(msg)  Logger::Instance().Info(msg)
#define LOG_ERROR(msg) Logger::Instance().Error(msg)
#define LOG_DEBUG(msg) Logger::Instance().Debug(msg)
