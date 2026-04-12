# cit

a version control system written in C from scratch — no tutorials, no copying git's internals, just reasoning through the problem.

## how it works

cit tracks changes using a custom tree structure similar to a Rose tree (N-ary tree):

- **internal nodes** → `struct tree` (directories)
- **leaf nodes** → `struct node` (files)
- each node stores its content, a hash, and a diff against the previous commit

commits form a backwards linked list, where each commit points to its parent:

```mermaid
graph RL
    C3[commit 3\nHEAD] --> C2[commit 2]
    C2 --> C1[commit 1\ninitial]
    C1 --> NULL
```

each commit holds a snapshot of the tree at that point in time:


```mermaid
graph LR
    C1[commit 1 - initial] --> C2[commit 2]
    C2 --> C3[commit 3 - HEAD]

    C1 --> R1[root]
    R1 --> S1[src]
    R1 --> I1[include]
    S1 --> A1[main.c]
    I1 --> B1[tree.h]

    C2 --> R2[root]
    R2 --> S2[src]
    R2 --> I2[include]
    S2 --> A2[main.c]
    S2 --> C22[tree.c - added]
    I2 --> B2[tree.h]
    I2 --> D2[commit.h - added]

    C3 --> R3[root]
    R3 --> S3[src]
    R3 --> I3[include]
    S3 --> A3[main.c - modified]
    S3 --> C33[tree.c]
    S3 --> E3[commit.c - added]
    I3 --> B3[tree.h]
    I3 --> D3[commit.h - modified]
```