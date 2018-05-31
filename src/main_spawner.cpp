#include <iostream>
#include <fstream>
#include <string>
#include "proto/neurongraph.pb.h"
  
using namespace std;
using namespace player;

int main(){
  NeuronGraph neurongraph;
  NeuronGraph_Neuron *neuron;
  NeuronGraph_Synapse *synapse;
  
  // One input neuron
  neuron = neurongraph.add_neurons();
  neuron->set_id(0);
  neuron->set_type(NeuronGraph_Neuron::INPUT);
  neuron->set_input_output_id(0);
  
  
  // One output neuron
  neuron = neurongraph.add_neurons();
  neuron->set_id(1);
  neuron->set_type(NeuronGraph_Neuron::OUTPUT);
  neuron->set_input_output_id(0);
  
  // One synapse
  synapse = neurongraph.add_synapses();
  synapse->set_id(0);
  synapse->set_presynaptic(0);
  synapse->set_postsynaptic(1);
  synapse->set_weight(0.5);
  synapse->set_enabled(true);
  
  fstream output("out.neurongraph", ios::out | ios::trunc | ios::binary);
  if (!neurongraph.SerializeToOstream(&output)) {
    cerr << "Failed to write neuron graph." << endl;
    return -1;
  }
}

