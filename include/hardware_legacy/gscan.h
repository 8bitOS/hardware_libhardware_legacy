
#include "wifi_hal.h"

#ifndef __WIFI_HAL_GSCAN_H__
#define __WIFI_HAL_GSCAN_H__

/* AP Scans */

typedef enum {
    WIFI_BAND_UNSPECIFIED,
    WIFI_BAND_BG = 1,                       // 2.4 GHz
    WIFI_BAND_A = 2,                        // 5 GHz without DFS
    WIFI_BAND_A_DFS = 4,                    // 5 GHz DFS only
    WIFI_BAND_A_WITH_DFS = 6,               // 5 GHz with DFS
    WIFI_BAND_ABG = 3,                      // 2.4 GHz + 5 GHz; no DFS
    WIFI_BAND_ABG_WITH_DFS = 7,             // 2.4 GHz + 5 GHz with DFS
} wifi_band;

#define MAX_CHANNELS                16
#define MAX_BUCKETS                 16
#define MAX_HOTLIST_APS             128
#define MAX_SIGNIFICANT_CHANGE_APS  64
#define MAX_PNO_SSID                128;

wifi_error wifi_get_valid_channels(wifi_interface_handle handle,
        int band, int max_channels, wifi_channel *channels, int *num_channels);

typedef struct {
    int max_scan_cache_size;                 // total space allocated for scan (in bytes)
    int max_scan_buckets;                    // maximum number of channel buckets
    int max_ap_cache_per_scan;               // maximum number of APs that can be stored per scan
    int max_rssi_sample_size;                // number of RSSI samples used for averaging RSSI
    int max_scan_reporting_threshold;        // max possible report_threshold as described
                                             // in wifi_scan_cmd_params
    int max_hotlist_aps;                     // maximum number of entries for hotlist APs
    int max_significant_wifi_change_aps;     // maximum number of entries for
                                             // significant wifi change APs
    int max_bssid_history_entries;           // number of BSSID/RSSI entries that device can hold
} wifi_gscan_capabilities;

wifi_error wifi_get_gscan_capabilities(wifi_interface_handle handle,
        wifi_gscan_capabilities *capabilities);

typedef enum {
   WIFI_SCAN_BUFFER_FULL,
   WIFI_SCAN_COMPLETE,
} wifi_scan_event;


/* Format of information elements found in the beacon */
typedef struct {
    byte id;                            // element identifier
    byte len;                           // number of bytes to follow
    byte data[];
} wifi_information_element;

typedef struct {
    wifi_timestamp ts;                  // time since boot (in microsecond) when the result was
                                        // retrieved
    char ssid[32+1];                    // null terminated
    mac_addr bssid;
    wifi_channel channel;               // channel frequency in MHz
    wifi_rssi rssi;                     // in db
    wifi_timespan rtt;                  // in nanoseconds
    wifi_timespan rtt_sd;               // standard deviation in rtt
    unsigned short beacon_period;       // period advertised in the beacon
    unsigned short capability;          // capabilities advertised in the beacon
    unsigned int ie_length;             // size of the ie_data blob
    char         ie_data[1];            // blob of all the information elements found in the
                                        // beacon; this data should be a packed list of
                                        // wifi_information_element objects, one after the other.
    // other fields
} wifi_scan_result;

typedef struct {
    /* reported when report_threshold is reached in scan cache */
    void (*on_scan_results_available) (wifi_request_id id, unsigned num_results_available);

    /* reported when each probe response is received, if report_events
     * enabled in wifi_scan_cmd_params */
    void (*on_full_scan_result) (wifi_request_id id, wifi_scan_result *result);

    /* optional event - indicates progress of scanning statemachine */
    void (*on_scan_event) (wifi_scan_event event, unsigned status);

} wifi_scan_result_handler;

typedef struct {
    wifi_channel channel;               // frequency
    int dwellTimeMs;                    // dwell time hint
    int passive;                        // 0 => active, 1 => passive scan; ignored for DFS
    /* Add channel class */
} wifi_scan_channel_spec;


