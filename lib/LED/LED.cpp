#include "LED.h"

using namespace std::chrono;
using namespace led;
using namespace std;

// State constructor
LEDState::LEDState(vector<int> output, int time, int brightness) {
	m_output = output;
	m_time = time;
	m_brightness = brightness;
}

int LEDState::getTime() {
	return m_time;
}

int LEDState::getBrightness() {
	return m_brightness;
}

vector<int>& LEDState::getOutput() {
	return m_output;
}

// StateMachine Constructor
LEDStateMachine::LEDStateMachine(vector<LEDState> states, int priority) {
	setStates(states);
	m_priority = priority;
}

void LEDStateMachine::setStates(vector<LEDState> states) {
	m_states = states;
}

vector<LEDState>& LEDStateMachine::getStates() {
	return m_states;
}

int LEDStateMachine::getPriority() {
	return m_priority;
}

LEDState LEDStateMachine::getCurrentState() {
	return m_states.at(m_current_state_index);
}

int LEDStateMachine::getIndexOfNextState() {
	if (m_current_state_index == (m_states.size() - 1)) {
		return 0;
	}
	else {
		return m_current_state_index + 1;
	}
}

bool LEDStateMachine::isDisabled() {
	return disabled;
}

void LEDStateMachine::check() {
	LEDState currentState = getCurrentState();
	disabled = false;

	long long now_ms = duration_cast<milliseconds> (system_clock::now().time_since_epoch()).count();

	// First time
	if (m_state_start_time_ms == 0) {
		m_state_start_time_ms = now_ms;
	}
	if (now_ms - m_state_start_time_ms >= currentState.getTime()) {
		m_current_state_index = getIndexOfNextState();
		m_state_start_time_ms = now_ms;
	}
}

void LEDStateMachine::disable() {
	m_state_start_time_ms = 0;
	m_current_state_index = 0;
	disabled = true;
}

// TRASH: Config should not also keep statemachine states....
// Who coded this...
map<int, LEDOutput> LEDHelper::determineOutput(map<int, LEDStateMachine>& fullConfig, map<int, bool> inputs) {
  // Set all outputs to off
  map<int, LEDOutput> outputs;
  for(int i = 0; i < NUMBER_OF_OUTPUTS; ++i) {
    outputs.insert({i, LEDOutput{0, 0, 0}});
  }

  // Figure out what statemachines are active
  vector<LEDStateMachine*> activeStateMachines;
  for (auto &x: fullConfig) {
    int input = x.first;
    LEDStateMachine &stateMachine = fullConfig.find(input)->second;

    if (inputs[input]) {
      stateMachine.check();
      activeStateMachines.push_back(&stateMachine);
    } else {
      stateMachine.disable();
    }
  }

  // Of the active stateMachine, set outputs based on priority
  // This code is slowly turning into a gigantic mess.
  for (auto stateMachine: activeStateMachines) {
    LEDState state = stateMachine->getCurrentState();
    for (int output : state.getOutput()) {
      if (stateMachine->getPriority() > outputs[output].priority) {
        if (outputs[output].stateMachine != 0) {
          outputs[output].stateMachine->disable();
        }
        outputs[output] = LEDOutput{state.getBrightness(), stateMachine->getPriority(), stateMachine};
      }
    }
  }
  return outputs;
}
