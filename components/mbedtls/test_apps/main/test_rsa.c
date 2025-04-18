/* mbedTLS RSA functionality tests
 *
 * Focus on testing functionality where we use ESP32 hardware
 * accelerated crypto features
 *
 * SPDX-FileCopyrightText: 2021-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <string.h>
#include <stdbool.h>
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "mbedtls/rsa.h"
#include "mbedtls/pk.h"
#include "mbedtls/x509_crt.h"
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include "entropy_poll.h"
#include "freertos/FreeRTOS.h"
#include "unity.h"
#include "test_utils.h"
#include "memory_checks.h"
#include "ccomp_timer.h"

#define PRINT_DEBUG_INFO

/* Taken from openssl s_client -connect api.gigafive.com:443 -showcerts
 */
static const char *rsa4096_cert = "-----BEGIN CERTIFICATE-----\n"\
    "MIIExzCCA6+gAwIBAgIBAzANBgkqhkiG9w0BAQsFADCBkjELMAkGA1UEBhMCVVMx\n"\
    "CzAJBgNVBAgMAkNBMRQwEgYDVQQHDAtTYW50YSBDbGFyYTElMCMGA1UECgwcR2ln\n"\
    "YWZpdmUgVGVjaG5vbG9neSBQYXJ0bmVyczEZMBcGA1UEAwwQR2lnYWZpdmUgUm9v\n"\
    "dCBDQTEeMBwGCSqGSIb3DQEJARYPY2FAZ2lnYWZpdmUuY29tMB4XDTE2MDgyNzE2\n"\
    "NDYyM1oXDTI2MDgyNTE2NDYyM1owgZcxCzAJBgNVBAYTAlVTMQswCQYDVQQIDAJD\n"\
    "QTEUMBIGA1UEBwwLU2FudGEgQ2xhcmExKTAnBgNVBAoMIEdpZ2FmaXZlIFRlY2hu\n"\
    "b2xvZ3kgUGFydG5lcnMgTExDMRkwFwYDVQQDDBBhcGkuZ2lnYWZpdmUuY29tMR8w\n"\
    "HQYJKoZIhvcNAQkBFhBjcmxAZ2lnYWZpdmUuY29tMIICIjANBgkqhkiG9w0BAQEF\n"\
    "AAOCAg8AMIICCgKCAgEAof82VrEpXMpsI/ddW6RLeTeSYtxiXZZkRbDKN6otYgEk\n"\
    "vA8yRbzei2cO2A/8+Erhe9beYLAMXWF+bjoUAFwnuIcbmufgHprOYzX/7CYXCsrH\n"\
    "LrJfVF6kvjCXy2W3xSvgh8ZgHNWnBGzl13tq19Fz8x0AhK5GQ9608oJCbnQjpVSI\n"\
    "lZDl3JVOifCeXf2c7nMhVOC/reTeto0Gbchs8Ox50WyojmfYbVjOQcA7f8p1eI+D\n"\
    "XUJK01cUGVu6/KarVArGHh5LsiyXOadbyeyOXPmjyrgarG3IIBeQSNECfJZPc/OW\n"\
    "lFszjU4YLDckI4x+tReiuFQbQPN5sDplcEldmZZm/8XD36ddvAaDds+SYlPXxDK7\n"\
    "7L8RBVUG2Ylc9YZf7RE6IMDmdQmsCZDX0VxySYEmzv5lnAx4mzzaXcgS+kHMOLyK\n"\
    "n9UxmpzwQoqqC9tMZqwRaeKW1njR1dSwQLqirBPfGCWKkpkpm7C3HEfeeLrasral\n"\
    "aPf6LAwN3A4ZKHa5Jmne7W+1eYS1aTXOAOLIPcXRAh1B80H+SusIdM9d6vk2YTIg\n"\
    "khwGQV3sgM6nIO5+T/8z141UEjWbtP7pb/u0+G9Cg7TwvRoO2UukxdvOwNto1G2e\n"\
    "J3rKB/JSYsYWnPHvvh9XR+55PZ4iCf9Rqw/IP82uyGipR9gxlHqN8WhMTj9tNEkC\n"\
    "AwEAAaMhMB8wHQYDVR0OBBYEFISCemcSriz1HFhRXluw9H+Bv9lEMA0GCSqGSIb3\n"\
    "DQEBCwUAA4IBAQCMetK0xe6Y/uZpb1ARh+hHYcHI3xI+IG4opWJeoB1gDh/xpNAW\n"\
    "j6t5MGbLoqNMBXbqL26hnKVspyvCxw7ebI5ZJgjtbrD1t+0D8yrgIZzr7AWGA9Hj\n"\
    "WIHqDHGDxwkmfjVVPmuO3l5RtJmL6KV6kVL2bOvVI6gECpFLddmOTtg+iXDfSw3x\n"\
    "0+ueMYKr8QLF+TCxfzQTHvTHvOJtcZHecc1n7PYbRmI2p7tV6RoBpV69oM6NAVUV\n"\
    "i2QoSxm0pYzDzavOaxwhEPHT34Tpg6fwXy1QokFD9OtxRFtdpTjL3bMWpatZE+ba\n"\
    "cjvvf0utMW5fNjTTxu1nnpuxZM3ifTCqZJ+9\n"\
    "-----END CERTIFICATE-----\n";

