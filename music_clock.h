#include <string>
#include <cstdlib>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cassert>

using namespace std;



class music_clock
{
 public:
  class unit {
  public:
	int count;
	unit(int initial_count=1){count = initial_count;};

	bool operator==(unit& other) const{
	  return (this == &other);
	}
  };

  class unit_beat : public unit{
  public:
	unit_beat(int initial=1):unit(initial){};
  };

  class unit_measure: public unit {
  public:
	unit_measure(int initial=1):unit(initial){};
  };

  class unit_clock: public unit {
  public:
	unit_clock(int initial=1):unit(initial){};
  };

  //static unit unit_beat,unit_measure;
  music_clock(const music_clock& other);

  music_clock(int measure=0,int beat=0,int offset=0,int pulses_per_beat=24,int beats_per_measure=4);

  virtual ~music_clock();
  // calc elapsed midi clocks corresponding to the music clock
  //int clocks() const;
  // new music clock floor'd to beat
  const music_clock& beat_floor(); 
  // new music clock floor'd to measure
  const music_clock& measure_floor();
  // increment internal clock
  music_clock& operator++(int);
  music_clock& reset();
  
  music_clock& operator=(const music_clock& other);

  // compare 2 measure clock objects
  friend bool operator==(const music_clock& a, const music_clock& b);

  friend music_clock& operator+(const music_clock& a, const music_clock::unit_measure& b);
  friend music_clock& operator+(const music_clock& a, const music_clock::unit_clock& b);
  friend music_clock& operator+(const music_clock& a, const music_clock::unit_beat& b);

  friend ostream& operator<<(std::ostream&s, const music_clock& mc);

  // is_match(mc,string("*:01:*"); // matches any value on beat 1
  bool is_match(const string& to_be_compared);

  //private:
  unsigned int clock_value;
  int pulses_per_beat,beats_per_measure;
  int measure,beat,offset;
  int get_measure(){return measure+1;}
  int get_beat(){return beat+1;}
  int get_offset(){return offset;}
  // for converting musical durations to midi clock pulse values
  int sixteenth(){return (pulses_per_beat/4);};
  int eighth(){return(pulses_per_beat/2);};
  int quarter(){return(pulses_per_beat);};
  int half(){return(pulses_per_beat*2);};
  int whole(){return(pulses_per_beat*4);};
  int triplet_eighth(){return (pulses_per_beat/3);}
};

#ifdef MUSIC_CLOCK
const music_clock& operator+(const music_clock* a, const music_clock::unit& b);
bool operator==(const music_clock& a, const music_clock& b);
ostream& operator<<(std::ostream&s, const music_clock& mc);
#else
extern const music_clock& operator+(const music_clock* a, const music_clock::unit& b);
extern bool operator==(const music_clock& a, const music_clock& b);
extern ostream& operator<<(std::ostream&s, const music_clock& mc);
#endif
