#include <stdio.h>
#include <stdlib.h>

// helper definition to select maximum between two variables
#define MAX(X, Y) (X > Y ? X : Y)

// definitions for the station queue
// initial queue dimension, this will increase at powers of 2
#define INIT_STATION_QUEUE_DIM 32

// declaration of colors for Breadth-First Search
enum color_t {
  WHITE = 0,
  GREY = 1,
};

/*
A vehicle is an AVL tree node, to manage multiple cars with
the same autonomy we use a counter that keeps track of how
many cars with given autonomy are in a specific vehicle tree.
I choose this structure because is a BST, so insertion,
deletion, research, min, max, predecessor and successor
are all O(h), with h the height of the tree. In this case of
AVL tree we have that are all O(log(n)).
The operations of our interest for vehicles are:
- aggiungi-stazione e aggiungi-auto: insertion O(log(n))
  we also need to check if the vehicle we added has an autonomy
  greater than the maximum autonomy currently in station O(1),
  and update leftmost and rightmost reachable stations O(1);
- demolisci-stazione: deletion of all nodes O(n);
- rottama-auto: deletion O(log(n)) and we need to check
  if the vehicle we deleted had the max autonomy, so we check
  if it is still present with a research O(log(n)). This can be
  further improved with a flag set to 0 if we do not remove any
  vehicle, to 1 if we remove the only vehicle with given autonomy and
  2 if we remove a vehicle with num >= 2 O(1);
 */
struct vehicle_t {
  // key
  unsigned int autonomy;
  // number of vehicle with same autonomy
  unsigned short int num;
  // height of node, used to calculate balance vector
  unsigned short int height;

  // pointers to left and right children
  struct vehicle_t *left;
  struct vehicle_t *right;
};

/*
A station is an AVL tree node.
I choose this structure because is a BST, so insertion,
deletion, research, min, max, predecessor and successor
are all O(h), with h the height of the tree. In this case of
AVL tree we have that are all O(log(n)).
The operations of our interest for vehicles are:
- aggiungi-stazione: insertion O(log(n));
- demolisci-stazione: deletion O(log(n));
- pianifica-percorso: we search for begin and end stations
  O(log(n)), then we create an array with all stations between
  the two with distance O(n), leftmost and rightmost reachable
  stations, color, prev_on_path. These are all the information
  we need to do a Breadth First Search O(v) forward and
  O(v^2) backward. This asimmetry is given by the fact that
  going forward we enqueue stations sequentially, while
  backwards for every station we have to check every station
  on the right.
 */
struct station_t {
  // key
  unsigned int distance;
  // height of node, used to calculate balance vector
  unsigned int height;
  // pointers to parent, left and right child
  struct station_t *parent;
  struct station_t *left;
  struct station_t *right;

  // this is the root of an AVL tree for vehicles
  struct vehicle_t *vehicle_parking;
  // max vehicle autonomy among vehicles in vehicle_parking
  unsigned int max_vehicle_autonomy;
  /*
    theoretical reachable stations on the left and right,
    based on distance and max_vehicle_autonomy
   */
  unsigned int leftmost_reachable_station;
  unsigned int rightmost_reachable_station;

  struct station_t *next; // used for breadth-first search
  struct station_t *prev; // used for breadth-first search
};

// Used for BFS
struct station_graph_node_t {
  // these fields are copied from the stations
  unsigned int distance;
  unsigned int leftmost_reachable_station;
  unsigned int rightmost_reachable_station;

  enum color_t color; // used for breadth-first search

  int prev_on_path; // used to print the final path
};

// Queue used for BFS
struct station_queue_t {
  int *station_ref;
  unsigned int head;
  unsigned int tail;
  unsigned int dim;
};

// we initialize with queue = create_station_queue(INIT_STATION_QUEUE_DIM)
struct station_queue_t *create_station_queue(unsigned int dim) {

  // Allocation and initialization of queue
  struct station_queue_t *res;
  res = malloc(sizeof(struct station_queue_t));
  res->station_ref = malloc(sizeof(int) * dim);
  res->head = 0;
  res->tail = 0;
  res->dim = dim;