static const char *rsa3072_cert = "-----BEGIN CERTIFICATE-----\n"\
    "MIIEszCCAxugAwIBAgIUNTBsyv59/rRarOVm3KBA29zqEtUwDQYJKoZIhvcNAQEL\n"\
    "BQAwaTELMAkGA1UEBhMCQ04xETAPBgNVBAgMCFNoYW5naGFpMREwDwYDVQQHDAhT\n"\
    "aGFuZ2hhaTESMBAGA1UECgwJRXNwcmVzc2lmMQwwCgYDVQQLDANJREYxEjAQBgNV\n"\
    "BAMMCWVzcHJlc3NpZjAeFw0yMDA3MTQwODQ5NDdaFw0yMTA3MTQwODQ5NDdaMGkx\n"\
    "CzAJBgNVBAYTAkNOMREwDwYDVQQIDAhTaGFuZ2hhaTERMA8GA1UEBwwIU2hhbmdo\n"\
    "YWkxEjAQBgNVBAoMCUVzcHJlc3NpZjEMMAoGA1UECwwDSURGMRIwEAYDVQQDDAll\n"\
    "c3ByZXNzaWYwggGiMA0GCSqGSIb3DQEBAQUAA4IBjwAwggGKAoIBgQDMj3ZwPd2y\n"\
    "+UxzmMUdZC5I5JQIzvUmHRNJWUe99Vht/rIEQuNSGg7xjyvuZoyeFo+Yg+QYUICa\n"\
    "Ipe4y2bZS12QsTxUmeoEhYORDSeQXFEo4aUmWuKIs6Y41dBOL7eDYDL3FRmIgmcn\n"\
    "qMonyCrSzXlcgHOVtMd8U8ifkX5u+nTigQLSIHVeAFz8CvC0tIiPm9YFurtMN15p\n"\
    "P1K/AH17ljtwVqacrI/asZgX+ECY5rauNJLigEYgfr7+xV6GofaXp6rUpGgWbVxM\n"\
    "hqKe/dbDuIzte3VK+zRDNDCeE5gPQjgoSDblOVmPemrq7KKjZ/PKmP47ct5a/0Ov\n"\
    "zWcdCgaXDRoPiwbpmz3Z6uh3JdvsDf214svLK+z4EDIRzpvggM0pfDvOADatiPkr\n"\
    "KmnFD1ZZx3R29/7IZ5OVvQL1hgWbm3cL4JADOc8PQKcqCzBE9JDdAVoa228ESaJ/\n"\
    "n4b63qaqfgBnoaFzCEruEcXj5nuXBxlk19WWtgY1tZtAgoA8hTWxxH0CAwEAAaNT\n"\
    "MFEwHQYDVR0OBBYEFPlwrvgkde/r+F8VRMMtpDUIxAtgMB8GA1UdIwQYMBaAFPlw\n"\
    "rvgkde/r+F8VRMMtpDUIxAtgMA8GA1UdEwEB/wQFMAMBAf8wDQYJKoZIhvcNAQEL\n"\
    "BQADggGBAH9nBaEP+FWyaZnmxCblKhs8eIEYXzjxbnRUPo5b3uL/PAv1XD1kEUwY\n"\
    "GWnJ7Z5HOSCdVMgo1opmKGLWuiVP6Vlt9QuA/tWh0bGScL4QfriPXuA7aXAcLbW/\n"\
    "BqHNJ9Z+H2Fq09XktkZE4Nfnv3iTMMqfNCchM3t3iWZRf2sRVYIdd5OjhM+CLLUK\n"\
    "kYNiseAgbcBX0/kqTdHlC6OS8Mcu9btJ/663DZy8tndf+PH+EB6fexQd9T31jWoj\n"\
    "OkEkJ4vDRZP+0LceK7kNcMOcLx8DnF9LwUyHQitW7NMFServoTfxy8A0yep7nIOH\n"\
    "M/ndECzirQ6WkR9jMG3cw0Jm5mZvA9IAvnLhUO45AyZGC8mShJ0AaXtqejqPg9ng\n"\
    "//5VIpzoqwVkrMYlMA7ZrccQiRsd2nlBHr+64PRwRCp7y5FOxIzhGzsJibXUpO/V\n"\
    "FNwuPz+VcnPvJE7r4gB1oRViiGYojMDQV3G+jbgvpTHKUKP6zzavSAKs+FlfEAmh\n"\
    "EtmuT/beDA==\n"\
    "-----END CERTIFICATE-----\n";

/* Root cert from openssl s_client -connect google.com:443 -showcerts
 */
static const char *rsa2048_cert = "-----BEGIN CERTIFICATE-----\n"\
    "MIIFCzCCAvOgAwIBAgIQf/AFoHxM3tEArZ1mpRB7mDANBgkqhkiG9w0BAQsFADBH\n"\
    "MQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExM\n"\
    "QzEUMBIGA1UEAxMLR1RTIFJvb3QgUjEwHhcNMjMxMjEzMDkwMDAwWhcNMjkwMjIw\n"\
    "MTQwMDAwWjA7MQswCQYDVQQGEwJVUzEeMBwGA1UEChMVR29vZ2xlIFRydXN0IFNl\n"\
    "cnZpY2VzMQwwCgYDVQQDEwNXUjIwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEK\n"\
    "AoIBAQCp/5x/RR5wqFOfytnlDd5GV1d9vI+aWqxG8YSau5HbyfsvAfuSCQAWXqAc\n"\
    "+MGr+XgvSszYhaLYWTwO0xj7sfUkDSbutltkdnwUxy96zqhMt/TZCPzfhyM1IKji\n"\
    "aeKMTj+xWfpgoh6zySBTGYLKNlNtYE3pAJH8do1cCA8Kwtzxc2vFE24KT3rC8gIc\n"\
    "LrRjg9ox9i11MLL7q8Ju26nADrn5Z9TDJVd06wW06Y613ijNzHoU5HEDy01hLmFX\n"\
    "xRmpC5iEGuh5KdmyjS//V2pm4M6rlagplmNwEmceOuHbsCFx13ye/aoXbv4r+zgX\n"\
    "FNFmp6+atXDMyGOBOozAKql2N87jAgMBAAGjgf4wgfswDgYDVR0PAQH/BAQDAgGG\n"\
    "MB0GA1UdJQQWMBQGCCsGAQUFBwMBBggrBgEFBQcDAjASBgNVHRMBAf8ECDAGAQH/\n"\
    "AgEAMB0GA1UdDgQWBBTeGx7teRXUPjckwyG77DQ5bUKyMDAfBgNVHSMEGDAWgBTk\n"\
    "rysmcRorSCeFL1JmLO/wiRNxPjA0BggrBgEFBQcBAQQoMCYwJAYIKwYBBQUHMAKG\n"\
    "GGh0dHA6Ly9pLnBraS5nb29nL3IxLmNydDArBgNVHR8EJDAiMCCgHqAchhpodHRw\n"\
    "Oi8vYy5wa2kuZ29vZy9yL3IxLmNybDATBgNVHSAEDDAKMAgGBmeBDAECATANBgkq\n"\
    "hkiG9w0BAQsFAAOCAgEARXWL5R87RBOWGqtY8TXJbz3S0DNKhjO6V1FP7sQ02hYS\n"\
    "TL8Tnw3UVOlIecAwPJQl8hr0ujKUtjNyC4XuCRElNJThb0Lbgpt7fyqaqf9/qdLe\n"\
    "SiDLs/sDA7j4BwXaWZIvGEaYzq9yviQmsR4ATb0IrZNBRAq7x9UBhb+TV+PfdBJT\n"\
    "DhEl05vc3ssnbrPCuTNiOcLgNeFbpwkuGcuRKnZc8d/KI4RApW//mkHgte8y0YWu\n"\
    "ryUJ8GLFbsLIbjL9uNrizkqRSvOFVU6xddZIMy9vhNkSXJ/UcZhjJY1pXAprffJB\n"\
    "vei7j+Qi151lRehMCofa6WBmiA4fx+FOVsV2/7R6V2nyAiIJJkEd2nSi5SnzxJrl\n"\
    "Xdaqev3htytmOPvoKWa676ATL/hzfvDaQBEcXd2Ppvy+275W+DKcH0FBbX62xevG\n"\
    "iza3F4ydzxl6NJ8hk8R+dDXSqv1MbRT1ybB5W0k8878XSOjvmiYTDIfyc9acxVJr\n"\
    "Y/cykHipa+te1pOhv7wYPYtZ9orGBV5SGOJm4NrB3K1aJar0RfzxC3ikr7Dyc6Qw\n"\
    "qDTBU39CluVIQeuQRgwG3MuSxl7zRERDRilGoKb8uY45JzmxWuKxrfwT/478JuHU\n"\
    "/oTxUFqOl2stKnn7QGTq8z29W+GgBLCXSBxC9epaHM0myFH/FJlniXJfHeytWt0=\n"\
    "-----END CERTIFICATE-----\n";


