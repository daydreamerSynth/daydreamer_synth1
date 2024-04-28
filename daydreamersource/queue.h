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
