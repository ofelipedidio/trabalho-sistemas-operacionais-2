#include <queue>
#include <semaphore.h>
#include <optional>

namespace AsyncQueue {
    template <typename item_t>
        class async_queue {
            private:
                std::queue<item_t> queue;
                sem_t mutex, awaiting;
                int last_id;

            public:
                async_queue() {
                    this->last_id = 0;
                    this->queue = std::queue<item_t>();
                    sem_init(&this->mutex, 0, 1);
                    sem_init(&this->awaiting, 0, 0);
                }

                void push(item_t task) {
                    sem_wait(&this->mutex);
                    this->queue.push(task);
                    sem_post(&this->mutex);
                    sem_post(&this->awaiting);
                }

                item_t pop() {
                    sem_wait(&this->awaiting);
                    sem_wait(&this->mutex);
                    item_t task = this->queue.front();
                    this->queue.pop();
                    sem_post(&this->mutex);
                    return task;
                }

                std::optional<item_t> try_pop() {
                    if(sem_trywait(&this->awaiting) == 0) {
                        sem_wait(&this->mutex);
                        item_t task = this->queue.front();
                        this->queue.pop();
                        sem_post(&this->mutex);
                        return task;
                    } else {
                        return {};
                    }
                }

                int next_id() {
                    sem_wait(&this->mutex);
                    return this->last_id++;
                    sem_post(&this->mutex);
                }
        };
}

