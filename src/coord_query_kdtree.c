#include <assert.h>
#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "coord_query.h"
#include "record.h"
/*
struct node {

  const struct record *record;
};

struct data {
  struct record *rs;
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

struct data *mk(struct record *rs, int n) {
  struct data *nd = malloc(sizeof(struct data));
  (*nd).rs = rs;
  (*nd).n = n;
  return nd;
}


void free_data(struct data *data) { free(data); }



const struct record *lookup(struct data *data, double lon,
                                  double lat) {
  double cdist = 1.79769e+308;
  struct record *crecord = NULL;
  for (int i = 0; i < data->n; i++) {
    double dist = sqrt(pow((data->rs[i].lon - lon), 2.0) +
                       pow((data->rs[i].lat - lat), 2.0));
    if (dist < cdist) {
      cdist = dist;
      crecord = &data->rs[i];
    }
  }
  return crecord;
}

int main(int argc, char **argv) {
  return coord_query_loop(argc, argv, (mk_index_fn)mk,
                          (free_index_fn)free_data, (lookup_fn)lookup);
}
*/
