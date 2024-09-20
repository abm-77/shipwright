# Shipwright: a simple(ish?) entity component ~~system~~

## Problem: Entity Component Systems Can Be Complicated

Anywhere from super object-oriented, inheritance, interface-based design to having to register components and systems that operate on them.
I wanted to aim for something very straight-forward, easy to reason about/extend, and somewhat performant. This repo is the result of a few iterations of my take
on an ECS-like system. I do not present any of the following as new or novel. In fact, I'd be surprised if no one else has thought of this. I personally have not
seen this design before, so I just wanted to document my findings.

## Entity, Component, Hold the System:

### Entities
Entities in `shipwright`, like most ECS-likes, are just integer handles called `EntityID`. You use the handles to refer
to specific entities and their components.

### Components and ComponentArrays

Components are just POD structs, ex:
```
struct TransformComponent {
  float x;
  float y;
  float z;
};
```

`ComponentArrays` are templated collections defined on a per-component basis, ex:
```
ComponentArray<TransformComponent> transforms;
```

`ComponentArrays` are based on static `std::arrays` of `MAX_ENTITIES` size (set to 4096 in my
example). `ComponentArrays` have the following members:
```
template <typename ComponentType> ComponentArray {
  ...
  StaticArray<ComponentType, MAX_ENTITIES> components;
  std::set<EntityID> active_entities;
  EntityID owner[MAX_ENTITIES];
  u32 indirect[MAX_ENTITIES];
};
```

One of the hard requirements I set for `shipwright` was having all active components be adjacent
and contiguous in memory (i.e. no gaps between components). 

The original design allocated a normal array of size `MAX_ENTITIES` that held a component for every possible `EntityID`. 
The issue with this is that you can have gaps in the component array if components within the array are mapped to non-contiguous entity IDs.
This means now you have to check whether a component is actively mapped while iterating over the components, which introduces a 
non-desirable branch.

The next iteration of the design introduced a set of active entities. The idea here is that you could just iterate over the IDs
within the active set, looking them up in the backing array as you go. While now we are only looking at actively mapped components,
they may not be next to each other in the array. They could even be entire cache lines apart! We don't benefit 
from pre-fetching either as adjacent components in the array could be inactive.

The third iteration of the design introduces an "indirect" array. This is an array that maps an `EntityID` to its mapped component.
The idea here is that when we allocate a component for an entity, we can just append it to the backing array (in a stack-like fashion),
and update the indirect mapping. We now know all the components in the backing array are actively mapped and therefore we can just iterate
over the array directly. We still keep around the active set just for quick queries and a mystery-mousesketool we'll get into later.
While this design meets our criteria of keeping the components adjacent and contiguous, we cant't remove (components from) entities! Whether this is an 
issue or not is a matter of perspective. Theoretically, if you only ever allocate entities and never remove them, you don't have to worry about this!
Or, if you recycle entities/components using some sort of pool, this is also a non-issue. However, let's say you wanted the flexibility to add/remove 
components/entities at will. 

The last iteration of the `ComponentArray` introduces another array called "owner" which holds `EntityIDs`. This is effecively the reverse mapping of the "indirect"
array (i.e. it maps slots in the array to the entity they're assigned to).  We "remove" components from the component array by swapping the component of 
interest with the last element in the component array, and decreasing the length by 1. For a given entity id, we can look up its mapped component with the 
indirect array to determine the index of the component to remove. We then update the indirect mapping of the swapped component's entity to the index of the removed component. 
We can determine the swapped component's entity by using the newly introduced owner array! We also need to update the owner of the removed slot to the owner of the swapped component.
And now we can arbitrarily remove entities!

While this design meets all of my requirements, it has a few downsides that may not be suitable for all needs. Chief amongst them is the large memory footprint of the component array.
Creating a new component array for each type will always allocate enough space to accomodate `MAX_ENTITIES`. For large components, this could use ALOT of memory.
This could be remedied by only allocating space for entities we're actively managing. This is a relatively simple change, but I opted to keep it simple and dumb for now. Currently, there is also a 
reliance on the STL for the set implementation. While I implemented my own static array class, I did not want to spend time creating a performant set class. The last issue I'll bring up 
here is generally the use of templates. You would have to define a templated ComponentArray<T> type for every component you want to add. This could hurt build times and overall is a bit cumbersome 
for implementing new types of components. You could use a union to represent each component, but then every component is the size of the largest union member. As for the ease of use, you could
implement some macros that cut down on the boilerplate you need to write. For my needs, it's not too annoying.

You might be asking, where are the systems? The answer is: nowhere! I do not implemnent systems because that would be more complexity than I wanted to deal with. Instead I introduce...

## `Archetypes`
Now for the mystery mouseketool: Archetypes! This was one of my favorite parts of `shipwright`. 

### So what is an `Archetype`?
An `Archetype` is defined as collection of component types. `Archetypes` are useful because you can group entities of interest by their types. For example,
I can define `using LivingCreatureArchetype = Archetype<TransformComponent, HealhtComponent, LocomotionComponent>`. We can "apply" the archetype to an enttity using
`LivingCreatureArchetype::apply_archetype(Entity ID)`. This will add all components within the archetype to the provided enemy. To find all entities that "exhibit"
the archetype, we can use `LivingCreatureArchetype::find(std::set<EntityID> &entities)`. This will iterate through the (global) component arrays for each component type
and find the set union of all of their active entities. Unfortunately, if we were to iterate over the returned set, they may not be contiguous in memory. This, in my opinion,
is fine because most processing should be done on a per-component array basis to really reap the perf benefits. `Archetypes` are for more fine-grained control. I think the 
`Archetype` idea is very powerful when combined with a pool allocator. You could have some `Pool<Archetype>` to allocate and reuse entities that exhibit an archetype (e.g. `Pool<BatArchetype>`
to manage bat enemies or something). Thus you can kinda recreate systems by having a pool allocator paired with some function that operates on entities that exhibit the archetype.
I left this up to the personal preference of the user.

## Closing:
Originally, I decided to write `shipwright` in Zig, which has really nice compile-time programming
features. But, because I want a job, I shiftd my focus to writing more C++ code and wanted to port this project. Unfortunately, C++'s compile-time programming features leave a bit to be desired.
In Zig, you can kinda use types like values. E.g. I can have a type defined as `[] type` which represents an array of types, that I can interact (switch on, iterate over, etc) with at compile-time.
Thus in Zig, `Archetype`s were trivial. In C++, they are a testament to the indomitable human spirit. It involved everything from variadic macros to template based for-loops. Honestly, learned some 
really interesting template programming tricks that I hopefully never have to actually use.