typedef struct {
    int bucket;                         // bucket index, 0 based
    wifi_band band;                     // when UNSPECIFIED, use channel list
    int period;                         // desired period, in millisecond; if this is too
                                        // low, the firmware should choose to generate results as
                                        // fast as it can instead of failing the command.
                                        // for exponential backoff bucket this is the min_period
    /* report_events semantics -
     *  0 => report only when scan history is % full
     *  1 => same as 0 + report a scan completion event after scanning this bucket
     *  2 => same as 1 + forward scan results (beacons/probe responses + IEs) in real time to HAL
     *  3 => same as 2 + forward scan results (beacons/probe responses + IEs) in real time to
             supplicant as well (optional) .
     */
    byte report_events;
    int max_period; // if max_period is non zero or different than period, then this bucket is
                    // an exponential backoff bucket and the scan period will grow exponentially
                    // as per formula: actual_period(N) = period ^ (N/(step_count+1))
                    // to a maximum period of max_period
    int exponent; // for exponential back off bucket: multiplier: new_period = old_period * exponent
    int step_count; // for exponential back off bucket, number of scans performed at a given
                    // period and until the exponent is applied

    int num_channels;
    // channels to scan; these may include DFS channels
    // Note that a given channel may appear in multiple buckets
    wifi_scan_channel_spec channels[MAX_CHANNELS];
} wifi_scan_bucket_spec;

typedef struct {
    int base_period;                    // base timer period in ms
    int max_ap_per_scan;                // number of APs to store in each scan ientryn the
                                        // BSSID/RSSI history buffer (keep the highest RSSI APs)
    int report_threshold_percent;       // in %, when scan buffer is this much full, wake up AP
    int report_threshold_num_scans;     // in number of scans, wake up AP after these many scans
    int num_buckets;
    wifi_scan_bucket_spec buckets[MAX_BUCKETS];
} wifi_scan_cmd_params;

/* Start periodic GSCAN */
wifi_error wifi_start_gscan(wifi_request_id id, wifi_interface_handle iface,
        wifi_scan_cmd_params params, wifi_scan_result_handler handler);

/* Stop periodic GSCAN */
wifi_error wifi_stop_gscan(wifi_request_id id, wifi_interface_handle iface);

typedef enum {
    WIFI_SCAN_FLAG_INTERRUPTED = 1      // Indicates that scan results are not complete because
                                        // probes were not sent on some channels
} wifi_scan_flags;

/* Get the GSCAN cached scan results */
typedef struct {
    int scan_id;                        // a unique identifier for the scan unit
    int flags;                          // a bitmask with additional information about scan
    int num_results;                    // number of bssids retrieved by the scan
    wifi_scan_result *results;          // scan results - one for each bssid
} wifi_cached_scan_results;

wifi_error wifi_get_cached_gscan_results(wifi_interface_handle iface, byte flush,
        int max, wifi_cached_scan_results *results, int *num);

/* BSSID Hotlist */
typedef struct {
    void (*on_hotlist_ap_found)(wifi_request_id id,
            unsigned num_results, wifi_scan_result *results);
    void (*on_hotlist_ap_lost)(wifi_request_id id,
            unsigned num_results, wifi_scan_result *results);
} wifi_hotlist_ap_found_handler;

typedef struct {
    mac_addr  bssid;                    // AP BSSID
    wifi_rssi low;                      // low threshold
    wifi_rssi high;                     // high threshold
} ap_threshold_param;

typedef struct {
    int lost_ap_sample_size;
    int num_ap;                                 // number of hotlist APs
    ap_threshold_param ap[MAX_HOTLIST_APS];     // hotlist APs
} wifi_bssid_hotlist_params;

/* Set the BSSID Hotlist */
wifi_error wifi_set_bssid_hotlist(wifi_request_id id, wifi_interface_handle iface,
        wifi_bssid_hotlist_params params, wifi_hotlist_ap_found_handler handler);

/* Clear the BSSID Hotlist */
wifi_error wifi_reset_bssid_hotlist(wifi_request_id id, wifi_interface_handle iface);

/* Significant wifi change */
typedef struct {
    mac_addr bssid;                     // BSSID
    wifi_channel channel;               // channel frequency in MHz
    int num_rssi;                       // number of rssi samples
    wifi_rssi rssi[];                   // RSSI history in db
} wifi_significant_change_result;

typedef struct {
    void (*on_significant_change)(wifi_request_id id,
            unsigned num_results, wifi_significant_change_result **results);
} wifi_significant_change_handler;

