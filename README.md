
Concurrency and lock free primitives and concurrent algorithms/data structures
==========

  

This repo contains some of the tasks involving concurrency I made during the Advanced C++ course at Higher School of Economics.

  

## List of stuff presented here:

- [hash-table](./hash-table/concurrent_hash_map.h) - hash table that can work concurrently with multiple keys, while a basic implimitation by mutex + std::unordered_map() can not work in concurrency correctly with count of keys greater than one 

- [buffered-channel](./buffered-channel/buffered_channel.h) - buffered channel from Go language

- [rw-lock](./rw-lock/rw_lock.h) - syncronization primitive that enables "reading" without locking and locks only if it is writing, implemented using condition variables

- [semaphore](./semaphore/sema.h) - implementation of semaphore using condition variables

- [timerqueue](./timerqueue/timerqueue.h) - a priority queue for objects scheduled to perform actions at clock times

- [unbuffered-channel](./unbuffered-channel/unbuffered_channel.h) - unbuffered channel from Go language

- [fast-queue](./fast-queue/mpmc.h) - Multiple-reader multiple-writer lock-free bounded queue

- [futex](./futex/mutex.h) - Mutex using Linux' "futex" and specific syscalls, implemented using lock-free paradigm

- [mpsc-stack](./mpsc-stack/mpsc_stack.h) - Multiple-reader single-writer lock-free stack

- [rw-spinlock](./rw-spinlock/rw_spinlock.h) - Like rw-lock, but it is lock-free