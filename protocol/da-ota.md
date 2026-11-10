# DA14585 OTA admission

DA keeps the 64-byte native pQ header, DAV1 layout, epoch admission, CRC,
single-slot transport and 96-byte App trailer budget. CRC and unsigned hashes
detect corruption; they do not authenticate a publisher.

## Recovery policy

Read SUOTA VERSION on the same Recovery connection before the first transfer
command. The exact nine-byte value is `0d 49 4b 01 01 53 50 01 PP`.
The original five bytes retain native version 13 and safe-resume schema1/bit1.
`53 50` is the policy marker, the following byte is schema1, and `PP` is
`00` (optional) or `01` (required). Only an exact supported value establishes
the policy. Old five-byte values, absent data, unknown versions, extra bytes
and invalid values mean required. This attribute is read-only.

Source builds default to optional. `-OtaSignaturePolicy required` explicitly
selects mandatory publisher authentication. Both profiles retain and enforce
signature verification for signed images. Changing an installed policy needs
a Recovery replacement; App OTA alone cannot change it.

## Image formats

The little-endian native header contains payload size at4, CRC32 at8 and
four-component ASCII version at12 (16-byte field). Its24-byte preflight record
at40 is six uint32 values: `IKP1` (`0x31504b49`), raw App size, epoch, board
`DAXB` (`0x42584144`), layout `DAV1` (`0x31564144`) and verifier identity.
Payload is raw App followed by a96-byte trailer. Its32-byte prefix is
`<IBBHIIIIII`: magic, schema1, algorithm, trailer size96, raw size, epoch,
board, layout, verifier and release version (`major*10000+minor*100+patch`).
Header and trailer contexts must match.

| Format | Magic | Algorithm | Verifier | Final64 bytes |
| --- | --- | --- | --- | --- |
| Publisher signed | IKS1 /0x31534b49 |1 | DAK1 /0x314b4144 | P-256 R/S,32B each, big endian, low-S |
| Explicitly unsigned | IKU1 /0x31554b49 |0 | DAU1 /0x31554144 | SHA256(raw+prefix),32B; then32 zero bytes |

Required accepts only valid signed images. Optional accepts valid signed or
valid explicitly unsigned images. The format is selected by the preflight
verifier and must match the trailer; failed signed verification never retries
as unsigned. No image supplies a trusted public key. All existing target,
layout, epoch, size, CRC, metadata and safe transport checks still apply.

`tools/build/package_ota.py` creates the unsigned format using Python's standard
library. It does not use keys, the SDK, BLE or a programmer. Raw maximum95648B,
payload maximum95744B and complete OTA maximum95808B apply to both formats.
