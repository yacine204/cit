# cit 

A minimal version control system written in C from scratch. Tracks file changes and creates commits with diffs.

## how it works

cit tracks changes using a custom N-ary tree structure (Rose tree):

- **internal nodes** → `struct tree` (directories)
- **leaf nodes** → `struct node` (files)
- each node stores its **full content**, a **hash**, and a **diff against the previous commit**

### commits

- commits are stored as text files in `.cit/` directory with format: `<message>_<timestamp>.cit`
- each commit contains the entire file tree snapshot with:
  - **changes**: diff showing what was added (`+`) or removed (`-`) from the previous commit
  - **context**: full file content for that commit
- commits form a linked list where each commit has a reference to its parent

### usage

```bash
./main . commit "message"  # create a commit with all tracked files
```

### commit file format

See [examples/commit_diff.txt](examples/commit_diff.txt) for a detailed example of what a commit file looks like.

Commit files are human-readable text with:
- **##commit** header: metadata (timestamp, tree count, node count)
- **##node** sections: one for each file
- **##changes**: line-by-line diff from previous commit
- **##context**: full file content

### what it tracks

- all files in the repository (excluding `.cit/` and `.git/`)
- line-by-line changes between commits
- full file history in each commit snapshot
