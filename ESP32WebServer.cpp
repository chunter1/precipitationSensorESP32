#include <Arduino.h>
#include <libb64/cencode.h>
#include "WiFiServer.h"
#include "WiFiClient.h"
#include "ESP32WebServer.h"

class FunctionRequestHandler : public RequestHandler {
public:
  FunctionRequestHandler(ESP32WebServer::THandlerFunction fn, ESP32WebServer::THandlerFunction ufn, const char* uri, HTTPMethod method)
    : _fn(fn)
    , _ufn(ufn)
    , _uri(uri)
    , _method(method)
  {
  }

  bool canHandle(HTTPMethod requestMethod, String requestUri) override {
    if (_method != HTTP_ANY && _method != requestMethod)
      return false;

    if (requestUri != _uri)
      return false;

    return true;
  }

  bool canUpload(String requestUri) override {
    if (!_ufn || !canHandle(HTTP_POST, requestUri))
      return false;

    return true;
  }

  bool handle(ESP32WebServer& server, HTTPMethod requestMethod, String requestUri) override {
    if (!canHandle(requestMethod, requestUri))
      return false;

    _fn();
    return true;
  }

  void upload(ESP32WebServer& server, String requestUri, HTTPUpload& upload) override {
    if (canUpload(requestUri))
      _ufn();
  }

protected:
  ESP32WebServer::THandlerFunction _fn;
  ESP32WebServer::THandlerFunction _ufn;
  String _uri;
  HTTPMethod _method;
};


//#define DEBUG_ESP_HTTP_SERVER
#ifdef DEBUG_ESP_PORT
#define DEBUG_OUTPUT DEBUG_ESP_PORT
#else
#define DEBUG_OUTPUT Serial
#endif

const char * AUTHORIZATION_HEADER = "Authorization";

ESP32WebServer::ESP32WebServer(IPAddress addr, int port)
: _server(addr, port)
, _currentMethod(HTTP_ANY)
, _currentHandler(0)
, _firstHandler(0)
, _lastHandler(0)
, _currentArgCount(0)
, _currentArgs(0)
, _headerKeysCount(0)
, _currentHeaders(0)
, _contentLength(0)
{
}

ESP32WebServer::ESP32WebServer(int port)
: _server(port)
, _currentMethod(HTTP_ANY)
, _currentHandler(0)
, _firstHandler(0)
, _lastHandler(0)
, _currentArgCount(0)
, _currentArgs(0)
, _headerKeysCount(0)
, _currentHeaders(0)
, _contentLength(0)
{
}

ESP32WebServer::~ESP32WebServer() {
  if (_currentHeaders)
    delete[]_currentHeaders;
  _headerKeysCount = 0;
  RequestHandler* handler = _firstHandler;
  while (handler) {
    RequestHandler* next = handler->next();
    delete handler;
    handler = next;
  }
  close();
}

void ESP32WebServer::begin() {
  _currentStatus = HC_NONE;
  _server.begin();
  if(!_headerKeysCount)
    collectHeaders(0, 0);
}

bool ESP32WebServer::authenticate(const char * username, const char * password){
  if(hasHeader(AUTHORIZATION_HEADER)){
    String authReq = header(AUTHORIZATION_HEADER);
    if(authReq.startsWith("Basic")){
      authReq = authReq.substring(6);
      authReq.trim();
      char toencodeLen = strlen(username)+strlen(password)+1;
      char *toencode = new char[toencodeLen + 1];
      if(toencode == NULL){
        authReq = String();
        return false;
      }
      char *encoded = new char[base64_encode_expected_len(toencodeLen)+1];
      if(encoded == NULL){
        authReq = String();
        delete[] toencode;
        return false;
      }
      sprintf(toencode, "%s:%s", username, password);
      if(base64_encode_chars(toencode, toencodeLen, encoded) > 0 && authReq.equals(encoded)){
        authReq = String();
        delete[] toencode;
        delete[] encoded;
        return true;
      }
      delete[] toencode;
      delete[] encoded;
    }
    authReq = String();
  }
  return false;
}

void ESP32WebServer::requestAuthentication(){
  sendHeader("WWW-Authenticate", "Basic realm=\"Login Required\"");
  send(401);
}

void ESP32WebServer::on(const char* uri, ESP32WebServer::THandlerFunction handler) {
  on(uri, HTTP_ANY, handler);
}

void ESP32WebServer::on(const char* uri, HTTPMethod method, ESP32WebServer::THandlerFunction fn) {
  on(uri, method, fn, _fileUploadHandler);
}

