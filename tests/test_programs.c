#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "../src/record.h"

#define SMALL_DATA "../10records.tsv"
#define FULL_DATA "../20000records.tsv"

static void fail(const char *message) {
  fprintf(stderr, "Test failure: %s\n", message);
  exit(EXIT_FAILURE);
}

static void check(int condition, const char *message) {
  if (!condition) {
    fail(message);
  }
}

static char *read_file(FILE *file) {
  size_t length = 0;
  size_t capacity = 4096;
  char *contents = malloc(capacity);
  int character;

  check(contents != NULL, "could not allocate command output");
  while ((character = fgetc(file)) != EOF) {
    if (length + 1 == capacity) {
      capacity *= 2;
      contents = realloc(contents, capacity);
      check(contents != NULL, "could not grow command output");
    }
    contents[length++] = (char)character;
  }
  contents[length] = '\0';
  return contents;
}

static char *run_program(const char *command, const char *input) {
  char input_name[] = "/tmp/compsys-test-input-XXXXXX";
  char output_name[] = "/tmp/compsys-test-output-XXXXXX";
  int input_fd = mkstemp(input_name);
  int output_fd = mkstemp(output_name);
  char shell_command[512];
  FILE *input_file;
  FILE *output_file;
  char *output;
  int exit_code;

  check(input_fd >= 0 && output_fd >= 0, "could not create temporary files");
  if (input != NULL) {
    input_file = fdopen(input_fd, "w");
    check(input_file != NULL, "could not open temporary input");
    fputs(input, input_file);
    fclose(input_file);
  } else {
    close(input_fd);
  }
  close(output_fd);

  if (input != NULL) {
    snprintf(shell_command, sizeof(shell_command), "%s < %s > %s 2>/dev/null",
             command, input_name, output_name);
  } else {
    snprintf(shell_command, sizeof(shell_command), "%s > %s 2>/dev/null",
             command, output_name);
  }
  exit_code = system(shell_command);
  check(WIFEXITED(exit_code) && WEXITSTATUS(exit_code) == 0, command);

  output_file = fopen(output_name, "r");
  check(output_file != NULL, "could not open temporary output");
  output = read_file(output_file);
  fclose(output_file);
  unlink(input_name);
  unlink(output_name);
  return output;
}

static void test_record_reader(void) {
  int count;
  struct record *records = read_records(SMALL_DATA, &count);

  check(records != NULL && count == 10, "record reader returned the wrong count");
  free_records(records, count);
}

static void test_id_queries(void) {
  const char *programs[] = {"./id_query_naive " SMALL_DATA,
                            "./id_query_indexed " SMALL_DATA,
                            "./id_query_binsort " SMALL_DATA};
  int count;
  struct record *records = read_records(SMALL_DATA, &count);
  char queries[128];

  check(records != NULL, "could not read ID test data");
  snprintf(queries, sizeof(queries), "%lld\n%lld\n-1\n",
           (long long)records[0].osm_id, (long long)records[count - 1].osm_id);
  for (size_t i = 0; i < sizeof(programs) / sizeof(programs[0]); i++) {
    char *output = run_program(programs[i], queries);
    char expected[128];
    snprintf(expected, sizeof(expected), "%lld: %s ",
             (long long)records[0].osm_id, records[0].name);
    check(strstr(output, expected) != NULL, "ID query missed the first record");
    snprintf(expected, sizeof(expected), "%lld: %s ",
             (long long)records[count - 1].osm_id, records[count - 1].name);
    check(strstr(output, expected) != NULL, "ID query missed the last record");
    check(strstr(output, "-1: not found") != NULL, "ID query found a missing ID");
    free(output);
  }
  free_records(records, count);
}

static const struct record *nearest(const struct record *records, int count,
                                    double lon, double lat) {
  const struct record *best = NULL;
  double best_distance = HUGE_VAL;
  for (int i = 0; i < count; i++) {
    double dx = records[i].lon - lon;
    double dy = records[i].lat - lat;
    double distance = dx * dx + dy * dy;
    if (isfinite(distance) && distance < best_distance) {
      best = &records[i];
      best_distance = distance;
    }
  }
  return best;
}

