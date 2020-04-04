#pragma once
#include <vector>
#include <chrono>
#include <ctime>

using namespace std;

namespace led
{
    class LEDStateMachine;
    class LEDState;

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

    const uint8_t NUMBER_OF_OUTPUTS = 3;

    const double PWM_FREQ = 5000;
    const uint8_t PWM_RES = 16;

    const uint8_t inputPinLookup[NUMBER_OF_OUTPUTS] = {
        INPUT_1_PIN,
        INPUT_2_PIN,
        INPUT_3_PIN
    };

    const uint8_t ledPinLookup[NUMBER_OF_OUTPUTS] = {
        LED_1_PIN,
        LED_2_PIN,
        LED_3_PIN
    };

    const uint8_t pwmChannelLookup[NUMBER_OF_OUTPUTS] = {
        LED_1_PWM_CHAN,
        LED_2_PWM_CHAN,
        LED_3_PWM_CHAN
    };

    struct LEDOutput {
        int brightness;
        int priority;
        LEDStateMachine *stateMachine;
    };

	class LEDState
	{
	private:
		vector<int> m_output;           // [1, 2, 3]
		int m_time;                     // time in milliseconds
		int m_brightness;               // brightness 0-100
	public:
		LEDState(vector<int> output, int time, int brightness);
		int getTime();
        int getBrightness();
        vector<int>& getOutput();
	};

	class LEDStateMachine
	{
	private:
		vector<LEDState> m_states;
		int m_current_state_index = 0;
		long long m_state_start_time_ms = 0;
        bool disabled = true;
        int m_priority;                 // priority 0-20
		
	public:
		LEDStateMachine(vector<LEDState> states, int priority);
		void setStates(vector<LEDState> states);
        vector<LEDState>& getStates();
		LEDState getCurrentState();
		LEDState getNextState();
        int getPriority();
        bool isDisabled();
		void check();
		int getIndexOfNextState();
		void disable();
	};

    class LEDHelper
    {
    public:
        //static std::map<int, int> determineOutput(std::map<int, LEDStateMachine> configMap, std::map<int, bool> inputs);
    };
}