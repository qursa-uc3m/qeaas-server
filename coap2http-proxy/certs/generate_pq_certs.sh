#!/bin/bash

# Script to generate Falcon NIST Level 1 and 5 certificate chains.
#
# Copyright 2021 wolfSSL Inc. All rights reserved.
# Original Author: Anthony Hu.
#
# Execute this script in the openssl directory after building OQS's fork of
# OpenSSL. Please see the README.md file for more details.

if [ "$OPENSSL" = "" ]; then
   #OPENSSL=/usr/local/bin/oqs_openssl
   OPENSSL=/usr/local/bin/oqs_openssl3
fi
# add provider path if not defined
if [ "$PROVIDER_PATH" = "" ]; then
   PROVIDER_PATH=/opt/oqs_openssl3/oqs-provider/_build/lib
fi

# Generate conf files.
printf "\
[ req ]\n\
prompt                 = no\n\
distinguished_name     = req_distinguished_name\n\
\n\
[ req_distinguished_name ]\n\
C                      = CA\n\
ST                     = ON\n\
L                      = Waterloo\n\
O                      = wolfSSL Inc.\n\
OU                     = Engineering\n\
CN                     = Root Certificate\n\
emailAddress           = root@wolfssl.com\n\
\n\
[ ca_extensions ]\n\
subjectKeyIdentifier   = hash\n\
authorityKeyIdentifier = keyid:always,issuer:always\n\
keyUsage               = critical, keyCertSign\n\
basicConstraints       = critical, CA:true\n" > root.conf

printf "\
[ req ]\n\
prompt                 = no\n\
distinguished_name     = req_distinguished_name\n\
\n\
[ req_distinguished_name ]\n\
C                      = CA\n\
ST                     = ON\n\
L                      = Waterloo\n\
O                      = wolfSSL Inc.\n\
OU                     = Engineering\n\
CN                     = Entity Certificate\n\
emailAddress           = entity@wolfssl.com\n\
\n\
[ x509v3_extensions ]\n\
subjectAltName = IP:127.0.0.1\n\
subjectKeyIdentifier   = hash\n\
authorityKeyIdentifier = keyid:always,issuer:always\n\
keyUsage               = critical, digitalSignature\n\
extendedKeyUsage       = critical, serverAuth,clientAuth\n\
basicConstraints       = critical, CA:false\n" > entity.conf

###############################################################################
# ML-DSA-44 (Dilithium2 — NIST Level 2, smallest)
###############################################################################

echo "Generating ML-DSA-44 (dilithium2) keys..."
${OPENSSL} genpkey -algorithm dilithium2 -outform pem -out dilithium2_root_key.pem -provider-path ${PROVIDER_PATH} -provider oqsprovider -provider default
${OPENSSL} genpkey -algorithm dilithium2 -outform pem -out dilithium2_entity_key.pem -provider-path ${PROVIDER_PATH} -provider oqsprovider -provider default

echo "Generating ML-DSA-44 root certificate..."
${OPENSSL} req -x509 -config root.conf -extensions ca_extensions -days 1095 -set_serial 200 -key dilithium2_root_key.pem -out dilithium2_root_cert.pem -provider-path ${PROVIDER_PATH} -provider oqsprovider -provider default

echo "Generating ML-DSA-44 entity CSR..."
${OPENSSL} req -new -config entity.conf -key dilithium2_entity_key.pem -out dilithium2_entity_req.pem -provider-path ${PROVIDER_PATH} -provider oqsprovider -provider default

echo "Generating ML-DSA-44 entity certificate..."
${OPENSSL} x509 -req -in dilithium2_entity_req.pem -CA dilithium2_root_cert.pem -CAkey dilithium2_root_key.pem -extfile entity.conf -extensions x509v3_extensions -days 1095 -set_serial 201 -out dilithium2_entity_cert.pem -provider-path ${PROVIDER_PATH} -provider oqsprovider -provider default

###############################################################################
# Dilithium3
###############################################################################

echo "Generating DILITHIUM3 keys..."
${OPENSSL} genpkey -algorithm dilithium3 -outform pem -out dilithium3_root_key.pem -provider-path ${PROVIDER_PATH} -provider oqsprovider -provider default
${OPENSSL} genpkey -algorithm dilithium3 -outform pem -out dilithium3_entity_key.pem -provider-path ${PROVIDER_PATH} -provider oqsprovider -provider default

echo "Generating DILITHIUM3 Level 1 root certificate..."
${OPENSSL} req -x509 -config root.conf -extensions ca_extensions -days 1095 -set_serial 512 -key dilithium3_root_key.pem -out dilithium3_root_cert.pem -provider-path ${PROVIDER_PATH} -provider oqsprovider -provider default

echo "Generating DILITHIUM3 Level 1 entity CSR..."
${OPENSSL} req -new -config entity.conf -key dilithium3_entity_key.pem -out dilithium3_entity_req.pem -provider-path ${PROVIDER_PATH} -provider oqsprovider -provider default

echo "Generating DILITHIUM3 Level 1 entity certificate..."
${OPENSSL} x509 -req -in dilithium3_entity_req.pem -CA dilithium3_root_cert.pem -CAkey dilithium3_root_key.pem -extfile entity.conf -extensions x509v3_extensions -days 1095 -set_serial 513 -out dilithium3_entity_cert.pem -provider-path ${PROVIDER_PATH} -provider oqsprovider -provider default

###############################################################################
# Falcon NIST Level 1
###############################################################################

