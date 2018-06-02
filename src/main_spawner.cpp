#include <iostream>
#include <fstream>
#include <string>
#include "proto/neurongraph.pb.h"
  
using namespace std;
using namespace player;

int main(int argc, char* argv[]){
  if (argc != 2) {
    printf("Usage: spawner TARGET_NEURONGRAPH_FILE\n");
    return -1;
  }
  
  NeuronGraph neurongraph;
  NeuronGraph_Neuron *neuron;
  NeuronGraph_Synapse *synapse;
  
  // Two input neurons
  neuron = neurongraph.add_neurons();
  neuron->set_id(0);
  neuron->set_type(NeuronGraph_Neuron::INPUT);
  neuron->set_input_output_id(650);
  
  neuron = neurongraph.add_neurons();
  neuron->set_id(1);
  neuron->set_type(NeuronGraph_Neuron::INPUT);
  neuron->set_input_output_id(20100);
  
  // Two hidden neurons
  
  neuron = neurongraph.add_neurons();
  neuron->set_id(2);
  neuron->set_type(NeuronGraph_Neuron::EXCITATORY);
  
  neuron = neurongraph.add_neurons();
  neuron->set_id(3);
  neuron->set_type(NeuronGraph_Neuron::INHIBITORY);
  
  // One output neuron
  neuron = neurongraph.add_neurons();
  neuron->set_id(4);
  neuron->set_type(NeuronGraph_Neuron::OUTPUT);
  neuron->set_input_output_id(40);
  
  // four synapses
  synapse = neurongraph.add_synapses();
  synapse->set_id(0);
  synapse->set_presynaptic(0);
  synapse->set_postsynaptic(2);
  synapse->set_weight(1);
  synapse->set_enabled(true);
  
  synapse = neurongraph.add_synapses();
  synapse->set_id(1);
  synapse->set_presynaptic(1);
  synapse->set_postsynaptic(3);
  synapse->set_weight(0.5);
  synapse->set_enabled(true);
  
  synapse = neurongraph.add_synapses();
  synapse->set_id(2);
  synapse->set_presynaptic(2);
  synapse->set_postsynaptic(4);
  synapse->set_weight(1);
  synapse->set_enabled(true);
  
  synapse = neurongraph.add_synapses();
  synapse->set_id(3);
  synapse->set_presynaptic(3);
  synapse->set_postsynaptic(4);
  synapse->set_weight(1);
  synapse->set_enabled(true);
  
  fstream output(argv[1], ios::out | ios::trunc | ios::binary);
  if (!neurongraph.SerializeToOstream(&output)) {
    cerr << "Failed to write neuron graph." << endl;
    return -1;
  }
}

