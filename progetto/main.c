#include <stdio.h>
#include <stdlib.h>

#define MAX(X,Y) (X > Y ? X : Y)

// definitions for the station queue
#define INIT_STATION_QUEUE_DIM 32

enum color_t {
    WHITE = 0,
    GREY = 1,
};

// A vehicle is an AVL tree node
struct vehicle_t {
    unsigned int autonomy;
    unsigned short int num; //number of vehicle with this autonomy
    unsigned short int height;

    struct vehicle_t* left;
    struct vehicle_t* right;
};

// A station is an AVL tree node
struct station_t {
    unsigned int distance; //key
    unsigned int height; //used to calculate height vector to keep AVL tree balanced
    struct station_t* parent;
    struct station_t* left;
    struct station_t* right;

    // We do not need a graph, because we can just do a breadth first research
    // enqueueing in a queue all nodes until
    // |exploredstation->distance - currentstation->distance| <= max_autonomy_vehicle->autonomy
    // we have to enqueue only nodes in one direction depending on the requested path:
    // if from 10 to 20, only more distant stations, if from 20 to 10 only nearest stations
    
    struct vehicle_t* vehicle_parking; //this is the root of an AVL tree for vehicles
    unsigned int max_vehicle_autonomy;
    unsigned int leftmost_reachable_station;
    unsigned int rightmost_reachable_station;

    struct station_t* next; //used for breadth-first search
    struct station_t* prev; //used for breadth-first search
};

// Used for BFS
struct station_graph_node_t {
    unsigned int distance; //key
    unsigned int leftmost_reachable_station;
    unsigned int rightmost_reachable_station;

    enum color_t color; //used for breadth-first search

    int prev_on_path; //used to print the final path
};

struct station_queue_t {
    int* station_ref;
    unsigned int head;
    unsigned int tail;
    unsigned int dim;
};

// we initialize with queue = create_station_queue(INIT_STATION_QUEUE_DIM)
struct station_queue_t* create_station_queue(unsigned int dim) {

    struct station_queue_t* res;
    res = malloc(sizeof(struct station_queue_t));
    res->station_ref = malloc(sizeof(int) * dim);
    res->head = 0;
    res->tail = 0;
    res->dim = dim;

    return res;
}

char is_empty_station_queue(struct station_queue_t* queue) {

    if(queue->head == queue->tail) return 1;

    return 0;
}

int dequeue_station(struct station_queue_t* queue) {

    if(is_empty_station_queue(queue)) return -1;

    int res;
    res = queue->station_ref[queue->head];
    queue->head = (queue->head + 1) % queue->dim;

    return res;
}

struct station_queue_t* deallocate_station_queue(struct station_queue_t* queue) {

    free(queue->station_ref);
    queue->station_ref = NULL;
    free(queue);
    queue = NULL;

    return NULL;
}

struct station_queue_t* enqueue_station(struct station_queue_t* queue, int station);

struct station_queue_t* reallocate_station_queue(struct station_queue_t* queue) {
    
    struct station_queue_t* res;
    res = create_station_queue(queue->dim << 1);
    
    // if we check for emptyness we get a false positive, we need to extract
    // an element from the queue
    res = enqueue_station(res, queue->station_ref[queue->head]);
    queue->head = (queue->head + 1) % queue->dim;

    while(!is_empty_station_queue(queue))
        res = enqueue_station(res,dequeue_station(queue));

    deallocate_station_queue(queue);

    return res;
}

struct station_queue_t* enqueue_station(struct station_queue_t* queue, int station) {

    queue->station_ref[queue->tail] = station;
    queue->tail = (queue->tail + 1) % queue->dim;
    
    if(queue->tail == queue->head) {
        queue = reallocate_station_queue(queue);
    }

    return queue;
}

// Create a new node with given autonomy
struct vehicle_t* create_vehicle_node(unsigned int autonomy) {
    struct vehicle_t* res;

    res = malloc(sizeof(struct vehicle_t));
    res->autonomy = autonomy;
    res->num = 1;
    res->height = 1; // this will be updated later if necessary
    res->left = NULL;
    res->right = NULL;

    return res;
}

unsigned int vehicle_height(struct vehicle_t* vehicle) {
    // if one of the children is NULL, return the height of the other + 1
    if(vehicle == NULL) {
        return 0;
    }
    return vehicle->height;
}

int get_vehicle_balance(struct vehicle_t* vehicle) {
    return (vehicle_height(vehicle->right)-vehicle_height(vehicle->left));
}

