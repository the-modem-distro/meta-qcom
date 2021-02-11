#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <endian.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>

#include "atfwd.h"


void create_qmi_request(uint8_t *buf, uint8_t service, uint8_t client_id, 
        uint16_t transaction_id, uint16_t message_id){
    qmux_hdr_t *qmux_hdr = (qmux_hdr_t*) buf;
    if(service == QMI_SERVICE_CTL)
    {
        //-1 is for removing the preamble
        qmux_hdr->length = htole16(sizeof(qmux_hdr_t) - 1 +
                sizeof(qmi_hdr_ctl_t));
    }
    else {
        qmux_hdr->length = htole16(sizeof(qmux_hdr_t) - 1 +
                sizeof(qmi_hdr_gen_t));
    }
    //Messages are send from the control point
    qmux_hdr->control_flags = 0;
    //Which service I want to request something from. Remember that CTL is 0
    qmux_hdr->service_type = service;
    qmux_hdr->client_id = client_id;

    //Can I somehow do this more elegant?
    if(service == QMI_SERVICE_CTL){
        qmi_hdr_ctl_t *qmi_hdr = (qmi_hdr_ctl_t*) (qmux_hdr+1);
        //Always sends request (message type 0, only flag)
        qmi_hdr->control_flags = 0;
        //Internal transaction sequence number (one message exchange)
        qmi_hdr->transaction_id = transaction_id;
        //Type of message
        qmi_hdr->message_id = htole16(message_id);
        qmi_hdr->length = 0;
    } else {
        qmi_hdr_gen_t *qmi_hdr = (qmi_hdr_gen_t*) (qmux_hdr+1);
        qmi_hdr->control_flags = 0;
        qmi_hdr->transaction_id = htole16(transaction_id);
        qmi_hdr->message_id = htole16(message_id);
        qmi_hdr->length = 0;
    }
}

//Assume only one TLV parameter for now
void add_tlv(uint8_t *buf, uint8_t type, uint16_t length, void *value){
    qmux_hdr_t *qmux_hdr = (qmux_hdr_t*) buf;
    qmi_tlv_t *tlv;

    assert(le16toh(qmux_hdr->length) + length + sizeof(qmi_tlv_t) <
            QMI_DEFAULT_BUF_SIZE);

    //+1 is to compensate or the mark, which is now part of message
    tlv = (qmi_tlv_t*) (buf + le16toh(qmux_hdr->length) + 1);
    tlv->type = type;
    tlv->length = htole16(length);
    memcpy(tlv + 1, value, length);

    //Update the length of thw qmux and qmi headers
    qmux_hdr->length += htole16(sizeof(qmi_tlv_t) + length);

    //Updte QMI service length
    if(qmux_hdr->service_type == QMI_SERVICE_CTL){
        qmi_hdr_ctl_t *qmi_hdr = (qmi_hdr_ctl_t*) (qmux_hdr+1);
        qmi_hdr->length += htole16(sizeof(qmi_tlv_t) + length);
    } else {
        qmi_hdr_gen_t *qmi_hdr = (qmi_hdr_gen_t*) (qmux_hdr+1);
        qmi_hdr->length += htole16(sizeof(qmi_tlv_t) + length);
    }
}

