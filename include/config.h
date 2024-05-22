typedef struct {
    char directory[256];
    int max_peers;
    unsigned short port;
} Config;

int parse_config(const char *filename, Config *config);