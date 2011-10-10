#include "harmonator.h"

// To handle some embedded testing calls
#ifdef HARMONATOR_TEST
#define matvisit(x) mat->signal_visit(x)
#else
#define matvisit(x)
#endif

/* Use this variable to remember original terminal attributes. */
struct termios saved_attributes;
     
void
reset_input_mode (void)
{
  tcsetattr (STDIN_FILENO, TCSANOW, &saved_attributes);
}
     
void
set_input_mode (void)
{
  struct termios tattr;
  //char *name;
     
  /* Make sure stdin is a terminal. */
  if (!isatty (STDIN_FILENO))
	{
	  fprintf (stderr, "Not a terminal.\n");
	  exit (EXIT_FAILURE);
	}
     
  /* Save the terminal attributes so we can restore them later. */
  tcgetattr (STDIN_FILENO, &saved_attributes);
  atexit (reset_input_mode);
     
  /* Set the funny terminal modes. */
  tcgetattr (STDIN_FILENO, &tattr);
  tattr.c_lflag &= ~(ICANON|ECHO); /* Clear ICANON and ECHO. */
  tattr.c_cc[VMIN] = 1;
  tattr.c_cc[VTIME] = 0;
  tcsetattr (STDIN_FILENO, TCSAFLUSH, &tattr);
}



#define log(msg) printf("%d:%s",__LINE__,msg)

string for_debug;

int ChordWizard::count=0;

pthread_mutex_t Harmonator::midi_action_test::mut = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t Harmonator::midi_action_test::cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t Harmonator::midi_action_test::hold = PTHREAD_COND_INITIALIZER;


ChordWizard cw;

int Chord_Notes::max_parts = 4;

extern ostream& operator<<(std::ostream&s, const music_clock& mc);

int  break_mark(int);


std::ostream& operator<<(std::ostream&s, const midi_data& m){
  return s << "midi_data(0x"<< hex << int(m.midi_value) << ")";
}


// hack of least common multiple
unsigned int lcm(unsigned int x, unsigned int y){
  unsigned int max;
  if (!(x % y))
	return x;
  if (!(y % x))
	return y;
  max = x>y?x:y;
  for (unsigned int val=max; val <= x*y;val+=max){
	if (!(val % x) && !(val % y))
	  return (val);
  }
  assert(0);
}


// void locker(){
//   ...
//   
//   s->wait();
//  do something
//   ...
// s->release();
//}

// void lockee(){
//   ...
// hold here until released
//   s->hold();
//   ...
// 	}


bool operator==(vector<note_event>& a,vector<note_event>& b){
  vector<note_event>::const_iterator a_iter,b_iter;
  for (a_iter=a.begin(),b_iter=b.begin();a_iter!=a.end();a_iter++,b_iter++)
	if (*a_iter != *b_iter)
	  return(false);
  return(true);
}

const Chord_Notes& get_amended_chord(Chord_Notes& chord_buffer,bool make_four=true,bool adjust_root=true){
  // make a copy of 'chord_buffer'
  static Chord_Notes copy;
  cout << __LINE__ << ":chord_buffer(sz:"<< chord_buffer.size() << ")"<<chord_buffer << endl;

  for (int ii=0;ii<chord_buffer.size();ii++){
	const midi_data& md(chord_buffer[ii]);
	cout << "cb[" <<ii << "]:" << md <<endl;
  }

  copy = chord_buffer;

  cout << __LINE__ << ":chord_buffer(sz:"<< chord_buffer.size() << ")"<<chord_buffer << endl;

  for (int ii=0;ii<chord_buffer.size();ii++){
	const midi_data& md(chord_buffer[ii]);
	cout << "cb[" <<ii << "]:" << md <<endl;
  }


  cout << __LINE__ << ":copy(sz:"<< copy.size() << ")"<<copy << endl;

  for (int ii=0;ii<copy.size();ii++){
	const midi_data& md(copy[ii]);
	cout << "copy[" <<ii << "]:" << md <<endl;
  }



  if (make_four){
	if (adjust_root){
	  if (!cw.is_root_position(copy)){
		// add doubling of  root with octave lower
		midi_data md=cw.get_root(copy)-12;
		if (copy.size()>=4) // 7th chord
		  copy.remove(cw.get_root(copy));
		copy.add(md);
	  } else {
		if (chord_buffer.size()==3){
		  // double lowest function an octave higher
		  copy.add(copy[0]+12);
		}
	  }
	} else {
	  if (chord_buffer.size()==3){
		// double lowest tone an octave higher
		  copy.add(copy[0]+12);
	  } 
	}
  } else {
	// don't make 4pt
	if (adjust_root){
	  if (!cw.is_root_position(copy)){
		midi_data md=cw.get_root(copy)-12;
		copy.remove(cw.get_root(copy));
		copy.add(md);
	  }
	}
  }
  return(copy);
}





std::ostream& operator<<(std::ostream&s, const pitch_assemblage& pa){
  s << "pitch assemblage: sz=" << pa.size() << endl;
  for (pitch_assemblage_iterator iter=pa.begin();iter!=pa.end();iter++){
	s << *iter << endl;
  }
  return s;
}


int slotcmp(const void *x,const void *y);

ChordWizard::chord_properties& ChordWizard::operator[](unsigned int key){
  map<unsigned int,chord_properties>::iterator iter=
	this->chord_dict.find(key);
  if (iter!=this->chord_dict.end())
	return (iter->second);
  throw domain_error("Undefined chord.");
}


const midi_data& ChordWizard::get_root(const Chord_Notes& cn){
  // return midi_data object of root position of assemblage
//   map<unsigned int,chord_properties>::iterator iter=
// 	this->chord_dict.find(ChordWizard::encode_intervals(cn));
//   if (iter!=this->chord_dict.end())
// 	return (cn[iter->second.root_position]);

  chord_properties& cp((*this)[ChordWizard::encode_intervals(cn)]);
  return (cn[cp.root_position]);
}

unsigned int ChordWizard::encode_intervals(const Chord_Notes& cn){
  vector<midi_data> ivals;
  Chord_Notes::const_iterator iter;
  // Begin iter on 1st(not 0th) element. Push difference between
  // adjacent elements on the ivals vector.
  for (iter=cn.begin()+1;iter!=cn.end();iter++){
	try {
	  ivals.push_back(*iter - *(iter-1));
	}
	catch (const char *s){
	  cout << "rc'd exc: " <<  s << endl;
	  cout << "Chord_Notes:" << cn << endl;
	  break;
	}
  }

  switch (ivals.size()){
  case 0:
	return (ChordWizard::encode_intervals());
  case 1:
	return (ChordWizard::encode_intervals((unsigned char)(ivals[0])));
  case 2:
	return (ChordWizard::encode_intervals(ivals[0],ivals[1]));
  case 3:
	return (ChordWizard::encode_intervals(ivals[0],ivals[1],ivals[2]));
  case 4:
	return (ChordWizard::encode_intervals(ivals[0],ivals[1],ivals[2],ivals[3]));
  default:
	throw "invalid interval size";
  };
}


bool ChordWizard::is_root_position (const Chord_Notes& cn){
  chord_properties& cp(cw.chord_dict[ChordWizard::encode_intervals(cn)]);
  return(cp.root_position==0);
}

