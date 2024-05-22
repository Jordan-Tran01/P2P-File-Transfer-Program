#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <config.h>
#include <peer.h>
#include <pthread.h>
#include <package.h>
#include <sys/stat.h>
#include <chk/pkgchk.h>
#include <tree/merkletree.h>
#include <crypt/sha256.h>
#include <unistd.h>
#include <netinet/in.h>

typedef struct {
    Config *config;
    package_node **head;
    pthread_mutex_t *mutex;
} ThreadData;

//
// PART 2
//
//SUBMISSION 34 FOR INPUTS
void process_res_packet(btide_packet* packet, package_node* package_list) {
    char identifier[1024];
    char hash[64];
    uint32_t file_offset;
    uint16_t data_len;
    char* received_data = malloc(MAX_RES_DATA);

    memcpy(&file_offset, packet->pl.data, sizeof(file_offset));
    memcpy(received_data, packet->pl.data + 4, MAX_RES_DATA);
    memcpy(&data_len, packet->pl.data + 4 + MAX_RES_DATA, sizeof(data_len));
    memcpy(hash, packet->pl.data + 6 + MAX_RES_DATA, sizeof(hash));
    memcpy(identifier, packet->pl.data + 70 + MAX_RES_DATA, sizeof(identifier));

    if (data_len > BUFFER_SIZE) {
        fprintf(stderr, "Error: data_len %u exceeds BUFFER_SIZE %d\n", data_len, BUFFER_SIZE);
        return; // Or handle error more appropriately
    }
    // FETCH 127.0.0.1:9856 5105d1a7ffbde836fe5aaa6704ecc751857561c6073e065f4a9e40654410b613b881a57aca9c0ec4853c5919d2998bc3355dac867137ffc74f3a85357de08b3b089a510c23312f87eb1604c81a895b554ef9f05138268ad65ed629e436d912e50eefe301c274c4a227850694c6dd9b3991975991c0585fa772c785b4c82545ed675b1ecde15310b827fbdbf931e76c5d4a85852667a775facf59a42d605cda91426c782dfc57141e367daab0a0d7a68e8eaf6cd692da6e8a407ea2f42ccc24ab34a917fa9c40adaed1d74cb6873acd5f208378d1411febdbca206e7e5a917e09e9c61975b849325fc703571b1ac89d276ed784c3c1cf31ff9f2e6497e6f731f90c081cbdd7d09c02b2016a8507c39bc98c198c66336cf6ea9fbf9349b1c4c8d44edca42d79a7f8187d800c9bd5fadce1ace400279ce9401cd9cd6693b8d95caf884896f22eb6ed2b6f8c5100ad9f94e1c295898a731503072837a384113a51c2457cef7538a3baae3e5d1d12f4d45969ee6ddd205d407eeac47e1800cd526bb59127e4731b49a2366b4835802d299de2f09d4116c5f672ded1616e19c325007f01d42eae395ac2893eaadb39e5ef55e56b98947cdc79ef31edbb9e47326875de076abd689d27d59b246b3a36c9f3ec1f389e5f7fd96aef504bc75fd18c47966cefb3e22bb9598a9c553a41bdd0466a349e770a3b384b3d883b37ce4ae804238 904fefa128f596dfb144e35faa7b7faefae93cd8d83847fb249d144f97e7ceb9

    printf("file offset: %u\n", file_offset);
    package_node* package = find_package(package_list, identifier);
    if (!package) {
        printf("Unable to request chunk, package is not managed\n");
        free(received_data); 
        return;
    }

    FILE *file = fopen(package->package_path, "r+b"); 
    if (!file) {
        perror("Failed to open file");
        free(received_data); 
        return;
    }

    // Seek to the specified file offset
    if (fseek(file, file_offset, SEEK_SET) != 0) {
        perror("Failed to seek in file");
        fclose(file);
        free(received_data); 
        return;
    }

    // Write the received data to the file
    if (fwrite(received_data, 1, data_len, file) != data_len) {
        perror("Failed to write to file");
    } else {
        printf("Successfully wrote %u bytes to the file at offset %u\n", data_len, file_offset);
    }

    free(received_data); 
    fclose(file);
}