struct vehicle_t* left_rotate_vehicle(struct vehicle_t* vehicle) {
    
    struct vehicle_t* tmp;
    tmp = vehicle->right;
    vehicle->right = tmp->left;
    tmp->left = vehicle;

    // now we recalculate correct height of moved nodes using subtrees as invariants
    vehicle->height = MAX(vehicle_height(vehicle->left),vehicle_height(vehicle->right)) + 1;
    tmp->height = MAX(vehicle_height(tmp->left),vehicle_height(tmp->right)) + 1;

    return tmp;
}

struct vehicle_t* right_rotate_vehicle(struct vehicle_t* vehicle) {
    
    struct vehicle_t* tmp;
    tmp = vehicle->left;
    vehicle->left = tmp->right;
    tmp->right = vehicle;

    // now we recalculate correct height of moved nodes using subtrees as invariants
    vehicle->height = MAX(vehicle_height(vehicle->left),vehicle_height(vehicle->right)) + 1;
    tmp->height = MAX(vehicle_height(tmp->left),vehicle_height(tmp->right)) + 1;

    return tmp;
}

// Add vehicle with given autonomy to specified tree.
struct vehicle_t* add_vehicle(struct vehicle_t* vehicle, unsigned int autonomy) {
    // if we reach the bottom of the tree without finding
    // a node with the key we add the new node here
    if(vehicle == NULL) return (create_vehicle_node(autonomy));

    // if we find the key we increment the counter
    if(autonomy == vehicle->autonomy) {
        vehicle->num++;
        return vehicle;
    }
 
    // if the key is different we try to add the vehicle on the correct side
    if(autonomy < vehicle->autonomy) {
        vehicle->left = add_vehicle(vehicle->left,autonomy);
    }
    else {
        vehicle->right = add_vehicle(vehicle->right,autonomy);
    }

    // we have to recalculate the height of current node
    vehicle->height = MAX(vehicle_height(vehicle->left),vehicle_height(vehicle->right)) + 1;

    // get the balance vector to check if we have lost the AVL property
    int balance = get_vehicle_balance(vehicle);

    // if node is now unbalanced we have now 4 possible cases
    // and we can reconduce two of them to the other two
    // Left Right to Left Left
    // Right Left to Right Right
    if(balance < -1) {
        // Left Right case
        if(autonomy > vehicle->left->autonomy)
        {
            vehicle->left = left_rotate_vehicle(vehicle->left);
        }
        // Now we are in Left Left case
        return right_rotate_vehicle(vehicle);
    }
    if(balance > 1) {
        // Right Left case
        if(autonomy < vehicle->right->autonomy) {
            vehicle->right = right_rotate_vehicle(vehicle->right);
        }
        // Now we are in Right Right case
        return left_rotate_vehicle(vehicle);
    }

    //if the node is still in balance we return it unmodified
    
    return vehicle;
}

// the minimum is the node in bottom left of the tree
struct vehicle_t* minimum_vehicle(struct vehicle_t* vehicle) {

    if(vehicle == NULL) return NULL;

    while(vehicle->left != NULL) {
        vehicle = vehicle->left;
    }

    return vehicle;

}

// the maximum is the node in bottom right of the tree
struct vehicle_t* maximum_vehicle(struct vehicle_t* vehicle) {

    if(vehicle == NULL) return NULL;

    while(vehicle->right != NULL) {
        vehicle = vehicle->right;
    }

    return vehicle;

}

struct vehicle_t* remove_vehicle(struct vehicle_t* vehicle, unsigned int autonomy) {

    // If the vehicle with given autonomy is not found, do nothing
    if(vehicle == NULL) return NULL;

    // If the vehicle with given autonomy has lower autonomy than current,
    // we recursively call the function on the left child
    if(autonomy < vehicle->autonomy) {
        vehicle->left = remove_vehicle(vehicle->left,autonomy);
    }
    if(autonomy > vehicle->autonomy) {
        vehicle->right = remove_vehicle(vehicle->right,autonomy);
    }

