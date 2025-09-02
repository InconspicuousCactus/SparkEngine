#include "Spark/containers/unordered_map.h"

hashmap_type(hashmap, u64, u64);
hashmap_impl(hashmap, u64, u64, hash_passthrough, u64_compare);

int main() {
    initialize_memory();
    const u32 value_count = 10000;
    u64 rand_values[value_count];
    u64 rand_keys[value_count];
    for (u32 i = 0; i < value_count; i++) {
        rand_values[i] = random();
        rand_keys[i] = random();
    }

    hashmap_t map;
    hashmap_create(100, &map);

    for (u32 i = 0; i < value_count; i++) {
        hashmap_insert(&map, rand_keys[i], rand_values[i]);
    }

    for (u32 i = 0; i < value_count; i++) {
        if (*hashmap_get(&map, rand_keys[i]) != rand_values[i]) {
            SERROR("Hashmap implementation failed tests. Did not receive same value key pair after insertions. Key: 0x%x, Value: 0x%x, Expected Value: 0x%x", 
                    rand_keys[i], 
                    *hashmap_get(&map, rand_keys[i]), 
                    rand_values[i]);
            return false;
        }
    }

    SDEBUG("Hashmap test success");
}

