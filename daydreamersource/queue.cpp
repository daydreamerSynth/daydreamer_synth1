/*
*   This file is part of daydreamer_synth1.
*
*   daydreamer_synth1 is free software: you can redistribute it and/or modify it 
*   under the terms of the GNU General Public License as published by the Free Software Foundation, 
*   either version 3 of the License, or (at your option) any later version.
*
*   daydreamer_synth1 is distributed in the hope that it will be useful, 
*   but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS 
*   FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License along with 
*   daydreamer_synth1. If not, see <https://www.gnu.org/licenses/>
*/

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