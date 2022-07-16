#ifndef TC_SYNCHRONIZATION_DUMMYTASK_H
#define TC_SYNCHRONIZATION_DUMMYTASK_H


#include "Task.hpp"

class DummyTask : public Task {

public:
    void execute();

    DummyTask() : Task("Dummy", nullptr, 2000) {}
};


#endif //TC_SYNCHRONIZATION_DUMMYTASK_H
