#include "Config.hpp"

int
isInteger(double &num) {
    return (num - static_cast<int32_t>(num) == 0);
}

int
isUInteger(double &num) {
    return (isInteger(num) && num >= 0);
}

std::string
getExpectedTypeName(ExpectedType type) {
    switch (type) {
        case ARRAY:
            return "array";
        case NUMBER:
            return "number";
        case OBJECT:
            return "object";
        case STRING:
            return "string";
        case BOOLEAN:
            return "boolean";
        default:
            return "unknown";
    }
}

int
typeExpected(JSON::AType *ptr, ExpectedType type) {
    switch (type) {
        case STRING:
            return ptr->isStr();
        case BOOLEAN:
            return ptr->isBool();
        case NUMBER:
            return ptr->isNum();
        case OBJECT:
            return ptr->isObj();
        case ARRAY:
            return ptr->isArr();
    }
    return 0;
}

template <typename T>
ConfStatus
basicCheck(JSON::Object *src, const std::string &key, ExpectedType type, T &res, T def) {
    JSON::AType *ptr = src->get(key);
    if (ptr->isNull()) {
        res = def;
        Log.info() << "Used default parameter for " << src->getKey() << "::" << key << Log.endl;
        return DEFAULT;
    }

    if (!typeExpected(ptr, type)) {
        Log.error() << key << ": expected " << getExpectedTypeName(type) << Log.endl;
        Log.error() << key << ": got " << ptr->getType() << Log.endl;
        return NONE_OR_INV;
    }
    return SET;
}

ConfStatus
basicCheck(JSON::Object *src, const std::string &key, ExpectedType type) {
    JSON::AType *ptr = src->get(key);
    if (ptr->isNull()) {
        Log.error() << key << "does not exist." << Log.endl;
        return NONE_OR_INV;
    }

    if (!typeExpected(ptr, type)) {
        Log.error() << key << ": expected " << getExpectedTypeName(type) << Log.endl;
        Log.error() << key << ": got " << ptr->getType() << Log.endl;
        return NONE_OR_INV;
    }
    return SET;
}

int
getUInteger(JSON::Object *src, const std::string &key, int &res, int def) {
    ConfStatus status = basicCheck(src, key, NUMBER, res, def);
    if (status != SET) {
        return status;
    }

    double num = src->get(key)->toNum();
    if (isUInteger(num)) {
        res = static_cast<unsigned int>(num);
        return SET;
    } else {
        Log.error() << key << ": should be an unsigned integer." << Log.endl;
        return NONE_OR_INV;
    }
}

int
getUInteger(JSON::Object *src, const std::string &key, int &res) {
    ConfStatus status = basicCheck(src, key, NUMBER);
    if (status != SET) {
        return status;
    }

    double num = src->get(key)->toNum();
    if (isUInteger(num)) {
        res = static_cast<unsigned int>(num);
        return SET;
    } else {
        Log.error() << key << ": should be an unsigned integer." << Log.endl;
        return NONE_OR_INV;
    }
}

int
getString(JSON::Object *src, const std::string &key, std::string &res, std::string def) {
    ConfStatus status = basicCheck(src, key, STRING, res, def);
    if (status != SET) {
        return status;
    }

    res = src->get(key)->toStr();
    return SET;
}

int
getString(JSON::Object *src, const std::string &key, std::string &res) {
    ConfStatus status = basicCheck(src, key, STRING);
    if (status != SET) {
        return status;
    }

    res = src->get(key)->toStr();
    return SET;
}

int
getBoolean(JSON::Object *src, const std::string &key, bool &res, bool def) {
    ConfStatus status = basicCheck(src, key, BOOLEAN, res, def);
    if (status != SET) {
        return status;
    }

    res = src->get(key)->toBool();
    return SET;
}

int
getBoolean(JSON::Object *src, const std::string &key, bool &res) {
    ConfStatus status = basicCheck(src, key, BOOLEAN);
    if (status != SET) {
        return status;
    }

    res = src->get(key)->toBool();
    return SET;
}

