#include "ServerBlock.hpp"

namespace HTTP {

ServerBlock::ServerBlock() {}

ServerBlock::ServerBlock(const ServerBlock &obj) {
    operator=(obj);
}

ServerBlock::~ServerBlock() { }

ServerBlock &
ServerBlock::operator=(const ServerBlock &obj) {
    if (this != &obj) {
        _addr            = obj._addr;
        _port            = obj._port;
        _blockname       = obj._blockname;
        _server_names    = obj._server_names;
        _locationBase    = obj._locationBase;
        _locations       = obj._locations;
    }
    return (*this);
}

void
ServerBlock::setAddr(const std::string &ipaddr) {
    this->_addr = ipaddr;
}

void
ServerBlock::setBlockname(const std::string &blockname) {
    this->_blockname = blockname;
}

const std::string &
ServerBlock::getBlockName(void) const {
    return this->_blockname;
}

std::string &
ServerBlock::getAddrRef(void) {
    return _addr;
}

ServerBlock::ServerNamesVec &
ServerBlock::getServerNamesRef(void) {
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

ServerBlock::LocationsMap &
ServerBlock::getLocationsRef(void) {
    return _locations;
}

const ServerBlock::LocationsMap &
ServerBlock::getLocationsRef(void) const {
    return _locations;
}

Location &
ServerBlock::getLocationBaseRef(void) {
    return _locationBase;
}

bool
ServerBlock::hasName(const std::string &name) const {
   return std::find(_server_names.begin(), _server_names.end(), name) != _server_names.end();
}

bool
ServerBlock::hasAddr(const std::string &addr) const {
   return (_addr == addr || _addr == "0.0.0.0");
}

Location *
ServerBlock::matchLocation(const std::string &path) {
    std::size_t matchMaxLen = 0;

    LocationsMap::iterator match = _locations.end();
    for (LocationsMap::iterator it = _locations.begin(); it != _locations.end(); ++it) {
        std::size_t len = it->first.length();
        if (path.find(it->first) == 0 && (path.length() == it->first.length() || path[it->first.length()] == '/')) {
            if (len > matchMaxLen) {
                match = it;
                matchMaxLen = len;
            }
        }
    }
    if (match == _locations.end()) {
        Log.debug() << "ServerBlock:: location: / for " << path << Log.endl;
        return &_locationBase;
    }
    Log.debug() << "ServerBlock:: location: " << match->first << " for " << path << Log.endl;
    return &(match->second);
}

}
