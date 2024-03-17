/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: inaranjo <inaranjo <inaranjo@student.42    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/03/12 14:59:00 by inaranjo          #+#    #+#             */
/*   Updated: 2024/03/17 09:46:48 by inaranjo         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <string>
#include <cstring> // Pour memset
#include <unistd.h> // Pour close
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <poll.h>
#include <map>
#include "../Client/Client.hpp"
#include "../Channel/Channel.hpp"

class Client;
class Channel;

class Server
{
    private: 
        int _socketFD; // Descripteur de fichier pour le socket d'écoute
        int _port; // Port d'écoute
        std::string _password;
        

        std::vector<struct pollfd> _fds; // Pour la gestion avec poll
        std::map<int, Client*> _users; // Map des clients, clé = descripteur de fichier
        std::map<std::string, Channel*> channels; // Canaux disponibles sur le serveur
        std::map<int, std::pair<std::string, std::string> > _tempUserInfo;

        void initServer(); // Initialisation du serveur
        void acceptNewConnection(); // Accepte une nouvelle connexion
        void handleClient(int fd); // Gère les entrées/sorties d'un client
        void closeClient(int fd); // Ferme la connexion avec un client
       
       
        std::string extractCommandParam(const std::string& command);
        
        void handleClientMessage(int fd, const std::string& message);

        Server();
        Server(const Server& cpy);
        Server & operator=(const Server& rhs);

    public:
        Server(const int &port, const std::string &password); // construction du serveur par port et mot de pass
        ~Server(); // destrucctor par defaut
        
        /*fonction membre*/
        void run(); //  Lance la boucle principale du serveur
};



#endif
