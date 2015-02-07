#include "wifi_hal.h"

#ifndef __WIFI_HAL_LOGGER_H
#define __WIFI_HAL_LOGGER_H

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#define LOGGER_MAJOR_VERSION      1
#define LOGGER_MINOR_VERSION      0
#define LOGGER_MICRO_VERSION      0

typedef int wifi_radio;
typedef int wifi_ring_buffer_id;

typedef struct {
    u8 direction:1; //  0: TX, 1: RX
    u8 success:1; // whether packet was transmitted or received/decrypted successfully
    u8 has_80211_header:1; // has full 802.11 header, else has 802.3 header
    u8 protected_packet:1; // whether packet was encrypted
    u8 tid:4; // transmit  or received tid
    u8 MCS; // modulation and bandwidth
    u8 rssi; // TX: RSSI of ACK for that packet
             // RX: RSSI of packet
    u8 num_retries; // number of attempted retries
    u16 last_transmit_rate; // last transmit rate in .5 mbps
    u16 link_layer_transmit_sequence; // transmit/reeive sequence for that MPDU packet
    u64 firmware_entry_timestamp;  // TX: firmware timestamp (us) when packet is queued within firmware buffer
                                   // for SDIO/HSIC or into PCIe buffer
                                   // RX : firmware receive timestamp
    u64 start_contention_timestamp; // firmware timestamp (us) when packet start contending for the
                                    // medium for the first time, at head of its AC queue,
                                    // or as part of an MPDU or A-MPDU. This timestamp is not updated
                                    // for each retry, only the first transmit attempt.
    u64 transmit_success_timestamp; // fimrware timestamp (us) when packet is successfully transmitted
                                    // or aborted because it has exhausted its maximum number of retries
    u8 data[0]; // packet data. The length of packet data is determined by the entry_size field of
                // the wifi_ring_buffer_entry structure. It is expected that first bytes of the
                // packet, or packet headers only (up to TCP or RTP/UDP headers) will be copied into the ring
} wifi_ring_per_packet_status_entry;



#define WIFI_EVENT_ASSOCIATION_REQUESTED 0 // driver receive association command from kernel
#define WIFI_EVENT_AUTH_COMPLETE 1
#define WIFI_EVENT_ASSOC_COMPLETE 2
#define WIFI_EVENT_FW_AUTH_STARTED 3 // received firmware event indicating auth frames are sent
#define WIFI_EVENT_FW_ASSOC_STARTED 4 // received firmware event indicating assoc frames are sent
#define WIFI_EVENT_FW_RE_ASSOC_STARTED 5 // received firmware event indicating reassoc frames are sent
#define WIFI_EVENT_DRIVER_SCAN_REQUESTED 6
#define WIFI_EVENT_DRIVER_SCAN_RESULT_FOUND 7
#define WIFI_EVENT_DRIVER_SCAN_COMPLETE 8
#define WIFI_EVENT_G_SCAN_STARTED 9
#define WIFI_EVENT_G_SCAN_COMPLETE 10
#define WIFI_EVENT_DISASSOCIATION_REQUESTED 11
#define WIFI_EVENT_RE_ASSOCIATION_REQUESTED 12
#define WIFI_EVENT_ROAM_REQUESTED 13
#define WIFI_EVENT_BEACON_RECEIVED 14 // received beacon from AP (event enabled only in verbose mode)
#define WIFI_EVENT_ROAM_SCAN_STARTED 15 // firmware has triggered a roam scan (not g-scan)
#define WIFI_EVENT_ROAM_SCAN_COMPLETE 16 // firmware has completed a roam scan (not g-scan)
#define WIFI_EVENT_ROAM_SEARCH_STARTED 17 // firmware has started searching for roam candidates (with reason =xx)
#define WIFI_EVENT_ROAM_SEARCH_STOPPED 18 // firmware has stopped searching for roam candidates (with reason =xx)
#define WIFI_EVENT_CHANNEL_SWITCH_ANOUNCEMENT 20 // received channel switch anouncement from AP
#define WIFI_EVENT_FW_EAPOL_FRAME_TRANSMIT_START 21 // fw start transmit eapol frame, with EAPOL index 1-4
#define WIFI_EVENT_FW_EAPOL_FRAME_TRANSMIT_STOP 22 // fw gives up eapol frame, with rate, success/failure and number retries
#define WIFI_EVENT_DRIVER_EAPOL_FRAME_TRANSMIT_REQUESTED 23 // kernel queue EAPOL for transmission in tdriver
                                                            // with EAPOL index 1-4
#define WIFI_EVENT_FW_EAPOL_FRAME_RECEIVED 24 // with rate, regardless of the fact that EAPOL frame
                                           // is accepted or rejected by firmware
#define WIFI_EVENT_DRIVER_EAPOL_FRAME_RECEIVED 26 // with rate, and eapol index, driver has received EAPOL
                                                    //frame and will queue it up to wpa_supplicant
