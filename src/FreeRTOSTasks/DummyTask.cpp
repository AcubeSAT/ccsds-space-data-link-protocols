#include "FreeRTOSTasks/DummyTask.h"
#include <iostream>

void DummyTask::execute() {
    std::cout << "Yo" << std::endl;
}