static void test_coordinate_queries(void) {
  const char *programs[] = {"./coord_query_naive " SMALL_DATA,
                            "./coord_query_kdtree " SMALL_DATA};
  int count;
  struct record *records = read_records(SMALL_DATA, &count);
  const double points[][2] = {{1.8753098, 46.7995347}, {0.0, 0.0},
                              {-100.0, 40.0}};
  char queries[256] = "";

  check(records != NULL, "could not read coordinate test data");
  for (size_t i = 0; i < sizeof(points) / sizeof(points[0]); i++) {
    char query[64];
    snprintf(query, sizeof(query), "%.17g %.17g\n", points[i][0], points[i][1]);
    strncat(queries, query, sizeof(queries) - strlen(queries) - 1);
  }
  for (size_t i = 0; i < sizeof(programs) / sizeof(programs[0]); i++) {
    char *output = run_program(programs[i], queries);
    const char *cursor = output;
    for (size_t point = 0; point < sizeof(points) / sizeof(points[0]); point++) {
      const struct record *expected = nearest(records, count, points[point][0], points[point][1]);
      char match[256];
      snprintf(match, sizeof(match), ": %s (", expected->name);
      cursor = strstr(cursor, match);
      check(cursor != NULL, "coordinate query returned the wrong nearest record");
      cursor += strlen(match);
    }
    free(output);
  }
  free_records(records, count);
}

static void test_random_ids(void) {
  int count;
  struct record *records = read_records(SMALL_DATA, &count);
  char *output = run_program("./random_ids " SMALL_DATA " | head -n 10", NULL);
  char *line = output;
  int lines = 0;

  check(records != NULL, "could not read random ID test data");
  while (*line != '\0') {
    char *end;
    int64_t id = strtoll(line, &end, 10);
    int found = 0;
    check(end != line, "random_ids produced a non-numeric value");
    for (int i = 0; i < count; i++) {
      found |= records[i].osm_id == id;
    }
    check(found, "random_ids produced an unknown ID");
    lines++;
    line = strchr(end, '\n');
    if (line == NULL) {
      break;
    }
    line++;
  }
  check(lines == 10, "random_ids produced too few IDs");
  free(output);
  free_records(records, count);
}

static double milliseconds(void) {
  struct timespec time;
  clock_gettime(CLOCK_MONOTONIC, &time);
  return time.tv_sec * 1000.0 + time.tv_nsec / 1000000.0;
}

static void benchmark(void) {
  const char *id_programs[] = {"./id_query_naive " FULL_DATA,
                               "./id_query_indexed " FULL_DATA,
                               "./id_query_binsort " FULL_DATA};
  const char *coord_programs[] = {"./coord_query_naive " FULL_DATA,
                                  "./coord_query_kdtree " FULL_DATA};
  int count;
  struct record *records = read_records(FULL_DATA, &count);
  size_t capacity = (size_t)count * 64;
  char *id_queries = malloc(capacity);
  char *coord_queries = malloc(capacity);

  check(records != NULL && id_queries != NULL && coord_queries != NULL,
        "could not prepare benchmark data");
  id_queries[0] = coord_queries[0] = '\0';
  for (int i = 0; i < count; i++) {
    char query[128];
    snprintf(query, sizeof(query), "%lld\n", (long long)records[i].osm_id);
    strncat(id_queries, query, capacity - strlen(id_queries) - 1);
    snprintf(query, sizeof(query), "%.17g %.17g\n", records[i].lon, records[i].lat);
    strncat(coord_queries, query, capacity - strlen(coord_queries) - 1);
  }

  printf("program,records,queries,elapsed_ms\n");
  for (size_t i = 0; i < sizeof(id_programs) / sizeof(id_programs[0]); i++) {
    double start = milliseconds();
    char *output = run_program(id_programs[i], id_queries);
      const char *name_end = strchr(id_programs[i] + 2, ' ');
      printf("%.*s,%d,%d,%.2f\n", (int)(name_end - (id_programs[i] + 2)),
        id_programs[i] + 2, count, count, milliseconds() - start);
    free(output);
  }
  for (size_t i = 0; i < sizeof(coord_programs) / sizeof(coord_programs[0]); i++) {
    double start = milliseconds();
    char *output = run_program(coord_programs[i], coord_queries);
      const char *name_end = strchr(coord_programs[i] + 2, ' ');
      printf("%.*s,%d,%d,%.2f\n", (int)(name_end - (coord_programs[i] + 2)),
        coord_programs[i] + 2, count, count, milliseconds() - start);
    free(output);
  }
  free(id_queries);
  free(coord_queries);
  free_records(records, count);
}

int main(int argc, char **argv) {
  if (argc == 2 && strcmp(argv[1], "--benchmark") == 0) {
    benchmark();
    return 0;
  }
  check(argc == 1, "usage: test_programs [--benchmark]");
  test_record_reader();
  test_id_queries();
  test_coordinate_queries();
  test_random_ids();
  printf("All tests passed\n");
  return 0;
}