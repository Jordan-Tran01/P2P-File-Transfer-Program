#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <chk/pkgchk.h>
#include <signal.h>
#include <peer.h>
#include <package.h>
#include <sys/time.h>
#include <sys/types.h>

void add_peer_to_list(peer_node **head, const char *ip, int port, int sock_fd) {
    peer_node *new_node = (peer_node *)malloc(sizeof(peer_node));
    if (new_node == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }

    new_node->ip = strdup(ip);
    new_node->port = port;
    new_node->socket_fd = sock_fd;

    new_node->next = NULL;

    if (*head == NULL) {
        *head = new_node;
    } else {
        peer_node *current = *head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_node;
    }
}

void print_peer_list(peer_node *head) {
    peer_node *current = head;
    if (head == NULL) {
        printf("Not connected to any peers\n");
    } else {
        printf("Connected to:\n\n");

        int index = 1;
        while (current != NULL) {
            printf("%d. %s:%d\n", index++, current->ip, current->port);
            current = current->next;
        }
    }
}

void free_peer_list(peer_node **head) {
    peer_node *current = *head;
    while (current != NULL) {
        peer_node *next = current->next;
        free(current->ip);
        free(current);
        current = next;
    }
    *head = NULL; // Reset the head of the list
}

// Function to find a peer in the list
peer_node* find_peer(peer_node *head, const char *ip, int port) {
    while (head != NULL) {
        if (strcmp(head->ip, ip) == 0 && head->port == port) {
            return head;
        }
        head = head->next;
    }
    return NULL;  // No match found
}

void add_package_to_list(package_node **head, const char *path, const char *bpkg, int complete, const char *ident) {
    package_node *new_node = (package_node *)malloc(sizeof(package_node));
    if (new_node == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }

    new_node->package_path = strdup(path);
    new_node->bpkg_path = strdup(bpkg);
    new_node->complete = complete;
    new_node->ident = strdup(ident);
    new_node->next = *head;

    new_node->next = NULL;

    if (*head == NULL) {
        *head = new_node;
    } else {
        package_node *current = *head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_node;
    }
}

int remove_package_from_list(package_node **head, const char *ident) {
    if (*head == NULL) {
        fprintf(stderr, "List is empty\n");
        return 0;
    }

    package_node *current = *head;
    package_node *previous = NULL;

    if (strlen(ident) < MIN_IDENT) {
        return 0;
    }

    while (current != NULL) {
        if (strncmp(current->ident, ident, MIN_IDENT) == 0) {
            if (previous == NULL) {
                *head = current->next;
            } else {
                previous->next = current->next;
            }

            free(current->package_path);
            free(current->bpkg_path);
            free(current->ident);
            free(current);
            return 1; 
        }
        previous = current;
        current = current->next;
    }


    return 0;
}

void print_package_list(package_node *head) {
    package_node *current = head;
    if (head == NULL) {
        printf("No packages managed\n");
    }

    int index = 1;
    while (current != NULL) {
        if (current->complete == 1) {
            printf("%d. %.32s, %s : COMPLETED\n", index++, current->ident, current->package_path);
        } else {
            printf("%d. %.32s, %s : INCOMPLETE\n", index++, current->ident, current->package_path);
        }
        current = current->next;
    }
}

void free_package_list(package_node **head) {
    package_node *current = *head;
    while (current != NULL) {
        package_node *next = current->next;
        free(current->package_path);
        free(current->bpkg_path);
        free(current->ident);
        free(current);
        current = next;
    }
    *head = NULL; // Reset the head of the list
}

// Function to find a package in the list
package_node* find_package(package_node *head, const char *identifier) {

    while (head != NULL) {
        if (strncmp(head->ident, identifier, strlen(identifier)) == 0) {
            return head;
        }
        head = head->next;
    }
    
    return NULL;  // No match found
}


void sigint_handler(int signum) {
    printf("\nReceived SIGINT (Ctrl + C). Quitting...\n");
    exit(signum); // Exit the program with the signal number
}

