#include "../include/task_queue.h"
#include <iostream>

#define assert(e) if (!(e)) { std::cerr << "ERROR: could not assert expression `" #e "`" << std::endl; exit(1); }

int main() {
    TaskQueue::async_task_queue<int> queue;

    queue.insert(10);
    queue.insert(20);
    queue.insert(30);

    int a = queue.take();
    int b = queue.take();
    int c = queue.take();

    assert(a == 10);
    assert(b == 20);
    assert(c == 30);

    return 0;
}
