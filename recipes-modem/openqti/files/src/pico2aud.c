/* pico2wave.c

 * Copyright (C) 2009 Mathieu Parent <math.parent@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *   Convert text to .wav using svox text-to-speech system.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../inc/audio.h"
#include "../inc/logger.h"
#include "../inc/openqti.h"
#include <picoapi.h>
#include <picoapid.h>
#include <picoos.h>

/* adaptation layer defines */
#define PICO_MEM_SIZE 2500000
#define DummyLen 100000000

/* string constants */
#define MAX_OUTBUF_SIZE 128
#ifdef picolangdir
const char *PICO_LINGWARE_PATH = picolangdir "/";
#else
const char *PICO_LINGWARE_PATH = "/usr/share/lang/";
#endif
const char *PICO_VOICE_NAME = "PicoVoice";

/* supported voices
   Pico does not seperately specify the voice and locale.   */
const char *picoSupportedLangIso3[] = {"eng"};
const char *picoSupportedCountryIso3[] = {"USA"};
const char *picoSupportedLang[] = {"en-US"};
const char *picoInternalLang[] = {"en-US"};
const char *picoInternalTaLingware[] = {"en-US_ta.bin"};
const char *picoInternalSgLingware[] = {"en-US_lh0_sg.bin"};
const char *picoInternalUtppLingware[] = {"en-US_utpp.bin"};
const int picoNumSupportedVocs = 6;

/* adapation layer global variables */
void *picoMemArea = NULL;
pico_System picoSystem = NULL;
pico_Resource picoTaResource = NULL;
pico_Resource picoSgResource = NULL;
pico_Resource picoUtppResource = NULL;
pico_Engine picoEngine = NULL;
pico_Char *picoTaFileName = NULL;
pico_Char *picoSgFileName = NULL;
pico_Char *picoUtppFileName = NULL;
pico_Char *picoTaResourceName = NULL;
pico_Char *picoSgResourceName = NULL;
pico_Char *picoUtppResourceName = NULL;
int picoSynthAbort = 0;

// So we need, wave sample pointer, and pointer to the actual text

