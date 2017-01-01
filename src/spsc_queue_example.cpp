
#include <iostream>
#include <thread>
#include <mutex>
#include <sys/time.h>

#include <errno.h>

#include <shm_cpp/spsc_queue.h>

const ssize_t MAX_MSG_SIZE = 100;
const ssize_t QUEUE_SIZE = 1000;
const char QUEUE_NAME [] = { "spsc_test" };

std::mutex g_lock;

void producer()
{
    g_lock.lock();
    shm_cpp::spsc_queue q(QUEUE_NAME);
    if (!q.open()) {
        std::cerr << "Unable to open queue : " << strerror(errno) << std::endl;
        return;
    }

    std::cout << "Producer successfully opened shm_cpp::spsc_queue (" << QUEUE_NAME << ")" 
              << std::endl;

    g_lock.unlock();

    char buffer[MAX_MSG_SIZE] = {"hello"};
    ssize_t blen = strlen(buffer);
    struct timeval start, end;
    gettimeofday(&start, nullptr);
    for (int i = 0; i < QUEUE_SIZE; i++) {
        while (!q.push(buffer, blen));
        //printf("push cnt %d\n", i);
    }
    gettimeofday(&end, nullptr);

    long diff = (end.tv_sec - start.tv_sec)*1000000 + (end.tv_usec - start.tv_usec);
    long lat = (diff * 1000) / 1000;
    printf("producer: %li ns\n", lat);

}

void consumer()
{
    g_lock.lock();
    shm_cpp::spsc_queue q(QUEUE_NAME);
    if (!q.open()) {
        std::cerr << "Unable to open queue : " << strerror(errno) << std::endl;
        return;
    }

    std::cout << "Consumer successfully opened shm_cpp::spsc_queue (" << QUEUE_NAME << ")"
              << std::endl;

    g_lock.unlock();

    char buffer[MAX_MSG_SIZE];
    ssize_t buf_size = 0;
    // wait for the first push before starting the clock
    while (!q.pop(buffer, &buf_size));


    struct timeval start, end;
    gettimeofday(&start, nullptr);
    for (int i = 0; i < 999 ;) {
        if (q.pop(buffer, &buf_size)) {
            ++i;
            //printf("pop cnt %d\n", i);
        }
    }
    gettimeofday(&end, nullptr);
    long diff = (end.tv_sec - start.tv_sec)*1000000 + (end.tv_usec - start.tv_usec);
    long lat = (diff * 1000) / 1000;
    printf("consumer: %li ns\n", lat);
}


int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    shm_cpp::spsc_queue q(QUEUE_NAME, QUEUE_SIZE);
    if (!q.create(MAX_MSG_SIZE)) {
        std::cerr << "Unable to create queue : " << strerror(errno) << std::endl;
        return -1;
    }

    g_lock.lock();

    std::thread producer_thr(producer);
    std::thread consumer_thr(consumer);

    sleep(1);
    g_lock.unlock();

    producer_thr.join();
    consumer_thr.join();


    q.destroy();

    return 0;
}
