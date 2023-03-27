#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <regex.h>
#include <time.h>

#define PORT 8080
#define BUFFER_SIZE 2048
#define FILENAME "sent_lines.txt"

void send_to_server(int sock, char *message) {
    if (send(sock, message, strlen(message), 0) == -1) {
        perror("send failed");
        exit(EXIT_FAILURE);
    }
}

void save_sent_line(char *line) {
    FILE *fp = fopen(FILENAME, "a");
    fprintf(fp, "%s\n", line);
    fclose(fp);
}

int is_line_sent(char *line) {
    FILE *fp = fopen(FILENAME, "r");
    char buffer[BUFFER_SIZE];
    while (fgets(buffer, BUFFER_SIZE, fp) != NULL) {
        if (strcmp(buffer, line) == 0) {
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}


int create_socket_and_connect_to_srv() {
    struct sockaddr_in address;
    int sock = 0, valread;
    struct timeval timeout;
    timeout.tv_sec = 60;
    timeout.tv_usec = 0;

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&address, '0', sizeof(address));

    // Filling address struct
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) <= 0) {
        perror("invalid address");
        exit(EXIT_FAILURE);
    }

    // Connect to server
    if (connect(sock, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("connection failed");
        exit(EXIT_FAILURE);
    }

    // Set timeout for read
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    return sock;
}

int main(int argc, char const *argv[]) {
    while (1)
    {
        int sock = create_socket_and_connect_to_srv();

        // Open audit log file
        //FILE *fp = fopen("/var/log/audit/audit.log", "r");
        FILE *fp = fopen("/home/hunter/audit.log", "r");
        if (fp == NULL) {
            perror("unable to open audit.log file");
            exit(EXIT_FAILURE);
        }

        // Read audit log file line by line
        char line[BUFFER_SIZE];
        char buffer[BUFFER_SIZE] = {0};
        regex_t regex;
        regcomp(&regex, "type=(AVC|USER_AVC|SELINUX_ERR|USER_SELINUX_ERR)", REG_EXTENDED);
        while (fgets(line, BUFFER_SIZE, fp) != NULL) {
            // Check if line matches the regex pattern
            if (regexec(&regex, line, 0, NULL, 0) == 0) {
                // Check if line was already sent
                if (!is_line_sent(line)) {
                    // Send line to server
                    send_to_server(sock, line);
                    // Save line as sent
                    save_sent_line(line);
                }
            }
        }

        regfree(&regex);
        fclose(fp);
        close(sock);
        sleep(60);
    }
    return 0;
}
