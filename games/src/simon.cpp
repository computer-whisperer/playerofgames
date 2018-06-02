#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <sys/time.h>

int main() {
   Display *d;
   Window w;
   XEvent e;
   int s;

  int pid = getpid();
  timeval t;
  gettimeofday(&t, NULL);
  srand(t.tv_usec * t.tv_sec * pid);

  char order[10];
  int order_pos = 0;
  
  for (int i = 0; i < 10; i++){
    switch (rand() % 4){
      case 0:
        order[i] = 'r';
        break;
      case 1:
        order[i] = 'g';
        break;
      case 2:
        order[i] = 'b';
        break;
      case 3:
        order[i] = 'y';
        break;
    }
  }
  printf(getenv("DISPLAY"));
   d = XOpenDisplay(NULL);
   if (d == NULL) {
      fprintf(stderr, "Cannot open display\n");
      exit(1);
   }
 
   s = DefaultScreen(d);
   w = XCreateSimpleWindow(d, RootWindow(d, s), 0, 0, 200, 200, 1,
                           BlackPixel(d, s), WhitePixel(d, s));
   XSelectInput(d, w, ButtonPressMask | ButtonReleaseMask);
   XMapWindow(d, w);
   
   XColor red, blue, green, yellow, white, black, dummy;
   XAllocNamedColor(d, DefaultColormap(d, s),"red", &red,&dummy);
   XAllocNamedColor(d, DefaultColormap(d, s),"blue", &blue,&dummy);
   XAllocNamedColor(d, DefaultColormap(d, s),"green", &green,&dummy);
   XAllocNamedColor(d, DefaultColormap(d, s),"yellow", &yellow,&dummy);
   XAllocNamedColor(d, DefaultColormap(d, s),"black", &black,&dummy);
   XAllocNamedColor(d, DefaultColormap(d, s),"white", &white,&dummy);
   
   char pressed_rect = 'r'; //red, blue, green, or yellow
   int pressed_ticks = 100; // num of tics since last button press
   
   bool center_value = false; //black for bad, white for good
   
   bool last_click = false;
   bool click = false;
   while (1) {
     XSetForeground(d, DefaultGC(d, s), red.pixel);
     XFillRectangle(d, w, DefaultGC(d, s), 0, 0, 100, 100);
     XSetForeground(d, DefaultGC(d, s), green.pixel);
     XFillRectangle(d, w, DefaultGC(d, s), 100, 0, 100, 100);
     XSetForeground(d, DefaultGC(d, s), yellow.pixel);
     XFillRectangle(d, w, DefaultGC(d, s), 0, 100, 100, 100);
     XSetForeground(d, DefaultGC(d, s), blue.pixel);
     XFillRectangle(d, w, DefaultGC(d, s), 100, 100, 100, 100);
     
     if (pressed_ticks < 10){
       XSetForeground(d, DefaultGC(d, s), white.pixel);
       switch (pressed_rect){
         case 'r':
           XFillRectangle(d, w, DefaultGC(d, s), 0, 0, 100, 100);
           break;
         case 'g':
           XFillRectangle(d, w, DefaultGC(d, s), 100, 0, 100, 100);
           break;
         case 'y':
           XFillRectangle(d, w, DefaultGC(d, s), 0, 100, 100, 100);
           break;
         case 'b':
           XFillRectangle(d, w, DefaultGC(d, s), 100, 100, 100, 100);
           break;
       }
       
       
     }
     if (center_value)
      XSetForeground(d, DefaultGC(d, s), white.pixel);
    else
      XSetForeground(d, DefaultGC(d, s), black.pixel);
     XFillRectangle(d, w, DefaultGC(d, s), 80, 80, 40, 40);
     
    XFlush(d);
    XEvent event;
    if (XCheckMaskEvent(d, ButtonPressMask | ButtonReleaseMask, &event))
      switch (event.type) {
        case ButtonPress :
          click = true;
          break;
        case ButtonRelease :
          click = false;
          break;
      }

    // Handle mouse input
    Window root_return, child_return;
    int root_x_return, root_y_return, win_x_return, win_y_return;
    unsigned int mask_return;
    bool result = XQueryPointer(d, \
      w, \
      &root_return, \
      &child_return, \
      &root_x_return, \
      &root_y_return, \
      &win_x_return, \
      &win_y_return, \
      &mask_return);
    
    pressed_ticks++;

    //printf("WTF!!\n");
    //printf("%i \n", mask_return);

    //bool click = false;
    //if (mask_return&0x100) {
    // printf("Click!\n");
    //  click = true;
    //}
    if (click && !last_click) {
      pressed_ticks = 0;
      if (0 < win_x_return && win_x_return < 100 && 0 < win_y_return && win_y_return < 100)
        pressed_rect = 'r';
      else if (100 < win_x_return && win_x_return < 200 && 0 < win_y_return && win_y_return < 100)
        pressed_rect = 'g';
      else if (0 < win_x_return && win_x_return < 100 && 100 < win_y_return && win_y_return < 200)
        pressed_rect = 'y';
      else if (100 < win_x_return && win_x_return < 200 && 100 < win_y_return && win_y_return < 200)
        pressed_rect = 'b';
      else
        pressed_ticks = 100;
      if (pressed_ticks == 0) {
        if (pressed_rect == order[order_pos]) {
          center_value = true;
          order_pos++;
          if (order[order_pos] == '\0')
            order_pos = 0;
          printf("Good\n");
          fflush(stdout);
        }
        else {
          order_pos = 0;
          center_value = false;
          printf("Bad\n");
          fflush(stdout);
        }
      }
    }
    
    last_click = click;
    usleep(50000);
    
   }

   
 
   XCloseDisplay(d);
   return 0;
}
