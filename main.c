#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>

#include "magiskboot_arm64-v8a.h"

int debug_mode = 0;
#define DEBUG_LOG(fmt, ...) if (debug_mode) fprintf(stderr, "[DEBUG] " fmt "\n", ##__VA_ARGS__)

// Write magiskboot binary
void write_magiskboot(const char *path) {
    DEBUG_LOG("Writing magiskboot binary to %s", path);
    int fd = open(path, O_CREAT | O_WRONLY, 0755);
    ssize_t written = write(fd, magiskboot, magiskboot_len);
    if (written != magiskboot_len) {
        DEBUG_LOG("ERROR: Failed to write all data: %ld of %u bytes", 
                 written, magiskboot_len);
        fprintf(stderr, "Error: Failed to write magiskboot binary\n");
        close(fd);
        exit(1);
    }

    close(fd);
    DEBUG_LOG("Successfully wrote magiskboot binary");
}

/**
 * Check if file exists and has content
 * To determine if magiskboot unpacked boot.img successfully
 */
int file_exists_and_not_empty(const char* path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return st.st_size > 0;
    }
    return 0;
}

/**
 * Extract data from magiskboot output
 * argument: KERNEL_FMT      [gzip]
 * return: "gzip"
 */
char* extract_bracketed_value(const char* line) {
    static char result[256];
    const char* start = strchr(line, '[');
    result[0] = '\0';

    if (!line) return result;
    if (start) {
        start++;
        const char* end = strchr(start, ']');
        if (end) {
            size_t len = end - start;
            if (len < sizeof(result)) {
                strncpy(result, start, len);
                result[len] = '\0';
            }
        }
    }

    return result;
}

/**
 * Find string in file
 * To get string from magiskboot unpack output log
 */
char* find_in_file(const char* filename, const char* search_str) {
    static char line[1024];
    FILE* file = fopen(filename, "r");
    if (!file) return "";

    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, search_str)) {
            fclose(file);
            return line;
        }
    }

    fclose(file);
    return "";
}

// Find Linux version string in kernel file
char* find_linux_version(const char* filename) {
    DEBUG_LOG("Searching Linux version in: %s", filename);
    static char result[1024] = {0};
    static char longest_match[1024] = {0};
    FILE* file = fopen(filename, "rb");
    if (!file) {
        DEBUG_LOG("Failed to open file");
        return "";
    }

    DEBUG_LOG("File opened successfully");
    char buffer[8192];
    const char* pattern = "Linux version ";
    int pattern_len = strlen(pattern);
    size_t bytes_read;
    size_t longest_len = 0;

    longest_match[0] = '\0';

    // Read file in chunks
    while ((bytes_read = fread(buffer, 1, sizeof(buffer) - 1, file)) > 0) {
        buffer[bytes_read] = '\0';

        // Look for "Linux version" in this chunk
        for (size_t i = 0; i < bytes_read; i++) {
            // Skip binary data
            if (!isprint((unsigned char)buffer[i]) && !isspace((unsigned char)buffer[i])) continue;

            // Check for pattern match
            if (i + pattern_len <= bytes_read && strncmp(&buffer[i], pattern, pattern_len) == 0) {
                size_t j = i;
                size_t result_len = 0;

                while (j < bytes_read && buffer[j] != '\n' && buffer[j] != '\r' && 
                       result_len < sizeof(result) - 1) {
                    if (isprint((unsigned char)buffer[j]) || isspace((unsigned char)buffer[j])) {
                        result[result_len++] = buffer[j];
                    }
                    j++;
                }
                result[result_len] = '\0';

                // Keep longest string
                if (result_len > longest_len) {
                    longest_len = result_len;
                    strcpy(longest_match, result);
                }
            }
        }
    }

    fclose(file);

    // Return the longest match found
    DEBUG_LOG("Final longest match: %s", longest_match[0] ? longest_match : "(none)");
    return longest_match[0] ? longest_match : "";
}

