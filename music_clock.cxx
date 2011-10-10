// for standalone compile with
// g++ music_clock.cxx -DMUSIC_CLOCK_TEST -gstabs+ -o music_clock
#define MUSIC_CLOCK

#include "music_clock.h"


// music_clock::unit music_clock::unit_beat;
// music_clock::unit music_clock::unit_measure;


// const music_clock& music_clock::unit_beat(){
//   static music_clock mc;
//   mc.measure=0;
//   mc.beat=1;
//   mc.offset=0;
//   return(mc);
// }

// const music_clock& music_clock::unit_measure(){
//   static music_clock mc;
//   mc.measure=1;
//   mc.beat=0;
//   mc.offset=0;
//   return(mc);
// }

const music_clock& music_clock::beat_floor(){
  static music_clock copy;
  copy = *this;
  copy.offset = 0;
  return copy;
}

const music_clock& music_clock::measure_floor(){
  static music_clock copy;
  copy = *this;
  copy.beat = 0;
  copy.offset = 0;
  return copy;
}

music_clock& operator+(const music_clock& a, const music_clock::unit_beat& b){
  return (
		  a+
		  music_clock::unit_clock(b.count*a.pulses_per_beat));
}


music_clock& operator+(const music_clock& a, const music_clock::unit_measure& b){
  const int pulses_per_measure=a.pulses_per_beat*a.beats_per_measure;
  return (
		  a+
		  music_clock::unit_clock(b.count*pulses_per_measure));
}

music_clock& operator+(const music_clock& a, const music_clock::unit_clock& b){
  static music_clock cum;
  cum=a;
  cum.clock_value=cum.clock_value+b.count;
  cum.offset=cum.offset+b.count;
  if (cum.offset >= cum.pulses_per_beat){
	cum.beat=cum.beat + (cum.offset/cum.pulses_per_beat);
	cum.offset=cum.offset%cum.pulses_per_beat;
	if (cum.beat >= cum.beats_per_measure){
	  cum.measure=cum.measure+(cum.beat/cum.beats_per_measure);  
	  cum.beat=cum.beat%cum.beats_per_measure;
	}
  }
  return(cum);
}



music_clock& music_clock::operator=(const music_clock& other){
  pulses_per_beat=other.pulses_per_beat;
  beats_per_measure=other.beats_per_measure;
  measure=other.measure;
  beat=other.beat;
  offset = other.offset;
  clock_value = other.clock_value;
  return (*this);
}

// const music_clock& operator+(const music_clock& a, const music_clock::unit& b){
//   // XXX: not sure about this ??
//   static music_clock result;
//   result = a; // copy a
//   if (b == music_clock::unit_beat){
// 	// increment the beat field of a
// 	result.beat++;
//   }
//   else if (b == music_clock::unit_measure){
// 	result.measure++;
//   }
//   return(result);
// }

bool operator==(const music_clock& a, const music_clock& b){
  return(
		 a.measure == b.measure &&
		 a.beat == b.beat &&
		 a.offset == b.offset
		 );
};

ostream& operator<<(std::ostream&s, const music_clock& mc){
  //ostringstream out;
  s<<dec<<setw(2)<<setfill('0')<<mc.measure+1<<setw(1)<<":"<<setw(2)<<setfill('0')<<mc.beat+1<<setw(1)<<":"<<setw(2)<<setfill('0')<<mc.offset;
  //return s << out.str();
  return s;
}

music_clock::music_clock(const music_clock& other){
  pulses_per_beat=other.pulses_per_beat;
  beats_per_measure=other.beats_per_measure;
  measure=other.measure;
  beat=other.beat;
  offset = other.offset;
  clock_value = other.clock_value;
}


music_clock::music_clock(int measure,int beat,int offset,int pulses_per_beat,int beats_per_measure){
  int carry=0;
  assert(pulses_per_beat==24 || pulses_per_beat==96);
  assert(beats_per_measure>0);
  this->pulses_per_beat=pulses_per_beat;
  this->beats_per_measure=beats_per_measure;
  this->clock_value=0;
  assert(measure>-2);
  assert(beat>=0);
  assert(offset>=0);
  // From user's view, measures enumerate from 1 up. Beat enums from 1
  // to beats_per_measure and offset enums from 0 upto pulses_per_beat-1

  // This is to allow calls with what I'll call an irregular
  // initialization is performed.
  // Example
  // music_clock mc(0,1,440);
  if (offset >= pulses_per_beat){
	carry = offset / pulses_per_beat;
	offset = offset % pulses_per_beat;
  }
  beat+=carry;
  carry=0;
  if (beat>=beats_per_measure){
	carry=beat/beats_per_measure;
	beat=beat%beats_per_measure;
  }
  measure+=carry;
  this->measure=measure;
  this->beat=beat;
  this->offset=offset;
}