void parse_qmi(uint8_t *buf){
    int i, j;
    uint8_t *tlv_val = NULL;
    qmux_hdr_t *qmux_hdr = (qmux_hdr_t*) buf;
    qmi_tlv_t *tlv = NULL;
    uint16_t tlv_length = 0;

    fprintf(stderr, " --> MSG: ");
    //When I call this function, a messages is either ready to be sent or has
    //been received. All values are in little endian
    for(i=0; i < le16toh(qmux_hdr->length); i++)
        fprintf(stderr, "%.2x:", buf[i]);

    //I need the last byte, since I have added the marker to the qmux header
    //(and this byte is not included in length)
    fprintf(stderr, "%.2x\n", buf[i]);

    fprintf(stderr, " ---> QMUX: ");
    fprintf(stderr, "length: %u | ", le16toh(qmux_hdr->length));
    fprintf(stderr, "flags: 0x%.2x | ", qmux_hdr->control_flags);
    fprintf(stderr, "service: 0x%.2x | ", qmux_hdr->service_type);
    fprintf(stderr, "client id: %u\n", qmux_hdr->client_id);

    if(qmux_hdr->service_type == QMI_SERVICE_CTL){
        qmi_hdr_ctl_t *qmi_hdr = (qmi_hdr_ctl_t*) (qmux_hdr+1);
        fprintf(stderr, " ---> QMI (control): ");
        fprintf(stderr, "flags: %u | ", qmi_hdr->control_flags >> 1);
        fprintf(stderr, "transaction id: %u | ", qmi_hdr->transaction_id);
        fprintf(stderr, "message type: 0x%.2x | ", le16toh(qmi_hdr->message_id));
        fprintf(stderr, "length: %u %x\n", le16toh(qmi_hdr->length), le16toh(qmi_hdr->length));
        tlv = (qmi_tlv_t *) (qmi_hdr+1);
        tlv_length = le16toh(qmi_hdr->length);
    } else {
        qmi_hdr_gen_t *qmi_hdr = (qmi_hdr_gen_t*) (qmux_hdr+1);
        fprintf(stderr, " ---> QMI (service): ");
        fprintf(stderr, "flags: %u\n", qmi_hdr->control_flags >> 1);
        fprintf(stderr, "transaction id: %u | ", le16toh(qmi_hdr->transaction_id));
        fprintf(stderr, "message type: 0x%.2x | ", le16toh(qmi_hdr->message_id));
        fprintf(stderr, "length: %u\n", le16toh(qmi_hdr->length));
        tlv = (qmi_tlv_t *) (qmi_hdr+1);
        tlv_length = le16toh(qmi_hdr->length);
    }

    i=0;
    while(i<tlv_length){
        tlv_val = (uint8_t*) (tlv+1);
        fprintf(stderr, " ---> TLV: ");
        fprintf(stderr, "type: 0x%.2x | ", tlv->type);
        fprintf(stderr, "len: %u | ", le16toh(tlv->length));
        fprintf(stderr, "value: ");
        
        for(j=0; j<le16toh(tlv->length)-1; j++)
            fprintf(stderr, "%.2x:", tlv_val[j]);
        fprintf(stderr, "%.2x", tlv_val[j]);

        fprintf(stderr, "\n");
        i += sizeof(qmi_tlv_t) + le16toh(tlv->length);

        if(i==tlv_length)
            break;
        else
            tlv = (qmi_tlv_t*) (((uint8_t*) (tlv+1)) + le16toh(tlv->length));
    }
}


static inline ssize_t qmi_ctl_write(struct qmi_device *qmid, uint8_t *buf,
        ssize_t len){
    //TODO: Only do this if request is sucessful?
    qmid->transaction_id = (qmid->transaction_id + 1) % UINT8_MAX;
    //According to spec, transaction id must be non-zero
    if(!qmid->transaction_id)
        qmid->transaction_id = 1;
        parse_qmi(buf);

    //+1 is to include marker
    return sendto(qmid->fd, buf, len + 1, MSG_DONTWAIT, (void*) &qmid->socket, sizeof(qmid->socket));
}

