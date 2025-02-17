#include <algorithm>
#include <array>
#include <queue>
#include <vector>

#include <Servo.h>

#include "src/DYPlayerArduino.h"
#include "src/IRremote.hpp"

void resetIdleCount();

// Pin assignments
constexpr uint8_t pinIrRemote = 3;
constexpr uint8_t pinPushButton = 9;
constexpr uint8_t pinEyeTilter = 2;
constexpr uint8_t pinEyeLedL = 4;
constexpr uint8_t pinEyeLedR = 5;
constexpr uint8_t pinHead = 6;
constexpr uint8_t pinArmL = 7;
constexpr uint8_t pinArmR = 8;
constexpr uint8_t pinMotorL1 = 10;
constexpr uint8_t pinMotorL2 = 11;
constexpr uint8_t pinMotorR1 = 12;
constexpr uint8_t pinMotorR2 = 13;

// Servo limits
constexpr int minArmL = 2200;
constexpr int maxArmL = 450;
constexpr int minArmR = 700;
constexpr int maxArmR = 2450;
constexpr int minHead = 600;
constexpr int maxHead = 2000;

enum AnimationType {
  // Change the value linearly to an end value, over a specified duration
  Linear,
  // Maintain the value for a specified duration
  Constant
};

struct Animation {
  AnimationType type;
  unsigned long millisDuration;
  float valueEnd;

  static Animation linear(float value, unsigned long duration) {
    return Animation{.type = AnimationType::Linear,
                     .millisDuration = duration,
                     .valueEnd = value};
  }

  static Animation instant(float value) {
    return Animation{
        .type = AnimationType::Linear, .millisDuration = 0, .valueEnd = value};
  }

  static Animation constant(unsigned long duration) {
    return Animation{.type = AnimationType::Constant,
                     .millisDuration = duration};
  }
};

class Animatable {
public:
  static void updateFrame() {
    unsigned long millisNow = millis();
    for (int i = 0; i < animatables.size(); i++) {
      animatables[i]->updateFrame(millisNow);
    }
  }

  virtual void setValue(float value) { value_ = value; }
  float getValue() { return value_; }

  void queueAnimation(Animation animation) {
    if (animationQueue.empty()) {
      millisStart = millis();
      valueStart = value_;
    }
    animationQueue.push(animation);
  }

  template <size_t N>
  void queueAnimations(const std::array<Animation, N> animations) {
    for (auto animation : animations) {
      queueAnimation(animation);
    }
  }

protected:
  static std::vector<Animatable *> animatables;

  float value_;
  std::queue<Animation> animationQueue;
  unsigned long millisStart = 0;
  float valueStart;

  Animatable() { animatables.push_back(this); }

  void transitionToNextAnimation(unsigned long millisNow) {
    if (animationQueue.empty()) {
      millisStart = 0;
      return;
    }
    millisStart = millisNow;
    valueStart = value_;
  }

  void updateFrame(unsigned long millisNow) {
    if (animationQueue.empty()) {
      return;
    }

    resetIdleCount();

    Animation animation = animationQueue.front();
    switch (animation.type) {
    case AnimationType::Linear: {
      float fraction = static_cast<float>(millisNow - millisStart) /
                       static_cast<float>(animation.millisDuration);
      if (fraction >= 1.0) {
        setValue(animation.valueEnd);
        animationQueue.pop();
        transitionToNextAnimation(millisNow);
      } else {
        float valueNext =
            (1.0 - fraction) * valueStart + fraction * animation.valueEnd;
        setValue(valueNext);
      }
      break;
    }

    case AnimationType::Constant: {
      if (millisNow - millisStart >= animation.millisDuration) {
        animationQueue.pop();
        transitionToNextAnimation(millisNow);
      }
      break;
    }
    }
  }
};

std::vector<Animatable *> Animatable::animatables;

class AnimatableServo : public Animatable {
public:
  AnimatableServo(uint8_t pin, uint16_t min, uint16_t max)
      : pin_(pin), min_(min), max_(max), Animatable() {}

  void setValue(float value) override {
    Animatable::setValue(value);
    uint16_t us = static_cast<uint16_t>(value * static_cast<float>(max_) +
                                        (1 - value) * static_cast<float>(min_));
    // attach() needs to be called after setup() has begun. so we can't do it in
    // the constructor.
    if (!servo_.attached()) {
      if (max_ > min_) {
        servo_.attach(pin_, min_, max_);
      } else {
        servo_.attach(pin_, max_, min_);
      }
    }
    servo_.writeMicroseconds(us);
    Serial.print("setting value ");
    Serial.println(us);
  }

private:
  Servo servo_;
  uint8_t pin_;
  uint16_t min_;
  uint16_t max_;
};

