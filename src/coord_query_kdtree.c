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
struct node {
  // 0 -> X 1-> Y
  int axis;
  struct node *left;
  struct node *right;
  const struct record *record;
};

struct data {
  struct node *noarr;
  int n;
};

int complat(const void *a, const void *b) {
  struct record x = *(const struct record *)a;
  struct record y = *(const struct record *)b;

  if (x.lat < y.lat)
    return -1;
  if (x.lat > y.lat)
    return 1;
  return 0;
}

int complon(const void *a, const void *b) {
  struct record x = *(const struct record *)a;
  struct record y = *(const struct record *)b;

  if (x.lon < y.lon)
    return -1;
  if (x.lon > y.lon)
    return 1;
  return 0;
}

struct node *mk_noarr(struct record *rs, int n, int depth, struct node *noarr,
                      int itt) {
  struct node *no = &noarr[itt]; // reserving space for node
  no->axis = depth % 2;          // depth of node
  if (no->axis == 0) {           // sorting
    qsort(rs, n, sizeof(struct record), complon);
  } else {
    qsort(rs, n, sizeof(struct record), complat);
  }
  // base case
  if (n == 1) {
    no->record = &rs[0];
    no->left = NULL;
    no->right = NULL;
    return no;
  }
  // Other base case
  if (n == 2) {
    no->record = &rs[0];
    no->left = NULL;
    no->right = mk_noarr(&rs[1], 1, depth + 1, noarr, itt + 1);
    return no;
  }
  int median = n / 2;
  no->record = &rs[median];
  no->right =
      mk_noarr(&rs[median + 1], n - median - 1, depth + 1, noarr, itt + 1);
  // since last call n - median - 1 elements have been added to noarr
  no->left = mk_noarr(&rs[0], median, depth + 1, noarr, itt + n - median);
  return no;
}

struct data *mk(struct record *rs, int n) {
  struct node *noarr = malloc(sizeof(struct node) * n);
  mk_noarr(rs, n, 0, noarr, 0);

  struct data *nd = malloc(sizeof(struct data));
  (*nd).noarr = noarr;
  (*nd).n = n;
  return nd;
}

void free_data(struct data *data) {
  free(data->noarr);
  free(data);
}

struct node *rec_lookup(struct node *no, double lon, double lat,
                        struct node *closest) {
  struct node *new_closest = closest;
  if (no == NULL) {
    return NULL;
  }
  if (closest == NULL) {
    new_closest = no;
  }
  double distself = sqrt(pow((no->record->lon - lon), 2.0) +
                         pow((no->record->lat - lat), 2.0));
  double distclosest = sqrt(pow((new_closest->record->lon - lon), 2.0) +
                            pow((new_closest->record->lat - lat), 2.0));
  double diff;
  if (no->axis == 0) {
    diff = no->record->lon - lon;
  } else {
    diff = no->record->lat - lat;
  }

  if (distself < distclosest) {
    new_closest = no;
  }

  if (diff >= 0 && distclosest > fabs(diff)) {
    struct node *left = rec_lookup(no->left, lon, lat, new_closest);
    if (left == NULL) {
      return new_closest;
    }
    double distleft = sqrt(pow((left->record->lon - lon), 2.0) +
                           pow((left->record->lat - lat), 2.0));
    if (distleft < distclosest) {
      new_closest = left;
    }
  } else if (diff <= 0 && distclosest > fabs(diff)) {
    rec_lookup(no->right, lon, lat, new_closest);
    struct node *right = rec_lookup(no->right, lon, lat, new_closest);
    if (right == NULL) {
      return new_closest;
    }
    double distright = sqrt(pow((right->record->lon - lon), 2.0) +
                            pow((right->record->lat - lat), 2.0));
    if (distright < distclosest) {
      new_closest = right;
    }
  }
  return new_closest;
}

const struct record *lookup(struct data *data, double lon, double lat) {
  return rec_lookup(data->noarr, lon, lat, NULL)->record;
}

int main(int argc, char **argv) {
  return coord_query_loop(argc, argv, (mk_index_fn)mk, (free_index_fn)free_data,
                          (lookup_fn)lookup);
}
