#include "engine.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>
#include <omp.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 256
#define THREAD_COUNT 4

struct worker_args {
    char *filename;
    char *target;
    long start;
    long end;
    int save_lines;
    int partial_count;
    char **matching_lines;
};

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

void search_worker(struct worker_args *worker) {
    FILE *file = fopen(worker->filename, "r");

    if (file == NULL) {
        return;
    }
    char *line = NULL;
    size_t line_size = 0;
    worker->partial_count = 0;
    worker->matching_lines = NULL;

    if (worker->start > 0) {
        fseek(file, worker->start - 1, SEEK_SET);

        if (fgetc(file) != '\n') {
            getline(&line, &line_size, file);
        }
    }
    long file_position = ftell(file);

    while (file_position < worker->end &&
       getline(&line, &line_size, file) != -1) {

    file_position = ftell(file);

        int count = count_words(line, worker->target);

        if (worker->save_lines && count > 0) {
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
                    worker->matching_lines,
                    (worker->partial_count + 1) * sizeof(char *)
                );
                if (matching_lines == NULL) {
                    break;
                }
                worker->matching_lines = matching_lines;
                worker->matching_lines[worker->partial_count] =
                    strdup(position);

                worker->partial_count++;
            }
        } else {
            worker->partial_count += count;
        }
    }
    free(line);
    fclose(file);
}
struct count_result run_search(char *filename, char *target,
                               int save_lines) {
    struct count_result res = {0, NULL};

    if (strlen(target) == 0) {
        return res;
    }
    struct stat file_info;

    if (stat(filename, &file_info) != 0) {
        return res;
    }
    long file_size = file_info.st_size;
    long chunk_size = file_size / THREAD_COUNT;

    struct worker_args workers[THREAD_COUNT];

    #pragma omp parallel for num_threads(THREAD_COUNT)
    for (int i = 0; i < THREAD_COUNT; i++) {
        workers[i].filename = filename;
        workers[i].target = target;
        workers[i].start = i * chunk_size;
        workers[i].end = (i == THREAD_COUNT - 1)
                         ? file_size : (i + 1) * chunk_size;
        workers[i].save_lines = save_lines;
        workers[i].partial_count = 0;
        workers[i].matching_lines = NULL;

        search_worker(&workers[i]);
    }

    for (int i = 0; i < THREAD_COUNT; i++) {
        res.count += workers[i].partial_count;
    }
    if (save_lines && res.count > 0) {
        res.instances = malloc(res.count * sizeof(char *));
        int position = 0;

        for (int i = 0; i < THREAD_COUNT; i++) {
            for (int j = 0; j < workers[i].partial_count; j++) {
                res.instances[position++] =
                    workers[i].matching_lines[j];
            }
            free(workers[i].matching_lines);
        }
    }
    return res;
}

int search_count(char *filename, char *target) {
    struct count_result res = run_search(filename, target, 0);
    return res.count;
   
}


struct count_result search_instance(char *filename,char *target){
    return run_search(filename, target, 1);
    
}