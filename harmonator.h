#ifndef harmonator_h
#define harmonator_h

#include <vector>
#include <alsa/asoundlib.h>
#include <pthread.h>
#include <time.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>



#include <cppunit/TestAssert.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestFixture.h>
#include <cppunit/TestResult.h>
#include <cppunit/ui/text/TestRunner.h>

#include "harmonator_ui.h"


#include "music_clock.h"
#include <iostream>

#include <iterator>
#include <set>
#include <map>
#include <numeric>
#include <algorithm>
#include <stdexcept>

typedef enum {REST,PP,P,MP,MF,F,FF} LOUDNESS;
typedef enum {WHOLE=16,HALF=8,QUARTER=4,EIGHTH=2,SIXTEENTH=1} DURATIONTYPE;
typedef enum {TRIGGER_CLOCK,TRIGGER_CHORD} PATTERN_TRIGGER;

typedef void (*fct)(int fd,void *data);

class Chord_Notes;

class duration {
 public:
  static int pulses_per_beat;  
  static int sixteenth(){return (pulses_per_beat/4);};
  static int eighth(){return(pulses_per_beat/2);};
  static int quarter(){return(pulses_per_beat);};
  static int half(){return(pulses_per_beat*2);};
  static int whole(){return(pulses_per_beat*4);};
};


class dynamic_midi_scale
{
public:
  char rest,pp,p,mp,mf,f,ff;
  dynamic_midi_scale();
  virtual ~dynamic_midi_scale();
};

class GuiInterface
{
public:
  GuiInterface();
  ~GuiInterface();
  
};

class midi_data {
public:
  static void test();
  char midi_value;
  midi_data(int value=0);

  midi_data(const midi_data& md){
	midi_value=md.midi_value;
  };


  ~midi_data(){
  };
  bool operator<(const midi_data other) const {
  	return(this->midi_value < other.midi_value);
  }; 
  bool operator<(const midi_data *other) const {
	return(this->midi_value<other->midi_value);
  };
  bool operator==(const midi_data *other) const{
	return(this->midi_value==other->midi_value);
  };
  bool operator==(const midi_data& other) const {
	return(this->midi_value==other.midi_value);
  };

  operator char(){
	return (this->midi_value);
  };
  
  midi_data& operator-(unsigned int mv) const {
	return (*this-midi_data(mv));
  }

  midi_data& operator-(const midi_data& other) const;
  
  midi_data& operator+(int mv) const {
	return (*this+midi_data((char)mv));
  }
  midi_data& operator+(const midi_data& other) const;

};


struct mfunctor {
  bool operator()(midi_data a,midi_data b){
	return (a<b);
  };
};

typedef std::set<midi_data,mfunctor>::const_iterator pitch_assemblage_iterator;

class pitch_assemblage :public std::set<midi_data,mfunctor> {
public:
  static void test();

  pitch_assemblage():std::set<midi_data,mfunctor>(){
  };
  unsigned int add(const midi_data& md){
	//add a pitch
	this->insert(md);
	return(this->size());
  };

  const vector<int>& directed_interval_vector();


/*   void dump(){ */
/* 	cout << "pitch assemblage"<<endl; */
/* 	for (pitch_assemblage_iterator iter=this->begin();iter!=this->end();iter++){ */
/* 	  cout << *iter << endl; */
/* 	} */
/*   }; */
	
};






/**
   Used to store rhythm properties for generation like duration and loudness
*/
class Rhythm_Element {
  
public:
  const midi_data *loudness; 
  Rhythm_Element(int duration,const midi_data& _loudness); 
  //Rhythm_Element& operator=(const Rhythm_Element&);
  int duration; 
  int value(){return 0;}
  friend std::ostream& operator <<(std::ostream& s,const Rhythm_Element& re);
};

// I want to derive an iterator that wraps back around to the 1st
// element in the vector
class _rhythm_walker : public std::vector <Rhythm_Element>::iterator {
  std::vector <Rhythm_Element>& vec;

public:
/*   _rhythm_walker& operator=(const std::vector <Rhythm_Element>::iterator& iter){ */
/* 	// recursion problem?? */
/* 	std::vector <Rhythm_Element>::iterator& local_iter((std::vector <Rhythm_Element>::iterator&)(*this)); */
/* 	//(std::vector <Rhythm_Element>::iterator&)(*this)=iter; */
/* 	local_iter=iter; */
/* 	return(*local_iter); */
/*   }; */

