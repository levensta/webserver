#pragma once

#include <string>
#include <unistd.h>
#include <deque>

#include "Request.hpp"
#include "Response.hpp"
#include "Logger.hpp"
#include "Status.hpp"
#include "Globals.hpp"
#include "Pool.hpp"
#include "Socket.hpp"

namespace HTTP {

class Client { 

private:
    Socket      *_clientSock;
    Socket      *_serverSock;
    Socket      *_targetSock;

    bool        _headSent;
    bool        _bodySent;
    bool        _shouldBeClosed;

    std::size_t      _nbRequests;
    std::size_t      _maxRequests;
    std::time_t      _clientTimeout;
    std::time_t      _targetTimeout;
    std::time_t      _maxTimeout;

    std::list<Request *>  _requests;
    std::list<Response *> _responses;

public:
    Client(void);
    ~Client(void);

    bool replyDone(void);
    void replyDone(bool);

    bool shouldBeClosed(void) const;
    void shouldBeClosed(bool);

    const std::string getHostname(void);

    time_t getClientTimeout(void) const;
    time_t getTargetTimeout(void) const;

    void setClientTimeout(time_t);
    void setTargetTimeout(time_t);

    Socket *getClientSock(void);
    Socket *getServerSock(void);
    Socket *getTargetSock(void);
    void setClientSock(Socket *);
    void setServerSock(Socket *);
    void setTargetSock(Socket *);

    void checkIfFailed(void);
    void addRequest(void);
    void addResponse(void);
    void removeRequest(void);
    void removeResponse(void);

    void pollin(int fd);
    void pollout(int fd);
    void pollhup(int fd);
    void pollerr(int fd);

    void receive(Request *);
    void receive(Response *);
    void reply(Request *);
    void reply(Response *);

    // HTTP::ServerBlock *matchServerBlock(const std::string &host, std::size_t port) const;
};

}