void ESP32WebServer::on(const char* uri, HTTPMethod method, ESP32WebServer::THandlerFunction fn, ESP32WebServer::THandlerFunction ufn) {
  _addRequestHandler(new FunctionRequestHandler(fn, ufn, uri, method));
}

void ESP32WebServer::addHandler(RequestHandler* handler) {
    _addRequestHandler(handler);
}

void ESP32WebServer::_addRequestHandler(RequestHandler* handler) {
    if (!_lastHandler) {
      _firstHandler = handler;
      _lastHandler = handler;
    }
    else {
      _lastHandler->next(handler);
      _lastHandler = handler;
    }
}

void ESP32WebServer::handleClient() {
  if (_currentStatus == HC_NONE) {
    WiFiClient client = _server.available();
    if (!client) {
      return;
    }

    _currentClient = client;
    _currentStatus = HC_WAIT_READ;
    _statusChange = millis();
  }

  // Wait for data from client to become available
  if (_currentStatus == HC_WAIT_READ) {
    if (!_currentClient.available()) {
      if (millis() - _statusChange > HTTP_MAX_DATA_WAIT) {
        _currentClient = WiFiClient();
        _currentStatus = HC_NONE;
      }
      yield();
      return;
    }

    if (!_parseRequest(_currentClient)) {
      _currentClient = WiFiClient();
      _currentStatus = HC_NONE;
      return;
    }

    _contentLength = CONTENT_LENGTH_NOT_SET;
    _handleRequest();

    if (!_currentClient.connected()) {
      _currentClient = WiFiClient();
      _currentStatus = HC_NONE;
      return;
    } else {
      _currentStatus = HC_WAIT_CLOSE;
      _statusChange = millis();
      return;
    }
  }

  if (_currentStatus == HC_WAIT_CLOSE) {
    if (millis() - _statusChange > HTTP_MAX_CLOSE_WAIT) {
      _currentClient = WiFiClient();
      _currentStatus = HC_NONE;
    } else {
      yield();
      return;
    }
  }
}

void ESP32WebServer::close() {
  _server.end();
}

void ESP32WebServer::stop() {
  close();
}

void ESP32WebServer::sendHeader(const String& name, const String& value, bool first) {
  String headerLine = name;
  headerLine += ": ";
  headerLine += value;
  headerLine += "\r\n";

  if (first) {
    _responseHeaders = headerLine + _responseHeaders;
  }
  else {
    _responseHeaders += headerLine;
  }
}


void ESP32WebServer::_prepareHeader(String& response, int code, const char* content_type, size_t contentLength) {
    response = "HTTP/1.1 ";
    response += String(code);
    response += " ";
    response += _responseCodeToString(code);
    response += "\r\n";

    if (!content_type)
        content_type = "text/html";

    sendHeader("Content-Type", content_type, true);
    if (_contentLength == CONTENT_LENGTH_NOT_SET) {
        sendHeader("Content-Length", String(contentLength));
    } else if (_contentLength != CONTENT_LENGTH_UNKNOWN) {
        sendHeader("Content-Length", String(_contentLength));
    }
    sendHeader("Connection", "close");
    sendHeader("Access-Control-Allow-Origin", "*");

    response += _responseHeaders;
    response += "\r\n";
    _responseHeaders = String();
}

void ESP32WebServer::send(int code, const char* content_type, const String& content) {
    String header;
    _prepareHeader(header, code, content_type, content.length());
    sendContent(header);

    sendContent(content);
}

void ESP32WebServer::send_P(int code, PGM_P content_type, PGM_P content) {
    size_t contentLength = 0;

    if (content != NULL) {
        contentLength = strlen_P(content);
    }

    String header;
    char type[64];
    memccpy_P((void*)type, (PGM_VOID_P)content_type, 0, sizeof(type));
    _prepareHeader(header, code, (const char* )type, contentLength);
    sendContent(header);
    sendContent_P(content);
}

void ESP32WebServer::send_P(int code, PGM_P content_type, PGM_P content, size_t contentLength) {
    String header;
    char type[64];
    memccpy_P((void*)type, (PGM_VOID_P)content_type, 0, sizeof(type));
    _prepareHeader(header, code, (const char* )type, contentLength);
    sendContent(header);
    sendContent_P(content, contentLength);
}

void ESP32WebServer::send(int code, char* content_type, const String& content) {
  send(code, (const char*)content_type, content);
}

void ESP32WebServer::send(int code, const String& content_type, const String& content) {
  send(code, (const char*)content_type.c_str(), content);
}

