#ifndef AWS_CLIENT_CREDENTIAL_KEYS_H
#define AWS_CLIENT_CREDENTIAL_KEYS_H

#include <stdint.h>

/*
 * PEM-encoded client certificate
 *
 * Must include the PEM header and footer:
 * "-----BEGIN CERTIFICATE-----\n"\
 * "...base64 data...\n"\
 * "-----END CERTIFICATE-----\n"
 */
#define keyCLIENT_CERTIFICATE_PEM    "-----BEGIN CERTIFICATE-----\n"\
"MIIDWTCCAkGgAwIBAgIUK5ZGOMTSz5znkHEp2vJE4oRFiBQwDQYJKoZIhvcNAQEL\n"\
"BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g\n"\
"SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTE5MDcxOTE5NTUz\n"\
"MloXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0\n"\
"ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAMaB/qTAHIsgsnGj0zfz\n"\
"F/Gw6NC9Q8Y6cazSPBNzWixN9VKU4bLTlKjrqNSDrn1ZZm1BKmCMnZx58y8V6Tth\n"\
"z5Gz6gKebqU/7VFjaPzvlGKPu+pcPz0mGxLLL93OvLl2Onl95U0w/Ez2afVJqWp7\n"\
"mrkoctZEvL8maNI/nNv4EmjyRftzVW+7HHrObPA/xqxUqeYKfc+n0paG7rDU/lyl\n"\
"kP1OIUIX/cWOohyk8kFqjTdTNlPwZdgpRAfsXrTfwdGHFouUv5enT9OBAeyGdzhc\n"\
"SeMPjJh31zG37n+CenriwIDnDqUTUdvqiQNowuu8MovC5CIQj+7ZQCcoG6UKGD6T\n"\
"R8cCAwEAAaNgMF4wHwYDVR0jBBgwFoAU2ma2TMW8a5MLtVmfpH8NosaL3OIwHQYD\n"\
"VR0OBBYEFDOHjqC4I9k3n12s7QqEgIlSGC/TMAwGA1UdEwEB/wQCMAAwDgYDVR0P\n"\
"AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQBLmjyHJVg4EooxcJ1/prAJsCRj\n"\
"fAoYE6XZ6M/962H12L/d/hdvF2eiL6L76SQTw8VDI5QeBf6IOAvnNL4V2R0tKxHH\n"\
"tUQs6s8HGeKgjVoXt4E64aFElT5NOV2sFyeU8ozqvHz+3hI0COExF32GnJPe00MC\n"\
"wVkvG0zL5j9wvTeEQBirnXRhIkWZ/cwsIvbIVleEM8qlLHsRG36yrKnsaLXoW5HO\n"\
"n2tBlaJlA8u535e1Iidq7x8lHAQ9wYJe5So7xjOYDLZD8VeTzpRhXk2Fa1wOG1pk\n"\
"a36RoKBJ49P/5ftaX2tKqP0yFIvdtcL8cwZtyd/d80/2M3vG9X4o/id/fIpu\n"\
"-----END CERTIFICATE-----\n"

/*
 * PEM-encoded issuer certificate for AWS IoT Just In Time Registration (JITR).
 * This is required if you're using JITR, since the issuer (Certificate
 * Authority) of the client certificate is used by the server for routing the
 * device's initial request. (The device client certificate must always be
 * sent as well.) For more information about JITR, see:
 *  https://docs.aws.amazon.com/iot/latest/developerguide/jit-provisioning.html,
 *  https://aws.amazon.com/blogs/iot/just-in-time-registration-of-device-certificates-on-aws-iot/.
 *
 * If you're not using JITR, set below to NULL.
 *
 * Must include the PEM header and footer:
 * "-----BEGIN CERTIFICATE-----\n"\
 * "...base64 data...\n"\
 * "-----END CERTIFICATE-----\n"
 */
#define keyJITR_DEVICE_CERTIFICATE_AUTHORITY_PEM    NULL

/*
 * PEM-encoded client private key.
 *
 * Must include the PEM header and footer:
 * "-----BEGIN RSA PRIVATE KEY-----\n"\
 * "...base64 data...\n"\
 * "-----END RSA PRIVATE KEY-----\n"
 */