const vector<int>& pitch_assemblage::directed_interval_vector(){
	// calc. directed-intervals between  pitches
  static vector<int> out;
  vector<int> v;
  out.clear();
  pitch_assemblage_iterator piter;
  for (piter=this->begin();piter!=this->end();piter++)
	v.push_back((*piter).midi_value);
  out.reserve(v.size());
  back_insert_iterator<vector<int> > bii(out);
  adjacent_difference(v.begin(),v.end(),bii);
  out.erase(out.begin());
  return(out);
  }

void pitch_assemblage::test(){
  pitch_assemblage pa;
  pa.add(midi_data(60));
  pa.add(midi_data(60+4));
  pa.add(midi_data(60+4+3));
  cout << pa << endl;
  const vector<int>& div(pa.directed_interval_vector());
  assert(div.size()==2);
  assert(div[0]==4);
  assert(div[1]==3);
}



midi_data& midi_data::operator-(const midi_data& other) const {
  static midi_data md;
  if (this->midi_value-other.midi_value >=0){
	md.midi_value = this->midi_value-other.midi_value;
	return(md);
  }
  throw("operation result out of range");
}


midi_data& midi_data::operator+(const midi_data& other) const {
  static midi_data md;
  if (this->midi_value+other.midi_value >=0){
	md.midi_value = this->midi_value+other.midi_value;
	return(md);
  }
  throw("operation result out of range");
}


midi_data::midi_data(int value){
	if (-1 < value && value < 128)
	  midi_value=value;
	else {
	  throw domain_error("Invalid midi data value.");
	}
  };


void midi_data::test(){
  midi_data *md = new midi_data(60);
  assert(*md+12 == midi_data(72));
  assert(*md+midi_data(12) == midi_data(72));
  assert(((*md+12).operator-(12)) == *md);
}


std::ostream& operator<<(std::ostream&s, const Chord_Notes& cn){
  s<<"Chord_Notes(";
  if (cn.size()){
	Chord_Notes::const_iterator iter=cn.begin();

	s<<*iter;
	for (
		 iter=cn.begin()+1;
		 iter!=cn.end();
		 iter++)
	  s << ","  << *iter;
  }
  s<<")";
  return s;
}


void Chord_Notes::test(){
  Chord_Notes cn;
  cn.add(midi_data(63));
  cn.add(midi_data(64));
  assert(cn.contains(midi_data(63)));
  assert(cn.contains(midi_data(64)));
  assert(cn[0]==cn['a']);
  assert(cn[0]==midi_data(63));
  assert(cn[1]==cn['b']);
  assert(cn[1]==midi_data(64));
  cn.remove(63);
  cn.remove(64);
  assert(cn.size()==0);
  cn.add(midi_data(60));
  cn.add(midi_data(64));
  cn.add(midi_data(67));
  // cn is in root psn so doubled tone should be root octave above
  assert(cw.is_root_position(cn));
  assert(get_amended_chord(cn).size()==4);
  assert((get_amended_chord(cn)[0]+12)==get_amended_chord(cn)[3]);
  assert(get_amended_chord(cn)[0]==midi_data(60));
  // Test chord is 1st inversion
  // make 3pt > 4pt and adjust the root tone to bottom
  cn.clear();
  cn.add(midi_data(60+12));
  cn.add(midi_data(64));
  cn.add(midi_data(67));
  assert(!cw.is_root_position(cn));
  assert(get_amended_chord(cn).size()==4);
  assert((get_amended_chord(cn)[0]+12)==get_amended_chord(cn)[3]);
  assert(get_amended_chord(cn)[0]==midi_data(60));
  // Test chord is 2nd inversion
  // make 3pt > 4pt and adjust the root tone to bottom
  cn.clear();
  cn.add(midi_data(60+12));
  cn.add(midi_data(64+12));
  cn.add(midi_data(67));
  assert(!cw.is_root_position(cn));
  assert(get_amended_chord(cn).size()==4);
  assert((get_amended_chord(cn)[0]+12)==get_amended_chord(cn)[2]);
  assert(get_amended_chord(cn)[0]==midi_data(60));
  // Test chord is root
  // make 3pt > 4pt and double root above
  cn.clear();
  cn.add(midi_data(60));
  cn.add(midi_data(64));
  cn.add(midi_data(67));
  assert(cw.is_root_position(cn));
  assert(get_amended_chord(cn,true,false).size()==4);
  assert((get_amended_chord(cn,true,false)[0]+12)==get_amended_chord(cn,true,false)[3]);
  assert(get_amended_chord(cn,true,false)[0]==midi_data(60));
  // Test chord is 1st inv
  // make 3pt > 4pt and double 3rd above
  cn.clear();
  cn.add(midi_data(60+12));
  cn.add(midi_data(64));
  cn.add(midi_data(67));
  assert(!cw.is_root_position(cn));
  assert(get_amended_chord(cn,true,false).size()==4);
  assert((get_amended_chord(cn,true,false)[0]+12)==get_amended_chord(cn,true,false)[3]);
  assert(get_amended_chord(cn,true,false)[0]==midi_data(64));
  // Test chord is 2nd inv
  // make 3pt > 4pt and double 3rd above
  cn.clear();
  cn.add(midi_data(60+12));
  cn.add(midi_data(64+12));
  cn.add(midi_data(67));
  assert(!cw.is_root_position(cn));
  assert(get_amended_chord(cn,true,false).size()==4);
  assert((get_amended_chord(cn,true,false)[0]+12)==get_amended_chord(cn,true,false)[3]);
  assert(get_amended_chord(cn,true,false)[0]==midi_data(67));


  // no doubling but do adjust root if necessary

  // Test chord is root
  // should not change
  cn.clear();
  cn.add(midi_data(60));
  cn.add(midi_data(64));
  cn.add(midi_data(67));
  assert(cw.is_root_position(cn));
  assert(get_amended_chord(cn,false,true).size()==3);
  assert(get_amended_chord(cn,false,true)[0]==midi_data(60));



  // Test chord is 1st inv
  // should not change
  cn.clear();
  cn.add(midi_data(60+12));
  cn.add(midi_data(64));
  cn.add(midi_data(67));
  assert(!cw.is_root_position(cn));
  assert(get_amended_chord(cn,false,true).size()==3);
  assert(get_amended_chord(cn,false,true)[0]==midi_data(60));
  assert(get_amended_chord(cn,false,true)[1]==midi_data(64));
  assert(get_amended_chord(cn,false,true)[2]==midi_data(67));


  // Test chord is 1st inv
  // should not change
  cn.clear();
  cn.add(midi_data(60+12));
  cn.add(midi_data(64+12));
  cn.add(midi_data(67));
  assert(!cw.is_root_position(cn));
  assert(get_amended_chord(cn,false,true).size()==3);
  assert(get_amended_chord(cn,false,true)[0]==midi_data(60));
  assert(get_amended_chord(cn,false,true)[1]==midi_data(67));
  assert(get_amended_chord(cn,false,true)[2]==midi_data(64+12));



}

bool Chord_Notes::contains(const midi_data& checked_value){
  vector<midi_data>::iterator iter;
  for (iter = this->begin();iter!=this->end();iter++){
	if (*iter == checked_value)
	  return(true);
  }
  return(false);
}

