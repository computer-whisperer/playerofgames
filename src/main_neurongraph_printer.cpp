#include "proto/neurongraph.pb.h"
#include <fstream>

using namespace player;
using namespace std;

int main(int argc, char * argv[]) {
  // Read the incoming neuron graph.
  NeuronGraph neurongraph;
  fstream input(argv[1], ios::in | ios::binary);
  if (!neurongraph.ParseFromIstream(&input)) {
    cerr << "Failed to parse neuron graph." << endl;
    return -1;
  }
  input.close();
  
  cout << "Name: " << neurongraph.name() << endl;
  cout << "Id: " << to_string(neurongraph.id()) << endl;
  cout << "Species: " << to_string(neurongraph.species()) << endl;
  cout << "Generation: " << to_string(neurongraph.generation()) << endl;
  cout << "Score: " << to_string(neurongraph.score()) << endl;
  
  cout << "Neurons:" << endl;
  
  for (int i = 0; i < neurongraph.neurons_size(); i++){
    NeuronGraph_Neuron neuron = neurongraph.neurons(i);
    string type = "Type: ";
    string io_id = "";
    switch (neuron.type()) {
      case NeuronGraph_Neuron::EXCITATORY:
        type += "EXCITATORY, ";
        break;
      case NeuronGraph_Neuron::INHIBITORY:
        type += "INHIBITORY, ";
        break;
      case NeuronGraph_Neuron::INPUT:
        type += "INPUT, ";
        io_id = "I/O_ID: " + to_string(neuron.input_output_id()) + ", ";
        break;
      case NeuronGraph_Neuron::OUTPUT:
        type += "OUTPUT, ";
        io_id = "I/O_ID: " + to_string(neuron.input_output_id()) + ", ";
        break;
    }
    cout << "ID: " << neuron.id() << ", " << type << io_id << endl;
  }
  
  cout << "Synapses:" << endl;
  
  for (int i = 0; i < neurongraph.synapses_size(); i++) {
    NeuronGraph_Synapse synapse = neurongraph.synapses(i);
    cout << "ID: " << synapse.id() << ", ";
    cout << "Enabled: " << synapse.enabled() << ", ";
    cout << synapse.presynaptic() << "->" << synapse.postsynaptic() <<  ", ";
    cout << "Weight: " << synapse.weight();
    cout << endl;
  }
}