ssize_t qmi_ctl_update_cid(struct qmi_device *qmid, uint8_t service,
        bool release, uint8_t cid){
    uint8_t buf[QMI_DEFAULT_BUF_SIZE];
    qmux_hdr_t *qmux_hdr = (qmux_hdr_t*) buf;
    uint16_t message_id = release ? QMI_CTL_RELEASE_CID : QMI_CTL_GET_CID;
    //TODO: Perhaps make this nicer, sinceit is only used in one case
    uint16_t tlv_value = htole16((cid << 8) | service);
    ssize_t retval = 0;

    create_qmi_request(buf, QMI_SERVICE_CTL, 0, qmid->transaction_id,
            message_id);

 
        if(release)
            fprintf(stderr, "Releasing CID %x for service %x\n",
                    cid, service);
        else
            fprintf(stderr, "Requesting CID for service %x\n",
                    service);


    if(release)
        add_tlv(buf, QMI_CTL_TLV_ALLOC_INFO, sizeof(uint16_t), &tlv_value);
    else
        add_tlv(buf, QMI_CTL_TLV_ALLOC_INFO, sizeof(uint8_t), &service);

    retval = qmi_ctl_write(qmid, buf, le16toh(qmux_hdr->length));

    if(retval <= 0)
            fprintf(stderr, "Failed to send request for CID for %x\n",
                    service);

    return retval;
}
uint8_t qmi_ctl_request_cid(struct qmi_device *qmid){
    //TODO: Add to timeout
    if(qmi_ctl_update_cid(qmid, qmid->service, false, 0) <= 0)
        return QMI_MSG_FAILURE;

    return QMI_MSG_SUCCESS;
}
ssize_t qmi_ctl_send_sync(struct qmi_device *qmid){
    uint8_t buf[QMI_DEFAULT_BUF_SIZE];
    qmux_hdr_t *qmux_hdr = (qmux_hdr_t*) buf;

    create_qmi_request(buf, QMI_SERVICE_CTL, 0, qmid->transaction_id,
            QMI_CTL_SYNC);
    return qmi_ctl_write(qmid, buf, le16toh(qmux_hdr->length));
}

ssize_t qmi_ctl_send_data_format(struct qmi_device *qmid){
    uint8_t buf[QMI_DEFAULT_BUF_SIZE];
    qmux_hdr_t *qmux_hdr = (qmux_hdr_t*) buf;

	//Values fetched from windows_telenor_qmi
	uint8_t format = 0;
	uint16_t proto = htole16(0x0001);

        fprintf(stderr, "Seding set data format request\n");

    create_qmi_request(buf, QMI_SERVICE_CTL, 0, qmid->transaction_id,
            QMI_CTL_SET_DATA_FORMAT);
	add_tlv(buf, QMI_CTL_TLV_DATA_FORMAT, sizeof(uint8_t), &format);
	add_tlv(buf, QMI_CTL_TLV_DATA_PROTO, sizeof(uint16_t), &proto);

    return qmi_ctl_write(qmid, buf, le16toh(qmux_hdr->length));
}

//Return false is something went wrong (typically no available CID)
static uint8_t qmi_ctl_handle_cid_reply(struct qmi_device *qmid){
    qmux_hdr_t *qmux_hdr = (qmux_hdr_t*) qmid->buf;
    qmi_hdr_ctl_t *qmi_hdr = (qmi_hdr_ctl_t*) (qmux_hdr + 1);
    qmi_tlv_t *tlv = (qmi_tlv_t*) (qmi_hdr + 1);
    uint16_t *result = NULL;
    uint8_t service = 0, cid = 0;

    //A CID reply has two TLVs. First is always the result of the operation
    result = (uint16_t*) (tlv+1);

        fprintf(stderr, "Received CID get/release reply\n");

    //TODO: Improve logic so that I know which service this is?
    if(le16toh(*result) == QMI_RESULT_FAILURE){
            fprintf(stderr, "Failed to get a CID for service %x\n",
                    service);
        return QMI_MSG_FAILURE;
    }

    //Get the CID
    tlv = (qmi_tlv_t*) (((uint8_t*) (tlv+1)) + le16toh(tlv->length));
    service = *((uint8_t*) (tlv+1));
    cid = *(((uint8_t*) (tlv+1)) + 1);

    fprintf(stderr, "Service %x got cid %u\n", service, cid);

    switch(service){
    /*    case QMI_SERVICE_DMS:
            qmid->dms_id = cid;
            qmid->dms_state = DMS_GOT_CID;
            qmid->ctl_num_cids++;
            break;
        case QMI_SERVICE_WDS:
            qmid->wds_id = cid;
            qmid->wds_state = WDS_GOT_CID;
            qmid->ctl_num_cids++;
            break;
        case QMI_SERVICE_NAS:
            qmid->nas_id = cid;
            qmid->nas_state = NAS_GOT_CID;
            qmid->ctl_num_cids++;
            break;*/
        default:
                fprintf(stderr, "CID for service not handled by "
                        "qmid\n");
            break;
    }


    return QMI_MSG_SUCCESS;
}