#define WIFI_EVENT_BLOCK_ACK_NEGOTIATION_COMPLETE 27 // with success/failure, parameters
#define WIFI_EVENT_BT_COEX_BT_SCO_START 28
#define WIFI_EVENT_BT_COEX_BT_SCO_STOP 29
#define WIFI_EVENT_BT_COEX_BT_SCAN_START 30 // for paging/scan etc..., when BT starts transmiting twice per BT slot
#define WIFI_EVENT_BT_COEX_BT_SCAN_STOP 31
#define WIFI_EVENT_BT_COEX_BT_HID_START 32
#define WIFI_EVENT_BT_COEX_BT_HID_STOP 33
#define WIFI_EVENT_ROAM_AUTH_STARTED 34 // firmware sends auth frame in roaming to next candidate
#define WIFI_EVENT_ROAM_AUTH_COMPLETE 35 // firmware receive auth confirm from ap
#define WIFI_EVENT_ROAM_ASSOC_STARTED 36 // firmware sends assoc/reassoc frame in roaming to next candidate
#define WIFI_EVENT_ROAM_ASSOC_COMPLETE 37 // firmware receive assoc/reassoc confirm from ap


typedef struct {
    u16 event;
    u8 event_data[0]; // separate parameter structure per event to be provided and optional data
                        // the event_data is expected to include an official android part, with some parameter
                        // as transmit rate, num retries, num scan result found etc...
                        // as well, event_data can include a vendor proprietary part which is understood
                        // by the developer only.
} wifi_ring_buffer_driver_connectivity_event;


/**
 * This structure represent a logger entry within a ring.
 * Binary entries can be used so as to store packet data or vendor specific information.
 */
typedef struct {
    u16 entry_size:13;
    u16 binary:1; //set for binary entries
    u16 has_timestamp:1; //set if 64 bits timestamp is present
    u16 reserved:1;
    u8 type; // Per ring specific
    u8 resvd;
    u64 timestamp; //present if has_timestamp bit is set.
    u8 data[0];
} wifi_ring_buffer_entry;

#define WIFI_RING_BUFFER_FLAG_HAS_BINARY_ENTRIES         0x00000001     // set if binary entries are present
#define WIFI_RING_BUFFER_FLAG_HAS_ASCII_ENTRIES          0x00000002     // set if ascii entries are present

/* ring buffer params */
/**
 * written_bytes and read_bytes implement a producer consumer API
 *     hence written_bytes >= read_bytes
 * a modulo arithmetic of the buffer size has to be applied to those counters:
 * actual offset into ring buffer = written_bytes % ring_buffer_byte_size
 *
 */
typedef struct {
   u8 name[32];
   u32 flags;
   u64 fd; // linux file descriptor for that buffer
   u32 ring_buffer_byte_size;   // total memory size allocated for the buffer
   u32 verbose_level; //
   u32 written_bytes; // number of bytes that was written to the buffer by driver, monotonously increasing integer
   u32 read_bytes;  // number of bytes that was read from the buffer by user land, monotonously increasing integer
} wifi_ring_buffer_status;

/* API to trigger the debug collection.
   Unless his API is invoked - logging is not triggered.
   - verbose_level 0 corresponds to minimal or no collection
   - verbose_level 1+ are TBD
   */
wifi_error wifi_start_logging(wifi_interface_handle iface, u32 verbose_level, u8 * buffer_name);

/* callback for reporting ring buffer status */
typedef struct {
  void (*on_ring_buffer_status_results) (wifi_request_id id, u32 num_buffers, wifi_ring_buffer_status *status);
} wifi_ring_buffer_status_result_handler;

/* api to get the status of a ring buffer */
wifi_error wifi_get_ring_buffer_status(wifi_request_id id,
        wifi_interface_handle iface, wifi_ring_buffer_id ring_id, wifi_ring_buffer_status_result_handler handler);

/* api to collect a firmware memory dump for a given iface */
wifi_error wifi_get_firmware_memory_dump(wifi_request_id id,
        wifi_interface_handle iface, char * buffer, int buffer_size);

/* api to collect a firmware version string */
wifi_error wifi_get_firmware_version(wifi_request_id id,
        wifi_interface_handle iface, char * buffer, int buffer_size);

/* api to collect a driver version string */
wifi_error wifi_get_driver_version(wifi_request_id id,
        wifi_interface_handle iface, char * buffer, int buffer_size);


/* Feature set */
#define WIFI_LOGGER_MEMORY_DUMP_SUPPORTED 1
#define WIFI_LOGGER_PER_PACKET_TX_RX_STATUS_SUPPORTED 2

wifi_error wifi_get_logger_supported_feature_set(wifi_interface_handle handle, unsigned int *support);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*__WIFI_HAL_STATS_ */

