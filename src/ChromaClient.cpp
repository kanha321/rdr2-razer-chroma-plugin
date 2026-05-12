/*
 * ChromaClient.cpp — WinHTTP-based Razer Chroma REST API client
 *
 * Full implementation of the Chroma session lifecycle using Windows
 * built-in WinHTTP library. No external HTTP dependencies.
 *
 * API flow follows the proven pattern from:
 *   - RGB4R/Chroma.cs — session management, heartbeat, graceful failure
 *   - ChromaSDKImpl.js — exact JSON payloads and endpoint paths
 *   - chroma-rest-sample — init/heartbeat/effect/uninit lifecycle
 *
 * All HTTP calls are synchronous (blocking) on the ScriptHook thread.
 * This is safe for localhost calls and matches RGB4R's approach.
 */

#include "ChromaClient.h"
#include "Config.h"
#include "Logger.h"

// nlohmann/json for JSON construction and parsing
#include "nlohmann/json.hpp"

#include <string>
#include <sstream>

using json = nlohmann::json;

// ============================================================================
// Constructor / Destructor
// ============================================================================

ChromaClient::ChromaClient()
    : m_sessionPort(0)
    , m_lastHeartbeat(0)
    , m_lastInitAttempt(0)
    , m_hSession(nullptr)
{
    LOG_INFO("ChromaClient: Creating WinHTTP session...");

    // Create a persistent WinHTTP session handle
    // This is reused for all HTTP requests (efficient, like a connection pool)
    m_hSession = WinHttpOpen(
        L"RDR2ChromaSync/1.0",
        WINHTTP_ACCESS_TYPE_NO_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0  // Synchronous mode
    );

    if (m_hSession)
    {
        LOG_INFO("ChromaClient: WinHTTP session created OK");

        // Set session-level timeouts (applies to all requests from this session)
        // This is CRITICAL — without this, a hanging connection freezes the game
        BOOL timeoutResult = WinHttpSetTimeouts(m_hSession, 2000, 2000, 2000, 2000);
        if (timeoutResult)
            LOG_INFO("ChromaClient: Session timeouts set (2s each)");
        else
            LOG_ERROR("ChromaClient: Failed to set session timeouts!");
    }
    else
    {
        DWORD err = GetLastError();
        LOG_ERROR("ChromaClient: WinHttpOpen FAILED, error=" + std::to_string(err));
    }
}

ChromaClient::~ChromaClient()
{
    Shutdown();

    if (m_hSession)
    {
        WinHttpCloseHandle(m_hSession);
        m_hSession = nullptr;
    }
}

// ============================================================================
// Public API
// ============================================================================

bool ChromaClient::Initialize()
{
    if (IsReady())
        return true;

    // Throttle retry attempts to avoid hammering the Chroma SDK
    DWORD now = GetTickCount();
    if (m_lastInitAttempt != 0 && (now - m_lastInitAttempt) < (DWORD)Config::CHROMA_INIT_RETRY_INTERVAL_MS)
        return false;

    m_lastInitAttempt = now;

    LOG_INFO("ChromaClient: Attempting initialization...");

    // Build registration payload
    json initPayload;
    try
    {
        initPayload = {
            {"title",            Config::APP_TITLE},
            {"description",      Config::APP_DESCRIPTION},
            {"author", {
                {"name",         Config::APP_AUTHOR_NAME},
                {"contact",      Config::APP_AUTHOR_URL}
            }},
            {"device_supported", json::array({"keyboard"})},
            {"category",         Config::APP_CATEGORY}
        };
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("ChromaClient: Failed to build JSON payload: " + std::string(e.what()));
        return false;
    }

    std::string body = initPayload.dump();
    LOG_DEBUG("ChromaClient: POST body: " + body);

    // POST to the Chroma SDK base endpoint
    LOG_INFO("ChromaClient: Sending POST to " + std::string(Config::CHROMA_BASE_URL) + ":" + std::to_string(Config::CHROMA_PORT) + Config::CHROMA_ENDPOINT);

    std::string response = SendHttpRequest(
        Config::CHROMA_BASE_URL,
        Config::CHROMA_PORT,
        Config::CHROMA_ENDPOINT,
        "POST",
        body
    );

    if (response.empty())
    {
        LOG_ERROR("ChromaClient: Init POST failed — empty response (Chroma SDK not running?)");
        return false;
    }

    LOG_INFO("ChromaClient: Got response: " + response);

    // Parse response to extract session URI
    try
    {
        json responseJson = json::parse(response);

        if (responseJson.contains("uri"))
        {
            m_sessionUri = responseJson["uri"].get<std::string>();
            LOG_INFO("ChromaClient: Session URI: " + m_sessionUri);

            if (!ParseSessionUri(m_sessionUri))
            {
                LOG_ERROR("ChromaClient: Failed to parse session URI!");
                m_sessionUri.clear();
                return false;
            }

            LOG_INFO("ChromaClient: Parsed — host=" + m_sessionHost + " port=" + std::to_string(m_sessionPort) + " path=" + m_sessionPath);

            m_lastHeartbeat = GetTickCount();
            LOG_INFO("ChromaClient: *** INITIALIZED SUCCESSFULLY ***");
            return true;
        }
        else
        {
            LOG_ERROR("ChromaClient: Response missing 'uri' field: " + response);
        }
    }
    catch (const json::exception& e)
    {
        LOG_ERROR("ChromaClient: JSON parse error: " + std::string(e.what()));
    }

    return false;
}

