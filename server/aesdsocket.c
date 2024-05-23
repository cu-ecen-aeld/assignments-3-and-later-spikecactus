#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>

#define DATA_FILE_PATH "/var/tmp/aesdsocketdata"

int server_fd;

void sig_handler(int signo) {
    if (signo == SIGINT || signo == SIGTERM) {
        syslog(LOG_INFO, "Caught signal, exiting");
        close(server_fd);
        closelog();
        remove(DATA_FILE_PATH);
        exit(0);
    }
}

int main(int argc, char* argv[]) {

    // Daemon mode
    int is_daemon = 0;
    if ((argc > 1) && (strcmp(argv[1], "-d") == 0)) {
        is_daemon = 1;
    }

    openlog(NULL, LOG_PID|LOG_NDELAY, LOG_USER);
    if (!is_daemon) {
        syslog(LOG_INFO, "Starting aesdsocket server");
    }
    else {
        syslog(LOG_INFO, "Starting aesdsocket server in daemon mode");
    }

    // Register signal handler
    if (signal(SIGINT, sig_handler) == SIG_ERR) {
        syslog(LOG_ERR, "Cannot catch SIGINT");
        closelog();
        return 1;
    }

    if (signal(SIGTERM, sig_handler) == SIG_ERR) {
        syslog(LOG_ERR, "Cannot catch SIGTERM");
        closelog();
        return 1;
    }

    // Get address info
    struct addrinfo hints;
    struct addrinfo *result;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len;

    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int s = getaddrinfo(NULL, "9000", &hints, &result);
    if (s != 0) {
        syslog(LOG_ERR, "getaddrinfo: %s", gai_strerror(s));
        freeaddrinfo(result);
        closelog();
        return 1;
    }

    // Create socket
    server_fd = socket(result->ai_family, result->ai_socktype, 0);
    if (server_fd == -1) {
        syslog(LOG_ERR, "Error creating socket");
        freeaddrinfo(result);
        closelog();
        close(server_fd);
        return 1;
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
        syslog(LOG_ERR, "Error setting socket options");
        freeaddrinfo(result);
        closelog();
        close(server_fd);
        return 1;
    }

    // Daemon mode - fork
    if (is_daemon) {
        pid_t pid = fork();

        if (pid == -1) {
            syslog(LOG_ERR, "Error forking");
            freeaddrinfo(result);
            closelog();
            close(server_fd);
            return 1;
        }
        else if (pid > 0) {
            closelog();
            close(server_fd);
            exit(0);
        }
    }

    // Bind socket
    if (bind(server_fd, result->ai_addr, result->ai_addrlen) == -1) {
        syslog(LOG_ERR, "Error binding socket");
        freeaddrinfo(result);
        closelog();
        close(server_fd);
        return 1;
    }
    freeaddrinfo(result);

    // Listen
    if (listen(server_fd, 10) == -1) {
        syslog(LOG_ERR, "Error listening on socket");
        closelog();
        close(server_fd);
        return 1;
    }

    int new_fd;
    char *ip_addr;
    FILE *data_file;
    while (1) {
        client_addr_len = sizeof(client_addr);
        new_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (new_fd == -1) {
            syslog(LOG_ERR, "Error accepting connection");
            closelog();
            close(server_fd);
            return 1;
        }
        
        ip_addr = inet_ntoa(client_addr.sin_addr);
        syslog(LOG_INFO, "Connection from %s", ip_addr);

        // Read data from client
        char buffer[1024];
        if ((data_file = fopen(DATA_FILE_PATH, "a")) == NULL) {
            syslog(LOG_ERR, "Error opening file");
            closelog();
            close(server_fd);
            return 1;
        }

        while (1) {
            int bytes_read = recv(new_fd, buffer, sizeof(buffer), 0);
            if (bytes_read == -1) {
                syslog(LOG_ERR, "Error reading from socket");
                break;
            }
            fwrite(buffer, 1, bytes_read, data_file);

            if (memchr(buffer, '\n', bytes_read) != NULL) { // End of message indicated by newline
                break;
            }
        }
        fclose(data_file);

        // Send response to client
        char response[] = "ACK\n";

        if ((data_file = fopen(DATA_FILE_PATH, "r")) == NULL) {
            syslog(LOG_ERR, "Error opening file");
            closelog();
            close(server_fd);
            return 1;
        }

        while (1) { // Send all the data in the file to the client
            int bytes_read = fread(buffer, 1, sizeof(buffer), data_file);
            if (bytes_read == 0) { // EOF
                break;
            }
            if (send(new_fd, buffer, bytes_read, 0) == -1) {
                syslog(LOG_ERR, "Error sending to socket");
                break;
            }
        }
    }

    closelog();
    syslog(LOG_USER, "losed connection from %s", ip_addr);
    close(server_fd);
    close(new_fd);
    remove(DATA_FILE_PATH);
    
    return 0;
}