//Processes the fetch command
void fetch_command_handler(const char* command, peer_node* peer_list, package_node* package_list) {
    char ip[INET_ADDRSTRLEN];
    int port;
    char identifier[1025];
    char hash[65];
    uint32_t offset = 0;
    uint32_t data_len = 0;

    // Parse IP, port, identifier, hash, and optionally offset
    int args = sscanf(command, "%[^:]:%d %1024s %64s %u", ip, &port, identifier, hash, &offset);

    if (args <= 3) {
        printf("Missing arguments from command\n");
        return;
    }

    peer_node* peer = find_peer(peer_list, ip, port);
    if (!peer) {
        fprintf(stderr, "Unable to request chunk, peer not in list\n");
        printf("Unable to request chunk, peer not in list\n");
        return;
    }

    package_node* package = find_package(package_list, identifier);
    if (!package) {
        fprintf(stderr, "Unable to request chunk, package is not managed\n");
        printf("Unable to request chunk, package is not managed\n");
        return;
    }
    
    // Load package object and validate hash
    struct bpkg_obj* package_obj = bpkg_load(package->bpkg_path);
    if (!package_obj) {
        fprintf(stderr, "object isn't loaded properly\n");
        printf("object isn't loaded properly\n");
        return;
    }
    struct merkle_tree_node* root = build_merkle_tree(package_obj->chunks, 0, package_obj->nchunks - 1);
    package_obj->merkle_root = root;
    if (find_hash(package_obj->merkle_root, hash) == 0) {
        fprintf(stderr, "Unable to request chunk, chunk hash does not belong to package\n");
        printf("Unable to request chunk, chunk hash does not belong to package\n");
        bpkg_obj_destroy(package_obj);
        return;
    }

    uint32_t file_offset = -1;
    for (size_t i = 0; i < package_obj->nchunks; i++) {
        if (strncmp(package_obj->chunks[i].hash, hash, 64) == 0) {
            file_offset = package_obj->chunks[i].offset;
            break;
        }
    }


    uint32_t chunk_size = package_obj->size /package_obj->nchunks;
    data_len = chunk_size - offset;
    bpkg_obj_destroy(package_obj);
    uint32_t total_offset = file_offset + offset;
    // Send REQ packet
    btide_packet req_packet;
    req_packet.msg_code = PKT_MSG_REQ;
    memcpy(req_packet.pl.data, &total_offset, sizeof(offset));
    memcpy(req_packet.pl.data + 4, &data_len, sizeof(data_len));
    memcpy(req_packet.pl.data + 8, hash, sizeof(hash));
    memcpy(req_packet.pl.data + 72, identifier, sizeof(identifier));
    send_packet(peer->socket_fd, &req_packet);

    int expected_packets = (data_len + MAX_RES_DATA - 1) / MAX_RES_DATA; 
    int packets_received = 0;

    while (packets_received < expected_packets) {
        btide_packet res_packet;
        receive_packet(peer->socket_fd, &res_packet);
        if (res_packet.msg_code != PKT_MSG_RES) {
            printf("Wrong packet type received\n");
            continue; 
        }

        if (res_packet.error > 0) {
            printf("Peer failed to send requested data, error: %d\n", res_packet.error);
            return;
        }

        process_res_packet(&res_packet, package_list);

        packets_received++;
    }
}

void process_add_package(char *command_str, Config *config, package_node **head) {
    struct bpkg_obj* obj = bpkg_load(command_str);
    if (!obj) {
        printf("Unable to parse bpkg file\n");
        return;
    }

    char full_path[1024];
    // Check if the directory ends with a slash
    size_t dir_len = strlen(config->directory);
    if (config->directory[dir_len - 1] == '/') {
        snprintf(full_path, sizeof(full_path), "%s%s", config->directory, obj->filename);
    } else {
        snprintf(full_path, sizeof(full_path), "%s/%s", config->directory, obj->filename);
    }

    // Attempt to open the file
    FILE *file = fopen(full_path, "r");
    if (!file) {
        file = fopen(full_path, "wb");
        if (!file) {
            printf("Failed to create file '%s'\n", full_path);
            bpkg_obj_destroy(obj);
            return;
        }
        fseek(file, obj->size - 1, SEEK_SET);
        fwrite("\0", 1, 1, file);
        fclose(file);

        file = fopen(full_path, "r");
        if (!file) {
            bpkg_obj_destroy(obj);
            return;
        }
    }
    fclose(file);

    struct bpkg_query qry = compare_files(obj, full_path);
    int complete = (qry.len == obj->nchunks) ? 1 : 0;

    add_package_to_list(head, full_path, command_str, complete, obj->ident);
    bpkg_obj_destroy(obj);
    bpkg_query_destroy(&qry);
}