int init_server(int port, int max_peers, package_node **package_list, pthread_mutex_t *mutex) {
    int server_fd, new_socket, client_socket[max_peers], max_sd, sd;
    int activity, i, valread;
    struct sockaddr_in address;
    char buffer[BUFFER_SIZE]; 

    fd_set readfds;

    //initialise all client_socket[] to 0 so not checked
    for (i = 0; i < max_peers; i++) {
        client_socket[i] = 0;
    }

    // Create a master socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // allow quit via ctrl-C
    if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        perror("Error registering signal handler for SIGINT");
        return 1;
    }

    // Set master socket to allow multiple connections
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // Bind the socket to localhost port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Try to specify maximum of 3 pending connections for the master socket
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // Accept the incoming connection
    socklen_t addrlen = sizeof(address);
    while (1) {
        FD_ZERO(&readfds);

        FD_SET(server_fd, &readfds);
        max_sd = server_fd;

        for (i = 0; i < max_peers; i++) {
            sd = client_socket[i];

            if (sd > 0)
                FD_SET(sd, &readfds);

            if (sd > max_sd)
                max_sd = sd;
        }

        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR)) {
            printf("select error");
        }

        if (FD_ISSET(server_fd, &readfds)) {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) >= 0) {
                // Send ACP to newly connected client
                btide_packet acp_packet;
                acp_packet.msg_code = PKT_MSG_ACP;
                acp_packet.error = 0;
                memset(acp_packet.pl.data, 0, DATA_MAX);
                send_packet(new_socket, &acp_packet);

                // Add new socket to array of sockets
                for (i = 0; i < max_peers; i++) {
                    if (client_socket[i] == 0) {
                        client_socket[i] = new_socket;
                        break;
                    }
                }
            }

            // Add new socket to array of sockets
            for (i = 0; i < max_peers; i++) {
                // If position is empty
                if (client_socket[i] == 0) {
                    client_socket[i] = new_socket;
                    break;
                }
            }
        }

        for (i = 0; i < max_peers; i++) {
            sd = client_socket[i];
            if (FD_ISSET(sd, &readfds)) { 
                if ((valread = recv(sd, buffer, BUFFER_SIZE - 1, 0)) == 0) {
                    // Clean closure from client
                    getpeername(sd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
                    printf("Host disconnected, ip %s, port %d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                    close(sd);
                    client_socket[i] = 0;
                } else if (valread > 0) {
                    buffer[valread] = '\0';
                    btide_packet* received_packet = (btide_packet*)buffer;

                    if (received_packet->msg_code == PKT_MSG_DSN) {
                        // Handle DSN packet
                        printf("Received DSN from %s, port %d. Closing connection.\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                        close(sd);
                        client_socket[i] = 0;
                        continue;  // Skip further processing since connection is closed
                    }
                    
                    if (received_packet->msg_code == PKT_MSG_ACP) {
                        send_ack(sd);
                    }

                    if (received_packet->msg_code == PKT_MSG_REQ) {
                        pthread_mutex_lock(mutex);  // Lock the mutex before accessing package_list

                        char requested_identifier[1024] = {0}; // Initializes all to zero
                        char requested_hash[64] = {0}; // Initializes all to zero
                        uint32_t requested_offset;
                        uint32_t requested_data_len;
                        
                        memcpy(&requested_offset, received_packet->pl.data, sizeof(requested_offset));
                        memcpy(&requested_data_len, received_packet->pl.data + 4, sizeof(requested_data_len));
                        memcpy(requested_hash, received_packet->pl.data + 8, 64);
                        memcpy(requested_identifier, received_packet->pl.data + 72, 1024);

                        package_node* package = find_package(*package_list, requested_identifier);

                        if (!package) {
                            btide_packet res_packet;
                            res_packet.msg_code = PKT_MSG_RES;
                            res_packet.error = 1;
                            send_packet(sd, &res_packet);
                            return 0;
                        }

                        struct bpkg_obj* obj = bpkg_load(package->bpkg_path);
                        if (!obj) {
                            btide_packet res_packet;
                            res_packet.msg_code = PKT_MSG_RES;
                            res_packet.error = 1;
                            send_packet(sd, &res_packet);
                            return 0;
                        }

                        const char* filename = package->package_path;

                        bpkg_obj_destroy(obj);

                        FILE *file = fopen(filename, "rb");
                        if (!file) {
                            btide_packet res_packet;
                            res_packet.msg_code = PKT_MSG_RES;
                            res_packet.error = 1;
                            send_packet(sd, &res_packet);
                            return 0;
                        }

                        // Seek to the start offset within the file
                        if (fseek(file, requested_offset, SEEK_SET) != 0) {
                            perror("Failed to seek in file");
                            fclose(file);
                            return EXIT_FAILURE;
                        }

                        // Allocate buffer to hold the data temporarily
                        char *buffer = malloc(MAX_RES_DATA);
                        if (!buffer) {
                            printf("Failed to allocate memory for the buffer\n");
                            fclose(file);
                            pthread_mutex_unlock(mutex);
                            return EXIT_FAILURE;
                        }

                        // Calculate number of packets needed
                        int num_packets = (requested_data_len + MAX_RES_DATA - 1) / MAX_RES_DATA;
                        int remaining_data = requested_data_len;
                        uint32_t offset_increment = 0;

                        uint32_t file_chunk_offset;

                        for (int j = 0; j < num_packets; j++) {
                            uint16_t current_packet_size = (remaining_data > MAX_RES_DATA) ? MAX_RES_DATA : remaining_data;

                            // Seek to the current offset within the file
                            if (fseek(file, requested_offset + offset_increment, SEEK_SET) != 0) {
                                perror("Failed to seek in file");
                                free(buffer);
                                fclose(file);
                                pthread_mutex_unlock(mutex);
                                return EXIT_FAILURE;
                            }

                            // Read the current chunk of data from the file
                            size_t bytes_read = fread(buffer, 1, current_packet_size, file);
                            if (bytes_read < current_packet_size) {
                                if (feof(file)) {
                                    printf("End of file reached before reading all the data\n");
                                }
                                if (ferror(file)) {
                                    printf("Error reading file\n");
                                }
                                free(buffer);
                                fclose(file);
                                pthread_mutex_unlock(mutex);
                                return EXIT_FAILURE;
                            }

                            file_chunk_offset = offset_increment + requested_offset;
                            // Prepare and send the response packet
                            btide_packet res_packet;
                            res_packet.msg_code = PKT_MSG_RES;
                            res_packet.error = 0;  // Assuming no error
                            memcpy(res_packet.pl.data, &file_chunk_offset, sizeof(uint32_t));
                            memcpy(res_packet.pl.data + 4, buffer, MAX_RES_DATA);
                            memcpy(res_packet.pl.data + 4 + MAX_RES_DATA, &current_packet_size, sizeof(uint16_t));
                            memcpy(res_packet.pl.data + 6 + MAX_RES_DATA, requested_hash, sizeof(requested_hash));
                            memcpy(res_packet.pl.data + 70 + MAX_RES_DATA, requested_identifier, sizeof(requested_identifier));

                            send_packet(sd, &res_packet);

                            remaining_data -= current_packet_size;
                            offset_increment += current_packet_size;
                        }

                        free(buffer);
                        fclose(file);
                        pthread_mutex_unlock(mutex);
                    }
                } else {
                    perror("recv failed");
                    close(sd);
                    client_socket[i] = 0;
                }
            }
        }
    }

    return 0;
}

int connect_to_peer(const char* ip, int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid address / Address not supported");
        close(sockfd);
        return -1;
    }

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sockfd);
        return -1;
    }

    // Wait for ACP
    btide_packet acp_packet;
    receive_packet(sockfd, &acp_packet);
    if (acp_packet.msg_code != PKT_MSG_ACP) {
        printf("Expected ACP but received: %d\n", acp_packet.msg_code);
        close(sockfd);
        return -1;
    }

    // Send ACK in response to ACP
    btide_packet ack_packet;
    ack_packet.msg_code = PKT_MSG_ACK;
    ack_packet.error = 0;  // No error
    memset(ack_packet.pl.data, 0, DATA_MAX);  // Clear payload
    send_packet(sockfd, &ack_packet);

    return sockfd; // Connection is established and acknowledged
}

int disconnect_from_peer(peer_node **head, const char *ip, int port) {
    peer_node *current = *head, *prev = NULL;

    while (current != NULL) {
        if (strcmp(current->ip, ip) == 0 && current->port == port) {
            // Close the socket associated with this peer
            close(current->socket_fd);

            // Remove the peer from the linked list
            if (prev == NULL) {
                *head = current->next; // Head is the one to be removed
            } else {
                prev->next = current->next;
            }
            free(current->ip);
            free(current);
            return 0;  // Success
        }
        prev = current;
        current = current->next;
    }

    return -1;  // Peer not found
}