    // If we find the vehicle we decrease its number if >1, else delete it
    if(autonomy == vehicle->autonomy) {
        
        if(vehicle->num>1) {
            vehicle->num--;
            return vehicle;
        }
        
        // we have to check if the vehicle to remove has 2 or less children
        struct vehicle_t* tmp;

        if(vehicle->left == NULL || vehicle->right == NULL) {
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
            vehicle->right = remove_vehicle(vehicle->right,tmp->autonomy);
            
    }

    //at this point we have removed the node if present. If there was only one node we return NULL
    if(vehicle == NULL) return NULL;

    // we have to recalculate current node height
    vehicle->height = MAX(vehicle_height(vehicle->left), vehicle_height(vehicle->right)) + 1;

    // now we calculate current node balance
    int balance = get_vehicle_balance(vehicle);

    // if the node is not balanced we have to perform same rotations as in the add_vehicle case
    if(balance < -1) {
        // Left Right case
        if(get_vehicle_balance(vehicle->left) == 1)
        {
            vehicle->left = left_rotate_vehicle(vehicle->left);
        }
        // Now we are in Left Left case
        return right_rotate_vehicle(vehicle);
    }
    if(balance > 1) {
        // Right Left case
        if(get_vehicle_balance(vehicle->right) == -1) {
            vehicle->right = right_rotate_vehicle(vehicle->right);
        }
        // Now we are in Right Right case
        return left_rotate_vehicle(vehicle);
    }

    //if the node is still in balance we return it unmodified
    
    return vehicle;

}

void remove_all_vehicles(struct vehicle_t* vehicle) {

    if(vehicle != NULL) {
        remove_all_vehicles(vehicle->left);
        remove_all_vehicles(vehicle->right);
        
        free(vehicle);
        vehicle = NULL;
    }

    return;
}

struct vehicle_t* find_vehicle(struct vehicle_t* vehicle, unsigned int autonomy) {

    // if we reach the end we return NULL
    if(vehicle == NULL) return NULL;

    if(autonomy == vehicle->autonomy) return vehicle;

    if(autonomy > vehicle->autonomy) return find_vehicle(vehicle->right,autonomy);
    if(autonomy < vehicle->autonomy) return find_vehicle(vehicle->left,autonomy);

    return NULL;

}

// the minimum is the node in bottom left of the tree
struct station_t* minimum_station(struct station_t* station) {

    if(station == NULL) return NULL;

    while(station->left != NULL) {
        station = station->left;
    }

    return station;

}

// the maximum is the node in bottom right of the tree
struct station_t* maximum_station(struct station_t* station) {

    if(station == NULL) return NULL;

    while(station->right != NULL) {
        station = station->right;
    }

    return station;

}

struct station_t* predecessor_station(struct station_t* station) {

    if (station->left != NULL)
        return maximum_station(station->left);
    
    while(station->parent != NULL && station->parent->left == station) {
        station = station->parent;
    }

    return station->parent;

}

struct station_t* successor_station(struct station_t* station) {

    if (station->right != NULL)
        return minimum_station(station->right);
    
    while(station->parent != NULL && station->parent->right == station) {
        station = station->parent;
    }

    return station->parent;

}

struct station_t* station_greater_or_equal_to_distance(struct station_t* station_tree, unsigned int distance) {
    if (station_tree == NULL) return NULL;

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
}

void update_reachable_stations(struct station_t* station) {
    
    station->rightmost_reachable_station = station->distance + station->max_vehicle_autonomy;
    if(station->max_vehicle_autonomy > station->distance)
        station->leftmost_reachable_station = 0;
    else
        station->leftmost_reachable_station = station->distance - station->max_vehicle_autonomy;

    return;
}

// Create a new node with given distance
struct station_t* create_station_node(unsigned int distance) {
    struct station_t* res;

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

unsigned int station_height(struct station_t* station) {
    // if one of the children is NULL, return the height of the other + 1
    if(station == NULL) {
        return 0;
    }
    return station->height;
}

int get_station_balance(struct station_t* station) {
    return (station_height(station->right)-station_height(station->left));
}

struct station_t* left_rotate_station(struct station_t* station) {
    
    struct station_t* tmp;
    tmp = station->right;
    station->right = tmp->left;
    if(station->right != NULL)
        station->right->parent = station;
    tmp->left = station;
    station->parent = tmp;

    // now we recalculate correct height of moved nodes using subtrees as invariants
    station->height = MAX(station_height(station->left),station_height(station->right)) + 1;
    tmp->height = MAX(station_height(tmp->left),station_height(tmp->right)) + 1;

    // when we return this we have to update the partent
    return tmp;
}

struct station_t* right_rotate_station(struct station_t* station) {
    
    struct station_t* tmp;
    tmp = station->left;
    station->left = tmp->right;
    if(station->left != NULL)
        station->left->parent = station;
    tmp->right = station;
    station->parent = tmp;