  _rhythm_walker& operator=(const std::vector <Rhythm_Element>::iterator& iter){
	// recursion problem??
	(*this)=iter;
	return(*this);
  };


 _rhythm_walker(std::vector<Rhythm_Element>& v):std::vector<Rhythm_Element>::iterator(v.begin()),vec(v){
  };
  _rhythm_walker& operator++(int j){
	//XXX: I'm hoping that this op will throw an excp past the end to
	//which I wrap it around
	std::vector<Rhythm_Element>::iterator& base=*this;
	const std::vector<Rhythm_Element>::iterator& iter(*this);
	base++;
	if (iter != vec.end()){
	  return (*this);
	} else {
	  base = vec.begin();
	  return (*this);
	}
  }; 


}; 


class _part_walker : public std::vector<string>::iterator {
  std::vector<string>& vec;
public:
 _part_walker(std::vector<string>& v) : std::vector<string>::iterator(v.begin()),vec(v) {
  };

  _part_walker& operator++(int a){
	std::vector<string>::iterator& base=*this;
	const std::vector<string>::iterator& iter(*this);
	base++;
	if (iter != vec.end()){
	  return (*this);
	} else {
	  base = vec.begin();
	  return (*this);
	}
  }; 
  _part_walker& operator=(const std::vector <string>::iterator& iter){
	(std::vector <string>::iterator&)(*this)=iter;
	return(*this);
  };
}; 


struct jfunctor {
  bool operator()(char a,char b){
	return (a<b);
  };
};


struct kfunctor {
  bool operator()(const char *a,const char *b){
	return (strcmp(a,b)<0);
  };
};

// class to identify standard chords
class ChordWizard {
 public:

  struct chord_properties {
	char root_position;

  chord_properties(char psn=0) : root_position(psn){
  }


	bool operator==(const chord_properties& other) const {
	  return((*this).root_position==other.root_position);
	}
	
  };
  const midi_data& get_root(const Chord_Notes& cn);

  static unsigned int encode_intervals(unsigned char i=0,unsigned char  j=0,unsigned char k=0,unsigned char l=0){
	return ((i<<24) + (j<<16) + (k<<8) +l);
  }

  static unsigned int encode_intervals(const Chord_Notes& cn);
  bool is_root_position(const Chord_Notes& cn);

  map<unsigned int,chord_properties> chord_dict;

  static int count;

