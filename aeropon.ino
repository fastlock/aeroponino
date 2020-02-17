/*
     |
    \|/|/      
  \|\\|//|/    |
   \|\|/|/    \|/
    \\|//    \ | /
     \|/      \|/
     \|/       |
      |        |
|=====================|
|     /|\      /|\    | 
|     AEREOPONINO     |
|_\|/__\|/__\|/__\|/_ |
|=====================|
****************************
  Water controller
  FW_Version:0.0.1
  Author:Fastlock
  Board:Arduino/Genuino Uno
****************************

 */

//--------------HARDWARE PARAMETERS-----------------------
#define HP_WP_A_PIN 12
#define HP_WP_B_PIN 11
#define DEVIATOR_PIN 10
#define AGITATOR_PIN 9
#define TOP_TANK_PIN 8
#define BOTTOM_TANK_PIN 2
//--------------STATE MACHINE PARAMETERS-------------------
#define IDLE_CYCLE_LIMIT 2
#define DELAY_IRRIGATION  10 //utes
#define IRRIGATION_DURATION 5//seconds
volatile int General_state=0;
bool is_night=false;
volatile bool pump_sel;//0->A,1->B
int idle_cycle_count=0;
bool tank_full=true;
int pumps[2]={HP_WP_A_PIN,HP_WP_B_PIN};
bool recirc_state=false;
long int current=0;
//---------------------------------------------------------
long int t_start,t_stop;
bool timer_armed=false;
void Set_timer1(uint16_t seconds)
{
    timer_armed=true;
    t_start=millis();
    t_stop=millis()+(seconds*1000);
}
void watchtime(){
  current=millis();
  if((current>=t_stop)&&timer_armed){
    timeout();
  }
}
void timeout ()
{
    switch(General_state){
      case 0:
        //return from idle,start watering
        timer_armed=false;
        watering();
        
        break;
      case 1:
        //return from watering,start idle
        timer_armed=false;
        idle();
        
        break;
    }
}
void alt(int sel){
  //serve a fermare selettivamente una funzione rispetto ad un altra
  //1-> solo pompe irrigazione
  //0->tutti gli attuatori
  if(sel==1){
     digitalWrite(HP_WP_B_PIN,LOW);
     digitalWrite(HP_WP_A_PIN,LOW);
  }
  else{
    digitalWrite(HP_WP_B_PIN,LOW);
    digitalWrite(HP_WP_A_PIN,LOW);
    digitalWrite(AGITATOR_PIN,LOW);
    digitalWrite(DEVIATOR_PIN,LOW);
  }
}
bool is_full(){
  //controlla se il serbatoio è pieno
  if(digitalRead(TOP_TANK_PIN)==HIGH){
     tank_full=true;
     return true;
  }
  else {
    tank_full=false;
    return false;
  }
  
}
bool daywatch(){
  if(is_night){
    sleep();
  }
  else
  return false;
}
void idle(){ 
  General_state=0;
  alt(1);//fermo le pompe
  idle_cycle_count++;
  if(idle_cycle_count >= IDLE_CYCLE_LIMIT){
    idle_cycle_count=0;
    recirculation(!recirc_state);//avvio il ricircolo un ciclo si e uno no
  }
  daywatch();
  Set_timer1(DELAY_IRRIGATION);//aspetto 2 min
}
void watering(){
  General_state=1;
  if(pump_sel)pump_sel=false;
  else pump_sel=true;
  digitalWrite(pumps[pump_sel],HIGH);//prima pompa A e dopo pompa B
  Set_timer1(IRRIGATION_DURATION);// irrigo per 20 s
  
}
void sleep(){
  General_state=2;
  alt(0);//fermo tutto
  while(1){
  if(daywatch()){
    idle();
  }
  }
  
}

void recirculation(bool state){
  if(state){
    digitalWrite(DEVIATOR_PIN,LOW);
    digitalWrite(AGITATOR_PIN,HIGH);
  }
  else{
    digitalWrite(DEVIATOR_PIN,LOW);
    digitalWrite(AGITATOR_PIN,LOW);
  }
  recirc_state=state;
}

void setup(){
  pinMode(HP_WP_A_PIN,OUTPUT);
  pinMode(HP_WP_B_PIN,OUTPUT);
  pinMode(AGITATOR_PIN,OUTPUT);
  pinMode(DEVIATOR_PIN,OUTPUT);
  pinMode(TOP_TANK_PIN,INPUT);
  pinMode(BOTTOM_TANK_PIN,INPUT);
  idle();
  attachInterrupt(0, fill,FALLING);
  
}

void loop(){
  watchtime();//controllo l'orologio
}
void fill(){
  //registro lo stato precendente all'interrupt
  int past_state=General_state;
  int stat_a=digitalRead(HP_WP_A_PIN);
  int stat_b=digitalRead(HP_WP_B_PIN);
  alt(0);//fermo tutto
  General_state=3;
  while(!is_full()){
    digitalWrite(DEVIATOR_PIN,HIGH);
    digitalWrite(AGITATOR_PIN,HIGH);
  }
  alt(0);
  //ristabilisco lo stato dopo l'esecuzione dell'interrupt
  General_state=past_state;
  digitalWrite(HP_WP_A_PIN,stat_a);
  digitalWrite(HP_WP_B_PIN,stat_b);
}