void ESP32WebServer::sendContent(const String& content) {
  const size_t unit_size = HTTP_DOWNLOAD_UNIT_SIZE;
  size_t size_to_send = content.length();
  const char* send_start = content.c_str();

  while (size_to_send) {
    size_t will_send = (size_to_send < unit_size) ? size_to_send : unit_size;
    size_t sent = _currentClient.write(send_start, will_send);
    if (sent == 0) {
      break;
    }
    size_to_send -= sent;
    send_start += sent;
  }
}

void ESP32WebServer::sendContent_P(PGM_P content) {
    char contentUnit[HTTP_DOWNLOAD_UNIT_SIZE + 1];

    contentUnit[HTTP_DOWNLOAD_UNIT_SIZE] = '\0';

    while (content != NULL) {
        size_t contentUnitLen;
        PGM_P contentNext;

        // due to the memccpy signature, lots of casts are needed
        contentNext = (PGM_P)memccpy_P((void*)contentUnit, (PGM_VOID_P)content, 0, HTTP_DOWNLOAD_UNIT_SIZE);

        if (contentNext == NULL) {
            // no terminator, more data available
            content += HTTP_DOWNLOAD_UNIT_SIZE;
            contentUnitLen = HTTP_DOWNLOAD_UNIT_SIZE;
        }
        else {
            // reached terminator. Do not send the terminator
            contentUnitLen = contentNext - contentUnit - 1;
            content = NULL;
        }

        // write is so overloaded, had to use the cast to get it pick the right one
        _currentClient.write((const char*)contentUnit, contentUnitLen);
    }
}

void ESP32WebServer::sendContent_P(PGM_P content, size_t size) {
    char contentUnit[HTTP_DOWNLOAD_UNIT_SIZE + 1];
    contentUnit[HTTP_DOWNLOAD_UNIT_SIZE] = '\0';
    size_t remaining_size = size;

    while (content != NULL && remaining_size > 0) {
        size_t contentUnitLen = HTTP_DOWNLOAD_UNIT_SIZE;

        if (remaining_size < HTTP_DOWNLOAD_UNIT_SIZE) contentUnitLen = remaining_size;
        // due to the memcpy signature, lots of casts are needed
        memcpy_P((void*)contentUnit, (PGM_VOID_P)content, contentUnitLen);

        content += contentUnitLen;
        remaining_size -= contentUnitLen;

        // write is so overloaded, had to use the cast to get it pick the right one
        _currentClient.write((const char*)contentUnit, contentUnitLen);
    }
}


String ESP32WebServer::arg(String name) {
  for (int i = 0; i < _currentArgCount; ++i) {
    if ( _currentArgs[i].key == name )
      return _currentArgs[i].value;
  }
  return String();
}

String ESP32WebServer::arg(int i) {
  if (i < _currentArgCount)
    return _currentArgs[i].value;
  return String();
}

String ESP32WebServer::argName(int i) {
  if (i < _currentArgCount)
    return _currentArgs[i].key;
  return String();
}

int ESP32WebServer::args() {
  return _currentArgCount;
}

bool ESP32WebServer::hasArg(String  name) {
  for (int i = 0; i < _currentArgCount; ++i) {
    if (_currentArgs[i].key == name)
      return true;
  }
  return false;
}


String ESP32WebServer::header(String name) {
  for (int i = 0; i < _headerKeysCount; ++i) {
    if (_currentHeaders[i].key == name)
      return _currentHeaders[i].value;
  }
  return String();
}

void ESP32WebServer::collectHeaders(const char* headerKeys[], const size_t headerKeysCount) {
  _headerKeysCount = headerKeysCount + 1;
  if (_currentHeaders)
     delete[]_currentHeaders;
  _currentHeaders = new RequestArgument[_headerKeysCount];
  _currentHeaders[0].key = AUTHORIZATION_HEADER;
  for (int i = 1; i < _headerKeysCount; i++){
    _currentHeaders[i].key = headerKeys[i-1];
  }
}

String ESP32WebServer::header(int i) {
  if (i < _headerKeysCount)
    return _currentHeaders[i].value;
  return String();
}

String ESP32WebServer::headerName(int i) {
  if (i < _headerKeysCount)
    return _currentHeaders[i].key;
  return String();
}

int ESP32WebServer::headers() {
  return _headerKeysCount;
}

bool ESP32WebServer::hasHeader(String name) {
  for (int i = 0; i < _headerKeysCount; ++i) {
    if ((_currentHeaders[i].key == name) &&  (_currentHeaders[i].value.length() > 0))
      return true;
  }
  return false;
}

