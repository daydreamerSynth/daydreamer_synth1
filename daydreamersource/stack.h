//queu class
//constructor that constructs a member array of the desired size_t
//destructor
//push 
//pop
#include <cstddef>
class Stack
{
    int *arr;
    int capacity;
    int top;

    public:
        Stack(int size);
        ~Stack();

        void push(int);
        int pop();
        int size();
        bool isEmpty();
        bool isFull();
        int peek();

};
