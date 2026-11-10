#include "inktag_da_ota_security.h"

#include <stddef.h>
#include <string.h>

#include "arch_wdg.h"
#include "datasheet.h"
#include "inktag_da_health.h"
#include "inktag_da_ota_policy.h"
#include "inktag_da_ota_public_key.h"
#include "inktag_da_recovery.h"
#include "inktag_firmware_signature.h"
#include "inktag_power.h"
#include "rwip.h"
#include "sha2.h"
#include "spi_flash.h"
#include "uECC.h"

void uECC_vli_mult(uint32_t *result, const uint32_t *left,
                   const uint32_t *right, int8_t words);
void PATCHED_uECC_vli_mult(uint32_t *result, const uint32_t *left,
                           const uint32_t *right, int8_t words);

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint32_t application_size;
    uint32_t compatibility_epoch;
    uint32_t board_signature;
    uint32_t layout_signature;
    uint32_t verifier_signature;
} da_pq_preflight_t;

typedef char da_pq_preflight_size_check[
    sizeof(da_pq_preflight_t) == 24u ? 1 : -1];

static da_pq_preflight_t s_preflight;
static bool s_preflight_valid;
static bool s_reject_pending;
static inktag_ota_reject_info_t s_reject;

/* Keep verification workspaces out of the small Recovery call stack. */
static inktag_firmware_signature_trailer_t s_trailer;
static sha256_ctx s_sha;
static uint8_t s_digest[SHA256_DIGEST_SIZE];
static uint8_t s_hash_chunk[SHA256_BLOCK_SIZE];

static uint32_t read_u32_le(const uint8_t *data)
{
    return (uint32_t)data[0] | ((uint32_t)data[1] << 8) |
           ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
}

void inktag_da_ota_reject_clear(void)
{
    s_reject_pending = false;
    memset(&s_reject, 0, sizeof(s_reject));
}

bool inktag_da_ota_reject_pending(void)
{
    return s_reject_pending;
}

void inktag_da_ota_reject_set(
    inktag_ota_reject_reason_t reason, inktag_ota_reject_stage_t stage,
    uint8_t flags, uint32_t detail, uint16_t sampled_mv,
    uint16_t threshold_mv)
{
    s_reject.reason = reason;
    s_reject.stage = stage;
    s_reject.flags = flags;
    s_reject.detail = detail;
    s_reject.sampled_mv = sampled_mv;
    s_reject.threshold_mv = threshold_mv;
    s_reject_pending = true;
}

void inktag_da_ota_reject_set_default(
    inktag_ota_reject_reason_t reason, inktag_ota_reject_stage_t stage,
    uint8_t flags, uint32_t detail)
{
    if (!s_reject_pending)
        inktag_da_ota_reject_set(
            reason, stage, flags, detail, 0u, 0u);
}

void inktag_da_ota_reject_power(
    inktag_ota_reject_stage_t stage, uint32_t detail,
    uint16_t threshold_mv, bool detail_is_address)
{
    bool valid = false;
    uint16_t sampled_mv =
        inktag_da_health_last_operation_voltage_mv(&valid);
    uint8_t flags = INKTAG_OTA_REJECT_FLAG_THRESHOLD_MV_VALID |
        (detail_is_address ? INKTAG_OTA_REJECT_FLAG_DETAIL_IS_ADDRESS :
                             INKTAG_OTA_REJECT_FLAG_DETAIL_IS_OFFSET);
    if (valid) flags |= INKTAG_OTA_REJECT_FLAG_SAMPLED_MV_VALID;
    inktag_da_ota_reject_set(
        valid ? INKTAG_OTA_REJECT_REASON_VOLTAGE_BELOW_MIN :
                INKTAG_OTA_REJECT_REASON_POWER_SAMPLE_INVALID,
        stage, flags, detail, sampled_mv, threshold_mv);
}

uint8_t inktag_da_ota_status_encode(
    uint8_t status, uint8_t *output, uint8_t capacity)
{
    uint8_t length;
    if (output == NULL || capacity == 0u) return 0u;
    if (!s_reject_pending) {
        output[0] = status;
        return 1u;
    }
    length = inktag_ota_reject_encode(output, capacity, status, &s_reject);
    inktag_da_ota_reject_clear();
    if (length == 0u) {
        output[0] = status;
        return 1u;
    }
    return length;
}