// map<char,midi_data,jfunctor>& Chord_Notes::get_dict(){
//   static map<char,midi_data,jfunctor> midi_map;
//   pitch_assemblage& pa(this->Sxp);
//   string::iterator f_iter=string("abcdefghijk").begin();
//   pitch_assemblage_iterator p_iter = pa.begin();
//   midi_map.clear();
//   //f_iter: iter thru 'a','b'...
//   //p_iter: pitch assm iter
//   for (;p_iter!=pa.end();p_iter++){
// 	midi_map[*f_iter++]=*p_iter;
//   }
//   return midi_map;
// }

Scheduled_Event::Scheduled_Event(const music_clock& mc,const pitch_assemblage& pa)
  :type(0),mclock(music_clock(mc)),payload(pa){
}


Scheduled_Event::Scheduled_Event(const music_clock& mc,const Chord_Notes& cn)
  :type(1),mclock(music_clock(mc)),payload(cn){
}

Scheduled_Event::Scheduled_Event(const music_clock& mc,const vector<note_event>& sent_notes)
  :type(2),mclock(music_clock(mc)),payload(sent_notes){
}

Scheduled_Event::~Scheduled_Event(){
  switch(type){
  case 0:
	delete payload.notes_to_off;break;
  case 1:
	delete payload.chord_notes;break;
  case 2:
	delete payload.sent_notes;break;
  }
}

dynamic_midi_scale::dynamic_midi_scale(){
}

dynamic_midi_scale::~dynamic_midi_scale(){
}



Rhythm_Element::Rhythm_Element(int duration,const midi_data& _loudness):loudness(&_loudness) {
  this->duration=duration;
  //this->loudness=loudness;
}

std::ostream& operator<<(std::ostream&s, const Rhythm_Element& re){
  const midi_data& md(*(re.loudness));
  return s << "Rhythm_Element(0x"<< hex << int(re.duration) << ","<< md << ")";
}



// Rhythm_Element& Rhythm_Element::operator=(const Rhythm_Element& a){
//   *this = Rhythm_Element(44,a.loudness);
//   return(*this);
// }




void Harmonator::gui_callback_handler(Fl_Widget *w, void *data){
  Harmonator *harmonator = (Harmonator *)data;
  switch (w->type()){
  case FL_TOGGLE_BUTTON:
	{
	  Fl_Light_Button *lbut=(Fl_Light_Button *)w;
	  if (strcmp(lbut->label(),"&Arpeggiate")==0){
		if (lbut->value()){ // check the arp button state
		  try {
			harmonator->parse_rhythm();
		  }
		  catch (Harmonator::rhythm_parse_exception rpe){
			lbut->value(0);
			throw;
		  }

		  try {
			harmonator->parse_part_string();
		  }
		  catch (Harmonator::voice_parse_exception vpe){
			lbut->value(0);
			throw;
		  }

		  harmonator->main_clock.reset();
		  harmonator->chord_buffer.clear();
		  harmonator->latched_chord.clear();
		  harmonator->arp_armed=true;
		  harmonator->pattern_trigger=TRIGGER_CHORD;
		  harmonator->rhythm_walker=new _rhythm_walker(harmonator->rhythm_pattern);
		  harmonator->part_walker= new _part_walker(harmonator->part_continuity);
		  harmonator->gui->configure_for_arp();
		}
		else{
		  harmonator->arp_armed = false;
		  harmonator->gui->restore_after_arp();
		  delete harmonator->part_walker;
		  if (harmonator->scheduled_latch){
			delete harmonator->scheduled_latch;
			harmonator->scheduled_latch=NULL;
		  }
		  if (harmonator->scheduled_notesoff){
			delete harmonator->scheduled_notesoff;
			harmonator->scheduled_notesoff=NULL;
		  }
		}
	  }
	  else if (strcmp(lbut->label(),"Midi Click")==0)
		{
		  //
		}

	  break;
	}

  default:
	{
	//if (strcmp(w->label(),"Beats Per Measure")==0){
	if (w == harmonator->gui->beats_per_measure){
	  // update the main clock
	  harmonator->main_clock.beats_per_measure=(int)(harmonator->gui->beats_per_measure->value());
  	  for (int i=0; i<(int)(harmonator->gui->beats_per_measure->value());i++){
  		harmonator->gui->metronome_lights[i]->show();
		assert(harmonator->gui->metronome_lights[i]->visible());
	  }
  	  for (int i=(int)(harmonator->gui->beats_per_measure->value()); i<7 ;i++){
  		harmonator->gui->metronome_lights[i]->hide();
		harmonator->gui->metronome_lights[i]->redraw();
		assert(!harmonator->gui->metronome_lights[i]->visible());
	  }
  	}
	else if (w == harmonator->gui->strig_midi){
	  harmonator->gui->sample_trigger = 0;
	  harmonator->gui->lsync_now->clear();
	  harmonator->gui->lsync_measure->clear();
	  harmonator->gui->lsync_beat->clear();
	  harmonator->gui->set_latch_button_type();
	}
	else if (w == harmonator->gui->strig_gate){
	  harmonator->gui->sample_trigger = 1;
	  harmonator->gui->lsync_now->clear();
	  harmonator->gui->lsync_measure->clear();
	  harmonator->gui->lsync_beat->clear();
	  harmonator->gui->set_latch_button_type(false);
	}
	else if (
			 w == harmonator->gui->lsync_now ||
			 w == harmonator->gui->lsync_measure ||
			 w == harmonator->gui->lsync_beat
			 ){
	  printf("latch key engaged\n");
	  if (harmonator->gui->strig_gate->value()){
		printf("executing a latch op\n");
		harmonator->do_latch();
	  }
	}
	else if (w == harmonator->gui->calculate_pattern_length)
	  {
		vector <Rhythm_Element>& rp(harmonator->rhythm_pattern);
		int clock_cum=0;
		unsigned int voice_sequence_size, rhythm_sequence_size;
		harmonator->parse_rhythm();
		harmonator->parse_part_string();
		rhythm_sequence_size=harmonator->rhythm_pattern.size();
		voice_sequence_size=harmonator->part_continuity.size();

		for (
			 std::vector <Rhythm_Element>::iterator rhythm_iter(rp.begin());
			 rhythm_iter!=rp.end();
			 rhythm_iter++
			 ){
		  clock_cum = clock_cum + (*rhythm_iter).duration;
		}

		// 1. Calc lcm(rhythm,voice)
		// 2. Divide 1 by voice size
		// 3. mult. 2 by rhythm clock sz
		// 4. create music_clock using 3
		// 5. output duration display of 4
		music_clock rhythm_music_time (
					   0, // meas
					   0, // bt
					   clock_cum*
					   (
						lcm(
							voice_sequence_size,
							rhythm_sequence_size)/rhythm_sequence_size
						),
					   harmonator->main_clock.pulses_per_beat,
					   harmonator->main_clock.beats_per_measure
					   );

		char mclk_str[15];
		sprintf(mclk_str,"%04d:%02d:%02d",
				rhythm_music_time.measure,
				rhythm_music_time.beat,
				rhythm_music_time.offset);

		harmonator->gui->pattern_music_length->value(mclk_str);

	  }
	}
  };

}

