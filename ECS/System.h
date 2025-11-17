#ifndef SYSTEM_H
#define SYSTEM_H

namespace bbl
{
class System
{
public:
    virtual ~System() = default;

public:
    virtual void update() = 0;
    virtual void Init() = 0;

};

};



#endif // SYSTEM_H
