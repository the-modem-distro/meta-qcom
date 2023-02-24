// SPDX-License-Identifier: MIT
#ifndef __VOICE__H_
#define __VOICE__H_

#include "openqti.h"
#include "qmi.h"
#include "call.h"
#include <stdbool.h>
#include <stdio.h>
/*
 * Headers for the Voice Service
 *
 *
 *
 */

enum {
	 VOICE_INDICATION_REGISTER = 0x0003, 
	 VOICE_GET_SUPPORTED_MESSAGES = 0x001E, 
	 VOICE_DIAL_CALL = 0x0020, 
	 VOICE_END_CALL = 0x0021, 
	 VOICE_ANSWER_CALL = 0x0022, 
	 VOICE_ALL_CALL_STATUS = 0x002E, 
	 VOICE_GET_ALL_CALL_INFO = 0x002F, 
	 VOICE_MANAGE_CALLS = 0x0031, 
	 VOICE_SUPPLEMENTARY_SERVICE = 0x0032, 
	 VOICE_SET_SUPPLEMENTARY_SERVICE = 0x0033, 
	 VOICE_GET_CALL_WAITING = 0x0034, 
	 VOICE_ORIGINATE_USSD = 0x003A, 
	 VOICE_ANSWER_USSD = 0x003B, 
	 VOICE_CANCEL_USSD = 0x003C, 
	 VOICE_RELEASE_USSD = 0x003D, 
	 VOICE_USSD = 0x003E, 
	 VOICE_GET_CONFIG = 0x0041, 
	 VOICE_ORIGINATE_USSD_NO_WAIT = 0x0043, 
	 VOICE_BURST_DTMF = 0x0028, 
	 VOICE_START_CONTINUOUS_DTMF = 0x0029, 
	 VOICE_STOP_CONTINUOUS_DTMF = 0x002A, 
};

static const struct {
    uint16_t id;
    const char *cmd;
} voice_svc_commands[] = {
{VOICE_INDICATION_REGISTER,"Indication Register"}, 
{VOICE_GET_SUPPORTED_MESSAGES,"Get Supported Messages"}, 
{VOICE_DIAL_CALL,"Dial Call"}, 
{VOICE_END_CALL,"End Call"}, 
{VOICE_ANSWER_CALL,"Answer Call"}, 
{VOICE_ALL_CALL_STATUS,"All Call Status"}, 
{VOICE_GET_ALL_CALL_INFO,"Get All Call Info"}, 
{VOICE_MANAGE_CALLS,"Manage Calls"}, 
{VOICE_SUPPLEMENTARY_SERVICE,"Supplementary Service"}, 
{VOICE_SET_SUPPLEMENTARY_SERVICE,"Set Supplementary Service"}, 
{VOICE_GET_CALL_WAITING,"Get Call Waiting"}, 
{VOICE_ORIGINATE_USSD,"Originate USSD"}, 
{VOICE_ANSWER_USSD,"Answer USSD"}, 
{VOICE_CANCEL_USSD,"Cancel USSD"}, 
{VOICE_RELEASE_USSD,"Release USSD"}, 
{VOICE_USSD,"USSD"}, 
{VOICE_GET_CONFIG,"Get Config"}, 
{VOICE_ORIGINATE_USSD_NO_WAIT,"Originate USSD No Wait"}, 
{VOICE_BURST_DTMF,"Burst DTMF"}, 
{VOICE_START_CONTINUOUS_DTMF,"Start Continuous DTMF"}, 
{VOICE_STOP_CONTINUOUS_DTMF,"Stop Continuous DTMF"}, 
};

enum VoiceIndicationTLVs {
    VOICE_EVENT_DTMF = 0x10,
    VOICE_EVENT_VOICE_PRIVACY = 0x11,
    VOICE_EVENT_SUPPLEMENTARY_SERVICES = 0x12,
    VOICE_EVENT_CALL_EVENTS = 0x13,
    VOICE_EVENT_HANDOVER_EVENTS = 0x14,
    VOICE_EVENT_SPEECH_EVENTS = 0x15,
    VOICE_EVENT_USSD_NOTIFICATIONS = 0x16,
    VOICE_EVENT_MODIFICATION_EVENTS = 0x18,
    VOICE_EVENT_UUS_EVENTS = 0x19,
    VOICE_EVENT_AOC_EVENTS = 0x1a,
    VOICE_EVENT_CONFERENCE_EVENTS = 0x1b,
    VOICE_EVENT_CALL_CONTROL_EVENTS = 0x1e,
    VOICE_EVENT_CONFERENCE_PARTICIPANT_EVENT = 0x1f,
    VOICE_EVENT_EMERG_CALL_ORIGINATION_FAILURE_EVENTS = 0x21,
    VOICE_EVENT_ADDITIONAL_CALL_INFO_EVENTS = 0x24,
    VOICE_EVENT_EMERGENCY_CALL_STATUS_EVENTS = 0x25,
    VOICE_EVENT_CALL_REESTABLISHMENT_STATUS = 0x26,
    VOICE_EVENT_EMERGENCY_CALL_OPERATING_MODE_EVENT = 0x28,
    VOICE_EVENT_AUTOREJECTED_INCOMING_CALL_END_EVENT = 0x29,
};
const char *get_voice_command(uint16_t msgid);
void *register_to_voice_service();
int handle_incoming_voice_message(uint8_t *buf, size_t buf_len);
#endif