void Harmonator::test(){
  
}

Harmonator::Harmonator():
  main_clock(-1,3,23,24,4)// intialize music clock so upon
						  // 1st("start") midiclock message the music
						  // clock will be at 0 measure, 0 beats, and 0 offset
{
  this->notes[0]=-1;
  this->notes[1]=-1;
  this->notes[2]=-1;
  this->bass_note=-1;
  this->bass_channel=1;
  this->midi_clock_counter=-1;
  scheduled_notesoff=NULL;
  //pulses_per_beat=24;
  pattern_trigger=TRIGGER_CHORD;
  dynamics_scale["rest"]=midi_data(0);
  dynamics_scale["ff"]=midi_data(105);
  dynamics_scale["f"]=midi_data(85);
  dynamics_scale["mf"]=midi_data(65);
  dynamics_scale["mp"]=midi_data(45);
  dynamics_scale["p"]=midi_data(35);
  dynamics_scale["pp"]=midi_data(20);
  memset(chan,1,4);
}
 
void Harmonator::open_seq() {
  /* open sequencer(???). The alsa docs say 2nd 'name' should be
             'default'. Opened for input and output. Mode is blocking which
             I recall means that if a read/write occurs that the program
             will wait until that happens
           */
          if (snd_seq_open(&(this->seq_handle), "default", SND_SEQ_OPEN_DUPLEX, 0) < 0) {
            fprintf(stderr, "Error opening ALSA sequencer.\n");
            exit(1);
          }
          snd_seq_set_client_name((this->seq_handle), "Harmonator");
    	  // create i/o ports for this client so it can input from a midi
    	  // source and output to presumably a synth
          if (
  			(midi_in_port=snd_seq_create_simple_port(
    					(this->seq_handle), 
    					"MIDI IN",
    					SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE, /* writeable,subscribable*/
    					SND_SEQ_PORT_TYPE_APPLICATION /* port belongs to an app */
  													 )) < 0
    		  ||
  			(midi_out_port=snd_seq_create_simple_port(
    					(this->seq_handle), 
    					"MIDI OUT",
    					SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ, /* readable,subscribable*/
    					SND_SEQ_PORT_TYPE_APPLICATION /* port belongs to an app */
  													  )) < 0
    		  ) {
            fprintf(stderr, "Error creating sequencer port.\n");
            exit(1);
          }
}



// void Harmonator::decr_duration_counters(){
//   if (scheduled_latch)
// 	scheduled_latch->counter--;
//   if (scheduled_notesoff)
// 	scheduled_notesoff->counter--;
// }

void Harmonator::set_mclk_watchdog(){
  // Recalculate a max expected period for the next midi clock. I
  // intend on calculating an average and std deviation. The expected
  // period will be the average + some multiple of the deviation. As a
  // starting figure use the pulses per beat as a starting value.
  log("mclk watchdog stub\n");
}


void Harmonator::service_conditions(){
  // service sched chord latch before other stuff to allow noteons to depart proper
//   if (scheduled_latch){
// 	if (scheduled_latch->mclock==main_clock)
// 	  // If we have a scheduled latch and chord hold is active latch
// 	  // the sched chord. Else the user has to be hold chord until
// 	  // latch event occurs.
// 	  if (chord_hold || note_buffer == *(scheduled_latch->chord_notes))
// 		{
// 		  latched_chord = *scheduled_latch->chord_notes;
// 		}
// 	delete scheduled_latch;
//   }

//   if (scheduled_notesoff){
// 	if (!scheduled_notesoff->counter){
// 	  // send noteoff midis
// 	  std::vector<int>::iterator iter;
// 	  for (
// 		   iter = scheduled_notesoff->notes_to_off->begin();
// 		   iter != scheduled_notesoff->notes_to_off->end();
// 		   iter++)
// 		send_midi_note(0,*iter,0);
// 	}
// 	delete scheduled_notesoff;
//   }
}



void Harmonator::do_latch(){
  // Called when a midi noteon rc'd or a latch-next key is pressed
		bool chord_beat_sync =gui->lsync_beat->value()==1;
		bool chord_measure_sync = gui->lsync_measure->value()==1;
		bool chord_now_sync = gui->lsync_now->value()==1;
		bool chord_ready = chord_buffer.is_defined() && chord_now_sync;

		if (scheduled_notesoff){
		  //gotta chord input, beat sync button pressed and clock not on a beat
		  if(
			 chord_beat_sync &&
			 chord_buffer.size() &&
			 !main_clock.is_match(string("*:*:00"))
			 )
			{
			  //schedule latch on next beat
			  printf("beat latch\n");
			  music_clock mc(main_clock);
			  mc=mc.beat_floor()+music_clock::unit_beat();
			  assert(chord_buffer.size());
			  scheduled_latch=new Scheduled_Event(mc,chord_buffer);
			}

		  else if (
				   chord_measure_sync  &&
				   chord_buffer.size() &&
				   !main_clock.is_match(string("*:01:00"))
				   )
			{
			  if (!main_clock.is_match(string("*:01:00"))){
				//schedule latch on next measure
				//printf("measure latch\n");
				music_clock mc(main_clock);
				mc=mc.measure_floor()+music_clock::unit_measure();
				assert(chord_buffer.size());
				scheduled_latch=new Scheduled_Event(mc,chord_buffer);
				matvisit(midi_action_test::SCHEDULED_MEASURE_LATCH);
			  }
			}

		  else if (chord_now_sync) {
			// Handles "now" latch-next
			switch (chord_buffer.size()){
			case 4:
			case 3:
			  latched_chord=get_amended_chord
				(
				 chord_buffer,
				 gui->four_part->value(),
				 gui->adjust_root->value()
				 );
			  break;
			case 2:
			case 1:
			  latched_chord=chord_buffer;
			  break;
			};

			update_chord_display(latched_chord);
		  }

		} else {
		  pitch_assemblage pa;
		  main_clock.reset();		
		  // scheduled_notesoff == NULL 1st chordpress????
			// a means to signal initiation of arpeggiation???
		  latched_chord=
			chord_buffer.size()>=3 && chord_now_sync ?
			get_amended_chord(
							  chord_buffer,
							  gui->four_part->value(),
							  gui->adjust_root->value()
							  ):
			chord_buffer;
		  
		  update_chord_display(latched_chord);
		  scheduled_notesoff=
			new Scheduled_Event(main_clock+music_clock::unit_clock(),pa);
		  matvisit(midi_action_test::FIRST_CHORD_CAPTURE);			
		}
}


