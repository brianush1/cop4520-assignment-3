# Problem 1

## Execution

To compile the program, run the following command in the directory of the repo:

```
g++ -O2 -o problem1 problem1.cpp
```

In order to run the program, use the command:

```
./problem1
```

## Approach

Since each operation was relatively short-lived, I opted to lock the entire linked list every time an operation is performed. This gives decent performance; the program runs in around 2.8 seconds on my machine.

## Experimental evaluation

I ran the program 4 times (output available in the files named problem1-output.X.txt; warning: the files are around 40 MB each, might crash some text editors), and checked that the number of thank yous matches the number of presents in each test case.

# Problem 2

## Execution

To compile the program, run the following command in the directory of the repo:

```
g++ -O2 -o problem2 problem2.cpp
```

In order to run the program, use the command:

```
./problem2
```

## Approach

The analysis is performed in a single thread, separate from the sensors. The analysis for each hour runs in O(nm) where n is the number of readings performed per hour, and m is the number of sensors. Each sensor running in its own thread produces readings in a loop, where for each reading, the sensor allocates a new node and appends it to a concurrent linked list that is only appended to by it, and only read from by the main analysis thread. All the operations performed in the sensor threads are lockless and performed using atomics, ensuring that sensors will have guaranteed progress. The analysis thread may occasionally spin when reading data, but that's okay since the analysis isn't time critical like the sensors are.

Since each sensor has a separate linked list, we only need to ensure that there are no race conditions between each individual sensor and the analysis thread. Since the analysis only advances the head once there is at least 60 minutes' worth of data in the linked list to analyze, there is no danger of race conditions.

## Experimental evaluation

I ran the program 4 times (output available in the files named problem2-output.X.txt), and manually went through to check if the output looks valid, and confirmed that it is.