template <typename T>
int
isSubset(std::vector<T> set, std::vector<T> subset) {
    typename std::vector<T>::iterator it  = subset.begin();
    typename std::vector<T>::iterator end = subset.end();
    for (; it != end; it++) {
        if (std::find(set.begin(), set.end(), *it) == set.end()) {
            return 0;
        }
    }
    return 1;
}

int
getArray(JSON::Object *src, const std::string &key, std::vector<std::string> &res, std::vector<std::string> def) {
    ConfStatus status = basicCheck(src, key, ARRAY, res, def);
    if (status != SET) {
        return status;
    }

    JSON::Array *arr = src->get(key)->toArr();

    // Overwriting inherited values from location_base
    res.clear();

    JSON::Array::iterator it  = arr->begin();
    JSON::Array::iterator end = arr->end();
    for (; it != end; it++) {
        if ((*it)->isNull() || !(*it)->isStr()) {
            Log.error() << key << " has mixed value(s)" << Log.endl;
            return NONE_OR_INV;
        }
        res.push_back((*it)->toStr());
    }
    return SET;
}

int
getArray(JSON::Object *src, const std::string &key, std::vector<std::string> &res) {
    ConfStatus status = basicCheck(src, key, ARRAY);
    if (status != SET) {
        return status;
    }

    JSON::Array *arr = src->get(key)->toArr();

    JSON::Array::iterator it  = arr->begin();
    JSON::Array::iterator end = arr->end();
    res.clear();
    for (; it != end; it++) {
        if ((*it)->isNull() || !(*it)->isStr()) {
            Log.error() << key << " has mixed value(s)" << Log.endl;
            return NONE_OR_INV;
        }
        res.push_back((*it)->toStr());
    }
    return SET;
}

std::vector<std::string>
getDefaultAllowedMethods() {

    std::vector<std::string> allowed(9);
    for (size_t i = 0; validMethods[i]; i++) {
        allowed.push_back(validMethods[i]);   
    }

    return allowed;
}

const char * validKeywords[] = {
    KW_ADDR, KW_PORT, KW_SERVER_NAMES, KW_ERROR_PAGES,
    KW_LOCATIONS, KW_CGI, KW_ROOT, KW_ALIAS, KW_INDEX, KW_AUTOINDEX, 
    KW_METHODS_ALLOWED, KW_POST_MAX_BODY, KW_REDIRECT, KW_AUTH_BASIC, NULL
};

const char * validServerBlockKeywords[] = {
    KW_ADDR, KW_PORT, KW_SERVER_NAMES, KW_ERROR_PAGES,
    KW_LOCATIONS, KW_CGI, KW_ROOT, KW_INDEX, KW_AUTOINDEX, 
    KW_METHODS_ALLOWED, KW_POST_MAX_BODY, KW_REDIRECT, KW_AUTH_BASIC, NULL
};

const char * validLocationKeywords[] = {
    KW_CGI, KW_ROOT, KW_ALIAS, KW_INDEX, KW_AUTOINDEX, KW_ERROR_PAGES,
    KW_METHODS_ALLOWED, KW_POST_MAX_BODY, KW_REDIRECT, KW_AUTH_BASIC, NULL
};

const char * validRedirectKeywords[] = {
    KW_CODE, KW_URL, NULL
};

const char * validAuthBasicKeywords[] = {
    KW_REALM, KW_USER_FILE, NULL
};

bool
isValidKeyword(const std::string &key, const char *contextKeywords[]) {

    for (size_t i = 0; contextKeywords[i]; i++) {
        if (contextKeywords[i] == key) {
            return true;
        }
    }

    for (size_t i = 0; validKeywords[i]; i++) {
        if (validKeywords[i] == key) {
            Log.error() << "Invalid context for keyword " << key << Log.endl;
            return false;
        }
    }

    Log.error() << "Unrecognized keyword " << key << Log.endl;
    return false;
}