int pico2aud(char *phrase, size_t len) {
  char *lang = "en-US";
  char *wavefile = "/tmp/wave.wav";
  int langIndex = 0, langIndexTmp = 0;
  int8_t *buffer;
  size_t bufferSize = 256;

  char *text = phrase;
  if (len < 1) {
    logger(MSG_WARN, "%s: Nothing to say\n", __func__);
    return 0;
  }

  buffer = malloc(bufferSize);

  int ret, getstatus;
  pico_Char *inp = NULL;
  pico_Char *local_text = NULL;
  short outbuf[MAX_OUTBUF_SIZE / 2];
  pico_Int16 bytes_sent, bytes_recv, text_remaining, out_data_type;
  pico_Retstring outMessage;

  picoSynthAbort = 0;
  logger(MSG_DEBUG, "%s: Starting PicoTTS Engine\n", __func__);
  picoMemArea = malloc(PICO_MEM_SIZE);
  if ((ret = pico_initialize(picoMemArea, PICO_MEM_SIZE, &picoSystem))) {
    pico_getSystemStatusMessage(picoSystem, ret, outMessage);
    logger(MSG_ERROR, "Cannot initialize pico (%i): %s\n", ret, outMessage);
    goto terminate;
  }

  logger(MSG_DEBUG, "%s: Text analysis Lingware resouce file\n", __func__);

  /* Load the text analysis Lingware resource file.   */
  picoTaFileName = (pico_Char *)malloc(PICO_MAX_DATAPATH_NAME_SIZE +
                                       PICO_MAX_FILE_NAME_SIZE);
  strcpy((char *)picoTaFileName, PICO_LINGWARE_PATH);
  strcat((char *)picoTaFileName,
         (const char *)picoInternalTaLingware[langIndex]);
  if ((ret = pico_loadResource(picoSystem, picoTaFileName, &picoTaResource))) {
    pico_getSystemStatusMessage(picoSystem, ret, outMessage);
    logger(MSG_ERROR, "Cannot load text analysis resource file (%i): %s\n", ret,
           outMessage);
    goto unloadTaResource;
  }

  logger(MSG_DEBUG, "%s: Signal generation lingware resource file\n", __func__);
  /* Load the signal generation Lingware resource file.   */
  picoSgFileName = (pico_Char *)malloc(PICO_MAX_DATAPATH_NAME_SIZE +
                                       PICO_MAX_FILE_NAME_SIZE);
  strcpy((char *)picoSgFileName, PICO_LINGWARE_PATH);
  strcat((char *)picoSgFileName,
         (const char *)picoInternalSgLingware[langIndex]);
  if ((ret = pico_loadResource(picoSystem, picoSgFileName, &picoSgResource))) {
    pico_getSystemStatusMessage(picoSystem, ret, outMessage);
    logger(MSG_ERROR,
           "Cannot load signal generation Lingware resource file (%i): %s\n",
           ret, outMessage);
    goto unloadSgResource;
  }

  logger(MSG_DEBUG, "%s: Text analysis resource file\n", __func__);
  /* Get the text analysis resource name.     */
  picoTaResourceName = (pico_Char *)malloc(PICO_MAX_RESOURCE_NAME_SIZE);
  if ((ret = pico_getResourceName(picoSystem, picoTaResource,
                                  (char *)picoTaResourceName))) {
    pico_getSystemStatusMessage(picoSystem, ret, outMessage);
    logger(MSG_ERROR, "Cannot get the text analysis resource name (%i): %s\n",
           ret, outMessage);
    goto unloadUtppResource;
  }

  logger(MSG_DEBUG, "%s: Signal generation resource file\n", __func__);
  /* Get the signal generation resource name. */
  picoSgResourceName = (pico_Char *)malloc(PICO_MAX_RESOURCE_NAME_SIZE);
  if ((ret = pico_getResourceName(picoSystem, picoSgResource,
                                  (char *)picoSgResourceName))) {
    pico_getSystemStatusMessage(picoSystem, ret, outMessage);
    logger(MSG_ERROR,
           "Cannot get the signal generation resource name (%i): %s\n", ret,
           outMessage);
    goto unloadUtppResource;
  }

  logger(MSG_DEBUG, "%s: Voice definition\n", __func__);
  /* Create a voice definition.   */
  if ((ret = pico_createVoiceDefinition(picoSystem,
                                        (const pico_Char *)PICO_VOICE_NAME))) {
    pico_getSystemStatusMessage(picoSystem, ret, outMessage);
    logger(MSG_ERROR, "Cannot create voice definition (%i): %s\n", ret,
           outMessage);
    goto unloadUtppResource;
  }

  logger(MSG_DEBUG, "%s: Add Text analysis resource \n", __func__);
  /* Add the text analysis resource to the voice. */
  if ((ret = pico_addResourceToVoiceDefinition(
           picoSystem, (const pico_Char *)PICO_VOICE_NAME,
           picoTaResourceName))) {
    pico_getSystemStatusMessage(picoSystem, ret, outMessage);
    logger(MSG_ERROR,
           "Cannot add the text analysis resource to the voice (%i): %s\n", ret,
           outMessage);
    goto unloadUtppResource;
  }

  logger(MSG_DEBUG, "%s: Add Signal generation resource \n", __func__);
  /* Add the signal generation resource to the voice. */
  if ((ret = pico_addResourceToVoiceDefinition(
           picoSystem, (const pico_Char *)PICO_VOICE_NAME,
           picoSgResourceName))) {
    pico_getSystemStatusMessage(picoSystem, ret, outMessage);
    logger(MSG_ERROR,
           "Cannot add the signal generation resource to the voice (%i): %s\n",
           ret, outMessage);
    goto unloadUtppResource;
  }

  logger(MSG_DEBUG, "%s: Create engine \n", __func__);
  /* Create a new Pico engine. */
  if ((ret = pico_newEngine(picoSystem, (const pico_Char *)PICO_VOICE_NAME,
                            &picoEngine))) {
    pico_getSystemStatusMessage(picoSystem, ret, outMessage);
    logger(MSG_ERROR, "Cannot create a new pico engine (%i): %s\n", ret,
           outMessage);
    goto disposeEngine;
  }

  local_text = (pico_Char *)text;
  text_remaining = strlen((const char *)local_text) + 1;

  inp = (pico_Char *)local_text;

  size_t bufused = 0;

  picoos_Common common = (picoos_Common)pico_sysGetCommon(picoSystem);

  picoos_SDFile sdOutFile = NULL;

  picoos_bool done = TRUE;
  if (TRUE !=
      (done = picoos_sdfOpenOut(common, &sdOutFile, (picoos_char *)wavefile,
                                SAMPLE_FREQ_16KHZ, PICOOS_ENC_LIN))) {
    logger(MSG_ERROR, "Cannot open output wave file\n");
    ret = 1;
    goto disposeEngine;
  }

  /* synthesis loop   */
  while (text_remaining) {
    /* Feed the text into the engine.   */
    if ((ret =
             pico_putTextUtf8(picoEngine, inp, text_remaining, &bytes_sent))) {
      pico_getSystemStatusMessage(picoSystem, ret, outMessage);
      logger(MSG_ERROR, "Cannot put Text (%i): %s\n", ret, outMessage);
      goto disposeEngine;
    }

    text_remaining -= bytes_sent;
    inp += bytes_sent;

    do {
      if (picoSynthAbort) {
        goto disposeEngine;
      }
      /* Retrieve the samples and add them to the buffer. */
      getstatus = pico_getData(picoEngine, (void *)outbuf, MAX_OUTBUF_SIZE,
                               &bytes_recv, &out_data_type);
      if ((getstatus != PICO_STEP_BUSY) && (getstatus != PICO_STEP_IDLE)) {
        pico_getSystemStatusMessage(picoSystem, getstatus, outMessage);
        logger(MSG_ERROR, "Cannot get Data (%i): %s\n", getstatus, outMessage);
        goto disposeEngine;
      }
      if (bytes_recv) {
        if ((bufused + bytes_recv) <= bufferSize) {
          memcpy(buffer + bufused, (int8_t *)outbuf, bytes_recv);
          bufused += bytes_recv;
        } else {
          done = picoos_sdfPutSamples(sdOutFile, bufused / 2,
                                      (picoos_int16 *)(buffer));
          bufused = 0;
          memcpy(buffer, (int8_t *)outbuf, bytes_recv);
          bufused += bytes_recv;
        }
      }
    } while (PICO_STEP_BUSY == getstatus);
    /* This chunk of synthesis is finished; pass the remaining samples. */
    if (!picoSynthAbort) {
      done = picoos_sdfPutSamples(sdOutFile, bufused / 2,
                                  (picoos_int16 *)(buffer));
    }
    picoSynthAbort = 0;
  }

  if (TRUE != (done = picoos_sdfCloseOut(common, &sdOutFile))) {
    logger(MSG_ERROR, "Cannot close output wave file\n");
    ret = 1;
    goto disposeEngine;
  }

disposeEngine:
  if (picoEngine) {
    pico_disposeEngine(picoSystem, &picoEngine);
    pico_releaseVoiceDefinition(picoSystem, (pico_Char *)PICO_VOICE_NAME);
    picoEngine = NULL;
  }
unloadUtppResource:
  if (picoUtppResource) {
    pico_unloadResource(picoSystem, &picoUtppResource);
    picoUtppResource = NULL;
  }
unloadSgResource:
  if (picoSgResource) {
    pico_unloadResource(picoSystem, &picoSgResource);
    picoSgResource = NULL;
  }
unloadTaResource:
  if (picoTaResource) {
    pico_unloadResource(picoSystem, &picoTaResource);
    picoTaResource = NULL;
  }
terminate:
  if (picoSystem) {
    pico_terminate(&picoSystem);
    picoSystem = NULL;
  }

  logger(MSG_DEBUG, "%s: Getting out of pico2aud - %i\n", __func__, ret);
  
  free(buffer);
  free(picoMemArea);
  free(picoTaFileName);
  free(picoSgFileName);
  free(picoTaResourceName);
  free(picoSgResourceName);
  free(picoSystem);
  free(picoTaResource);
  free(picoSgResource);
  free(picoUtppResource);
  free(picoEngine);
  free(picoUtppFileName);
  free(picoUtppResourceName);

  buffer = NULL;
  picoMemArea = NULL;
  picoTaFileName = NULL;
  picoSgFileName = NULL;
  picoTaResourceName = NULL;
  picoSgResourceName = NULL;
  picoSystem = NULL;
  picoTaResource = NULL;
  picoSgResource = NULL;
  picoUtppResource = NULL;
  picoEngine = NULL;
  picoUtppFileName = NULL;
  picoUtppResourceName = NULL;
  return ret;
}
