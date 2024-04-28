#include "queue.h"


/********************definitions***********************************/


Queue::Queue(int size)
{
    arr = new int[size];
    capacity = size;
    begin = 0;
    end = -1;
    count = 0;
}

Queue::~Queue()
{
    delete[] arr;
}

int Queue::pop()
{
    if(isEmpty())
    {
        return 0;
    }
    int x = arr[begin];
    begin = (begin + 1) % capacity;
    count--;
    return x;
}

void Queue::push(int item)
{
    if(isFull())
    {
        return;
    }
    end = (end + 1) % capacity;
    arr[end] = item;
    count++;
}

int Queue::size()
{
    return count;
}

bool Queue::isEmpty() {
    return (size() == 0);
}

bool Queue::isFull() {
    return (size() == capacity);
}

int Queue::peek()
{
    if (isEmpty())
    {
        return 0;
    }
    return arr[begin];
}