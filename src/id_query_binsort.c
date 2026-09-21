#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "id_query.h"
#include "record.h"

struct index_record {
  int64_t osm_id;
  const struct record *record;
};

struct index_data {
  struct index_record *irs;
  int n;
};

int comp(const void *a, const void *b) {
  struct index_record x = *(const struct index_record *)a;
  struct index_record y = *(const struct index_record *)b;

  if (x.osm_id < y.osm_id)
    return -1;
  if (x.osm_id > y.osm_id)
    return 1;
  return 0;
}

struct index_data *mk_indexed(struct record *rs, int n) {
  struct index_record *irs = malloc(sizeof(struct index_record) * n);
  for (int i = 0; i < n; i++) {
    struct index_record ir;
    ir.osm_id = rs[i].osm_id;
    ir.record = &rs[i];
    irs[i] = ir;
  }
  qsort(irs, n, sizeof(struct index_record), comp);

  struct index_data *nd = malloc(sizeof(struct index_data));
  (*nd).irs = irs;
  (*nd).n = n;
  return nd;
}

void free_indexed(struct index_data *data) {
  free(data->irs);
  free(data);
}

const struct record *lookup_indexed(struct index_data *data, int64_t needle) {
  int left = 0;
  int right = data->n - 1;
  int middle = right / 2;
  // binsort algorithm
  while (left >= right) {
    if ((*data).irs[middle].osm_id == needle) {
      return (*data).irs[middle].record;
    }
    if ((*data).irs[middle].osm_id < needle) {
      left = middle;
      middle = (left + right) / 2;
    }
    if ((*data).irs[middle].osm_id > needle) {
      right = middle;
      middle = (left + right) / 2;
    }
  }
  return NULL;
}

int main(int argc, char **argv) {
  return id_query_loop(argc, argv, (mk_index_fn)mk_indexed,
                       (free_index_fn)free_indexed, (lookup_fn)lookup_indexed);
}
