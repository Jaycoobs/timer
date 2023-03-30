#include <stdio.h>
#include <errno.h>

#include "category.h"
#include "util.h"

/**
 * Initializes the given vec and reads lines from the file at the given
 * path into it.
 *
 * If the file cannot be opened, the vec is not initialized and false is
 * returned.
 *
 * Otherwise, true is returned.
 */
bool names_load(vec_vec_char_t_t* names, char* path) {
    FILE* f = fopen(path, "r");

    if (!f) {
        fprintf(stderr, "Failed to open names file: %s\n", path);
        perror(NULL);
        return false;
    }

    vec_vec_char_t_init(names);

    vec_char_t current_name;
    vec_char_init(&current_name);

    while (read_line(&current_name, f)) {
        if (current_name.length < 2)
            continue;

        vec_vec_char_t_push(names, current_name);
        vec_char_init(&current_name);
    }

    fclose(f);
    return true;
}

void names_delete(vec_vec_char_t_t* names) {
    for (size_t i = 0; i < names->length; i++)
        vec_char_delete(&names->data[i]);
    vec_vec_char_t_delete(names);
}

/**
 * Initializes the given vec and reads values in the third column of the
 * given file into it.
 *
 * Returns true if the file either does not exist or was successfully
 * read. Returns false if the file could not be opened for some reason
 * other than it doesn't exist or the contents do not have three columns.
 */
bool attempts_load(vec_size_t_t* attempts, char* path) {
    vec_size_t_init(attempts);
    FILE* f = fopen(path, "r");

    if (!f) {
        if (errno == ENOENT) {
            return true;
        } else {
            fprintf(stderr, "Failed to attempts file.");
            perror(NULL);
            return false;
        }
    }

    size_t line_num = 1;

    vec_char_t line;
    vec_char_init(&line);

    while (read_line(&line, f)) {
        char* saveptr;
        char* token;

        token = strtok_r(line.data, " \t", &saveptr);
        for (size_t i = 0; i < 2; i++) {
            token = strtok_r(NULL, " \t", &saveptr);
            if (!token) {
                fprintf(stderr, "Failed to read attempts file %s at line %d\n", path, line_num);
                fprintf(stderr, "line must have split name, reset count, attempt count\n");
                return false;
            }
        }

        size_t count = atol(token);
        vec_size_t_push(attempts, count);

        line.length = 0;
        line_num++;
    }

    vec_char_delete(&line);

    fclose(f);
    return true;
}

/**
 * Write the given vecs to the given file.
 *
 * The attempts vec must be length one greater than length of the names vec.
 *
 * The names are written in the first column, the difference between
 * consecutive entries is written in the second column, and the entries
 * themselves are written in the third column.
 *
 * Writes a final row with name 'completed' and '0' in the difference
 * column.
 *
 * Returns false if the file could not be opened for writing and true
 * otherwise.
 */
bool attempts_write(vec_size_t_t* attempts, vec_vec_char_t_t* names, char* path) {
    FILE* f = fopen(path, "w");

    if (!f) {
        fprintf(stderr, "Failed to open file for writing: %s\n", path);
        perror(NULL);
        return false;
    }

    size_t buffer_lengths = 15;
    char duration_buffer[buffer_lengths];
    char time_buffer[buffer_lengths];

    for (size_t i = 0; i < names->length; i++)
        fprintf(f, "%-20s%6d%6d\n",
                names->data[i].data,
                attempts->data[i]-attempts->data[i+1],
                attempts->data[i]);
    fprintf(f, "%-20s%6ld%6ld\n", "completed", 0, attempts->data[attempts->length-1]);

    fclose(f);
    return true;
}

void category_delete(category_t* category) {
    run_delete(&category->pb);
}

bool category_load(category_t* category, char* path) {
    category->name = path;

    char* names_path = path_join(path, "splits");
    if (!names_load(&category->names, names_path)) {
        free(names_path);
        return false;
    }
    free(names_path);

    char* pb_path = path_join(path, "pb");
    if (!run_load(&category->pb, pb_path, false)) {
        free(pb_path);
        names_delete(&category->names);
        return false;
    }
    free(pb_path);

    char* golds_path = path_join(path, "golds");
    if (!run_load(&category->golds, golds_path, true)) {
        free(golds_path);
        names_delete(&category->names);
        run_delete(&category->pb);
        return false;
    }
    free(golds_path);

    char* attempts_path = path_join(path, "attempts");
    if (!attempts_load(&category->attempts, attempts_path)) {
        free(attempts_path);
        names_delete(&category->names);
        run_delete(&category->pb);
        run_delete(&category->golds);
        return false;
    }
    free(attempts_path);

    if (!category->pb.present) {
        run_empty(&category->pb, category->names.length);
        category->pb.present = false;
    }

    if (!category->golds.present) {
        run_empty(&category->golds, category->names.length);
    }

    if (category->pb.splits.length != category->names.length) {
        fprintf(stderr, "The number of splits in your pb doesn't match the number of splits in this category.\n");
        category_delete(category);
        return false;
    }

    if (category->golds.splits.length != category->names.length) {
        fprintf(stderr, "The number of splits in your golds doesn't match the number of splits in this category.\n");
        category_delete(category);
        return false;
    }

    if (category->attempts.length != category->names.length + 1) {
        fprintf(stderr, "The number of splits in your attempts doesn't match the number of splits in this category.\n");
        category_delete(category);
        return false;
    }

    return true;
}

void category_write(category_t* category, char* path) {
    if (category->pb.present) {
        char* pb_path = path_join(path, "pb");
        run_write(&category->pb, &category->names, pb_path, false);
        free(pb_path);
    }

    char* golds_path = path_join(path, "golds");
    run_write(&category->golds, &category->names, golds_path, true);
    free(golds_path);

    char* attempts_path = path_join(path, "attempts");
    attempts_write(&category->attempts, &category->names, attempts_path);
    free(attempts_path);
}


