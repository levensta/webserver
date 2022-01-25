#include "ServerBlock.hpp"

enum ServerBlockError {
    SOCKET_ERR = -2,
    SETOPT_ERR = -3,
    BIND_ERR = -5,
    LISTEN_ERR = -6,
};

ServerBlock::ServerBlock() : _addr("127.0.0.1"), _port(7676){};

ServerBlock::ServerBlock(const std::string &ipaddr, const uint16_t port) : _addr(ipaddr), _port(port) {}

ServerBlock::ServerBlock(const ServerBlock &obj) {
    operator=(obj);
}

ServerBlock::~ServerBlock() {}

ServerBlock &ServerBlock::operator=(const ServerBlock &obj) {
    if (this != &obj) {
        _addr = obj._addr; // Потом будет браться из конфига
        _port = obj._port;
        _servfd = obj._servfd;
    }
    return (*this);
}

void ServerBlock::createSock(void) {
    _servfd = socket(PF_INET, SOCK_STREAM, 0);
    if (_servfd < 0) {
        Log.error("Cannot create listening socket");
        exit(SOCKET_ERR);
    }
}

void ServerBlock::reuseAddr(void) {
    int i = 1;
    if (setsockopt(_servfd, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(int)) < 0) {
        Log.error("Cannot set options of the listening socket");
        exit(SETOPT_ERR);
    }
}

void ServerBlock::bindAddr(void) {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(_port);
    addr.sin_addr.s_addr = inet_addr(_addr.c_str());
    if (bind(_servfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        Log.error("Cannot bind listening socket");
        exit(BIND_ERR);
    }
}

void ServerBlock::listenSock(void) {
    if (listen(_servfd, SOMAXCONN) < 0) {
        Log.error("Listen failed");
        exit(LISTEN_ERR);
    }
}

int ServerBlock::getServFd(void) {
    return (_servfd);
}

void ServerBlock::createListenSock(void) {
    createSock();
    reuseAddr();
    bindAddr();
    listenSock();
}
