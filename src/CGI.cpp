#include "CGI.hpp"
#include "Request.hpp"
#include "Client.hpp"

namespace HTTP {

CGI::CGI(void)
    : _isCompiled(false) {
    for (size_t i = 0; i < 3; i++)
        _args[i] = NULL;
}

CGI::~CGI() { }

static void
log_error(const std::string &location) {
    Log.error(location);
    Log.error("Errno: " + to_string(errno));
    Log.error("Description: " + std::string(strerror(errno)));
}

static void
restore_std(int in, int out) {
    if (in != -1 && dup2(in, fileno(stdin)) == -1) {
        log_error("CGI::restore::in: ");
    }
    if (out != -1 && dup2(out, fileno(stdout)) == -1) {
        log_error("CGI::restore::out: ");
    }
}

static void
close_pipe(int in, int out) {
    if (in != -1) {
        close(in);
    }
    if (out != -1) {
        close(out);
    }
}

static void
setValue(char *const env, const std::string &value) {
    char *ptr = strchr(env, '=');
    if (ptr == NULL) {
        return;
    }
    ptr[1] = '\0';
    strncat(env, value.c_str(), 1023 - strlen(env));
}

void
CGI::linkRequest(Request *req) {
    _req = req;
}

// This version passes all the env, including system
// Currently env pass with setEnv function.
void
CGI::setFullEnv(void) {
    setenv("PATH_INFO", "", 1);

    setenv("PATH_TRANSLATED", _req->getResolvedPath().c_str(), 1); // ?

    setenv("REMOTE_HOST", _req->getHeaderValue(HOST).c_str(), 1);
    setenv("REMOTE_ADDR", _req->getClient()->getIpAddr().c_str(), 1);
    setenv("REMOTE_USER", "", 1);
    setenv("REMOTE_IDENT", "", 1);

    setenv("AUTH_TYPE", _req->getLocation()->getAuthRef().isSet() ? "Basic" : "", 1);
    setenv("QUERY_STRING", _req->getQueryString().c_str(), 1);
    setenv("REQUEST_METHOD", _req->getMethod().c_str(), 1);
    setenv("SCRIPT_NAME", _req->getResolvedPath().c_str(), 1);
    setenv("CONTENT_LENGTH", to_string(_req->getBody().length()).c_str(), 1);
    setenv("CONTENT_TYPE", _req->getHeaderValue(CONTENT_TYPE).c_str(), 1);

    setenv("GATEWAY_INTERFACE", GATEWAY_INTERFACE, 1);
    setenv("SERVER_NAME",  _req->getUriRef().getAuthority().c_str(), 1);
    setenv("SERVER_SOFTWARE", SERVER_SOFTWARE, 1);
    setenv("SERVER_PROTOCOL", "HTTP/1.1", 1);

    // Current server block
    setenv("SERVER_PORT", to_string(_req->getServerBlock()->getPort()).c_str(), 1); // 80
    setenv("REDIRECT_STATUS", "200", 1);
}

void
CGI::setEnv(void) {

    // PATH_INFO
    setValue(env[0], "");
    
    // PATH_TRANSLATED
    setValue(env[1], _req->getResolvedPath()); // Definitely not like that
    
    // REMOTE_HOST
    setValue(env[2], _req->getHeaderValue(HOST)); // host, maybe should be without port
    
    // REMOTE_ADDR
    setValue(env[3], _req->getClient()->getIpAddr()); // ipv4 addr
    
    // REMOTE_USER
    setValue(env[4], "");
    
    // REMOTE_IDENT
    setValue(env[5], "");
    
    // AUTH_TYPE
    setValue(env[6], _req->getLocation()->getAuthRef().isSet() ? "Basic" : "");
    
    // QUERY_STRING
    setValue(env[7], _req->getQueryString());
    
    // REQUEST_METHOD
    setValue(env[8], _req->getMethod());
    
    // SCRIPT_NAME
    setValue(env[9], _req->getResolvedPath());
    
    // CONTENT_LENGTH
    setValue(env[10], to_string(_req->getBody().length()));
    
    // CONTENT_TYPE
    setValue(env[11], _req->getHeaderValue(CONTENT_TYPE));
    
    // GATEWAY_INTERFACE
    setValue(env[12], GATEWAY_INTERFACE);
    
    // SERVER_NAME
    setValue(env[13], _req->getUriRef().getAuthority()); // Not like that!
    
    // SERVER_SOFTWARE
    setValue(env[14], SERVER_SOFTWARE);
    
    // SERVER_PROTOCOL
    setValue(env[15], "HTTP/1.1");
    
    // SERVER_PORT
    setValue(env[16], to_string(_req->getServerBlock()->getPort()));

    // REDIRECT_STATUS
    setValue(env[17], "200");
}

void
CGI::setCompiled(bool value) {
    _isCompiled = value;
}

bool
CGI::isCompiled(void) {
    return _isCompiled;
}

void
CGI::setExecPath(const std::string path) {
    _execpath = path;
}

const std::string
CGI::getExecPath(void) const {
    return _execpath;
}

bool
CGI::setScriptPath(const std::string path) {
    _filepath = path;
    return true;
}

const std::string
CGI::getResult(void) const {
    return _res;
}

void
CGI::reset(void) {
    _res = "";
}

int
CGI::exec() {
    if (_isCompiled && !isExecutableFile(_filepath)) {
        Log.error(_filepath + " is not executable");
        return 0;
    }

    if (_isCompiled) {
        _args[0] = _filepath.c_str();
        _args[1] = "";
    } else {
        _args[0] = _execpath.c_str();
        _args[1] = _filepath.c_str();
    }

    int in[2]  = { -1 };
    int out[2] = { -1 };

    if (pipe(in) != 0) {
        log_error("CGI::pipe::in: ");
        return 0;
    }

    if (pipe(out) != 0) {
        log_error("CGI::pipe::out: ");
        close_pipe(in[0], in[1]);
        return 0;
    }

    int tmp[2] = { -1 };

    tmp[0] = dup(fileno(stdin));
    if (tmp[0] == -1) {
        log_error("CGI::backup::in: ");
        close_pipe(tmp[0], tmp[1]);
        close_pipe(in[0], in[1]);
        close_pipe(out[0], out[1]);
        return 0;
    }

    tmp[1] = dup(fileno(stdout));
    if (tmp[1] == -1) {
        log_error("CGI::backup::out: ");
        close_pipe(tmp[0], tmp[1]);
        close_pipe(in[0], in[1]);
        close_pipe(out[0], out[1]);
        return 0;
    }

    // Redirect for child process
    if (dup2(in[0], fileno(stdin)) == -1) {
        log_error("CGI::redirect::in: ");
        close_pipe(tmp[0], tmp[1]);
        close_pipe(in[0], in[1]);
        close_pipe(out[0], out[1]);
        return 0;
    }

    if (dup2(out[1], fileno(stdout)) == -1) {
        log_error("CGI::redirect::out: ");
        restore_std(tmp[0], -1);
        close_pipe(tmp[0], tmp[1]);
        close_pipe(in[0], in[1]);
        close_pipe(out[0], out[1]);
        return 0;
    }

    int childPID = fork();
    if (childPID < 0) {
        log_error("CGI::fork: ");
        restore_std(tmp[0], tmp[1]);
        close_pipe(tmp[0], tmp[1]);
        close_pipe(in[0], in[1]);
        close_pipe(out[0], out[1]);
        return 0;
    } else if (childPID == 0) {
        close_pipe(in[1], out[0]);
        if (execve(_args[0], const_cast<char *const *>(_args), env) == -1) {
            exit(125);
        }
    }

    if (_req->getBody() != "") {
        if (write(in[1], _req->getBody().c_str(), _req->getBody().length()) == -1) {
            log_error("CGI::write: ");
            restore_std(tmp[0], tmp[1]);
            close_pipe(tmp[0], tmp[1]);
            close_pipe(in[0], in[1]);
            close_pipe(out[0], out[1]);
            kill(childPID, SIGKILL);
            return 0;
        }
    }

    restore_std(tmp[0], tmp[1]);
    close_pipe(tmp[0], tmp[1]);
    close_pipe(in[0], in[1]);

    int status;
    waitpid(childPID, &status, 0);

    // Important to close it before reading
    close_pipe(-1, out[1]);

    if (WIFSIGNALED(status)) {
        log_error("CGI::signaled:" + to_string(WTERMSIG(status)));
        return 0;
    } else if (WIFSTOPPED(status)) {
        log_error("CGI::stopped:" + to_string(WSTOPSIG(status)));
        return 0;
    } else if (WIFEXITED(status)) {

        if (WEXITSTATUS(status)) {
            log_error("CGI::exited:" + to_string(WEXITSTATUS(status)));
            return 0;
        }

        int       readBytes = 1;
        const int size      = 300;

        char buf[size];
        while (readBytes > 0) {
            readBytes = read(out[0], buf, size - 1);
            if (readBytes < 0) {
                log_error("CGI::read");
                return 1;
            }
            buf[readBytes] = 0;
            _res += buf;
        }
        // Send response to the client in body
    }
    close_pipe(out[0], -1);
    return 1;
}

static char **
initEnv() {
    char **env = (char **)calloc(19, sizeof(char *));

    for (size_t i = 0; i < 18; i++) {
        env[i] = (char *)calloc(1024, sizeof(char));
    }

    strcpy(env[0], "PATH_INFO=");
    strcpy(env[1], "PATH_TRANSLATED=");
    strcpy(env[2], "REMOTE_HOST=");
    strcpy(env[3], "REMOTE_ADDR=");
    strcpy(env[4], "REMOTE_USER=");
    strcpy(env[5], "REMOTE_IDENT=");
    strcpy(env[6], "AUTH_TYPE=");
    strcpy(env[7], "QUERY_STRING=");
    strcpy(env[8], "REQUEST_METHOD=");
    strcpy(env[9], "SCRIPT_NAME=");
    strcpy(env[10], "CONTENT_LENGTH=");
    strcpy(env[11], "CONTENT_TYPE=");
    strcpy(env[12], "GATEWAY_INTERFACE=");
    strcpy(env[13], "SERVER_NAME=");
    strcpy(env[14], "SERVER_SOFTWARE=");
    strcpy(env[15], "SERVER_PROTOCOL=");
    strcpy(env[16], "SERVER_PORT=");
    strcpy(env[17], "REDIRECT_STATUS=");
    env[18] = NULL;

    return env;
}

char ** const CGI::env = initEnv();

const std::string CGI::compiledExt = ".cgi";

}