void ChromaClient::Heartbeat()
{
    if (!IsReady())
        return;

    // Time guard
    DWORD now = GetTickCount();
    if ((now - m_lastHeartbeat) < (DWORD)Config::HEARTBEAT_INTERVAL_MS)
        return;

    std::string response = SendSessionRequest("/heartbeat", "PUT");

    if (response.empty())
    {
        LOG_ERROR("ChromaClient: Heartbeat FAILED — resetting session");
        m_sessionUri.clear();
        m_sessionPath.clear();
        m_sessionHost.clear();
        m_sessionPort = 0;
    }

    m_lastHeartbeat = now;
}

bool ChromaClient::SetKeyboardColor(int bgrColor)
{
    if (!IsReady())
        return false;

    json effectPayload = {
        {"effect", "CHROMA_STATIC"},
        {"param", {
            {"color", bgrColor}
        }}
    };

    std::string body = effectPayload.dump();

    LOG_DEBUG("ChromaClient: SetKeyboardColor BGR=0x" + 
        ([](int v) { char buf[16]; sprintf_s(buf, "%06X", v); return std::string(buf); })(bgrColor));

    std::string response = SendSessionRequest("/keyboard", "PUT", body);

    if (response.empty())
    {
        LOG_ERROR("ChromaClient: SetKeyboardColor FAILED");
        return false;
    }

    return true;
}

void ChromaClient::Shutdown()
{
    if (!IsReady())
        return;

    LOG_INFO("ChromaClient: Shutting down session...");
    SendSessionRequest("", "DELETE");

    m_sessionUri.clear();
    m_sessionPath.clear();
    m_sessionHost.clear();
    m_sessionPort = 0;
    LOG_INFO("ChromaClient: Session closed");
}

bool ChromaClient::IsReady() const
{
    return !m_sessionUri.empty() && m_hSession != nullptr;
}

// ============================================================================
// URI Parsing
// ============================================================================

bool ChromaClient::ParseSessionUri(const std::string& uri)
{
    size_t schemeEnd = uri.find("://");
    if (schemeEnd == std::string::npos)
        return false;

    std::string rest = uri.substr(schemeEnd + 3);

    size_t pathStart = rest.find('/');
    std::string hostPort;
    std::string path;

    if (pathStart != std::string::npos)
    {
        hostPort = rest.substr(0, pathStart);
        path = rest.substr(pathStart);
    }
    else
    {
        hostPort = rest;
        path = "/";
    }

    size_t colonPos = hostPort.find(':');
    if (colonPos != std::string::npos)
    {
        m_sessionHost = hostPort.substr(0, colonPos);
        try
        {
            m_sessionPort = std::stoi(hostPort.substr(colonPos + 1));
        }
        catch (...)
        {
            LOG_ERROR("ChromaClient: Failed to parse port from: " + hostPort);
            return false;
        }
    }
    else
    {
        m_sessionHost = hostPort;
        m_sessionPort = 80;
    }

    m_sessionPath = path;
    return true;
}

// ============================================================================
// HTTP Helpers
// ============================================================================