  ChordWizard(){
	//if (!count){
	  // major
	  // the key for chord_dicts is the intervals of the chords
	  // encoded into an unsigned int

	  chord_dict[encode_intervals(4,3)]= chord_properties(0);
	  assert(chord_dict[encode_intervals(4,3)] == chord_properties(0));
	  chord_dict[encode_intervals(3,5)]= chord_properties(2);
	  assert(chord_dict[encode_intervals(3,5)] == chord_properties(2));
	  chord_dict[encode_intervals(5,4)]= chord_properties(1);
	  assert(chord_dict[encode_intervals(5,4)] == chord_properties(1));

	  // minor
	  chord_dict[encode_intervals(3,4)]= chord_properties(0);
	  chord_dict[encode_intervals(4,5)]= chord_properties(2);
	  chord_dict[encode_intervals(5,3)]= chord_properties(1);

	  // dim
	  chord_dict[encode_intervals(3,3)]= chord_properties(0);
	  chord_dict[encode_intervals(3,6)]= chord_properties(2);
	  chord_dict[encode_intervals(6,3)]= chord_properties(1);

	  // aug
	  chord_dict[encode_intervals(4,4)]= chord_properties(0);

	  // list of sevenths: M7,7,m7,m7b5,mM7,dim7,M7+5
	  // M7
	  chord_dict[encode_intervals(4,3,4)]= chord_properties(0);
	  chord_dict[encode_intervals(3,4,1)]= chord_properties(3);
	  chord_dict[encode_intervals(4,1,4)]= chord_properties(2);
	  chord_dict[encode_intervals(1,4,3)]= chord_properties(1);

	  // m7
	  chord_dict[encode_intervals(3,4,3)]= chord_properties(0);
	  chord_dict[encode_intervals(4,3,2)]= chord_properties(3);
	  chord_dict[encode_intervals(3,2,3)]= chord_properties(2);
	  chord_dict[encode_intervals(2,3,4)]= chord_properties(1);

	  // 7
	  chord_dict[encode_intervals(4,3,3)]= chord_properties(0);
	  chord_dict[encode_intervals(3,3,2)]= chord_properties(3);
	  chord_dict[encode_intervals(3,2,4)]= chord_properties(2);
	  chord_dict[encode_intervals(2,4,3)]= chord_properties(1);
	  //m7b5: 3,3,4
	  chord_dict[encode_intervals(3,3,4)]= chord_properties(0);
	  chord_dict[encode_intervals(3,4,2)]= chord_properties(3);
	  chord_dict[encode_intervals(4,2,3)]= chord_properties(2);
	  chord_dict[encode_intervals(2,3,4)]= chord_properties(1);
	  // mM7: 3,4,4
	  chord_dict[encode_intervals(3,4,4)]= chord_properties(0);
	  chord_dict[encode_intervals(4,4,1)]= chord_properties(3);
	  chord_dict[encode_intervals(4,1,3)]= chord_properties(2);
	  chord_dict[encode_intervals(1,3,4)]= chord_properties(1);
	  // dim7: diff between inversions???
	  chord_dict[encode_intervals(3,3,3)]= chord_properties(0);
	  // M7+5:4,4,3
	  chord_dict[encode_intervals(4,4,3)]= chord_properties(0);
	  chord_dict[encode_intervals(4,3,1)]= chord_properties(3);
	  chord_dict[encode_intervals(3,1,4)]= chord_properties(2);
	  chord_dict[encode_intervals(1,4,4)]= chord_properties(1);
	  count++;
  }

  //  return a element of chord dictionary
  chord_properties& operator[](unsigned int);

#ifdef HARMONATOR_TEST  
  class ChordWizardTest: public CppUnit::TestFixture{
  public: 
	ChordWizard *cwz;
	Chord_Notes *cn;
	static CppUnit::Test *suite()
	{
	  CppUnit::TestSuite *suiteOfTests = new CppUnit::TestSuite( "ChordWizardTest" );
	  suiteOfTests->addTest(
							new CppUnit::TestCaller<ChordWizardTest>
							("testEncode",&ChordWizardTest::testEncode ));
	  suiteOfTests->addTest(
							new CppUnit::TestCaller<ChordWizardTest>
							("test_get_root",&ChordWizardTest::test_get_root ));

	  suiteOfTests->addTest(
							new CppUnit::TestCaller<ChordWizardTest>
							("test_op_access",&ChordWizardTest::test_op_access ));
	  suiteOfTests->addTest(
							new CppUnit::TestCaller<ChordWizardTest>
							("test_is_root_position",&ChordWizardTest::test_is_root_position ));

	  return suiteOfTests;
	}
	void setUp();
	void tearDown();
	void test_get_root();
	void test_is_root_position();

	void testEncode()
	{
	  // test function encode_intervals
	  CPPUNIT_ASSERT(ChordWizard::encode_intervals(0)==0);
	  CPPUNIT_ASSERT(ChordWizard::encode_intervals(1)==1<<24);
	  CPPUNIT_ASSERT(ChordWizard::encode_intervals(0,1)==1<<16);
	  CPPUNIT_ASSERT(ChordWizard::encode_intervals(0,0,1)==1<<8);
	  CPPUNIT_ASSERT(ChordWizard::encode_intervals(0,0,0,1)==1);
	}


	void test_op_access(){
	  //CPPUNIT_ASSERT_THROW((*cwz)[0],domain_error);
	  // test chordwiz[chordkey] operator
	  //CPPUNIT_ASSERT(cwz[ChordWizard::encode_intervals(4,3)]
	};
  };
#endif
};

