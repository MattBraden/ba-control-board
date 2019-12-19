#pragma once
#include <vector>
#include <chrono>
#include <ctime>
#include "Arduino.h"

using namespace std;

namespace led
{
    // Immutable variables
    const uint8_t INPUT_1_PIN = 34;
    const uint8_t INPUT_2_PIN = 35;
    const uint8_t INPUT_3_PIN = 32;

    const uint8_t LED_1_PIN = 23;
    const uint8_t LED_2_PIN = 22;
    const uint8_t LED_3_PIN = 18;

    const uint8_t LED_1_PWM_CHAN = 0;
    const uint8_t LED_2_PWM_CHAN = 1;
    const uint8_t LED_3_PWM_CHAN = 2;

    const double PWM_FREQ = 5000;
    const uint8_t PWM_RES = 16;

    const uint8_t inputPinLookup[4] = {
        0,
        INPUT_1_PIN,
        INPUT_2_PIN,
        INPUT_3_PIN
    };

    const uint8_t ledPinLookup[4] = {
        0,
        LED_1_PIN,
        LED_2_PIN,
        LED_3_PIN
    };

    const uint8_t pwmChannelLookup[4] = {
        42, // Meaning of life
        LED_1_PWM_CHAN,
        LED_2_PWM_CHAN,
        LED_3_PWM_CHAN
    };


	class LEDState
	{
	private:
		vector<int> m_output;    // [1, 2, 3]
		int m_time;                   // time in milliseconds
		int m_brightness;             // brightness 0-100
	public:
		LEDState(vector<int> output, int time, int brightness);
		int getTime();
        int getBrightness();
        vector<int>& getOutput();
		void enable();
		void disable();
	};

	class LEDStateMachine
	{
	private:
		vector<LEDState> m_states;
		int m_current_state_index = 0;
		long long m_state_start_time_ms = 0;
        bool disabled = true;
		
	public:
		LEDStateMachine(vector<LEDState> states);
		void setStates(vector<LEDState> states);
        vector<LEDState>& getStates();
		LEDState getCurrentState();
		LEDState getNextState();
        bool isDisabled();
		void check();
		int getIndexOfNextState();
		void disable();
	};
}