void* command_handler(void* arg) {
    ThreadData *tdata = (ThreadData *)arg;
    char command[4096];
    peer_node *head_peer = NULL;

    while (1) {
        if (fgets(command, sizeof(command), stdin) == NULL) {
            continue;
        }
        command[strcspn(command, "\n")] = 0; 

        if (strcmp(command, "QUIT") == 0) {
            pthread_mutex_lock(tdata->mutex);
            free_package_list(tdata->head);
            pthread_mutex_unlock(tdata->mutex);
            free_peer_list(&head_peer);
            exit(0);
        } else if (strncmp(command, "CONNECT", 7) == 0) {
            char *command_str = command + 8;
            if (*command_str == '\0') {
                printf("Missing address and port argument\n");
                continue;
            }

            char ip[INET_ADDRSTRLEN];  // Enough space to store an IPv4 string
            int port;

            if (sscanf(command_str, "%[^:]:%d", ip, &port) != 2) {
                printf("Missing address and port argument\n");
            }
            int sockfd = connect_to_peer(ip, port);
            if (sockfd != -1) {
                printf("Connection established with peer\n");
                add_peer_to_list(&head_peer, ip, port, sockfd);
            } else {
                printf("Unable to connect to request peer\n");
            }
        } else if (strncmp(command, "DISCONNECT", 10) == 0) {
            char *command_str = command + 11;
            char ip[INET_ADDRSTRLEN];
            int port;

            if (sscanf(command_str, "%[^:]:%d", ip, &port) == 2) {
                if (disconnect_from_peer(&head_peer, ip, port) == 0) {
                    printf("Disconnected from peer\n");
                } else {
                    printf("Unknown peer, not connected\n");
                }
            } else {
                printf("Missing address and port argument\n");
            }
        } else if (strncmp(command, "PEERS", 5) == 0) {
            print_peer_list(head_peer);
        } else if (strncmp(command, "ADDPACKAGE", 10) == 0) {
            char *command_str = command + 11;
            if (*command_str == '\0') {
                printf("Missing file argument\n");
                continue;
            }

            pthread_mutex_lock(tdata->mutex);
            // Process the ADDPACKAGE command within the mutex lock
            process_add_package(command_str, tdata->config, tdata->head);
            pthread_mutex_unlock(tdata->mutex);
        } else if (strcmp(command, "PACKAGES") == 0) {
            print_package_list(*tdata->head);
        } else if (strncmp(command, "REMPACKAGE", 10) == 0) {
            char *command_str = command + 11;
            if (*command_str == '\0') {
                printf("Missing identifier argument, please specify the whole 1024 character or at least 20 characters.\n");
                continue;
            }
            //FETCH 127.0.0.1:9856 3cf007c14ded16ab85d168fcf9d9b20effef7a0b8f89524d72eeaea97832a3193f9aa9528ebd0 2498107bfb98022cc64e4c8bede532ea940b95bd5cb6b63c8d8493cea5251305
            pthread_mutex_lock(tdata->mutex);
            if (remove_package_from_list(tdata->head, command_str) == 1) {
                printf("Package has been removed\n");
            } else {
                printf("Identifier provided does not match managed packages\n");
            }
            pthread_mutex_unlock(tdata->mutex);
        } else if (strncmp(command, "FETCH", 5) == 0) {
            char* command_str = command + 6;
            fetch_command_handler(command_str, head_peer, *tdata->head);
        }
    }
    return NULL;  // To satisfy the compiler, won't actually reach here
}

int main(int argc, char** argv) {
    pthread_t cmd_thread;
    pthread_mutex_t list_mutex;
    pthread_mutex_init(&list_mutex, NULL);

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <config_file>\n", argv[0]);
        return 2;
    }
    //
    Config config;
    int code = parse_config(argv[1], &config);
    if (code != 0) {
        printf("%d\n", code);
        exit(code);
    }

    package_node *head = NULL;
    ThreadData tdata = {&config, &head, &list_mutex};

    if (pthread_create(&cmd_thread, NULL, command_handler, &tdata) != 0) {
        perror("Failed to create the command handler thread");
        return 1;
    }

    init_server(config.port, config.max_peers, &head, &list_mutex);

    pthread_join(cmd_thread, NULL);
    pthread_mutex_destroy(&list_mutex);
    free_package_list(&head);
    return 0;
}