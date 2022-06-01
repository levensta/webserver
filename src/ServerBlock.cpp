#include "ServerBlock.hpp"

namespace HTTP {

enum ServerBlockError {
    SOCKET_ERR = -2,
    SETOPT_ERR = -3,
    BIND_ERR   = -5,
    LISTEN_ERR = -6
};

ServerBlock::ServerBlock()
    : _addr("127.0.0.1")
    , _port(7676) { }

ServerBlock::ServerBlock(const std::string &ipaddr, const int port)
    : _addr(ipaddr)
    , _port(port) { }

ServerBlock::ServerBlock(const ServerBlock &obj) {
    operator=(obj);
}

ServerBlock::~ServerBlock() { }

ServerBlock &
ServerBlock::operator=(const ServerBlock &obj) {
    if (this != &obj) {
        _addr            = obj._addr; // Потом будет браться из конфига
        _port            = obj._port;
        _servfd          = obj._servfd;
        _blockname       = obj._blockname;
        _server_names    = obj._server_names;
        _locationBase    = obj._locationBase;
        _errorPagesPaths = obj._errorPagesPaths;
        _locations       = obj._locations;
    }
    return (*this);
}

void
ServerBlock::createSock(void) {
    _servfd = socket(PF_INET, SOCK_STREAM, 0);
    if (_servfd < 0) {
        Log.error("Cannot create listening socket");
        exit(SOCKET_ERR);
    }
}

void
ServerBlock::reuseAddr(void) {
    int i = 1;
    if (setsockopt(_servfd, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(int)) < 0) {
        Log.error("Cannot set options of the listening socket");
        exit(SETOPT_ERR);
    }
}

void
ServerBlock::bindAddr(void) {
    struct sockaddr_in addr;
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(_port);
    addr.sin_addr.s_addr = inet_addr(_addr.c_str());
    if (bind(_servfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        Log.error("Cannot bind listening socket");
        Log.error("Port:" + to_string(_port));
        exit(BIND_ERR);
    }
}

void
ServerBlock::listenSock(void) {
    if (listen(_servfd, SOMAXCONN) < 0) {
        Log.error("Listen failed");
        exit(LISTEN_ERR);
    }
}

int
ServerBlock::getServFd(void) {
    return (_servfd);
}

void
ServerBlock::createListenSock(void) {
    createSock();
    reuseAddr();
    bindAddr();
    listenSock();
}

void
ServerBlock::setAddr(const std::string &ipaddr) {
    this->_addr = ipaddr;
}

void
ServerBlock::setBlockname(const std::string &blockname) {
    this->_blockname = blockname;
}

std::string &
ServerBlock::getAddrRef(void) {
    return _addr;
}

std::vector<std::string> &
ServerBlock::getServerNameRef(void) {
    return _server_names;
}

int
ServerBlock::getPort(void) const {
    return _port;
}

int &
ServerBlock::getPortRef(void) {
    return _port;
}

std::map<std::string, Location> &
ServerBlock::getLocationsRef(void) {
    return _locations;
}

const std::map<std::string, Location> &
ServerBlock::getLocationsRef(void) const {
    return _locations;
}

std::map<int, std::string> &
ServerBlock::getErrPathsRef(void) {
    return _errorPagesPaths;
}

Location &
ServerBlock::getLocationBaseRef(void) {
    return _locationBase;
}

Location *
ServerBlock::matchLocation(const std::string &path) {
    size_t matchMaxLen = 0;

    // uri: /about/cgi valid
    // loc: /about
    // root: /var/site
    // res: /var/site/cgi   301

    // uri: /about/ valid
    // loc: /about
    // root: /var/site/
    // res: /var/site//  OK
    
    // uri: /about/ valid
    // loc: /about
    // root: /var/site
    // res: /var/site/  OK

    // uri: /about valid
    // loc: /about
    // root: /var/site/
    // res: /var/site  301

    // uri: /about  valid
    // loc: /about

    // uri: /aboutsfff not valid
    // loc: /about

    // uri: /about/cgi valid
    // loc: /about/

    // uri: /about not valid
    // loc: /about/

    // uri: /aboutcfofgg not valid
    // loc: /about/
    std::map<std::string, Location>::iterator it = _locations.begin();
    std::map<std::string, Location>::iterator end = _locations.end();
    std::map<std::string, Location>::iterator match = _locations.end();
    for (; it != end; it++) {
        size_t len = it->first.length();
        if (path.find(it->first) == 0 && (path.length() == it->first.length() || path[it->first.length()] == '/')) {
            if (len > matchMaxLen) {
                match = it;
                matchMaxLen = len;
            }
        }
    }
    if (match == end) {
        return &_locationBase;
    } 
    return &(match->second);
}

}
