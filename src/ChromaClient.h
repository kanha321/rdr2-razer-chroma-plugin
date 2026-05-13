/*
 * ChromaClient.h — Razer Chroma REST API client declarations
 *
 * Manages the full Chroma session lifecycle:
 *   1. Initialize (POST register session)
 *   2. Heartbeat (PUT keep-alive every 1s)
 *   3. SetKeyboardColor (PUT static effect)
 *   4. Shutdown (DELETE session)
 *
 * Architecture follows RGB4R/Chroma.cs translated to C++ with WinHTTP.
 * Key patterns adopted:
 *   - Session URI stored after registration (RGB4R/Chroma.cs:L68)
 *   - Heartbeat with time guard (RGB4R/Chroma.cs:L76)
 *   - Graceful failure: reset session on HTTP error (RGB4R/Chroma.cs:L82-85)
 *   - API payloads match ChromaSDKImpl.js exactly
 */

#pragma once

#include <string>
#include <windows.h>
#include <winhttp.h>
#include "runtime/IChromaSession.h"

class ChromaClient : public IChromaSession
{
public:
    ChromaClient();
    ~ChromaClient();

    // Register a new Chroma session with Razer Synapse
    // POST http://localhost:54235/razer/chromasdk
    // Returns true if session was created successfully
    bool Initialize() override;

    // Send heartbeat to keep session alive
    // PUT {session_uri}/heartbeat
    // Only sends if enough time has elapsed (heartbeat interval guard)
    void Heartbeat() override;

    // Set entire keyboard to a static BGR color
    // PUT {session_uri}/keyboard
    // Body: {"effect":"CHROMA_STATIC","param":{"color": bgrColor}}
    bool SetKeyboardColor(int bgrColor) override;

    // Deregister the Chroma session
    // DELETE {session_uri}
    void Shutdown() override;

    // Check if a valid session exists
    bool IsReady() const override;

    // Send request to the session endpoint (used by ChromaRenderer)
    std::string SendSessionRequest(
        const std::string& subpath,
        const std::string& method,
        const std::string& body = ""
    ) override;
private:
    // Session state
    std::string m_sessionUri;     // Full URI returned by Chroma SDK (e.g., "http://localhost:54235/razer/chromasdk/12345")
    std::string m_sessionHost;    // Host extracted from session URI
    int         m_sessionPort;    // Port extracted from session URI
    std::string m_sessionPath;    // Path extracted from session URI

    // Timing
    DWORD m_lastHeartbeat;        // Tick count of last heartbeat
    DWORD m_lastInitAttempt;      // Tick count of last init attempt (for retry throttle)

    // WinHTTP session handle (reused across requests)
    HINTERNET m_hSession;

    // Parse a session URI into host/port/path components
    bool ParseSessionUri(const std::string& uri);

    // Generic HTTP request helper
    // Returns response body as string, empty on failure
    std::string SendHttpRequest(
        const std::string& host,
        int port,
        const std::string& path,
        const std::string& method,
        const std::string& body = ""
    );



    // Prevent copying
    ChromaClient(const ChromaClient&) = delete;
    ChromaClient& operator=(const ChromaClient&) = delete;
};
