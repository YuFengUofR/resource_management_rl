#include <unistd.h>
#include "cpufreq.h"
/*
 * If DEBUG is defined, enable contracts and printing on dbg_printf.
 */
#ifdef DEBUG
/* When debugging is enabled, these form aliases to useful functions */
#define dbg_printf(...) printf(__VA_ARGS__)
#define dbg_requires(...) assert(__VA_ARGS__)
#define dbg_assert(...) assert(__VA_ARGS__)
#define dbg_ensures(...) assert(__VA_ARGS__)
#else
/* When debugging is disabled, no code gets generated for these */
#define dbg_printf(...)
#define dbg_requires(...)
#define dbg_assert(...)
#define dbg_ensures(...)
#endif

#define MAXLINE 50
#ifndef _SC_NPROCESSORS_ONLN
#endif

/* Function prototypes */
void eval(int argc, char **argv, char **envp, char *cmdline);
int split_cmd(const char *cmdline, char *program_line);
void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);
void sigquit_handler(int sig);

/*all the helper functions */
void redirect_io(char *infile, char *outfile);
int builtin(const char *token);
void print_job(struct job_t *job);
void print(struct cmdline_tokens * token);
void do_bg(struct cmdline_tokens * token);
void do_fg(struct cmdline_tokens * token);


const char * prompt = "~Sh>>";
Controller *controller;
typedef struct program_info_t *program_info;
struct program_info_t {
    int cpu;
    int policy;
    unsigned long freq;
    char name[MAXLINE];
};

/*
 * <Write main's function header documentation. What does main do?>
 * "Each function should be prefaced with a comment describing the purpose
 *  of the function (in a sentence or two), the function's arguments and
 *  return value, any error cases that are relevant to the caller,
 *  any pertinent side effects, and any assumptions that the function makes."
 */
int main(int argc, char **argv, char **envp)
{
    int num_cpus = sysconf( _SC_NPROCESSORS_ONLN );
    printf("The number %d of CPU available in this machine.\n", num_cpus);
    char c;
    int index = 0;
    char cmdline[MAXLINE];  // Cmdline for fgets
    char program_line[MAXLINE];  // program command for execution
    bool emit_prompt = true;    // Emit prompt (default)

    // // Install the signal handlers
    // Signal(SIGINT,  sigint_handler);   // Handles ctrl-c
    // Signal(SIGTSTP, sigtstp_handler);  // Handles ctrl-z
    // Signal(SIGCHLD, sigchld_handler);  // Handles terminated or stopped child
    // Signal(SIGTTIN, SIG_IGN);
    // Signal(SIGTTOU, SIG_IGN);

    // Signal(SIGQUIT, sigquit_handler); 

    printf("The usage of this program:\nPlease enter: \'<PROGRAM>\' <CPU> <POLICY>\n");

    controller = new Controller();

    // Execute the shell's read/eval loop
    while (true)
    {
        if (emit_prompt)
        {
            printf("%s", prompt);
            fflush(stdout);
        }

        if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
        {
            printf("ERROR: can't read the command properly.\n");
            exit(-1);
        }

        if (feof(stdin))
        {
            // End of file (ctrl-d)
            printf ("\n");
            fflush(stdout);
            fflush(stderr);
            return 0;
        }
        // Remove the trailing newline
        cmdline[strlen(cmdline)-1] = '\0';

        index = split_cmd(cmdline, program_line);
        if (index < 0) {
            ERROR_RETURN("can't parse this line.");
        }
        // Evaluate the command line
        eval(0, argv, envp, &(cmdline[index]));
        fflush(stdout);
    }
    return -1; // control never reaches here
}

int split_cmd(const char *cmdline, char *program_line) {
    char program_cmd[MAXLINE];
    int len = strlen(cmdline);
    bool start_to_copy = false;
    int cnt = 0;
    int index = -1;
    for (int i = 0; i < len; i++) {
        if (cmdline[i] == '\'' && !start_to_copy) {
            start_to_copy = true;
            printf("start_to_copy\n");
        } else if (start_to_copy && cmdline[i] != '\'') {
            program_cmd[cnt++] = cmdline[i]; 
        } else if (start_to_copy && cmdline[i] == '\'') {
            index = i;
            program_cmd[cnt] = '\0'; 
            break;
        }
    }
    if (index == -1) {
        program_line = NULL;
        return -1;
    } else {
        strcpy(program_line, program_cmd);
        return index+1;
    }
}


/* Handy guide for eval:
 *
 * If the user has requested a built-in command (quit, jobs, bg or fg),
 * then execute it immediately. Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.
 * Note: each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.
 */
