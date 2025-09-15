#include "catch2/catch_all.hpp"
#include "ExternalContainers.hpp"

using namespace CCSDSDataLinkLayer;

TEST_CASE("Queue Operations", "[Data Structures]") {
    uint8_t testStorage[3];
    auto testQueue = Queue(etl::span(testStorage, 3));

    SECTION("Pushing and popping") {
        REQUIRE(testQueue.maxSize() == 3);
        REQUIRE(testQueue.isEmpty());
        REQUIRE(testQueue.currentSize() == 0);
        REQUIRE(testQueue.remainingCapacity() == 3);

        testQueue.push(10);
        testQueue.push(20);
        REQUIRE(!testQueue.isFull());
        REQUIRE(!testQueue.isEmpty());
        REQUIRE(testQueue.currentSize() == 2);
        REQUIRE(testQueue.remainingCapacity() == 1);
        REQUIRE(testQueue.getFront() == 10);
        REQUIRE(testQueue.getBack() == 20);

        testQueue.push(30);
        REQUIRE(testQueue.isFull());
        REQUIRE(testQueue.currentSize() == 3);
        REQUIRE(testQueue.remainingCapacity() == 0);
        REQUIRE(testQueue.getFront() == 10);
        REQUIRE(testQueue.getBack() == 30);

        testQueue.pop();
        testQueue.pop();
        REQUIRE(testQueue.currentSize() == 1);
        REQUIRE(testQueue.remainingCapacity() == 2);
        REQUIRE(testQueue.getFront() == 30);
        REQUIRE(testQueue.getBack() == 30);
    }

    SECTION("Pushing and popping with wrapping") {
        testQueue.push(10);
        testQueue.push(20);
        testQueue.push(30);
        testQueue.pop();
        testQueue.push(40);

        REQUIRE(testQueue.isFull());
        REQUIRE(testQueue.currentSize() == 3);
        REQUIRE(testQueue.getFront() == 20);
        REQUIRE(testQueue.getBack() == 40);
    }

    SECTION("Reset") {
        testQueue.push(10);
        testQueue.reset();
        REQUIRE(testQueue.isEmpty());
        REQUIRE(testQueue.currentSize() == 0);
        REQUIRE(testQueue.remainingCapacity() == 3);
    }
}

TEST_CASE("Dequeue Operations", "[Data Structures]") {
    uint8_t testStorage[3];
    auto testDequeue = Dequeue(etl::span(testStorage, 3));

    SECTION("Pushing and popping") {
        REQUIRE(testDequeue.maxSize() == 3);
        REQUIRE(testDequeue.isEmpty());
        REQUIRE(testDequeue.currentSize() == 0);
        REQUIRE(testDequeue.remainingCapacity() == 3);

        testDequeue.pushBack(10);
        testDequeue.pushBack(20);
        REQUIRE(!testDequeue.isFull());
        REQUIRE(!testDequeue.isEmpty());
        REQUIRE(testDequeue.currentSize() == 2);
        REQUIRE(testDequeue.remainingCapacity() == 1);
        REQUIRE(testDequeue.getFront() == 10);
        REQUIRE(testDequeue.getBack() == 20);

        testDequeue.pushBack(30);
        REQUIRE(testDequeue.isFull());
        REQUIRE(testDequeue.currentSize() == 3);
        REQUIRE(testDequeue.remainingCapacity() == 0);
        REQUIRE(testDequeue.getFront() == 10);
        REQUIRE(testDequeue.getBack() == 30);

        testDequeue.popFront();
        testDequeue.popFront();
        REQUIRE(testDequeue.currentSize() == 1);
        REQUIRE(testDequeue.remainingCapacity() == 2);
        REQUIRE(testDequeue.getFront() == 30);
        REQUIRE(testDequeue.getBack() == 30);

        testDequeue.pushFront(40);
        REQUIRE(testDequeue.currentSize() == 2);
        REQUIRE(testDequeue.remainingCapacity() == 1);
        REQUIRE(testDequeue.getFront() == 40);
        REQUIRE(testDequeue.getBack() == 30);

        testDequeue.popBack();
        REQUIRE(testDequeue.currentSize() == 1);
        REQUIRE(testDequeue.remainingCapacity() == 2);
        REQUIRE(testDequeue.getFront() == 40);
        REQUIRE(testDequeue.getBack() == 40);
    }

    SECTION("Pushing and popping with wrapping") {
        testDequeue.pushBack(10);
        testDequeue.pushBack(20);
        testDequeue.pushBack(30);
        testDequeue.popFront();
        testDequeue.pushBack(40);

        REQUIRE(testDequeue.isFull());
        REQUIRE(testDequeue.currentSize() == 3);
        REQUIRE(testDequeue.getFront() == 20);
        REQUIRE(testDequeue.getBack() == 40);
    }

    SECTION("Reset") {
        testDequeue.pushFront(10);
        testDequeue.reset();
        REQUIRE(testDequeue.isEmpty());
        REQUIRE(testDequeue.currentSize() == 0);
        REQUIRE(testDequeue.remainingCapacity() == 3);
    }
}