bool inktag_da_ota_preflight(const uint8_t *pq_header, uint32_t length)
{
    da_pq_preflight_t candidate;
    uint32_t code_size;
    inktag_recovery_result_t epoch_result;
    s_preflight_valid = false;
    inktag_da_ota_reject_clear();
    if (pq_header == NULL || length < INKTAG_DA_PQ_HEADER_BYTES) {
        inktag_da_ota_reject_set(
            INKTAG_OTA_REJECT_REASON_IMAGE_INFO_INVALID,
            INKTAG_OTA_REJECT_STAGE_PROGRAM_START, 0u, length, 0u, 0u);
        return false;
    }
    memcpy(&candidate, pq_header + INKTAG_DA_PQ_PREFLIGHT_OFFSET,
           sizeof(candidate));
    code_size = read_u32_le(&pq_header[4]);
    if (candidate.magic != INKTAG_DA_PQ_PREFLIGHT_MAGIC ||
        candidate.application_size == 0u ||
        candidate.application_size > INKTAG_DA_APPLICATION_RAW_LIMIT -
                INKTAG_FIRMWARE_SIGNATURE_TRAILER_BYTES ||
        candidate.application_size +
                INKTAG_FIRMWARE_SIGNATURE_TRAILER_BYTES != code_size ||
        code_size > INKTAG_DA_APPLICATION_RAW_LIMIT ||
        candidate.compatibility_epoch >
                INKTAG_RECOVERY_COMPATIBILITY_EPOCH_MAX ||
        candidate.board_signature != INKTAG_DA_OTA_BOARD_SIGNATURE ||
        candidate.layout_signature != INKTAG_DA_OTA_LAYOUT_SIGNATURE ||
        (candidate.verifier_signature != INKTAG_DA_OTA_VERIFIER_SIGNATURE
#if !INKTAG_DA_OTA_SIGNATURE_REQUIRED
         && candidate.verifier_signature != INKTAG_DA_OTA_UNSIGNED_VERIFIER
#endif
         )) {
        inktag_da_ota_reject_set(
            INKTAG_OTA_REJECT_REASON_IMAGE_INFO_INVALID,
            INKTAG_OTA_REJECT_STAGE_PROGRAM_START, 0u, code_size, 0u, 0u);
        return false;
    }
    epoch_result = inktag_da_recovery_check_epoch(
        candidate.compatibility_epoch);
    /* The common journal adapter parks external Flash at transaction end;
     * this preflight is called from an already active SUOTA session. */
    (void)spi_flash_release_from_power_down();
    if (epoch_result != INKTAG_RECOVERY_OK) {
        inktag_da_ota_reject_set(
            epoch_result == INKTAG_RECOVERY_ROLLBACK_ERROR ?
                INKTAG_OTA_REJECT_REASON_IMAGE_INFO_INVALID :
                INKTAG_OTA_REJECT_REASON_INTERNAL,
            epoch_result == INKTAG_RECOVERY_ROLLBACK_ERROR ?
                INKTAG_OTA_REJECT_STAGE_PROGRAM_START :
                INKTAG_OTA_REJECT_STAGE_METADATA_COMMIT,
            0u, candidate.compatibility_epoch, 0u, 0u);
        return false;
    }
    s_preflight = candidate;
    s_preflight_valid = true;
    return true;
}

static bool scalar_is_zero(const uint8_t *scalar)
{
    uint8_t combined = 0u;
    for (uint8_t i = 0u; i < 32u; ++i) combined |= scalar[i];
    return combined == 0u;
}

static bool scalar_is_low_s(const uint8_t *scalar)
{
    static const uint8_t half_order[32] = {
        0x7F,0xFF,0xFF,0xFF,0x80,0x00,0x00,0x00,
        0x7F,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
        0xDE,0x73,0x7D,0x56,0xD3,0x8B,0xCF,0x42,
        0x7C,0xF5,0xD6,0x56,0x1E,0xBF,0xF3,0x7A,
    };
    return memcmp(scalar, half_order, sizeof(half_order)) <= 0;
}

static void ecc_verify_noop_schedule(void)
{
}

