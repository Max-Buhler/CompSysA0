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

struct naive_data {
  struct record *rs;
  int n;
};

struct naive_data *mk_naive(struct record *rs, int n) {
  struct naive_data *nd = malloc(sizeof(struct naive_data));
  (*nd).rs = rs;
  (*nd).n = n;
  return nd;
}

void free_naive(struct naive_data *data) { free(data); }

const struct record *lookup_naive(struct naive_data *data, double lon,
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
  return coord_query_loop(argc, argv, (mk_index_fn)mk_naive,
                          (free_index_fn)free_naive, (lookup_fn)lookup_naive);
}