/**
   Handles incoming midi events from keyboard
*/
void Harmonator::midi_action() { //n-begin
  snd_seq_event_t *ev;
  // save a timestamp for later
  do {//n-a
	snd_seq_event_input(seq_handle, &ev);
	if (arp_armed){//n-b
	  // little kludge to handle keyboard which substitutes noteon
	  // with 0 velocity for noteoff events
	  if (ev->type==SND_SEQ_EVENT_NOTEON && ev->data.note.velocity==0)//n-c
		ev->type = SND_SEQ_EVENT_NOTEOFF;
	  switch (ev->type) {//n-d


	  case SND_SEQ_EVENT_CLOCK:{
		bool chord_beat_sync =gui->lsync_beat->value()==1;
		bool chord_measure_sync = gui->lsync_measure->value()==1;
		bool chord_now_sync = gui->lsync_now->value()==1;

		main_clock++;

		  // synchonization with a test driver
		  matvisit(midi_action_test::MIDI_CLOCK_CAPTURE);


		  //cout << __FILE__<< "("<<__LINE__<<")"<<"main_clock:" << main_clock;
		  // 		if (scheduled_notesoff){
		  // 		  cout << "scheduled_notesoff->mclock:"<<scheduled_notesoff->mclock;
		  // 		}
		  //		cout << endl;



		  // Code to update metronome display
		  update_metronome();

		  // recalc expected clock		
		  bool chord_ready = chord_buffer.is_defined();
		  bool scheduled_latch_ready=(scheduled_latch && scheduled_latch->mclock==main_clock);

		  if (scheduled_latch_ready){//n-e
			// A latch was previously scheduled so either begin
			// arpeggiation based upon the scheduled latch or if the
			// user happens to be playing a chord now, override the
			// scheduled latch with the current chord.

			if (
				chord_buffer.size() &&
				(chord_beat_sync || chord_measure_sync || chord_now_sync)
				){//n-g
			  // Override sched latch if there's chord currently pressed.

			  // chord available and one of the latch buttons is enabled.
			  latched_chord=get_amended_chord(
											  chord_buffer,
											  gui->four_part->value(),
											  gui->adjust_root->value()
											  );
			  matvisit(midi_action_test::SCHEDULED_LATCH_OVERRIDE);
			}
			else{//n-f
			  cout << __FILE__<< "("<<__LINE__<<"):scheduled latch"<<endl;
			  latched_chord=get_amended_chord(
											  *scheduled_latch->payload.chord_notes,
											  gui->four_part->value(),
											  gui->adjust_root->value()
											  );
			  matvisit(midi_action_test::SCHEDULED_CHORD_CAPTURE);
			}
			update_chord_display(latched_chord);//n-h
			delete scheduled_latch;
			scheduled_latch=NULL;
		  }


		  // Check the scheduled noteoff event. If active send the
		  // notesoff message and set duration_expired
		  if (scheduled_notesoff && scheduled_notesoff->mclock==main_clock){//n-i

			//pitch_assemblage& pa=*scheduled_notesoff->payload.notes_to_off;
			//std::set<midi_data,mfunctor>::const_iterator iter=pa.begin();
			//cout << __FILE__<< "("<<__LINE__<<"): sched noteoff"<<endl;

			if (scheduled_notesoff->type==2){//n-j
			  const vector<note_event>& sent_notes(*(scheduled_notesoff->payload.sent_notes));
			  //printf("\n(%u) turn off(sent_notes) %p",clock(),&sent_notes);
			  for (//n-k
				   vector<note_event>::const_iterator iter=sent_notes.begin();
				   iter!=sent_notes.end();
				   iter++)
				{
				  note_event copy(*iter,true);
				  send_midi_note(copy);
				}
			}
			  delete scheduled_notesoff;//n-l
			  scheduled_notesoff=NULL;
			  duration_expired=true;
		  }

		  // The 'duration_expired' signals a condition in which
		  // note(s) in the arpeggiation should be sent.
		  if (duration_expired){//n-m
			// A chord(pitch assemblage) is latched for arp
			if (latched_chord.size()){//n-n
			  // Fetch the string containing the current parts. Create an
			  // iterator for that string. Assemble a pitch_assemblage
			  // that holds notes to turn on and later turn off. Construct
			  // a scheduled event which holds the turnoff deadline and
			  // the notes to turn off. Finally send the noteons.
			  string current_parts(**part_walker);
			  sort(current_parts.begin(),current_parts.end());

			  char chan;
			  vector<note_event> sent_notes;

			  duration_expired=false;


			  // 			cout<<"current_parts "<<current_parts<<endl;
			  // 			cout << "latched chord functions" << endl ;
			  // 			cout<< latched_chord.Sxp;




			  // by *rhythm_walker.value().
			  //cout << "main_clock:" << main_clock << endl;
			  //cout << "scheduled_notesoff->mclock:"<<scheduled_notesoff->mclock << endl;

			  for (//n-q
				   string::iterator cp_iter=current_parts.begin();
				   cp_iter!=current_parts.end();
				   cp_iter++
				   ){
				midi_data md(latched_chord[*cp_iter-'a']);
				switch (*cp_iter){//n-s
				case 'a':chan=gui->a_channel->value()-1;break;
				case 'b':chan=gui->b_channel->value()-1;break;
				case 'c':chan=gui->c_channel->value()-1;break;
				case 'd':chan=gui->d_channel->value()-1;break;
				default:assert(0);
				};
				note_event *ne = new note_event(chan,md.midi_value,(**rhythm_walker).loudness->midi_value,this->midi_out_port);
				send_midi_note(*ne);
				sent_notes.push_back(*ne);
			  }

			  scheduled_notesoff=
				new Scheduled_Event(main_clock+music_clock::unit_clock((**rhythm_walker).duration),sent_notes);
			  //printf("\n(%u) sent_notes = %p",clock(),scheduled_notesoff->payload.sent_notes);

			  assert(*(scheduled_notesoff->payload.sent_notes)==sent_notes);
			  assert(scheduled_notesoff->payload.sent_notes != &sent_notes);

			} else {//n-o
			  // No chord latched but still need to traverse the rhythm pattern
			  // schedule a dummy "notesoff"
			  pitch_assemblage pa;
			  scheduled_notesoff=
				new Scheduled_Event(main_clock+music_clock::unit_clock((**rhythm_walker).duration),pa);
			  matvisit(midi_action_test::PATTERN_BUMP);
			}
			(*rhythm_walker)++;//n-p
			(*part_walker)++;
		  }
		break;//n-u
	  }
	  case SND_SEQ_EVENT_NOTEON:{
		// We've received a midi noteon event. 
		// Add it to chord_buffer(no duplicates).
		// Update the chord buffer display
		//
		chord_buffer.add(ev->data.note.note);
		update_chord_display(chord_buffer,true);

		do_latch();

		break;
	  }

	  case SND_SEQ_EVENT_NOTEOFF:
		//	  pitch_assemblage_iterator iter=chord_buffer.Sxp.begin();
		//cout << "rc'd noteoff:0x" <<hex<<int(ev->data.note.note)<<dec<<endl;
		//cout << chord_buffer.Sxp<<endl;
		chord_buffer.remove(ev->data.note.note);
		
		for (int i=0;i<4;i++)
		  gui->chord_fnc_display[i]->value("*");

		for (
			 std::vector<midi_data>::iterator iter=chord_buffer.begin();
			 iter!=chord_buffer.end();
			 iter++){
		  char s[7];
		  sprintf(s,"0x%x",(*iter).midi_value);
		  assert(iter-chord_buffer.begin() >= 0);
		  assert(iter-chord_buffer.begin() < 4);
		  gui->chord_fnc_display[iter-chord_buffer.begin()]->value(s);
		}

		bool chord_beat_sync =gui->lsync_beat->value()==1;
		bool chord_measure_sync = gui->lsync_measure->value()==1;
		bool chord_now_sync = gui->lsync_now->value()==1;
		bool chord_hold = gui->chord_hold->value()==1;
		bool chord_ready = chord_buffer.is_defined() && (chord_beat_sync || chord_measure_sync || chord_now_sync);

		if (!(chord_ready||chord_hold)){
		  latched_chord.clear();
		}
		break;
	  }
	}
	snd_seq_free_event(ev);
  } while (snd_seq_event_input_pending(seq_handle, 0) > 0);
}  


