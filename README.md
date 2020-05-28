# Header only and simple drop-in libraries

Below is an overview of all the available libraries.
They are all cross-platform unless stated otherwise.

| Library                                  | Description                                    | Dependencies                |
|------------------------------------------|------------------------------------------------|-----------------------------|
| intrusive_containers.h                   | Intrusive list, hash set, dictionary.          | _none_                      |
| cache.h                                  | LRU Cache without dynamic memory allocations.  | intrusive_containers.h      |
|                                          |                                                |                             |
| file_system.h / file_system.cpp          | Dir/file listing. Simple file ext and reading. | tinydir.h                   |
|                                          |                                                |                             |
| process.h / process.cpp                  | Run a process and write/read its output.       | subprocess.h                |

# Tests

The tests can only be run on a Linux System and require CMake.

# License

MIT License
