#include <queue>
#include <semaphore.h>
#include <optional>

namespace TaskQueue {
    template <typename task_t>
        class async_task_queue {
            private:
                std::queue<task_t> in_queue;
                std::queue<task_t> out_queue;
                sem_t in_mutex, out_mutex, in_ready, out_ready;
                int last_id;

            public:
                async_task_queue() {
                    this->last_id = 0;
                    this->in_queue = std::queue<task_t>();
                    this->out_queue = std::queue<task_t>();
                    sem_init(&this->in_mutex, 0, 1);
                    sem_init(&this->out_mutex, 0, 1);
                    sem_init(&this->in_ready, 0, 0);
                    sem_init(&this->out_ready, 0, 0);
                }

                void insert(task_t task) {
                    sem_wait(&this->in_mutex);
                    this->in_queue.push(task);
                    sem_post(&this->in_mutex);
                    sem_post(&this->in_ready);
                }

                task_t take() {
                    sem_wait(&this->in_ready);
                    sem_wait(&this->in_mutex);
                    task_t task = this->in_queue.front();
                    this->in_queue.pop();
                    sem_post(&this->in_mutex);
                    return task;
                }

                void insert_done(task_t task) {
                    sem_wait(&this->out_mutex);
                    this->out_queue.push(task);
                    sem_post(&this->out_mutex);
                    sem_post(&this->out_ready);
                }

                task_t take_done() {
                    sem_wait(&this->out_ready);
                    sem_wait(&this->out_mutex);
                    task_t task = this->out_queue.front();
                    this->out_queue.pop();
                    sem_post(&this->out_mutex);
                    return task;
                }

                std::optional<task_t> try_take_done() {
                    if (sem_trywait(&this->out_ready)) {
                        sem_wait(&this->out_mutex);
                        task_t task = this->out_queue.front();
                        this->out_queue.pop();
                        sem_post(&this->out_mutex);
                        return task;
                    } else {
                        return {};
                    }
                }

                int next_id() {
                    sem_wait(&this->in_mutex);
                    return this->last_id++;
                    sem_post(&this->in_mutex);
                }
        };
}