void Harmonator::set_chord_functions(const char *fnc){
  log("set gui chord func boxes");
}

/**
   send a "kill all notes" midi mode message
*/
void Harmonator::kill_sound() {
  snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);
    snd_seq_ev_set_source(&ev,this->midi_out_port);
    snd_seq_ev_set_subs(&ev);
    snd_seq_ev_set_direct(&ev);
    snd_seq_ev_set_controller(&ev,0,123,0);
    snd_seq_event_output(this->seq_handle,&ev);
    snd_seq_drain_output(this->seq_handle);
}



/**
   Parse the rhythm string entered by user into rhythm tokens
*/
void Harmonator::parse_rhythm() {
  // extract out comma separated tokens. Return the number of tokens
  // extracted. The tokens are passed back via 'tokens'
  char *str_of_tokens=strdup(gui->rhythm_string->value());
  char *rhythm_tokens_ptr;
      	  
            
  rhythm_tokens_ptr = strtok(str_of_tokens,",");
  if (rhythm_tokens_ptr){
	rhythm_pattern.clear();
  
	while (rhythm_tokens_ptr){
	  try {
		rhythm_pattern.push_back(*parse_rhythm_element(rhythm_tokens_ptr));
	  }
	  catch (rhythm_parse_exception rpe){
		// adjust psn and rethrow
		throw rhythm_parse_exception(rhythm_tokens_ptr-str_of_tokens+rpe.cp);
	  }
	  rhythm_tokens_ptr=strtok(NULL,",");
	}
  }
  else {
	free(str_of_tokens);
	throw rhythm_parse_exception(0);
  }
}


/**
   parse parts string
*/
void Harmonator::parse_part_string() {
  // extract out comma separated tokens. Return the number of tokens
    // extracted. The tokens are passed back via 'tokens'
  string str_of_tokens(gui->part_string->value());
  string allowed_parts("abcd");
  string current_token;
  part_continuity.clear();
  string::iterator begin_token,end_token;
  begin_token=str_of_tokens.begin();
  end_token=str_of_tokens.begin();
  do {
	if (*end_token==','){
	  current_token=string(begin_token,end_token);
	  if (current_token.length()>0)
		part_continuity.push_back(current_token);
	  else
		throw voice_parse_exception(begin_token-str_of_tokens.begin());

	  begin_token=end_token+1;
	}
	else if (allowed_parts.find(*end_token)==string::npos)
	  throw voice_parse_exception(end_token-str_of_tokens.begin());

  } while (++end_token!=str_of_tokens.end());

  current_token=string(begin_token,end_token);
  if (current_token.length()>0)
	part_continuity.push_back(current_token);
  else
	throw voice_parse_exception(begin_token-str_of_tokens.begin());
}





int Harmonator::get_current_velocity() {
  // convert from a musical loudness to a midi velocity value
      if (!this->tokens[1])//null ptr
    	return(100);
    
      if (strcasecmp(this->tokens[1],"r")==0)
      	{
      	  return(dyn.rest);
      	}
      else if (strcasecmp(this->tokens[1],"pp")==0)
      	{
      	  return (dyn.pp);
      	}
      else if (strcasecmp(this->tokens[1],"p")==0)
      	{
      	  return(dyn.p);
      	}
      else if (strcasecmp(this->tokens[1],"mp")==0)
      	{
      	  return(dyn.mp);
      	}
      else if (strcasecmp(this->tokens[1],"mf")==0)
      	{
      	  return(dyn.mf);
      	}
      else if (strcasecmp(this->tokens[1],"f")==0)
      	{
      	  return(dyn.f);
      	}
      else if (strcasecmp(this->tokens[1],"ff")==0)
      	{
      	  return(dyn.ff);
      	}
      else
  	  return(dyn.ff);
}


Rhythm_Element* Harmonator::parse_rhythm_element(char *rhythm_el) {
  // point at duration which should be a string of set {w,h,q,e,s}
      // example qe=quarter + eighth tied together
      char *cur=rhythm_el;
      char *end;
	  int dur=0;
	  string loudness;
	  // strip away leading whitespace
	  while (strchr(" \n\t",*cur)) cur++;

	  // find other end of this term
      end = (end=strpbrk(cur,"prmf"))?end:cur+strlen(cur);

      while (cur < end){
    	switch (*cur){
      	case 'w': dur+=main_clock.whole();break;
		case 'h': dur+=main_clock.half();break;
      	case 'q': dur+=main_clock.quarter();break;
      	case 'e': dur+=main_clock.eighth();break;
      	case 's': dur+=main_clock.sixteenth();break;
      	case 't': dur+=main_clock.triplet_eighth();break;
		  // throw parse exc with error position
		default : throw rhythm_parse_exception(cur-rhythm_el);
 
    	};
      	cur++;
      }
      // end either points to a string containing a loudness or null character
      if (*end){
    	if (strcasecmp(end,"r")==0)
    	  loudness="rest";
		else {
		  loudness=string(end);
		  // check for valid dynamic
		  if (
			  loudness!="pp" &&
			  loudness!="p"  &&
			  loudness!="mp" &&
			  loudness!="mf" &&
			  loudness!="f"  &&
			  loudness!="ff"
			  )
			throw rhythm_parse_exception(end-rhythm_el);

		}
      }
      else 
    	loudness="f";

      return (new Rhythm_Element(dur,dynamics_scale[loudness.c_str()]));
}

void Harmonator::send_midi_note(note_event& ne){
  snd_seq_event_output(this->seq_handle,&(ne.get_event()));
  snd_seq_drain_output(this->seq_handle);
}

void Harmonator::send_midi_note(int channel, int note_value, int velocity,bool noteon) {
  snd_seq_event_t ev;
    
      	snd_seq_ev_clear(&ev);
      	snd_seq_ev_set_source(&ev,this->midi_out_port);
      	snd_seq_ev_set_subs(&ev);
      	snd_seq_ev_set_direct(&ev);
		if (noteon)
		  snd_seq_ev_set_noteon(&ev,channel,note_value,velocity);
		else
		  snd_seq_ev_set_noteoff(&ev,channel,note_value,velocity);
    	snd_seq_event_output(this->seq_handle,&ev);
      	snd_seq_drain_output(this->seq_handle);
}


void Harmonator::HandleAlsaPort(int fd, void *data) {
  ((Harmonator *)data)->midi_action();
}

void Harmonator::update_chord_display(const Chord_Notes& cn, bool ctl){
  Fl_Output **display = ctl?gui->chord_fnc_display:gui->latch_fnc_display;

  std::vector<midi_data>::const_iterator iter=cn.begin();
  for (int i=0;i<4;i++)
	display[i]->value("*");

  for (int i=0;iter!=cn.end();iter++,i++)
	try {
	  char s[7];
	  sprintf(s,"0x%x",(*iter).midi_value);
	  display[i]->value(s);
	}
	catch(...) {

	}
}

