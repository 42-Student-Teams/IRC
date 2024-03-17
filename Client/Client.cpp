#include "Client.hpp"

Client::Client(int fd, const std::string& nickname, const std::string& username): _fd(fd), _nickname(nickname), _username(username) {}

Client::~Client(){}

int Client::getFd() const { return _fd; }

std::string Client::getNickname() const { return _nickname; }
std::string Client::getUsername() const { return _username; }

void Client::setNickname(const std::string& nick) { _nickname = nick; }
void Client::setUsername(const std::string& user) { _username = user; }