class Led : public Animatable {
public:
  Led(uint8_t pin) : pin_(pin), Animatable() { pinMode(pin, OUTPUT); }

  void setValue(float value) override {
    Animatable::setValue(value);
    analogWrite(pin_, static_cast<int>(value * 255.0));
  }

private:
  uint8_t pin_;
};

// Unidirectional motor, controlled by a PWM pin and a MOSFET
class UniMotor : public Animatable {
public:
  UniMotor(uint8_t pin) : pin_(pin), Animatable() { pinMode(pin, OUTPUT); }

  void setValue(float value) override {
    Animatable::setValue(value);
    analogWrite(pin_, static_cast<int>(value * 255.0));
  }

private:
  uint8_t pin_;
};

// Bidirectional motor, controlled by an H-bridge
class BiMotor : public Animatable {
  static constexpr float low_threshold = 0.05;

public:
  BiMotor(uint8_t pin1, uint8_t pin2) : pin1_(pin1), pin2_(pin2), Animatable() {
    pinMode(pin1, OUTPUT);
    pinMode(pin2, OUTPUT);
  }

  void setValue(float value) override {
    Animatable::setValue(value);

    int value1;
    int value2;
    if (abs(value) < low_threshold) {
      value1 = 0;
      value2 = 0;
    } else if (value > 0) {
      value1 = 0;
      value2 = static_cast<int>(value * 255.0);
    } else {
      value1 = static_cast<int>(-value * 255.0);
      value2 = 0;
    }

    analogWrite(pin1_, value1);
    analogWrite(pin2_, value2);
  }

private:
  uint8_t pin1_;
  uint8_t pin2_;
};

class PushButton {
  static constexpr int debounce_threshold = 100;

public:
  PushButton(int pin) : pin_(pin) {}

  void begin() {
    pinMode(pin_, INPUT_PULLUP);
    last_ = millis();
  }

  bool isPushed() {
    unsigned long now = millis();
    if (now - last_ < debounce_threshold) {
      return false;
    }
    last_ = now;
    int state = digitalRead(pin_);
    // No hold and repeat. The button is only considered pushed
    // if it was released in the previous scan.
    if (state == LOW && released_) {
      released_ = false;
      return true;
    }
    if (state == HIGH) {
      released_ = true;
    }
    return false;
  }

private:
  int pin_;
  unsigned long last_ = 0;
  bool released_ = true;
};

class AudioQueue {
public:
  struct Entry {
    enum { Delay, Play } tag;
    unsigned long millisStart = 0;
    union {
      unsigned long duration;
      unsigned int index;
    };
  };

  static AudioQueue audioQueue;
  static void updateFrame() {
    unsigned long millisNow = millis();
    audioQueue.updateFrame(millisNow);
  }

  static void queueDelay(unsigned long duration) {
    Entry entry{
        .tag = Entry::Delay, .millisStart = millis(), duration = duration};
    audioQueue.queue.push(entry);
  }

  static void queuePlay(unsigned int index) {
    Entry entry{.tag = Entry::Play, .millisStart = millis(), index = index};
    audioQueue.queue.push(entry);
  }

  AudioQueue(DY::Player &player) : player_(player) {}

private:
  void updateFrame(unsigned long millisNow) {
    if (queue.empty()) {
      return;
    }

    resetIdleCount();

    Entry entry = queue.front();
    switch (entry.tag) {
    case Entry::Delay:
      if (millisNow > entry.millisStart + entry.duration) {
        queue.pop();
      }
      break;
    case Entry::Play:
      player_.playSpecified(entry.index);
      queue.pop();
      break;
    }
  }

protected:
  static std::queue<Entry> queue;
  DY::Player &player_;
};

std::queue<AudioQueue::Entry> AudioQueue::queue;

AnimatableServo head(pinHead, minHead, maxHead);
AnimatableServo leftArm(pinArmL, minArmL, maxArmL);
AnimatableServo rightArm(pinArmR, minArmR, maxArmR);
UniMotor eyeTilter(pinEyeTilter);
BiMotor leftTread(pinMotorL1, pinMotorL2);
BiMotor rightTread(pinMotorR1, pinMotorR2);
Led leftEye(pinEyeLedL);
Led rightEye(pinEyeLedR);
PushButton pushButton(pinPushButton);