class Chord_Notes : public vector<midi_data>{
public:
  static void test();
  static int max_parts;
  friend class HarmonatorUI;
  friend std::ostream& operator<<(std::ostream&s, const Chord_Notes& cn);
  Chord_Notes(){
	//three_part= new std::vector<int>(3,-1);
	//bass=-1;
  };

/*   bool operator==(const Chord_Notes& a){ */
/* 	return (this->vector<midi_data> == a.vector<midi_data>); */
/*   }; */

  const midi_data& operator[](int n) const {
	return(this->vector<midi_data>::operator[](n % this->size()));
  }


/*   const midi_data& operator[](const string& key) const { */
/* 	char c=key[0]; */
/* 	return(this->operator[](c-'a')); */
/*   } */



/*   const midi_data& operator[](char c) const { */
/* 	return(this->operator[]((int)(c-'a'))); */
/*   } */


/*   bool empty(){ */
/* 	// no latched functions for this chord */
/* 	return (Sxp.empty()); */
/*   }; */


  bool is_defined(){
	// sufficient configuration of tones to say a chord is defined
	return(this->size()>=3); // necessary but not quite sufficient
  };

  void add(const midi_data &md){
	if (this->size()<(unsigned int)Chord_Notes::max_parts && !this->contains(md)){
	  this->push_back(md);
	  sort(this->begin(),this->end());
	}
	else
	  {};
	  //throw("chord data overrun");
  };

  void add(int midi_value){
	  this->add(midi_data(midi_value));
  };
  void remove(const midi_data &md){
	if (find(this->begin(),this->end(),md)!=this->end())
	  this->erase(find(this->begin(),this->end(),md));
  };
  void remove(int midi_value){
	this->remove(midi_data(midi_value));
  };
/*   void clear(){ */
/* 	// clear all elements in Sxp */
/* 	Sxp.clear(); */
/*   }; */
  ~Chord_Notes(){}
  //  map<char,midi_data,jfunctor>& get_dict();
 private:
  bool contains(const midi_data& md);
};



class note_event {
 private:
  snd_seq_event_t ev;
 public:
  note_event(char channel=0, char pitch_value=60, char velocity=85,char port=0,bool noteon=true){
	snd_seq_ev_clear(&ev);
	snd_seq_ev_set_source(&ev,port);
	snd_seq_ev_set_subs(&ev);
	snd_seq_ev_set_direct(&ev);
	if (noteon)
	  snd_seq_ev_set_noteon(&ev,channel,pitch_value,velocity);
	else
	  snd_seq_ev_set_noteoff(&ev,channel,pitch_value,velocity);
	  
  }
  // copy constr which allow changing 'on/'off
 note_event(const note_event& ne,bool note_switch=false){
   ev = ne.ev;
   if (note_switch)
	 ev.type=ne.ev.type==SND_SEQ_EVENT_NOTEON?SND_SEQ_EVENT_NOTEOFF:SND_SEQ_EVENT_NOTEON;
  }

 bool operator==(const note_event& other) const{
   return(!memcmp((void *)&(this->ev),(void *)&(other.ev),sizeof(snd_seq_event_t)));
 }

 bool operator!=(const note_event& other) const{
   return(!(*this == other));
 }

  snd_seq_event_t& get_event(){return(ev);}
};


class Scheduled_Event {

public:
  char type;
  music_clock mclock;
  Scheduled_Event(const music_clock& mc, const pitch_assemblage& pa);
  Scheduled_Event(const music_clock& mc, const Chord_Notes& cn);
  Scheduled_Event(const music_clock& mc, const vector<note_event>& sent_notes);
  ~Scheduled_Event();
  friend class HarmonatorUI;
  union _payload{
	Chord_Notes *chord_notes;
	pitch_assemblage *notes_to_off;
	const vector<note_event> *sent_notes;
  _payload(const pitch_assemblage& pa): notes_to_off(new pitch_assemblage(pa)){};
  _payload(const Chord_Notes& cn): chord_notes(new Chord_Notes(cn)){};
  _payload(const vector<note_event>& sent): sent_notes(new vector<note_event>(sent)){}; 
  } payload;
};