# Generate root key and entity private keys.
echo "Generating Falcon NIST Level 1 keys..."
${OPENSSL} genpkey -algorithm falcon512 -outform pem -out falcon_level1_root_key.pem -provider-path ${PROVIDER_PATH} -provider oqsprovider -provider default
${OPENSSL} genpkey -algorithm falcon512 -outform pem -out falcon_level1_entity_key.pem -provider-path ${PROVIDER_PATH} -provider oqsprovider -provider default

# Generate the root certificate
echo "Generating Falcon NIST Level 1 root certificate..."
${OPENSSL} req -x509 -config root.conf -extensions ca_extensions -days 1095 -set_serial 512 -key falcon_level1_root_key.pem -out falcon_level1_root_cert.pem -provider-path ${PROVIDER_PATH} -provider oqsprovider -provider default

# Generate the entity CSR.
echo "Generating Falcon NIST Level 1 entity CSR..."
${OPENSSL} req -new -config entity.conf -key falcon_level1_entity_key.pem -out falcon_level1_entity_req.pem -provider-path ${PROVIDER_PATH} -provider oqsprovider -provider default

# Generate the entity X.509 certificate.
echo "Generating Falcon NIST Level 1 entity certificate..."
${OPENSSL} x509 -req -in falcon_level1_entity_req.pem -CA falcon_level1_root_cert.pem -CAkey falcon_level1_root_key.pem -extfile entity.conf -extensions x509v3_extensions -days 1095 -set_serial 513 -out falcon_level1_entity_cert.pem -provider-path ${PROVIDER_PATH} -provider oqsprovider -provider default

###############################################################################
# Falcon NIST Level 5
###############################################################################

# Generate root key and entity private keys.
echo "Generating Falcon NIST Level 5 keys..."
${OPENSSL} genpkey -algorithm falcon1024 -outform pem -out falcon_level5_root_key.pem -provider-path ${PROVIDER_PATH} -provider oqsprovider -provider default
${OPENSSL} genpkey -algorithm falcon1024 -outform pem -out falcon_level5_entity_key.pem -provider-path ${PROVIDER_PATH} -provider oqsprovider -provider default

# Generate the root certificate
echo "Generating Falcon NIST Level 5 root certificate..."
${OPENSSL} req -x509 -config root.conf -extensions ca_extensions -days 1095 -set_serial 1024 -key falcon_level5_root_key.pem -out falcon_level5_root_cert.pem -provider-path ${PROVIDER_PATH} -provider oqsprovider -provider default

# Generate the entity CSR.
echo "Generating Falcon NIST Level 5 entity CSR..."
${OPENSSL} req -new -config entity.conf -key falcon_level5_entity_key.pem -out falcon_level5_entity_req.pem -provider-path ${PROVIDER_PATH} -provider oqsprovider -provider default

# Generate the entity X.509 certificate.
echo "Generating Falcon NIST Level 5 entity certificate..."
${OPENSSL} x509 -req -in falcon_level5_entity_req.pem -CA falcon_level5_root_cert.pem -CAkey falcon_level5_root_key.pem -extfile entity.conf -extensions x509v3_extensions -days 1095 -set_serial 1025 -out falcon_level5_entity_cert.pem -provider-path ${PROVIDER_PATH} -provider oqsprovider -provider default

echo "Generating RSA 2048 keys..."
${OPENSSL} genpkey -algorithm RSA -out rsa_2048_root_key.pem
${OPENSSL} genpkey -algorithm RSA -out rsa_2048_entity_key.pem

# Generate the root certificate
echo "Generating RSA 2048 root certificate..."
${OPENSSL} req -x509 -config root.conf -extensions ca_extensions -days 1095 -set_serial 512 -key rsa_2048_root_key.pem -out rsa_2048_root_cert.pem

# Generate the entity CSR.
echo "Generating RSA 2048 entity CSR..."
${OPENSSL} req -new -config entity.conf -key rsa_2048_entity_key.pem -out rsa_2048_entity_req.pem

# Generate the entity X.509 certificate.
echo "Generating RSA 2048 entity certificate..."
${OPENSSL} x509 -req -in rsa_2048_entity_req.pem -CA rsa_2048_root_cert.pem -CAkey rsa_2048_root_key.pem -extfile entity.conf -extensions x509v3_extensions -days 1095 -set_serial 513 -out rsa_2048_entity_cert.pem

###############################################################################
# Verify all generated certificates.
###############################################################################
echo "Verifying certificates..."
${OPENSSL} verify -no-CApath -check_ss_sig -provider-path ${PROVIDER_PATH} -provider oqsprovider -provider default -CAfile dilithium2_root_cert.pem dilithium2_entity_cert.pem
${OPENSSL} verify -no-CApath -check_ss_sig -provider-path ${PROVIDER_PATH} -provider oqsprovider -provider default -CAfile dilithium3_root_cert.pem dilithium3_entity_cert.pem
${OPENSSL} verify -no-CApath -check_ss_sig -provider-path ${PROVIDER_PATH} -provider oqsprovider -provider default -CAfile falcon_level1_root_cert.pem falcon_level1_entity_cert.pem
${OPENSSL} verify -no-CApath -check_ss_sig -provider-path ${PROVIDER_PATH} -provider oqsprovider -provider default -CAfile falcon_level5_root_cert.pem falcon_level5_entity_cert.pem
${OPENSSL} verify -no-CApath -check_ss_sig -CAfile rsa_2048_root_cert.pem rsa_2048_entity_cert.pem