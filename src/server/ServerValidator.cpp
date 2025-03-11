#include "server/ServerValidator.hpp"
#include <iostream>
#include <cstring>

ServerValidator::ServerValidator()
{
    fillErrorMap();
}

ServerValidator::~ServerValidator() {};

/**
 * @brief fills the map with known errors and there return values for easy lookup later
 * 
 */
void ServerValidator::fillErrorMap()
{
    error_map_ = {
        {EAGAIN, 		{"No pending connection, just return.", -1}},
        {ECONNABORTED,	{"Connection aborted before accept(), Ignoring", -1}},
        {EMFILE,		{"Too many open files. Consider increasing file descriptor limits.", -1}},
        {ENFILE,		{"Too many open files. Consider increasing file descriptor limits.", -1}},
        {ENOMEM,		{"System out of memory. Cannot accept new connections.", -2}},
        {EBADF,			{"Critical error: Invalid socket or file descpritor. Restarting server may be required.", -2}},
        {EINVAL,		{"Critical error: Invalid socket or arguments. Restarting server may be required.", -2}},
        {ENOSPC,		{"Reached system limit for epoll FDs. Consider increasing limit.", -2}},
        {EEXIST,		{"FD already in epoll. Modifying instead.", -1}},
        {ENOENT,		{"FD does not exist. Retrying with EPOLL_CTL_ADD", -1}},
        {EADDRINUSE, 	{"Port is already in use. Try another port or wait.", -1}},
        {EADDRNOTAVAIL,	{"The requested address is not available on this machine.", -2}},
        {EAFNOSUPPORT,	{"Address family not supported.", -2}},
        {ENOTSOCK, 		{"The file descriptor is not a socket.", -2}},
        {EACCES,		{"Permission denied. Try using a different port or running as root.", -2}},
        {EPERM,			{"Operation not permitted on socket or FD.", -2}},
        {ENOBUFS,		{"Insufficient resources to complete the operation.", -2}},
        {EFAULT,		{"Invalid memory address provided for sockaddr or read.", -2}},
        {EOPNOTSUPP,	{"Operation not supported on this socket type. Check socket configuration.", -2}},
        {EWOULDBLOCK,	{"Resource temporarily unavailable (try again).", -1}},
        {ECONNRESET,	{"Connection reset by peer. The connection was forcibly closed.", -1}},
        {EINTR,			{"Operation interrupted by signal, try again.", -1}},
        {EIO,			{"I/O error occurred during read operation.", -2}},
        {ESHUTDOWN,		{"Socket has been shut down; no further reading possible.", -1}}
    };
}

const std::unordered_map<int, s_ErrorInfo>& ServerValidator::getErrorMap() const
{
    return error_map_;
}

/**
 * @brief checks the error number against the error map and spit outs its return code
 * 
 * @param err the error code for lookup
 * @return the return value
 */
int ServerValidator::checkErrno(int err)
{
    std::unordered_map<int, s_ErrorInfo>::iterator it = error_map_.find(err);
    if (it != error_map_.end())
    {
        std::cerr << it->second.message << std::endl;
        if (err == EEXIST)
            return 1;
        else if (err == ENOENT)
            return 2;
        else if (err == 0) {};
        return it->second.return_value;
    }
    std::cerr << "Unknown error:" << strerror(err) << std::endl;
    return -1;
}