static bool verify_p256(const uint8_t digest[32], const uint8_t signature[64])
{
    volatile uint32_t *schedule_destination =
        (volatile uint32_t *)0x07FC0104u;
    volatile uint32_t *multiply_destination =
        (volatile uint32_t *)0x07FC0108u;
    uint32_t old_schedule_address = GetWord32(PATCH_ADDR17_REG);
    uint32_t old_multiply_address = GetWord32(PATCH_ADDR18_REG);
    uint32_t old_schedule_destination = *schedule_destination;
    uint32_t old_multiply_destination = *multiply_destination;
    uint32_t old_valid = GetWord32(PATCH_VALID_REG);
    int verified;

    /* Renesas' SDK6 ECC_Verify_Image() installs both patches together.  ROM
     * micro-ecc calls rwip_schedule() internally, but re-entering the BLE
     * scheduler from the SUOTA GATT write callback deadlocks IMAGE_END before
     * the valid flag can be committed.  Slots 17/18 are saved and restored so
     * the ordinary SDK patch table remains intact outside this bounded call. */
    SetWord32(PATCH_VALID_REG, old_valid & ~((1u << 17) | (1u << 18)));
    SetWord32(PATCH_ADDR17_REG, (uint32_t)(uintptr_t)rwip_schedule);
    *schedule_destination = (uint32_t)(uintptr_t)ecc_verify_noop_schedule;
    SetWord32(PATCH_ADDR18_REG, (uint32_t)(uintptr_t)uECC_vli_mult);
    *multiply_destination = (uint32_t)(uintptr_t)PATCHED_uECC_vli_mult;
    SetWord32(PATCH_VALID_REG, old_valid | (1u << 17) | (1u << 18));
    /* The ROM P-256 verifier can exceed the normal ~2.05 s watchdog window
     * on DA14585. Unlike the chunked SHA pass above, it offers no bounded
     * point where Recovery can reload the watchdog. Freeze only for this
     * indivisible verification, then restore the normal watchdog contract
     * before committing any metadata. */
    wdg_freeze();
    verified = uECC_verify(k_inktag_da_ota_public_key, digest, 32u,
                           signature, uECC_secp256r1());
    wdg_reload(WATCHDOG_DEFAULT_PERIOD);
    wdg_resume();
    SetWord32(PATCH_VALID_REG, old_valid & ~((1u << 17) | (1u << 18)));
    SetWord32(PATCH_ADDR17_REG, old_schedule_address);
    *schedule_destination = old_schedule_destination;
    SetWord32(PATCH_ADDR18_REG, old_multiply_address);
    *multiply_destination = old_multiply_destination;
    SetWord32(PATCH_VALID_REG, old_valid);
    return verified == 1;
}

static bool flash_read_exact(uint32_t address, void *data, uint32_t length)
{
    uint32_t actual = 0u;
    return spi_flash_read_data(data, address, length, &actual) ==
               SPI_FLASH_ERR_OK && actual == length;
}