  return res;
}

// if head and tail are in same position, the queue is empty
char is_empty_station_queue(struct station_queue_t *queue) {

  if (queue->head == queue->tail)
    return 1;

  return 0;
}

int dequeue_station(struct station_queue_t *queue) {

  // if the queue is empty we return a not valid value
  if (is_empty_station_queue(queue))
    return -1;

  // return head element and update queue head
  int res;
  res = queue->station_ref[queue->head];
  queue->head = (queue->head + 1) % queue->dim;

  return res;
}

struct station_queue_t *
deallocate_station_queue(struct station_queue_t *queue) {

  free(queue->station_ref);
  queue->station_ref = NULL;
  free(queue);
  queue = NULL;

  return NULL;
}

struct station_queue_t *enqueue_station(struct station_queue_t *queue,
                                        int station);

struct station_queue_t *
reallocate_station_queue(struct station_queue_t *queue) {

  struct station_queue_t *res;
  res = create_station_queue(queue->dim << 1);

  // if we check for emptyness we get a false positive, we need to extract
  // an element from the queue
  res = enqueue_station(res, queue->station_ref[queue->head]);
  queue->head = (queue->head + 1) % queue->dim;

  // we move all elements to reallocated queue
  while (!is_empty_station_queue(queue))
    res = enqueue_station(res, dequeue_station(queue));

  deallocate_station_queue(queue);

  return res;
}

struct station_queue_t *enqueue_station(struct station_queue_t *queue,
                                        int station) {

  queue->station_ref[queue->tail] = station;
  queue->tail = (queue->tail + 1) % queue->dim;

  // if tail reached head, the array is full and we need to reallocate
  if (queue->tail == queue->head) {
    queue = reallocate_station_queue(queue);
  }

  return queue;
}

// Create a new node with given autonomy
struct vehicle_t *create_vehicle_node(unsigned int autonomy) {
  struct vehicle_t *res;

  res = malloc(sizeof(struct vehicle_t));
  res->autonomy = autonomy;
  res->num = 1;
  res->height = 1; // this will be updated later if necessary
  res->left = NULL;
  res->right = NULL;

  return res;
}

// helper function to get height of node in tree
unsigned int vehicle_height(struct vehicle_t *vehicle) {
  if (vehicle == NULL) {
    return 0;
  }
  return vehicle->height;
}

// function to get balance vector
int get_vehicle_balance(struct vehicle_t *vehicle) {
  return (vehicle_height(vehicle->right) - vehicle_height(vehicle->left));
}

struct vehicle_t *left_rotate_vehicle(struct vehicle_t *vehicle) {

  struct vehicle_t *tmp;
  tmp = vehicle->right;
  vehicle->right = tmp->left;
  tmp->left = vehicle;

  // now we recalculate correct height of moved nodes using subtrees as
  // invariants
  vehicle->height =
      MAX(vehicle_height(vehicle->left), vehicle_height(vehicle->right)) + 1;
  tmp->height = MAX(vehicle_height(tmp->left), vehicle_height(tmp->right)) + 1;

  return tmp;
}

struct vehicle_t *right_rotate_vehicle(struct vehicle_t *vehicle) {

  struct vehicle_t *tmp;
  tmp = vehicle->left;
  vehicle->left = tmp->right;
  tmp->right = vehicle;

  // now we recalculate correct height of moved nodes using subtrees as
  // invariants
  vehicle->height =
      MAX(vehicle_height(vehicle->left), vehicle_height(vehicle->right)) + 1;
  tmp->height = MAX(vehicle_height(tmp->left), vehicle_height(tmp->right)) + 1;

  return tmp;
}