// Extract kernel version from Linux version string
char* extract_kernel_version(const char* linux_ver) {
    static char result[256] = {0};

    if (!linux_ver || !*linux_ver) {
        DEBUG_LOG("Empty Linux version string");
        return "";
    }

    DEBUG_LOG("Extracting from: %s", linux_ver);

    // Create a copy we can modify
    char* copy = strdup(linux_ver);
    if (!copy) {
        DEBUG_LOG("Failed to duplicate string");
        return "";
    }

    // Check if it's an android gki kernel
    if (strstr(linux_ver, "android")) {
        // Android gki kernel format
        // Expected format: "Linux version X.Y.Z-androidAB-..." -> "androidAB-X.Y.Z"
        DEBUG_LOG("GKI kernel detected");

        char version[256] = {0};
        char android_ver[256] = {0};

        // Parse to extract the version parts
        if (sscanf(linux_ver, "Linux version %255[0-9.]-%255[^-]", version, android_ver) == 2) {
            if (strncmp(android_ver, "android", 7) == 0) {
                snprintf(result, sizeof(result), "%s-%s", android_ver, version);
                free(copy);
                return result;
            }
        }
    } else {
        // Non-gki kernel
        DEBUG_LOG("Non GKI kernel detected");

        char* saveptr = NULL;
        char* token = strtok_r(copy, " ", &saveptr);

        if (token) token = strtok_r(NULL, " ", &saveptr); // Skip "Linux"
        if (token) token = strtok_r(NULL, " ", &saveptr); // Skip "version"
        if (token) {
            char* dash = strchr(token, '-');
            if (dash) {
                // Found a dash, only copy up to that point
                size_t len = dash - token;
                if (len < sizeof(result) - 1) {
                    strncpy(result, token, len);
                    result[len] = '\0';
                } else {
                    strncpy(result, token, sizeof(result) - 1);
                    result[sizeof(result) - 1] = '\0';
                }
            } else {
                // No dash found, use the whole token
                strncpy(result, token, sizeof(result) - 1);
                result[sizeof(result) - 1] = '\0';
            }

            free(copy);
            return result;
        }
    }

    // Fallback to standard extraction
    char* saveptr = NULL;
    char* token = strtok_r(copy, " ", &saveptr);

    if (token) token = strtok_r(NULL, " ", &saveptr); // Skip "Linux"
    if (token) token = strtok_r(NULL, " ", &saveptr); // Skip "version"
    if (token) {
        strncpy(result, token, sizeof(result) - 1);
        result[sizeof(result) - 1] = '\0';
    }

    free(copy);
    return result;
}

