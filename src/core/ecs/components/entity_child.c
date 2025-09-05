#include "Spark/ecs/components/entity_child.h"
#include "Spark/ecs/entity.h"

void entity_child_destroy_callback(void* component) {
    entity_child_t* child = component;
    darray_entity_destroy(&child->children);
}