bool
isValidKeywords(JSON::Object *src, const char *validKeywords[]) {
    JSON::Object::iterator it  = src->begin();
    JSON::Object::iterator end = src->end();
    for (; it != end; it++) {
        if (!isValidKeyword(it->first, validKeywords)) {
            return false;
        }
    }
    return true;
}


// Object parsing
int
parseCGI(JSON::Object *src, std::map<std::string, HTTP::CGI> &res) {
    std::map<std::string, HTTP::CGI> def;

    res.clear();
    const std::string &key = KW_CGI;

    ConfStatus status = basicCheck(src, key, OBJECT, res, def);
    if (status != SET) {
        return status;
    }

    JSON::Object *obj = src->get(key)->toObj();

    JSON::Object::iterator it  = obj->begin();
    JSON::Object::iterator end = obj->end();
    for (; it != end; it++) {
        HTTP::CGI cgi;

        std::string value;
        if (!getString(obj, it->first, value)) {
            return NONE_OR_INV;
        }
        cgi.setExecPath(value);
        if (it->first == cgi.compiledExt) {
            cgi.compiled(true);
        }
        
        res.insert(std::make_pair(it->first, cgi));
    }
    return SET;
}

int
isValidCGI(std::map<std::string, HTTP::CGI> &res) {
    std::map<std::string, HTTP::CGI>::iterator it;

    for (it = res.begin(); it != res.end(); it++) {
        if (!isExtension(it->first)) {
            Log.error() << it->first << ": incorrect extension" << Log.endl;
            return false;

        } else if (!it->second.compiled() && !isExecutableFile(it->second.getExecPath())) {
            Log.error() << it->second.getExecPath() << " is not an executable file" << Log.endl;
            return false;
        }
    }
    return true;
}

int
parseErrorPages(JSON::Object *src, std::map<int, std::string> &res) {

    res.clear();

    std::map<int, std::string> def;

    ConfStatus status = basicCheck(src, KW_ERROR_PAGES, OBJECT, res, def);
    if (status != SET) {
        return status;
    }

    JSON::Object *obj = src->get(KW_ERROR_PAGES)->toObj();

    JSON::Object::iterator it;
    for (it = obj->begin(); it != obj->end(); it++) {

        double value = strtod(it->first.c_str(), NULL);
        if (!isUInteger(value)) {
            Log.error() << KW_ERROR_PAGES << " code is not an positive interger" << Log.endl;
            return NONE_OR_INV;
        } else if (value < 300 || value > 599) {
            Log.error() << KW_ERROR_PAGES << " code " << value << " is beyong boundaries" << Log.endl;
            return NONE_OR_INV;
        }

        int code = static_cast<int>(value);
        if (it->second->isNull() || !it->second->isStr()) {
            Log.error() << KW_ERROR_PAGES << " value " << value <<  " is not a string" << Log.endl;
            return NONE_OR_INV;
        }

        res.insert(std::make_pair(code, it->second->toStr()));
    }
    return SET;
}

int
isValidErrorPages(std::map<int, std::string> &res) {
    std::map<int, std::string>::iterator it;

    for (it = res.begin(); it != res.end(); it++) {
        if (!resourceExists(it->second)) {
            Log.error() << it->second << ": file does not exist" << Log.endl;
            return false;
        }
        if (!isReadableFile(it->second)) {
            Log.error() << it->second + ": is not readable file" << Log.endl;
            return false;
        }
    }
    return true;
}

int
parseRedirect(JSON::Object *src, HTTP::Redirect &res) {
    HTTP::Redirect def;

    ConfStatus status = basicCheck(src, KW_REDIRECT, OBJECT, res, def);
    if (status != SET) {
        return status;
    }

    JSON::Object *obj = src->get(KW_REDIRECT)->toObj();
    int code = 0;
    if (!getUInteger(obj, KW_CODE, code))
        return NONE_OR_INV;

    if (!getString(obj, KW_URL, res.getURIRef()))
        return NONE_OR_INV;

    res.getCodeRef() = static_cast<HTTP::StatusCode>(code);
    res.set(true);
    return SET;
}