String ESP32WebServer::hostHeader() {
  return _hostHeader;
}

void ESP32WebServer::onFileUpload(THandlerFunction fn) {
  _fileUploadHandler = fn;
}

void ESP32WebServer::onNotFound(THandlerFunction fn) {
  _notFoundHandler = fn;
}

void ESP32WebServer::_handleRequest() {
  bool handled = false;
  if (!_currentHandler){
#ifdef DEBUG_ESP_HTTP_SERVER
    DEBUG_OUTPUT.println("request handler not found");
#endif
  }
  else {
    handled = _currentHandler->handle(*this, _currentMethod, _currentUri);
#ifdef DEBUG_ESP_HTTP_SERVER
    if (!handled) {
      DEBUG_OUTPUT.println("request handler failed to handle request");
    }
#endif
  }

  if (!handled) {
    if(_notFoundHandler) {
      _notFoundHandler();
    }
    else {
      send(404, "text/plain", String("Not found: ") + _currentUri);
    }
  }

  _currentUri = String();
}

String ESP32WebServer::_responseCodeToString(int code) {
  switch (code) {
    case 100: return F("Continue");
    case 101: return F("Switching Protocols");
    case 200: return F("OK");
    case 201: return F("Created");
    case 202: return F("Accepted");
    case 203: return F("Non-Authoritative Information");
    case 204: return F("No Content");
    case 205: return F("Reset Content");
    case 206: return F("Partial Content");
    case 300: return F("Multiple Choices");
    case 301: return F("Moved Permanently");
    case 302: return F("Found");
    case 303: return F("See Other");
    case 304: return F("Not Modified");
    case 305: return F("Use Proxy");
    case 307: return F("Temporary Redirect");
    case 400: return F("Bad Request");
    case 401: return F("Unauthorized");
    case 402: return F("Payment Required");
    case 403: return F("Forbidden");
    case 404: return F("Not Found");
    case 405: return F("Method Not Allowed");
    case 406: return F("Not Acceptable");
    case 407: return F("Proxy Authentication Required");
    case 408: return F("Request Time-out");
    case 409: return F("Conflict");
    case 410: return F("Gone");
    case 411: return F("Length Required");
    case 412: return F("Precondition Failed");
    case 413: return F("Request Entity Too Large");
    case 414: return F("Request-URI Too Large");
    case 415: return F("Unsupported Media Type");
    case 416: return F("Requested range not satisfiable");
    case 417: return F("Expectation Failed");
    case 500: return F("Internal Server Error");
    case 501: return F("Not Implemented");
    case 502: return F("Bad Gateway");
    case 503: return F("Service Unavailable");
    case 504: return F("Gateway Time-out");
    case 505: return F("HTTP Version not supported");
    default:  return "";
  }
}

static char* readBytesWithTimeout(WiFiClient& client, size_t maxLength, size_t& dataLength, int timeout_ms)
{
  char *buf = nullptr;
  dataLength = 0;
  while (dataLength < maxLength) {
    int tries = timeout_ms;
    size_t newLength;
    while (!(newLength = client.available()) && tries--) delay(1);
    if (!newLength) {
      break;
    }
    if (!buf) {
      buf = (char *)malloc(newLength + 1);
      if (!buf) {
        return nullptr;
      }
    }
    else {
      char* newBuf = (char *)realloc(buf, dataLength + newLength + 1);
      if (!newBuf) {
        free(buf);
        return nullptr;
      }
      buf = newBuf;
    }
    client.readBytes(buf + dataLength, newLength);
    dataLength += newLength;
    buf[dataLength] = '\0';
  }
  return buf;
}

