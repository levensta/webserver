#include "Request.hpp"

#include <iostream>

namespace HTTP {

Request::Request() : _parseFlags(PARSED_NONE) {}

Request::~Request() {}

const std::string &Request::getMethod() const {
    return _method;
}

const std::string &Request::getPath() const {
    return _path;
}

const std::string &Request::getProtocol() const {
    return _protocol;
}

const std::map<uint32, Header> &Request::getHeaders() const {
    return _headers;
}

const std::string &Request::getBody() const {
    return _body;
}

void Request::parseLine(std::string line) {
    HTTP::StatusCode status;

    if (!(getFlags() & PARSED_SL)) {
        if ((status = parseStartLine(line)) != HTTP::CONTINUE) {
            Log.error("Request::parseLine, parsing SL");
            // return HTTP::Response(status);
        }
    } else if (!(getFlags() & PARSED_HEADERS)) {
        if ((status = parseHeader(line)) != HTTP::CONTINUE) {
            Log.error("Request::parseLine, parsing Headers");
            // return HTTP::Response(status);
        }
    } else if (!(getFlags() & PARSED_BODY)) {
        Log.error("Request::parseLine, parsing Body");
    } else if (getFlags() & PARSED_BODY) {
        return;
    }
}

StatusCode Request::parseStartLine(const std::string &line) {
    size_t pos = 0;

    _method = getWord(line, ' ', pos);
    if (!isValidMethod(_method)) {
        Log.debug("Method is not implemented");
        return NOT_IMPLEMENTED;
    }

    skipSpaces(line, pos);
    _path = getWord(line, ' ', pos);
    if (!isValidPath(_path)) {
        Log.debug("Invalid URI");
        return BAD_REQUEST;
    }

    skipSpaces(line, pos);
    _protocol = getWord(line, ' ', pos);

    skipSpaces(line, pos);
    if (line[pos]) {
        std::cout << "|" << (int)line[pos] << "|" << std::endl;
        Log.debug("Forbidden symbols at the end of the line");
        return BAD_REQUEST;
    }
    if (!isValidProtocol(_protocol)) {
        Log.debug("Protocol is not supported or invalid");
        // Or 505, need to improve
        return BAD_REQUEST;
    }

    setFlag(PARSED_SL);
    return CONTINUE;
}

StatusCode Request::isValidMethod(const std::string &method) {
    std::string validMethods[9] = {
        "GET", "DELETE", "POST",
        "PUT", "HEAD", "CONNECT",
        "OPTIONS", "TRACE", "PATCH"};
    for (int i = 0; i < 9; ++i) {
        if (validMethods[i] == method)
            return CONTINUE;
    }
    return NOT_IMPLEMENTED;
}

StatusCode Request::isValidPath(const std::string &path) {
    if (path[0] != '/') {
        return BAD_REQUEST;
    }
    return CONTINUE;
}

StatusCode Request::isValidProtocol(const std::string &protocol) {
    char protocol_valid[] = "HTTP/1.1";
    if (protocol_valid == protocol)
        return CONTINUE;
    return HTTP_VERSION_NOT_SUPPORTED;
}

void Request::setFlag(uint8 flag) {
    _parseFlags |= flag;
}

void Request::removeFlag(uint8 flag) {
    _parseFlags &= ~flag;
}

const uint8 &Request::getFlags() const {
    return _parseFlags;
}

StatusCode Request::parseHeader(std::string line) {
    if (line == "") {
        setFlag(PARSED_HEADERS);
        return CONTINUE;
    }

    Header header;
    // header.line.swap(line);
    header.line = line;

    size_t sepPos = line.find(':');
    header.line[sepPos] = '\0';
    header.keyLen = sepPos;
    //
    // Some errors skipped with this method... (Maybe should be rewritten with nginx parser)
    header.valStart = line.find_first_not_of(" \t\n\r", sepPos);
    size_t valEnd = line.find_last_not_of(" \t\n\r", sepPos);
    header.valLen = valEnd - header.valStart;

    toLowerCase(header.line);

    header.hash = crc(header.line.data(), header.keyLen);
    if (validHeaders.find(header.hash) == validHeaders.end()) {
        Log.debug("Maybe header is not supported");
        Log.debug(header.line.data() + std::string("    |    ") + to_string(header.hash));
        std::cout << header.line.data() + std::string("    |    ") + to_string(header.hash) << std::endl;
        return BAD_REQUEST;
    }

    // Copying here need to replace
    _headers.insert(std::make_pair(header.hash, header));

    return CONTINUE;
}
}; // namespace HTTP