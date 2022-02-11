#include <Arduino.h>
#include <DaisyDuino.h>

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Tlv493d.h>
#include <Arduino_APDS9960.h>

#include <cmath>
#include "sounds.h"

#define M_PI 3.141592653589793

int16_t dummy[] = {0,0,0,0,0,0,0,0,0,0};

volatile float offsetB = 0.0;
volatile float offsetG = 0.0;
volatile float offsetR = 0.0;
volatile bool updateOffset = true;

Tlv493d magnetic = Tlv493d();

DaisyHardware hw;

volatile uint32_t lastTime = 0;
volatile const uint32_t interval = 250;

volatile float intensity = 0.0f;

int red, green, blue;
volatile float rf, gf, bf;

volatile float proximity = 0.0;

volatile bool stateUpdated = false;

volatile uint32_t iteration = 0;

int16_t* localPtr = Bird;
int16_t localMaximum = 0;
int16_t localMinimum = 0;
int16_t localDivision = 1;
int16_t localSize = 1;

EStates mv0, mv1, mv2, mv3, mv4;
EStates lastState;

void AudioCallback(float **in, float **out, size_t size) {
  float localIntensity = 0.0;
  if(state != EStates::None){
    localIntensity = intensity;
  }

  switch(state){
    case EStates::Bird:
      localSize = sizeBird;
      localMaximum = maxBird;
      localMinimum = minBird;
      localDivision = frameDivisionBird;
      break;
    case EStates::Chainsaw:
      localSize = sizeChainsaw;
      localMaximum = maxChainsaw;
      localMinimum = minChainsaw;
      localDivision = frameDivisionChainsaw;
      break;
    case EStates::Fire:
      localSize = sizeFire;
      localMaximum = maxFire;
      localMinimum = minFire;
      localDivision = frameDivisionFire;
      break;
    case EStates::Whale:
      localSize = sizeWhale;
      localMaximum = maxWhale;
      localMinimum = minWhale;
      localDivision = frameDivisionWhale;
      break;
    default:
      localSize = 10;
      localMaximum = 1;
      localMinimum = -1;
      localDivision = 1;
  };

  for (size_t i = 0; i < size; i++) {

    bool birdValid = (localPtr >= Bird) && (localPtr < (Bird + sizeBird));
    bool chainValid = (localPtr >= Chainsaw) && (localPtr < (Chainsaw + sizeChainsaw));
    bool fireValid = (localPtr >= Fire) && (localPtr < (Fire + sizeFire));
    bool whaleValid = (localPtr >= Whale) && (localPtr < (Whale + sizeWhale));
    bool noneValid = (localPtr >= dummy) && (localPtr < (dummy + 10));

    bool inValidRegion = whaleValid || chainValid || fireValid || noneValid || birdValid;

    if(inValidRegion){
      out[0][i] = out[1][i] = localIntensity * float(*localPtr) / float(localMaximum);
    } else{
      out[0][i] = out[1][i] = 0;
    }

    if((++iteration == (localSize * localDivision)) || !inValidRegion || stateUpdated){
      stateUpdated = false;
      iteration = 0;

      switch(state){
        case EStates::Bird:
          localPtr = (int16_t*)Bird;
          break;
        case EStates::Chainsaw:
          localPtr = (int16_t*)Chainsaw;
          break;
        case EStates::Fire:
          localPtr = (int16_t*)Fire;
          break;
        case EStates::Whale:
          localPtr = (int16_t*)Whale;
          break;
        default:
          localPtr = (int16_t*)dummy;
      };
    }else{
      if(iteration % localDivision == 0){
        localPtr++;
      }
    }
  }
}

void setup(){

  pinMode(LED_BUILTIN, OUTPUT);

  mv0 = mv1 = mv2 = mv3 = mv4 = EStates::None;
  lastState = EStates::None;

  hw = DAISY.init(DAISY_SEED, AUDIO_SR_48K);

  float sample_rate = DAISY.get_samplerate();

  for(int i = 0; i < 10; i++){
    digitalWrite(LED_BUILTIN, HIGH);
    delay(40);
    digitalWrite(LED_BUILTIN, LOW);
    delay(40);
  }

  digitalWrite(LED_BUILTIN, HIGH);

  magnetic.begin();
  delay(100);

  if (!APDS.begin()) {
    while (true); // Stop forever
  }
  DAISY.begin(AudioCallback);

  digitalWrite(LED_BUILTIN, LOW);
}


void loop(){
  while ((millis() - lastTime) < interval);

  // Get data from magnetic sensor
  magnetic.updateData();
  float magneticValue = magnetic.getZ();
  float magneticValueAbs = abs(magneticValue);
  lastTime = millis();

  // Check if a proximity reading is available.
  if (APDS.proximityAvailable()) {
    proximity = APDS.readProximity();
  }

  // Update colors
  if(APDS.colorAvailable()){
    APDS.readColor(red, green, blue);
    if(updateOffset){
      updateOffset = false;
      offsetB = float(blue);
      offsetG = float(green);
      offsetR = float(red);
    }
    rf = float(red) - offsetR;
    gf = float(green) - offsetG;
    bf= float(blue) - offsetB;
  }

  if(proximity < 2.0){
    if(gf > bf && bf > rf){
      mv0 = EStates::Bird;
    } else if(rf > bf && bf > gf){
      mv0 = EStates::Chainsaw;
    } else if(bf > gf && bf > rf){
      mv0 = EStates::Whale;
    } else if(rf > gf && gf > bf){
      mv0 = EStates::Fire;
    } else{
      mv0 = EStates::None;
    }
  } else{
    mv0 = EStates::None;
  }

  // Moving average
  mv4 = mv3;
  mv3 = mv2;
  mv2 = mv1;
  mv1 = mv0;

  bool equal = (mv1 == mv0) && (mv2 == mv1) && (mv3 == mv2) && (mv4 == mv3);

  // Adjust audio intensity with magnetic sensor
  const float magneticMax = 1.0;
  const float magneticMin = 0.3;
  __disable_irq();
  intensity = min(magneticValueAbs, magneticMax);
  if (intensity <= magneticMin){
    intensity = 0.0;
  }

  // Check if state needs to be updated
  if(equal && mv0 != lastState){
    lastState = mv0;
    state = mv0;
  }
  __enable_irq();
}
