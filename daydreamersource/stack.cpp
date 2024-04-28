#include "stack.h"
/********************definitions***********************************/

Stack::Stack(int size)
{
    arr = new int[size];
    capacity = size;
    top = -1;
}

Stack::~Stack()
{
    delete[] arr;
}

int Stack::pop()
{
    if(isEmpty())
    {
        return 0;
    }
    return arr[top--];
}

void Stack::push(int item)
{
    if(isFull())
    {
        return;
    }
    arr[++top] = item;
}

int Stack::size()
{
    return top + 1;
}

bool Stack::isEmpty() {
    return size() == 0;
}

bool Stack::isFull() {
    return size() == capacity;
}

int Stack::peek()
{
    if (isEmpty())
    {
        return 0;
    }
    else
    {
        return arr[top];
    }
}