int
isValidRedirect(HTTP::Redirect &res) {

    if (res.set()) {
        if (res.getCodeRef() < 300 && res.getCodeRef() > 308) {
            Log.error() << KW_REDIRECT << "::" << KW_CODE << " " << res.getCodeRef() << " is invalid" << Log.endl;
            return 0;
        } else if (res.getURIRef().empty()) {
            Log.error() << KW_REDIRECT << "::" << KW_URL << " is empty" << Log.endl;
            return 0;
        }
    }
    return 1;
}

int
parseAuth(JSON::Object *src, HTTP::Auth &res) {

    ConfStatus status = basicCheck(src, KW_AUTH_BASIC, OBJECT, res, res);
    if (status != SET) {
        return status;
    }

    JSON::Object *obj = src->get(KW_AUTH_BASIC)->toObj();

    if (!getString(obj, KW_REALM, res.getRealmRef())) {
        return NONE_OR_INV;
    }

    if (!getString(obj, KW_USER_FILE, res.getFileRef())) {
        return NONE_OR_INV;
    }

    res.set(true);
    return SET;
}

int
isValidAuth(HTTP::Auth &res) {

    if (res.isSet()) {
        if (res.getRealmRef().empty()) {
            Log.error() << KW_REALM << " is empty" << Log.endl;
            return 0;
        } else if (!resourceExists(res.getFileRef())) {
            Log.error() << KW_USER_FILE << " " << res.getFileRef() << " does not exist" << Log.endl;
            return 0;
        } else if (!isReadableFile(res.getFileRef())) {
            Log.error() << KW_USER_FILE << " " << res.getFileRef() << " is not readable" << Log.endl;
            return 0;
        } else if (!res.loadData()) {
            Log.error() << KW_USER_FILE << " " << res.getFileRef() << " load failed" << Log.endl;
            return 0;
        }
    }
    return 1;
}

int checkMutualExclusions(JSON::Object *src, const std::string &key1, const std::string &key2) {
    JSON::AType *ptr1 = src->get(key1);
    JSON::AType *ptr2 = src->get(key2);
    
    return ptr1->isNull() || ptr2->isNull();
}