// Add vehicle with given autonomy to specified tree.
struct vehicle_t *add_vehicle(struct vehicle_t *vehicle,
                              unsigned int autonomy) {
  // if we reach the bottom of the tree without finding
  // a node with the key we add the new node here
  if (vehicle == NULL)
    return (create_vehicle_node(autonomy));

  // if we find the key we increment the counter
  if (autonomy == vehicle->autonomy) {
    vehicle->num++;
    return vehicle;
  }

  // if the key is different we try to add the vehicle on the correct side
  if (autonomy < vehicle->autonomy) {
    vehicle->left = add_vehicle(vehicle->left, autonomy);
  } else {
    vehicle->right = add_vehicle(vehicle->right, autonomy);
  }

  // we have to recalculate the height of current node
  vehicle->height =
      MAX(vehicle_height(vehicle->left), vehicle_height(vehicle->right)) + 1;

  // get the balance vector to check if we have lost the AVL property
  int balance = get_vehicle_balance(vehicle);

  // if node is now unbalanced we have now 4 possible cases
  // and we can reconduce two of them to the other two
  // Left Right to Left Left
  // Right Left to Right Right
  if (balance < -1) {
    // Left Right case
    if (autonomy > vehicle->left->autonomy) {
      vehicle->left = left_rotate_vehicle(vehicle->left);
    }
    // Now we are in Left Left case
    return right_rotate_vehicle(vehicle);
  }
  if (balance > 1) {
    // Right Left case
    if (autonomy < vehicle->right->autonomy) {
      vehicle->right = right_rotate_vehicle(vehicle->right);
    }
    // Now we are in Right Right case
    return left_rotate_vehicle(vehicle);
  }

  // if the node is still in balance we return it unmodified

  return vehicle;
}

// the minimum is the node in bottom left of the tree
struct vehicle_t *minimum_vehicle(struct vehicle_t *vehicle) {

  if (vehicle == NULL)
    return NULL;

  while (vehicle->left != NULL) {
    vehicle = vehicle->left;
  }

  return vehicle;
}

// the maximum is the node in bottom right of the tree
struct vehicle_t *maximum_vehicle(struct vehicle_t *vehicle) {

  if (vehicle == NULL)
    return NULL;

  while (vehicle->right != NULL) {
    vehicle = vehicle->right;
  }

  return vehicle;
}

// the flag is 0 if not removed, 1 if removed but still present and 2 if removed
// completely
struct vehicle_t *remove_vehicle(struct vehicle_t *vehicle,
                                 unsigned int autonomy, char *flag) {

  // If the vehicle with given autonomy is not found, do nothing
  if (vehicle == NULL)
    return NULL;

  // If the vehicle with given autonomy has lower autonomy than current,
  // we recursively call the function on the left child, else the right one
  if (autonomy < vehicle->autonomy) {
    vehicle->left = remove_vehicle(vehicle->left, autonomy, flag);
  }
  if (autonomy > vehicle->autonomy) {
    vehicle->right = remove_vehicle(vehicle->right, autonomy, flag);
  }

  // If we find the vehicle we decrease its number if >1, else delete it
  if (autonomy == vehicle->autonomy) {

    if (vehicle->num > 1) {
      *flag = 1;
      vehicle->num--;
      return vehicle;
    }

    // we have to check if the vehicle to remove has 2 or less children
    *flag = 2;
    struct vehicle_t *tmp;

    if (vehicle->left == NULL || vehicle->right == NULL) {
      tmp = vehicle->left ? vehicle->left : vehicle->right;
      free(vehicle);
      vehicle = NULL;
      return tmp;
    }

    // if the vehicle has two children we update its content with the one of its
    // successor which is the minimum in his right subtree
    tmp = minimum_vehicle(vehicle->right);
    vehicle->autonomy = tmp->autonomy;
    vehicle->num = tmp->num;
    // we have to delete now moved successor, so we set his num to 1
    tmp->num = 1;
    char f;
    f = 0;
    vehicle->right = remove_vehicle(vehicle->right, tmp->autonomy, &f);
  }

  // at this point we have removed the node if present. If there was only one
  // node we return NULL
  if (vehicle == NULL)
    return NULL;

  // we have to recalculate current node height
  vehicle->height =
      MAX(vehicle_height(vehicle->left), vehicle_height(vehicle->right)) + 1;

  // now we calculate current node balance
  int balance = get_vehicle_balance(vehicle);

  // if the node is not balanced we have to perform same rotations as in the
  // add_vehicle case
  if (balance < -1) {
    // Left Right case
    if (get_vehicle_balance(vehicle->left) == 1) {
      vehicle->left = left_rotate_vehicle(vehicle->left);
    }
    // Now we are in Left Left case
    return right_rotate_vehicle(vehicle);
  }
  if (balance > 1) {
    // Right Left case
    if (get_vehicle_balance(vehicle->right) == -1) {
      vehicle->right = right_rotate_vehicle(vehicle->right);
    }
    // Now we are in Right Right case
    return left_rotate_vehicle(vehicle);
  }

  // if the node is still in balance we return it unmodified

  return vehicle;
}