void Harmonator::update_metronome(){
  char control[9]="\x00\x00\x00\x00\x00\x00\x00\x00";

  if (arp_armed && scheduled_notesoff){
	char mclk_str[15];
	sprintf(mclk_str,"%04d:%02d:%02d",
			main_clock.get_measure(),
			main_clock.get_beat(),
			main_clock.offset);

	gui->music_clock_coord->value(mclk_str);
	if (main_clock.is_match("*:*:00")){
	  // handle last light 
	  int bpm=gui->beats_per_measure->value();

	  // on last beat of measure, turn off 1st beat light
	  if (main_clock.beat==bpm-1){
		for (int i=0;i<bpm;i++)
		  switch(i){
		  case 0:
			control[i] ='-';
			break;
		  default:
			control[i]=i<bpm-1?' ':'+';
		  };
	  }
	  else {
		// on a measure boundary
		if (main_clock.is_match("*:01:00")){
		  control[0]='+';
		  for (int i=1;i<bpm;i++)
			control[i]='-';
		}
		// beat boundary
		else if (main_clock.is_match("*:*:00")){
		  for (int i=0;i<main_clock.beat;i++)
			control[i] = ' ';
		  control[main_clock.beat]='+';
		}
	  }
	  //cout << "control("<<main_clock<<")="<<control<<endl;
	  gui->update_metronome_display(control);
	  if (gui->send_midi_click->value() && main_clock.is_match("*:01:00")){
		this->send_midi_note(9,34,110);
	  }  
	  else if (gui->send_midi_click->value())
		this->send_midi_note(9,33,75);
	}
  } else {
	// arp armed but has not been initiated
	memset(control,' ',7);
	if (main_clock.get_offset()==0){
	  control[0]='+';
	  if (gui->send_midi_click->value())
		this->send_midi_note(9,33,75);
	}
	else if (main_clock.get_offset()>=((main_clock.pulses_per_beat*3)/4)){
	  control[0]='-';
	}
	gui->update_metronome_display(control);
  }
}

// fct Harmonator::emit_alsa_handler(){
//   void HandleAlsaPort(int fd, void *data) {
// 	//((HarmonatorUI *)data)->midi_action();
//   }
//   return (&HandleAlsaPort);
// }

int slotcmp(const void *x,const void *y) {
  const char i = *(const char *)x;
    const char j = *(const char *)y;
    return (int)(i > j) - (int)(i < j);
// @@
}


#if defined HARMONATOR_TEST

pthread_t test_thread;

// thread function
void *test_thread_fnc(void *obj){
  Harmonator::midi_action_test *mat=(Harmonator::midi_action_test *)obj;
  mat->runTest();
}

void tcb(void *t){
  //runTest= &Harmonator::midi_action_test::runTest;
  if (pthread_create(
		&test_thread,
		NULL,
		test_thread_fnc,
		t		
					  )
	  )
	throw "bad thread create";//XXX fixup
}



int main(int argc, char *argv[])
{
  if (argc>1){
	if (strcmp(argv[1],"ChordWizard")==0) {
	  CppUnit::TextUi::TestRunner runner;
	  runner.addTest( ChordWizard::ChordWizardTest::suite() );
	  runner.run();
	  return 0;
	}
	else if (strcmp(argv[1],"midi_action")==0){
	  int npfd=0;
	  struct pollfd* pfd;
	  Harmonator *harmonator = new Harmonator;
	  printf("harmonator=%p\n",harmonator);
	  HarmonatorUI hui(harmonator);
	  //cout << hui.cfg_fn << endl;
	  printf("hui.harmonator=%p\n",hui.harmonator);
	  harmonator->gui = &hui;

	  hui.install_cb((void *)&Harmonator::gui_callback_handler);

	  // open alsa client
	  harmonator->open_seq();
	  int y=break_mark(0);
	  log("After alsa setup");
  
	  /* I'm guessing this is the number of sequencer input ports */
	  npfd = snd_seq_poll_descriptors_count(harmonator->seq_handle, POLLIN);
  
	  // Extract file descriptors from alsa plumbing and pass on to fltk
	  // event plumbing which seems to use the 'select' mechanism
	  pfd = (struct pollfd *)alloca(npfd * sizeof(struct pollfd));
	  snd_seq_poll_descriptors(harmonator->seq_handle, pfd, npfd, POLLIN);
  
	  for (int i = 0; i < npfd; i++)
		{
		  // this form looks for POLLIN events
		  hui.register_fd_handler(pfd[i].fd,Harmonator::HandleAlsaPort);
		}
  
	  hui.show(argc, argv);

	  harmonator->mat = new Harmonator::midi_action_test("mat",*harmonator);

	  // fire off midi_action_test
	  Fl::add_timeout(1.0,tcb,harmonator->mat);

	  Fl::lock(); //enable multithreading 
	  hui.loop_gui();


	  return(0);

	}
	
  } else {
	cout << "harmonator(test configuration)" << endl;
	cout << "harmonator [testname]" << endl;
	cout << "Tests: ChordWizard,midi_action" << endl;
  }
  return 0;
}
#else
// normal entry
int main(int argc, char **argv) {
  int npfd=0;
  struct pollfd* pfd;
  time_t start;

				 
  //HarmonatorUI *hui=new HarmonatorUI;
  Harmonator *harmonator = new Harmonator;
  printf("harmonator=%p\n",harmonator);
  HarmonatorUI hui(harmonator);
  //cout << hui.cfg_fn << endl;
  printf("hui.harmonator=%p\n",hui.harmonator);
  harmonator->gui = &hui;

  hui.install_cb((void *)&Harmonator::gui_callback_handler);

  // open alsa client
  harmonator->open_seq();
  int y=break_mark(0);
  log("After alsa setup");
  
  /* I'm guessing this is the number of sequencer input ports */
  npfd = snd_seq_poll_descriptors_count(harmonator->seq_handle, POLLIN);
  
  // Extract file descriptors from alsa plumbing and pass on to fltk
  // event plumbing which seems to use the 'select' mechanism
  pfd = (struct pollfd *)alloca(npfd * sizeof(struct pollfd));
  snd_seq_poll_descriptors(harmonator->seq_handle, pfd, npfd, POLLIN);
  
  for (int i = 0; i < npfd; i++)
    {
  	// this form looks for POLLIN events
	  hui.register_fd_handler(pfd[i].fd,Harmonator::HandleAlsaPort);
    }
  
  hui.splash->show();
  start=time(NULL);
  do {
	Fl::wait(3);
  } while (difftime(time(NULL),start)<3.0);
  hui.splash->hide();
  	hui.show(argc, argv);
  
	return(hui.loop_gui());
}
#endif





int break_mark(int i){
  return(i+1);
}


#ifdef HARMONATOR_TEST


void ChordWizard::ChordWizardTest::setUp()
{
  //  cn = new Chord_Notes();
  cwz = new ChordWizard();
}

void ChordWizard::ChordWizardTest::tearDown() 
{
  delete cwz;
  //delete cn;
}