bool inktag_da_ota_verify_and_arm(void)
{
    uint8_t header[INKTAG_DA_PQ_HEADER_BYTES];
    bool is_unsigned;
    bool verified;
    uint32_t address;
    uint32_t remaining;
    inktag_recovery_result_t arm_result;
    if (!s_preflight_valid ||
        !flash_read_exact(INKTAG_DA_APPLICATION_START, header,
                          sizeof(header))) {
        inktag_da_ota_reject_set(
            INKTAG_OTA_REJECT_REASON_IMAGE_INFO_INVALID,
            INKTAG_OTA_REJECT_STAGE_IMAGE_VERIFY,
            INKTAG_OTA_REJECT_FLAG_DETAIL_IS_ADDRESS,
            INKTAG_DA_APPLICATION_START, 0u, 0u);
        return false;
    }
    is_unsigned = s_preflight.verifier_signature == INKTAG_DA_OTA_UNSIGNED_VERIFIER;
    address = INKTAG_DA_APPLICATION_START + INKTAG_DA_PQ_HEADER_BYTES +
        s_preflight.application_size;
    if (read_u32_le(&header[4]) != s_preflight.application_size +
            INKTAG_FIRMWARE_SIGNATURE_TRAILER_BYTES ||
        memcmp(header + INKTAG_DA_PQ_PREFLIGHT_OFFSET, &s_preflight,
               sizeof(s_preflight)) != 0 ||
        !flash_read_exact(address, &s_trailer, sizeof(s_trailer)) ||
        s_trailer.magic != (is_unsigned ? INKTAG_DA_OTA_UNSIGNED_MAGIC :
                                          INKTAG_FIRMWARE_SIGNATURE_MAGIC) ||
        s_trailer.version != INKTAG_FIRMWARE_SIGNATURE_VERSION ||
        s_trailer.algorithm != (is_unsigned ? INKTAG_DA_OTA_UNSIGNED_ALGORITHM :
                                              INKTAG_FIRMWARE_SIGNATURE_ECDSA_P256) ||
        s_trailer.trailer_size != INKTAG_FIRMWARE_SIGNATURE_TRAILER_BYTES ||
        s_trailer.application_size != s_preflight.application_size ||
        s_trailer.compatibility_epoch != s_preflight.compatibility_epoch ||
        s_trailer.signed_context.board_signature != s_preflight.board_signature ||
        s_trailer.signed_context.layout_signature != s_preflight.layout_signature ||
        s_trailer.signed_context.verifier_signature != s_preflight.verifier_signature ||
        (!is_unsigned && (scalar_is_zero(s_trailer.signature) ||
        scalar_is_zero(&s_trailer.signature[32]) ||
        !scalar_is_low_s(&s_trailer.signature[32])))) {
        inktag_da_ota_reject_set(
            INKTAG_OTA_REJECT_REASON_CHECKSUM_OR_SIGNATURE_FAILED,
            INKTAG_OTA_REJECT_STAGE_SIGNATURE_VERIFY,
            INKTAG_OTA_REJECT_FLAG_DETAIL_IS_ADDRESS, address, 0u, 0u);
        return false;
    }

    sha256_init(&s_sha);
    address = INKTAG_DA_APPLICATION_START + INKTAG_DA_PQ_HEADER_BYTES;
    remaining = s_preflight.application_size +
        INKTAG_FIRMWARE_SIGNATURE_PREFIX_BYTES;
    while (remaining != 0u) {
        uint32_t take = remaining > sizeof(s_hash_chunk) ?
            sizeof(s_hash_chunk) : remaining;
        if (!flash_read_exact(address, s_hash_chunk, take)) {
            inktag_da_ota_reject_set(
                INKTAG_OTA_REJECT_REASON_INTERNAL,
                INKTAG_OTA_REJECT_STAGE_SIGNATURE_VERIFY,
                INKTAG_OTA_REJECT_FLAG_DETAIL_IS_ADDRESS,
                address, 0u, 0u);
            return false;
        }
        sha256_update(&s_sha, s_hash_chunk, take);
        address += take;
        remaining -= take;
        wdg_reload(WATCHDOG_DEFAULT_PERIOD);
        if ((GetBits16(CLK_RADIO_REG, BLE_ENABLE) == 1) &&
            (GetBits32(BLE_DEEPSLCNTL_REG, DEEP_SLEEP_STAT) == 0))
            rwip_schedule();
    }
    sha256_final(&s_sha, s_digest);
    /* The explicit preflight context selects exactly one format. Never retry
     * a failed signed image as unsigned, including in the optional profile. */
    verified = false;
    if (!is_unsigned) {
        verified = verify_p256(s_digest, s_trailer.signature);
    }
#if !INKTAG_DA_OTA_SIGNATURE_REQUIRED
    else {
        uint8_t reserved = 0u;
        for (uint8_t i = 32u; i < 64u; ++i) reserved |= s_trailer.signature[i];
        verified = reserved == 0u &&
            memcmp(s_digest, s_trailer.signature, sizeof(s_digest)) == 0;
    }
#endif
    if (!verified) {
        inktag_da_ota_reject_set(
            INKTAG_OTA_REJECT_REASON_CHECKSUM_OR_SIGNATURE_FAILED,
            INKTAG_OTA_REJECT_STAGE_SIGNATURE_VERIFY,
            INKTAG_OTA_REJECT_FLAG_DETAIL_IS_ADDRESS,
            INKTAG_DA_APPLICATION_START, 0u, 0u);
        return false;
    }
    arm_result = inktag_da_recovery_arm_epoch(
        s_trailer.compatibility_epoch);
    if (arm_result != INKTAG_RECOVERY_OK) {
        inktag_da_ota_reject_set(
            arm_result == INKTAG_RECOVERY_ROLLBACK_ERROR ?
                INKTAG_OTA_REJECT_REASON_IMAGE_INFO_INVALID :
                INKTAG_OTA_REJECT_REASON_INTERNAL,
            INKTAG_OTA_REJECT_STAGE_METADATA_COMMIT, 0u,
            s_trailer.compatibility_epoch, 0u, 0u);
        return false;
    }
    if (!inktag_da_recovery_release_request()) {
        inktag_da_ota_reject_set(
            INKTAG_OTA_REJECT_REASON_INTERNAL,
            INKTAG_OTA_REJECT_STAGE_METADATA_COMMIT,
            INKTAG_OTA_REJECT_FLAG_DETAIL_IS_ADDRESS,
            INKTAG_DA_RECOVERY_REQUEST_ADDRESS, 0u, 0u);
        return false;
    }
    s_preflight_valid = false;
    return true;
}