void remove_all_vehicles(struct vehicle_t *vehicle) {

  if (vehicle != NULL) {
    remove_all_vehicles(vehicle->left);
    remove_all_vehicles(vehicle->right);

    free(vehicle);
    vehicle = NULL;
  }

  return;
}

struct vehicle_t *find_vehicle(struct vehicle_t *vehicle,
                               unsigned int autonomy) {

  // if we reach the end we return NULL
  if (vehicle == NULL)
    return NULL;

  // if we find the vehicle we return it
  if (autonomy == vehicle->autonomy)
    return vehicle;

  // we go recursively in the correct direction down the BST
  if (autonomy > vehicle->autonomy)
    return find_vehicle(vehicle->right, autonomy);
  if (autonomy < vehicle->autonomy)
    return find_vehicle(vehicle->left, autonomy);

  return NULL;
}

// the minimum is the node in bottom left of the tree
struct station_t *minimum_station(struct station_t *station) {

  if (station == NULL)
    return NULL;

  while (station->left != NULL) {
    station = station->left;
  }

  return station;
}

// the maximum is the node in bottom right of the tree
struct station_t *maximum_station(struct station_t *station) {

  if (station == NULL)
    return NULL;

  while (station->right != NULL) {
    station = station->right;
  }

  return station;
}

// successor and predecessor are used for next and prev pointers,
// then used to create the array for BFS

struct station_t *predecessor_station(struct station_t *station) {

  // if the node has a left child, the predecessor is the max
  // of the left subtree
  if (station->left != NULL)
    return maximum_station(station->left);

  // else we go towards the root and the first left ancestor is the predecessor
  while (station->parent != NULL && station->parent->left == station) {
    station = station->parent;
  }

  return station->parent;
}

struct station_t *successor_station(struct station_t *station) {

  // if the node has a right child, the successor is the min
  // of the right subtree
  if (station->right != NULL)
    return minimum_station(station->right);

  // else we go towards the root and the first right ancestor is the successor
  while (station->parent != NULL && station->parent->right == station) {
    station = station->parent;
  }

  return station->parent;
}

/* struct station_t* station_greater_or_equal_to_distance(struct station_t*
station_tree, unsigned int distance) { if (station_tree == NULL) return NULL;

    struct station_t* tmp,
                    * res;
    res = NULL;
    tmp = station_tree;

    while (tmp != NULL) {
        if (tmp->distance == distance)
            return tmp;
        if (tmp->distance > distance) {
            res = tmp;
            tmp = tmp->left;
        }
        else if (tmp->distance < distance) {
            tmp = tmp->right;
        }
    }

    return res;
} */

// function used to calculate leftmost and rightmost reachable stations
// when we update max_vehicle_autonomy
void update_reachable_stations(struct station_t *station) {

  station->rightmost_reachable_station =
      station->distance + station->max_vehicle_autonomy;
  if (station->max_vehicle_autonomy > station->distance)
    station->leftmost_reachable_station = 0;
  else
    station->leftmost_reachable_station =
        station->distance - station->max_vehicle_autonomy;

  return;
}

// Create a new node with given distance
struct station_t *create_station_node(unsigned int distance) {
  struct station_t *res;

  res = malloc(sizeof(struct station_t));
  res->distance = distance;
  res->height = 1; // this will be updated later if necessary
  res->parent = NULL;
  res->left = NULL;
  res->right = NULL;
  res->vehicle_parking = NULL;
  res->max_vehicle_autonomy = 0;
  update_reachable_stations(res);
  res->next = NULL;
  res->prev = NULL;

  return res;
}