class HarmonatorUI;

class Harmonator
{
 public:
  class rhythm_parse_exception {
  public:
	int cp;
	rhythm_parse_exception(int cursor_position=0){
	  cp=cursor_position;
	}
  };

  class voice_parse_exception {
  public:
	int cp;
	voice_parse_exception(int cursor_position=0){
	  cp=cursor_position;
	}
  };

  static void test();
  HarmonatorUI *gui;
/*   int beats_per_measure; */
/*   int pulses_per_beat; */
  music_clock main_clock;
  map<const char *,midi_data,kfunctor> dynamics_scale;
  Chord_Notes chord_buffer;
  char chan[4];
  char *rhythm_string,*part_string;
  void do_latch();

 private:

  // fetch a chord that's modified according gui settings
  /**
     Alsa port ids
  */
  int midi_out_port,midi_in_port; 
  void update_chord_display(const Chord_Notes& cn,bool ctl=false);
  /**
     keeps running count of rc'd midi clocks
  */

  dynamic_midi_scale dyn;

  int midi_clock_counter; 

  Scheduled_Event *scheduled_latch;

/* A scheduled_notesoff object is set whenever a midi note(s) is sent
   and whenever a rest is encounterd. I also functions to signal that
   the next midi noteon message is to be sent. */
  Scheduled_Event *scheduled_notesoff;

  Chord_Notes note_buffer,latched_chord;
  PATTERN_TRIGGER pattern_trigger;
  bool chord_hold,arp_armed,duration_expired;
  int rhythm_token_index,parts_token_index;
  void update_metronome();
  // XXX
  char arp_notes[3]; 
  char notes[3],bass_note,bass_channel;
  char arp_bass; 
  int rhythm_token_cnt,parts_token_cnt;

  /**
     holds rhythm info used during generation
  */
  std::vector <Rhythm_Element> rhythm_pattern; 
  // derived iterator for rhythm pattern
  class _rhythm_walker *rhythm_walker;

  std::vector<string> part_continuity;
  
  class _part_walker *part_walker;

  char *tokens[2]; 

public:
  /**
     Handle to alsa sequencer client
  */
  snd_seq_t* seq_handle; 
  
  // wrapper for 'snd_seq_event_t' from alsa

  Harmonator();
  ~Harmonator();
  void open_seq();
  //fct emit_alsa_handler();
  static void HandleAlsaPort(int fd, void *data);
  //void decr_duration_counters();
  void set_mclk_watchdog();
  void service_conditions();
  void midi_action();
  void kill_sound();
  void parse_rhythm();
  void parse_part_string();
  int get_current_velocity();
  Rhythm_Element* parse_rhythm_element(char *rhythm_el);
  void send_midi_note(int channel, int note_value, int velocity,bool noteon=true);
  void send_midi_note(note_event& ne);
  void remove_timeout();
  void set_chord_functions(const char *fnc);
  static void gui_callback_handler(Fl_Widget *w, void *data);
  class midi_action_test : public CppUnit::TestCase
  {
  public:
	typedef enum {
	  NONE=-1,
	  MIDI_CLOCK_CAPTURE,
	  FIRST_CHORD_CAPTURE,
	  PATTERN_BUMP,
	  SCHEDULED_CHORD_CAPTURE,
	  SCHEDULED_LATCH_OVERRIDE,
	  SCHEDULED_MEASURE_LATCH,
	  LATCH_CAPTURE
	} SYNC_POINT;
	set<SYNC_POINT> spts;
	Harmonator& container;
	static pthread_mutex_t mut;
	static pthread_cond_t cond;
	static pthread_cond_t hold;
	bool hold_flag; //hold in 'signal_visit' until released
	void signal_visit(SYNC_POINT); // signal waiting threads this point visited
	void wait_for_visit(set<SYNC_POINT>& spts,bool set_hold=false); // wait for this point to be visited
	void wait_for_visit(SYNC_POINT spt,bool set_hold=false); // wait for this point to be visited
	void release_visit(SYNC_POINT);
	midi_action_test(string name, Harmonator& container);
	void runTest();
  };
  midi_action_test *mat;
};


#endif