/* Some random input bytes to public key encrypt */
static const uint8_t pki_input[4096/8] = {
    0, 1, 4, 6, 7, 9, 33, 103, 49, 11, 56, 211, 67, 92 };

/* Result of an RSA4096 operation using cert's public key
   (raw PKI, no padding/etc) */
static const uint8_t pki_rsa4096_output[] = {
    0x91, 0x87, 0xcd, 0x04, 0x80, 0x7c, 0x8b, 0x0b,
    0x0c, 0xc0, 0x38, 0x37, 0x7a, 0xe3, 0x2c, 0x94,
    0xea, 0xc4, 0xcb, 0x83, 0x2c, 0x77, 0x71, 0x14,
    0x11, 0x85, 0x16, 0x61, 0xd3, 0x64, 0x2a, 0x0f,
    0xf9, 0x6b, 0x45, 0x04, 0x66, 0x5d, 0x15, 0xf1,
    0xcf, 0x69, 0x77, 0x90, 0xb9, 0x41, 0x68, 0xa9,
    0xa6, 0xfd, 0x94, 0xdc, 0x6a, 0xce, 0xc7, 0xb6,
    0x41, 0xd9, 0x44, 0x3c, 0x02, 0xb6, 0xc7, 0x26,
    0xce, 0xec, 0x66, 0x21, 0xa8, 0xe8, 0xf4, 0xa9,
    0x33, 0x4a, 0x6c, 0x28, 0x0f, 0x50, 0x30, 0x32,
    0x28, 0x00, 0xbb, 0x2c, 0xc3, 0x44, 0x72, 0x31,
    0x93, 0xd4, 0xde, 0x29, 0x6b, 0xfa, 0x31, 0xfd,
    0x3a, 0x05, 0xc6, 0xb1, 0x28, 0x43, 0x57, 0x20,
    0xf7, 0xf8, 0x13, 0x0c, 0x4a, 0x80, 0x00, 0xab,
    0x1f, 0xe8, 0x88, 0xad, 0x56, 0xf2, 0xda, 0x5a,
    0x50, 0xe9, 0x02, 0x09, 0x21, 0x2a, 0xfc, 0x82,
    0x68, 0x34, 0xf9, 0x04, 0xa3, 0x25, 0xe1, 0x0f,
    0xa8, 0x77, 0x29, 0x94, 0xb6, 0x9d, 0x5a, 0x08,
    0x33, 0x8d, 0x27, 0x6a, 0xc0, 0x3b, 0xad, 0x91,
    0x8a, 0x83, 0xa9, 0x2e, 0x48, 0xcd, 0x67, 0xa3,
    0x3a, 0x35, 0x41, 0x85, 0xfa, 0x3f, 0x61, 0x1f,
    0x80, 0xeb, 0xcd, 0x5a, 0xc5, 0x14, 0x7b, 0xab,
    0x9c, 0x45, 0x11, 0xd2, 0x25, 0x9a, 0x16, 0xeb,
    0x9c, 0xfa, 0xbe, 0x73, 0x18, 0xbd, 0x25, 0x8e,
    0x99, 0x6d, 0xb3, 0xbc, 0xac, 0x2d, 0xa2, 0x53,
    0xe8, 0x7c, 0x38, 0x1b, 0x7a, 0x75, 0xff, 0x76,
    0x4f, 0x48, 0x5b, 0x39, 0x20, 0x5a, 0x7b, 0x82,
    0xd3, 0x33, 0x33, 0x2a, 0xab, 0x6a, 0x7a, 0x42,
    0x1d, 0x1f, 0xd1, 0x61, 0x58, 0xd7, 0x38, 0x52,
    0xdf, 0xb0, 0x61, 0x98, 0x63, 0xb7, 0xa1, 0x4e,
    0xdb, 0x9b, 0xcb, 0xb7, 0x85, 0xc4, 0x3e, 0x03,
    0xe5, 0x59, 0x50, 0x28, 0x5a, 0x4d, 0x7f, 0x53,
    0x2e, 0x99, 0x1d, 0x6d, 0x85, 0x27, 0x78, 0x34,
    0x5e, 0xae, 0xc9, 0x1b, 0x37, 0x96, 0xde, 0x40,
    0x87, 0x35, 0x3c, 0x1f, 0xe0, 0x8f, 0xfb, 0x3a,
    0x58, 0x0e, 0x60, 0xe9, 0x06, 0xbd, 0x83, 0x03,
    0x92, 0xde, 0x5e, 0x69, 0x28, 0xb1, 0x00, 0xeb,
    0x44, 0xca, 0x3c, 0x49, 0x03, 0x10, 0xa8, 0x84,
    0xa6, 0xbb, 0xd5, 0xda, 0x98, 0x8c, 0x6f, 0xa3,
    0x0f, 0x39, 0xf3, 0xa7, 0x7d, 0xd5, 0x3b, 0xe2,
    0x85, 0x12, 0xda, 0xa4, 0x4d, 0x80, 0x97, 0xcb,
    0x11, 0xe0, 0x89, 0x90, 0xff, 0x5b, 0x72, 0x19,
    0x59, 0xd1, 0x39, 0x23, 0x9f, 0xb0, 0x00, 0xe2,
    0x45, 0x72, 0xc6, 0x9a, 0xbc, 0xe1, 0xd1, 0x51,
    0x6b, 0x35, 0xd2, 0x49, 0xbf, 0xb6, 0xfe, 0xab,
    0x09, 0xf7, 0x9d, 0xa4, 0x6e, 0x69, 0xb6, 0xf9,
    0xde, 0xe3, 0x57, 0x0c, 0x1a, 0x96, 0xf1, 0xcc,
    0x1c, 0x92, 0xdb, 0x44, 0xf4, 0x45, 0xfa, 0x8f,
    0x87, 0xcf, 0xf4, 0xd2, 0xa1, 0xf8, 0x69, 0x18,
    0xcf, 0xdc, 0xa0, 0x1f, 0xb0, 0x26, 0xad, 0x81,
    0xab, 0xdf, 0x78, 0x18, 0xa2, 0x74, 0xba, 0x2f,
    0xec, 0x70, 0xa2, 0x1f, 0x56, 0xee, 0xff, 0xc9,
    0xfe, 0xb1, 0xe1, 0x9b, 0xea, 0x0e, 0x33, 0x14,
    0x5f, 0x6e, 0xca, 0xee, 0x02, 0x56, 0x5a, 0x67,
    0x42, 0x9a, 0xbf, 0x55, 0xc0, 0x0f, 0x8e, 0x01,
    0x67, 0x63, 0x6e, 0xd1, 0x57, 0xf7, 0xf1, 0xc6,
    0x92, 0x9e, 0xb5, 0x45, 0xe1, 0x50, 0x58, 0x94,
    0x20, 0x90, 0x6a, 0x29, 0x2d, 0x4b, 0xd1, 0xb5,
    0x68, 0x63, 0xb5, 0xe6, 0xd8, 0x6e, 0x84, 0x80,
    0xad, 0xe6, 0x03, 0x1e, 0x51, 0xc2, 0xa8, 0x6d,
    0x84, 0xec, 0x2d, 0x7c, 0x61, 0x02, 0xd1, 0xda,
    0xf5, 0x94, 0xfa, 0x2d, 0xa6, 0xed, 0x89, 0x6a,
    0x6a, 0xda, 0x07, 0x5d, 0x83, 0xfc, 0x43, 0x76,
    0x7c, 0xca, 0x8c, 0x00, 0xfc, 0xb9, 0x2c, 0x23,
};

