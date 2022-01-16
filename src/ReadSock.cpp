#include "ReadSock.hpp"

static const size_t MAX_PACKET_SIZE = 65536;

int ReadSock::readSocket(int fd) {
    char buf[MAX_PACKET_SIZE + 1];

    int recvBytes = recv(fd, buf, MAX_PACKET_SIZE, 0);
    if (recvBytes < 0) {
        _rems.erase(fd);
        return RECV_FAIL;

    } else if (recvBytes == 0) {
        _rems.erase(fd);
        return RECV_END;

    } else {
        buf[recvBytes + 1] = '\0';
        _rems[fd] += buf;
        return RECV_DONE;
    }
}

int ReadSock::getline(struct s_sock &sock, std::string &line) {
    int fd = sock.fd;
    if (fd < 0) {
        return INVALID_FD;
    }

    if (sock.perm & PERM_READ) {
        int status = readSocket(fd);

        if (status <= 0) {
            return status;
        }
        sock.perm |= ~PERM_READ;
    }

    size_t pos = _rems[fd].find("\r\n");
    if (pos == std::string::npos) {
        return LINE_NOT_FOUND;
    }

    line = _rems[fd].substr(0, pos);
    try {
        _rems[fd] = _rems[fd].substr(pos + 2);
    } catch (std::out_of_range &e) {
        _rems[fd] = "";
    }

    return LINE_FOUND;
}