// helper function to get height of node in tree
unsigned int station_height(struct station_t *station) {
  if (station == NULL) {
    return 0;
  }
  return station->height;
}

// function to get balance vector
int get_station_balance(struct station_t *station) {
  return (station_height(station->right) - station_height(station->left));
}

struct station_t *left_rotate_station(struct station_t *station) {

  struct station_t *tmp;
  tmp = station->right;
  station->right = tmp->left;
  if (station->right != NULL)
    station->right->parent = station;
  tmp->left = station;
  station->parent = tmp;

  // now we recalculate correct height of moved nodes using subtrees as
  // invariants
  station->height =
      MAX(station_height(station->left), station_height(station->right)) + 1;
  tmp->height = MAX(station_height(tmp->left), station_height(tmp->right)) + 1;

  // when we return this we have to update the partent
  return tmp;
}

struct station_t *right_rotate_station(struct station_t *station) {

  struct station_t *tmp;
  tmp = station->left;
  station->left = tmp->right;
  if (station->left != NULL)
    station->left->parent = station;
  tmp->right = station;
  station->parent = tmp;

  // now we recalculate correct height of moved nodes using subtrees as
  // invariants
  station->height =
      MAX(station_height(station->left), station_height(station->right)) + 1;
  tmp->height = MAX(station_height(tmp->left), station_height(tmp->right)) + 1;

  // when we return this we have to update the partent
  return tmp;
}

// Add station with given distance to specified tree.
struct station_t *add_station(struct station_t *station, unsigned int distance,
                              struct station_t **station_ref) {
  // if we reach the bottom of the tree without finding
  // a node with the key we add the new node here
  if (station == NULL) {

    *station_ref = (create_station_node(distance));
    return *station_ref;
  }

  // if we find the key we have to return the station but leave station_ref to
  // null
  if (distance == station->distance)
    return station;

  // if the key is different we try to add the station on the correct side
  if (distance < station->distance) {
    station->left = add_station(station->left, distance, station_ref);
    station->left->parent = station;
  } else if (distance > station->distance) {
    station->right = add_station(station->right, distance, station_ref);
    station->right->parent = station;
  }

  // we have to recalculate the height of current node
  station->height =
      MAX(station_height(station->left), station_height(station->right)) + 1;

  // get the balance vector to check if we have lost the AVL property
  int balance = get_station_balance(station);

  // if node is now unbalanced we have now 4 possible cases
  // and we can reconduce two of them to the other two
  // Left Right to Left Left
  // Right Left to Right Right
  if (balance < -1) {
    // Left Right case
    if (distance > station->left->distance) {
      station->left = left_rotate_station(station->left);
      station->left->parent = station;
    }
    // Now we are in Left Left case
    return right_rotate_station(station);
  }
  if (balance > 1) {
    // Right Left case
    if (distance < station->right->distance) {
      station->right = right_rotate_station(station->right);
      station->right->parent = station;
    }
    // Now we are in Right Right case
    return left_rotate_station(station);
  }

  // if the node is still in balance we return it unmodified

  return station;
}