bool ESP32WebServer::_parseRequest(WiFiClient& client) {
  // Read the first line of HTTP request
  String req = client.readStringUntil('\r');
  client.readStringUntil('\n');
  //reset header value
  for (int i = 0; i < _headerKeysCount; ++i) {
    _currentHeaders[i].value = String();
  }

  // First line of HTTP request looks like "GET /path HTTP/1.1"
  // Retrieve the "/path" part by finding the spaces
  int addr_start = req.indexOf(' ');
  int addr_end = req.indexOf(' ', addr_start + 1);
  if (addr_start == -1 || addr_end == -1) {
#ifdef DEBUG_ESP_HTTP_SERVER
    DEBUG_OUTPUT.print("Invalid request: ");
    DEBUG_OUTPUT.println(req);
#endif
    return false;
  }

  String methodStr = req.substring(0, addr_start);
  String url = req.substring(addr_start + 1, addr_end);
  String searchStr = "";
  int hasSearch = url.indexOf('?');
  if (hasSearch != -1) {
    searchStr = url.substring(hasSearch + 1);
    url = url.substring(0, hasSearch);
  }
  _currentUri = url;

  HTTPMethod method = HTTP_GET;
  if (methodStr == "POST") {
    method = HTTP_POST;
  }
  else if (methodStr == "DELETE") {
    method = HTTP_DELETE;
  }
  else if (methodStr == "OPTIONS") {
    method = HTTP_OPTIONS;
  }
  else if (methodStr == "PUT") {
    method = HTTP_PUT;
  }
  else if (methodStr == "PATCH") {
    method = HTTP_PATCH;
  }
  _currentMethod = method;

#ifdef DEBUG_ESP_HTTP_SERVER
  DEBUG_OUTPUT.print("method: ");
  DEBUG_OUTPUT.print(methodStr);
  DEBUG_OUTPUT.print(" url: ");
  DEBUG_OUTPUT.print(url);
  DEBUG_OUTPUT.print(" search: ");
  DEBUG_OUTPUT.println(searchStr);
#endif

  //attach handler
  RequestHandler* handler;
  for (handler = _firstHandler; handler; handler = handler->next()) {
    if (handler->canHandle(_currentMethod, _currentUri))
      break;
  }
  _currentHandler = handler;

  String formData;
  // below is needed only when POST type request
  if (method == HTTP_POST || method == HTTP_PUT || method == HTTP_PATCH || method == HTTP_DELETE) {
    String boundaryStr;
    String headerName;
    String headerValue;
    bool isForm = false;
    uint32_t contentLength = 0;
    //parse headers
    while (1) {
      req = client.readStringUntil('\r');
      client.readStringUntil('\n');
      if (req == "") break;//no moar headers
      int headerDiv = req.indexOf(':');
      if (headerDiv == -1) {
        break;
      }
      headerName = req.substring(0, headerDiv);
      headerValue = req.substring(headerDiv + 1);
      headerValue.trim();
      _collectHeader(headerName.c_str(), headerValue.c_str());

#ifdef DEBUG_ESP_HTTP_SERVER
      DEBUG_OUTPUT.print("headerName: ");
      DEBUG_OUTPUT.println(headerName);
      DEBUG_OUTPUT.print("headerValue: ");
      DEBUG_OUTPUT.println(headerValue);
#endif

      if (headerName == "Content-Type") {
        if (headerValue.startsWith("text/plain")) {
          isForm = false;
        }
        else if (headerValue.startsWith("multipart/form-data")) {
          boundaryStr = headerValue.substring(headerValue.indexOf('=') + 1);
          isForm = true;
        }
      }
      else if (headerName == "Content-Length") {
        contentLength = headerValue.toInt();
      }
      else if (headerName == "Host") {
        _hostHeader = headerValue;
      }
    }

    if (!isForm) {
      size_t plainLength;
      char* plainBuf = readBytesWithTimeout(client, contentLength, plainLength, HTTP_MAX_POST_WAIT);
      if (plainLength < contentLength) {
        free(plainBuf);
        return false;
      }
#ifdef DEBUG_ESP_HTTP_SERVER
      DEBUG_OUTPUT.print("Plain: ");
      DEBUG_OUTPUT.println(plainBuf);
#endif
      if (contentLength > 0) {
        if (searchStr != "") searchStr += '&';
        if (plainBuf[0] == '{' || plainBuf[0] == '[' || strstr(plainBuf, "=") == NULL) {
          //plain post json or other data
          searchStr += "plain=";
          searchStr += plainBuf;
        }
        else {
          searchStr += plainBuf;
        }
        free(plainBuf);
      }
    }
    _parseArguments(searchStr);
    if (isForm) {
      if (!_parseForm(client, boundaryStr, contentLength)) {
        return false;
      }
    }
  }
  else {
    String headerName;
    String headerValue;
    //parse headers
    while (1) {
      req = client.readStringUntil('\r');
      client.readStringUntil('\n');
      if (req == "") break;//no moar headers
      int headerDiv = req.indexOf(':');
      if (headerDiv == -1) {
        break;
      }
      headerName = req.substring(0, headerDiv);
      headerValue = req.substring(headerDiv + 2);
      _collectHeader(headerName.c_str(), headerValue.c_str());

#ifdef DEBUG_ESP_HTTP_SERVER
      DEBUG_OUTPUT.print("headerName: ");
      DEBUG_OUTPUT.println(headerName);
      DEBUG_OUTPUT.print("headerValue: ");
      DEBUG_OUTPUT.println(headerValue);
#endif

      if (headerName == "Host") {
        _hostHeader = headerValue;
      }
    }
    _parseArguments(searchStr);
  }
  client.flush();

#ifdef DEBUG_ESP_HTTP_SERVER
  DEBUG_OUTPUT.print("Request: ");
  DEBUG_OUTPUT.println(url);
  DEBUG_OUTPUT.print(" Arguments: ");
  DEBUG_OUTPUT.println(searchStr);
#endif

  return true;
}

