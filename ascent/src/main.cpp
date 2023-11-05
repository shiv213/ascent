#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <LittleFS.h>
#include <vector>
#include <iostream>

int LED = D4;

ESP8266WebServer server(80);

int exertion = 0;
int drop = 0;
int fift_prev_avg = 0;
int flag = 0;

int counter = 0;
int time_exerted = 0;
int time_fatigue = 0;
int peaks = 0;
int valleys = 0;

class MovingAverage {
  public:
    MovingAverage(int prevN) : prevN(prevN) , readings(prevN, 0) {}
    void update(int val) {
      if (readings.size() + 1 < prevN) {
        readings.push_back(val);
      } else {
        total -= readings.at(0);
        readings.erase(readings.begin());
        readings.push_back(val);
        total += val;
      }
      avg = total / int(readings.size());
    }
  int currentAvg() {
    return avg;
  }
  private:
    int prevN;
    std::vector<int> readings;
    int total =0;
    int avg = 0;
};

MovingAverage fiveSMA = MovingAverage(5);
MovingAverage tenSMA = MovingAverage(10);
MovingAverage fifteenSMA = MovingAverage(15);

void change_fifteen_avg() {
    fift_prev_avg = fifteenSMA.currentAvg();
}

void detect_fifteen_change() {
    if (fifteenSMA.currentAvg() > fift_prev_avg && exertion == 0) {
        // std::cout << "prolonged exertion" << std::endl;
        exertion = 1;
    } else if (fifteenSMA.currentAvg() > fift_prev_avg && exertion == 1) {
        exertion = 0;
        // std::cout << "prolonged fatigue" << std::endl;
    }
}

void detect_exertion() {
    int five = fiveSMA.currentAvg();
    int ten = tenSMA.currentAvg();
    int fift = fifteenSMA.currentAvg();
    if (five > fift) {
        // std::cout << "Heavy Exertion" << std::endl;
        exertion = 1;
    }
    if (ten > fift) {
        // std::cout << "Exertion" << std::endl;
        exertion = 1;
    }
}

void detect_fatigue() {
    int five = fiveSMA.currentAvg();
    int ten = tenSMA.currentAvg();
    int fift = fifteenSMA.currentAvg();
    if (five < fift) {
        // std::cout << "Heavy Fatigue" << std::endl;
        exertion = 0;
    }
    if (ten < fift) {
        // std::cout << "Fatigue" << std::endl;
        exertion = 0;
    }   
}

int detect_peak(int current_value) {
    if (current_value > fiveSMA.currentAvg()) {
        // std::cout << "crazy peak" << std::endl;
        drop = 1;
        return 1;
    } else if (current_value > tenSMA.currentAvg ()) {
        // std::cout << "slight peak" << std::endl;
    } else {
        // std::cout << "negligible peak" << std::endl;
    }
    return 0;
}

int detect_valley(int current_value) {
    if (current_value < fiveSMA.currentAvg()) {
        // std::cout << "crazy drop" << std::endl;
        drop = 0;
        return 1;
    } else if (current_value < tenSMA.currentAvg ()) {
        // std::cout << "slight drop" << std::endl;
    } else {
        // std::cout << "negligible drop" << std::endl;
    }
    return 0;
}

void setup() {
  Serial.begin(9600);
  pinMode(LED, OUTPUT);
}

void loop() {
  digitalWrite(LED, LOW);
  if (analogRead(A0) > 60) {
    digitalWrite(LED, HIGH);
  }
  int current_value = analogRead(A0);
  fiveSMA.update(current_value);
  tenSMA.update(current_value);
  fifteenSMA.update(current_value);
  if (counter % 15 == 0) {
      detect_fifteen_change();
      change_fifteen_avg();
  }
  
  if (drop == 0) {
      if (detect_peak(current_value)) {
        peaks += 1;
      }
  } else {
      if (detect_valley(current_value)) {
        valleys += 1;
      }
  }
  counter += 1;
  if (exertion == 1) {
    time_exerted +=  1;
  } else {
    time_fatigue += 1;
  }
}