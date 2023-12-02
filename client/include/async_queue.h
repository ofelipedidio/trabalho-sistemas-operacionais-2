#pragma once

#include <cstdint>
#include <semaphore.h>
#include <optional>
#include <utility>
#include <vector>

namespace AsyncQueue {
    template <typename item_t>
        class async_queue {
            private:
                std::vector<std::pair<int, item_t>> queue;
                sem_t mutex, awaiting;

            public:
                async_queue() {
                    this->queue = std::vector<std::pair<int, item_t>>();
                    sem_init(&this->mutex, 0, 1);
                    sem_init(&this->awaiting, 0, 0);
                }

                void push(int id, item_t task) {
                    sem_wait(&this->mutex);
                    this->queue.push_back({id, task});
                    sem_post(&this->mutex);
                    sem_post(&this->awaiting);
                }

                item_t pop() {
                    sem_wait(&this->awaiting);
                    sem_wait(&this->mutex);
                    item_t task = this->queue.front().second;
                    this->queue.erase(this->queue.begin());
                    sem_wait(&this->mutex);
                    sem_post(&this->mutex);
                    return task;
                }

                std::optional<item_t> try_pop() {
                    if(sem_trywait(&this->awaiting) == 0) {
                        sem_wait(&this->mutex);
                        item_t task = this->queue.front().second;
                        this->queue.erase(this->queue.begin());
                        sem_post(&this->mutex);
                        return task;
                    } else {
                        return {};
                    }
                }

                item_t pop_by_id(int id) {
                    // TODO - Didio: find a way of doing this that doesn't require busy waiting
                    while (true) {
                        sem_wait(&this->awaiting);
                        sem_wait(&this->mutex);
                        for (std::size_t i = 0; i < this->queue.size(); i++) {
                            auto task = this->queue[i];
                            if (task.first == id) {
                                this->queue.erase(std::next(this->queue.begin(), i));
                                sem_post(&this->mutex);
                                return task.second;
                            }
                        }
                        sem_post(&this->mutex);
                        sem_post(&this->awaiting);
                    }
                }

                std::optional<item_t> try_pop_by_id(int id) {
                    sem_wait(&this->awaiting);
                    sem_wait(&this->mutex);
                    for (std::size_t i = 0; i < this->queue.size(); i++) {
                        auto task = this->queue[i];
                        if (task.first == id) {
                            this->queue.erase(std::next(this->queue.begin(), i));
                            sem_post(&this->mutex);
                            return task.second;
                        }
                    }
                    sem_post(&this->mutex);
                    sem_post(&this->awaiting);
                    return {};
                }

                uint64_t size() {
                    sem_wait(&this->mutex);
                    uint64_t size = this->queue.size();
                    sem_post(&this->mutex);
                    return size;
                }
        };
}

