#include "Response.hpp"
#include "CGI.hpp"
#include "Client.hpp"

namespace HTTP {

Response::Response() 
            : _req(NULL)
            , _client(NULL)
            , _cgi(NULL)
            , _bodyLength(0)
            , _isFormed(false) {}

Response::~Response() { }

void
Response::initMethodsHeaders(void) {
    methods.insert(std::make_pair("GET", &Response::GET));
    methods.insert(std::make_pair("PUT", &Response::PUT));
    methods.insert(std::make_pair("POST", &Response::POST));
    methods.insert(std::make_pair("HEAD", &Response::HEAD));
    methods.insert(std::make_pair("DELETE", &Response::DELETE));
    methods.insert(std::make_pair("OPTIONS", &Response::OPTIONS));

    headers.push_back(ResponseHeader(DATE));
    headers.push_back(ResponseHeader(SERVER));
    headers.push_back(ResponseHeader(KEEP_ALIVE));
    headers.push_back(ResponseHeader(CONNECTION));
    headers.push_back(ResponseHeader(CONTENT_TYPE));
    headers.push_back(ResponseHeader(CONTENT_LENGTH));
}

void
Response::clear() {

    _res = "";
    _body = "";
    _bodyLength = 0;
    _isFormed = false;
    _cgi = NULL;

    _resourceFileStream.clear();
    iter rm = headers.begin();
    std::advance(rm, 6);
    headers.erase(rm, headers.end());
    for (iter it = headers.begin(); it != headers.end(); ++it) {
        it->value = "";
    }
}

void
Response::handle(void) {
    std::map<std::string, Response::Handler>::iterator it;

    if (getStatus() >= BAD_REQUEST) {
        this->setErrorResponse(getStatus());
    } else if (_req->authNeeded() && !_req->isAuthorized()) {
        this->unauthorized();
    } else {
        it = methods.find(_req->getMethod());
        (this->*(it->second))();
    }

    _res = getStatusLine() + makeHeaders() + getBody();
}

void
Response::unauthorized(void) {
    Log.debug("Response:: Unauthorized");
    setStatus(UNAUTHORIZED);
    addHeader(DATE, getDateTimeGMT());
    addHeader(WWW_AUTHENTICATE);
    _client->shouldBeClosed(true);
}

void
Response::DELETE(void) {
    std::string resourcePath = _req->getResolvedPath();

    if (!resourceExists(resourcePath)) {
        setErrorResponse(NOT_FOUND);
        return;
    } else if (isDirectory(resourcePath)) {
        if (rmdirNonEmpty(resourcePath)) {
            setErrorResponse(FORBIDDEN);
            return;
        }
    } else {
        if (remove(resourcePath.c_str())) {
            setErrorResponse(FORBIDDEN);
            return;
        }
    }
    setStatus(OK);
    setBody("<html><body>"
            " <h1>File deleted.</h1>"
            "</body></html>");
}

void
Response::HEAD(void) {
    if (!contentForGetHead()) {
        setErrorResponse(getStatus());
    } else {
        _body = "";
    }
}

void
Response::GET(void) {
    if (!contentForGetHead()) {
        setErrorResponse(getStatus());
    }
}

void
Response::OPTIONS(void) {
    addHeader(ALLOW);
}

void
Response::POST(void) {
    std::map<std::string, CGI>::iterator it;
    it = isCGI(_req->getResolvedPath(), _req->getLocation()->getCGIsRef());
    if (it != _req->getLocation()->getCGIsRef().end()) {
        passToCGI(it->second);
    } else {
        setStatus(NO_CONTENT);
    }
}

void
Response::PUT(void) {
    std::string resourcePath = _req->getResolvedPath();
    if (isDirectory(resourcePath)) {
        setErrorResponse(FORBIDDEN);
        return ;
    } else if (isFile(resourcePath)) {
        if (isWritableFile(resourcePath)) {
            writeFile(resourcePath);
            setBody("<html>"
                    "<body>"
                    " <h1>File is overwritten.</h1>"
                    "</body>"
                    "</html>");
        } else {
            setErrorResponse(FORBIDDEN);
            return ;
        }
    } else {
        writeFile(resourcePath);
        setBody("<html>"
                "<body>"
                " <h1>File created.</h1>"
                "</body>"
                "</html>");
    }
}

int
Response::contentForGetHead(void) {
    std::string resourcePath = _req->getResolvedPath();

    if (!resourceExists(resourcePath)) {
        setStatus(NOT_FOUND);
        return 0;
    }

    if (isDirectory(resourcePath)) {
        if (redirectForDirectory(resourcePath)) {
            return 1;
        } else if (isSetIndexFile(resourcePath)) {
            return makeGetHeadResponseForFile(resourcePath);
        } else {
            return directoryListing(resourcePath);
        }
    } else if (isFile(resourcePath)) {
        return makeGetHeadResponseForFile(resourcePath);
    } else {
        // not readable files and other types
        setStatus(FORBIDDEN);
        return 0;
    }
}

int
Response::redirectForDirectory(const std::string &resourcePath) {
    if (resourcePath[resourcePath.length() - 1] != '/') {
        setStatus(MOVED_PERMANENTLY);
        addHeader(LOCATION, _req->getRawUri() + "/");
        addHeader(CONTENT_TYPE, "text/html");
        setBody("<html><body>"
                " <h1>Redirect 301</h1>"
                "</body></html>");
        return 1;
    }
    return 0;
}

bool
Response::isSetIndexFile(std::string &resourcePath) {
    const std::vector<std::string> &indexes = _req->getLocation()->getIndexRef();
    for (size_t i = 0; i < indexes.size(); ++i) {
        std::string path = resourcePath + indexes[i];
        if (isFile(path)) {
            resourcePath = path;
            return true;
        }
    }
    return false;
}

int
Response::makeGetHeadResponseForFile(const std::string &resourcePath) {
    std::map<std::string, CGI>::iterator it;
    it = isCGI(resourcePath, _req->getLocation()->getCGIsRef());
    if (it != _req->getLocation()->getCGIsRef().end()) {
        return passToCGI(it->second);
    }
    addHeader(CONTENT_TYPE, getContentType(resourcePath));
    addHeader(ETAG, getEtagFile(resourcePath));
    addHeader(LAST_MODIFIED, getLastModifiedTimeGMT(resourcePath));
    addHeader(TRANSFER_ENCODING, "chunked");
    return openFileToResponse(resourcePath);
}

int
Response::directoryListing(const std::string &resourcePath) {
    // autoindex on
    if (_req->getLocation()->getAutoindexRef() == true && isDirectory(resourcePath)) {
        addHeader(CONTENT_TYPE, "text/html; charset=utf-8");
        return listing(resourcePath);
    } else {
        // autoindex off
        setStatus(FORBIDDEN);
        return 0;
    }
}

int
Response::openFileToResponse(std::string resourcePath) {
    _resourceFileStream.open(resourcePath.c_str(), std::ifstream::in | std::ios_base::binary);
    if (!_resourceFileStream.is_open()) {
        setStatus(FORBIDDEN);
        return 0;
    }

    return 1;
}

int
Response::listing(const std::string &resourcePath) {
    std::string pathToDir;
    _body = "<!DOCTYPE html>"
            "<html>"
            "<head>"
            "<meta charset=\"UTF-8\">"
            "<title>" + _req->getPath() + "</title>"
            "</head>"
            "<body>"
            "<h1>Index on " + _req->getPath() + "</h1>"
            "<p>"
            "<hr>";
    DIR *r_opndir = opendir(resourcePath.c_str());
    if (NULL == r_opndir) {
        setStatus(INTERNAL_SERVER_ERROR);
        return 0;
    } else {
        struct dirent *dirContent;
        while ((dirContent = readdir(r_opndir))) {
            if (NULL == dirContent) {
                setStatus(INTERNAL_SERVER_ERROR);
                return 0;
            }
            _body += "<a href=\"" + _req->getPath();
            if (_body[_body.length() - 1] != '/') {
                _body += "/";
            }
            _body += dirContent->d_name;
            _body += "\"><br>";
            _body += dirContent->d_name;
            _body += "</a>";
        }
        _body += "</body></html>";
        setBody(_body);
    }
    closedir(r_opndir);
    return 1;
}

std::string
Response::getContentType(std::string resourcePath) {
    std::string contType;
    for (int i = resourcePath.length() - 1; i >= 0; --i) {
        if (resourcePath[i] == '.') {
            std::map<std::string, std::string>::const_iterator iter = MIMEs.find(resourcePath.substr(i + 1));
            if (iter != MIMEs.end()) {
                contType = iter->second;
                if (iter->second == "text") {
                    contType += "; charset=utf-8";
                }
            }
            break;
        }
    }
    return (contType);
}

void
Response::writeFile(const std::string &resourcePath) {
    std::ofstream outputToNewFile(resourcePath.c_str(),
        std::ios_base::out | std::ios_base::trunc);
    outputToNewFile << _req->getBody();
}

const std::string &
Response::getStatusLine(void) {
    return statusLines[_req->getStatus()];
}

std::string
Response::makeHeaders() {

    std::string allHeaders;

    if ((_req->getStatus() >= BAD_REQUEST) ||
        // (если не cgi и методы GET HEAD еще обдумать) ||
        (_req->getMethod() == "PUT" || _req->getMethod() == "DELETE")) {
        addHeader(CONTENT_TYPE, "text/html; charset=UTF-8");
    }

    for (iter it = headers.begin(); it != headers.end(); ++it) {
        if (it->value.empty()) {
            it->handle(*this);
        }
        if (!it->value.empty()) {
            allHeaders += headerNames[it->hash] + ": " + it->value + "\r\n";
        }
    }

    if (_cgi != NULL) {
        const_iter it = _cgi->getExtraHeaders().begin();
        for ( ; it != _cgi->getExtraHeaders().end(); ++it) {
            allHeaders += it->key + ": " + it->value + "\r\n";
        }
    }

    allHeaders += "\r\n";
    return allHeaders;
}

ResponseHeader *
Response::getHeader(uint32_t hash) {
    std::list<ResponseHeader>::iterator it;
    it = std::find(headers.begin(), headers.end(), ResponseHeader(hash));

    if (it == headers.end()) {
        return NULL;
    } else {
        return &(*it);
    }
}

void
Response::addHeader(uint32_t hash, const std::string &value) {
    ResponseHeader *ptr = getHeader(hash);
    if (ptr == NULL) {
        headers.push_back(ResponseHeader(hash, value));
    } else {
        ptr->value = value;
    }
}

void
Response::addHeader(uint32_t hash) {
    addHeader(hash, "");
}

int
Response::passToCGI(CGI &cgi) {
    _cgi = &cgi;
    cgi.clear();
    cgi.linkRequest(_req);
    cgi.setEnv();
    if (!cgi.exec()) {
        setStatus(BAD_GATEWAY);
        return 0;
    }
    cgi.parseHeaders();
    setBody(cgi.getBody());

    if (!cgi.isValidContentLength()) {
        setStatus(BAD_GATEWAY);
        return 0;
    }

    const_iter it = cgi.getHeaders().begin();
    const_iter end = cgi.getHeaders().end();
    for (; it != end; ++it) {
        addHeader(it->hash, it->value);
    }

    return 1;
}

void
Response::makeChunk() {
    _res = "";

    if (!getHeader(TRANSFER_ENCODING) ) {

        return ;
    }

    if (_resourceFileStream.eof()) {
        _resourceFileStream.close();
        _resourceFileStream.clear();
        return ;
    }
    char buffer[CHUNK_SIZE] = {0};
    _resourceFileStream.read(buffer, sizeof(buffer));
    if (_resourceFileStream.fail() && !_resourceFileStream.eof()) {
        setStatus(INTERNAL_SERVER_ERROR);
        _client->shouldBeClosed(true);
        return ;
    } else {
        _res.assign(buffer, CHUNK_SIZE);
        // _res = itoh(_resourceFileStream.gcount()) + "\r\n" + _res + "\r\n";
        _res = itoh(_res.length()) + "\r\n" + _res + "\r\n";
    }
    if (_resourceFileStream.eof()) {
        _res += "0\r\n\r\n";
    }
}

size_t
Response::getResponseLength() {
    return (_res.length());
}

const std::string &
Response::getResponse() {
    return _res;
}

const std::string &
Response::getBody() const {
    return _body;
}

void
Response::setBody(const std::string &body) {
    _body = body;
    setBodyLength(_body.length());
}

size_t
Response::getBodyLength(void) const {
    return _bodyLength;
}

void
Response::setBodyLength(size_t len) {
    _bodyLength = len;
}

void
Response::setRequest(Request *req) {
    _req = req;
}

Request *
Response::getRequest(void) const {
    return _req;
}

StatusCode
Response::getStatus() {
    return getRequest()->getStatus();
}

void
Response::setStatus(StatusCode status) {
    getRequest()->setStatus(status);
}

void
Response::setClient(Client *client) {
    _client = client;
}

Client *
Response::getClient(void) {
    return _client;
}

bool
Response::isFormed(void) const {
    return _isFormed;
}

void
Response::isFormed(bool formed) {
    _isFormed = formed;
}

// std::string Response::TransferEncodingChunked(std::string buffer, size_t bufSize) {
//     size_t i = 0;
//     std::string sizeChunck = itoh(SIZE_FOR_CHUNKED) + "\r\n";
//     while (bufSize > i + SIZE_FOR_CHUNKED) {
//         _res += sizeChunck;
//         _res += buffer.substr(i, (size_t)SIZE_FOR_CHUNKED) + "\r\n";
//         i += SIZE_FOR_CHUNKED;
//     }
//     _res += itoh(bufSize - i) + "\r\n";
//     _res += buffer.substr(i, bufSize - i) + "\r\n"
//             "0\r\n\r\n";
//     return ("");
// }

std::map<std::string, CGI>::iterator
Response::isCGI(const std::string &filepath, std::map<std::string, CGI> &cgis) {
    std::map<std::string, CGI>::iterator it  = cgis.begin();
    std::map<std::string, CGI>::iterator end = cgis.end();
    for (; it != end; it++) {
        if (endsWith(filepath, it->first)) {
            it->second.setScriptPath(filepath);
            return it;
        }
    }
    return end;
}

} // namespace HTTP