//static uint8_t qmi_ctl_request_cid(struct qmi_device *qmid);
static uint8_t qmi_ctl_handle_sync_reply(struct qmi_device *qmid){
    qmux_hdr_t *qmux_hdr = (qmux_hdr_t*) qmid->buf;
    qmi_hdr_ctl_t *qmi_hdr = (qmi_hdr_ctl_t*) (qmux_hdr + 1);
    qmi_tlv_t *tlv = (qmi_tlv_t*) (qmi_hdr + 1);
    uint16_t result = *((uint16_t*) (tlv+1));

        fprintf(stderr, "Received SYNC reply\n");

    //All the "rouge" SYNC messages seem to have transaction_id == 0. Use that
    //for now, see if it is consistent or not. I know that I only send one sync
    //message with ID 1, so ignore all SYNC messages that does not have this ID
    if(qmi_hdr->transaction_id != 1 || qmid->ctl_state == CTL_SYNCED){
            fprintf(stderr, "Ignoring sync packet from modem. %u %u\n",
                    qmi_hdr->transaction_id, qmid->ctl_state);
        return QMI_MSG_IGNORE;
    }

    if(le16toh(result) == QMI_RESULT_FAILURE){
            fprintf(stderr, "Sync operation failed\n");
        return QMI_MSG_FAILURE;
    } else{
        qmid->ctl_state = CTL_SYNCED;

        //This can be viewed as the proper start of the dialer. After
        //getting the sync reply, request cid for each service I will use
        //return qmi_ctl_request_cid(qmid);
		return qmi_ctl_send_data_format(qmid);
    }
}

static uint8_t qmi_ctl_handle_data_format(struct qmi_device *qmid){
    qmux_hdr_t *qmux_hdr = (qmux_hdr_t*) qmid->buf;
    qmi_hdr_ctl_t *qmi_hdr = (qmi_hdr_ctl_t*) (qmux_hdr + 1);
    qmi_tlv_t *tlv = (qmi_tlv_t*) (qmi_hdr + 1);
    uint16_t result = *((uint16_t*) (tlv+1));

	if(le16toh(result) == QMI_RESULT_FAILURE){
            fprintf(stderr, "Sync operation failed\n");
        return QMI_MSG_FAILURE;
    }

	tlv = (qmi_tlv_t*) (((uint8_t*) (tlv+1)) + le16toh(tlv->length));
	result = *((uint16_t*) (tlv+1));

		fprintf(stderr, "Data format set to %x\n", le16toh(result));

	return qmi_ctl_request_cid(qmid);
}



uint8_t qmi_ctl_handle_msg(struct qmi_device *qmid){
    qmux_hdr_t *qmux_hdr = (qmux_hdr_t*) qmid->buf;
    qmi_hdr_ctl_t *qmi_hdr = (qmi_hdr_ctl_t*) (qmux_hdr + 1);
    uint8_t retval;

    switch(le16toh(qmi_hdr->message_id)){
        case QMI_CTL_GET_CID:
        case QMI_CTL_RELEASE_CID:
            //Do not set any values unless I am synced
            if(qmid->ctl_state == CTL_NOT_SYNCED){
                retval = QMI_MSG_IGNORE;
                break;
            }

            retval = qmi_ctl_handle_cid_reply(qmid);
            break;
        case QMI_CTL_SYNC:
            //TODO: I suspected some of these packages are sent by the modem
            //every now and then. Check up on that and perhaps have a check on
            //state
            retval = qmi_ctl_handle_sync_reply(qmid);
            break;
		case QMI_CTL_SET_DATA_FORMAT:
			retval = qmi_ctl_handle_data_format(qmid);
			break;
        default:
                fprintf(stderr, "Unknown CTL message of type %x\n",
                        le16toh(qmi_hdr->message_id));
            retval = QMI_MSG_IGNORE;
            break;
    }

    return retval;
}

