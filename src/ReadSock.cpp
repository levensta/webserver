#include "ReadSock.hpp"

static const size_t MAX_PACKET_SIZE = 65536;

#include <iostream>

ReadSock::Status ReadSock::readSocket(int fd) {
    char buf[MAX_PACKET_SIZE + 1] = {0};

    int recvBytes = recv(fd, buf, MAX_PACKET_SIZE, 0);

    if (recvBytes < 0) {
        //_rems.erase(fd); // Not needed with nonblocking sockets
        return RECV_END_NB;

    } else if (recvBytes == 0) {
        _rems.erase(fd);
        return RECV_END;

    } else {
        buf[recvBytes + 1] = '\0';
        _rems[fd] += buf;
        return RECV_DONE;
    }
}

ReadSock::Status ReadSock::getline(struct s_sock &sock, std::string &line) {
    int fd = sock.fd;
    if (fd < 0) {
        return INVALID_FD;
    }

    if (sock.perm & PERM_READ) {
        Log.debug("Readsock:34, before readSocket");
        ReadSock::Status status = readSocket(fd);
        Log.debug("Readsock:36, after readSocket");

        if (status == ReadSock::RECV_END) {
            return status; // ???
        }
        if (status == ReadSock::RECV_DONE) {
            sock.perm |= ~PERM_READ;
        }
    }

    size_t pos = _rems[fd].find("\r\n");
    if (pos == std::string::npos) {
        return LINE_NOT_FOUND;
    }

    line = _rems[fd].substr(0, pos);
    _rems[fd].erase(0, pos + 2);

    return LINE_FOUND;
}



// Location /index.html {
//     rewrite ^/oldURL$ https://www.your_domain.com/newURL redirect;
// }
// if (rewrite.empty() == false) {
//     func();
//     return 301;
// }
