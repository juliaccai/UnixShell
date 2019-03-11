    #include <stdio.h>
    #include "getword.h"
    #define MAXITEM 100 /* max number of words per line */
    
    int flag_backslash;
    
    void parsehelper();
    void parse();
    char* tilda(char* arg);
   /* 
    *  Julia Cai
    *  11/28/18
    *  
    *  Shell-like program 
    *  SPECS:
    *  - Takes in input from the command line or files if specified, and performs
    *    tasks based on the command given as the first argument. 
    *  - The arguments for that command can consist of words and 
metacharacters such 
   *    as '&', '|', '<', and '>'. These commands work similarly to how they are
   *    expected to in a real shell, except if there is more than one pipe it throws
   *    an error.
   *  - Implements hereis delimiter, reads stdin until the delimiter is found
   *  - Implements multiple pipes
   *  - Gets environment when environ is called, and can set environment
   *  - Looks up user with ~ in front
   *  WEIRD CASES:
   *  - If there are redirect cases such as "hi <&" or "hi <|" the program reports
   *    that a file name is missing, just like how traditional shells do
   *  - If there is an error with metacharacter usage on a line, then the whole line
   *  is cleared out. Either with just going to the beginning of parse, or also using
   *  a loop to traverse through each leftover word on the line
   *  EXIT:
   *  3 -> cannot open /dev/null
   *  4 -> nothing listed to redirect into the output file
   *  5 -> the file named already exists
   *  6 -> no file to redirect into
   *  7 -> could not open file
   *  8 -> nothing to pipe data into
   *  9 -> execvp failed for the newargv[0]
   *  10 -> there is already an input being pointed to for the input redirection
   */
   #define _XOPEN_SOURCE_EXTENDED 1
   #include "p2.h"
   #include <string.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include <unistd.h>
   #include <fcntl.h> /* Obtain O_* constant definitions */
   #include <sys/types.h>
   #include <string.h>
   #include <sys/wait.h>
   #include <signal.h>
   #include <sys/stat.h>
   #include <fcntl.h>
   #include <errno.h>
   #include <fcntl.h> /* Definition of AT_* constants */
   #include "CHK.h"
   #include <math.h>
   
   #define LEAVE 1
   
   //pointer to the starting location for the input and output file names, and to delimiter name
   char *ptr_input = NULL;
   char *ptr_output = NULL;
   char *ptr_delim = NULL;
   
   /* flags for indicating if the parse found an ampersand at the end of the line, or a pipe, or 
   invalid/undefine user or variable, and if a delimiter is being looked for 
*/
   int flag_amp;
   int flag_pipe;
   int flag_invalid_user;
   int flag_undefined_var;
   int flag_delim = 0;
   
   //for keeping track of and accessing the arguments inputted into p2
   char *newargv[MAXITEM];
   char stor[MAXITEM * STORAGE];
   char *arg;
   
   //Variables for storing the username lookup information
   char *dirname;
   char linecopy[STORAGE];
   
   //indicies of newargv array that are filled with words
   int length = 0;
   //last word evaluated's returned length/value
   int len;
   
   //value the fork returns to be used to see if it is the parent or child
   pid_t c_pid;
   //for file redirection and the file for storing the lines before the delimiter is found
   int inputfile, outputfile, delimfile;
   
   /******************* PARSE *********************/
   /* parse is for syntactic analysis, sets appropriate flags when getword encounters metacharacters */
  void parse()
  {
        //clears out previous run
        length = 0;
        len = 0;
        ptr_input = NULL;
        ptr_output = NULL;
        ptr_delim = NULL;
        flag_amp = 0;
        flag_pipe = 0;
        flag_invalid_user = 0;
        flag_undefined_var = 0;
        inputfile = 0;
        outputfile = 0;
  
        //assigns first index of stor array to the pointer arg
        arg = stor;
  
        /* makes a array full of the input
        incase a "$" made a string length value negative, this makes them positive */
        while ((len = getword(arg)) != 255 && len != -255)
        {
              parsehelper();
              //if encounters a newline then exits loop
              if (len == 0)
              {
                    break;               
              }
        }
        --length;
  }
  
  //evaluates each char that is inputted, special rules for certain metacharacters
  void parsehelper()
  {
        //if the previous word was a backslash then ignore if the word is a
        //metacharacter by going directly to adding to the newargv array
        if (flag_backslash == 1)
        {
              flag_backslash = 0;
              newargv[length++] = arg;
              arg += len + 1;
        }
        else if (ptr_input != NULL && ptr_delim != NULL)
        {
              fprintf(stderr, "*** ERROR: Ambiguous output when using << and < on the same line ***\n");
  
              //ignores everything on the current line
              arg += abs(len) + 1;
              while (abs(len) != 0 && abs(len) != 255)
              {
                    len = getword(arg);
                    len = abs(len);
                    arg += len + 1;
              }
              parse();
        }
        //user look up
        else if (strncmp(arg, "~", 1) == 0)
        {
  
              FILE *fp;
              char *line = NULL;
              size_t linelen = 0;
  
              char *token;
              char *search = ":";
              char *user = arg + 1;
              char file[STORAGE];
              char *final;
  
              //if is -1 means user not found
              int found;
  
              int i;
  
              /*if the ~ has no arguments puts the HOME directory path into newargv
              if the ~ in the form of ~/, puts the root directory into newargv */
              if (strlen(arg) == 1 || strncmp(arg + 1, "/", 1) == 0)
              {
                    final = strcpy(linecopy, getenv("HOME"));
                    if (strlen(arg) > 2)
                    {
                          final = strcat(final, "/");
                          final = strcat(final, (arg + 2));
                    }
                    newargv[length++] = final;
                    arg += strlen(final) + 1;
              }
              //searches the /etc/passwd for the user, to get the user's home directory
              else
              {
                    ssize_t getline(char **lineptr, size_t *n, FILE *stream);
                    strcpy(file, arg);
                    user = strtok(user, "/");
  
                    fp = fopen("/etc/passwd", "r");
  
                    //finds the user, and gets their home dir
                    while ((found = (int)getline(&line, &linelen, fp)) != -1)
                    {
                          strcpy(linecopy, line);
                          token = strtok(line, search);
  
                          if (strcmp(token, user) == 0)
                          {
                                token = strtok(linecopy, search);
                                for (i = 0; i < 5; i++)
                                {
                                      token = strtok(NULL, search);
                                }
                                break;
                          }
                    }
  
                    fclose(fp);
  
                    //if username is not found prints error and does not modify newargv
                    if (found == -1)
                    {
                          flag_invalid_user = 1;
                          fprintf(stderr, "*** ERROR: Invalid user \"%s\",user not found ***\n", user);
                    }
                    //puts the user's home dir into newargv
                    else
                    {
                          final = strcat(token, (file + strlen(user) + 1));
  
                          newargv[length++] = final;
                          arg += strlen(newargv[length - 1]) + 1;
                    }
              }
        }
        //checks if << is inputted, and assigns the next string as the delimiter
        else if (strncmp(arg, "<", 1) == 0 && strncmp(arg + 1, "<", 1) == 0)
        {
              /*only sets a delimiter value if one hasn't already been set, prints and error to stderr
              if there is more than one << symbol on the same line */
              if (ptr_delim == NULL)
              {
                    arg += abs(len) + 1;
                    len = getword(arg);
                    len = abs(len);
  
                    //if the next word after the "<<" is not a with special traits, or a '\n'
                    //or an EOF, then it is set as the delimiter pointer, otherwise an error message is printed
                    if (len == 0 || len == 255)
                    {
                          fprintf(stderr, "*** ERROR: no delimiter name was included ***\n");
                          //resets newargv array and other variables to ignore the current line
                          parse();
                    }
                    else if (strcmp(arg, ">") == 0 || strcmp(arg, "<") == 0 || strcmp(arg, "|") == 0 || strcmp(arg, "&") == 0)
                    {
                          fprintf(stderr, "*** ERROR: missing delimeter, \"%s\" was found instead ***\n", arg);
                          //ignores everything on the current line
                          arg += abs(len) + 1;
                          while (len != 0 && len != 255)
                          {
                                len = getword(arg);
                                len = abs(len);
                                arg += len + 1;
                          }
                          parse();
                    }
                    else
                    {
                          //if reading contents from stdin, prevents that portion of stdin from being exec
                          flag_delim = 1;
                          ptr_delim = arg;
                          arg += abs(len) + 1;
                    }
              }
              else
              {
                    fprintf(stderr, "*** ERROR: there is already an delimiter being pointed to \"%s\" ***\n", ptr_delim);
                    arg += abs(len) + 1;
                    //ignores everything on the current line
                    do
                    {
                          len = getword(arg);
                          arg += abs(len) + 1;
                          len = abs(len);
  
                    } while (len != 0 && len != 255);
                    parse();
              }
        }
        /* sets pointer for input redirection for the file it is redircting input from,
        if the word is "<<" does not set flag */
        else if (strcmp(arg, "<") == 0 && len != 2)
        {
              /* only sets a input file if one hasn't already been set, prints and error to stderr
              if there is more than one input redirection symbol on the same line
              the same process is done for ">" except for output redirectio*/
              if (ptr_input == NULL)
              {
                    arg += abs(len) + 1;
                    len = getword(arg);
  
                    //if there is a $ before the arg, then looks up theenvironment for the arg and has the ptr_input point to it
                    if (len < 0)
                    {
                          char *env = getenv(arg);
  
                          //if the environment variable doesn't exist,prints error
                          if (env == NULL)
                          {
                                flag_undefined_var = 1;
                                fprintf(stderr, "*** ERROR: %s: Undefinedvariable ***\n", arg);
                          }
                          else
                          {
                                ptr_input = env;
                                arg += abs(len) + 1;
                          }
                    }
                    //checking for the input file to point to
                    else
                    {
                          len = abs(len);
                          /* if the next word after the "<" is not a metacharater with special traits, or a '\n' or an 
                          EOF, then it is set as the input file pointer, otherwise an error message is printed */
                          if (len == 0 || len == 255)
                          {
                                fprintf(stderr, "*** ERROR: no file input name was included ***\n");
                                //resets newargv array and other variables to ignore the current line
                                parse();
                          }
                          else if (strcmp(arg, ">") == 0 || strcmp(arg, "<") == 0 || strcmp(arg, "|") == 0 || strcmp(arg, "&") == 0)
                          {
                                fprintf(stderr, "*** ERROR: missing name for file input name, \"%s\" was found instead ***\n", arg);
                                //ignores everything on the current line
                                arg += abs(len) + 1;
                                while (len != 0 && len != 255)
                                {
                                      len = getword(arg);
                                      len = abs(len);
                                      arg += len + 1;
                                }
                                parse();
                          }
                          else
                          {
                                ptr_input = arg;
                                arg += abs(len) + 1;
                          }
                    }
              }
              else
              {
                    fprintf(stderr, "*** ERROR: there is already an input being pointed to \"%s\" ***\n", ptr_input);
  
                    //ignores everything on the current line
                    arg += abs(len) + 1;
                    while (len != 0 && len != 255)
                    {
                          len = getword(arg);
                          len = abs(len);
                          arg += len + 1;
                    }
                    parse();
              }
        }
        //sets pointer output redirection for the file it is redirecting output from
        else if (strcmp(arg, ">") == 0)
        {
              if (ptr_output == NULL)
              {
                    arg += abs(len) + 1;
                    len = getword(arg);
  
                    //if there is a $ before the arg, then looks up the environment for the arg and and has the ptr_output point to it
                    if (len < 0)
                    {
                          char *env = getenv(arg);
  
                          //if the environment variable doesn't exist, prints error
                          if (env == NULL)
                          {
                                flag_undefined_var = 1;
                                fprintf(stderr, "*** ERROR: %s: Undefined variable ***\n", arg);
                          }
                          else
                          {
                                ptr_output = env;
                                arg += abs(len) + 1;
                          }
                    }
                    //checking for the output file to point to
                    else
                    {
                          len = abs(len);
  
                          //same reason as for input redirection
                          if (len == 0 || len == 255)
                          {
                                fprintf(stderr, "*** ERROR: no file output name was included ***\n");
                                //resets newargv array and other variables to ignore the current line
                                parse();
                          }
                          else if (strcmp(arg, ">") == 0 || strcmp(arg, "<") == 0 || strcmp(arg, "|") == 0 || strcmp(arg, "&") == 0)
                          {
                                fprintf(stderr, "*** ERROR: missing name for file output, \"%s\" was found instead  ***\n", arg);
                                //ignores everything on the current line
                                arg += len + 1;
                                while (len != 0 && len != 255)
                                {
                                      len = getword(arg);
                                      len = abs(len);
                                      arg += len + 1;
                                }
                                parse();
                          }
                          else
                          {
                                ptr_output = arg;
                                arg += len + 1;
                          }
                    }
              }
              else
              {
                    fprintf(stderr, "*** ERROR: there is already an output file being pointed to \"%s\" ***\n", ptr_output);
                    ptr_output = NULL;
                    //ignores everything on the current line
                    arg += abs(len) + 1;
                    while (len != 0 && len != 255)
                    {
                          len = getword(arg);
                          len = abs(len);
                          arg += len + 1;
                    }
                    parse();
              }
               
        }
        //sets flag for "|" and stores the location of the second process it will pipe into as the flag value
        //if there is more than one pipe, prints an error to stderr
        else if (strcmp(arg, "|") == 0)
        {
              newargv[length++] = NULL;
              flag_pipe++;
        }
        //maybe change it to put the NULL in the newargv when there is and & 

        //sets flag for "&" if it is the last word in a line
        else if (strcmp(arg, "&") == 0)
        {
              //initially puts "&" in newargv array in case it is not the last word on a line
              newargv[length] = arg;
              arg += abs(len) + 1;
              len = getword(arg);
  
              if (len == 255 || len == 0)
              {
                    //removes "&" from newargv array if is the last word on a line and
                    //sets the flag to indicate the line is going to be a background process
                    newargv[length] = NULL;
                    flag_amp = 1;
              }
              else
              {
                    //if "&" is not the last word in a line, the previous word gotten is
                    //able to go through all the checks again to see if it is a word or metacharacter
                    length++;
                    parsehelper();
              }
        }
        //terminates the newargv array when a newline or ';' is encountered
        else if (len == 0)
        {
              newargv[length++] = NULL;
        }
        else
        {
              //stores the location/address of environment variable in newargv or prints error
              if (len < 0)
              {
                    char *env = getenv(arg);
                    if (env == NULL)
                    {
                          flag_undefined_var = 1;
                          fprintf(stderr, "*** ERROR: %s: Undefined variable ***\n", arg);
                    }
                    else
                    {
                          newargv[length++] = env;
                          arg += strlen(env) + 1;
                    }
              }
              //stores the location/address of the word in newargv
              else
              {
                    
                    newargv[length++] = arg;
                    arg += abs(len) + 1;
              }
        }
  }
  
  /******************* END OF PARSE *********************/
  
  /******************* PIPE PROCESS *********************/
  
  //executes the input from the left side of the pipe and redirects the output to the arguments
  //on the right side of the pipe then executes the command and arguments on 
 right side of the pipe
  int pipe_helper(int in, int out, char *process[])
  {
        pid_t pid;
  
        //clears before forking
        fflush(stdout);
        fflush(stderr);
  
        /* forks again, so if the process is the grandchild then it 
        only writes into the pipe if the input side is open, and
        only closes the grandchild's writing side of the pipe since
        the close(fd[1]) is called since the process is done writing */
        if ((pid = fork()) == 0)
        {
  
              if (in != 0)
              {
                    dup2(in, 0);
                    close(in);
              }
  
              if (out != 1)
              {
                    dup2(out, 1);
                    close(out);
              }
              //executes process1, the left side of the pipe
              return execvp(process[0], process);
        }
        //pid of the grandchild
        return pid;
  }
  
  int pipe_fork()
  {
        int i;
        int in, fd[2];
        char *process[STORAGE];
        int k;
        int j;
  
        //the first process takes its input from the original file descriptor 0
        in = 0;
  
        //works for multiple pipes, loops for each process in the pipeline, except for the last pipe process
        for (i = 0; i < flag_pipe; ++i)
        {
              pipe(fd);
  
              j = 0;
  
              //gets the current process to be put in the pipe
              while (newargv[k] != NULL)
              {
                    process[j++] = newargv[k++];
              }
              k++;
              process[j] = NULL;
  
              //since f[1] is the write end of the pipe, the output from from the previous iteration
              pipe_helper(in, fd[1], process);
  
              //the write end of the pipe is not needed as the grandchild in this case will write here
              close(fd[1]);
  
              // keeps the read end of the pipe, the second process will read from here
              in = fd[0];
        }
  
        j = 0;
        //gets the last process in the pipeline to be execvp
        while (newargv[k] != NULL)
        {
              process[j++] = newargv[k++];
        }
        k++;
        process[j] = NULL;
  
        /* last stage of the pipeline: sets the stdin be the read end of the previous pipe
        and output to the original file descriptor 1 */
        if (in != 0)
              dup2(in, 0);
  
        /* executes the last process, the right side of the pipe*/
        return execvp(process[0], process);
  }
  
  int pipe_main()
  {
        /* calls pipe function and returns the 
        final result of piping */
        return pipe_fork();
  }
  
  /******************* END OF PIPE PROCESS *********************/
  
  /******************* SIGHANDLER PROCESS *********************/
  
  // signalhandler just catches the signal
  int complete = 0;
  
  void myhandler(int signum)
  {
        complete = 1;
  }
  
  /******************* END OF SIGHANDLER PROCESS *********************/
  
  main()
  {
        char *tail = "";
        int killpg(int pgrp, int sig);
  
        (void)signal(SIGTERM, myhandler);
  
        //sets group to the process id value
        if (setpgid(getpid(), 0) != 0)
              fprintf(stderr, "***** ERROR: setpgid() error *****");
  
        for (;;)
        {
  
              //issues PROMPT, includes tail of the director in the front of the prompt name when cd is called
              printf("%s:570: ", tail);
  
              //breaks out of prompting if currently evaluating the input from stdin for the delimiter (hereis)
              if (flag_delim == 1)
              {
                    break;
              }
  
              //calls parse function which sets [global] flags as needed;
              parse();
  
              //makes the prog prompt the user again since there is an input redirection without an executable file
              if (length == 0 && ptr_input != NULL)
              {
                    fprintf(stderr, "*** ERROR: no executable specified to redirect into ***\n");
                    ptr_input == NULL;
                    continue;
              }
  
              //makes the prog prompt the user again since there is an output redirection without and executable file
              if (length == 0 && ptr_output != NULL)
              {
                    fprintf(stderr, "*** ERROR: no executable specified to redirect to the output file ***\n");
                    ptr_output == NULL;
                    continue;
              }
  
              //breaks if the first word is EOF, if first word is EOF then the length is set to -1
              if (length == -1)
                    break;
              //continues if there is an empty line
              if (length == 0)
                    continue;
  
              //changes the directory, if 'cd' is the first command
              if (strcmp(newargv[0], "cd") == 0)
              {
                    char *hdir;
                    char cwd[STORAGE];
  
                    hdir = getenv("HOME");
  
                    //if 'cd' is the only command on the line, changes directory to the home directory
                    if (newargv[1] == NULL)
                    {
                          char *token;
                          char *search = "/";
                          chdir(hdir);
  
                          //if the home directory is the root directory, include "/" in front of the prompt
                          if (strcmp(hdir, "/") == 0)
                          {
                                tail = "/";
                          }
                          //gets the current working directory, sets the tail of the directory name to be later put in front of the prompt
                          else
                          {
                                (void)getcwd(cwd, sizeof(cwd));
  
                                token = strtok(cwd, search);
  
                                while (token != NULL)
                                {
                                      tail = token;
                                      token = strtok(NULL, search);
                                }
                          }
                    }
                    //if there are more than one arguments (not including 'cd') prints an error
                    else if (newargv[2] != NULL)
                    {
                          fprintf(stderr, "*** ERROR: cannot modify directory, more than 2 args listed ***\n");
                    }
                    //changes the directory to what the argument listed directly after the 'cd'
                    else
                    {
                          //if "cd /" is inputted, then changes to the root dir and makes the prompt "/:570:"
                          if (strcmp(newargv[1], "/") == 0)
                          {
                                tail = "/";
                                chdir(newargv[1]);
                          }
                          //changes directory to the argument after "cd " if the directory is valid
                          else if (chdir(newargv[1]) == 0)
                          {
                                char *token;
                                char *search = "/";
  
                                (void)getcwd(cwd, sizeof(cwd));
  
                                token = strtok(cwd, search);
  
                                while (token != NULL)
                                {
                                      tail = token;
                                      token = strtok(NULL, search);
                                }
                          }
                          else
                                fprintf(stderr, "*** ERROR: no file/directory of the name \"%s\" exists ***\n", newargv[1]);
                    }
                    continue;
              }
  
              //changes the environment variable specified or gets it
              if (strcmp(newargv[0], "environ") == 0)
              {
                    char *env;
                    int setenv(const char *envname, const char *envval, int overwrite);
                    env = getenv("HOME");
  
                    if (newargv[1] == NULL)
                    {
                          fprintf(stderr, "*** ERROR: cannot modify environ, no args listed ***\n");
                    }
                    //if 'cd' is the only command on the line, changes directory to the home directory
                    else if (newargv[2] == NULL)
                    {
                          env = getenv(newargv[1]);
                          if (env == NULL)
                          {
                                printf("\n");
                          }
                          else
                                printf("%s\n", env);
                    }
                    //if there are more than two arguments (not including 'environ') prints an error
                    else if (newargv[3] != NULL)
                    {
                          fprintf(stderr, "*** ERROR: cannot modify environ, more than 2 args listed ***\n");
                    }
                    //changes the directory to what the argument listed directly after the 'cd'
                    else
                    {
                          setenv(newargv[1], newargv[2], 1);
                    }
                    continue;
              }
  
              //prevents fork if there is an invalid user lookup with ~
              if (flag_invalid_user == 1 || flag_undefined_var == 1)
              {
                    continue;
              }
  
              //empty buffer for children to inherit clean buffer
              fflush(stdout);
              fflush(stderr);
  
              //value the fork returns to be used to see if it is the parent or child
              if ((c_pid = fork()) == 0)
              {
  
                    //sets stdin to go to /dev/null by default if "&" is at the end of the line and there is no input redirection
                    if (flag_amp == 1 && ptr_input == NULL)
                    {
                          int dn_output;
                          if ((dn_output = open("/dev/null", O_RDONLY)) < 0)
                          {
                                fprintf(stderr, "*** ERROR: cannot open \"/dev/null\" ***\n");
                                exit(3);
                          }
                          dup2(dn_output, STDIN_FILENO);
                          close(dn_output);
                    }
  
                    //implements hereis, checks for input for the delimiter that was placed right after "<<"
                    if (ptr_delim != NULL)
                    {
                          int fileno(FILE * stream);
                          FILE *df;
  
                          char val[2] = {'\0', '\0'};
                          int equal = 1;
                          char oldvals[STORAGE];
                          int i;
  
                          df = fopen("delimfile", "ab+");
  
                          /* gets each individual char from input, and checks if consecutive chars are the delimiter,
                           otherwise puts the chars into a file that is later redirected to act as STDIN */
                          while (((val[0] = fgetc(stdin)) != EOF) && equal == 1)
                          {
                                //checks if the char on STDIN is equal to the first char on the delimiter
                                if (ptr_delim[0] == val[0])
                                {
                                      /* saves the char into a string, just incase the string is not the delimiter and it needs to 
                                      be put into a file */
                                      strcpy(oldvals, val);
                                      equal = 0;
  
                                      /* continues checking if the string is equal to the delimiter, if it is breaks out of loop
                                      and marks that the string is equal to the delimiter. If not equal, then the chars are put
                                      into the file */
                                      for (i = 1; i < (int)strlen(ptr_delim) + 1; i++)
                                      {
                                            if (i == (int)strlen(ptr_delim))
                                            {
                                                  val[0] = fgetc(stdin);
                                                  if (strncmp("\n", val, 1) != 0)
                                                  {
                                                        fprintf(df, oldvals);
                                                        equal = 1;
                                                        break;
                                                  }
                                            }
                                            else if (ptr_delim[i] != (val[0] = fgetc(stdin)))
                                            {
                                                  fprintf(df, oldvals);
                                                  equal = 1;
                                                  break;
                                            }
                                            strcat(oldvals, val);
                                      }
                                }
                                //if the delimiter is found, breaks out of reading STDIN for hereis/delimiter finder
                                if (equal == 0)
                                {
                                      break;
                                }
  
                                //puts the char into the file
                                fputc(val[0], df);
                          }
  
                          fclose(df);
                          df = fopen("delimfile", "r");
  
                          if (length == 0)
                          {
                                fprintf(stderr, "*** ERROR: no file/location to redirect into ***\n");
                                continue;
                          }
  
                          //puts the file to be read as STDIN, and closes as well as removes the file
                          dup2(fileno(df), STDIN_FILENO);
                          close(fileno(df));
                          remove("delimfile");
                          ptr_delim = NULL;
  
                          //indicates that the delimiter looking is over, so can continue to prompt the user for input again
                          flag_delim = 0;
                    }
  
                    /* redirects stdout is instead redirected into the specified output file, 
                    does not overwrite the file if it already exists */
                    if (ptr_output != NULL)
                    {
                          if ((outputfile = open(ptr_output, O_CREAT | O_EXCL | O_WRONLY, S_IRUSR | S_IWUSR)) < 0)
                          {
                                fprintf(stderr, "*** ERROR: the file named \"%s\" already exists ***\n", ptr_output);
                                ptr_output = NULL;
                                exit(5);
                          }
                          else
                          {
                                dup2(outputfile, STDOUT_FILENO);
                                close(outputfile);
                                ptr_output = NULL;
                          }
                    }
  
                    //redirects input to come from the input file specified instead of stdin
                    if (ptr_input != NULL)
                    {
                          //prints error if only a input redirect symbol and file that is being redirected is listed
                          if (length == 1)
                          {
                                fprintf(stderr, "*** ERROR: no file to redirect into ***\n");
                                exit(6);
                          }
                          if ((inputfile = open(ptr_input, O_RDONLY)) < 0)
                          {
                                fprintf(stderr, "*** ERROR: could not open file named \"%s\" ***\n", ptr_input);
                                ptr_input = NULL;
                                exit(7);
                          }
                          else
                          {
                                dup2(inputfile, STDIN_FILENO);
                                close(inputfile);
                                ptr_input = NULL;
                          }
                    }
  
                    //runs pipe command(s) on the arguments in argv
                    if (flag_pipe > 0)
                    {
                          /*if the pipe is set for the last position inf the newargv, then throw an
                          error since there is nothing to pipe the data into */
                          if (newargv[length - 1] == NULL)
                          {
                                fprintf(stderr, "*** ERROR: nothing to pipe data into ***\n");
                                exit(8);
                          }
                          pipe_main();
                    }
  
                    //executes first argument on the line (in the newargv array)
                    if (execvp(newargv[0], newargv) < 0)
                    {
                          /*if the execvp() failed print an error message;*/
                          fprintf(stderr, "*** ERROR: execvp failed for \"%s\" ***\n", newargv[0]);
                          exit(9);
                    }
              }
  
              /* runs if there is an '&' at the end of a line, prints the child's pid, and 
              runs the loop again so the user can be prompted again, even before the background 
              process finishes */
              if (flag_amp == 1)
              {
                    printf("%s [%d]\n", *newargv, c_pid);
                    flag_amp = 0;
                    continue;
              }
              else
              {
                    //find the first child; continue reaping children until the second child is found
                    for (;;)
                    {
                          pid_t find_pid;
                          find_pid = wait(NULL);
                          if (find_pid == c_pid)
                          {
                                break;
                          }
                    }
              }
        }
  
        // Terminate any children that are still running.
        killpg(getpgrp(), SIGTERM);
        printf("p2 terminated.\n");
        exit(0);
  }
