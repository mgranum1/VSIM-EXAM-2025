#ifndef ENTITY_H
#define ENTITY_H

#include <cstdint>

namespace bbl
{

// Remember guys, Entities are just unique IDs - nothing more!
// All component management is handled by the EntityManager class
using EntityID = uint32_t;

// Invalid entity constant for error checking
constexpr EntityID INVALID_ENTITY = 0;

// Entity factory/manager - generates unique IDs
class EntityIDGenerator
{
private:
    static EntityID nextID;

public:
    static EntityID generateID() {
        return ++nextID;
    }

    // Added specificID so i can use it for saving/loading files
    static EntityID generateSpecificID(EntityID desiredID) {
        if (desiredID > nextID) {
            nextID = desiredID;
        }
        return desiredID;
    }

    // For debugging/testing - reset the counter
    static void reset() {
        nextID = 0;
    }

    static EntityID getLastID() {
        return nextID;
    }
};

} // namespace bbl

#endif // ENTITY_H
