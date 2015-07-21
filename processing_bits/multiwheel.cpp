#include "multiwheel.h"
#include <iostream>
#include <functional>

/*
 * todo:
 * maybe use std::async instead of std::thread, or define the move function as a lambda.  Probably unneeded.
 * */

void asyncFilterMove(FLIFilterWheel* wheel, long pos)
{
    wheel->setFilterPos(pos);
}

multiwheel::multiwheel(std::vector<FLIFilterWheel*> wheels) : wheels_(wheels)
{ }

void multiwheel::setFilterPos(long pos)
{
    std::vector<std::thread> threads;
    for (auto wheel : wheels_)
        threads.push_back(std::thread(asyncFilterMove, wheel, pos));

    for (auto& t : threads)
        t.join();
}
