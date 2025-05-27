#ifndef _BOUNDED_QUEUE_HPP
#define _BOUNDED_QUEUE_HPP

#include "Constants.h"

#include <queue>

template <typename T>
class BoundedQueue {
private:
  std::queue<T> q;
  uint16_t capacity;

public:
  BoundedQueue() : capacity(NUM_SAMPLES) {}
  BoundedQueue(uint16_t capacity) : capacity(capacity) {}

  // Enqueue an item into the queue. If the queue is full, remove the oldest item.
  // Returns the removed item if the queue was full, otherwise returns NULL.
  T enqueue(const T& item) {
    T front = NULL;
    if (isFull()) {
      front = q.front();
      q.pop();
    }
    q.push(item);
    return front;
  }

  // Dequeue an item from the queue. Returns NULL if the queue is empty.
  T dequeue() {
    if (isEmpty()) {
      return NULL;
    }
    T item = q.front();
    q.pop();
    return item;
  }

  // Peek at the front item of the queue without removing it. Returns NULL if the queue is empty.
  T peek() const {
    if (isEmpty()) {
      return NULL;
    }
    return q.front();
  }

  // Check if the queue is full.
  bool isFull() const {
    return q.size() == capacity;
  }

  // Check if the queue is empty.
  bool isEmpty() const {
    return q.empty();
  }

  // Get the current size of the queue.
  uint16_t size() const {
    return q.size();
  }

  // Get the maximum capacity of the queue.
  uint16_t getCapacity() const {
    return capacity;
  }

  // Clear the queue.
  void clear() {
    while (!q.empty()) {
      q.pop();
    }
  }
};

#endif  // _BOUNDED_QUEUE_HPP
