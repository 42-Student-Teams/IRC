#ifndef CLIENT_HPP 
#define CLIENT_HPP

#include <iostream>
#include <string>

class Channel;

class Client 
{
    private:
        int _fd; // Descripteur de fichier du socket client
        std::string _nickname; // Pseudo du user
        std::string _username; // Nom d'utilisateur du user


    public:
        Client(int fd, const std::string& nickname, const std::string& username);
        ~Client();

        int         getFd() const;
        std::string getNickname() const;
        std::string getUsername() const;

        void setNickname(const std::string& nick);
        void setUsername(const std::string& user);

    


};

#endif