static const uint8_t pki_rsa3072_output[] = {
    0x86, 0xc0, 0xe4, 0xa5, 0x4b, 0x45, 0xe4, 0xd4, 0x0f, 0xb7, 0xe3, 0x10, 0x4f, 0xea, 0x88, 0x91,
    0x3d, 0xad, 0x43, 0x86, 0x90, 0xf0, 0xd8, 0xf0, 0x29, 0x21, 0xc7, 0x5c, 0x75, 0x49, 0x91, 0xce,
    0xf8, 0x34, 0x91, 0xbd, 0x89, 0x61, 0xcf, 0x47, 0x0e, 0x4d, 0x3f, 0x29, 0xd1, 0x02, 0xa7, 0xa8,
    0x8f, 0x6a, 0xda, 0x1a, 0xf2, 0xf1, 0x18, 0x92, 0x35, 0xf6, 0x0c, 0x07, 0x5a, 0x84, 0xfa, 0x65,
    0xd3, 0x02, 0xe0, 0x53, 0x17, 0x5d, 0xf7, 0x45, 0x26, 0xcc, 0xf9, 0x26, 0xf5, 0x6a, 0x66, 0xbb,
    0xef, 0x33, 0xcb, 0x03, 0x6e, 0x6a, 0x93, 0x6c, 0x2a, 0x27, 0xa7, 0xf7, 0x2c, 0xdc, 0x00, 0xdd,
    0x98, 0x52, 0xfb, 0xce, 0x31, 0xe2, 0x96, 0x20, 0x98, 0x0a, 0xf4, 0x19, 0x0f, 0xbf, 0x22, 0xed,
    0x37, 0xb2, 0x14, 0x10, 0x88, 0xa3, 0x6a, 0x43, 0x26, 0xb8, 0x54, 0xf1, 0xb8, 0xc6, 0x56, 0xb7,
    0x89, 0x34, 0xc0, 0xba, 0xae, 0x38, 0x35, 0x2c, 0x13, 0x57, 0x7a, 0xa4, 0x4b, 0xf2, 0x21, 0x82,
    0xf4, 0xea, 0x1a, 0x2c, 0xd8, 0x32, 0xe8, 0x5f, 0x37, 0x04, 0x52, 0x3d, 0xff, 0xc2, 0x85, 0x00,
    0xd2, 0x8d, 0x84, 0x36, 0x61, 0x61, 0x7b, 0xea, 0x7c, 0x3d, 0xeb, 0x51, 0xea, 0xf2, 0x67, 0xc9,
    0xb8, 0xa6, 0x98, 0x54, 0x3f, 0x5b, 0x8f, 0x1a, 0x8a, 0x93, 0x81, 0x05, 0xa3, 0x15, 0xf8, 0x54,
    0x8f, 0x75, 0xe2, 0x01, 0xc3, 0x47, 0xc3, 0x8f, 0xc7, 0x6d, 0x04, 0xbc, 0x05, 0x88, 0xd9, 0x62,
    0xcc, 0x14, 0xea, 0x30, 0x68, 0x73, 0xd5, 0xe5, 0x53, 0x7c, 0xb1, 0xa0, 0xe5, 0x6c, 0xd0, 0xa3,
    0x07, 0x2a, 0x5e, 0x2a, 0x0f, 0x89, 0x39, 0xea, 0xf9, 0xf5, 0xfb, 0x3b, 0xee, 0x66, 0xd9, 0xd4,
    0x04, 0x2d, 0x1b, 0xc9, 0xc2, 0x37, 0xc8, 0xa8, 0x71, 0xea, 0xa8, 0xf6, 0xe6, 0xc1, 0xdc, 0x5b,
    0x70, 0x68, 0x89, 0xa5, 0x69, 0xc0, 0x7f, 0x15, 0x8b, 0x6d, 0xc6, 0x88, 0x41, 0x8b, 0x25, 0x8f,
    0x2f, 0x5c, 0x81, 0x94, 0x1b, 0x8c, 0x52, 0x3f, 0xe5, 0x97, 0x6d, 0x4a, 0xc6, 0x42, 0x35, 0x0e,
    0x59, 0xce, 0x00, 0x3c, 0x2b, 0x0f, 0x5a, 0xc5, 0x1b, 0x01, 0xf3, 0x02, 0x70, 0xb1, 0x88, 0xda,
    0x7b, 0x5b, 0x4d, 0x3e, 0xd1, 0x15, 0x57, 0xc8, 0x39, 0x14, 0xff, 0x8d, 0x2b, 0x12, 0xf5, 0x5b,
    0xaf, 0x78, 0x2e, 0x0b, 0xcd, 0x27, 0x83, 0xdb, 0x4e, 0xe1, 0x5d, 0xa5, 0xbd, 0xfe, 0x2b, 0x6e,
    0x8b, 0x54, 0x7d, 0x14, 0x6f, 0x4d, 0xe1, 0x14, 0xc8, 0x30, 0x0e, 0x10, 0x23, 0x2a, 0xe1, 0xe5,
    0xee, 0xa3, 0x69, 0x8d, 0xe2, 0x9a, 0xed, 0x0c, 0x23, 0x16, 0x8e, 0x95, 0xae, 0x1a, 0xa2, 0x28,
    0x61, 0x25, 0xa2, 0x15, 0x74, 0xc4, 0xec, 0x6b, 0x73, 0xb2, 0x8c, 0xd2, 0x64, 0xfd, 0x2b, 0x92,
};

