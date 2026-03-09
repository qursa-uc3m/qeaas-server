/*
 * gen_mldsa44_certs.c — Generate ML-DSA-44 (FIPS 204) CA + server certs
 *                        using wolfSSL's native Dilithium implementation.
 *
 * Compile (inside Docker where wolfSSL is installed):
 *   gcc -o gen_mldsa44_certs gen_mldsa44_certs.c \
 *       -lwolfssl -I/usr/local/include -L/usr/local/lib
 *
 * Run:
 *   LD_LIBRARY_PATH=/usr/local/lib ./gen_mldsa44_certs <output_dir>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/wolfcrypt/asn_public.h>
#include <wolfssl/wolfcrypt/dilithium.h>
#include <wolfssl/wolfcrypt/random.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

/* Buffer sizes */
#define CERT_BUF_SZ   16384
#define KEY_BUF_SZ    8192

static int write_file(const char *path, const byte *buf, int sz)
{
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); return -1; }
    fwrite(buf, 1, sz, f);
    fclose(f);
    printf("  wrote %s (%d bytes)\n", path, sz);
    return 0;
}

static int write_pem_file(const char *path, const byte *der, int derSz,
                          int pemType)
{
    byte pem[CERT_BUF_SZ];
    int pemSz = wc_DerToPem(der, derSz, pem, sizeof(pem), pemType);
    if (pemSz < 0) {
        fprintf(stderr, "DerToPem failed: %d\n", pemSz);
        return pemSz;
    }
    return write_file(path, pem, pemSz);
}