// int music_clock::clocks() const{
//   int clocks=measure*pulses_per_beat*beats_per_measure;
//   clocks+=beat*pulses_per_beat;
//   clocks+=offset;
//   return clocks;
// }


music_clock::~music_clock(){
}

music_clock& music_clock::reset(){
//   this->measure=-1;
//   this->beat=beats_per_measure-1;
//   this->offset=pulses_per_beat-1;
  measure=beat=offset=clock_value=0;
  return(*this);
}

bool music_clock::is_match(const string& to_be_compared){
  // Compare current music_clock to a coordinate string. This is
  // different than simply comparing to measure,beat,offset
  // attibutes. "Origin" in coordinate terms is "01:01:00"
  
// to_be_compared = measure : beat : pulse
  // offset measure = wildcard digit digit beat = wildcard digit digit
  // pulse_offset = wildcard digit digit examples *:02:23 beat 2-pulse
  // offset 23 24:*:* any coord within measure 24
  
  // parse 'to_be_compared' into measure spec, beat spec, pulse_offset spec
  string::const_iterator iter= to_be_compared.begin();
  //string::iterator iter
  int ctl=0;
  string test;
  do {
	if (*iter!='*'){
	  switch(ctl){
	  case 0:
		if (atoi(string(iter,iter+2).c_str())-1!=measure)
		  return false;
		break;
	  case 1:
		if (atoi(string(iter,iter+2).c_str())-1!=beat)
		  return false;
		break;
	  case 2:
		if (atoi(string(iter,iter+2).c_str())==offset)
		  return true;
		else
		  return false;
	  };
	}
	ctl++;
	// advance to character after ':'
	while (*iter++!=':');
  } while (iter!=to_be_compared.end());
  return false;
}


music_clock& music_clock::operator++(int x){
//   offset++;
//   if (!(offset % pulses_per_beat)){
// 	offset=0;
// 	beat++;
// 	if (!(beat % beats_per_measure)){
// 	  beat=0;
// 	  measure++;
// 	}
//   }
  return (*this=(*this+music_clock::unit_clock(1)));
}

#ifdef MUSIC_CLOCK_TEST
int main(int argc, char **argv){
  music_clock mc;
  music_clock *mcptr;
  cout << "mc="<<mc<<endl;
  mc++;
  assert(mc.measure==0);
  assert(mc.beat==0);
  assert(mc.offset==1);
  assert(mc.is_match("01:01:01"));
  assert(mc.is_match("*:*:01"));
  mcptr = new music_clock(1,1,23);
  assert(mcptr->offset==23);
  cout << "*mcptr ==" << *mcptr << endl;
  (*mcptr)++;
  assert(mcptr->is_match("02:03:00"));
  assert(mcptr->offset==0);
  assert(mcptr->beat==2);
  assert(mcptr->measure==1);
  delete mcptr;
  // test beat_floor member
  mcptr = new music_clock(2,2,23);
  assert(mcptr->offset==23);
  assert(mcptr->beat==2);
  assert(mcptr->measure==2);
  cout << "mcptr="<<*mcptr<<endl;
  assert(mcptr->beat_floor().measure==2);
  assert(mcptr->beat_floor().beat==2);
  assert(mcptr->beat_floor().offset==0);
  cout << "mcptr->beat_floor="<<mcptr->beat_floor()<<endl;
  cout << "mcptr->measure_floor="<<mcptr->measure_floor()<<endl;  
  assert(mcptr->measure_floor().measure==2);
  assert(mcptr->measure_floor().beat==0);
  assert(mcptr->measure_floor().offset==0);
  
  assert((*mcptr+music_clock::unit_measure())==music_clock(3,2,23));
  cout << "*mcptr+music_clock::unit_measure="<<*mcptr+music_clock::unit_measure()<<endl;
  assert((*mcptr+music_clock::unit_measure())==music_clock(3,2,23));
  cout << "*mcptr+music_clock::unit_beat="<<*mcptr+music_clock::unit_beat()<<endl;
}
#endif
