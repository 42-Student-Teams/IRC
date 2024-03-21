#include "Server.hpp"

Server::Server(const int &port, const std::string &password): _port(port), _password(password)
{
    _socketFD = -1;
    _fds.clear();
}

Server::~Server()
{
    if (_socketFD != -1) 
        close(_socketFD);
}

void Server::initServer()
{
    _socketFD= socket(AF_INET, SOCK_STREAM, 0);
    if (_socketFD < 0) 
    {
        std::cerr << "ERROR :Socket could not be created" << std::endl;
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(_port);

    if (bind(_socketFD, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        std::cerr << "ERROR : Bind could not process." << std::endl;
        exit(EXIT_FAILURE);
    }

    if (listen(_socketFD, 5) < 0) 
    {
        std::cerr << "Erreur lors de l'écoute." << std::endl;
        exit(EXIT_FAILURE);
    }
    fcntl(_socketFD, F_SETFL, O_NONBLOCK); // socket non bloquant

    std::cout << "Serveur initialisé sur le port " << _port << std::endl;
}
 
void Server::run()
{
    initServer();

    /* la struct pollfd va permetre de surveiller les events des fd*/
    struct pollfd check;
    check.fd = _socketFD;
    check.events = POLLIN;// Ici, POLLIN indique qu'on surveille les données prêtes à être lues

    _fds.push_back(check);// Ajout de la structure check dans le vecteur _fds

    while (true) 
    {
        int ret = poll(_fds.data(), _fds.size(), -1);

        if (ret < 0) 
        {
            std::cerr << "Error poll." << std::endl;
            break;
        }

       for (size_t i = 0; i < _fds.size(); i++) 
       {
            if (_fds[i].revents & POLLIN) 
            {
                if (_fds[i].fd == _socketFD)
                    acceptNewConnection();
                else 
                    handleClient(_fds[i].fd);
            }
       }
    }
}

void Server::acceptNewConnection()
{
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    int clientSock = accept(_socketFD, (struct sockaddr *)&clientAddr, &clientAddrLen);

    if (clientSock < 0) 
    {
        std::cerr << "Error: new connection denied." << std::endl;
        return;
    }
    fcntl(clientSock, F_SETFL, O_NONBLOCK);

    struct pollfd pfd;
    pfd.fd = clientSock;
    pfd.events = POLLIN;
    _fds.push_back(pfd);

    /* test pour mettre par defaut un client, a supprimer apres mise en place des obligation  d enregistrement !!
    Client* newClient = new Client(clientSock, "defaultNickname", "defaultUsername");
    _users[clientSock] = newClient;*/

    std::cout << "New connection accepted: User FD attribuated => " << clientSock << std::endl;

    const char* welcomeMsg = "Welcome to the IRC server. Please register by sending the NICK and USER commands.\r\n";
    send(clientSock, welcomeMsg, strlen(welcomeMsg), 0);

    // Log du nombre de connexions après acceptation de la nouvelle connexion
    std::cout << "Total Client_Users connected: " << _users.size() + 1 << std::endl; // +1 car _users n'est pas encore mis à jour
}



void Server::handleClient(int fd) {
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    int nbytes = recv(fd, buffer, sizeof(buffer) - 1, 0);
    if (nbytes <= 0) {
        if (nbytes == 0) {
            std::cout << "USER FD=" << fd << " disconnected." << std::endl;
        } else {
            std::cerr << "Error: Reading from client FD=" << fd << " failed." << std::endl;
        }
        closeClient(fd);
    } else {
        buffer[nbytes] = '\0';
        _commandFragments[fd] += std::string(buffer);

        if (_commandFragments[fd].find('\n') != std::string::npos) {
            std::string fullCommand = _commandFragments[fd];
            // Nettoi après avoir détecté une commande complète
            _commandFragments[fd].clear();
            
            // Vérifie si la commande == QUIT avant de traiter d'autres commandes
            if (fullCommand.find("QUIT") != std::string::npos) {
                const char* quitMsg = "Vous quittez le serveur. Au revoir!\r\n";
                send(fd, quitMsg, strlen(quitMsg), 0);
                closeClient(fd);
                return;
            }
            // Traitement d'autres commandes complètes
            handleClientMessage(fd, fullCommand);
        }
    }
}


void Server::closeClient(int fd)
{
    if (_users.find(fd) != _users.end())
    {
        Client* user = _users[fd];

        std::cout << "Client \"" << user->getNickname() << "\" disconnected: FD= " << fd << std::endl;

        // Fermeture de la connexion réseau
        shutdown(fd, SHUT_RDWR);
        close(fd);
        std::cout << "User " << user->getNickname() << " disconnected." << std::endl;

        // Supprime le client des channels, en cours de dev pas encore utilise
        std::map<std::string, Channel*>::iterator it;
        for (it = channels.begin(); it != channels.end(); ++it)
            it->second->removeClient(fd);

        /* Supprime le client de la map des utilisateurs et libère la mémoire
        delete _users[fd];
        _users.erase(fd);*/
        if (_users.find(fd) != _users.end()) 
        {
            delete _users[fd];
            _users.erase(fd);
        }

        std::cout << "Total clients connected: " << _users.size() << std::endl;
    }
}

std::string Server::extractCommandParam(const std::string& command)
{
    // Trouve la position du premier espace
    size_t spacePos = command.find(' ');
    if (spacePos != std::string::npos)
        return command.substr(spacePos + 1);
     else 
        return ""; // Retourner une chaîne vide si aucun espace n'a été trouvé
}


void Server::handleClientMessage(int fd, const std::string& message)
{
    std::string command = message.substr(0, message.find(' '));
    std::string param = extractCommandParam(message);

      if (command != "NICK" && command != "USER" && command != "QUIT") {
        // Vérifie si l'utilisateur est enregistré
        if (_users.find(fd) == _users.end() || (_users[fd]->getNickname().empty() || _users[fd]->getUsername().empty())) {
            const char* errMsg = "You must register first with NICK and USER commands.\r\n";
            send(fd, errMsg, strlen(errMsg), 0);
            return; // Empêchel'exécution des commandes si l'utilisateur n'est pas enregistré
        }
    }

    // Indique si les informations du client ont été mises à jour lors de cette commande
    bool userInfoUpdated = false;

    // Stocke temporairement les informations NICK et USER
    if (command == "NICK")
    {
        _tempUserInfo[fd].first = param;
        userInfoUpdated = true;
    } 
    else if (command == "USER") 
    {
        _tempUserInfo[fd].second = param;
        userInfoUpdated = true;
    }

    // Vérifie si les deux informations NICK et USER sont présentes pour ce client
    if (!_tempUserInfo[fd].first.empty() && !_tempUserInfo[fd].second.empty())
    {
        bool isNewUser = _users.find(fd) == _users.end();
        // Créer ou mettre à jour le client
        if (isNewUser)
            _users[fd] = new Client(fd, _tempUserInfo[fd].first, _tempUserInfo[fd].second);
        else 
        {
            _users[fd]->setNickname(_tempUserInfo[fd].first);
            _users[fd]->setUsername(_tempUserInfo[fd].second);
        }
        // Envoie un message de confirmation d'enregistrement
        std::string registrationConfirmation = isNewUser ? 
            "Vous êtes maintenant pleinement enregistré sur le serveur avec le NICK : " + _tempUserInfo[fd].first + " et le USER : " + _tempUserInfo[fd].second + ". Bienvenue !\r\n" : 
            "Votre NICK et USER ont été mis à jour avec succès. NICK : " + _tempUserInfo[fd].first + ", USER : " + _tempUserInfo[fd].second + ".\r\n";
        send(fd, registrationConfirmation.c_str(), registrationConfirmation.length(), 0);

        // Nettoye les informations temporaires pour ce client
        _tempUserInfo.erase(fd);
    } 
    else if (userInfoUpdated) 
    {
        // Si seulement une partie des informations update, informe l'utilisateur
        std::string partConfirmation = !param.empty() ? 
            "Votre " + command + " est maintenant " + param + ".\r\n" : 
            "Commande reçue mais incomplète.\r\n";
        send(fd, partConfirmation.c_str(), partConfirmation.length(), 0);
    }
}