DY::Player audioPlayer(&Serial1);
AudioQueue AudioQueue::audioQueue(audioPlayer);

static unsigned long millisIdleStart;
float breathingValue = 1;

void setIdleBreathing() {
  leftEye.setValue(breathingValue);
  rightEye.setValue(breathingValue);
}
void resetIdleCount() {
  if (isIdling()) {
    breathingValue = 1;
  }

  millisIdleStart = millis();
}

bool isIdling() { return millis() - millisIdleStart >= 10000; }

void demo() {
  // Hold for 0.5s, look left, right, then straight
  std::array<Animation, 5> headAnimation = {
      Animation::instant(0.5), Animation::constant(500),
      Animation::linear(0, 500), Animation::linear(1, 1000),
      Animation::linear(0.5, 500)};
  head.queueAnimations(headAnimation);

  // Hold for 2s, blink twice
  std::array<Animation, 9> eyeAnimation = {
      Animation::instant(1),    Animation::constant(2000),
      Animation::linear(0, 20), Animation::constant(100),
      Animation::linear(1, 20), Animation::constant(300),
      Animation::linear(0, 20), Animation::constant(100),
      Animation::linear(1, 20)};
  leftEye.queueAnimations(eyeAnimation);
  rightEye.queueAnimations(eyeAnimation);

  AudioQueue::queueDelay(2000);
  AudioQueue::queuePlay(1);
}

void play_next_audio() {
  constexpr int audio_file_count = 10;
  static int index = 2;
  index++;
  if (index > audio_file_count) {
    index = 1;
  }
  AudioQueue::queueDelay(200);
  AudioQueue::queuePlay(index);
}

void stop() {
  std::array<Animation, 1> animations = {Animation::linear(0, 200)};
  leftTread.queueAnimations(animations);
  rightTread.queueAnimations(animations);
}

void spin_left() {
  leftTread.queueAnimation(Animation::linear(-1, 200));
  rightTread.queueAnimation(Animation::linear(1, 200));
}

void spin_right() {
  leftTread.queueAnimation(Animation::linear(1, 200));
  rightTread.queueAnimation(Animation::linear(-1, 200));
}

void go_forward() {
  leftTread.queueAnimation(Animation::linear(1, 200));
  rightTread.queueAnimation(Animation::linear(1, 200));
}

void go_backwards() {
  leftTread.queueAnimation(Animation::linear(-1, 200));
  rightTread.queueAnimation(Animation::linear(-1, 200));
}

void forward_left() {
  leftTread.queueAnimation(Animation::linear(0, 200));
  rightTread.queueAnimation(Animation::linear(1, 200));
}

void forward_right() {
  leftTread.queueAnimation(Animation::linear(1, 200));
  rightTread.queueAnimation(Animation::linear(0, 200));
}

void backward_left() {
  leftTread.queueAnimation(Animation::linear(0, 200));
  rightTread.queueAnimation(Animation::linear(-1, 200));
}

void backward_right() {
  leftTread.queueAnimation(Animation::linear(-1, 200));
  rightTread.queueAnimation(Animation::linear(0, 200));
}

void left_arm_up() { leftArm.queueAnimation(Animation::linear(1, 1000)); }

void left_arm_down() { leftArm.queueAnimation(Animation::linear(0, 1000)); }

void right_arm_up() { rightArm.queueAnimation(Animation::linear(1, 1000)); }

void right_arm_down() { rightArm.queueAnimation(Animation::linear(0, 1000)); }

void breathe() {
  std::array<Animation, 2> animations = {Animation::linear(0, 2000),
                                         Animation::linear(1, 2000)};
  leftEye.queueAnimations(animations);
  rightEye.queueAnimations(animations);
}

void blink_twice() {
  std::array<Animation, 7> animations = {
      Animation::linear(0, 20), Animation::constant(100),
      Animation::linear(1, 20), Animation::constant(300),
      Animation::linear(0, 20), Animation::constant(100),
      Animation::linear(1, 20)};
  leftEye.queueAnimations(animations);
  rightEye.queueAnimations(animations);
}

void look_left() { head.queueAnimation(Animation::linear(0, 1000)); }

void look_right() { head.queueAnimation(Animation::linear(1, 1000)); }

void look_straight() { head.queueAnimation(Animation::linear(0.5, 1000)); }

