
#define GAME_X 200
#define GAME_Y 200
#define CURSOR_DOWNSCALING 10

#include <carlsim.h>
#include "proto/neurongraph.pb.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <math.h>
#include <vector>
#include <map>

using namespace player;
using namespace std;

int get_shift (unsigned long mask) {
  int shift = 0;
  while (mask) {
    if (mask & 1) break;
    shift++;
    mask >>=1;
  }
  return shift;
}

// Get mouse coordinates
void coords (Display *display, int *x, int *y)
{
  XEvent event;
  XQueryPointer (display, DefaultRootWindow (display),
                 &event.xbutton.root, &event.xbutton.window,
                 &event.xbutton.x_root, &event.xbutton.y_root,
                 &event.xbutton.x, &event.xbutton.y,
                 &event.xbutton.state);
  *x = event.xbutton.x;
  *y = event.xbutton.y;
}

// Simulate mouse click
void press (Display *display, int button)
{
  // Create and setting up the event
  XEvent event;
  memset (&event, 0, sizeof (event));
  event.xbutton.button = button;
  event.xbutton.same_screen = True;
  event.xbutton.subwindow = DefaultRootWindow (display);
  while (event.xbutton.subwindow)
    {
      event.xbutton.window = event.xbutton.subwindow;
      XQueryPointer (display, event.xbutton.window,
		     &event.xbutton.root, &event.xbutton.subwindow,
		     &event.xbutton.x_root, &event.xbutton.y_root,
		     &event.xbutton.x, &event.xbutton.y,
		     &event.xbutton.state);
    }
  // Press
  event.type = ButtonPress;
  if (XSendEvent (display, PointerWindow, True, ButtonPressMask, &event) == 0)
    fprintf (stderr, "Error to send the event!\n");
  XFlush (display);
}

void release (Display *display, int button)
{
  // Create and setting up the event
  XEvent event;
  memset (&event, 0, sizeof (event));
  event.xbutton.button = button;
  event.xbutton.same_screen = True;
  event.xbutton.subwindow = DefaultRootWindow (display);
  while (event.xbutton.subwindow)
    {
      event.xbutton.window = event.xbutton.subwindow;
      XQueryPointer (display, event.xbutton.window,
		     &event.xbutton.root, &event.xbutton.subwindow,
		     &event.xbutton.x_root, &event.xbutton.y_root,
		     &event.xbutton.x, &event.xbutton.y,
		     &event.xbutton.state);
    }
  // Release
  event.type = ButtonRelease;
  if (XSendEvent (display, PointerWindow, True, ButtonReleaseMask, &event) == 0)
    fprintf (stderr, "Error to send the event!\n");
  XFlush (display);
}

// Move mouse pointer (absolute)
void move_to (Display *display, int x, int y)
{
  int cur_x, cur_y;
  coords (display, &cur_x, &cur_y);
  XWarpPointer (display, None, None, 0,0,0,0, -cur_x, -cur_y);
  XWarpPointer (display, None, None, 0,0,0,0, x, y);
  usleep (1);
}

// custom ConnectionGenerator
class MyConnection : public ConnectionGenerator {
  public:
  vector<int> *input_neuron_ids, *output_neuron_ids, *excitatory_neuron_ids, *inhibitory_neuron_ids;
  int gin, ghidden_ex, ghidden_in, gout;
  std::map<int, std::map<int, float_t>> *connection_map;
  MyConnection(){}
  
  void loadvals(int gin, int ghidden_ex, int ghidden_in, int gout, vector<int> *input_neuron_ids, vector<int> *excitatory_neuron_ids, vector<int> *inhibitory_neuron_ids,vector<int> *output_neuron_ids, std::map<int, std::map<int, float_t>> *connection_map) {
    this->input_neuron_ids = input_neuron_ids;
    this->output_neuron_ids = output_neuron_ids;
    this->excitatory_neuron_ids = excitatory_neuron_ids;
    this->inhibitory_neuron_ids = inhibitory_neuron_ids;
    this->gin = gin;
    this->ghidden_ex = ghidden_ex;
    this->ghidden_in = ghidden_in;
    this->connection_map = connection_map;
    this->gout = gout;
  }
  ~MyConnection() {}
  