int main(int argc, char *argv[]) {
    DEBUG_LOG("Starting kmi_info (dynamic version)");

    char img_path[512] = "";
    char cmd[2048];
    char compression[64] = "";
    char security_patch[256] = "unknown";
    char cwd[512];
    char mb_path[1024];
    int ret;

    // Enable debug log when first arg is debug
    if (argc > 1 && strcmp(argv[1], "debug") == 0) {
        debug_mode = 1;
        DEBUG_LOG("Debug mode enabled");

        argc--;
        argv++;
    }

    // Get boot image path
    if (argc > 1) {
        DEBUG_LOG("Using provided boot image path: %s", argv[1]);
        snprintf(img_path, sizeof(img_path), "%s", argv[1]);
    } else {
        char buf[256] = "";
        FILE *fp = popen("getprop ro.boot.slot_suffix", "r");
        if (fp) {
            if (fgets(buf, sizeof(buf), fp)) {
                buf[strcspn(buf, "\n")] = 0;
                DEBUG_LOG("Slot suffix: %s", buf);
            }
            pclose(fp);
        }

        snprintf(img_path, sizeof(img_path), "/dev/block/by-name/boot%s", buf);
        DEBUG_LOG("Using default boot image path: %s", img_path);
    }

    // Get current working directory
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd() error");
        return 1;
    }

    // Construct magiskboot path
    snprintf(mb_path, sizeof(mb_path), "%s/magiskboot", cwd);

    // Clean old files
    DEBUG_LOG("Cleaning old files");
    unlink(mb_path);
    unlink("kernel");
    unlink("info");

    // Write magiskboot binary
    write_magiskboot(mb_path);
    chmod(mb_path, 0755);

    // Unpack boot image
    DEBUG_LOG("Unpacking boot image");
    snprintf(cmd, sizeof(cmd), "%s unpack \"%s\" > info 2>&1", mb_path, img_path);
    ret = system(cmd);
    if (ret != 0) {
        DEBUG_LOG("Command failed with return code %d: %s", ret, cmd);
    }

    // Check if kernel exists
    DEBUG_LOG("Checking if kernel file exists");
    if (!file_exists_and_not_empty("kernel")) {
        DEBUG_LOG("Kernel file not found or empty");
        fprintf(stderr, "Error: Invalid boot image\n");
        goto cleanup;
    }

    // Decompress kernel
    DEBUG_LOG("Decompressing kernel");
    snprintf(cmd, sizeof(cmd), "%s decompress kernel 2>/dev/null", mb_path);
    ret = system(cmd);
    if (ret != 0) {
        DEBUG_LOG("Kernel decompression failed with return code %d", ret);
    }

    // Get kernel version
    DEBUG_LOG("Extracting kernel version");
    char* linux_ver = find_linux_version("kernel");
    char* kernel_ver = extract_kernel_version(linux_ver);
    if (!kernel_ver || !*kernel_ver) {
        DEBUG_LOG("Using default kernel version (unknown)");
        kernel_ver = "unknown";
    } else {
        DEBUG_LOG("Kernel version: %s", kernel_ver);
    }

    // Get security patch level
    DEBUG_LOG("Getting security patch level");
    char* security_path_line = find_in_file("info", "OS_PATCH_LEVEL");
    DEBUG_LOG("Security patch line: %s", security_path_line ? security_path_line : "NULL");
    
    // Copy the security patch value to a separate buffer
    char* temp = extract_bracketed_value(security_path_line);
    DEBUG_LOG("Extracted security patch value: '%s'", temp);
    if (temp && temp[0]) {
        strncpy(security_patch, temp, sizeof(security_patch) - 1);
        security_patch[sizeof(security_patch) - 1] = '\0';
    }
    DEBUG_LOG("Final security patch: %s", security_patch);
    
    // Get compression format
    DEBUG_LOG("Getting compression format");
    char* compression_line = find_in_file("info", "KERNEL_FMT");
    char comp_format[32] = "";
    temp = extract_bracketed_value(compression_line);
    DEBUG_LOG("Compression format raw: %s", temp);
    
    // Copy compression format to a separate buffer
    if (temp && temp[0]) {
        strncpy(comp_format, temp, sizeof(comp_format) - 1);
        comp_format[sizeof(comp_format) - 1] = '\0';
    }

    // Determine compression string
    if (strcmp(comp_format, "raw") == 0) {
        DEBUG_LOG("Raw format, using empty string");
        strcpy(compression, "");
    } else if (strcmp(comp_format, "gzip") == 0) {
        DEBUG_LOG("gzip format");
        strcpy(compression, "-gz");
    } else if (strcmp(comp_format, "lz4") == 0) {
        DEBUG_LOG("lz4 format");
        strcpy(compression, "-lz4");
    } else if (comp_format[0]) {
        DEBUG_LOG("%s format", comp_format);
        printf("Warning: kernel compression format is %s, please use AnyKernel3.zip\n", comp_format);
        snprintf(compression, sizeof(compression), "-%s", comp_format);
    } else {
        DEBUG_LOG("Empty compression format, using empty string");
        strcpy(compression, "");
    }
    DEBUG_LOG("Compression string: %s", compression);

    printf("KMI: %s_%s-boot%s\n", kernel_ver, security_patch, compression);

cleanup:
    // Cleanup
    DEBUG_LOG("Cleaning up");
    snprintf(cmd, sizeof(cmd), "%s cleanup 2>/dev/null", mb_path);
    ret = system(cmd);
    if (ret != 0) {
        DEBUG_LOG("Cleanup command failed with return code %d", ret);
    }
    
    ret = system("rm -f info");
    if (ret != 0) {
        DEBUG_LOG("Failed to remove info file, return code %d", ret);
    }
    unlink(mb_path);

    DEBUG_LOG("Done");
    return 0;
}