struct station_t *remove_station(struct station_t *station,
                                 unsigned int distance, char *flag) {

  // If the station with given distance is not found, do nothing
  if (station == NULL)
    return NULL;

  if (distance == station->distance) {

    *flag = 1;

    // fixing next prev hole
    if (station->prev != NULL)
      station->prev->next = station->next;
    if (station->next != NULL)
      station->next->prev = station->prev;

    // we have to check if the station to remove has 2 or less children
    struct station_t *tmp;

    if (station->left == NULL || station->right == NULL) {
      tmp = station->left ? station->left : station->right;
      remove_all_vehicles(station->vehicle_parking);
      free(station);
      station = NULL;
      return tmp;
    }

    // if the station has two children we update its content with the one of its
    // successor
    tmp = minimum_station(station->right);
    station->distance = tmp->distance;
    remove_all_vehicles(station->vehicle_parking);
    station->vehicle_parking = tmp->vehicle_parking;
    tmp->vehicle_parking =
        NULL; // this is necessary to avoid removing useful vehicles
    station->max_vehicle_autonomy = tmp->max_vehicle_autonomy;
    station->rightmost_reachable_station = tmp->rightmost_reachable_station;
    station->leftmost_reachable_station = tmp->leftmost_reachable_station;
    station->next = tmp->next;
    if (station->next != NULL)
      station->next->prev = station;
    tmp->next = NULL;
    station->prev = tmp->prev;
    if (station->prev != NULL)
      station->prev->next = station;
    tmp->prev = NULL;
    station->right = remove_station(station->right, tmp->distance, flag);
    if (station->right != NULL)
      station->right->parent = station;
  }

  // If the station with given distance has lower distance than current,
  // we recursively call the function on the left child, else ri
  if (distance < station->distance) {
    station->left = remove_station(station->left, distance, flag);
    if (station->left != NULL)
      station->left->parent = station;
  } else {
    station->right = remove_station(station->right, distance, flag);
    if (station->right != NULL)
      station->right->parent = station;
  }

  // at this point we have removed the node if present. If there was only one
  // node we return NULL
  if (station == NULL)
    return NULL;

  // we have to recalculate current node height
  station->height =
      MAX(station_height(station->left), station_height(station->right)) + 1;

  // now we calculate current node balance
  int balance = get_station_balance(station);

  // if the node is not balanced we have to perform same rotations as in the
  // add_station case
  if (balance < -1) {
    // Left Right case
    if (get_station_balance(station->left) == 1) {
      station->left = left_rotate_station(station->left);
      station->left->parent = station;
    }
    // Now we are in Left Left case
    return right_rotate_station(station);
  }
  if (balance > 1) {
    // Right Left case
    if (get_station_balance(station->right) == -1) {
      station->right = right_rotate_station(station->right);
      station->right->parent = station;
    }
    // Now we are in Right Right case
    return left_rotate_station(station);
  }

  // if the node is still in balance we return it unmodified

  return station;
}

void remove_all_stations(struct station_t *station) {

  if (station != NULL) {
    remove_all_stations(station->left);
    remove_all_stations(station->right);

    remove_all_vehicles(station->vehicle_parking);
    free(station);
    station = NULL;
  }

  return;
}

struct station_t *find_station(struct station_t *station,
                               unsigned int distance) {

  // if we reach the end we return NULL
  if (station == NULL)
    return NULL;

  // if we find the correct station we return it
  if (distance == station->distance)
    return station;

  // else we recursively call the research on left or right subtree
  if (distance > station->distance)
    return find_station(station->right, distance);
  if (distance < station->distance)
    return find_station(station->left, distance);

  return NULL;
}

void add_vehicle_to_station(struct station_t *station, unsigned int autonomy) {

  station->vehicle_parking = add_vehicle(station->vehicle_parking, autonomy);
  // check if we need to update max vehicle height
  if (autonomy > station->max_vehicle_autonomy) {
    station->max_vehicle_autonomy = autonomy;
    update_reachable_stations(station);
  }

  return;
}

void remove_vehicle_from_station(struct station_t *station,
                                 unsigned int autonomy) {

  char flag;
  flag = 0;
  station->vehicle_parking =
      remove_vehicle(station->vehicle_parking, autonomy, &flag);
  // check if we need to update max vehicle height
  if (autonomy == station->max_vehicle_autonomy && flag == 2) {
    struct vehicle_t *tmp;
    tmp = maximum_vehicle(station->vehicle_parking);
    if (tmp == NULL) {
      station->max_vehicle_autonomy = 0;
      update_reachable_stations(station);
    } else {
      station->max_vehicle_autonomy = tmp->autonomy;
      update_reachable_stations(station);
    }
  }

  return;
}

// We can use the stack to print the correct order of stations
void print_route(struct station_graph_node_t *vect, unsigned int station,
                 unsigned int end_station) {

  if (vect[station].prev_on_path == -1) {
    printf("%d\n", vect[station].distance);
    return;
  }

  printf("%d ", vect[station].distance);

  print_route(vect, vect[station].prev_on_path, end_station);

  return;
}

