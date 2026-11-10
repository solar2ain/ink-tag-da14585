#ifndef INKTAG_DA_OTA_POLICY_H
#define INKTAG_DA_OTA_POLICY_H

/* Signed OTA policy for the common DA14585 AESL application. Release versions
 * identify builds but are not an anti-rollback boundary. Increment the epoch
 * only when an older signed image is incompatible with durable state or the
 * fixed Recovery contract. */
#define INKTAG_DA_OTA_BOARD_ID "da14585_aesl"
#define INKTAG_DA_OTA_LAYOUT_ID "layout-v1"
#define INKTAG_DA_OTA_BOARD_SIGNATURE      0x42584144u /* "DAXB" */
#define INKTAG_DA_OTA_LAYOUT_SIGNATURE     0x31564144u /* "DAV1" */
#define INKTAG_DA_OTA_VERIFIER_SIGNATURE   0x314B4144u /* "DAK1" */
#define INKTAG_DA_OTA_COMPATIBILITY_EPOCH  1u

/* Source builds admit explicitly unsigned, hash-checked applications. Official
 * production builds explicitly select 1 in their compiler inputs. Signed IKS1
 * images always require a valid publisher signature in either profile. */
#ifndef INKTAG_DA_OTA_SIGNATURE_REQUIRED
#define INKTAG_DA_OTA_SIGNATURE_REQUIRED 0
#endif
#if INKTAG_DA_OTA_SIGNATURE_REQUIRED != 0 && INKTAG_DA_OTA_SIGNATURE_REQUIRED != 1
#error "INKTAG_DA_OTA_SIGNATURE_REQUIRED must be 0 or 1"
#endif
#define INKTAG_DA_OTA_UNSIGNED_MAGIC      0x31554B49u /* "IKU1" */
#define INKTAG_DA_OTA_UNSIGNED_VERIFIER   0x31554144u /* "DAU1" */
#define INKTAG_DA_OTA_UNSIGNED_ALGORITHM  0u

#endif
