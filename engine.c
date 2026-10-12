#include "engine.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 256

int count_words(char *line, char *target) {
  int count = 0;
    int line_length = strlen(target);
    char *position = line;

    if (line_length == 0) {
        return 0;
    }

    while ((position = strstr(position, target)) != NULL) {
        count++;
        position += line_length;
    }

    return count;
}

int search_count(char *filename, char *target) {
    FILE *file = fopen(filename, "r");
    char *line = NULL;
    size_t line_size = 0;
    int count = 0;

    if (file == NULL) {
        return 0;
    }

    while (getline(&line, &line_size, file) != -1) {
        count += count_words(line, target);
    }

    free(line);
    fclose(file);

    return count;
}

struct count_result search_instance(char *filename, char *target) {
    struct count_result res = {0, NULL};

    FILE *file = fopen(filename, "r");
    char *line = NULL;
    size_t line_size = 0;

    if (file == NULL) {
        return res;
    }

    while (getline(&line, &line_size, file) != -1) {
        int count = count_words(line, target);

        char *position = line;
        while (isspace((unsigned char)*position)) {
            position++;
        }
    int line_length = strlen(position);
        while (line_length > 0 &&
               isspace((unsigned char)position[line_length - 1])) {
            position[--line_length] = '\0';
        }

        for (int i = 0; i < count; i++) {
            char **matching_lines = realloc(
                res.instances,
                (res.count + 1) * sizeof(char *)
            );

            if (matching_lines == NULL) {
                break;
            }

            res.instances = matching_lines;
            res.instances[res.count] = strdup(position);
            res.count++;
        }
    }

    free(line);
    fclose(file);

    return res;
}