static const uint8_t pki_rsa2048_output[] = {
    0x3c, 0xd6, 0xc2, 0xbf, 0x01, 0x4a, 0x00, 0x95,
    0x2c, 0x32, 0x11, 0xc0, 0xc9, 0x7e, 0x8f, 0x0a,
    0x15, 0xee, 0xfb, 0x34, 0x1d, 0xaa, 0xae, 0x15,
    0x11, 0x6d, 0x99, 0x2b, 0x09, 0xeb, 0x3f, 0x89,
    0x46, 0x98, 0x08, 0x2f, 0x10, 0x13, 0xa1, 0x17,
    0xc7, 0xec, 0x67, 0x3a, 0x34, 0x4f, 0x40, 0xcd,
    0xe2, 0xc0, 0xbe, 0x99, 0xc7, 0xe7, 0xff, 0xea,
    0xd0, 0x82, 0xd2, 0x62, 0x73, 0xde, 0x56, 0xe8,
    0xb6, 0xa7, 0xe7, 0xe1, 0x64, 0x90, 0x00, 0x56,
    0x1d, 0x2c, 0x1c, 0xc5, 0xec, 0x7f, 0xb1, 0x87,
    0x59, 0xb1, 0xd6, 0x44, 0x0f, 0x67, 0x35, 0xb4,
    0x91, 0x49, 0xed, 0x10, 0x4c, 0xef, 0xe5, 0xc8,
    0xea, 0x0d, 0xbd, 0xaf, 0xb9, 0xad, 0x12, 0x41,
    0xaa, 0xf4, 0x68, 0x54, 0x08, 0xec, 0x70, 0x8c,
    0xac, 0x6b, 0x57, 0xcf, 0x0a, 0x0c, 0x08, 0x34,
    0x28, 0x29, 0x27, 0xa4, 0x71, 0x80, 0x43, 0x59,
    0xd9, 0x35, 0x88, 0x28, 0x1d, 0xfa, 0x0b, 0x72,
    0xa0, 0xe1, 0x03, 0x65, 0x7a, 0xf8, 0x1c, 0x76,
    0x9a, 0xad, 0x21, 0x23, 0x11, 0x2f, 0x45, 0x40,
    0x72, 0x05, 0x69, 0x1b, 0x2a, 0x74, 0x9f, 0x95,
    0x44, 0x60, 0x05, 0x6a, 0x17, 0x80, 0x4a, 0xa0,
    0xed, 0x23, 0xa6, 0xef, 0x79, 0x5d, 0x83, 0xd8,
    0x8d, 0xd8, 0xe1, 0x4c, 0x5e, 0xf8, 0xfa, 0x11,
    0x57, 0xbe, 0xca, 0x22, 0x93, 0x5b, 0xe6, 0x8b,
    0xe1, 0x31, 0xde, 0x70, 0x80, 0x4a, 0xa2, 0xd3,
    0x91, 0xe8, 0xde, 0x88, 0xa2, 0x98, 0x73, 0x49,
    0x0d, 0x26, 0xe1, 0x42, 0xd7, 0xb9, 0x5e, 0xf6,
    0x05, 0x09, 0x27, 0xc6, 0x8c, 0xc2, 0xb1, 0x53,
    0x5f, 0x19, 0xaf, 0x2b, 0xfe, 0xac, 0x6a, 0x27,
    0xde, 0x89, 0xbc, 0x72, 0x3e, 0xd5, 0x9f, 0x36,
    0xc2, 0x91, 0x68, 0x30, 0xe7, 0x76, 0x96, 0x56,
    0x8f, 0x01, 0xc4, 0x5b, 0xb7, 0xb3, 0x90, 0x7f,
};

