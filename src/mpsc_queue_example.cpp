
#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <sys/time.h>

#include <errno.h>

#include <shm_cpp/mpsc_queue.h>

const ssize_t MAX_MSG_SIZE = 100;
const ssize_t QUEUE_SIZE = 1000;
const char QUEUE_NAME [] = { "mpsc_test" };
const ssize_t NUM_THREADS = 4;

std::mutex g_lock;

void producer()
{
    g_lock.lock();
    shm_cpp::mpsc_queue q(QUEUE_NAME);
    if (!q.open()) {
        std::cerr << "Unable to open queue : " << strerror(errno) << std::endl;
        return;
    }

    std::cout << "Producer successfully opened shm_cpp::mpsc_queue (" << QUEUE_NAME << ")" 
              << std::endl;

    g_lock.unlock();

    char buffer[MAX_MSG_SIZE] = {"hello"};
    ssize_t blen = strlen(buffer);
    struct timeval start, end;
    gettimeofday(&start, nullptr);
    for (int i = 0; i < (QUEUE_SIZE / NUM_THREADS); i++) {
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
    shm_cpp::mpsc_queue q(QUEUE_NAME);
    if (!q.open()) {
        std::cerr << "Unable to open queue : " << strerror(errno) << std::endl;
        return;
    }

    std::cout << "Consumer successfully opened shm_cpp::mpsc_queue (" << QUEUE_NAME << ")"
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

    //printf("sizeof(shm_cpp::mpsc_queue::header) = %lu\n", sizeof(shm_cpp::mpsc_queue::header));
    //return -1;

    shm_cpp::mpsc_queue q(QUEUE_NAME, QUEUE_SIZE);
    if (!q.create(MAX_MSG_SIZE)) {
        std::cerr << "Unable to create queue : " << strerror(errno) << std::endl;
        return -1;
    }

    g_lock.lock();

    std::thread consumer_thr(consumer);
    std::vector<std::thread> producer_thrds;
    for (int i = 0; i < NUM_THREADS; i++) {
        producer_thrds.push_back(std::thread(producer));
    }


    sleep(1);
    g_lock.unlock();

    for (auto& thrd : producer_thrds) {
        thrd.join();
    }
    consumer_thr.join();


    q.destroy();

    return 0;
}
