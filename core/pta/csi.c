#include <kernel/pseudo_ta.h>
#include <pta_csi.h>

#include <tee_api_types.h>
#include <mm/core_mmu.h>
#include <mm/core_memprot.h>
#include <unistd.h>


#define PTA_NAME "csi.pta"


#define CSI_PHYSICAL_ADDR_START     0x09000000
#define CSI_PHYSICAL_ADDR_SIZE      0x00800000


TEE_Result physical_memory_address_to_pointer(size_t physical_addr, void** virtual_addr) {
    void* vaddr;

    vaddr = core_mmu_add_mapping(MEM_AREA_IO_SEC, CSI_PHYSICAL_ADDR_START, CSI_PHYSICAL_ADDR_SIZE);

    if (vaddr == NULL)
        vaddr = phys_to_virt(physical_addr, MEM_AREA_IO_SEC, 1);

    if (vaddr == NULL)
        return TEE_ERROR_OUT_OF_MEMORY;

    *virtual_addr = vaddr;

    return TEE_SUCCESS;
}


/* available == 0  <==>  data is available */
TEE_Result check_if_response_available(uint8_t* available, uint8_t* return_reason, uint32_t* num_samples_collected) {
    TEE_Result res;
    uint8_t* base;

    res = physical_memory_address_to_pointer(CSI_PHYSICAL_ADDR_START, (void**) &base);
    if (res != TEE_SUCCESS)
        return res;

    if ((base[0] & 0b10) > 0) {
        *available = true;
        *return_reason = base[7];
        *num_samples_collected = (base[11] << 24) | (base[10] << 16) | (base[9] << 8) | base[8];
    } else {
        *available = false;
    }

    return TEE_SUCCESS;
}


TEE_Result read_data(uint8_t* buffer, uint32_t bytes_to_read, uint32_t read_offset, uint32_t* actually_read) {
    TEE_Result res;
    uint8_t* base;

    uint32_t num_bytes_collected;
    *actually_read = 0;

    res = physical_memory_address_to_pointer(CSI_PHYSICAL_ADDR_START, (void**) &base);
    if (res != TEE_SUCCESS)
        return res;

    for (uint32_t i = 0; i < bytes_to_read; i++) {
        buffer[i] = base[312 + read_offset + i];
    }

    *actually_read = bytes_to_read;

    return TEE_SUCCESS;
}


TEE_Result set_mac_filter(uint8_t* mac_addrs, uint8_t num_macs) {
    TEE_Result res;
    uint8_t* base;

    res = physical_memory_address_to_pointer(CSI_PHYSICAL_ADDR_START, (void**) &base);
    if (res != TEE_SUCCESS)
        return res;

    for (uint16_t i = 0; i < num_macs * 6; i++) {
        base[12 + i] = mac_addrs[i];
    }

    base[6] = num_macs;

    base[0] |= 0b10000000; // set most significant bit;

    return TEE_SUCCESS;
}


TEE_Result disable_mac_filter() {
    TEE_Result res;
    uint8_t* base;

    res = physical_memory_address_to_pointer(CSI_PHYSICAL_ADDR_START, (void**) &base);
    if (res != TEE_SUCCESS)
        return res;

    base[0] &= 0b01111111; // clear most significant bit;

    return TEE_SUCCESS;
}


TEE_Result set_recording_parameters(uint8_t wifi_channel, uint8_t wifi_channel_bandwidth, uint16_t recording_timeout, uint8_t num_samples_per_device) {
    TEE_Result res;
    uint8_t* base;

    res = physical_memory_address_to_pointer(CSI_PHYSICAL_ADDR_START, (void**) &base);
    if (res != TEE_SUCCESS)
        return res;

    base[1] = wifi_channel;
    base[2] = wifi_channel_bandwidth;

    base[3] = num_samples_per_device;

    base[4] = recording_timeout & 0x00ff;
    base[5] = (recording_timeout & 0xff00) >> 8;

    return TEE_SUCCESS;
}


TEE_Result start_query() {
    TEE_Result res;
    uint8_t* base;

    res = physical_memory_address_to_pointer(CSI_PHYSICAL_ADDR_START, (void**) &base);
    if (res != TEE_SUCCESS)
        return res;

    base[0] |= 0b1;

    return TEE_SUCCESS;
}


TEE_Result zero_all() {
    TEE_Result res;
    uint8_t* base;

    res = physical_memory_address_to_pointer(CSI_PHYSICAL_ADDR_START, (void**) &base);
    if (res != TEE_SUCCESS)
        return res;

    for (uint8_t i = 0; i < 12; i++)
        base[i] = 0;

    return TEE_SUCCESS;
}



/* ********************* */
/* COMMAND WRAPPER FUNCS */
/* ********************* */


TEE_Result command_check_if_response_available(uint32_t param_types, TEE_Param params[TEE_NUM_PARAMS]) {
    TEE_Result res;
    uint32_t exp_param_types = TEE_PARAM_TYPES(
        TEE_PARAM_TYPE_VALUE_OUTPUT,
        TEE_PARAM_TYPE_VALUE_OUTPUT,
        TEE_PARAM_TYPE_VALUE_OUTPUT,
        TEE_PARAM_TYPE_NONE
    );

    if (param_types != exp_param_types)
        return TEE_ERROR_BAD_PARAMETERS;

    uint8_t available;
    uint8_t return_reason;
    uint32_t num_bytes_collected;

    res = check_if_response_available(&available, &return_reason, &num_bytes_collected);
    if (res == TEE_SUCCESS) {
        params[0].value.a = available;
        params[1].value.a = return_reason;
        params[2].value.a = num_bytes_collected;
    }

    return res;
}


