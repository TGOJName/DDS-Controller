// The menus on lcd used for DDS with an analog control by Juntian Tu at JQI in Jan. 2023
// Inherited and editted from https://github.com/JQIamo/hv-piezo-driver/tree/master/code written by Neal Pisenti at JQI

#include "menus.h"
#include <string>
#include <AD9910.h>
#include "encoder.h"
#include <EEPROM.h>

/*
 The containing relation of the menus (which is realized by Linkedlist library) is:
    root->{
            analog_switch
            analog_setting->{
                            freq_max
                            freq_min
                            back
                            }
            channel_set
          }
*/

Menu::Menu(const char * display_name, LCD * display) {
  _parent = 0;
  strcpy(_display_name, display_name);
  _display = display;
}

void Menu::add(Menu * menu){
  _menus.add(menu);
  menu->_parent = this;
};

void Menu::enter(){
  root._active = this;
  update();
};

void Menu::update(){
    _display->setCursor(0, 0);
    char firstline_cursor[17];
    sprintf(firstline_cursor,">%s",_menus.get(_current)->_display_name);
    _display->printer(firstline_cursor);
    _display->setCursor(0, 1);
    _display->printer(_menus.get((_current + 1) % _menus.size())->_display_name);
}

void Menu::exit(){
  if (_parent != 0) _parent->enter();
};

void Menu::process(char c){
  //  +,p,P and -,m,M are the encoder rotation at different speed colckwise and counter-clockwisely
  if (c == '+' or c == 'p' or c == 'P'){
    _menu_counter++;
    _current = _menu_counter % _menus.size();
    update();
    delay(10);
  }else if (c == '-' or c == 'm' or c == 'M'){
    _menu_counter--;
    if (_menu_counter < 0){
      _menu_counter = _menus.size() - 1;
    }

    _current = _menu_counter % _menus.size();
     update();
     delay(10);
  }else if (c == 'c'){
    // c is the button press of the encoder
    Menu * next;
    next = _menus.get(_current);
    if (next->is_executive==false){
      root._active = next;
    }
    next->enter();
    if (next->is_executive){
      update();
      delay(10);
    }
  }
}

Root::Root(const char * display_name, LCD * output)
  : Menu(display_name, output){};

A_switch::A_switch(const char * display_name, LCD * output)
  : Menu(display_name, output){};

void A_switch::enter(){
  if (DDS0.isAnalogMode){
    strcpy(_display_name, "ANLG:OFF");
    DDS0.isAnalogMode = false;
    DDS0.setFreq(10000000);
  }
  else{
    strcpy(_display_name, "ANLG:ON");
    DDS0.isAnalogMode = true;
  }
  _parent->enter();
}


Back::Back(const char * display_name, LCD * output)
  : Menu(display_name, output){};

void Back::enter(){
  _parent->exit();
}

Freq_max::Freq_max(const char * display_name, LCD * output)
  : Menu(display_name, output){
    strcpy(unit,"MHz");
  };

void Freq_max::update(){
  realtime();
  _display->setCursor(0, 0);
  _display->printer(_display_name);
  _display->setCursor(0, 1);
  _display->printer("Press To Return");
}

void Freq_max::enter(){
  _display->setCursor(0, 0);
  _display->printer(_display_name);
  _display->setCursor(0, 1);
  _display->printer("Press To Return");
}

void Freq_max::process(char c){
  if (c == 'c'){
    // Set the frequency limit and leave to the parent menu
    if(unit[0] == 'M'){
      newFreq = current_mfreq * 1e6;
    }else if(unit[0] == 'k'){
      newFreq = current_mfreq * 1e3;
    }else{
      newFreq = current_mfreq;
    }
    checker();
  }else{
    if (c == '+'){
    current_mfreq++;
    }else if (c == '-'){
    current_mfreq--;
    }else if (c == 'p'){
    current_mfreq += 10;
    }else if (c == 'P'){
    current_mfreq += 100;
    }else if (c == 'm'){
    current_mfreq -= 10;
    }else if (c == 'M'){
    current_mfreq -= 100;
    }
  carry();
  update();
  } 
}

void Freq_max::checker(){ // Make sure the range of the analog freq is well defined
  if(newFreq<DDS0.minAnalogFreq){
    _display->setCursor(0, 0);
    _display->printer("Max < Min!");
    _display->setCursor(0, 1);
    _display->printer(" ");
    delay(3000);
    enter();
  }else{
    realtime();
    exit();
  }
}

