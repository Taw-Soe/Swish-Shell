# Swish Shell

A Unix-style command-line shell written in C that demonstrates process management, job control, signal handling, and file I/O.

## Overview

Swish is a command-line shell developed as part of **CSCI 4061: Introduction to Operating Systems** at the University of Minnesota. It interprets user commands and launches programs while managing foreground and background processes.

The project explores how operating systems handle process creation, terminal ownership, signals, and file descriptors—the mechanisms that make command-line environments work.

## Features

- **Command execution:** Launch external programs using `fork()` and `execvp()`.
- **Built-in commands:** Support shell operations such as changing directories.
- **Job control:** Run processes in the foreground or background, list jobs, and resume jobs.
- **Process management:** Manage child processes, process groups, and process completion.
- **Signal handling:** Handle terminal-related signals to support interactive job control.
- **I/O redirection:** Redirect standard input and output using `<`, `>`, and `>>`.
- **System-level file operations:** Use file descriptors and system calls to manage input and output.

## Technologies

- **Language:** C
- **Operating system concepts:** Processes, signals, process groups, terminal control, and file descriptors
- **System calls and APIs:** `fork()`, `execvp()`, `waitpid()`, `kill()`, `setpgid()`, `tcsetpgrp()`, `open()`, `dup2()`, and `close()`
- **Build system:** Make

## Getting Started

### Prerequisites

- A Unix-like environment, such as Linux
- GCC
- GNU Make

### Build

Clone the repository and enter the project directory:

```bash
git clone https://github.com/Taw-Soe/Swish-Shell.git
cd Swish-Shell
```

Compile the project:

```bash
make
```

This builds the `swish` executable and the supporting `slow_write` program used by the project’s test setup.

### Run

Start the shell:

```bash
./swish
```

You can then enter commands at the shell prompt.

## Usage Examples

Run an external command:

```bash
ls -l
```

Change the current directory:

```bash
cd ..
```

Redirect command output to a file:

```bash
ls > files.txt
```

Append output to an existing file:

```bash
echo hello >> output.txt
```

Run a command in the background:

```bash
sleep 10 &
```

List and manage jobs using the job-control commands supported by the shell.

## Implementation Highlights

### Process Creation and Execution

Swish creates child processes to execute external programs, allowing the shell to continue operating independently of the commands it launches.

### Foreground and Background Job Control

The shell manages process groups and terminal ownership to coordinate foreground execution and background jobs. It also handles signals associated with terminal access and suspended processes.

### File Descriptor Management

Input and output redirection are implemented using file operations and file-descriptor manipulation, allowing commands to read from files or write output to destinations other than the terminal.

## Testing

The repository includes test cases for checking shell behavior. The project Makefile provides a test target:

```bash
make test
```

Running the tests may require the `testius` testing utility used by the course test setup.

## What I Learned

- Managing processes and child-process lifecycles in C
- Using POSIX system calls to launch and control programs
- Implementing foreground and background job control
- Handling signals and terminal ownership
- Working with file descriptors and I/O redirection
- Debugging interactions between processes and the operating system

## Author

**Taw Soe**

- GitHub: [Taw-Soe](https://github.com/Taw-Soe)
- LinkedIn: [Taw Soe](https://www.linkedin.com/in/taw-soe-7941a53b2/)
