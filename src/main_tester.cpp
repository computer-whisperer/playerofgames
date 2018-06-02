#include <stdio.h>
#include <string>
#include <iostream>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <poll.h>
#include <signal.h>
#include <fcntl.h>
#include <cstring>

using namespace std;

#define READ 0

#define WRITE 1

pid_t popen2(const char *command, int *infp, int *outfp) {
    int p_stdin[2], p_stdout[2];
    pid_t pid;
    if (pipe2(p_stdin, O_NONBLOCK) != 0 || pipe2(p_stdout, O_NONBLOCK) != 0)
        return -1;
    pid = fork();
    if (pid < 0)
        return pid;
    else if (pid == 0)
    {
        close(p_stdin[WRITE]);
        dup2(p_stdin[READ], READ);
        close(p_stdout[READ]);
        dup2(p_stdout[WRITE], WRITE);
        execl("/bin/sh", "sh", "-c", command, NULL);
        perror("execl");
        exit(1);
    }
    if (infp == NULL)
        close(p_stdin[WRITE]);
    else
        *infp = p_stdin[WRITE];
    if (outfp == NULL)
        close(p_stdout[READ]);
    else
        *outfp = p_stdout[READ];
    return pid;
}



int main(int argc, char * argv[]) {
  if (argc < 4) {
    cerr << "USAGE: " << argv[0] << " NEURONGRAPH SECONDS_TRAINING SECONDS_TESTING [TESTING_DISPLAY] [--nographics]" << endl;
    exit(-1);
  }
  int training_secs = stoi(argv[2]);
  int testing_secs = stoi(argv[3]);
  
  bool graphics = true;
  string display = "";
  bool own_display = true;
  for (int i = 0; i < argc; i++) {
    if (strcmp(argv[i], "--nographics") == 0)
      graphics = false;
    if (argv[i][1] == 'd'){
      display = argv[i]+3;
      own_display = false;
    }
  }
    
  string original_display = getenv("DISPLAY");
  
  int xvfb_in, xvfb_out;
  pid_t xvfb_pid;
  if (own_display) {
    // Find unused display  
    int i = 0;
    while(display == ""){
      struct stat buffer;   
      if (stat (("/tmp/.X11-unix/X" + to_string(i)).c_str(), &buffer)){
        display = ":"+to_string(i);
        break;
      }
      i++;
    }
    
    xvfb_pid = popen2(("Xvfb " + display + " -screen 0 200x200x24").c_str(), &xvfb_in, &xvfb_out);
    usleep(1000000);
  }
  putenv((char *)("DISPLAY="+display).c_str());
  
  int simon_in, simon_out;
  pid_t simon_pid = popen2("./games/simon", &simon_in, &simon_out);
  
  putenv((char *)("DISPLAY="+original_display).c_str());
  
  int player_in, player_out;
  string g = "";
  if (!graphics)
    g = "--nographics";
  pid_t player_pid = popen2(("./player " + display + " " + argv[1] + " " + g).c_str(), &player_in, &player_out);
  
  sleep(training_secs);
  char buffer[128];
  while (read(simon_out, buffer, 128) == 128) {}
  sleep(testing_secs);
  kill(player_pid, SIGTERM);
  kill(simon_pid, SIGTERM);
  if (own_display)
    kill(xvfb_pid, SIGTERM);
  
  int score = 0;
  while(read(simon_out, buffer, 1) == 1){
    if (buffer[0] == 'G')
      score++;
  }
  cout << to_string(score) << endl;
}