int
parseLocation(JSON::Object *src, HTTP::Location &dst, HTTP::Location &def) {

    if (&dst != &def) {

        if (!isValidKeywords(src, validLocationKeywords)) {
            return NONE_OR_INV;
        }
        if (!checkMutualExclusions(src, KW_ALIAS, KW_ROOT)) {
            Log.error() << KW_ROOT << " and " << KW_ALIAS " are mutually exclusive" << Log.endl;
            return NONE_OR_INV;
        }

        ConfStatus aliasStatus = (ConfStatus)getString(src, KW_ALIAS, dst.getAliasRef(), "");
        
        if (aliasStatus == NONE_OR_INV) {
            Log.error() << KW_ALIAS << " parsing failed" << Log.endl;
            return NONE_OR_INV;
        }

        if (aliasStatus != DEFAULT) {
            if (!resourceExists(dst.getAliasRef())) {
                Log.error() << KW_ALIAS << " " << dst.getAliasRef() + " does not exist" << Log.endl;
                return NONE_OR_INV;
            } else if (!isDirectory(dst.getAliasRef())) {
                Log.error() << KW_ALIAS << " must be a directory" << Log.endl;
                return NONE_OR_INV;
            }
        } 
    }

    if (!getString(src, KW_ROOT, dst.getRootRef(), def.getRootRef())) {
        Log.error() << KW_ROOT << " parsing failed" << Log.endl;
        return NONE_OR_INV;
    } else if (!resourceExists(dst.getRootRef())) {
        Log.error() << KW_ROOT << dst.getRootRef() << " does not exist" << Log.endl;
        return NONE_OR_INV;
    } else if (!isDirectory(dst.getRootRef())) {
        Log.error() << KW_ROOT << " must be a directory" << Log.endl;
        return NONE_OR_INV;
    } else if (dst.getRootRef()[dst.getRootRef().length() - 1] != '/') { // ??
        dst.getRootRef() += "/";
    }

    if (!getUInteger(src, KW_POST_MAX_BODY, dst.getPostMaxBodyRef(), 200)) {
        Log.error() << KW_POST_MAX_BODY << " parsing failed" << Log.endl;
        return NONE_OR_INV;
    }

    if (!getBoolean(src, KW_AUTOINDEX, dst.getAutoindexRef(), false)) {
        Log.error() << KW_AUTOINDEX << " parsing failed" << Log.endl;
        return NONE_OR_INV;
    }

    if (!parseRedirect(src, dst.getRedirectRef())) {
        Log.error() << KW_REDIRECT << " parsing failed" << Log.endl;
        return NONE_OR_INV;
    } else if (!isValidRedirect(dst.getRedirectRef())) {
        return NONE_OR_INV;
    }

    if (!parseErrorPages(src, dst.getErrorPagesRef())) {
        Log.error() << KW_ERROR_PAGES << " parsing failed" << Log.endl;
        return NONE_OR_INV;
    } else if (!isValidErrorPages(dst.getErrorPagesRef())) {
        Log.error() << KW_ERROR_PAGES << " parsing failed" << Log.endl;
        return NONE_OR_INV;
    }

    if (!parseAuth(src, dst.getAuthRef())) {
        Log.error() << KW_AUTH_BASIC << " parsing failed" << Log.endl;
        return NONE_OR_INV;
    } else if (!isValidAuth(dst.getAuthRef())) {
        return NONE_OR_INV;
    }

    if (!parseCGI(src, dst.getCGIsRef())) {
        Log.error() << KW_CGI << " parsing failed" << Log.endl;
        return NONE_OR_INV;
    } else if (!isValidCGI(dst.getCGIsRef())) {
        Log.error() << KW_CGI << " is invalid, usage: <ext>:<path-to-exec>" << Log.endl;
        return NONE_OR_INV;
    }

    if (!getArray(src, KW_METHODS_ALLOWED, dst.getAllowedMethodsRef(), getDefaultAllowedMethods())) {
        Log.error() << KW_METHODS_ALLOWED << " parsing failed" << Log.endl;
        return NONE_OR_INV;
    } else if (!isSubset(getDefaultAllowedMethods(), dst.getAllowedMethodsRef())) {
        Log.error() << KW_METHODS_ALLOWED << " has unrecognized value" << Log.endl;
        return NONE_OR_INV;
    }

    if (!getArray(src, KW_INDEX, dst.getIndexRef(), def.getIndexRef())) {
        Log.error() << KW_INDEX << " parsing failed" << Log.endl;
        return NONE_OR_INV;
    }

    return SET;
}

int
parseLocations(JSON::Object *src, std::map<std::string, HTTP::Location> &res, HTTP::Location &base) {
    ConfStatus status = basicCheck(src, KW_LOCATIONS, OBJECT, res, res);
    if (status != SET) {
        return status;
    }

    JSON::Object *locations = src->get(KW_LOCATIONS)->toObj();

    JSON::Object::iterator it;
    for (it = locations->begin(); it != locations->end(); it++) {
    
        HTTP::Location location = base;
        if (!basicCheck(locations, it->first, OBJECT)) {
            return NONE_OR_INV;
        }

        if (!isValidPath(it->first)) {
            Log.error() << "location " << it->first << " invalid path" << Log.endl;
            return NONE_OR_INV;
        }
        location.getPathRef() = it->first;

        JSON::Object *obj = it->second->toObj();
        if (!parseLocation(obj, location, base)) {
            Log.error() << "location " << it->first << " parsing failed" << Log.endl;
            return NONE_OR_INV;
        }
        
        res.insert(std::make_pair(it->first, location));
    }
    return SET;
}

