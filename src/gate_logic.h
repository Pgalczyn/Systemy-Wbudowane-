enum GateState
{
    GATE_WAITING_CARD,
    GATE_PROCESSING,
    GATE_SUCCESS,
    GATE_FAILURE,
    GATE_SUCCESS_FADE,
    GATE_FAILURE_FADE
};

extern GateState gateState;

void handleGateLogic();
