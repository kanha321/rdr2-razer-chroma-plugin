/*
 * IChromaSession.h — Chroma session abstraction
 */

#pragma once

#include <string>

class IChromaSession
{
public:
    virtual ~IChromaSession() = default;
    virtual bool Initialize() = 0;
    virtual void Heartbeat() = 0;
    virtual bool SetKeyboardColor(int bgrColor) = 0;
    virtual void Shutdown() = 0;
    virtual bool IsReady() const = 0;
    virtual std::string SendSessionRequest(
        const std::string& subpath,
        const std::string& method,
        const std::string& body = ""
    ) = 0;
};
