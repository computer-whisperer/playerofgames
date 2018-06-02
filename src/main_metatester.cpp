#include <stdio.h>
#include <string>
#include <iostream>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <poll.h>
#include <signal.h>
#include <fcntl.h>
#include <dirent.h>
#include <cstring>
#include <fstream>
#include "proto/neurongraph.pb.h"

using namespace player;
using namespace std;

#define READ 0

#define WRITE 1

pid_t popen2(const char *command, int *infp, int *outfp) {
    int p_stdin[2], p_stdout[2];
    pid_t pid;
    if (pipe(p_stdin) != 0 || pipe(p_stdout) != 0)
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

string watch_dir(string path) {
    while (1) {
      DIR *d;
      struct dirent *dir;
      d = opendir(path.c_str());
      if (d) {
        while ((dir = readdir(d)) != NULL) {
          if (dir->d_name[0] != '.') {
            string result = dir->d_name;
            closedir(d);
            return result;
          }
        }
        closedir(d);
      }
      usleep(500000);
    }
}

#define TRIALS 30

int main(int argc, char * argv[]) {
  if (argc < 3) {
    cerr << "USAGE: " << argv[0] << " INPUT_DIR OUTPUT_DIR [--nographics]" << endl;
    exit(-1);
  }
  
  bool graphics = true;
  for (int i = 0; i < argc; i++)
    if (strcmp(argv[i], "--nographics") == 0)
      graphics = false;
  
  string input_dir = argv[1];
  string output_dir = argv[2];
  
  if (input_dir.back() != '/') {
    input_dir += "/";
  }
  if (output_dir.back() != '/') {
    output_dir += "/";
  }
  
  // Init displays
  
  pid_t xvfb_pid[TRIALS];
  int xvfb_in[TRIALS], xvfb_out[TRIALS];
  string display[TRIALS];
  for (int i = 0; i < TRIALS; i++) {
  // Find unused display  
    int j = 0;
    while(1){
      struct stat buffer;   
      if (stat (("/tmp/.X11-unix/X" + to_string(j)).c_str(), &buffer)){
        display[i] = ":"+to_string(j);
        break;
      }
      j++;
    }
    
    xvfb_pid[i] = popen2(("Xvfb " + display[i] + " -screen 0 200x200x24").c_str(), &(xvfb_in[i]), &(xvfb_out[i]));
    sleep(1);
  }
  cout << "Initialized xvfb instances" << endl;
  
  while (1) {
    
    // loop for an available neurongraph to test
    string target_neuron_graph = watch_dir(input_dir);
    cout << "Using " << target_neuron_graph << endl;
    
    pid_t trainers[TRIALS];
    int trainers_in[TRIALS], trainers_out[TRIALS];
    for (int i = 0; i < TRIALS; i++) {
      string g = "";
      if (!graphics || i > 0) g = "--nographics";
      trainers[i] = popen2(("./tester " + input_dir + target_neuron_graph + " 5 15 -d=" + display[i] + " " + g).c_str(), &(trainers_in[i]), &(trainers_out[i]));
    }
    
    // Read the incoming neuron graph.
    NeuronGraph neurongraph;
    fstream input(input_dir + target_neuron_graph, ios::in | ios::binary);
    if (!neurongraph.ParseFromIstream(&input)) {
      cerr << "Failed to parse neuron graph." << endl;
      return -1;
    }
    input.close();
    sleep(3);
    remove((input_dir + target_neuron_graph).c_str());
    
    // Get scores
    int score = 0;
    char buffer[128];
    for (int i = 0; i < TRIALS; i++) {
      buffer[0] = '\0';
      read(trainers_out[i], buffer, 128);
      for (int j = 0; j < 128; j++) {
        if (buffer[j] == '\n')
          buffer[j] = '\0';
        if (buffer[j] == '\0')
          break;
      }
      score += stoi(buffer);
    }
    neurongraph.set_score(score);
    cout << "Score recieved: " << to_string(score) << endl;
    
    fstream output(output_dir + target_neuron_graph, ios::out | ios::trunc | ios::binary);
    if (!neurongraph.SerializeToOstream(&output)) {
      cerr << "Failed to write neuron graph." << endl;
      return -1;
    }
    cout << "Waiting for next file."<< endl;
  }
  
  for (int i = 0; i < TRIALS; i++) {
    kill(xvfb_pid[i], SIGTERM);
  }
}
