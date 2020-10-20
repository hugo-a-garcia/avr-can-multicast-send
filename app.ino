#include <mcp_can_dfs.h>
#include <mcp_can.h>

#include "src/libcanard/canard.h"
#include "src/libcanard/canard_dsdl.h"

CanardInstance canard_instance;
static const uint16_t HeartbeatSubjectID = 32085;

// Seeed-Studio/CAN_BUS_Shield
const int SPI_CS_PIN = 53;
MCP_CAN CAN(SPI_CS_PIN);

static void *canardAllocate(CanardInstance *const ins, const size_t amount)
{
    (void)ins;
    return malloc(amount);
}

static void canardFree(CanardInstance *const ins, void *const pointer)
{
    (void)ins;
    free(pointer);
}

void setup()
{
    Serial.begin(115200);

    // Initialize the node with a static node-ID as specified in the command-line arguments.
    canard_instance = canardInit(&canardAllocate, &canardFree);
    canard_instance.mtu_bytes = CANARD_MTU_CAN_CLASSIC; // Do not use CAN FD to enhance compatibility.
    canard_instance.node_id = (CanardNodeID)42;

    // Initialize the CAN bus module
    while (CAN_OK != CAN.begin(CAN_500KBPS))
    {
        Serial.println("CAN BUS Shield init fail");
        Serial.println(" Init CAN BUS Shield again");
        delay(100);
    }
    Serial.println("CAN BUS Shield init ok!");
}

void loop()
{
    publishHeartbeat(0);

    // Transmit pending frames.
    const CanardFrame *txf = canardTxPeek(&canard_instance);
    if (txf == NULL) {
        Serial.println("IT IS ALAWAYS NULL");
    }

    // It never enters the while although 
    while (txf != NULL)
    {
        canTransmit(txf);
        canardTxPop(&canard_instance);
        free((void *)txf);
        txf = canardTxPeek(&canard_instance);
    }
    //canTransmit(txf); //Uncomment to test something is being sent
    Serial.flush();
    delay(1000);
}

static void publishHeartbeat(const uint32_t uptime)
{
    static CanardTransferID transfer_id;
    const uint8_t payload[7] = {
        (uint8_t)(uptime >> 0U),
        (uint8_t)(uptime >> 8U),
        (uint8_t)(uptime >> 16U),
        (uint8_t)(uptime >> 24U),
        0,
        0,
        0,
    };
    const CanardTransfer transfer = {
        timestamp_usec : 0,
        priority : CanardPriorityNominal,
        transfer_kind : CanardTransferKindMessage,
        port_id : HeartbeatSubjectID,
        remote_node_id : CANARD_NODE_ID_UNSET,
        transfer_id : transfer_id,
        payload_size : sizeof(payload),
        payload : &payload[0]
    };
    ++transfer_id;
    int out = canardTxPush(&canard_instance, &transfer);
    // out = -2 always???
}

int canTransmit(const CanardFrame *frame)
{

    // CAN.sendMsgBuf(frame->extended_can_id, 1, frame->payload_size, frame->payload);

    // Just a test frame to confirm something is being sent.
    unsigned char stmp[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    // send data:  id = 0x00, standrad frame, data len = 8, stmp: data buf
    stmp[7] = stmp[7] + 1;
    if (stmp[7] == 100)
    {
        stmp[7] = 0;
        stmp[6] = stmp[6] + 1;

        if (stmp[6] == 100)
        {
            stmp[6] = 0;
            stmp[5] = stmp[6] + 1;
        }
    }

    CAN.sendMsgBuf(0x00, 0, 8, stmp);
    return 1;
}