TEE_Result command_read_data(uint32_t param_types, TEE_Param params[TEE_NUM_PARAMS]) {
    TEE_Result res;
    uint32_t exp_param_types = TEE_PARAM_TYPES(
        TEE_PARAM_TYPE_MEMREF_OUTPUT,
        TEE_PARAM_TYPE_VALUE_INPUT,
        TEE_PARAM_TYPE_VALUE_OUTPUT,
        TEE_PARAM_TYPE_NONE
    );

    if (param_types != exp_param_types)
        return TEE_ERROR_BAD_PARAMETERS;

    uint32_t actually_read;
    res = read_data(params[0].memref.buffer, params[0].memref.size, params[1].value.a, &actually_read);
    params[2].value.a = res == TEE_SUCCESS ? actually_read : 0;

    return res;
}


TEE_Result command_set_mac_filter(uint32_t param_types, TEE_Param params[TEE_NUM_PARAMS]) {
    uint32_t exp_param_types = TEE_PARAM_TYPES(
        TEE_PARAM_TYPE_MEMREF_INPUT,
        TEE_PARAM_TYPE_NONE,
        TEE_PARAM_TYPE_NONE,
        TEE_PARAM_TYPE_NONE
    );

    if (param_types != exp_param_types)
        return TEE_ERROR_BAD_PARAMETERS;

    uint8_t num_macs = params[0].memref.size / 6;
    if (num_macs > 50)
        return TEE_ERROR_BAD_PARAMETERS;

    return set_mac_filter(params[0].memref.buffer, num_macs);
}


TEE_Result command_disable_mac_filter(uint32_t param_types, TEE_Param params[TEE_NUM_PARAMS]) {
    uint32_t exp_param_types = TEE_PARAM_TYPES(
        TEE_PARAM_TYPE_NONE,
        TEE_PARAM_TYPE_NONE,
        TEE_PARAM_TYPE_NONE,
        TEE_PARAM_TYPE_NONE
    );

    if (param_types != exp_param_types)
        return TEE_ERROR_BAD_PARAMETERS;

    return disable_mac_filter();
}


TEE_Result command_set_recording_parameters_and_start(uint32_t param_types, TEE_Param params[TEE_NUM_PARAMS]) {
    TEE_Result res;
    uint32_t exp_param_types = TEE_PARAM_TYPES(
        TEE_PARAM_TYPE_VALUE_INPUT,
        TEE_PARAM_TYPE_VALUE_INPUT,
        TEE_PARAM_TYPE_VALUE_INPUT,
        TEE_PARAM_TYPE_VALUE_INPUT
    );

    if (param_types != exp_param_types)
        return TEE_ERROR_BAD_PARAMETERS;

    uint8_t wifi_channel = params[0].value.a;
    uint8_t wifi_channel_bandwidth = params[1].value.a;
    uint16_t recording_timeout = params[2].value.a;
    uint8_t num_samples_per_device = params[3].value.a;

    res = set_recording_parameters(
        wifi_channel,
        wifi_channel_bandwidth,
        recording_timeout,
        num_samples_per_device
    );

    if (res != TEE_SUCCESS)
        return res;

    return start_query();
}


TEE_Result command_zero_all(uint32_t param_types, TEE_Param params[TEE_NUM_PARAMS]) {
    TEE_Result res;
    uint32_t exp_param_types = TEE_PARAM_TYPES(
        TEE_PARAM_TYPE_NONE,
        TEE_PARAM_TYPE_NONE,
        TEE_PARAM_TYPE_NONE,
        TEE_PARAM_TYPE_NONE
    );

    if (param_types != exp_param_types)
        return TEE_ERROR_BAD_PARAMETERS;

    return zero_all();
}



/* ****************** */
/* MANDATORY TA STUFF */
/* ****************** */


static TEE_Result invoke_command(void* sess_ctx, uint32_t cmd_id, uint32_t param_types, TEE_Param params[TEE_NUM_PARAMS]) {
    switch (cmd_id) {
        case PTA_CSI_CMD_CHECK_IF_RESPONSE_AVAILABLE:
            return command_check_if_response_available(param_types, params);
        case PTA_CSI_CMD_READ_DATA:
            return command_read_data(param_types, params);
        case PTA_CSI_CMD_SET_MAC_FILTER:
            return command_set_mac_filter(param_types, params);
        case PTA_CSI_CMD_DISABLE_MAC_FILTER:
            return command_disable_mac_filter(param_types, params);
        case PTA_CSI_CMD_SET_PARAMS_AND_START:
            return command_set_recording_parameters_and_start(param_types, params);
        case PTA_CSI_CMD_ZERO_ALL:
            return command_zero_all(param_types, params);
        default:
            return TEE_ERROR_NOT_SUPPORTED;
    }
}


pseudo_ta_register(
    .uuid = PTA_CSI_UUID,
    .name = PTA_NAME,
    .flags = PTA_DEFAULT_FLAGS,
    .invoke_command_entry_point = invoke_command
);
