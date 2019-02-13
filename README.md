# memory_debug
A simple way to find heap memory bugs.

When I started to teach others data structures in C++, I realized that it's very easy for beginners to write unsafe code. And once it happened, there's no good way to tell what was wrong or pin point the memory violation from a segfault. To reduce the frustration of my students, I made this file to visualize common memory bugs.

This header only file provides several macros to replace the traditional new/delete operations. To new a value, instead of:

```c++
int *p = new int(666);
delete p;
```

use:

```c++
PTR(int) p = NEW(int);
*p = 666;
DELETE(p)
```

To new an array, instead of:

```c++
int *p = new int[16];
delete [] p;
```

use:

```c++
PTR(int) p = NEW_ARRAY(int, 16);
DELETE_ARRAY(p);
```

It can detect:

1. double free;
2. access after free;
3. memory leak;
4. access violation (index out of bound)

![image](https://user-images.githubusercontent.com/3524125/52694769-a62d7780-2f1f-11e9-92c4-f3f08832fc11.png)
![image](https://user-images.githubusercontent.com/3524125/52694796-b9d8de00-2f1f-11e9-99b6-518fbae35263.png)
![image](https://user-images.githubusercontent.com/3524125/52694808-c2c9af80-2f1f-11e9-8c2a-ebff86d732cc.png)
![image](https://user-images.githubusercontent.com/3524125/52694819-c9582700-2f1f-11e9-871e-5b8a86a819a8.png)


For now it's only for instruction purpose, so the ability of the `PTR` is limited:

1. To operate on the object you have to dereference it first, which means only `(*ptr).foo()` is valid and `ptr->foo()` is not;
2. The `NEW` macro now does not support constructor parameters, which means only the classes with default constructors can be created by it.