void Freq_max::carry(){ // Change the displayed unit properly
  if (current_mfreq > 500){
    if (unit[0] == 'M'){
      current_mfreq = 500;
    }else if (current_mfreq > 1000){
      if (unit[0] == 'k'){
      current_mfreq = 1;
      strcpy(unit,"MHz");
    }else{
      current_mfreq = 1;
      strcpy(unit,"kHz");
    }
    encoder.timescounter = 0;
    }
  }else if (current_mfreq < 1){
    if (unit[0] == 'H'){
      current_mfreq = 1;
    }else if (unit[0] == 'k'){
      current_mfreq = 1000;
      strcpy(unit,"Hz");
    }else{
      current_mfreq = 1000;
      strcpy(unit,"kHz");
    }
  encoder.timescounter = 0;    
  }
}

void Freq_max::realtime(){
  sprintf(place_holder, "MaxFreq:%i%s",current_mfreq, unit);
  strcpy(_display_name, place_holder);
}

void Freq_max::exit(){
  DDS0.maxAnalogFreq = newFreq; // Set the range to the DDS
  if (_parent != 0) _parent->enter();
};

Freq_min::Freq_min(const char * display_name, LCD * output)
  : Freq_max(display_name, output){
    strcpy(unit,"MHz");
};

void Freq_min::realtime(){
  sprintf(place_holder, "MinFreq:%i%s",current_mfreq, unit);
  strcpy(_display_name, place_holder);
}

void Freq_min::exit(){
  DDS0.minAnalogFreq = newFreq;
  if (_parent != 0) _parent->enter();
};

void Freq_min::checker(){
  if(newFreq>DDS0.maxAnalogFreq){
    _display->setCursor(0, 0);
    _display->printer("Min > Max!");
    _display->setCursor(0, 1);
    _display->printer(" ");
    delay(3000);
    enter();
  }else{
    realtime();
    exit();
  }
}

Channel_set::Channel_set(const char * display_name, LCD * output)
  : Menu(display_name, output){};

void Channel_set::update(){
  sprintf(place_holder, "Channel:%i",current_channel);
  strcpy(_display_name, place_holder);
  _display->setCursor(0, 0);
  _display->printer(_display_name);
  _display->setCursor(0, 1);
  _display->printer("Press To Return");
}

void Channel_set::process(char c){
  if (c == 'c'){
    exit();
  }else{
    if (c == '+'){
    current_channel++;
    }else if (c == '-'){
    current_channel--;
    }else if (c == 'p'){
    current_channel ++;
    }else if (c == 'P'){
    current_channel ++;
    }else if (c == 'm'){
    current_channel --;
    }else if (c == 'M'){
    current_channel --;
    }
  if (current_channel < 0){current_channel = 0;}
    else if (current_channel > 7){current_channel = 9;}
  update();
  } 
}

void Channel_set::exit(){
  channel_index = current_channel; // Set the range to the DDS
  EEPROM.update(0,channel_index);
  _parent->enter();
};

Static_out::Static_out(const char * display_name, LCD * output)
  : Menu(display_name, output){};

void Static_out::enter(){
  current_freq = DDS0._freq[0];
  sprintf(place_holder,"Freq:%9liHz",current_freq);
  _display->setCursor(0, 0);
  _display->printer(_display_name);
  _display->setCursor(7, 1);
  _display->write(0b00010111);
  cursor = 6;
}

void Static_out::cursor_update(){
  _display->setCursor(13-cursor, 1);
  _display->write(0b00010111);
}

void Static_out::process(char c){
  if (c == 'c'){
    // Set the frequency limit and leave to the parent menu
    if(cursor == 6 || cursor == 3){
      cursor -= 3;
    }else{
      cursor = 6;
    }
    cursor_update();
  }else if (c == 'h'){
    exit();
  }else{
    if (c == '+'){
    current_freq += pow(10,cursor);
    }else if (c == '-'){
    current_freq -= pow(10,cursor);
    }else if (c == 'p'){
    current_freq += 10 * pow(10,cursor);
    }else if (c == 'P'){
    current_freq += 100 * pow(10,cursor);
    }else if (c == 'm'){
    current_freq -= 10 * pow(10,cursor);
    }else if (c == 'M'){
    current_freq -= 100 * pow(10,cursor);
    }
  checker();
  update();
  } 
}

void Static_out::checker(){
  if (current_freq<1000){
    current_freq = 1000;
  }else if (current_freq>4e8){
    current_freq = 400000000;
  }
}

void Static_out::update(){
  DDS0.setWave(current_freq,0,100);
  sprintf(place_holder,"Freq:%9liHz",current_freq);
  _display->setCursor(0, 0);
  _display->printer(place_holder);
}

uint8_t channel_index = 0;

Root root("root", &lcd);
Menu analog_setting("ANLG_SET", &lcd);
Freq_max freq_max("MaxFreq:200MHz", &lcd);
Freq_min freq_min("MinFreq:100MHz", &lcd);
A_switch analog_switch("ANLG:OFF", &lcd);
Static_out static_out("Static_Mode",&lcd);
Back back("BACK", &lcd);