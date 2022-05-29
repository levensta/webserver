#pragma once

#include <map>
#include <string>
#include <vector>

#include "Redirect.hpp"
#include "CGI.hpp"

namespace HTTP {

    class Location
    {
    private:
        std::string _path;
        std::string _root;
        bool _autoindex;
        std::vector<std::string> _index;
        int _post_max_body;
        std::vector<std::string> _allowedMethods;
        std::map<std::string, CGI> _cgiPaths;
        Redirect _redirect;

    public:
        Location(void);
        ~Location(void);

        Redirect &getRedirectRef(void);
        std::string &getPathRef(void);
        bool &getAutoindexRef(void);
        int &getPostMaxBodyRef(void);
        std::string &getRootRef(void);
        std::vector<std::string> &getIndexRef(void);
        std::vector<std::string> &getAllowedMethodsRef(void);
        std::map<std::string, CGI> &getCGIPathsRef(void);
    };

}
