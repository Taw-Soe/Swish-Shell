#define _GNU_SOURCE

#include "swish_funcs.h"

#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "job_list.h"
#include "string_vector.h"

#define MAX_ARGS 10

int tokenize(char *s, strvec_t *tokens)
{
    // TODO Task 0: Tokenize string s
    // Assume each token is separated by a single space (" ")
    // Use the strtok() function to accomplish this
    // Add each token to the 'tokens' parameter (a string vector)
    // Return 0 on success, -1 on error

    char *token = strtok(s, " ");
    while (token != NULL)
    {
        if (strvec_add(tokens, token) != 0)
        {
            return -1; // Error adding token to vector
        }
        token = strtok(NULL, " ");
    }
    return 0;
}

int run_command(strvec_t *tokens)
{
    // TODO Task 2: Execute the specified program (token 0) with the
    // specified command-line arguments
    // THIS FUNCTION SHOULD BE CALLED FROM A CHILD OF THE MAIN SHELL PROCESS
    // Hint: Build a string array from the 'tokens' vector and pass this into execvp()
    // Another Hint: You have a guarantee of the longest possible needed array, so you
    // won't have to use malloc.

    char *argv[MAX_ARGS + 1]; // string array, +1 for NULL terminator if needed
    unsigned argc = 0;        // count of arguments
    for (unsigned i = 0; i < tokens->length; i++)
    {
        if (strcmp(strvec_get(tokens, i), "<") == 0 || strcmp(strvec_get(tokens, i), ">") == 0 || strcmp(strvec_get(tokens, i), ">>") == 0)
        {
            // Skip redirection operators and their corresponding file names
            i++; // Skip the next token (file name)
            continue;
        }
        argv[argc] = strvec_get(tokens, i);
        argc++; // Increment argument count
    }
    argv[argc] = NULL; // NULL terminate the array

    // TODO Task 3: Extend this function to perform output redirection before exec()'ing
    // Check for '<' (redirect input), '>' (redirect output), '>>' (redirect and append output)
    // entries inside of 'tokens' (the strvec_find() function will do this for you)
    // Open the necessary file for reading (<), writing (>), or appending (>>)
    // Use dup2() to redirect stdin (<), stdout (> or >>)
    // DO NOT pass redirection operators and file names to exec()'d program
    // E.g., "ls -l > out.txt" should be exec()'d with strings "ls", "-l", NULL
    int input_redirect = strvec_find(tokens, "<");
    int output_redirect = strvec_find(tokens, ">");
    int append_redirect = strvec_find(tokens, ">>");

    if (input_redirect != -1)
    {
        // Input redirection
        char *input_file = strvec_get(tokens, input_redirect + 1);
        int fd = open(input_file, O_RDONLY);
        if (fd == -1)
        {
            perror("Failed to open input file");
            return -1;
        }
        // Redirect stdin to the input file
        if (dup2(fd, STDIN_FILENO) == -1)
        {
            perror("Failed to redirect stdin");
            close(fd);
            return -1;
        }
        if (close(fd) == -1)
        {
            perror("Failed to close input file");
            return -1;
        }
    }
    if (output_redirect != -1)
    {
        // Output redirection
        char *output_file = strvec_get(tokens, output_redirect + 1);
        int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
        if (fd == -1)
        {
            perror("Failed to open output file");
            return -1;
        }
        // Redirect stdout to the output file
        if (dup2(fd, STDOUT_FILENO) == -1)
        {
            perror("Failed to redirect stdout");
            close(fd);
            return -1;
        }
        if (close(fd) == -1)
        {
            perror("Failed to close output file");
            return -1;
        }
    }
    if (append_redirect != -1)
    {
        // Append redirection
        char *output_file = strvec_get(tokens, append_redirect + 1);
        int fd = open(output_file, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
        if (fd == -1)
        {
            perror("Failed to open output file");
            return -1;
        }
        // Redirect stdout to the output file
        if (dup2(fd, STDOUT_FILENO) == -1)
        {
            perror("Failed to redirect stdout");
            close(fd);
            return -1;
        }
        if (close(fd) == -1)
        {
            perror("Failed to close output file");
            return -1;
        }
    }

    // TODO Task 4: You need to do two items of setup before exec()'ing
    // 1. Restore the signal handlers for SIGTTOU and SIGTTIN to their defaults.
    // The code in main() within swish.c sets these handlers to the SIG_IGN value.
    // Adapt this code to use sigaction() to set the handlers to the SIG_DFL value.
    // 2. Change the process group of this process (a child of the main shell).
    // Call getpid() to get its process ID then call setpgid() and use this process
    // ID as the value for the new process group ID
    struct sigaction sac;
    sac.sa_handler = SIG_DFL; // Restore default signal handling for SIGTTOU and SIGTTIN
    if (sigfillset(&sac.sa_mask) == -1)
    {
        perror("sigfillset");
        return 1;
    }
    sac.sa_flags = 0;
    if (sigaction(SIGTTIN, &sac, NULL) == -1 || sigaction(SIGTTOU, &sac, NULL) == -1)
    {
        perror("sigaction");
        return 1;
    }

    // Set the new process group ID to the process's own PID
    pid_t pid = getpid();
    if (setpgid(pid, pid) == -1)
    {
        perror("setpgid failed");
        return -1; // setpgid failed
    }
    // Finally, exec() the program with the specified arguments
    if (execvp(argv[0], argv) == -1)
    {
        perror("exec");
        return -1; // execvp failed
    }

    return 0;
}

int resume_job(strvec_t *tokens, job_list_t *jobs, int is_foreground)
{
    // TODO Task 5: Implement the ability to resume stopped jobs in the foreground
    // 1. Look up the relevant job information (in a job_t) from the jobs list
    //    using the index supplied by the user (in tokens index 1)
    //    Feel free to use sscanf() or atoi() to convert this string to an int
    // 2. Call tcsetpgrp(STDIN_FILENO, <job_pid>) where job_pid is the job's process ID
    // 3. Send the process the SIGCONT signal with the kill() system call
    // 4. Use the same waitpid() logic as in main -- don't forget WUNTRACED
    // 5. If the job has terminated (not stopped), remove it from the 'jobs' list
    // 6. Call tcsetpgrp(STDIN_FILENO, <shell_pid>). shell_pid is the *current*
    //    process's pid, since we call this function from the main shell process

    int job_index = atoi(strvec_get(tokens, 1));    // convert the job index from string to int
    if (job_index < 0 || job_index >= jobs->length) // check if job index is out of bounds
    {
        fprintf(stderr, "Job index out of bounds\n");
        return -1;
    }

    job_t *job = job_list_get(jobs, job_index);

    // if the job is in the foreground, we need to set the terminal's process group to the job's process group and wait for it to finish or stop
    if (is_foreground)
    {
        if (tcsetpgrp(STDIN_FILENO, job->pid) == -1) // set the process group of the terminal to the job's process group
        {
            perror("tcsetpgrp() failed"); // print error if tcsetpgrp fails
            return -1;
        }
        if (kill(job->pid, SIGCONT) == -1) // send job the SIGCONT signal to resume it
        {
            perror("kill() failed"); // print error if kill fails
            return -1;
        }
        int status;
        if (waitpid(job->pid, &status, WUNTRACED) == -1) // wait for the job to terminate or stop
        {
            perror("waitpid() failed"); // print error if waitpid fails
            return -1;
        }
        if (!WIFSTOPPED(status))
        {
            job_list_remove(jobs, job_index); // remove job from jobs list if it has terminated
        }
        if (tcsetpgrp(STDIN_FILENO, getpid()) == -1) // set the process group of the terminal back to the shell's process group
        {
            perror("tcsetpgrp() failed");
        }
    }

    // TODO Task 6: Implement the ability to resume stopped jobs in the background.
    // This really just means omitting some of the steps used to resume a job in the foreground:
    // 1. DO NOT call tcsetpgrp() to manipulate foreground/background terminal process group
    // 2. DO NOT call waitpid() to wait on the job
    // 3. Make sure to modify the 'status' field of the relevant job list entry to BACKGROUND
    //    (as it was STOPPED before this)
    else
    {
        kill(job->pid, SIGCONT);  // send job the SIGCONT signal to resume it
        job->status = BACKGROUND; // set job status to BACKGROUND
    }

    return 0;
}

int await_background_job(strvec_t *tokens, job_list_t *jobs)
{
    // TODO Task 6: Wait for a specific job to stop or terminate
    // 1. Look up the relevant job information (in a job_t) from the jobs list
    //    using the index supplied by the user (in tokens index 1)
    // 2. Make sure the job's status is BACKGROUND (no sense waiting for a stopped job)
    // 3. Use waitpid() to wait for the job to terminate, as you have in resume_job() and main().
    // 4. If the process terminates (is not stopped by a signal) remove it from the jobs list

    int job_index = atoi(strvec_get(tokens, 1));    // convert the job index from string to int
    if (job_index < 0 || job_index >= jobs->length) // check if job index is out of bounds
    {
        fprintf(stderr, "Job index out of bounds\n");
        return -1;
    }
    job_t *job = job_list_get(jobs, job_index); // get the job from the jobs list
    if (job->status != BACKGROUND)              // check if the job is in the background
    {
        fprintf(stderr, "Job index is for stopped process not background process\n");
        return -1;
    }
    int status;
    waitpid(job->pid, &status, WUNTRACED); // wait for the job to terminate or stop
    if (WIFSTOPPED(status))
    {
        job->status = STOPPED; // change status to STOPPED
    }
    else if (!WIFSTOPPED(status)) // check if the job has terminated (not stopped)
    {
        job_list_remove(jobs, job_index); // remove job from jobs list if it has terminated
    }

    return 0;
}

int await_all_background_jobs(job_list_t *jobs)
{
    // TODO Task 6: Wait for all background jobs to stop or terminate
    // 1. Iterate through the jobs list, ignoring any stopped jobs
    // 2. For a background job, call waitpid() with WUNTRACED.
    // 3. If the job has stopped (check with WIFSTOPPED), change its
    //    status to STOPPED. If the job has terminated, do nothing until the
    //    next step (don't attempt to remove it while iterating through the list).
    // 4. Remove all background jobs (which have all just terminated) from jobs list.
    //    Use the job_list_remove_by_status() function.
    for (unsigned i = 0; i < jobs->length; i++)
    {
        job_t *job = job_list_get(jobs, i);
        if (job->status == STOPPED)
        {
            continue; // ignore stopped jobs
        }
        int status;
        if (waitpid(job->pid, &status, WUNTRACED) == -1) // wait for the job to terminate or stop
        {
            perror("waitpid() failed"); // print error if waitpid fails
            return -1;
        }
        if (WIFSTOPPED(status))
        {
            job->status = STOPPED; // change status to STOPPED
        }
    }
    job_list_remove_by_status(jobs, BACKGROUND); // remove all background jobs from jobs list

    return 0;
}