// The sample size parameters in the wifi_significant_change_params structure
// represent the number of occurence of a g-scan where the BSSID was seen and RSSI was
// collected for that BSSID, or, the BSSID was expected to be seen and didn't.
// for instance: lost_ap_sample_size : number of time a g-scan was performed on the
// channel the BSSID was seen last, and the BSSID was not seen during those g-scans
typedef struct {
    int rssi_sample_size;               // number of samples for averaging RSSI
    int lost_ap_sample_size;            // number of samples to confirm AP loss
    int min_breaching;                  // number of APs breaching threshold
    int num_ap;                         // max 64
    ap_threshold_param ap[MAX_SIGNIFICANT_CHANGE_APS];
} wifi_significant_change_params;

/* Set the Signifcant AP change list */
wifi_error wifi_set_significant_change_handler(wifi_request_id id, wifi_interface_handle iface,
        wifi_significant_change_params params, wifi_significant_change_handler handler);

/* Clear the Signifcant AP change list */
wifi_error wifi_reset_significant_change_handler(wifi_request_id id, wifi_interface_handle iface);

/* Random MAC OUI for PNO */
wifi_error wifi_set_scanning_mac_oui(wifi_interface_handle handle, oui scan_oui);


#define WIFI_PNO_FLAG_DIRECTED_SCAN = 1 // whether directed scan needs to be performed (for hidden SSIDs)
#define WIFI_PNO_FLAG_HASH_PROVIDED = 2 // whether a crc32 hash of the ssid is provided instead of the ssid

// Code for matching the beacon AUTH IE - additional codes TBD
#define WIFI_PNO_AUTH_CODE_OPEN  1 // open
#define WIFI_PNO_AUTH_CODE_PSK   2 // WPA_PSK or WPA2PSK
#define WIFI_PNO_AUTH_CODE_EAPOL 4 // any EAPOL

// Enhanced PNO:
// for each network framework will either specify a ssid or a crc32
// if ssid is specified (i.e. ssid[0] != 0) then crc32 field shall be ignored.
// A PNO network shall be reported once, that is, once a network is reported by firmware
// its entry shall be marked as "done" until framework call wifi_set_epno_list.
 // Calling wifi_set_epno_list shall reset the "done" status of pno networks in firmware.
typedef struct {
    char ssid[32];
    char rssi_threshold; // threshold for considering this SSID as found
    char flags;
    int crc32;  // crc32 of the SSID, this allows for memory size optimization
                // i.e. not passing the whole SSID
                // in firmware and instead storing a shorter string
    char auth_bit_field; // auth bitfield for matching WPA IE
} wifi_epno_network;

/* PNO list */
typedef struct {
    int num_networks;                                // number of SSIDs
    wifi_epno_network networks[];     // PNO networks
} wifi_epno_params;

typedef struct {
    int network_index; // index of the network found in the pno list
    char ssid[32+1]; // null terminated
    wifi_channel channel;
    int rssi;
} wifi_epno_result;


typedef struct {
    // on results
    void (*on_network_found)(wifi_request_id id,
            unsigned num_results, wifi_epno_result *results);
} wifi_epno_handler;


/* Set the PNO list */
wifi_error wifi_set_epno_list(wifi_request_id id, wifi_interface_handle iface,
        int num_networks, wifi_epno_network *networks, wifi_epno_handler handler);


/* SSID white list */
/* Note that this feature requires firmware to be able to indicate to kernel sme and wpa_supplicant
 * that the SSID of the network has changed
 * and thus requires further changed in cfg80211 stack, for instance, the below function would change:

 void __cfg80211_roamed(struct wireless_dev *wdev,
 		       struct cfg80211_bss *bss,
 		       const u8 *req_ie, size_t req_ie_len,
 		       const u8 *resp_ie, size_t resp_ie_len)
 * when firmware roam to a new SSID the corresponding link layer stats info need to be updated:
     struct wifi_interface_link_layer_info;
 */
typedef struct {
    char ssid[32+1]; // null terminated
} wifi_ssid;

wifi_error wifi_set_ssid_white_list(wifi_request_id id, wifi_interface_handle iface,
        int num_networks, wifi_ssid *ssids);

typedef struct {
    int max_number_epno_networks_by_crc32; //max number of epno entries if crc32 is specified
    int max_number_epno_networks_by_ssid;  //max number of epno entries if ssid is specified
    int max_number_of_white_losted_ssid;   //max number of white listed SSIDs, M target is 2 to 4 */
} wifi_roam_autojoin_offload_capabilities;

wifi_error wifi_get_roam_autojoin_offload_capabilities(wifi_interface_handle handle,
        wifi_roam_autojoin_offload_capabilities *capabilities);

#endif