#ifdef CONFIG_MBEDTLS_HARDWARE_MPI
/* Pregenerated RSA 4096 size keys using openssl */
static const char privkey_4096_buf[] = "-----BEGIN RSA PRIVATE KEY-----\n"
                                  "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\n"
                                  "-----END RSA PRIVATE KEY-----";

static const char privkey_2048_buf[] = "-----BEGIN RSA PRIVATE KEY-----\r\n"
    "MIIEowIBAAKCAQEA8N8hdkemvj6Tpk975/OWhv9BrTsCBCu+ZYfDb5VI7U2meKBg\r\n"
    "3dAkyyhRlY3fNwSRzBUMCzsHjpgnsB40wxOgiwlB9n6PMhq0qUVKAdCpKwFztsKd\r\n"
    "JJAsCUC+Zlwxn4RpH6ZnMl3a/njRYjuDyI32kucMP/lBRo7ks1798Gy/j+x1h5xA\r\n"
    "vZSlFoEXKjCC6S1DWhALePuZnk4m/jGP6g+YfyJXSTqsenKa/DcWndfn/JoElZ0J\r\n"
    "nhud8lBXwVe6mMheE1yqfL+VTU1nwg/TPNZrZsFz2sXig/RQCKt6LuSuzhRpsLp+\r\n"
    "BdwqEs9xrwlhZnp7j4kQBomISd6kAxQfYVROHQIDAQABAoIBAHgtO4rB8QWWPyCJ\r\n"
    "I670r7OnA2OkvzrJgHMzq2SuvPX4+gfRLMM+qDzcXugZIrdWhk+maJ3p07lnXNXY\r\n"
    "HEcAMedstQaA2n0LKfwSX/xL2TtlvBABRVoKvI3ZSaXUdcW60KBD69ULUsoICZ/T\r\n"
    "Rcr4WX+t20TH3bOQc7ayvEwKVgE95xIUpTH9asw8uOPvKxW2j5OLQgZuWrWyUDg0\r\n"
    "MFh92PhWtw3i5zq6OpTTsFJeceKYV/VstIYjZ+FslmhjQxJbr+2DJRbpHXKceqy6\r\n"
    "9yWlSV0EM7neFCHlDa2WPhK8we+6IvMiNVQKj46fHGYNBaW/ZSX7TiG5J0Uqj2e9\r\n"
    "0MUGJ8ECgYEA+frJabhfzW5+JfGjTObeznJZE6fAOjFzaBIwFu8Kz2mIjYpQlwVK\r\n"
    "EepMkv2KkrJuqS4GnI+Nkq7G0BAUyUj9tTJ3HQzvtJrxsnxVi99Yofx1s1P4YAnu\r\n"
    "c8t3ElJoQ4BRoQIs/hIvyYn22IxllBHiGESrnPQ38D82xyXQgd6S8JkCgYEA9qww\r\n"
    "j7jx6Xpy/D1Dq8Dvalm7pz3J+yHnti4w2cqZ67grUoyGnNPtciNDdfi4JzLiKkUu\r\n"
    "SDS3DacvFpFyND0m8sbpMjnR8Rvhj+bfH8KcOAowD+YR/+6vSb/P/aBt6gYXcaBn\r\n"
    "cjepx+sE81mnC7UrHb4TjG4hO5t3ZTc6X28gyCUCgYAMZn9lSisecrO5SCJUp0M4\r\n"
    "NH3stq6XdGqIKBbQnG0J2u9WLh1PUIjbGKdRx1f/bPCGXe0gCRL5yse7/IA7d+51\r\n"
    "9ZnpDAI8EE+bDgXkWWD5MB/alHjGstdsURSICSR47L2f4g6/T8GlGr3vAg/r53My\r\n"
    "xv1IXOkFdu1NtbeBKbxaSQKBgENDmw5mAVmIcXiFAEICn4ahp4EoYT6g9T2BhQKu\r\n"
    "s6BKnU2qUj7Lr5ETOp8dzqGpx3B9Yux/q3cGotmFmd3S2x8SzJ5MlAoqbyy9aRSR\r\n"
    "DeZeKNL9CuV+YcA7lOz1ZWOOe7AZbHwB38NLPBNb3CheI769iTkfAuLtNvabw8go\r\n"
    "VokdAoGBALyvBhW+Squ5tx8NOEgAisakhAVOnT6jcoeKy6FyjcvKaWagmCOCC7Gz\r\n"
    "QB9Yf1tJ+3di+aLtWWdmU494iKJHBtPMhfrYltCpxHHQGlUc/GLPY3Z5bBYYYWpb\r\n"
    "Wzw4ZvDraKlAs7a9CRwS5cpktk5ptK4rc5noSXkvV+yOT75zXat2\r\n"
    "-----END RSA PRIVATE KEY-----\r\n";

static const char privkey_3072_buf[] = "-----BEGIN RSA PRIVATE KEY-----\r\n"
    "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\r\n"
    "-----END RSA PRIVATE KEY-----\r\n";

#endif

_Static_assert(sizeof(pki_rsa2048_output) == 2048/8, "rsa2048 output is wrong size");
_Static_assert(sizeof(pki_rsa3072_output) == 3072/8, "rsa3072 output is wrong size");
_Static_assert(sizeof(pki_rsa4096_output) == 4096/8, "rsa4096 output is wrong size");

void mbedtls_mpi_printf(const char *name, const mbedtls_mpi *X);


