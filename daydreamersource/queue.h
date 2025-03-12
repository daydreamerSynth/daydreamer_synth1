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

//queu class
//constructor that constructs a member array of the desired size_t
//destructor
//push 
//pop
// #include <cstddef>
class Queue
{
    int *arr;
    int capacity;
    int begin;
    int end;
    int count;  //current space usage of queue

    public:
        Queue(int size);
        ~Queue();

        void push(int);
        int pop();
        int size();
        bool isEmpty();
        bool isFull();
        int peek();

};