    // now we recalculate correct height of moved nodes using subtrees as invariants
    station->height = MAX(station_height(station->left),station_height(station->right)) + 1;
    tmp->height = MAX(station_height(tmp->left),station_height(tmp->right)) + 1;

    // when we return this we have to update the partent
    return tmp;
}

// Add station with given distance to specified tree.
struct station_t* add_station(struct station_t* station, unsigned int distance, struct station_t** station_ref) {
    // if we reach the bottom of the tree without finding
    // a node with the key we add the new node here
    if(station == NULL) {

        *station_ref = (create_station_node(distance));
        return *station_ref;
    }

    // if we find the key we have to return NULL: only a station with given distance
    //if(distance == station->distance) return NULL;
    // maybe implement logic to check if the station exists here to avoid calling find_station()

    // if the key is different we try to add the station on the correct side
    if(distance < station->distance) {
        station->left = add_station(station->left,distance,station_ref);
        station->left->parent = station;
    }
    else if(distance > station->distance){
        station->right = add_station(station->right,distance,station_ref);
        station->right->parent = station;
    }

    // we have to recalculate the height of current node
    station->height = MAX(station_height(station->left),station_height(station->right)) + 1;

    // get the balance vector to check if we have lost the AVL property
    int balance = get_station_balance(station);

    // if node is now unbalanced we have now 4 possible cases
    // and we can reconduce two of them to the other two
    // Left Right to Left Left
    // Right Left to Right Right
    if(balance < -1) {
        // Left Right case
        if(distance > station->left->distance)
        {
            station->left = left_rotate_station(station->left);
            station->left->parent = station;
        }
        // Now we are in Left Left case
        return right_rotate_station(station);
    }
    if(balance > 1) {
        // Right Left case
        if(distance < station->right->distance) {
            station->right = right_rotate_station(station->right);
            station->right->parent = station;
        }
        // Now we are in Right Right case
        return left_rotate_station(station);
    }

    //if the node is still in balance we return it unmodified
    
    return station;
}

struct station_t* remove_station(struct station_t* station, unsigned int distance, char* flag) {

    // If the station with given distance is not found, do nothing
    if(station == NULL) return NULL;

    // If the station with given distance has lower distance than current,
    // we recursively call the function on the left child
    if(distance < station->distance) {
        station->left = remove_station(station->left,distance,flag);
        if(station->left != NULL)
            station->left->parent = station;
    }
    if(distance > station->distance) {
        station->right = remove_station(station->right,distance,flag);
        if(station->right != NULL)
            station->right->parent = station;
    }

    if(distance == station->distance) {

        *flag = 1;

        // fixing next prev hole
        if (station->prev != NULL)
            station->prev->next = station->next;
        if (station->next != NULL)
            station->next->prev = station->prev;
        
        // we have to check if the station to remove has 2 or less children
        struct station_t* tmp;

        if(station->left == NULL || station->right == NULL) {
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
        tmp->vehicle_parking = NULL; // this is necessary to avoid removing useful vehicles
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
        station->right = remove_station(station->right,tmp->distance,flag);
        if(station->right != NULL)
            station->right->parent = station;
            
    }

    //at this point we have removed the node if present. If there was only one node we return NULL
    if(station == NULL) return NULL;

    // we have to recalculate current node height
    station->height = MAX(station_height(station->left), station_height(station->right)) + 1;

    // now we calculate current node balance
    int balance = get_station_balance(station);

    // if the node is not balanced we have to perform same rotations as in the add_station case
    if(balance < -1) {
        // Left Right case
        if(get_station_balance(station->left) == 1)
        {
            station->left = left_rotate_station(station->left);
            station->left->parent = station;
        }
        // Now we are in Left Left case
        return right_rotate_station(station);
    }
    if(balance > 1) {
        // Right Left case
        if(get_station_balance(station->right) == -1) {
            station->right = right_rotate_station(station->right);
            station->right->parent = station;
        }
        // Now we are in Right Right case
        return left_rotate_station(station);
    }

    //if the node is still in balance we return it unmodified
    
    return station;

}

void remove_all_stations(struct station_t* station) {

    if(station != NULL) {
        remove_all_stations(station->left);
        remove_all_stations(station->right);
        
        remove_all_vehicles(station->vehicle_parking);
        free(station);
        station = NULL;
    }

    return;
}

struct station_t* find_station(struct station_t* station, unsigned int distance) {