static void test_cert(const char *cert, const uint8_t *expected_output, size_t output_len);

TEST_CASE("mbedtls RSA4096 cert", "[mbedtls]")
{

    test_cert(rsa4096_cert, pki_rsa4096_output, 4096/8);
}

TEST_CASE("mbedtls RSA3072 cert", "[mbedtls]")
{

    test_cert(rsa3072_cert, pki_rsa3072_output, 3072/8);
}

TEST_CASE("mbedtls RSA2048 cert", "[mbedtls]")
{
    test_cert(rsa2048_cert, pki_rsa2048_output, 2048/8);
}

static void test_cert(const char *cert, const uint8_t *expected_output, size_t output_len)
{
    mbedtls_x509_crt crt;
    mbedtls_rsa_context *rsa;
    char buf[output_len];
    int res;

    bzero(buf, output_len);

    mbedtls_x509_crt_init(&crt);

    TEST_ASSERT_EQUAL_HEX16_MESSAGE(0,
                                    -mbedtls_x509_crt_parse(&crt,
                                                            (const uint8_t *)cert,
                                                            strlen(cert)+1),
                                    "parse cert");

    rsa = mbedtls_pk_rsa(crt.pk);
    TEST_ASSERT_NOT_NULL(rsa);

    res = mbedtls_rsa_check_pubkey(rsa);
    TEST_ASSERT_EQUAL_HEX16_MESSAGE(0,
                                    -res,
                                    "check cert pubkey");

    mbedtls_x509_crt_info(buf, sizeof(buf), "", &crt);
    puts(buf);

    res = mbedtls_rsa_public(rsa, pki_input, (uint8_t *)buf);
    if (res == MBEDTLS_ERR_MPI_NOT_ACCEPTABLE + MBEDTLS_ERR_RSA_PUBLIC_FAILED) {
        mbedtls_x509_crt_free(&crt);
        TEST_IGNORE_MESSAGE("Hardware does not support this key length");
    }

    TEST_ASSERT_EQUAL_HEX16_MESSAGE(0,
                                    -res,
                                    "RSA PK operation");

    /*
    // Dump buffer for debugging
    for(int i = 0; i < output_len; i++) {
        printf("0x%02x, ", buf[i]);
    }
    printf("\n");
    */

    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_output, buf, output_len);

    mbedtls_x509_crt_free(&crt);
}

#ifdef CONFIG_MBEDTLS_HARDWARE_MPI
static void rsa_key_operations(int keysize, bool check_performance, bool generate_new_rsa);

static int myrand(void *rng_state, unsigned char *output, size_t len)
{
    size_t olen;
    return mbedtls_hardware_poll(rng_state, output, len, &olen);
}

#ifdef PRINT_DEBUG_INFO
static void print_rsa_details(mbedtls_rsa_context *rsa)
{
    mbedtls_mpi X[5];
    for (int i=0; i<5; ++i) {
        mbedtls_mpi_init( &X[i] );
    }

    if (0 == mbedtls_rsa_export(rsa, &X[0], &X[1], &X[2], &X[3], &X[4])) {
        for (int i=0; i<5; ++i) {
            mbedtls_mpi_printf((char*)"N\0P\0Q\0D\0E" + 2*i, &X[i]);
            mbedtls_mpi_free( &X[i] );
        }
    }
}
#endif

#if CONFIG_FREERTOS_SMP // IDF-5260
TEST_CASE("test performance RSA key operations", "[bignum][timeout=60]")
#else
TEST_CASE("test performance RSA key operations", "[bignum]")
#endif
{
    /** NOTE:
    * For ESP32-S3, CONFIG_ESP_CONSOLE_SECONDARY_USB_SERIAL_JTAG is enabled
    * by default; allocating a lock of 92 bytes, which is never freed.
    *
    * MR !18574 adds the MPI crypto lock for S3 increasing the leakage by
    * 92 bytes. This caused the RSA UT to fail with a leakage more than
    * 1024 bytes.
    *
    * The allocations made by ESP32-S2 (944 bytes) and ESP32-S3 are the same,
    * except for the JTAG lock (92 + 944 > 1024).
    */
    TEST_ESP_OK(test_utils_set_leak_level(1088, ESP_LEAK_TYPE_CRITICAL, ESP_COMP_LEAK_GENERAL));
    for (int keysize = 2048; keysize <= SOC_RSA_MAX_BIT_LEN; keysize += 1024) {
        rsa_key_operations(keysize, true, false);
    }
}

#if CONFIG_FREERTOS_SMP // IDF-5260
TEST_CASE("test RSA-3072 calculations", "[bignum][timeout=60]")
#else
TEST_CASE("test RSA-3072 calculations", "[bignum]")
#endif
{
    // use pre-genrated keys to make the test run a bit faster
    rsa_key_operations(3072, false, false);
}

#if CONFIG_FREERTOS_SMP // IDF-5260
TEST_CASE("test RSA-2048 calculations", "[bignum][timeout=60]")
#else
TEST_CASE("test RSA-2048 calculations", "[bignum]")
#endif
{
    // use pre-genrated keys to make the test run a bit faster
    rsa_key_operations(2048, false, false);
}

#if CONFIG_FREERTOS_SMP // IDF-5260
TEST_CASE("test RSA-4096 calculations", "[bignum][timeout=60]")
#else
TEST_CASE("test RSA-4096 calculations", "[bignum]")
#endif
{
    // use pre-genrated keys to make the test run a bit faster
    rsa_key_operations(4096, false, false);
}


