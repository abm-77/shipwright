#include "entity.hpp"

int main(void) {
  {
    using namespace Entity;
    using EnemyArchetype = Archetype<TransformComponent, HealthComponent>;

    init();

    EntityID e = EnemyArchetype::alloc();
    set<TransformComponent>(e, {.x = 1, .y = 2, .z = 3});
    set<HealthComponent>(e, {.health = 100, .max_health = 200});

    TransformComponent T = get<TransformComponent>(e);
    printf("TransformComponent = { x = %f, y = %f, z =  %f }\n", T.x, T.y, T.z);

    HealthComponent H = get<HealthComponent>(e);
    printf("HealthComponent = { health = %u, max_health = %u  }\n", H.health,
           H.max_health);

    std::set<EntityID> al;
    EnemyArchetype::find_entities(al);

    for (EntityID id : al) {
      printf("Enemy: %u\n", id);
    }

    free_entity(e);

    deinit();
  }

  return 0;
}