int main(int argc, char **argv)
{
    const char *outdir = (argc > 1) ? argv[1] : ".";
    char path[512];
    int ret;

    /* Buffers */
    byte *caDer    = NULL, *srvDer   = NULL;
    byte *caKeyDer = NULL, *srvKeyDer = NULL;
    byte *certDer  = NULL;
    dilithium_key caKey, srvKey;
    WC_RNG rng;
    Cert cert;
    int caDerSz, srvDerSz, caKeyDerSz, srvKeyDerSz, certDerSz;

    /* Init wolfCrypt */
    wolfCrypt_Init();

    caDer     = (byte *)malloc(CERT_BUF_SZ);
    srvDer    = (byte *)malloc(CERT_BUF_SZ);
    caKeyDer  = (byte *)malloc(KEY_BUF_SZ);
    srvKeyDer = (byte *)malloc(KEY_BUF_SZ);
    certDer   = (byte *)malloc(CERT_BUF_SZ);
    if (!caDer || !srvDer || !caKeyDer || !srvKeyDer || !certDer) {
        fprintf(stderr, "malloc failed\n");
        return 1;
    }

    ret = wc_InitRng(&rng);
    if (ret) { fprintf(stderr, "InitRng: %d\n", ret); return 1; }

    /* ── 1) Generate CA key pair ────────────────────────────── */
    printf("Generating CA ML-DSA-44 key...\n");
    ret = wc_dilithium_init(&caKey);
    if (ret) { fprintf(stderr, "ca init: %d\n", ret); return 1; }
    ret = wc_dilithium_set_level(&caKey, WC_ML_DSA_44);
    if (ret) { fprintf(stderr, "ca set_level: %d\n", ret); return 1; }
    ret = wc_dilithium_make_key(&caKey, &rng);
    if (ret) { fprintf(stderr, "ca make_key: %d\n", ret); return 1; }

    /* Export CA private key to DER */
    caKeyDerSz = wc_Dilithium_PrivateKeyToDer(&caKey, caKeyDer, KEY_BUF_SZ);
    if (caKeyDerSz < 0) {
        fprintf(stderr, "ca key export: %d\n", caKeyDerSz); return 1;
    }

    /* ── 2) Make self-signed CA cert ────────────────────────── */
    printf("Creating self-signed CA cert...\n");
    wc_InitCert(&cert);
    strncpy(cert.subject.country,  "ES", CTC_NAME_SIZE);
    strncpy(cert.subject.state,    "Madrid", CTC_NAME_SIZE);
    strncpy(cert.subject.locality, "Leganes", CTC_NAME_SIZE);
    strncpy(cert.subject.org,      "UC3M", CTC_NAME_SIZE);
    strncpy(cert.subject.unit,     "GAST", CTC_NAME_SIZE);
    strncpy(cert.subject.commonName, "QEaaS ML-DSA-44 Root CA", CTC_NAME_SIZE);
    cert.isCA    = 1;
    cert.sigType = CTC_ML_DSA_LEVEL2;
    cert.daysValid = 3650;  /* 10 years */
#ifdef WOLFSSL_CERT_VERSION
    cert.version = 2;  /* v3 */
#endif

    /* Use ML_DSA_LEVEL2_TYPE (Cert_KeyType) not ML_DSA_LEVEL2k (OID sum)
     * to avoid BAD_FUNC_ARG with WC_OID_SUM_HASH (--enable-all) builds. */
    caDerSz = wc_MakeCert_ex(&cert, caDer, CERT_BUF_SZ,
                              ML_DSA_LEVEL2_TYPE, &caKey, &rng);
    if (caDerSz < 0) {
        fprintf(stderr, "MakeCert CA: %d (%s)\n", caDerSz,
                wc_GetErrorString(caDerSz));
        return 1;
    }
    /* Self-sign */
    caDerSz = wc_SignCert_ex(cert.bodySz, cert.sigType, caDer, CERT_BUF_SZ,
                             ML_DSA_LEVEL2_TYPE, &caKey, &rng);
    if (caDerSz < 0) {
        fprintf(stderr, "SignCert CA: %d (%s)\n", caDerSz,
                wc_GetErrorString(caDerSz));
        return 1;
    }

    /* Write CA cert + key */
    snprintf(path, sizeof(path), "%s/mldsa44_root_cert.pem", outdir);
    write_pem_file(path, caDer, caDerSz, CERT_TYPE);
    snprintf(path, sizeof(path), "%s/mldsa44_root_key.pem", outdir);
    write_pem_file(path, caKeyDer, caKeyDerSz, PKCS8_PRIVATEKEY_TYPE);

    /* ── 3) Generate server key pair ────────────────────────── */
    printf("Generating server ML-DSA-44 key...\n");
    ret = wc_dilithium_init(&srvKey);
    if (ret) { fprintf(stderr, "srv init: %d\n", ret); return 1; }
    ret = wc_dilithium_set_level(&srvKey, WC_ML_DSA_44);
    if (ret) { fprintf(stderr, "srv set_level: %d\n", ret); return 1; }
    ret = wc_dilithium_make_key(&srvKey, &rng);
    if (ret) { fprintf(stderr, "srv make_key: %d\n", ret); return 1; }

    srvKeyDerSz = wc_Dilithium_PrivateKeyToDer(&srvKey, srvKeyDer, KEY_BUF_SZ);
    if (srvKeyDerSz < 0) {
        fprintf(stderr, "srv key export: %d\n", srvKeyDerSz); return 1;
    }

    /* ── 4) Make server cert signed by CA ───────────────────── */
    printf("Creating server cert (signed by CA)...\n");
    wc_InitCert(&cert);
    strncpy(cert.subject.country,    "ES", CTC_NAME_SIZE);
    strncpy(cert.subject.state,      "Madrid", CTC_NAME_SIZE);
    strncpy(cert.subject.locality,   "Leganes", CTC_NAME_SIZE);
    strncpy(cert.subject.org,        "UC3M", CTC_NAME_SIZE);
    strncpy(cert.subject.unit,       "GAST", CTC_NAME_SIZE);
    strncpy(cert.subject.commonName, "QEaaS CoAP Proxy", CTC_NAME_SIZE);
    cert.isCA    = 0;
    cert.sigType = CTC_ML_DSA_LEVEL2;
    cert.daysValid = 1095;  /* 3 years */
#ifdef WOLFSSL_CERT_VERSION
    cert.version = 2;  /* v3 */
#endif

#ifdef WOLFSSL_ALT_NAMES
    /* SAN: IP addresses + DNS */
    {
        /* IP:127.0.0.1 */
        byte ip4_lo[] = { 127, 0, 0, 1 };
        ret = wc_SetAltNames(&cert, NULL);  /* clear */
        /* Use raw extension approach for IP SANs */
    }
#endif

    /* Set issuer from the CA cert DER */
    ret = wc_SetIssuerBuffer(&cert, caDer, caDerSz);
    if (ret) {
        fprintf(stderr, "SetIssuerBuffer: %d (%s)\n", ret,
                wc_GetErrorString(ret));
        return 1;
    }

    certDerSz = wc_MakeCert_ex(&cert, certDer, CERT_BUF_SZ,
                                ML_DSA_LEVEL2_TYPE, &srvKey, &rng);
    if (certDerSz < 0) {
        fprintf(stderr, "MakeCert srv: %d (%s)\n", certDerSz,
                wc_GetErrorString(certDerSz));
        return 1;
    }
    /* Sign with CA key */
    certDerSz = wc_SignCert_ex(cert.bodySz, cert.sigType, certDer, CERT_BUF_SZ,
                               ML_DSA_LEVEL2_TYPE, &caKey, &rng);
    if (certDerSz < 0) {
        fprintf(stderr, "SignCert srv: %d (%s)\n", certDerSz,
                wc_GetErrorString(certDerSz));
        return 1;
    }

    /* Write server cert + key */
    snprintf(path, sizeof(path), "%s/mldsa44_entity_cert.pem", outdir);
    write_pem_file(path, certDer, certDerSz, CERT_TYPE);
    snprintf(path, sizeof(path), "%s/mldsa44_entity_key.pem", outdir);
    write_pem_file(path, srvKeyDer, srvKeyDerSz, PKCS8_PRIVATEKEY_TYPE);

    printf("Done! ML-DSA-44 certs written to %s/\n", outdir);

    /* Cleanup */
    wc_dilithium_free(&caKey);
    wc_dilithium_free(&srvKey);
    wc_FreeRng(&rng);
    free(caDer); free(srvDer); free(caKeyDer); free(srvKeyDer); free(certDer);
    wolfCrypt_Cleanup();

    return 0;
}