  int get_training_id(int group, int id) {
    if (group == gin)
      return (*input_neuron_ids)[id];
    else if (group == ghidden_in)
      return (*inhibitory_neuron_ids)[id];
    else if (group == ghidden_ex)
      return (*excitatory_neuron_ids)[id];
    else if (group == gout)
      return (*output_neuron_ids)[id];
    return -1;
  }
  
  // the pure virtual function inherited from base class
  // note that weight, maxWt, delay, and connected are passed by reference
  void connect(CARLsim* sim, int srcGrp, int i, int destGrp, int j, float& weight, float& maxWt, float& delay, bool& connected) {
    int src_training_id = get_training_id(srcGrp, i);
    int tgt_training_id = get_training_id(destGrp, j);
    
    maxWt = 2.0f;
    delay = 1.0f;
    weight = 0.0f;
    
    if (connection_map->find(src_training_id) == connection_map->end()){
      connected = false;
      return;
    }
    if ((*connection_map)[src_training_id].find(tgt_training_id) == (*connection_map)[src_training_id].end()){
      connected = false;
      return;
    }
    connected = true;
    weight = (*connection_map)[src_training_id][tgt_training_id];
  }
};


int main(int argc, char **argv) {
  
  if (argc < 3) {
    cerr << "Usage: player DISPLAY INPUT_NEURONGRAPH_FILE [--nographics]" << endl;
    return -1;
  }
  
  bool graphics = true;
  for (int i = 0; i < argc; i++)
    if (strcmp(argv[i], "--nographics") == 0)
      graphics = false;
  

  
	// keep track of execution time
	Stopwatch watch;
  
  // neuron id maps -- maps group neuron ids to training neuron ids
  std::vector<int> input_neuron_ids;
  std::vector<int> output_neuron_ids;
  std::vector<int> excitatory_neuron_ids;
  std::vector<int> inhibitory_neuron_ids;
  
  // io neuron maps -- maps group neuron ids to io neuron ids
  std::vector<int> io_input_neuron_ids;
  std::vector<int> io_output_neuron_ids;

  
  // relates the training neuron ids of the presynaptic neuron to the postsynaptic neuron and contains the weight.
  std::map<int, std::map<int, float_t>> connection_map;
  
  // Read the incoming neuron graph.
  NeuronGraph neurongraph;
  fstream input(argv[2], ios::in | ios::binary);
  if (!neurongraph.ParseFromIstream(&input)) {
    cerr << "Failed to parse neuron graph." << endl;
    return -1;
  }
  input.close();
  // Read the node list
  for (int i = 0; i < neurongraph.neurons_size(); i++){
    NeuronGraph_Neuron neuron = neurongraph.neurons(i);
    switch (neuron.type()) {
      case NeuronGraph_Neuron::EXCITATORY:
        excitatory_neuron_ids.push_back(neuron.id());
        break;
      case NeuronGraph_Neuron::INHIBITORY:
        inhibitory_neuron_ids.push_back(neuron.id());
        break;
      case NeuronGraph_Neuron::INPUT:
        input_neuron_ids.push_back(neuron.id());
        io_input_neuron_ids.push_back(neuron.input_output_id());
        break;
      case NeuronGraph_Neuron::OUTPUT:
        output_neuron_ids.push_back(neuron.id());
        io_output_neuron_ids.push_back(neuron.input_output_id());
        break;
    }
  }
  // Read the connection list
  for (int i = 0; i < neurongraph.synapses_size(); i++) {
    NeuronGraph_Synapse synapse = neurongraph.synapses(i);
    connection_map[synapse.presynaptic()][synapse.postsynaptic()] = synapse.weight();
  }
  printf("Loaded %i excitatory neurons.\n", excitatory_neuron_ids.size());
  printf("Loaded %i inhibitory neurons.\n", inhibitory_neuron_ids.size());
  printf("Loaded %i input neurons.\n", input_neuron_ids.size());
  printf("Loaded %i output neurons.\n", output_neuron_ids.size());

  // Xlib init
  Display *infoDisplay;
  Display *gameDisplay;
  Window gameRoot;
  Window infoWindow;
  int s;
  
  if (graphics) {
    infoDisplay = XOpenDisplay(":0");
    if (infoDisplay == NULL){
      fprintf(stderr, "ERROR: Could not open the info display: %s!\n", getenv("DISPLAY"));
      exit(1);
    }
    
    s = DefaultScreen(infoDisplay);
    
    /* create stats window */
    infoWindow = XCreateSimpleWindow(infoDisplay, RootWindow(infoDisplay, s), 10, 10, 200, 200, 1,
                           BlackPixel(infoDisplay, s), WhitePixel(infoDisplay, s));
                           
    /* map (show) the window */
    XMapWindow(infoDisplay, infoWindow);
  }
  gameDisplay = XOpenDisplay(argv[1]);
  if (gameDisplay == NULL) {
    fprintf(stderr, "ERROR: Could not open the game display!\n");
    exit(1);
  }

  gameRoot = XDefaultRootWindow(gameDisplay);
  
  // Figure out the bit offsets of r g and b
  XImage *image = XGetImage(gameDisplay, gameRoot, 0, 0, GAME_X, GAME_Y, AllPlanes, ZPixmap);
  int red_shift = get_shift(image->red_mask);
  int green_shift = get_shift(image->green_mask);
  int blue_shift = get_shift(image->blue_mask);
  XDestroyImage(image);
  
  // Mouse button tracking
  bool last_left_click = false;
  bool last_right_click = false;
  

  // CONFIG CARLSIM

	// Create a network on the CPU.
	// In order to run a network on the GPU, change CPU_MODE to GPU_MODE. However, please note that this
	// won't work if you compiled CARLsim with flag NO_CUDA=1.
	// USER mode will print status and error messages to console. Suppress these by changing mode to SILENT.
	int ithGPU = 0;
	int randSeed = 42;
	CARLsim sim("player", CPU_MODE, USER, ithGPU, randSeed);

/*	// Configure the network.
	// Organize neurons on a 2D grid: A SpikeGenerator group `gin` and a regular-spiking group `gout`
	Grid3D gridIn(GAME_X, GAME_Y, 1); // pre is a framebuffer input
  
  Grid3D gridHidden(20,20,2); // 3-layer hidden network
  
	Grid3D gridOutX(GAME_X/CURSOR_DOWNSCALING,1,1); // output is two dimensions of weighted-average mouse control
  Grid3D gridOutY(1,GAME_Y/CURSOR_DOWNSCALING,1);
  Grid3D gridOutCtrl(1, 3, 1); //right and left click outputs
  
	int gin=sim.createSpikeGeneratorGroup("input", gridIn, EXCITATORY_NEURON);
  int ghidden=sim.createGroup("hidden", gridHidden, EXCITATORY_NEURON);
	int goutx=sim.createGroup("outputx", gridOutX, EXCITATORY_NEURON);
  int gouty=sim.createGroup("outputy", gridOutY, EXCITATORY_NEURON);
  int goutc=sim.createGroup("outputCtrl", gridOutCtrl, EXCITATORY_NEURON);
  
  sim.setNeuronParameters(ghidden, 0.02f, 0.2f, -65.0f, 8.0f);
	sim.setNeuronParameters(goutx, 0.02f, 0.2f, -65.0f, 8.0f);
  sim.setNeuronParameters(gouty, 0.02f, 0.2f, -65.0f, 8.0f);
  sim.setNeuronParameters(goutc, 0.02f, 0.2f, -65.0f, 8.0f);

	sim.connect(gin, ghidden, "random", RangeWeight(0.05), 0.001f);
  sim.connect(ghidden, ghidden, "gaussian", RangeWeight(0.00, 0.02, 0.05), 0.05f, RangeDelay(1), RadiusRF(5,5,1), SYN_PLASTIC);
  sim.connect(ghidden, goutx, "gaussian", RangeWeight(0.05), 1.0f, RangeDelay(1), RadiusRF(5,5,1));
  sim.connect(ghidden, gouty, "gaussian", RangeWeight(0.05), 1.0f, RangeDelay(1), RadiusRF(5,5,1));
  sim.connect(ghidden, goutc, "gaussian", RangeWeight(0.035), 1.0f, RangeDelay(1), RadiusRF(5,5,1));
 */
 
 	int gin = sim.createSpikeGeneratorGroup("input", input_neuron_ids.size(), EXCITATORY_NEURON);
  int ghidden_ex=sim.createGroup("hidden_excitatory", excitatory_neuron_ids.size(), EXCITATORY_NEURON);
  int ghidden_in=sim.createGroup("hidden_inhibitory", inhibitory_neuron_ids.size(), INHIBITORY_NEURON);
	int gout=sim.createGroup("output", output_neuron_ids.size(), EXCITATORY_NEURON);
  
  sim.setNeuronParameters(ghidden_ex, 0.02f, 0.2f, -65.0f, 8.0f);
  sim.setNeuronParameters(ghidden_in, 0.02f, 0.2f, -65.0f, 8.0f);
	sim.setNeuronParameters(gout, 0.02f, 0.2f, -65.0f, 8.0f);
  
  // create an instance of MyConnection class and pass it to CARLsim::connect
  MyConnection myConn;
  myConn.loadvals(gin, ghidden_ex, ghidden_in, gout, \
    &input_neuron_ids, \
    &excitatory_neuron_ids, \
    &inhibitory_neuron_ids, \
    &output_neuron_ids, \
    &connection_map);
  sim.connect(gin, ghidden_ex, &myConn, SYN_PLASTIC);
  sim.connect(gin, ghidden_in, &myConn, SYN_PLASTIC);
  sim.connect(ghidden_ex, ghidden_ex, &myConn, SYN_PLASTIC);
  sim.connect(ghidden_ex, ghidden_in, &myConn, SYN_PLASTIC);
  sim.connect(ghidden_in, ghidden_ex, &myConn, SYN_PLASTIC);
  sim.connect(ghidden_in, ghidden_in, &myConn, SYN_PLASTIC);
  sim.connect(ghidden_ex, gout, &myConn, SYN_PLASTIC);
  sim.connect(ghidden_in, gout, &myConn, SYN_PLASTIC);
  sim.connect(gin, gout, &myConn, SYN_PLASTIC);
  
  sim.setSTP(ghidden_in, true);
  sim.setSTP(ghidden_ex, true);

#define ALPHA_LTP 0.001f
#define TAU_LTP 20.0f
#define ALPHA_LTD 0.0015f
#define TAU_LTD 20.0f
  sim.setESTDP(ghidden_ex, true, STANDARD, ExpCurve(ALPHA_LTP/100, TAU_LTP, ALPHA_LTD/100, TAU_LTP));
  sim.setISTDP(ghidden_in, true, STANDARD, ExpCurve(ALPHA_LTP/100, TAU_LTP, ALPHA_LTD/100, TAU_LTP));
  sim.setESTDP(gout, true, STANDARD, ExpCurve(ALPHA_LTP/100, TAU_LTP, ALPHA_LTD/100, TAU_LTP));


	// Make synapses conductance based. Set to false for current based synapses.
	sim.setConductances(true);

	// Use the forward Euler integration method with 2 integration steps per 1 ms time step.
	sim.setIntegrationMethod(FORWARD_EULER, 2);
  
  sim.setSpikeCounter(gout, -1);
    
	// ---------------- SETUP STATE -------------------
	// build the network
	watch.lap("setupNetwork");
	sim.setupNetwork();

	// Set spike monitors on both groups. Also monitor the weights between `gin` and `gout`.
	//sim.setSpikeMonitor(gin,"DEFAULT");
	//sim.setSpikeMonitor(ghidden_ex,"DEFAULT");
  //sim.setSpikeMonitor(gout,"DEFAULT");
	//sim.setConnectionMonitor(ghidden,gout,"DEFAULT");
  

	// Setup some baseline input: Every neuron in `gin` will spike according to a Poisson process with
	// 30 Hz mean firing rate.
	PoissonRate in(input_neuron_ids.size());
	in.setRates(0.0f);
	sim.setSpikeRate(gin,&in);

	// ---------------- RUN STATE -------------------
	watch.lap("runNetwork");
	for (int i=0; i<10000; i++) {
    clock_t startloop = clock();
		sim.runNetwork(0,100);
    
    // Handle framebuffer input
    XImage *image = XGetImage(gameDisplay, gameRoot, 0, 0, GAME_X, GAME_Y, AllPlanes, ZPixmap);
    char framebuffer[GAME_X*GAME_Y];
    for (int x = 0; x < GAME_X; x++)
      for (int y = 0; y < GAME_Y; y++) {
        unsigned long pixel = XGetPixel(image, x, y);
        int red = (pixel & image->red_mask) >> red_shift;
        int green = (pixel & image->green_mask) >> green_shift;
        int blue = (pixel & image->blue_mask) >> blue_shift;
        int grey = ((0.3 * red) + (0.59 * green) + (0.11 * blue));
        unsigned long new_pixel = pixel && ~(image->red_mask | image->green_mask | image->blue_mask);
        new_pixel |= grey << red_shift | grey << blue_shift | grey << green_shift;
        if (graphics)
          XPutPixel(image, x, y, new_pixel);
        framebuffer[x+y*GAME_X] = grey;
      }
    
    for (int i = 0; i < input_neuron_ids.size(); i++) {
      in.setRate(i, ((int)framebuffer[io_input_neuron_ids[i]])/3);
      //printf("%i\n", ((int)framebuffer[io_input_neuron_ids[i]])/3);
    }
    
    sim.setSpikeRate(gin,&in);
    
    bool left_click, right_click;
    
    // Neural net mouse position out
    
    int * counts = sim.getSpikeCounter(gout);
    int xcount_total = 0;
    int ycount_total = 0;
    int mouse_x = 0;
    int mouse_y = 0;
    
    int right_click_count = 0;
    int left_click_count = 0;
    int null_click_count = 0;
    
    for (int i = 0; i < output_neuron_ids.size(); i++) {
      int training_id = io_output_neuron_ids[i];
      if (training_id < 20) {// X coordinate scale
        xcount_total += counts[i];
        mouse_x += training_id*10*counts[i];
      }
      else if (training_id < 40) {// Y coordinate scale
        ycount_total += counts[i];
        mouse_y += (training_id-20)*10*counts[i];
      }
      else if (training_id == 40)
        left_click_count = counts[i];
      else if (training_id == 41)
        right_click_count = counts[i];
      else if (training_id == 42)
        null_click_count = counts[i];
    }
    
    if (xcount_total == 0)
      mouse_x = GAME_X/2;
    else
      mouse_x /= xcount_total;
    
    if (ycount_total == 0)
      mouse_y = GAME_Y/2;
    else
      mouse_y /= ycount_total;
    
    // Neural net mouse clicks out
    left_click = left_click_count > null_click_count;
    right_click = right_click_count > null_click_count;
    
    sim.resetSpikeCounter(gout);
    
    // Handle mouse input
    if (graphics) {
      Window root_return, child_return;
      int root_x_return, root_y_return, win_x_return, win_y_return;
      unsigned int mask_return;
      bool overrideMouse = XQueryPointer(infoDisplay, \
        infoWindow, \
        &root_return, \
        &child_return, \
        &root_x_return, \
        &root_y_return, \
        &win_x_return, \
        &win_y_return, \
        &mask_return);
        
      if (overrideMouse && 2 < win_x_return && win_x_return < GAME_X-2 && 2 < win_y_return && win_y_return < GAME_Y-2 ) {
        left_click = false;
        right_click = false;
        if (mask_return&0x100)
          left_click = true;
        if (mask_return&0x400)
          right_click = true;
        mouse_x = win_x_return;
        mouse_y = win_y_return;
      }
    }
    mouse_x = std::max(1, std::min(mouse_x, GAME_X-2));
    mouse_y = std::max(1, std::min(mouse_y, GAME_Y-2));
    
    //printf("x:%i, y:%i\n", mouse_x, mouse_y);
    // move mouse and send click events
    move_to(gameDisplay, mouse_x, mouse_y);
    
    if (left_click && !last_left_click)
      press(gameDisplay, Button1);
    if (!left_click && last_left_click)
      release(gameDisplay, Button1);
    if (right_click && !last_right_click)
      press(gameDisplay, Button3);
    if (!right_click && last_right_click)
      release(gameDisplay, Button3);
    
    last_left_click = left_click;
    last_right_click = right_click;
    
    // Draw cursor stuff
    if (graphics) {
      unsigned long cursorcolor = image->red_mask;
      if (left_click)
        cursorcolor = image->green_mask;
      if (right_click)
        cursorcolor = image->blue_mask;
      for (int x = -1; x < 2; x++)
        for (int y = -1; y < 2; y++)
          XPutPixel(image, x+mouse_x, y+mouse_y, cursorcolor);
      
      // copy image to info display
      XPutImage(infoDisplay, infoWindow, DefaultGC(infoDisplay, s), image, 0, 0, 0, 0, GAME_X, GAME_Y);
      XDestroyImage(image);
    }

    float looptime = float_t(clock() - startloop)/CLOCKS_PER_SEC;
    if(looptime < 0.1)
      usleep(1000000*(0.1-looptime));
  }

	// Print stopwatch summary
	watch.stop();
	
   
  /* close connection to server */
  XCloseDisplay(gameDisplay);
  XCloseDisplay(infoDisplay);
  
	return 0;
}
