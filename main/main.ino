#include <algorithm>
#include <array>
#include <queue>
#include <vector>

#include <Servo.h>

#include "src/DYPlayerArduino.h"
#define DECODE_NEC // Decodes NEC, NEC2, and ONKYO
#define DECODE_DISTANCE_WIDTH
#include "src/IRremote.hpp"

#ifdef NDEBUG
#define DEBUG_OUTPUT(...) Serial.print(__VA_ARGS__)
#else
#define DEBUG_OUTPUT(...)
#endif

#ifdef abs
#undef abs
#endif

template <typename T> T clamp(T v, T low, T high) {
  if (v < low) {
    return low;
  } else if (v > high) {
    return high;
  } else {
    return v;
  }
}

void resetIdleCount();

// Pin assignments
constexpr uint8_t pinIrRemote = 3;
constexpr uint8_t pinPushButton = 15;
constexpr uint8_t pinEyeTilter = 2;
constexpr uint8_t pinEyeLedL = 5;
constexpr uint8_t pinEyeLedR = 4;
constexpr uint8_t pinHead = 6;
constexpr uint8_t pinArmL = 7;
constexpr uint8_t pinArmR = 8;
constexpr uint8_t pinMotorLs = 14;
constexpr uint8_t pinMotorL1 = 12;
constexpr uint8_t pinMotorL2 = 13;
constexpr uint8_t pinMotorRs = 9;
constexpr uint8_t pinMotorR1 = 11;
constexpr uint8_t pinMotorR2 = 10;
constexpr uint8_t pinTrialMode = 26;
constexpr unsigned int millisIdle = 10000;
constexpr unsigned int millisBreathing = 3600;

// Servo limits, in terms of pulse width in microseconds
constexpr int minArmL = 2200;
constexpr int maxArmL = 450;
constexpr int minArmR = 700;
constexpr int maxArmR = 2450;
constexpr int minHead = 550;
constexpr int maxHead = 1950;

enum AnimationType {
  // Change the value linearly to a target value, over a specified duration, in
  // milliseconds
  LinearToOver,
  // Change the value linearly to a target value, at a specified speed, in
  // fraction per milliseconds
  LinearToAt,
  // Change the value linearly by an increment, over a specified duration
  LinearByOver,
  // Change the value linearly by an increment, at a specified speed
  LinearByAt,
  // Maintain the value for a specified duration
  Constant
};

struct Animation {
  AnimationType type;
  union {
    unsigned long millisDuration; // milliseconds
    float speed;                  // fraction per millisecond
  };
  float value; // target or incremental

  // to: absolute target value
  // by: incremental value
  // hold: maintain current value
  // over: duration in milliseconds
  // at: speed in fraction per millisecond
  // now: instantaneously

  static Animation toOver(float value, unsigned long duration) {
    return Animation{AnimationType::LinearToOver, {duration}, value};
  }

  static Animation toAt(float value, float speed) {
    return Animation{AnimationType::LinearToAt, {.speed = speed}, value};
  }

  static Animation byOver(float value, unsigned long duration) {
    return Animation{AnimationType::LinearByOver, {duration}, value};
  }

  static Animation byAt(float value, float speed) {
    return Animation{AnimationType::LinearByAt, {.speed = speed}, value};
  }

  static Animation toNow(float value) {
    return Animation{AnimationType::LinearToOver, {0L}, value};
  }

  static Animation byNow(float value) {
    return Animation{AnimationType::LinearByOver, {0L}, value};
  }

