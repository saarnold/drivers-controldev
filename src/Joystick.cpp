
#include "Joystick.hpp"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/joystick.h>
#include <errno.h>
#include <stdio.h>

namespace controldev
{

    Joystick::Joystick() : initialized(false), deadspot(false), deadspot_size(0) {
	axes = 0;
	buttons = 0;
	fd = -1;
    }

    bool Joystick::init(std::string const& dev) {
      initialized = false;

      if(axes) {
	delete[] axes;
	axes = 0;
      }
      
      if(buttons) {
	delete[] buttons;
	buttons = 0;
      }
      
      if(fd != -1)
	close(fd);

      if ((fd = open(dev.c_str(),O_RDONLY | O_NONBLOCK)) < 0) {
        //std::cout << "Warning: could not initialize joystick.\n";
	fd = -1;
        return false;
      } 

      if(ioctl(fd, JSIOCGAXES, &nb_axes) == -1) {
        perror("axes");
        return false;
      }
      if(ioctl(fd, JSIOCGBUTTONS, &nb_buttons) == -1) {
        perror("button");
        return false;	
      }
      
      char name_char[50];
      
      if(ioctl(fd, JSIOCGNAME(50), name_char) == -1) {
        perror("name");
        return false;	
      }
      
      name = std::string(name_char);

      std::cout << "Axes: " << (int) nb_axes <<" Buttons: " << (int) nb_buttons << " Name: " << name <<std::endl;

      axes = new int[nb_axes];
      buttons = new int[nb_buttons];
      
      for(int i = 0; i < nb_buttons; i++) {
        buttons[i] = 0;
      }

      for(int i = 0; i < nb_axes; i++) {
        axes[i] = 0;
      }

      initialized = true;

      return true;
    }

      
    void Joystick::setDeadspot(bool onOff, double size) {
      deadspot = onOff;
      deadspot_size = size;
    }

    bool Joystick::updateState() {
      struct js_event mybuffer[64];
      int n, i;

      if (!initialized)
        return false;

      n = read (fd, mybuffer, sizeof(struct js_event) * 64);
      if (n != -1) {
        for(i = 0; i < n / (signed int)sizeof(struct js_event); i++) {
          if(mybuffer[i].type & JS_EVENT_BUTTON &~ JS_EVENT_INIT) {
        buttons[mybuffer[i].number] = mybuffer[i].value;
          }
          else if(mybuffer[i].type & JS_EVENT_AXIS &~ JS_EVENT_INIT) {
        axes[mybuffer[i].number] = mybuffer[i].value;
        
        if(mybuffer[i].number == 0 || mybuffer[i].number == 1) {
          if(deadspot) {
            if(abs(axes[mybuffer[i].number]) < deadspot_size * 32767)
              axes[mybuffer[i].number] = 0;
            else if(axes[mybuffer[i].number] > 0)
              axes[mybuffer[i].number] = (axes[mybuffer[i].number] - 
                          deadspot_size * 32767)
            / ((1.0-deadspot_size) * 32767.0) * 32767.0;
            else if(axes[mybuffer[i].number] < 0)
              axes[mybuffer[i].number] = 
            (axes[mybuffer[i].number] + 
             deadspot_size * 32767)
            / ((1.0-deadspot_size) * 32767.0) * 32767.0;
          } else
            axes[mybuffer[i].number] = axes[mybuffer[i].number];
        }
        if(mybuffer[i].number == 1 || mybuffer[i].number == 5)
          axes[mybuffer[i].number] *= -1;
          }
        }
        return true;
      }

      //go into error state on any error
      if(errno != EAGAIN) {
	initialized = false;
	
	if(fd != -1)
	  close(fd);

	fd = -1;
      }
	
      return false;

    }

    bool Joystick::getButtonPressed(int btn_nr) const{
      if(btn_nr > nb_buttons)
        return false;
      
      return buttons[btn_nr];
    }



    Joystick::~Joystick() {
      if(axes)
	delete[] axes;
      
      if(buttons)
	delete[] buttons;

      if(fd != -1)
	close(fd);
	
      if(!initialized)
        return;
    }

    double Joystick::getAxis(Axis axis_nr) const
    {
      if (!initialized) return 0;
      if (axis_nr > nb_axes) return 0;

      return axes[axis_nr] / 32767.0;
    }

}