void eval(int argc, char **argv, char** envp, char *cmdline) 
{
    struct program_info_t info;
    int policy = kInvalid, cpu;
    char program_name[MAXLINE];
    char policy_name[MAXLINE];

    // Parse command line
    if (sscanf(cmdline, "%d %s", &cpu, policy_name) !=  2) {
        ERROR_RETURN("Input command is invalid.\n");
    }

    for (int i = 0; i < kNumPolicy; i++) {
        if (strcmp(policy_name, policy_string[i]) == 0) {
            policy = i;
            break;
        } 
    }

    if (policy == kInvalid || cpu >= controller->get_num_cpu()) {
        ERROR_RETURN("current policy or cpu id is invalid.");
    }

    // check if built-in commands
    if (builtin(program_name)) 
    {
        // pid_t pid;
        // sigset_t prev_mask;    // save previous mask;
 
        // Sigfillset(&ourmask);  // make a full-blocked mask; 
        // Sigprocmask(SIG_BLOCK, &ourmask, &prev_mask);   // switch mask
        
        // // check child process
        // if ((pid = Fork())==0) { 
        //     // switch back the mask;
        //     Sigprocmask(SIG_SETMASK, &prev_mask, NULL);
        //     Setpgid(0, 0);        // !! change new pid !!
        //     redirect_io(token.infile, token.outfile); // change io
        //     Execve(token.argv[0], token.argv, environ);
        // }
        // // check FG or BG
        // job_state state = (parse_result == PARSELINE_FG) ? FG:BG;

        // struct job_t *job;
        // // add in job list;
        // if (!addjob(job_list, pid, state, cmdline)) {
        //     Sio_error("Error: fails to addjslafsld invalid job.\n");
        // }

        // job = getjobpid(job_list, pid);

        // if (parse_result != PARSELINE_FG) 
        // {
        //     print_job(job);   // print job's info
        //     Sigprocmask(SIG_SETMASK, &prev_mask, NULL); // BG return
        //     return;
        // }
        // // FG wait for job STP or KILL
        // while ((job!=NULL)&&(job->state == FG)) 
        // {
        //     Sigsuspend(&prev_mask);
        // }
        // Sigprocmask(SIG_SETMASK, &prev_mask, NULL);
    }
}

// /*  help func for print job's info */
// void print_job(struct job_t *job) {
//     Sio_puts("[");
//     Sio_putl(job->jid);
//     Sio_puts("]");
//     Sio_puts(" (");
//     Sio_putl(job->pid);
//     Sio_puts(") ");
//     Sio_puts(job->cmdline);
//     Sio_puts("\n");
// }


// /* change in the io direction */
// void redirect_io(char *infile, char *outfile)
// {
//     int fid_in = -1;
//     /* check input file id */
//     if (infile) {
//         fid_in = Open(infile, O_RDONLY, S_IRUSR);
//         /* switch to fid_in */
//         Dup2(fid_in, STDIN_FILENO);
//         Close(fid_in);
//     }

//     int fid_out = -1;
//     /* check output file id */
//     if (outfile) {
//         fid_out = Open(outfile, O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR);
//         /* switch to fid_out */
//         Dup2(fid_out, STDOUT_FILENO);
//         Close(fid_out);
//     }
// }


int builtin(const char* token) 
{
    return 1;
}


// void print(struct cmdline_tokens *token) 
// {

//     sigset_t prev_mask, ourmask;
//     Sigfillset(&ourmask);

//     if (token->outfile == NULL)  // check output stream, STDOUT print here
//     {
//         Sigprocmask(SIG_BLOCK, &ourmask, &prev_mask);
//         listjobs(job_list, STDOUT_FILENO);    // print command

//     } else {   // NON-STDOUT print here
//         Sigprocmask(SIG_BLOCK, &ourmask, &prev_mask);
//         int fid;
//         // open file and print
//         if ((fid=open(token->outfile, O_WRONLY, 0644)) < 0) {
//             listjobs(job_list, fid);           // print command
//             Close(fid);
//         } else {         // illegal file ID
//             if (verbose) {
//                 Sio_puts("Error:: ");
//                 Sio_puts(token->outfile);
//                 Sio_puts("invalid filename.\n");
//             }
//         }
//     }
//     Sigprocmask(SIG_SETMASK, &prev_mask, NULL);
//

// /*****************
//  * Signal handlers
//  *****************/

