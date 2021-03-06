
#include <stdint.h>
#include <string.h>
#include "nrf_gpio.h"
#include "our_service.h"
#include "ble_srv_common.h"
#include "app_error.h"

/**@brief Function for handling BLE GATTS EVENTS
 * 
 * This function prints out data that is received when you try to write to your characteristic or CCCD. 
 * In general it is a bad idea to to so much printf stuff and UART transfer inside the BLE handler,
 * but this is just for demonstrate purposes.
 *
 * @param[in]   p_our_service        Our Service structure.
 * @param[in]   p_ble_evt            BLE event passed from BLE stack
 *
 */
static void on_ble_write(ble_os_t * p_our_service, ble_evt_t * p_ble_evt)
{
    // Declare buffer variable to hold received data. The data can only be 32 bit long.
    uint32_t data_buffer;
    // Pupulate ble_gatts_value_t structure to hold received data and metadata.
    ble_gatts_value_t rx_data;
    rx_data.len = sizeof(data_buffer);
    rx_data.offset = 0;
    rx_data.p_value = (uint8_t*)&data_buffer;
    
    // Check if write event is performed on our characteristic or the CCCD
    if(p_ble_evt->evt.gatts_evt.params.write.handle == p_our_service->char_handles.value_handle)
    {
        // Get data
        sd_ble_gatts_value_get(p_our_service->conn_handle, p_our_service->char_handles.value_handle, &rx_data);
        // Print handle and value 
        printf("Value received on handle %#06x: %#010x\r\n", p_ble_evt->evt.gatts_evt.params.write.handle, data_buffer);
    }
    else if(p_ble_evt->evt.gatts_evt.params.write.handle == p_our_service->char_handles.cccd_handle)
    {
        // Get data
        sd_ble_gatts_value_get(p_our_service->conn_handle, p_our_service->char_handles.cccd_handle, &rx_data);
        // Print handle and value 
        printf("Value received on handle %#06x: %#06x\r\n", p_ble_evt->evt.gatts_evt.params.write.handle, data_buffer);
        if(data_buffer == 0x0001)
        {
            printf("Notification enabled\r\n");
        }
        else if(data_buffer == 0x0000)
        {
            printf("Notification disabled\r\n");
        }
    }
}

// Declaration of a function that will take care of some housekeeping of ble connections related to our service and characteristic
void ble_our_service_on_ble_evt(ble_os_t * p_our_service, ble_evt_t * p_ble_evt)
{

    // 3.D Implement switch case handling BLE events related to our service. 
    switch (p_ble_evt->header.evt_id)
    {        
        case BLE_GATTS_EVT_WRITE:
            on_ble_write(p_our_service, p_ble_evt);
            break;
        case BLE_GAP_EVT_CONNECTED:
            p_our_service->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            break;
        case BLE_GAP_EVT_DISCONNECTED:
            p_our_service->conn_handle = BLE_CONN_HANDLE_INVALID;
            break;
        default:
            // No implementation needed.
            break;
    }
}

/**@brief Function for adding our new characterstic to "Our service"
 *
 * @param[in]   p_our_service        Our Service structure.
 *
 */