void ChordWizard::ChordWizardTest::test_get_root(){
  Chord_Notes *cnotes;
  cnotes=new Chord_Notes;
  cnotes->add(60);
  cnotes->add(64);
  cnotes->add(67);
  CPPUNIT_ASSERT(cwz->get_root(*cnotes)==midi_data(60));
  delete cnotes;
  cnotes=new Chord_Notes;
  cnotes->add(60+12);
  cnotes->add(64);
  cnotes->add(67);
  CPPUNIT_ASSERT(cwz->get_root(*cnotes)==midi_data(60+12));
  delete cnotes;
  cnotes=new Chord_Notes;
  cnotes->add(60+12);
  cnotes->add(64+12);
  cnotes->add(67);
  CPPUNIT_ASSERT(cwz->get_root(*cnotes)==midi_data(60+12));
  delete cnotes;

}


void ChordWizard::ChordWizardTest::test_is_root_position(){
  Chord_Notes *cnotes;
  cnotes=new Chord_Notes;
  cnotes->add(60);
  cnotes->add(64);
  cnotes->add(67);
  CPPUNIT_ASSERT(cwz->is_root_position(*cnotes)==true);
  delete cnotes;
  cnotes=new Chord_Notes;
  cnotes->add(60+12);
  cnotes->add(64);
  cnotes->add(67);
  CPPUNIT_ASSERT(cwz->is_root_position(*cnotes)==false);
  delete cnotes;
  cnotes=new Chord_Notes;
  cnotes->add(60+12);
  cnotes->add(64+12);
  cnotes->add(67);
  CPPUNIT_ASSERT(cwz->is_root_position(*cnotes)==false);
  delete cnotes;

}


Harmonator::midi_action_test::midi_action_test(string name,Harmonator& container):TestCase(name),container(container),spts(set<SYNC_POINT>()),hold_flag(false){
}


void Harmonator::midi_action_test::signal_visit(SYNC_POINT local_sp){
#ifdef HARMONATOR_TEST
  pthread_mutex_lock(&midi_action_test::mut);
  if (spts.find(local_sp)!=spts.end()){
	pthread_cond_broadcast(&midi_action_test::cond);
	if (hold_flag)
	  pthread_cond_wait(&hold, &mut);
  }
  pthread_mutex_unlock(&midi_action_test::mut);
#endif
}
void Harmonator::midi_action_test::release_visit(SYNC_POINT local_sp){
  pthread_mutex_lock(&midi_action_test::mut);
  if (spts.find(local_sp)!=spts.end()){
	pthread_cond_broadcast(&midi_action_test::hold);
	spts.clear();
  }
  pthread_mutex_unlock(&midi_action_test::mut);
}

void Harmonator::midi_action_test::wait_for_visit(SYNC_POINT local_sp,bool set_hold){
  // create a single element chk set
  pthread_mutex_lock(&mut);
  spts.clear();
  spts.insert(local_sp);
  hold_flag=set_hold;
  pthread_cond_wait(&cond, &mut);
  pthread_mutex_unlock(&mut);
}

void Harmonator::midi_action_test::wait_for_visit(set<SYNC_POINT>&  spts,bool set_hold){
  pthread_mutex_lock(&mut);
  hold_flag=set_hold;
  this->spts = spts;
  pthread_cond_wait(&cond, &mut);
  pthread_mutex_unlock(&mut);
}


void Harmonator::midi_action_test::runTest(){
  struct timespec tmo;
  char c;
  set_input_mode();
  Fl::lock();
  container.gui->arpeggiate_switch->value(1);
  container.gui->arpeggiate_switch->do_callback();
  Fl::unlock();
  cout << "Start up the midiclock application, connect, and enable it. Press return to continue." << endl;
  read(STDIN_FILENO, &c, 1);
  cout << "Waiting for midi_action to sync at MIDI_CLOCK_CAPTURE"<<endl;
  wait_for_visit(MIDI_CLOCK_CAPTURE,true);
  cout << "Midi clocks captured" << endl;
  // chk main_clock
  CPPUNIT_ASSERT(container.main_clock.clock_value>=0);
  // chks????
  release_visit(MIDI_CLOCK_CAPTURE); // allow other thread to continue
  cout << "Releasing the midi clock block" << endl;
  cout << "Press a key to continue then play a midi file." << endl;
  read(STDIN_FILENO, &c, 1);
  cout << "Waiting for midi_action to sync at FIRST_CHORD_CAPTURE"<<endl;
  wait_for_visit(FIRST_CHORD_CAPTURE,true);
  cout << "First chord rc'd" << endl;
  // chk main_clock
  CPPUNIT_ASSERT(container.scheduled_notesoff!=NULL);
  CPPUNIT_ASSERT(container.main_clock.clock_value==0);
  release_visit(FIRST_CHORD_CAPTURE); // allow other thread to continue
  cout << "Releasing 1st chord block" << endl;
  cout << "Press a key to continue then play 'test90' midifile." << endl;
  read(STDIN_FILENO, &c, 1);
  cout << "Waiting for midi_action to sync at PATTERN_BUMP"<<endl;
  wait_for_visit(PATTERN_BUMP);
  cout << "Pattern bump visited" << endl;
  // trigger a scheduled latch
  cout << "Engaging measure latch-next event" << endl;
  Fl::lock();
  container.gui->lsync_measure->value(1);
  container.gui->lsync_measure->do_callback();
  Fl::unlock();
  cout << "Press a key to continue then play 'test110." << endl;
  read(STDIN_FILENO, &c, 1);
  // begin playing midifile
  cout << "Waiting for midi_action to sync at SCHEDULED_CHORD_CAPTURE"<<endl;
  wait_for_visit(SCHEDULED_CHORD_CAPTURE,true);
  cout << "midi_action has sync'd on SCHEDULED_CHORD_CAPTURE" << endl;
  // chks
  cout << "Releasing hold at SCHEDULED_CHORD_CAPTURE" << endl;
  release_visit(SCHEDULED_CHORD_CAPTURE);
  // faking a condition in which a schd latch is defined but ovrd with chord_buffer
  // sync to creation of a sch latch create
  cout << "Waiting for midi_action to sync at SCHEDULED_MEASURE_LATCH"<<endl;
  wait_for_visit(SCHEDULED_MEASURE_LATCH);
  cout << "midi_action has sync'd on  SCHEDULED_MEASURE_LATCH" << endl;
  cout << "Syncing music clock to a measure boundary" << endl;
  do {
	wait_for_visit(MIDI_CLOCK_CAPTURE,true);
	if (!container.main_clock.is_match("*:01:00")){
	  release_visit(MIDI_CLOCK_CAPTURE);
	  continue;
	}
	break;
  } while (true);
  cout << "Sync'd music clock to a measure boundary" << endl;
  cout << "Adjusting chord_buffer" << endl;
  assert(container.chord_buffer.size()==0);
  container.chord_buffer.add(60+12);
  container.chord_buffer.add(64+12);
  container.chord_buffer.add(67+12);
  release_visit(MIDI_CLOCK_CAPTURE);
  cout << "Releasing midi clock hold" << endl;
  reset_input_mode();
  cout<<"End of test" << endl;
}

#endif
