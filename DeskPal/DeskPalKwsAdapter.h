#pragma once

// Integration seam for the generated Edge Impulse Arduino library.
// Replace this contract after exporting the trained model. Do not set the
// readiness macro until the implementation has passed the KWS acceptance tests.
#define DESKPAL_KWS_ADAPTER_READY 0

// Return KWS_LABEL_WAKE / KWS_LABEL_START_TIMER / KWS_LABEL_STOP_TIMER only
// when the model confidence meets KWS_CONFIDENCE_FLOOR; otherwise return
// KWS_LABEL_REJECT. The implementation must consume a complete model window
// from the 16 kHz mono I2S stream and must not retain a pointer to DMA memory.
int classifyKeywordEdge();