bool
isValidPort(int port) {
    if (port > 65535) {
        Log.error() << KW_PORT << " upper bound in 65535" << Log.endl;
        return false;
    } else if (port > 49151) {
        Log.info() << KW_PORT << " 49151-65535 reserved for client apps" << Log.endl;
    } else if (port < 1024 && getuid() != 0) {
        Log.error() << KW_PORT << " 0-1023 reserved for OS-daemons in unix (use sudo)" << Log.endl;
        return false;
    }
    return true;
}

int
parseServerBlock(JSON::Object *src, HTTP::ServerBlock &dst) {

    if (!isValidKeywords(src, validServerBlockKeywords)) {
        return NONE_OR_INV;
    }

    if (!getArray(src, KW_SERVER_NAMES, dst.getServerNamesRef(), dst.getServerNamesRef())) {
        Log.error() << KW_SERVER_NAMES << " parsing failed" << Log.endl;
        return NONE_OR_INV;
    }

    if (!getString(src, KW_ADDR, dst.getAddrRef(), "0.0.0.0")) {
        Log.error() << KW_ADDR << " parsing failed" << Log.endl;
        return NONE_OR_INV;
    } else if (!isValidIpv4(dst.getAddrRef())) {
        Log.error() << KW_ADDR << " is invalid or not in ipv4 format" << Log.endl;
        return NONE_OR_INV;
    }

    if (!getUInteger(src, KW_PORT, dst.getPortRef())) {
        Log.error() << KW_PORT << " parsing failed" << Log.endl;
        return NONE_OR_INV;
    } else if (!isValidPort(dst.getPortRef())) {
        return NONE_OR_INV;
    }

    // char resolvedPath[256] = {0};
    // realpath("./", resolvedPath);
    dst.getLocationBaseRef().getRootRef() = "./";
    if (!parseLocation(src, dst.getLocationBaseRef(), dst.getLocationBaseRef())) {
        Log.error() << "location_base" << " parsing failed" << Log.endl;
        return NONE_OR_INV;
    }

    if (!parseLocations(src, dst.getLocationsRef(), dst.getLocationBaseRef())) {
        Log.error() << KW_LOCATIONS << " parsing failed" << Log.endl;
        return NONE_OR_INV;
    }

    return SET;
}

int
parseServerBlocks(JSON::Object *src, Server *serv) {

    ConfStatus status = basicCheck(src, KW_SERVERS, OBJECT);
    if (status != SET) {
        return status;
    }

    JSON::Object *servers = src->get(KW_SERVERS)->toObj();

    if (servers->begin() == servers->end()) {
        Log.error() << "Serverblocks not found" << Log.endl;
        return NONE_OR_INV;
    }

    JSON::Object::iterator it;
    for (it = servers->begin(); it != servers->end(); it++) {
        HTTP::ServerBlock servBlock;
        servBlock.setBlockname(it->first);

        if (!basicCheck(servers, it->first, OBJECT)) {
            return NONE_OR_INV;
        }

        JSON::Object *obj = it->second->toObj();
        if (!parseServerBlock(obj, servBlock)) {
            Log.error() << "Serverblock " << it->first << " parsing failed" << Log.endl;
            return NONE_OR_INV;
        }
        serv->addServerBlock(servBlock);
    }
    return SET;
}

Server *
loadConfig(const string filename) {
    JSON::Object *ptr;
    try {
        JSON::JSON json(filename);
        ptr = json.parse();
        if (ptr == NULL) {
            Log.error() << "Failed to parse " << filename << Log.endl;
            return NULL;
        }
    } catch (std::exception &e) {
        Log.error() << e.what() << " " << filename <<  Log.endl;
        return NULL;
    }

    Server *serv = new Server();
    if (!parseServerBlocks(ptr, serv)) {
        delete serv;
        serv = NULL;
    }

    delete ptr;
    return serv;
}