bool ESP32WebServer::_collectHeader(const char* headerName, const char* headerValue) {
  for (int i = 0; i < _headerKeysCount; i++) {
    if (_currentHeaders[i].key == headerName) {
      _currentHeaders[i].value = headerValue;
      return true;
    }
  }
  return false;
}

void ESP32WebServer::_parseArguments(String data) {
#ifdef DEBUG_ESP_HTTP_SERVER
  DEBUG_OUTPUT.print("args: ");
  DEBUG_OUTPUT.println(data);
#endif
  if (_currentArgs)
    delete[] _currentArgs;
  _currentArgs = 0;
  if (data.length() == 0) {
    _currentArgCount = 0;
    return;
  }
  _currentArgCount = 1;

  for (int i = 0; i < (int)data.length(); ) {
    i = data.indexOf('&', i);
    if (i == -1)
      break;
    ++i;
    ++_currentArgCount;
  }
#ifdef DEBUG_ESP_HTTP_SERVER
  DEBUG_OUTPUT.print("args count: ");
  DEBUG_OUTPUT.println(_currentArgCount);
#endif

  _currentArgs = new RequestArgument[_currentArgCount];
  int pos = 0;
  int iarg;
  for (iarg = 0; iarg < _currentArgCount;) {
    int equal_sign_index = data.indexOf('=', pos);
    int next_arg_index = data.indexOf('&', pos);
#ifdef DEBUG_ESP_HTTP_SERVER
    DEBUG_OUTPUT.print("pos ");
    DEBUG_OUTPUT.print(pos);
    DEBUG_OUTPUT.print("=@ ");
    DEBUG_OUTPUT.print(equal_sign_index);
    DEBUG_OUTPUT.print(" &@ ");
    DEBUG_OUTPUT.println(next_arg_index);
#endif
    if ((equal_sign_index == -1) || ((equal_sign_index > next_arg_index) && (next_arg_index != -1))) {
#ifdef DEBUG_ESP_HTTP_SERVER
      DEBUG_OUTPUT.print("arg missing value: ");
      DEBUG_OUTPUT.println(iarg);
#endif
      if (next_arg_index == -1)
        break;
      pos = next_arg_index + 1;
      continue;
    }
    RequestArgument& arg = _currentArgs[iarg];
    arg.key = data.substring(pos, equal_sign_index);
    arg.value = urlDecode(data.substring(equal_sign_index + 1, next_arg_index));
#ifdef DEBUG_ESP_HTTP_SERVER
    DEBUG_OUTPUT.print("arg ");
    DEBUG_OUTPUT.print(iarg);
    DEBUG_OUTPUT.print(" key: ");
    DEBUG_OUTPUT.print(arg.key);
    DEBUG_OUTPUT.print(" value: ");
    DEBUG_OUTPUT.println(arg.value);
#endif
    ++iarg;
    if (next_arg_index == -1)
      break;
    pos = next_arg_index + 1;
  }
  _currentArgCount = iarg;
#ifdef DEBUG_ESP_HTTP_SERVER
  DEBUG_OUTPUT.print("args count: ");
  DEBUG_OUTPUT.println(_currentArgCount);
#endif

}

void ESP32WebServer::_uploadWriteByte(uint8_t b) {
  if (_currentUpload.currentSize == HTTP_UPLOAD_BUFLEN) {
    if (_currentHandler && _currentHandler->canUpload(_currentUri))
      _currentHandler->upload(*this, _currentUri, _currentUpload);
    _currentUpload.totalSize += _currentUpload.currentSize;
    _currentUpload.currentSize = 0;
  }
  _currentUpload.buf[_currentUpload.currentSize++] = b;
}

uint8_t ESP32WebServer::_uploadReadByte(WiFiClient& client) {
  int res = client.read();
  if (res == -1) {
    while (!client.available() && client.connected())
      yield();
    res = client.read();
  }
  return (uint8_t)res;
}