// We can use the stack to print the correct order of stations
void print_route_reverse(struct station_graph_node_t *vect,
                         unsigned int station, unsigned int end_station) {

  if (vect[station].prev_on_path == -1) {
    printf("%d ", vect[station].distance);
    return;
  }

  print_route_reverse(vect, vect[station].prev_on_path, end_station);

  if (station == end_station) {
    printf("%d\n", vect[station].distance);
    return;
  }

  printf("%d ", vect[station].distance);
  return;
}

// function that gives the number of station to allocate an array of perfect
// size
unsigned int number_of_stations_between(struct station_t *begin,
                                        struct station_t *end) {

  unsigned int res = 1;

  struct station_t *curr;

  curr = begin;

  while (curr != end) {
    res++;
    curr = curr->next;
  }

  return res;
}

// function that creates the array used for BFS
void vector_of_stations_between(struct station_graph_node_t *vect,
                                struct station_t *begin,
                                struct station_t *end) {

  unsigned int idx = 0;

  struct station_t *curr;

  curr = begin;

  while (curr != end->next) {
    vect[idx].distance = curr->distance;
    vect[idx].color = WHITE;
    vect[idx].rightmost_reachable_station = curr->rightmost_reachable_station;
    vect[idx].leftmost_reachable_station = curr->leftmost_reachable_station;
    vect[idx].prev_on_path = -1;

    idx++;
    curr = curr->next;
  }

  return;
}

// this is a Breadth-First Search, going forward is optimized, backwards is a
// normal BFS. this asimmetry is given by the request that if two routes have
// the same number of stations we have to select the one with the first
// different station with the lower distance. All edges are calculated at
// runtime using distance of stations and leftmost and rightmost reachable
// stations.
void plan_route(struct station_t *station_tree, struct station_t *begin_station,
                struct station_t *end_station, struct station_queue_t **queue) {

  unsigned int num_stations; // number of stations between begin and end station

  unsigned int curr, tmp, begin, end;

  *queue = create_station_queue(INIT_STATION_QUEUE_DIM);

  // if the start and end stations are the same print the distance and return
  if (begin_station->distance == end_station->distance) {

    printf("%d\n", begin_station->distance);
    *queue = deallocate_station_queue(*queue);
    return;
  }

  // forward case
  if (begin_station->distance < end_station->distance) {

    num_stations = number_of_stations_between(begin_station, end_station);
    struct station_graph_node_t station_vector[num_stations];
    vector_of_stations_between(station_vector, begin_station, end_station);

    end = num_stations - 1;

    station_vector[0].color = GREY;
    *queue = enqueue_station(*queue, 0);
    tmp = 1;
    while (!is_empty_station_queue(*queue)) {
      curr = dequeue_station(*queue);

      while (tmp < num_stations &&
             station_vector[tmp].distance <=
                 station_vector[curr].rightmost_reachable_station) {
        if (tmp == end) {
          station_vector[tmp].prev_on_path = curr;
          print_route_reverse(station_vector, tmp, end);
          *queue = deallocate_station_queue(*queue);
          return;
        }
        station_vector[tmp].prev_on_path = curr;
        *queue = enqueue_station(*queue, tmp);

        tmp = tmp + 1;
      }
    }
  }
  // backward case
  else {

    num_stations = number_of_stations_between(end_station, begin_station);
    struct station_graph_node_t station_vector[num_stations];
    vector_of_stations_between(station_vector, end_station, begin_station);

    begin = num_stations - 1;

    station_vector[0].color = GREY;
    *queue = enqueue_station(*queue, 0);

    while (!is_empty_station_queue(*queue)) {
      curr = dequeue_station(*queue);
      tmp = curr + 1;
      while (tmp != num_stations) {
        if (station_vector[curr].distance <
            station_vector[tmp].leftmost_reachable_station) {
          tmp = tmp + 1;
          continue;
        }
        if (tmp == begin) {
          station_vector[tmp].prev_on_path = curr;
          print_route(station_vector, tmp, begin);
          *queue = deallocate_station_queue(*queue);
          return;
        }
        if (station_vector[tmp].color == WHITE) {
          station_vector[tmp].color = GREY;
          station_vector[tmp].prev_on_path = curr;
          *queue = enqueue_station(*queue, tmp);
        }
        tmp = tmp + 1;
      }
    }
  }

  printf("nessun percorso\n");
  *queue = deallocate_station_queue(*queue);
  return;
}