static uint32_t our_char_add(ble_os_t * p_our_service)
{
    uint32_t   err_code = 0; // Variable to hold return codes from library and softdevice functions
    
    // 2.A, Add a custom characteristic UUID
    ble_uuid_t          char_uuid;
    ble_uuid128_t       base_uuid = BLE_UUID_OUR_BASE_UUID;
    char_uuid.uuid      = BLE_UUID_OUR_CHARACTERISTC_UUID;
    sd_ble_uuid_vs_add(&base_uuid, &char_uuid.type);
    APP_ERROR_CHECK(err_code);
    
    // 2.F Add read/write properties to our characteristic
    ble_gatts_char_md_t char_md;
    memset(&char_md, 0, sizeof(char_md));
    char_md.char_props.read = 1;
    char_md.char_props.write = 1;

    
    // 3.A, Configuring Client Characteristic Configuration Descriptor metadata and add to char_md structure
    ble_gatts_attr_md_t cccd_md;
    memset(&cccd_md, 0, sizeof(cccd_md));
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);
    cccd_md.vloc                = BLE_GATTS_VLOC_STACK;    
    char_md.p_cccd_md           = &cccd_md;
    char_md.char_props.notify   = 1;
   
    
    // 2.B, Configure the attribute metadata
    ble_gatts_attr_md_t attr_md;
    memset(&attr_md, 0, sizeof(attr_md)); 
    attr_md.vloc        = BLE_GATTS_VLOC_STACK;   
    
    
    // 2.G, Set read/write security levels to our characteristic
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    //BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);
    
    
    // Step 2.C, Configure the characteristic value attribute
    ble_gatts_attr_t    attr_char_value;
    memset(&attr_char_value, 0, sizeof(attr_char_value));        
    attr_char_value.p_uuid      = &char_uuid;
    attr_char_value.p_attr_md   = &attr_md;
    
    // 2.H, Set characteristic length in number of bytes
    attr_char_value.max_len     = 20;
    attr_char_value.init_len    = 20;
    uint8_t value[20]            = {0x12,0x34,0x56,0x78,0x12,0x34,0xBE,0xEF,0xBE,0xEF,0x56,0x78,0x12,0x34,0xBE,0xEF,0xBE,0xEF,0x56,0x78};
    attr_char_value.p_value     = value;

    // 2.E, Add our new characteristic to the service
    err_code = sd_ble_gatts_characteristic_add(p_our_service->service_handle,
                                       &char_md,
                                       &attr_char_value,
                                       &p_our_service->char_handles);
    APP_ERROR_CHECK(err_code);
			
    /* Removed printf header, no printing now
    printf("\r\nService handle: %#x\n\r", p_our_service->service_handle);
    printf("Char value handle: %#x\r\n", p_our_service->char_handles.value_handle);
    printf("Char cccd handle: %#x\r\n\r\n", p_our_service->char_handles.cccd_handle);
    */
			
    return NRF_SUCCESS;
}


/**@brief Function for initiating our new service.
 *
 * @param[in]   p_our_service        Our Service structure.
 *
 */
void our_service_init(ble_os_t * p_our_service)
{
    uint32_t   err_code; // Variable to hold return codes from library and softdevice functions

    // FROM_SERVICE_TUTORIAL: Declare 16-bit service and 128-bit base UUIDs and add them to the BLE stack
    ble_uuid_t        service_uuid;
    ble_uuid128_t     base_uuid = BLE_UUID_OUR_BASE_UUID;
    service_uuid.uuid = BLE_UUID_OUR_SERVICE_UUID;
    err_code = sd_ble_uuid_vs_add(&base_uuid, &service_uuid.type);
    APP_ERROR_CHECK(err_code);    
    
    // 3.B, Set our service connection handle to default value. I.e. an invalid handle since we are not yet in a connection.
    p_our_service->conn_handle = BLE_CONN_HANDLE_INVALID;

    // FROM_SERVICE_TUTORIAL: Add our service
    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                        &service_uuid,
                                        &p_our_service->service_handle);
    
    APP_ERROR_CHECK(err_code);
    
    // Call the function our_char_add() to add our new characteristic to the service. 
    our_char_add(p_our_service);
}

// Function to be called when updating characteristic value
void ecg_characteristic_update(ble_os_t *p_our_service, int32_t ecg_data_value[20])
{
    // 3.E, Update characteristic value
    if (p_our_service->conn_handle != BLE_CONN_HANDLE_INVALID)
    {
        uint16_t               len = 4;
        ble_gatts_hvx_params_t hvx_params;
        memset(&hvx_params, 0, sizeof(hvx_params));

        hvx_params.handle = p_our_service->char_handles.value_handle;
        hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
        hvx_params.offset = 0;
        hvx_params.p_len  = &len;
        hvx_params.p_data = (uint8_t*)ecg_data_value;  

        sd_ble_gatts_hvx(p_our_service->conn_handle, &hvx_params);
    }   
}
