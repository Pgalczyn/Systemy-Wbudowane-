
enum GateState
{
    GATE_WAITING_CARD,
    GATE_PROCESSING,
    GATE_PREPARE_SUCCESS,
    GATE_PREPARE_FAILURE,
    GATE_SUCCESS,
    GATE_FAILURE
};

enum ReceptionState
{
    RX_SHOW_MENU,
    RX_WAIT_FOR_CHOICE,
    RX_WAIT_FOR_NAME,
    RX_WAIT_FOR_SURNAME,
    RX_WAIT_FOR_EMAIL,
    RX_WAIT_FOR_POINTS,
    RX_WAIT_FOR_STATE,
    RX_WAITING_FOR_CARD_MSG, // Show message once
    RX_WAIT_FOR_CARD         // Wait for card or cancel
};

extern ReceptionState currentStep;

void handleGateLogic();

void handleReceptionNonBlocking();