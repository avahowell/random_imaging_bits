#ifndef MULTIWHEEL_H
#define MULTIWHEEL_H
#include <vector>
#include <thread>
#include "flifilterwheel.h"

/*
 * */

class multiwheel
{
private:
    std::vector<FLIFilterWheel*> wheels_;
public:
    multiwheel(std::vector<FLIFilterWheel*> wheels);
    void setFilterPos(long pos);
    int size() { return wheels_.size(); }
};

#endif // MULTIWHEEL_H
