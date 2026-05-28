enum GateState
{
    GATE_WAITING_CARD,
    GATE_PROCESSING,
    GATE_PREPARE_SUCCESS,
    GATE_PREPARE_FAILURE,
    GATE_SUCCESS,
    GATE_FAILURE
};

extern GateState gateState;

void handleGateLogic();
