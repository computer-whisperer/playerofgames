#include "proto/neurongraph.pb.h"
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <math.h>
#include <vector>
#include <map>
#include <dirent.h>
#include <cstring>
#include <fstream>

using namespace std;
using namespace player;

#define MAX_POP 10

map<int, int> species_size;
map<int, int> species_fitness;
map<string, int> fitness;
map<string, NeuronGraph> population;

string test_dir;
string pop_dir;
string graveyard_dir;

void update_fitness() {
  species_fitness.clear();
  for (map<string, NeuronGraph>::iterator it=population.begin(); it!=population.end(); ++it){
    string fname = it->first;
    NeuronGraph graph = it->second;
    int species = graph.species();
    fitness[fname] = graph.score() / species_size[species];
    species_fitness[species] += fitness[fname];
  }
}

void update_population() {
  bool updated_pop = false;
  
  DIR *d;
  struct dirent *dir;
  d = opendir(pop_dir.c_str());
  if (!d) {
    cerr << "Could not open population directory!" << endl;
    exit(-1);
  }
  while ((dir = readdir(d)) != NULL) {
    if (dir->d_name[0] != '.') {
      if (population.find(dir->d_name) == population.end()){
        // New population member that we did not have before: process it!
        cout << "Loading " << dir->d_name << endl;
        fstream input(pop_dir + dir->d_name, ios::in | ios::binary);
        if (!population[dir->d_name].ParseFromIstream(&input)) {
          cerr << "Failed to parse neuron graph " << dir->d_name << endl;
          exit(-1);
        }
        if (!population[dir->d_name].has_species())
          population[dir->d_name].set_species(0);
        int species = population[dir->d_name].species();
        updated_pop = true;
        species_size[species]++;
      }
    }
  }
  closedir(d);
  
  if (updated_pop)
    update_fitness();
  updated_pop = false;
  
  // Check for overpopulation and prune.
  while (population.size() > MAX_POP) {
    int min_fitness = INT_MAX;
    string worst_agent;
    for (map<string, int>::iterator it=fitness.begin(); it!=fitness.end(); ++it){
      if (it->second < min_fitness) {
        min_fitness = it->second;
        worst_agent = it->first;
      }
    }
    if (min_fitness == INT_MAX)
      cerr << "Sum ting wong!" << endl;
    cout << "Killing " << worst_agent << endl;
    species_size[population[worst_agent].species()]--;
    population.erase(worse_agent);
    fitness.erase(worst_agent);
    updated_pop = true;
    rename(pop_dir + worst_agent, graveyard_dir + worst_agent);
  }
  
  if (updated_pop)
    update_fitness();
}



int main(int argc, char * argv[]) {
  if (argc < 4) {
    cerr << "Usage: " << argv[0] << " population_dir testing_dir graveyard_dir" << endl;
    exit(-1);
  }
  
  pop_dir = argv[1];
  test_dir = argv[2];
  graveyard_dir = argv[3];
  
  if (pop_dir.back() != '/')
    pop_dir += "/";

  if (testing_dir.back() != '/')
    testing_dir += "/";

  if (graveyard_dir.back() != '/')
    graveyard_dir += "/";
  
  update_population();  
}
