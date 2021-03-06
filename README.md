Multiple Producer Single Consumer FIFO

A C implementation of a MPSC FIFO, this implementation
is wait free/thread safe. This algorithm is from Dimitry
Vyukov's non intrusive MPSC code here:
  http://www.1024cores.net/home/lock-free-algorithms/queues/non-intrusive-mpsc-node-based-queue

The fifo has a head and tail, the elements are added
to the head of the queue and removed from the tail.
To allow for a wait free algorithm a stub element is used
so that a single atomic instruction can be used to add and
remove an element. Therefore, when you create a queue you
must pass in an areana which is used to manage the stub.

A consequence of this algorithm is that when you add an
element to the queue a different element is returned when
you remove it from the queue. Of course the contents are
the same but the returned pointer will be different.

Prerequisites:
 (meson)[https://github.com/mesonbuild/meson]

Build:
```
mkdir build
cd build
meson ..
ninja
```

Test:
```
./test/testmpscfifo
```