static void rsa_key_operations(int keysize, bool check_performance, bool generate_new_rsa)
{
    mbedtls_pk_context clientkey;
    mbedtls_rsa_context rsa;
    unsigned char orig_buf[4096 / 8];
    unsigned char encrypted_buf[4096 / 8];
    unsigned char decrypted_buf[4096 / 8];
    int res = 0;

    printf("First, orig_buf is encrypted by the public key, and then decrypted by the private key\n");
    printf("keysize=%d check_performance=%d generate_new_rsa=%d\n", keysize, check_performance, generate_new_rsa);

    memset(orig_buf, 0xAA, sizeof(orig_buf));
    orig_buf[0] = 0; // Ensure that orig_buf is smaller than rsa.N
    if (generate_new_rsa) {
        mbedtls_rsa_init(&rsa);
        TEST_ASSERT_EQUAL(0, mbedtls_rsa_gen_key(&rsa, myrand, NULL, keysize, 65537));
    } else {
        mbedtls_pk_init(&clientkey);

        switch(keysize) {
        case 4096:
            res = mbedtls_pk_parse_key(&clientkey, (const uint8_t *)privkey_4096_buf, sizeof(privkey_4096_buf), NULL, 0, myrand, NULL);
            break;
        case 3072:
            res = mbedtls_pk_parse_key(&clientkey, (const uint8_t *)privkey_3072_buf, sizeof(privkey_3072_buf), NULL, 0, myrand, NULL);
            break;
        case 2048:
            res = mbedtls_pk_parse_key(&clientkey, (const uint8_t *)privkey_2048_buf, sizeof(privkey_2048_buf), NULL, 0, myrand, NULL);
            break;
        default:
            TEST_FAIL_MESSAGE("unsupported keysize, pass generate_new_rsa=true or update test");
        }

        TEST_ASSERT_EQUAL_HEX16(0, -res);

        memcpy(&rsa, mbedtls_pk_rsa(clientkey), sizeof(mbedtls_rsa_context));
    }

#ifdef PRINT_DEBUG_INFO
    print_rsa_details(&rsa);
#endif

    TEST_ASSERT_EQUAL(keysize, (int)rsa.MBEDTLS_PRIVATE(len) * 8);
    TEST_ASSERT_EQUAL(keysize, (int)rsa.MBEDTLS_PRIVATE(D).MBEDTLS_PRIVATE(n) * sizeof(mbedtls_mpi_uint) * 8); // The private exponent

#ifdef SOC_CCOMP_TIMER_SUPPORTED
    int public_perf, private_perf;
    ccomp_timer_start();
    res = mbedtls_rsa_public(&rsa, orig_buf, encrypted_buf);
    public_perf = ccomp_timer_stop();

    if (res == MBEDTLS_ERR_MPI_NOT_ACCEPTABLE + MBEDTLS_ERR_RSA_PUBLIC_FAILED) {
        mbedtls_rsa_free(&rsa);
        TEST_IGNORE_MESSAGE("Hardware does not support this key length");
    }
    TEST_ASSERT_EQUAL_HEX16(0, -res);

    ccomp_timer_start();
    res =  mbedtls_rsa_private(&rsa, myrand, NULL, encrypted_buf, decrypted_buf);
    private_perf = ccomp_timer_stop();
    TEST_ASSERT_EQUAL_HEX16(0, -res);

    if (check_performance && keysize == 2048) {
        TEST_PERFORMANCE_CCOMP_LESS_THAN(RSA_2048KEY_PUBLIC_OP, "%d us", public_perf);
        TEST_PERFORMANCE_CCOMP_LESS_THAN(RSA_2048KEY_PRIVATE_OP, "%d us", private_perf);
    } else if (check_performance && keysize == 4096) {
        TEST_PERFORMANCE_CCOMP_LESS_THAN(RSA_4096KEY_PUBLIC_OP, "%d us", public_perf);
        TEST_PERFORMANCE_CCOMP_LESS_THAN(RSA_4096KEY_PRIVATE_OP, "%d us", private_perf);
    }
#else
    res = mbedtls_rsa_public(&rsa, orig_buf, encrypted_buf);
    TEST_ASSERT_EQUAL_HEX16(0, -res);
    res =  mbedtls_rsa_private(&rsa, myrand, NULL, encrypted_buf, decrypted_buf);
    TEST_ASSERT_EQUAL_HEX16(0, -res);
    TEST_IGNORE_MESSAGE("Performance check skipped! (soc doesn't support ccomp timer)");
#endif

    TEST_ASSERT_EQUAL_MEMORY_MESSAGE(orig_buf, decrypted_buf, keysize / 8, "RSA operation");

    mbedtls_rsa_free(&rsa);
}


TEST_CASE("mbedtls RSA Generate Key", "[mbedtls][timeout=60]")
{

    mbedtls_rsa_context ctx;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;

    const unsigned int key_size = 2048;
    const int exponent = 65537;

#if CONFIG_MBEDTLS_MPI_USE_INTERRUPT && CONFIG_ESP_TASK_WDT_EN && !CONFIG_ESP_TASK_WDT_INIT
    /* Check that generating keys doesn't starve the watchdog if interrupt-based driver is used */
    esp_task_wdt_config_t twdt_config = {
        .timeout_ms = 1000,
        .idle_core_mask = (1 << 0), // Watch core 0 idle
        .trigger_panic = true,
    };
    TEST_ASSERT_EQUAL(ESP_OK, esp_task_wdt_init(&twdt_config));
#endif // CONFIG_MBEDTLS_MPI_USE_INTERRUPT && CONFIG_ESP_TASK_WDT_EN && !CONFIG_ESP_TASK_WDT_INIT

    mbedtls_rsa_init(&ctx);
    mbedtls_ctr_drbg_init(&ctr_drbg);

    mbedtls_entropy_init(&entropy);
    TEST_ASSERT_FALSE( mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, NULL, 0) );

    TEST_ASSERT_FALSE( mbedtls_rsa_gen_key(&ctx, mbedtls_ctr_drbg_random, &ctr_drbg, key_size, exponent) );

    mbedtls_rsa_free(&ctx);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);

#if CONFIG_MBEDTLS_MPI_USE_INTERRUPT && CONFIG_ESP_TASK_WDT_EN && !CONFIG_ESP_TASK_WDT_INIT
    TEST_ASSERT_EQUAL(ESP_OK, esp_task_wdt_deinit());
#endif // CONFIG_MBEDTLS_MPI_USE_INTERRUPT && CONFIG_ESP_TASK_WDT_EN && !CONFIG_ESP_TASK_WDT_INIT

}

#endif // CONFIG_MBEDTLS_HARDWARE_MPI
