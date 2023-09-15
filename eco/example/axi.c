/* Red Pitaya C API example Acquiring a signal from a buffer
 * This application acquires a signal on a specific channel */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "rp.h"

// size in samples 16Bit
#define DATA_SIZE ((1024 * 1024 * 128) / 2)

#define READ_DATA_SIZE (1024 * 256)

int main(int argc, char **argv)
{
    /* Print error, if rp_Init() function failed */
    if (rp_InitReset(false) != RP_OK) {
        fprintf(stderr, "Rp api init failed!\n");
        return -1;
    }

    uint32_t g_adc_axi_start,g_adc_axi_size;
    rp_AcqAxiGetMemoryRegion(&g_adc_axi_start,&g_adc_axi_size);

    printf("Reserved memory start 0x%X size 0x%X\n",g_adc_axi_start,g_adc_axi_size);
//    rp_AcqResetFpga();

    if (rp_AcqAxiSetDecimationFactor(RP_DEC_1) != RP_OK) {
        fprintf(stderr, "rp_AcqAxiSetDecimationFactor failed!\n");
        return -1;
    }
    if (rp_AcqAxiSetTriggerDelay(RP_CH_1, DATA_SIZE  )  != RP_OK) {
        fprintf(stderr, "rp_AcqAxiSetTriggerDelay RP_CH_1 failed!\n");
        return -1;
    }
    if (rp_AcqAxiSetTriggerDelay(RP_CH_2, DATA_SIZE  ) != RP_OK) {
        fprintf(stderr, "rp_AcqAxiSetTriggerDelay RP_CH_2 failed!\n");
        return -1;
    }
    if (rp_AcqAxiSetBufferSamples(RP_CH_1, g_adc_axi_start, DATA_SIZE) != RP_OK) {
        fprintf(stderr, "rp_AcqAxiSetBuffer RP_CH_1 failed!\n");
        return -1;
    }
    if (rp_AcqAxiSetBufferSamples(RP_CH_2, g_adc_axi_start + g_adc_axi_size / 2, DATA_SIZE) != RP_OK) {
        fprintf(stderr, "rp_AcqAxiSetBuffer RP_CH_2 failed!\n");
        return -1;
    }
    if (rp_AcqAxiEnable(RP_CH_1, true)) {
        fprintf(stderr, "rp_AcqAxiEnable RP_CH_1 failed!\n");
        return -1;
    }
    printf("Enable cha\n");

    if (rp_AcqAxiEnable(RP_CH_2, true)) {
        fprintf(stderr, "rp_AcqAxiEnable RP_CH_2 failed!\n");
        return -1;
    }
    printf("Enable chb\n");

    rp_AcqSetTriggerLevel(RP_T_CH_1,0);
    printf("Start acq\n");
    if (rp_AcqStart() != RP_OK) {
        fprintf(stderr, "rp_AcqStart failed!\n");
        return -1;
    }

    rp_AcqSetTriggerSrc(RP_TRIG_SRC_NOW);
    rp_acq_trig_state_t state = RP_TRIG_STATE_TRIGGERED;

    while(1){
        rp_AcqGetTriggerState(&state);
        if(state == RP_TRIG_STATE_TRIGGERED){
            sleep(1);
            break;
        }
    }

    bool fillState = false;
    while (!fillState) {
        if (rp_AcqAxiGetBufferFillState(RP_CH_1, &fillState) != RP_OK) {
            fprintf(stderr, "rp_AcqAxiGetBufferFillState RP_CH_1 failed!\n");
            return -1;
        }
    }
    rp_AcqStop();
    printf("Stop acq\n");

    uint32_t posChA,posChB;
    rp_AcqAxiGetWritePointerAtTrig(RP_CH_1,&posChA);
    rp_AcqAxiGetWritePointerAtTrig(RP_CH_2,&posChB);

    // fprintf(stderr,"Tr pos1: 0x%X pos2: 0x%X\n",posChA,posChB);

    int16_t *buff1 = (int16_t *)malloc(READ_DATA_SIZE * sizeof(int16_t));
    int16_t *buff2 = (int16_t *)malloc(READ_DATA_SIZE * sizeof(int16_t));

    int readed_size = 0;

    FILE *fp = fopen ("out.txt", "w");

    int line = 1;
    while (readed_size < DATA_SIZE){
        uint32_t size1 = READ_DATA_SIZE;
        uint32_t size2 = READ_DATA_SIZE;
        rp_AcqAxiGetDataRaw(RP_CH_1, posChA, &size1, buff1);
        rp_AcqAxiGetDataRaw(RP_CH_2, posChB, &size2, buff2);
        for (int i = 0; i < READ_DATA_SIZE; i++) {
            fprintf(fp,"%d:  %d\t%d\n",line++, buff1[i], buff2[i]);
        }
        posChA += size1;
        posChB += size2;
        readed_size += READ_DATA_SIZE;
        printf("Saved data size %d\n",readed_size);
    }

    /* Releasing resources */
    rp_AcqAxiEnable(RP_CH_1, false);
    rp_AcqAxiEnable(RP_CH_2, false);
    rp_Release();
    free(buff1);
    free(buff2);
    fclose(fp);
    return 0;
}
