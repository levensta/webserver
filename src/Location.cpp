#include "Location.hpp"

namespace HTTP {

Location::Location() { }
Location::~Location() { }

std::string &
Location::getPathRef(void) {
    return _path;
}

Auth &
Location::getAuthRef(void) {
    return _auth;
}

bool &
Location::getAutoindexRef(void) {
    return _autoindex;
}

int &
Location::getPostMaxBodyRef(void) {
    return _post_max_body;
}

std::string &
Location::getAliasRef(void) {
    return _alias;
}

std::string &
Location::getRootRef(void) {
    return _root;
}

std::vector<std::string> &
Location::getIndexRef(void) {
    return _index;
}

std::vector<std::string> &
Location::getAllowedMethodsRef(void) {
    return _allowedMethods;
}

std::map<std::string, HTTP::CGI> &
Location::getCGIsRef(void) {
    return _CGIs;
}

Redirect &
Location::getRedirectRef(void) {
    return _redirect;
}
}