std::string ChromaClient::SendHttpRequest(
    const std::string& host,
    int port,
    const std::string& path,
    const std::string& method,
    const std::string& body)
{
    if (!m_hSession)
    {
        LOG_ERROR("ChromaClient: SendHttpRequest — no WinHTTP session!");
        return "";
    }

    LOG_DEBUG("ChromaClient: HTTP " + method + " " + host + ":" + std::to_string(port) + path);

    // Convert host to wide string for WinHTTP
    std::wstring wHost(host.begin(), host.end());
    std::wstring wPath(path.begin(), path.end());
    std::wstring wMethod(method.begin(), method.end());

    // Connect to the server
    HINTERNET hConnect = WinHttpConnect(
        m_hSession,
        wHost.c_str(),
        static_cast<INTERNET_PORT>(port),
        0
    );

    if (!hConnect)
    {
        DWORD err = GetLastError();
        LOG_ERROR("ChromaClient: WinHttpConnect FAILED, error=" + std::to_string(err));
        return "";
    }

    // Create the request
    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect,
        wMethod.c_str(),
        wPath.c_str(),
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        0
    );

    if (!hRequest)
    {
        DWORD err = GetLastError();
        LOG_ERROR("ChromaClient: WinHttpOpenRequest FAILED, error=" + std::to_string(err));
        WinHttpCloseHandle(hConnect);
        return "";
    }

    // Set request-level timeouts (belt AND suspenders)
    WinHttpSetTimeouts(hRequest, 2000, 2000, 2000, 2000);

    // Send the request
    BOOL result;
    if (!body.empty())
    {
        LPCWSTR contentType = L"Content-Type: application/json\r\n";
        result = WinHttpSendRequest(
            hRequest,
            contentType,
            -1L,
            (LPVOID)body.c_str(),
            static_cast<DWORD>(body.length()),
            static_cast<DWORD>(body.length()),
            0
        );
    }
    else
    {
        result = WinHttpSendRequest(
            hRequest,
            WINHTTP_NO_ADDITIONAL_HEADERS,
            0,
            WINHTTP_NO_REQUEST_DATA,
            0,
            0,
            0
        );
    }

    if (!result)
    {
        DWORD err = GetLastError();
        LOG_ERROR("ChromaClient: WinHttpSendRequest FAILED, error=" + std::to_string(err));
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        return "";
    }

    // Wait for response
    result = WinHttpReceiveResponse(hRequest, NULL);
    if (!result)
    {
        DWORD err = GetLastError();
        LOG_ERROR("ChromaClient: WinHttpReceiveResponse FAILED, error=" + std::to_string(err));
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        return "";
    }

    // Check HTTP status code
    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    WinHttpQueryHeaders(
        hRequest,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX,
        &statusCode,
        &statusCodeSize,
        WINHTTP_NO_HEADER_INDEX
    );

    LOG_DEBUG("ChromaClient: HTTP response status=" + std::to_string(statusCode));

    // Read response body
    std::string responseBody;
    DWORD bytesAvailable = 0;
    DWORD bytesRead = 0;

    do
    {
        bytesAvailable = 0;
        WinHttpQueryDataAvailable(hRequest, &bytesAvailable);

        if (bytesAvailable > 0)
        {
            DWORD bufferSize = (bytesAvailable > 8192) ? 8192 : bytesAvailable;
            char* buffer = new char[bufferSize + 1];

            ZeroMemory(buffer, bufferSize + 1);
            WinHttpReadData(hRequest, buffer, bufferSize, &bytesRead);

            responseBody.append(buffer, bytesRead);
            delete[] buffer;
        }
    } while (bytesAvailable > 0);

    // Cleanup
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);

    if (statusCode >= 400)
    {
        LOG_ERROR("ChromaClient: HTTP error " + std::to_string(statusCode) + " body=" + responseBody);
        return "";
    }

    return responseBody;
}

std::string ChromaClient::SendSessionRequest(
    const std::string& subpath,
    const std::string& method,
    const std::string& body)
{
    if (!IsReady())
        return "";

    std::string fullPath = m_sessionPath + subpath;

    return SendHttpRequest(
        m_sessionHost,
        m_sessionPort,
        fullPath,
        method,
        body
    );
}