TEST_CASE("Circular Buffer Operations", "[Data Structures]") {
    uint8_t testStorage[3];
    auto testCircularBuffer = CircularBuffer(etl::span(testStorage, 3));

    SECTION("Pushing and popping") {
        REQUIRE(testCircularBuffer.maxSize() == 3);
        REQUIRE(testCircularBuffer.isEmpty());
        REQUIRE(testCircularBuffer.currentSize() == 0);
        REQUIRE(testCircularBuffer.remainingCapacity() == 3);

        testCircularBuffer.push(10);
        testCircularBuffer.push(20);
        REQUIRE(!testCircularBuffer.isFull());
        REQUIRE(!testCircularBuffer.isEmpty());
        REQUIRE(testCircularBuffer.currentSize() == 2);
        REQUIRE(testCircularBuffer.remainingCapacity() == 1);
        REQUIRE(testCircularBuffer.getFront() == 10);
        REQUIRE(testCircularBuffer.getBack() == 20);

        testCircularBuffer.push(30);
        REQUIRE(testCircularBuffer.isFull());
        REQUIRE(testCircularBuffer.currentSize() == 3);
        REQUIRE(testCircularBuffer.remainingCapacity() == 0);
        REQUIRE(testCircularBuffer.getFront() == 10);
        REQUIRE(testCircularBuffer.getBack() == 30);

        testCircularBuffer.pop();
        testCircularBuffer.pop();
        REQUIRE(testCircularBuffer.currentSize() == 1);
        REQUIRE(testCircularBuffer.remainingCapacity() == 2);
        REQUIRE(testCircularBuffer.getFront() == 30);
        REQUIRE(testCircularBuffer.getBack() == 30);
    }

    SECTION("Pushing and popping with wrapping") {
        testCircularBuffer.push(10);
        testCircularBuffer.push(20);
        testCircularBuffer.push(30);
        testCircularBuffer.pop();
        testCircularBuffer.push(40);

        REQUIRE(testCircularBuffer.isFull());
        REQUIRE(testCircularBuffer.currentSize() == 3);
        REQUIRE(testCircularBuffer.getFront() == 20);
        REQUIRE(testCircularBuffer.getBack() == 40);
    }

    SECTION("Pushing and popping with overflow") {
        testCircularBuffer.push(10);
        testCircularBuffer.push(20);
        testCircularBuffer.push(30);
        testCircularBuffer.push(40);

        REQUIRE(testCircularBuffer.isFull());
        REQUIRE(testCircularBuffer.currentSize() == 3);
        REQUIRE(testCircularBuffer.getFront() == 20);
        REQUIRE(testCircularBuffer.getBack() == 40);
    }

    SECTION("Reset") {
        testCircularBuffer.push(10);
        testCircularBuffer.reset();
        REQUIRE(testCircularBuffer.isEmpty());
        REQUIRE(testCircularBuffer.currentSize() == 0);
        REQUIRE(testCircularBuffer.remainingCapacity() == 3);
    }
}

TEST_CASE("Unordered Pool Operations", "[Data Structures]") {
    uint8_t testStorage[3];
    uint32_t testFreeIndicesStorage[3];
    auto testUnorderedPool =
        UnorderedPool<uint8_t>(etl::span(testStorage, 3), etl::span(testFreeIndicesStorage, 3));

    SECTION("Pushing, erasing and iterating") {
        REQUIRE(testUnorderedPool.maxSize() == 3);
        REQUIRE(testUnorderedPool.isEmpty());
        REQUIRE(testUnorderedPool.currentSize() == 0);
        REQUIRE(testUnorderedPool.remainingCapacity() == 3);

        uint8_t elem1 = 10;
        uint8_t elem2 = 20;
        uint8_t elem3 = 30;
        uint8_t* elem2Address;

        testUnorderedPool.push(elem1);
        elem2Address = testUnorderedPool.push(elem2);
        REQUIRE(!testUnorderedPool.isEmpty());
        REQUIRE(!testUnorderedPool.isFull());
        REQUIRE(testUnorderedPool.currentSize() == 2);
        REQUIRE(testUnorderedPool.remainingCapacity() == 1);

        testUnorderedPool.push(elem3);
        REQUIRE(!testUnorderedPool.isEmpty());
        REQUIRE(testUnorderedPool.isFull());
        REQUIRE(testUnorderedPool.currentSize() == 3);
        REQUIRE(testUnorderedPool.remainingCapacity() == 0);

        // iterate through the elements (we do not care about the order they are found,
        // just that all of them exist
        uint8_t loopCounter = 0;
        bool elem1Found = false;
        bool elem2Found = false;
        bool elem3Found = false;
        for (auto& element : testUnorderedPool) {
            if (element == elem1) {
                elem1Found = true;
            }
            if (element == elem2) {
                elem2Found = true;
            }
            if (element == elem3) {
                elem3Found = true;
            }
            loopCounter++;
        }

        REQUIRE(loopCounter == 3);
        REQUIRE((elem1Found && elem2Found && elem3Found));

        // erase element 2 and repeat
        testUnorderedPool.erase(elem2Address);
        loopCounter = 0;
        elem1Found = false;
        elem2Found = false;
        elem3Found = false;
        for (auto& element : testUnorderedPool) {
            if (element == elem1) {
                elem1Found = true;
            }
            if (element == elem2) {
                elem2Found = true;
            }
            if (element == elem3) {
                elem3Found = true;
            }
            loopCounter++;
        }

        REQUIRE(testUnorderedPool.currentSize() == 2);
        REQUIRE(loopCounter == 2);
        REQUIRE((elem1Found && !elem2Found && elem3Found));
    }

    SECTION("Reset") {
        testUnorderedPool.push(10);
        testUnorderedPool.reset();

        REQUIRE(testUnorderedPool.isEmpty());
        REQUIRE(testUnorderedPool.currentSize() == 0);
        REQUIRE(testUnorderedPool.remainingCapacity() == 3);

        uint8_t loopCounter = 0;
        for (auto &element : testUnorderedPool) {
            loopCounter++;
        }

        REQUIRE(loopCounter == 0);
    }
}