// /* 
//  * sigchld_handler: do all the dirty works
//  *    -- reap exited and SIGINT children
//  *    -- alarm SIGINT and SIGSTP children info 
//  *    -- change STOP state
//  *    -- delete zombie children 
//  */
// void sigchld_handler(int sig) 
// {
//     int status, olderrno = errno; // copy old errno
//     pid_t pid;
//     int jid;
//     sigset_t prev_mask, ourmask;
//     Sigfillset(&ourmask);
//     /* reap the zombie "kids" */ 
//     while ((pid = waitpid (-1, &status, WNOHANG|WUNTRACED)) > 0 ) 
//     {
//         // check if exited 
//         if (WIFEXITED(status)) { 
//             Sigprocmask(SIG_BLOCK, &ourmask, &prev_mask);    
//             jid=pid2jid(job_list, pid); // get jid info
//             deletejob(job_list, pid);   // delete jobs
//             Sigprocmask(SIG_SETMASK, &prev_mask, NULL); 
//             if (verbose) {              // print info
//                 Sio_putl(pid);
//                 Sio_puts(" : exited normally with status: ");
//                 Sio_putl(WEXITSTATUS(status));
//                 Sio_puts(".\n");
//             }
//             continue;     // check if still have zombie kids
//           /* check if there are signal terminated kids */
//         } else if (WIFSIGNALED(status)) {
//             Sigprocmask(SIG_BLOCK, &ourmask, &prev_mask);
//             jid=pid2jid(job_list, pid);  // get jid info 
//             deletejob(job_list, pid);    // delete job
//             Sio_puts("Job [");           // print info
//             Sio_putl(jid);               // using SIO for the sake of safe 
//             Sio_puts("]");
//             Sio_puts(" (");
//             Sio_putl(pid);
//             Sio_puts(") terminated by signal ");
//             Sio_putl(WTERMSIG(status));
//             Sio_puts("\n");
//             Sigprocmask(SIG_SETMASK, &prev_mask, NULL);
//             continue;    // check if still have zombie kids
//           /* check if there are signal stopped kids */
//         } else if (WIFSTOPPED(status)) {
//             Sigprocmask(SIG_BLOCK, &ourmask, &prev_mask);
//             jid=pid2jid(job_list, pid);   // get jib info
//             struct job_t *job = getjobpid(job_list, pid); // get a job struct
//             job->state = ST;      // change state
//             Sio_puts("Job [");    // print stop alarm msg
//             Sio_putl(jid);        // using SIO
//             Sio_puts("]");
//             Sio_puts(" (");
//             Sio_putl(pid);
//             Sio_puts(") stopped by signal ");
//             Sio_putl(WSTOPSIG(status));
//             Sio_puts("\n");
//             Sigprocmask(SIG_SETMASK, &prev_mask, NULL);
//             continue;
//           /* check if there are continue signal */
//         } else if (WIFCONTINUED(status)) {
//             if (verbose) {
//                 Sio_puts("one child process is set to be continued.\n");
//             }
//             continue;
//           /* alarm if there is another signal */
//         } else {
//             if (verbose) {
//                 Sio_puts("Other signal for child.\n");
//             }
//             continue;
//         } 
        
//     }

//     errno = olderrno;

// }

// /* 
//  * sigint_handler: only send SIGINT if FG job
//  */
// void sigint_handler(int sig) 
// {
//     int olderrno = errno;
//     pid_t pid;
//     sigset_t prev_mask, ourmask;

//     Sigfillset(&ourmask);
//     Sigprocmask(SIG_BLOCK, &ourmask, &prev_mask);

//     if (!(pid=fgpid(job_list))) {
//         Sigprocmask(SIG_SETMASK, &prev_mask, NULL);
//     } else {
//         Sigprocmask(SIG_SETMASK, &prev_mask, NULL);
//         Kill(-pid, SIGINT);
//     }
//     errno = olderrno;
    
// }

// /*
//  * sigtstp_handler: only send SIGSTP if FG job
//  */
// void sigtstp_handler(int sig) 
// {
//     int olderrno = errno;
//     pid_t pid;
//     sigset_t prev_mask, ourmask;

//     Sigfillset(&ourmask);
//     Sigprocmask(SIG_BLOCK, &ourmask, &prev_mask);

//     if (!(pid=fgpid(job_list))) {
//         Sigprocmask(SIG_SETMASK, &prev_mask, NULL);
//     } else {
//         Sigprocmask(SIG_SETMASK, &prev_mask, NULL);
//         Kill(-pid, SIGTSTP);
//     }
//     errno = olderrno;

// }