    // if we reach the end we return NULL
    if(station == NULL) return NULL;

    if(distance == station->distance) return station;

    if(distance > station->distance) return find_station(station->right,distance);
    if(distance < station->distance) return find_station(station->left,distance);

    return NULL;

}

void add_vehicle_to_station(struct station_t* station, unsigned int autonomy) {

    station->vehicle_parking = add_vehicle(station->vehicle_parking, autonomy);
    if(autonomy > station->max_vehicle_autonomy) {
        station->max_vehicle_autonomy = autonomy;
        update_reachable_stations(station);
    }

    return;
}

void remove_vehicle_from_station(struct station_t* station, unsigned int autonomy) {

    station->vehicle_parking = remove_vehicle(station->vehicle_parking, autonomy);
    if(find_vehicle(station->vehicle_parking,autonomy) == NULL) {
        struct vehicle_t* tmp;
        tmp = maximum_vehicle(station->vehicle_parking);
        if(tmp == NULL) {
            station->max_vehicle_autonomy = 0;
            update_reachable_stations(station);
        }
        else {
            station->max_vehicle_autonomy = tmp->autonomy;
            update_reachable_stations(station);
        }
    }

    return;
}

/* //this makes all nodes white again
void clean_tree(struct station_t* station) {
    if(station == NULL) return;

    clean_tree(station->left);
    clean_tree(station->right);

    return;
}
 */

// We can use the stack to print the correct order of stations
void print_route(struct station_graph_node_t* vect, unsigned int station,unsigned int end_station) {

    if(vect[station].prev_on_path == -1) {
        printf("%d\n", vect[station].distance);
        return;
    }

    printf("%d ", vect[station].distance);

    print_route(vect,vect[station].prev_on_path,end_station);
    
    return;
}

// We can use the stack to print the correct order of stations
void print_route_reverse(struct station_graph_node_t* vect, unsigned int station, unsigned int end_station) {

    if(vect[station].prev_on_path == -1) {
        printf("%d ", vect[station].distance);
        return;
    }

    print_route_reverse(vect,vect[station].prev_on_path,end_station);

    if(station==end_station) {
        printf("%d\n", vect[station].distance);
        return;
    }

    printf("%d ", vect[station].distance);
    return;
}

unsigned int number_of_stations_between(struct station_t* begin, struct station_t* end) {

    unsigned int res = 1;

    struct station_t* curr;

    curr = begin;

    while(curr != end) {
        res++;
        curr = curr->next;
    }

    return res;

}

void vector_of_stations_between(struct station_graph_node_t* vect, struct station_t* begin, struct station_t* end) {

    unsigned int idx = 0;

    struct station_t* curr;

    curr = begin;

    while(curr != end->next) {
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

void plan_route(struct station_t* station_tree,struct station_t* begin_station,struct station_t* end_station,struct station_queue_t** queue) {

    unsigned int num_stations; // number of stations between begin and end station

    unsigned int curr,
                 tmp;

    *queue = create_station_queue(INIT_STATION_QUEUE_DIM);

    // clean_tree(station_tree);

    // if the start and end stations are the same print the distance and return
    if(begin_station->distance == end_station->distance) {

        printf("%d\n", begin_station->distance);
        *queue = deallocate_station_queue(*queue);
        return;
    }

    if(begin_station->distance < end_station->distance) {

        num_stations = number_of_stations_between(begin_station,end_station);
        struct station_graph_node_t station_vector[num_stations];
        vector_of_stations_between(station_vector,begin_station,end_station);

        //vect_begin_station = &station_vector[0];
        //vect_end_station = &station_vector[num_stations-1];

        station_vector[0].color = GREY;
        *queue = enqueue_station(*queue, 0);
        tmp = 1;
        while(!is_empty_station_queue(*queue)) {
            curr = dequeue_station(*queue);
            
            while(tmp < num_stations && station_vector[tmp].distance <= station_vector[curr].rightmost_reachable_station) {
                if(tmp == num_stations - 1) {
                    station_vector[tmp].prev_on_path = curr;
                    print_route_reverse(station_vector,tmp,num_stations - 1);
                    *queue = deallocate_station_queue(*queue);
                    return;
                }
                // We do not need to keep track of visited nodes because
                // of how the graph is structured when going forward
                //if(tmp->color == WHITE) {
                    //tmp->color = GREY;
                    station_vector[tmp].prev_on_path = curr;
                    *queue = enqueue_station(*queue, tmp);
                //}
                tmp = tmp + 1;
            }
        }
    }

    if(begin_station->distance > end_station->distance) {

        num_stations = number_of_stations_between(end_station,begin_station);
        struct station_graph_node_t station_vector[num_stations];
        vector_of_stations_between(station_vector,end_station,begin_station);
        
        //vect_begin_station = &station_vector[num_stations-1];
        //vect_end_station = &station_vector[0];

        station_vector[0].color = GREY;
        *queue = enqueue_station(*queue, 0);

        while(!is_empty_station_queue(*queue)) {
            curr = dequeue_station(*queue);
            tmp = curr + 1;
            while(tmp != num_stations ) {
                if(station_vector[curr].distance < station_vector[tmp].leftmost_reachable_station) {
                    tmp = tmp + 1;
                    continue;
                }
                if(tmp == num_stations - 1) {
                    station_vector[tmp].prev_on_path = curr;
                    print_route(station_vector,tmp,num_stations - 1);
                    *queue = deallocate_station_queue(*queue);
                    return;
                }
                if(station_vector[tmp].color == WHITE) {
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
    unsigned int station_distance,
                 end_station_distance,
                 vehicle_autonomy,
                 vehicles_number;

    struct station_t* stations = NULL; // Empty tree for stations
    struct station_t* station; //the station on which we do operations
    struct station_queue_t* queue = NULL; // Pointer to queue

    
     // Scan every command until EOF or Ctrl-D in terminal
    while(scanf("%s %d", command, &station_distance) != EOF) {
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
        switch(command[12]) {
            // aggiungi-stazione
            case 'z':
                // We check that the station does not already exist
                station = NULL;
                stations = add_station(stations, station_distance,&station);
                stations->parent = NULL;
                if(station!=NULL) {

                    // fix next prev pointers
                    struct station_t* tmp;
                    tmp = predecessor_station(station);
                    if(tmp != NULL) {
                        station->prev = tmp;
                        station->next = tmp->next;
                        tmp->next = station;
                        if (station->next != NULL)
                            station->next->prev = station;
                    }
                    else {
                        tmp = successor_station(station);
                        station->next = tmp;
                        if (tmp != NULL)
                            tmp->prev = station;
                    }

                    if(scanf("%d", &vehicles_number) == 1) {
                        for(int i=0;i<vehicles_number;i++) {
                            if(scanf("%d", &vehicle_autonomy) == 1) {
                                add_vehicle_to_station(station,vehicle_autonomy);
                            }
                        }
                    }

                    printf("aggiunta\n");
                }
                else {
                    if(scanf("%d", &vehicles_number) == 1) {
                        for(int i=0;i<vehicles_number;i++) {
                            if(scanf("%d", &vehicle_autonomy) == 1) {}
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
                if(flag == 1) {
                    
                    printf("demolita\n");
                }
                else {
                    printf("non demolita\n");
                }
                break;
            // aggiungi-auto
            case 'o':
                // Check if station with given distance exists.
                // If exists find_station returns pointer to station,
                // else NULL
                if(scanf("%d", &vehicle_autonomy) == 1) {
                    station = find_station(stations,station_distance); 
                    if(station!=NULL) {
                        add_vehicle_to_station(station,vehicle_autonomy);
                        printf("aggiunta\n");
                    }
                    else {
                      printf("non aggiunta\n");
                    }
                }
                break;
            // rottama-auto
            case '\0':
                // Check if the station exists then check
                // if the car has been removed or not
                if(scanf("%d", &vehicle_autonomy) == 1) {
                    station = find_station(stations,station_distance);
                    if(station!=NULL) {
                        if(find_vehicle(station->vehicle_parking, vehicle_autonomy) != NULL) {
                            remove_vehicle_from_station(station,vehicle_autonomy);
                            printf("rottamata\n");
                        }
                        else {
                          printf("non rottamata\n");
                        }
                    }
                    else {
                      printf("non rottamata\n");
                    }
                }
                break;
            // pianifica-percorso
            case 'r':
                if (scanf("%d", &end_station_distance) == 1) {
                struct station_t* begin_station;
                struct station_t* end_station;
                begin_station = find_station(stations,station_distance);
                end_station = find_station(stations,end_station_distance);
                if(begin_station != NULL && end_station != NULL)
                    plan_route(stations,begin_station,end_station,&queue);
                else printf("nessun percorso\n");
                }
                break;

        }
    }

    remove_all_stations(stations);

    stations = NULL;

    return 0;
}
