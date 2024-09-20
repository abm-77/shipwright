#pragma once

#include <algorithm>
#include <assert.h>
#include <cstddef>
#include <cstdio>
#include <iterator>
#include <set>
#include <tuple>
#include <type_traits>

#include "common.hpp"
#include "static_array.hpp"

namespace Entity {
static constexpr u32 MAX_ENTITIES = 4096;

using EntityID = u32;
using EntityArray = StaticArray<EntityID, MAX_ENTITIES>;
using EntitySet = std::set<EntityID>;

template <typename Component> class ComponentArray {
public:
  void free() {}

  void alloc_for(EntityID id) {
    if (active_entities.insert(id).second) {
      components.append(Component());
      const size_t idx = components.size() - 1;
      update_mapping(id, idx);
      update_owner(idx, id);
    }
  }

  void remove_from(EntityID id) {
    if (active_entities.erase(id) > 0) {
      const u32 remove_idx = component_idx_for(id);
      const u32 swap_idx = components.size() - 1;
      const EntityID swapped_entity = owner_of(swap_idx);

      components.swap_remove(remove_idx);
      update_mapping(swapped_entity, remove_idx);
      update_owner(remove_idx, swapped_entity);
    }
  }

  void set(EntityID id, Component val) {
    assert_entity_has_component(id);
    components.set(component_idx_for(id), val);
  }

  Component get(EntityID id) {
    assert_entity_has_component(id);
    return components.get(component_idx_for(id));
  }

  Component *get_ptr(EntityID id) {
    assert_entity_has_component(id);
    return components.get_ptr(component_idx_for(id));
  }

  auto begin_active() { return active_entities.begin(); }
  auto end_active() { return active_entities.end(); }

private:
  inline void assert_entity_has_component(EntityID id) {
    assert(active_entities.count(id) > 0);
  }
  inline u32 component_idx_for(EntityID id) { return indirect[id]; }
  inline EntityID owner_of(u32 index) { return owner[index]; }
  inline void update_owner(u32 index, EntityID id) { owner[index] = id; }
  inline void update_mapping(EntityID id, u32 index) { indirect[id] = index; }

private:
  using ArrayType = StaticArray<Component, MAX_ENTITIES>;
  ArrayType components;
  EntitySet active_entities;
  EntityID owner[MAX_ENTITIES];
  u32 indirect[MAX_ENTITIES];
};

struct TransformComponent {
  f32 x;
  f32 y;
  f32 z;
};

struct HealthComponent {
  u32 health;
  u32 max_health;
};

class EntitySubsystem {
public:
  EntityArray free_entities;
  ComponentArray<TransformComponent> transforms;
  ComponentArray<HealthComponent> healths;

  template <typename T> ComponentArray<T> *get_component_array() {
    if constexpr (std::is_same_v<T, TransformComponent>) {
      return &transforms;
    } else if constexpr (std::is_same_v<T, HealthComponent>) {
      return &healths;
    }
    return nullptr;
  }
};
static EntitySubsystem sys;

void init() {
  for (int i = 0; i < MAX_ENTITIES; i++)
    sys.free_entities.append(i);
}

void deinit() {
  sys.transforms.free();
  sys.healths.free();
}

EntityID alloc_entity() { return sys.free_entities.pop(); }
void free_entity(EntityID id) {
  sys.transforms.remove_from(id);
  sys.free_entities.append(id);
}

template <typename Component> void add(EntityID id) {
  sys.get_component_array<Component>()->alloc_for(id);
}

template <typename Component> void set(EntityID id, Component val) {
  sys.get_component_array<Component>()->set(id, val);
}

template <typename Component> Component get(EntityID id) {
  return sys.get_component_array<Component>()->get(id);
}

template <typename Component> Component *get_ptr(EntityID id) {
  return sys.get_component_array<Component>()->get_ptr(id);
}

template <class... Components> EntityID create_entity_with_archetype() {
  EntityID entity = alloc_entity();
  return entity;
}

template <class... Types> class Archetype {
public:
  static EntityID alloc() {
    EntityID entity = alloc_entity();
    apply_archetype(entity);
    return entity;
  }

  static void find_entities(std::set<EntityID> &out) { find(out); }

private:
  static constexpr size_t size = sizeof...(Types);
  template <int i = 0> static void apply_archetype(EntityID entity) {
    if constexpr (i < size) {
      add<std::tuple_element_t<i, std::tuple<Types...>>>(entity);
      apply_archetype<i + 1>(entity);
    }
  }

  template <int i = 0> static void find(std::set<EntityID> &entities) {
    if constexpr (i < size) {
      auto component_array = sys.get_component_array<
          std::tuple_element_t<i, std::tuple<Types...>>>();
      std::set_union(entities.begin(), entities.end(),
                     component_array->begin_active(),
                     component_array->end_active(),
                     std::inserter(entities, entities.begin()));
      find<i + 1>(entities);
    }
  }
};

} // namespace Entity
  //