  static Animation holdOver(unsigned long duration) {
    return Animation{AnimationType::Constant, {duration}};
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

protected:
  static std::vector<Animatable *> animatables;

  float value_;
  std::queue<Animation> animationQueue;
  unsigned long millisStart = 0;
  float valueStart;

  Animatable() { animatables.push_back(this); }

  virtual void setValue(float value) { value_ = value; }
  float getValue() { return value_; }

public:
  void queueAnimation(Animation animation) {
    animationQueue.push(animation);
    if (animationQueue.size() == 1) {
      transitionToNextAnimation(millis());
    }
  }

  template <size_t N>
  void queueAnimations(const std::array<Animation, N> animations) {
    for (auto animation : animations) {
      queueAnimation(animation);
    }
  }

  bool isAnimating() { return !animationQueue.empty(); }

protected:
  void transitionToNextAnimation(unsigned long millisNow) {
    if (animationQueue.empty()) {
      millisStart = 0;
      return;
    }
    millisStart = millisNow;
    valueStart = value_;

    Animation &animation = animationQueue.front();
    if (animation.type == AnimationType::LinearByOver ||
        animation.type == AnimationType::LinearByAt) {
      // Calculate the target value based on the current value and the
      // increment.

      constexpr float epsilon = 0.05;
      float valueCurrent = getValue();

      float valueTarget = clamp(valueCurrent + animation.value, 0.0f, 1.0f);
      if (std::abs(valueCurrent - valueTarget) <= epsilon) {
        animationQueue.pop();
        transitionToNextAnimation(millisNow);
        return;
      }
      animation.value = valueTarget;
    }

    if (animation.type == AnimationType::LinearToAt ||
        animation.type == AnimationType::LinearByAt) {
      // Calculate the duration based on the speed and the increment.
      float valueCurrent = getValue();

      // animation.value is already target value, rather than the increment.
      unsigned long duration =
          std::abs(animation.value - valueCurrent) / animation.speed;

      animation.millisDuration = duration;
    }
  }

  void updateFrame(unsigned long millisNow) {
    if (animationQueue.empty()) {
      return;
    }

    resetIdleCount();

    Animation animation = animationQueue.front();
    switch (animation.type) {
    case AnimationType::LinearToOver:
    case AnimationType::LinearToAt:
    case AnimationType::LinearByOver:
    case AnimationType::LinearByAt: {
      // Target value and duration should've already been calculated.
      float fraction = static_cast<float>(millisNow - millisStart) /
                       static_cast<float>(animation.millisDuration);
      if (fraction >= 1.0) {
        setValue(animation.value);
        animationQueue.pop();
        transitionToNextAnimation(millisNow);
      } else {
        float valueNext =
            (1.0 - fraction) * valueStart + fraction * animation.value;
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
    // Specific to RP2040, since the Servo library uses PIO, attach() needs to
    // be called after setup() has begun. so we can't do it in the constructor.
    // For any other MCU, we can most likely move this to the constructor and
    // get rid of pin_
    if (!servo_.attached()) {
      if (max_ > min_) {
        servo_.attach(pin_, min_, max_);
      } else {
        servo_.attach(pin_, max_, min_);
      }
    }
    servo_.writeMicroseconds(us);
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
  static constexpr float minValue = 0.20;

private:
  uint8_t pinSpeed_;
  uint8_t pin1_;
  uint8_t pin2_;

public:
  BiMotor(uint8_t pinSpeed, uint8_t pin1, uint8_t pin2)
      : pinSpeed_(pinSpeed), pin1_(pin1), pin2_(pin2), Animatable() {
    pinMode(pinSpeed, OUTPUT);
    pinMode(pin1, OUTPUT);
    pinMode(pin2, OUTPUT);
  }

  void setValue(float value) override {
    if (std::abs(value) <= minValue) {
      value = 0;
    }

    Animatable::setValue(value);

    if (value > 0) {
      digitalWrite(pin1_, 1);
      digitalWrite(pin2_, 0);
    } else if (value < 0) {
      digitalWrite(pin1_, 0);
      digitalWrite(pin2_, 1);
    } else {
      digitalWrite(pin1_, 0);
      digitalWrite(pin2_, 0);
    }
    analogWrite(pinSpeed_, static_cast<int>(std::abs(value) * 255.0));
  }
};

class PushButton {
  static constexpr int debounceThreshold = 100;

public:
  PushButton(int pin) : pin_(pin) {}

  void begin() {
    pinMode(pin_, INPUT_PULLUP);
    last_ = millis();
  }

  bool isPushed() {
    unsigned long now = millis();
    if (now - last_ < debounceThreshold) {
      return false;
    }
    last_ = now;
    int bitmap = digitalRead(pin_);
    // No hold and repeat. The button is only considered pushed
    // if it was released in the previous scan.
    if (bitmap == LOW && released_) {
      released_ = false;
      return true;
    }
    if (bitmap == HIGH) {
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
BiMotor leftTread(pinMotorLs, pinMotorL1, pinMotorL2);
BiMotor rightTread(pinMotorRs, pinMotorR1, pinMotorR2);
Led leftEye(pinEyeLedL);
Led rightEye(pinEyeLedR);
PushButton pushButton(pinPushButton);

DY::Player audioPlayer(&Serial1);
AudioQueue AudioQueue::audioQueue(audioPlayer);

unsigned long millisIdleStart;
float breathingPhase = 1; // a value in the range of [0-1]
boolean trialMode = false;

void resetIdleCount() {
  if (isIdling()) {
    breathingPhase = 1;
  }

  millisIdleStart = millis();
}

void setIdle() { millisIdleStart = 0; }

bool isIdling() { return millis() - millisIdleStart >= millisIdle; }

void demo() {
  // Hold, look left, right, then straight
  std::array<Animation, 5> headAnimation = {
      Animation::toAt(0.5, 0.0005), Animation::holdOver(500),
      Animation::toOver(0, 1000), Animation::toOver(1, 2000),
      Animation::toOver(0.5, 1000)};
  head.queueAnimations(headAnimation);

  // Hold, then blink twice
  std::array<Animation, 9> eyeAnimation = {
      Animation::toNow(1),      Animation::holdOver(4500),
      Animation::toOver(0, 20), Animation::holdOver(100),
      Animation::toOver(1, 20), Animation::holdOver(300),
      Animation::toOver(0, 20), Animation::holdOver(100),
      Animation::toOver(1, 20)};
  leftEye.queueAnimations(eyeAnimation);
  rightEye.queueAnimations(eyeAnimation);

  // Wait, then play wall-e sound
  AudioQueue::queueDelay(5000);
  AudioQueue::queuePlay(1);
}

void playNextAudio() {
  constexpr int audioFileCount = 10;
  static int index = 2;
  index++;
  if (index > audioFileCount) {
    index = 1;
  }
  AudioQueue::queueDelay(100);
  AudioQueue::queuePlay(index);
}

void stop() {
  std::array<Animation, 1> animations = {Animation::toOver(0, 400)};
  leftTread.queueAnimations(animations);
  rightTread.queueAnimations(animations);
}

void spinLeft() {
  leftTread.queueAnimation(Animation::toOver(-1, 200));
  rightTread.queueAnimation(Animation::toOver(1, 200));
}

void spinRight() {
  leftTread.queueAnimation(Animation::toOver(1, 200));
  rightTread.queueAnimation(Animation::toOver(-1, 200));
}

void forward() {
  leftTread.queueAnimation(Animation::toOver(1, 200));
  rightTread.queueAnimation(Animation::toOver(1, 200));
}

void backward() {
  leftTread.queueAnimation(Animation::toOver(-1, 200));
  rightTread.queueAnimation(Animation::toOver(-1, 200));
}

void forwardLeft() {
  leftTread.queueAnimation(Animation::toOver(0, 200));
  rightTread.queueAnimation(Animation::toOver(1, 200));
}

void forwardRight() {
  leftTread.queueAnimation(Animation::toOver(1, 200));
  rightTread.queueAnimation(Animation::toOver(0, 200));
}

void backwardLeft() {
  leftTread.queueAnimation(Animation::toOver(0, 200));
  rightTread.queueAnimation(Animation::toOver(-1, 200));
}

void backwardRight() {
  leftTread.queueAnimation(Animation::toOver(-1, 200));
  rightTread.queueAnimation(Animation::toOver(0, 200));
}

void leftArmUp() { leftArm.queueAnimation(Animation::toAt(1, 0.001f)); }

void leftArmDown() { leftArm.queueAnimation(Animation::toAt(0, 0.001f)); }

void rightArmUp() { rightArm.queueAnimation(Animation::toAt(1, 0.001f)); }

void rightArmDown() { rightArm.queueAnimation(Animation::toAt(0, 0.001f)); }

void leftArmMove(float value) {
  leftArm.queueAnimation(Animation::byAt(value, 0.001f));
}

void rightArmMove(float value) {
  rightArm.queueAnimation(Animation::byAt(value, 0.001f));
}

void breathe() {
  std::array<Animation, 2> animations = {Animation::toOver(0, 2000),
                                         Animation::toOver(1, 2000)};
  leftEye.queueAnimations(animations);
  rightEye.queueAnimations(animations);
}

void wink() {
  std::array<Animation, 3> animations = {Animation::toOver(0, 60),
                                         Animation::holdOver(300),
                                         Animation::toOver(1, 60)};
  rightEye.queueAnimations(animations);
}

void blinkTwice() {
  std::array<Animation, 7> animations = {
      Animation::toOver(0, 20), Animation::holdOver(100),
      Animation::toOver(1, 20), Animation::holdOver(300),
      Animation::toOver(0, 20), Animation::holdOver(100),
      Animation::toOver(1, 20)};
  leftEye.queueAnimations(animations);
  rightEye.queueAnimations(animations);
}

void lookLeft() { head.queueAnimation(Animation::toAt(0, 0.0005)); }

void lookRight() { head.queueAnimation(Animation::toAt(1, 0.0005)); }

void lookStraight() { head.queueAnimation(Animation::toAt(0.5, 0.0005)); }

void rotateHeadBy(float value) {
  head.queueAnimation(Animation::byAt(value, 0.0005));
}

void tiltEye() {
  std::array<Animation, 3> animations = {Animation::toOver(1, 0),
                                         Animation::toOver(1, 1600),
                                         Animation::toOver(0, 0)};
  eyeTilter.queueAnimations(animations);
}

void toggleLeftArm() {
  static uint8_t position = 1;
  if (position == 1) {
    leftArm.queueAnimation(Animation::toOver(0, 1000));
    position = 0;
  } else {
    leftArm.queueAnimation(Animation::toOver(1, 1000));
    position = 1;
  }
}

void toggleRightArm() {
  static uint8_t position = 1;
  if (position == 1) {
    rightArm.queueAnimation(Animation::toOver(0, 1000));
    position = 0;
  } else {
    rightArm.queueAnimation(Animation::toOver(1, 1000));
    position = 1;
  }
}

void lookAround() {
  // Turn left, right, then straight
  std::array<Animation, 3> animations1 = {
      Animation::toOver(0, 700),
      Animation::toOver(1, 1400),
      Animation::toOver(0.5, 700),
  };
  // Open eyes, hold while turning head, then blink twice
  std::array<Animation, 9> animations2 = {
      Animation::toOver(1, 0),  Animation::holdOver(3000),
      Animation::toOver(0, 20), Animation::holdOver(100),
      Animation::toOver(1, 20), Animation::holdOver(300),
      Animation::toOver(0, 20), Animation::holdOver(100),
      Animation::toOver(1, 20)};

  head.queueAnimations(animations1);
  leftEye.queueAnimations(animations2);
  rightEye.queueAnimations(animations2);
}

void setup() {
#ifdef NDEBUG
  Serial.begin(115200);
#endif

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

  // Set up audio player and play "wall-e"
  // The dyplayer needs a little time to be ready after powering up.
  // A 300ms delay seems to be pretty reliable.
  delay(300);
  audioPlayer.begin();
  delay(50);
  audioPlayer.setVolume(15);
  delay(50);

  pinMode(pinTrialMode, INPUT_PULLUP);
  trialMode = digitalRead(pinTrialMode);

  if (trialMode) {
    demo();
  } else {
    AudioQueue::queuePlay(1);
  }

  resetIdleCount();
}

void loop() {
  AudioQueue::updateFrame();

  if (pushButton.isPushed()) {
    resetIdleCount();
    demo();
    return;
  }

  if (IrReceiver.decode()) {
    resetIdleCount();

    uint64_t code = IrReceiver.decodedIRData.decodedRawData;
    decode_type_t protocol = IrReceiver.decodedIRData.protocol;

    IrReceiver.resume();
    // The Roku remote control
    if (protocol == NEC) {
      switch (code) {
      case 3893872618: // Power
        demo();
        break;
      case 2573649898: // Back
        lookAround();
        break;
      case 4228106218: // Home
        lookStraight();
        break;

      case 3860449258: // Up
        forward();
        break;
      case 3425945578: // Down
        backward();
        break;
      case 3776890858: // Left
        spinLeft();
        break;
      case 3526215658: // Right
        spinRight();
        break;
      case 3576350698: // OK
        stop();
        break;

      case 2272839658: // Repeat
        blinkTwice();
        break;
      case 2640496618: // Sleep
        tiltEye();
        break;
      case 2657208298: // Star
        breathe();
        break;

      case 3409233898: // Rewind
        lookLeft();
        break;
      case 3008153578: // Play/Pause
        playNextAudio();
        break;
      case 2857748458: // F. Fwd
        lookRight();
        break;

      case 2907883498: // Netflix
        forwardLeft();
        break;
      case 2991441898: // Hulu
        backwardLeft();
        break;
      case 3024865258: // AmazonPrime
        forwardRight();
        break;
      case 4077701098: // Disney
        backwardRight();
        break;

      case 4144547818: // Vudu
        leftArmUp();
        break;
      case 2974730218: // HBO
        leftArmDown();
        break;
      case 2841036778: // YouTube
        rightArmUp();
        break;
      case 4177971178: // Sling
        rightArmDown();
        break;

      case 4027566058: // Vol Up
        AudioQueue::queuePlay(2);
        break;
      case 4010854378: // Vol Down
        AudioQueue::queuePlay(3);
        break;
      case 3743467498: // Mute
        AudioQueue::queuePlay(1);
        break;
      }
    }

    // Custom remote control
    if (protocol == PULSE_DISTANCE) {
      byte *p = (byte *)(&code);
      byte correction = 0;
      for (int i = 0; i < 6; i++) {
        correction ^= p[i];
      }
      if (correction == 0) {
        static uint64_t prevCode = 0x000000008888; // The neutral state

        int8_t x1, y1;
        int8_t x2, y2;
        decodeJoystick(p[0], x1, y1);
        decodeJoystick(p[1], x2, y2);

        // if any of the button bits (16 - 40) has changed
        if ((code ^ prevCode) & 0xFFFFFF0000) {
          for (int i = 16; i < 40; i++) {
            bool curr = (code >> i) & 1;
            bool prev = (prevCode >> i) & 1;

            if (curr && !prev) {
              // Button pressed
              switch (i - 16) {
              case 0: // R1
                rightArmMove(0.1);
                break;
              case 1: // R2
                rightArmMove(-0.1);
                break;
              case 2: // L1
                leftArmMove(0.1);
                break;
              case 3: // L2
                leftArmMove(-0.1);
                break;
              case 4: // SEL
                wink();
                break;
              case 5: // MODE
                demo();
                break;
              case 6: // N/A
                break;
              case 7: // START
                playNextAudio();
                break;
              case 8: // A
                backward();
                break;
              case 9: // X
                spinLeft();
                break;
              case 10: // B
                spinRight();
                break;
              case 11: // Y
                forward();
                break;
              case 12: // Down
                tiltEye();
                break;
              case 13: // Up
                lookStraight();
                break;
              case 14: // Right
                rotateHeadBy(0.1);
                break;
              case 15: // Left
                rotateHeadBy(-0.1);
                break;
              case 16: // Joystick1
                stop();
                break;
              case 17: // Joystick2
                lookAround();
                break;
              default:
                break;
              }
            }
          }
        }

        // Joystick 1 position changed
        if ((code ^ prevCode) & 0x00000000FF) {
          // Normalize x and y values. Range: [-1,1]
          float xNormalized = x1 / 7.0;
          float yNormalized = y1 / 7.0;

          float xSign = xNormalized >= 0 ? 1 : -1;
          float ySign = yNormalized >= 0 ? 1 : -1;

          float left, right;
          if (xSign == ySign) {
            left = ySign * max(abs(xNormalized), abs(yNormalized));
            right = ySign * (abs(yNormalized) - abs(xNormalized));
          } else {
            right = ySign * max(abs(xNormalized), abs(yNormalized));
            left = ySign * (abs(yNormalized) - abs(xNormalized));
          }
          leftTread.setValue(left);
          rightTread.setValue(right);
        }

        // Joystick 2 is not in the neutral position
        if (y2 != 0) {
          static unsigned long last = millis();

          // Range with which a normalized x value is considered
          // "centered"
          const float thresholdLeft = -0.5;
          const float thresholdRight = 0.5;

          // Normalize x and y values. Range: [-1,1]
          float xNormalized = x2 / 7.0;
          float yNormalized = y2 / 7.0;

          bool moveLeft = xNormalized <= thresholdRight;
          bool moveRight = xNormalized >= thresholdLeft;

          // Calculate the speed to move: fast, slow, or still
          float ySign = yNormalized >= 0 ? 1 : -1;
          float yAbsolute = abs(yNormalized);
          float speed;
          if (yAbsolute > 0.85) {
            speed = 0.0003;
          } else if (yAbsolute > 0.25) {
            speed = 0.00015;
          } else {
            speed = 0;
          }

          // Only queue an animation if the queue is empty. Otherwise
          // it'd be animating long after the joystick is returned to
          // the neutral position, which is not what we want.
          if (moveLeft && speed != 0 && !leftArm.isAnimating()) {
            leftArm.queueAnimation(Animation::byAt(0.1 * ySign, speed));
          }

          if (moveRight && speed != 0 && !rightArm.isAnimating()) {
            rightArm.queueAnimation(Animation::byAt(0.1 * ySign, speed));
          }
        }

        prevCode = code;
      } else {
        DEBUG_OUTPUT("Error detected\n");
      }
    }
  }

  if (isIdling()) {
    leftTread.setValue(0);
    rightTread.setValue(0);

    unsigned long idleDuration = (millis() - millisIdleStart) % millisBreathing;

    if (idleDuration < millisBreathing / 2.0) {
      breathingPhase = 1.0 - idleDuration / (millisBreathing / 2.0);
    } else {
      breathingPhase = -1.0 + idleDuration / (millisBreathing / 2.0);
    }

    leftEye.setValue(breathingPhase);
    rightEye.setValue(breathingPhase);
  }
}

void decodeJoystick(byte code, int8_t &x, int8_t &y) {
  if (code & 0x08) {
    x = -(code & 0x07);
  } else {
    x = code & 0x07;
  }

  if (code & 0x80) {
    y = -((code & 0x70) >> 4);
  } else {
    y = (code & 0x70) >> 4;
  }
}