void tilt_eye() {
  std::array<Animation, 3> animations = {Animation::linear(1, 0),
                                         Animation::linear(1, 1600),
                                         Animation::linear(0, 0)};
  eyeTilter.queueAnimations(animations);
}

void toggle_left_arm() {
  static uint8_t position = 1;
  if (position == 1) {
    leftArm.queueAnimation(Animation::linear(0, 1000));
    position = 0;
  } else {
    leftArm.queueAnimation(Animation::linear(1, 1000));
    position = 1;
  }
}

void toggle_right_arm() {
  static uint8_t position = 1;
  if (position == 1) {
    rightArm.queueAnimation(Animation::linear(0, 1000));
    position = 0;
  } else {
    rightArm.queueAnimation(Animation::linear(1, 1000));
    position = 1;
  }
}

void look_around() {
  // Turn left, right, then straight
  std::array<Animation, 3> animations1 = {
      Animation::linear(0, 700),
      Animation::linear(1, 1400),
      Animation::linear(0.5, 700),
  };
  // Open eyes, hold while turning head, then blink twice
  std::array<Animation, 9> animations2 = {
      Animation::linear(1, 0),  Animation::constant(3000),
      Animation::linear(0, 20), Animation::constant(100),
      Animation::linear(1, 20), Animation::constant(300),
      Animation::linear(0, 20), Animation::constant(100),
      Animation::linear(1, 20)};

  head.queueAnimations(animations1);
  leftEye.queueAnimations(animations2);
  rightEye.queueAnimations(animations2);
}

void setup() {
  // Set up serial for debugging
  Serial.begin(115200);

  // Set up audio player
  audioPlayer.begin();
  delay(200);
  audioPlayer.setVolume(15);

  audioPlayer.playSpecified(2);

  // Set up push button
  pushButton.begin();

  // Set up IR receiver
  IrReceiver.begin(pinIrRemote, false);

  // Set initial status
  head.setValue(0.5);
  leftArm.setValue(0);
  rightArm.setValue(0);
  eyeTilter.setValue(0);
  leftTread.setValue(0);
  rightTread.setValue(0);
  leftEye.setValue(1);
  rightEye.setValue(1);

  resetIdleCount();
}

void loop() {
  Animatable::updateFrame();
  AudioQueue::updateFrame();

  if (pushButton.isPushed()) {
    resetIdleCount();
    demo();
    return;
  }

  if (IrReceiver.decode()) {
    resetIdleCount();

    uint32_t code = IrReceiver.decodedIRData.decodedRawData;
    decode_type_t protocol = IrReceiver.decodedIRData.protocol;

    IrReceiver.resume();

    if (protocol == NEC) {
      switch (code) {
      case 3893872618: // Power
        demo();
        break;
      case 2573649898: // Back
        look_around();
        break;
      case 4228106218: // Home
        look_straight();
        break;

      case 3860449258: // Up
        go_forward();
        break;
      case 3425945578: // Down
        go_backwards();
        break;
      case 3776890858: // Left
        spin_left();
        break;
      case 3526215658: // Right
        spin_right();
        break;
      case 3576350698: // OK
        stop();
        break;

      case 2272839658: // Repeat
        blink_twice();
        break;
      case 2640496618: // Sleep
        tilt_eye();
        break;
      case 2657208298: // Star
        breathe();
        break;

      case 3409233898: // Rewind
        look_left();
        break;
      case 3008153578: // Play/Pause
        play_next_audio();
        break;
      case 2857748458: // F. Fwd
        look_right();
        break;

      case 2907883498: // Netflix
        forward_left();
        break;
      case 2991441898: // Hulu
        backward_left();
        break;
      case 3024865258: // AmazonPrime
        forward_right();
        break;
      case 4077701098: // Disney
        backward_right();
        break;

      case 4144547818: // Vudu
        left_arm_up();
        break;
      case 2974730218: // HBO
        left_arm_down();
        break;
      case 2841036778: // YouTube
        right_arm_up();
        break;
      case 4177971178: // Sling
        right_arm_down();
        break;
      }
    }

    if (isIdling()) {
      leftTread.setValue(0);
      rightTread.setValue(0);

      constexpr unsigned long breathingCycle = 3600;
      unsigned long idleDuration =
          (millis() - millisIdleStart) % breathingCycle;

      if (idleDuration < breathingCycle / 2.0) {
        breathingValue = 1.0 - idleDuration / (breathingCycle / 2.0);
      } else {
        breathingValue = -1.0 + idleDuration / (breathingCycle / 2.0);
      }

      setIdleBreathing();
    }
  }
}