int main(void) {

  char command[19], flag;
  unsigned int station_distance, end_station_distance, vehicle_autonomy,
      vehicles_number;

  struct station_t *stations = NULL;    // Empty tree for stations
  struct station_t *station;            // the station on which we do operations
  struct station_queue_t *queue = NULL; // Pointer to queue

  // Scan every command until EOF or Ctrl-D in terminal
  while (scanf("%s %d", command, &station_distance) != EOF) {
    /*
     * To avoid the use of strcmp, we can simply check
     * letter in position number 12 of the captured string,
     * which is different for every command:
     * aggiungi-stazione  -> z
     * demolisci-stazione -> a
     * aggiungi-auto      -> o
     * rottama-auto       -> \0 null character
     * pianifica-percorso -> r
     */
    switch (command[12]) {
    // aggiungi-stazione
    case 'z':
      // We check that the station does not already exist
      station = NULL;
      stations = add_station(stations, station_distance, &station);
      stations->parent = NULL;
      if (station != NULL) {

        // fix next prev pointers
        struct station_t *tmp;
        tmp = predecessor_station(station);
        if (tmp != NULL) {
          station->prev = tmp;
          station->next = tmp->next;
          tmp->next = station;
          if (station->next != NULL)
            station->next->prev = station;
        } else {
          tmp = successor_station(station);
          station->next = tmp;
          if (tmp != NULL)
            tmp->prev = station;
        }

        if (scanf("%d", &vehicles_number) == 1) {
          for (int i = 0; i < vehicles_number; i++) {
            if (scanf("%d", &vehicle_autonomy) == 1) {
              add_vehicle_to_station(station, vehicle_autonomy);
            }
          }
        }

        printf("aggiunta\n");
      } else {
        if (scanf("%d", &vehicles_number) == 1) {
          for (int i = 0; i < vehicles_number; i++) {
            if (scanf("%d", &vehicle_autonomy) == 1) {
            }
          }
          printf("non aggiunta\n");
        }
      }

      break;
    // demolisci-stazione
    case 'a':
      // Try to remove station with given distance.
      // If all goes well find_station() returns pointer to station,
      // else returns NULL
      flag = 0;
      stations = remove_station(stations, station_distance, &flag);
      if (stations != NULL)
        stations->parent = NULL;
      if (flag == 1) {

        printf("demolita\n");
      } else {
        printf("non demolita\n");
      }
      break;
    // aggiungi-auto
    case 'o':
      // Check if station with given distance exists.
      // If exists find_station returns pointer to station,
      // else NULL
      if (scanf("%d", &vehicle_autonomy) == 1) {
        station = find_station(stations, station_distance);
        if (station != NULL) {
          add_vehicle_to_station(station, vehicle_autonomy);
          printf("aggiunta\n");
        } else {
          printf("non aggiunta\n");
        }
      }
      break;
    // rottama-auto
    case '\0':
      // Check if the station exists then check
      // if the car has been removed or not
      if (scanf("%d", &vehicle_autonomy) == 1) {
        station = find_station(stations, station_distance);
        if (station != NULL) {
          if (find_vehicle(station->vehicle_parking, vehicle_autonomy) !=
              NULL) {
            remove_vehicle_from_station(station, vehicle_autonomy);
            printf("rottamata\n");
          } else {
            printf("non rottamata\n");
          }
        } else {
          printf("non rottamata\n");
        }
      }
      break;
    // pianifica-percorso
    case 'r':
      if (scanf("%d", &end_station_distance) == 1) {
        struct station_t *begin_station;
        struct station_t *end_station;
        begin_station = find_station(stations, station_distance);
        end_station = find_station(stations, end_station_distance);
        if (begin_station != NULL && end_station != NULL)
          plan_route(stations, begin_station, end_station, &queue);
        else
          printf("nessun percorso\n");
      }
      break;
    }
  }

  remove_all_stations(stations);

  stations = NULL;

  return 0;
}