bool ESP32WebServer::_parseForm(WiFiClient& client, String boundary, uint32_t len) {

#ifdef DEBUG_ESP_HTTP_SERVER
  DEBUG_OUTPUT.print("Parse Form: Boundary: ");
  DEBUG_OUTPUT.print(boundary);
  DEBUG_OUTPUT.print(" Length: ");
  DEBUG_OUTPUT.println(len);
#endif
  String line;
  int retry = 0;
  do {
    line = client.readStringUntil('\r');
    ++retry;
  } while (line.length() == 0 && retry < 3);

  client.readStringUntil('\n');
  //start reading the form
  if (line == ("--" + boundary)) {
    RequestArgument* postArgs = new RequestArgument[32];
    int postArgsLen = 0;
    while (1) {
      String argName;
      String argValue;
      String argType;
      String argFilename;
      bool argIsFile = false;

      line = client.readStringUntil('\r');
      client.readStringUntil('\n');
      if (line.startsWith("Content-Disposition")) {
        int nameStart = line.indexOf('=');
        if (nameStart != -1) {
          argName = line.substring(nameStart + 2);
          nameStart = argName.indexOf('=');
          if (nameStart == -1) {
            argName = argName.substring(0, argName.length() - 1);
          }
          else {
            argFilename = argName.substring(nameStart + 2, argName.length() - 1);
            argName = argName.substring(0, argName.indexOf('"'));
            argIsFile = true;
#ifdef DEBUG_ESP_HTTP_SERVER
            DEBUG_OUTPUT.print("PostArg FileName: ");
            DEBUG_OUTPUT.println(argFilename);
#endif
            //use GET to set the filename if uploading using blob
            if (argFilename == "blob" && hasArg("filename")) argFilename = arg("filename");
          }
#ifdef DEBUG_ESP_HTTP_SERVER
          DEBUG_OUTPUT.print("PostArg Name: ");
          DEBUG_OUTPUT.println(argName);
#endif
          argType = "text/plain";
          line = client.readStringUntil('\r');
          client.readStringUntil('\n');
          if (line.startsWith("Content-Type")) {
            argType = line.substring(line.indexOf(':') + 2);
            //skip next line
            client.readStringUntil('\r');
            client.readStringUntil('\n');
          }
#ifdef DEBUG_ESP_HTTP_SERVER
          DEBUG_OUTPUT.print("PostArg Type: ");
          DEBUG_OUTPUT.println(argType);
#endif
          if (!argIsFile) {
            while (1) {
              line = client.readStringUntil('\r');
              client.readStringUntil('\n');
              if (line.startsWith("--" + boundary)) break;
              if (argValue.length() > 0) argValue += "\n";
              argValue += line;
            }
#ifdef DEBUG_ESP_HTTP_SERVER
            DEBUG_OUTPUT.print("PostArg Value: ");
            DEBUG_OUTPUT.println(argValue);
            DEBUG_OUTPUT.println();
#endif

            RequestArgument& arg = postArgs[postArgsLen++];
            arg.key = argName;
            arg.value = argValue;

            if (line == ("--" + boundary + "--")) {
#ifdef DEBUG_ESP_HTTP_SERVER
              DEBUG_OUTPUT.println("Done Parsing POST");
#endif
              break;
            }
          }
          else {
            _currentUpload.status = UPLOAD_FILE_START;
            _currentUpload.name = argName;
            _currentUpload.filename = argFilename;
            _currentUpload.type = argType;
            _currentUpload.totalSize = 0;
            _currentUpload.currentSize = 0;
#ifdef DEBUG_ESP_HTTP_SERVER
            DEBUG_OUTPUT.print("Start File: ");
            DEBUG_OUTPUT.print(_currentUpload.filename);
            DEBUG_OUTPUT.print(" Type: ");
            DEBUG_OUTPUT.println(_currentUpload.type);
#endif
            if (_currentHandler && _currentHandler->canUpload(_currentUri))
              _currentHandler->upload(*this, _currentUri, _currentUpload);
            _currentUpload.status = UPLOAD_FILE_WRITE;
            uint8_t argByte = _uploadReadByte(client);
          readfile:
            while (argByte != 0x0D) {
              if (!client.connected()) return _parseFormUploadAborted();
              _uploadWriteByte(argByte);
              argByte = _uploadReadByte(client);
            }

            argByte = _uploadReadByte(client);
            if (!client.connected()) return _parseFormUploadAborted();
            if (argByte == 0x0A) {
              argByte = _uploadReadByte(client);
              if (!client.connected()) return _parseFormUploadAborted();
              if ((char)argByte != '-') {
                //continue reading the file
                _uploadWriteByte(0x0D);
                _uploadWriteByte(0x0A);
                goto readfile;
              }
              else {
                argByte = _uploadReadByte(client);
                if (!client.connected()) return _parseFormUploadAborted();
                if ((char)argByte != '-') {
                  //continue reading the file
                  _uploadWriteByte(0x0D);
                  _uploadWriteByte(0x0A);
                  _uploadWriteByte((uint8_t)('-'));
                  goto readfile;
                }
              }

              uint8_t endBuf[boundary.length()];
              client.readBytes(endBuf, boundary.length());

              if (strstr((const char*)endBuf, boundary.c_str()) != NULL) {
                if (_currentHandler && _currentHandler->canUpload(_currentUri))
                  _currentHandler->upload(*this, _currentUri, _currentUpload);
                _currentUpload.totalSize += _currentUpload.currentSize;
                _currentUpload.status = UPLOAD_FILE_END;
                if (_currentHandler && _currentHandler->canUpload(_currentUri))
                  _currentHandler->upload(*this, _currentUri, _currentUpload);
#ifdef DEBUG_ESP_HTTP_SERVER
                DEBUG_OUTPUT.print("End File: ");
                DEBUG_OUTPUT.print(_currentUpload.filename);
                DEBUG_OUTPUT.print(" Type: ");
                DEBUG_OUTPUT.print(_currentUpload.type);
                DEBUG_OUTPUT.print(" Size: ");
                DEBUG_OUTPUT.println(_currentUpload.totalSize);
#endif
                line = client.readStringUntil(0x0D);
                client.readStringUntil(0x0A);
                if (line == "--") {
#ifdef DEBUG_ESP_HTTP_SERVER
                  DEBUG_OUTPUT.println("Done Parsing POST");
#endif
                  break;
                }
                continue;
              }
              else {
                _uploadWriteByte(0x0D);
                _uploadWriteByte(0x0A);
                _uploadWriteByte((uint8_t)('-'));
                _uploadWriteByte((uint8_t)('-'));
                uint32_t i = 0;
                while (i < boundary.length()) {
                  _uploadWriteByte(endBuf[i++]);
                }
                argByte = _uploadReadByte(client);
                goto readfile;
              }
            }
            else {
              _uploadWriteByte(0x0D);
              goto readfile;
            }
            break;
          }
        }
      }
    }

    int iarg;
    int totalArgs = ((32 - postArgsLen) < _currentArgCount) ? (32 - postArgsLen) : _currentArgCount;
    for (iarg = 0; iarg < totalArgs; iarg++) {
      RequestArgument& arg = postArgs[postArgsLen++];
      arg.key = _currentArgs[iarg].key;
      arg.value = _currentArgs[iarg].value;
    }
    if (_currentArgs) delete[] _currentArgs;
    _currentArgs = new RequestArgument[postArgsLen];
    for (iarg = 0; iarg < postArgsLen; iarg++) {
      RequestArgument& arg = _currentArgs[iarg];
      arg.key = postArgs[iarg].key;
      arg.value = postArgs[iarg].value;
    }
    _currentArgCount = iarg;
    if (postArgs) delete[] postArgs;
    return true;
  }
#ifdef DEBUG_ESP_HTTP_SERVER
  DEBUG_OUTPUT.print("Error: line: ");
  DEBUG_OUTPUT.println(line);
#endif
  return false;
}

String ESP32WebServer::urlDecode(const String& text)
{
  String decoded = "";
  char temp[] = "0x00";
  unsigned int len = text.length();
  unsigned int i = 0;
  while (i < len)
  {
    char decodedChar;
    char encodedChar = text.charAt(i++);
    if ((encodedChar == '%') && (i + 1 < len))
    {
      temp[2] = text.charAt(i++);
      temp[3] = text.charAt(i++);

      decodedChar = strtol(temp, NULL, 16);
    }
    else {
      if (encodedChar == '+')
      {
        decodedChar = ' ';
      }
      else {
        decodedChar = encodedChar;  // normal ascii char
      }
    }
    decoded += decodedChar;
  }
  return decoded;
}

bool ESP32WebServer::_parseFormUploadAborted() {
  _currentUpload.status = UPLOAD_FILE_ABORTED;
  if (_currentHandler && _currentHandler->canUpload(_currentUri))
    _currentHandler->upload(*this, _currentUri, _currentUpload);
  return false;
}
