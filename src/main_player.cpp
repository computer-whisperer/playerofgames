
// include CARLsim user interface
#include <carlsim.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <vector>
#include <map>
#include "proto/neurongraph.pb.h"

#define GAME_X 200
#define GAME_Y 200
#define CURSOR_DOWNSCALING 10

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


int main(int argc, char **argv) {
  
  // Xlib init
  Display *infoDisplay;
  Display *gameDisplay;
  Window gameRoot;
  Window infoWindow;
  
  infoDisplay = XOpenDisplay(NULL);
  gameDisplay = XOpenDisplay(":1");
  
  int s = DefaultScreen(infoDisplay);
  
  gameRoot = XDefaultRootWindow(gameDisplay);
  
  /* create stats window */
  infoWindow = XCreateSimpleWindow(infoDisplay, RootWindow(infoDisplay, s), 10, 10, 200, 200, 1,
                           BlackPixel(infoDisplay, s), WhitePixel(infoDisplay, s));
                           
  /* map (show) the window */
  XMapWindow(infoDisplay, infoWindow);
  
  // Figure out the bit offsets of r g and b
  XImage *image = XGetImage(gameDisplay, gameRoot, 0, 0, GAME_X, GAME_Y, AllPlanes, ZPixmap);
  int red_shift = get_shift(image->red_mask);
  int green_shift = get_shift(image->green_mask);
  int blue_shift = get_shift(image->blue_mask);
  XDestroyImage(image);
  
  // Mouse button tracking
  bool last_left_click = false;
  bool last_right_click = false;
  
  
	// keep track of execution time
	Stopwatch watch;
  
  // neuron id maps -- maps a neuron id in the group to an official neuron id
  std::vector<int> input_neuron_ids = new std::vector<int>();
  std::vector<int> output_neuron_ids = new std::vector<int>();
  std::vector<int> exciatory_neuron_ids = new std::vector<int>();
  std::vector<int> inhibitory_neuron_ids = new std::vector<int>();
  
  // relates the incoming neuron ids of the presynaptic neuron to the postsynaptic neuron and contains the weight.
  std::map<int, std::map<int, float_t>> connection_map = new std::map<int, std::map<int, float_t>>();
  
  // Read the incoming neuron graph.
  NeuronGraph neurongraph;
  fstream input("in.neurongraph", ios::in | ios::binary);
  if (!neurongraph.ParseFromIstream(&input)) {
    cerr << "Failed to parse neuron graph book." << endl;
    return -1;
  }
  // Read the node list
  for (int i = 0; i < neurongrah.neurons_size(); i++){
    Neuron& neuron = neurongraph.neurons(i);
    switch (neuron.type()) {
      case Neuron::EXCIATORY:
        exciatory_neuron_ids.insert(neuron.id())
        break;
      case Neuron::INHIBITORY:
        inhibitory_neuron_ids.insert(neuron.id())
        break;
      case Neuron::INPUT:
        input_neuron_ids.insert(neuron.id())
        break;
      case Neuron::OUTPUT:
        output_neuron_ids.insert(neuron.id())
        break;
    }
  }
  // Read the connection list
  for (int i = 0; i < neurongraph.synapses_size(); i++) {
    Synapse& synapse = neurongraph.synapses(i);
    connection_map[synapse.presynaptic()][synapse.postsynaptic()] = synapse.weight();
  }
  printf("Loaded %i presynaptic neurons.", connection_map.count());
*/
  // ---------------- CONFIG STATE -------------------

	// Create a network on the CPU.
	// In order to run a network on the GPU, change CPU_MODE to GPU_MODE. However, please note that this
	// won't work if you compiled CARLsim with flag NO_CUDA=1.
	// USER mode will print status and error messages to console. Suppress these by changing mode to SILENT.
	int ithGPU = 0;
	int randSeed = 42;
	CARLsim sim("player", CPU_MODE, USER, ithGPU, randSeed);

	// Configure the network.
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
  
  sim.setSTP(ghidden, true);
#define ALPHA_LTP 0.001f
#define TAU_LTP 20.0f
#define ALPHA_LTD 0.0015f
#define TAU_LTD 20.0f
  sim.setESTDP(ghidden, true, STANDARD, ExpCurve(ALPHA_LTP/100, TAU_LTP, ALPHA_LTD/100, TAU_LTP));

	// Make synapses conductance based. Set to false for current based synapses.
	sim.setConductances(true);

	// Use the forward Euler integration method with 2 integration steps per 1 ms time step.
	sim.setIntegrationMethod(FORWARD_EULER, 2);
  
  sim.setSpikeCounter(goutx, -1);
  sim.setSpikeCounter(gouty, -1);
  sim.setSpikeCounter(goutc, -1);
    
	// ---------------- SETUP STATE -------------------
	// build the network
	watch.lap("setupNetwork");
	sim.setupNetwork();

	// Set spike monitors on both groups. Also monitor the weights between `gin` and `gout`.
	//sim.setSpikeMonitor(gin,"DEFAULT");
	//sim.setSpikeMonitor(goutx,"DEFAULT");
  //sim.setSpikeMonitor(gouty,"DEFAULT");
	//sim.setConnectionMonitor(ghidden,gout,"DEFAULT");
  

	// Setup some baseline input: Every neuron in `gin` will spike according to a Poisson process with
	// 30 Hz mean firing rate.
	PoissonRate in(GAME_X*GAME_Y);
	in.setRates(0.0f);
	sim.setSpikeRate(gin,&in);

	// ---------------- RUN STATE -------------------
	watch.lap("runNetwork");
	for (int i=0; i<1000; i++) {
    clock_t startloop = clock();
		sim.runNetwork(0,100);
    
    // Handle framebuffer input
    XImage *image = XGetImage(gameDisplay, gameRoot, 0, 0, GAME_X, GAME_Y, AllPlanes, ZPixmap);
    for (int x = 0; x < GAME_X; x++)
      for (int y = 0; y < GAME_Y; y++) {
        unsigned long pixel = XGetPixel(image, x, y);
        int red = (pixel & image->red_mask) >> red_shift;
        int green = (pixel & image->green_mask) >> green_shift;
        int blue = (pixel & image->blue_mask) >> blue_shift;
        int grey = ((0.3 * red) + (0.59 * green) + (0.11 * blue));
        unsigned long new_pixel = pixel && ~(image->red_mask | image->green_mask | image->blue_mask);
        new_pixel |= grey << red_shift | grey << blue_shift | grey << green_shift;
        XPutPixel(image, x, y, new_pixel);
        int rate = grey/30;
        in.setRate(x+y*GAME_X, rate);
      }
    sim.setSpikeRate(gin,&in);
    
    int mouse_x, mouse_y;
    bool left_click, right_click;
    
    // Neural net mouse clicks out
    int * ccounts = sim.getSpikeCounter(goutc);
    left_click = ccounts[0] > ccounts[2];
    right_click = ccounts[1] > ccounts[2];
    
    // Neural net mouse position out
    
    int * xcounts = sim.getSpikeCounter(goutx);
    int * ycounts = sim.getSpikeCounter(gouty);
    
    int xcount_total = 0;
    for (int x = 0; x < GAME_X/CURSOR_DOWNSCALING; x++) {
      xcount_total += xcounts[x];
      mouse_x += x*CURSOR_DOWNSCALING*xcounts[x];
    }
    if (xcount_total == 0)
      mouse_x = GAME_X/2;
    else
      mouse_x /= xcount_total;
    
    int ycount_total = 0;
    for (int y = 0; y < GAME_Y/CURSOR_DOWNSCALING; y++) {
      ycount_total += ycounts[y];
      mouse_y += y*CURSOR_DOWNSCALING*ycounts[y];
    }
    if (ycount_total == 0)
      mouse_y = GAME_Y/2;
    else
      mouse_y /= ycount_total;
    
    sim.resetSpikeCounter(goutx);
    sim.resetSpikeCounter(gouty);
    sim.resetSpikeCounter(goutc);
    
    // Handle mouse input
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