#define keyCLIENT_PRIVATE_KEY_PEM "-----BEGIN RSA PRIVATE KEY-----\n"\
"MIIEowIBAAKCAQEAxoH+pMAciyCycaPTN/MX8bDo0L1DxjpxrNI8E3NaLE31UpTh\n"\
"stOUqOuo1IOufVlmbUEqYIydnHnzLxXpO2HPkbPqAp5upT/tUWNo/O+UYo+76lw/\n"\
"PSYbEssv3c68uXY6eX3lTTD8TPZp9UmpanuauShy1kS8vyZo0j+c2/gSaPJF+3NV\n"\
"b7sces5s8D/GrFSp5gp9z6fSlobusNT+XKWQ/U4hQhf9xY6iHKTyQWqNN1M2U/Bl\n"\
"2ClEB+xetN/B0YcWi5S/l6dP04EB7IZ3OFxJ4w+MmHfXMbfuf4J6euLAgOcOpRNR\n"\
"2+qJA2jC67wyi8LkIhCP7tlAJygbpQoYPpNHxwIDAQABAoIBAGN6w+02ZVeHxEVU\n"\
"70XrqkUx/gqVvsA5i7wrA3ZP6ZGoY9fFyrG+fJKGvu35K9mfksgPzzfhYB8JskgX\n"\
"d/GNW+NNwWkckqB3v+t4oSWYvskB9Q9GuV5gmVK6xrsmnsdmq3k1EVO0x8ud9Y26\n"\
"I4+8b9Z4ocwlWF2G1yL+RBKsaDV0yohaauWzy3UtqQCkAlKZ3yQpBjdyJbrxsZpm\n"\
"4pL2ItEMeTUUxJkYDwWfieU/CT3LAGQ/dbfE/qXFLLgrutCWT7NLKYNDsSndAlmT\n"\
"EizEx20SKhkdM1/QeicapVq9AYo8fdIWN0OMNI7EM+6GV1g83xSySL4pqQ29uxIz\n"\
"aWetI7ECgYEA5YJaUmG0+OHH7tC0l+iIpWLENoPalWJ4jzy7i+S6Vs3yUlFfGLIQ\n"\
"tWQBqzTfMzPOsO/skgC0LyYT9SXiw7jS0o/rhkj57tjO/isCJij9+YLdPNLWKUBo\n"\
"ybX5my4vOjyTsS3wGgmKwdz1L5BCHMLMcXTCMVQNH2I2e2xrE/DIQj8CgYEA3WuZ\n"\
"BYxSwGM1ygx0WDhJX5PaO9mRceMBBC9iu5NjtghsCup4m2LyDnGD1HAhI0XvHo/E\n"\
"fQXedk072FFmLn220+ge8rcvu75m/0j4G7byN1Mswq+96G0C9X4Ve/4e2NN3bjgq\n"\
"Cx8J6z9bdhcrkA4peCT3hpx2WJltjftx5YemCHkCgYB/NLSjHIyVtW5/KyYtXDEA\n"\
"mbFvFb29NorjuSGp8+hj3FoGzhsLMQaZwwg5wGBFnN1erFxOwB0eVNiS86CwImyX\n"\
"UDWWhDQi8gAoV+YlCGtcM/AzmhghXRW3Vyk1nW+Hs7OYbIG7rLY/pRwwdKBwGHgA\n"\
"GlrXxGJRlrnjxr/CmZ4lOQKBgQCpc43hJBm0aHiiz1M+rJzii3lpckEQAmUlucn6\n"\
"uXqGtf1RgU2ZxWhvy0nTi5igsQWlwurhr1sn+EWDcBAeJlGD7NG6eJ0MNlQGrOZL\n"\
"9394/at0tyBEPyETlVGF2d3rnDJ7ZHowlql1osAxKNxK27u62behCh68AXdJQJRS\n"\
"z7QuKQKBgDZsTShNR/1O5RtrnFanQxUZvC6a9DQ84h1JGgH0FMVoOqqZQgcl9irD\n"\
"AAUltkeYUkdcICbF5KtBZ/ZHVOBHJcXm58jNyyk2dSrLG6cZJXesza7MgW5a8M7Z\n"\
"36tHD3rQlkcQNDYvSW59kwK4Uz/JCJGwb/WXYagnqK0W4nqrccu2\n"\
"-----END RSA PRIVATE KEY-----\n"

/* The constants above are set to const char * pointers defined in aws_dev_mode_key_provisioning.c,
 * and externed here for use in C files.  NOTE!  THIS IS DONE FOR CONVENIENCE
 * DURING AN EVALUATION PHASE AND IS NOT GOOD PRACTICE FOR PRODUCTION SYSTEMS
 * WHICH MUST STORE KEYS SECURELY. */
extern const char clientcredentialCLIENT_CERTIFICATE_PEM[];
extern const char * clientcredentialJITR_DEVICE_CERTIFICATE_AUTHORITY_PEM;
extern const char clientcredentialCLIENT_PRIVATE_KEY_PEM[];
extern const uint32_t clientcredentialCLIENT_CERTIFICATE_LENGTH;
extern const uint32_t clientcredentialCLIENT_PRIVATE_KEY_LENGTH;

#endif /* AWS_CLIENT_CREDENTIAL_KEYS_H */
