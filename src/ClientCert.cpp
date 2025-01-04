#include "ClientCert.h"

#include "openssl/x509.h"
#include "openssl/rsa.h"
#include "openssl/pem.h"

QPair<QSslCertificate, QSslKey> createTempClientCert(QString host)
{
    QByteArray hostBytes = host.toUtf8();
    
    EVP_PKEY * pkey;
    pkey = EVP_PKEY_new();
    
    RSA * rsa;
    rsa = RSA_generate_key(
        2048,   /* number of bits for the key - 2048 is a sensible value */
        RSA_F4, /* exponent - RSA_F4 is defined as 0x10001L */
        NULL,   /* callback - can be NULL if we aren't displaying progress */
        NULL    /* callback argument - not needed in this case */
    );
    
    EVP_PKEY_assign_RSA(pkey, rsa); // delete rsa when pkey is freed
    
    X509 * x509;
    x509 = X509_new();
    
    ASN1_INTEGER_set(X509_get_serialNumber(x509), 1);
    
    X509_gmtime_adj(X509_get_notBefore(x509), 0);
    X509_gmtime_adj(X509_get_notAfter(x509), 31536000L);
    
    X509_set_pubkey(x509, pkey);
    
    // set certificate subject details
    X509_NAME * name;
    name = X509_get_subject_name(x509);
    X509_NAME_add_entry_by_txt(name, "C",  MBSTRING_ASC,
                           (unsigned char *)"BR", -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "O",  MBSTRING_ASC,
                           (unsigned char *)hostBytes.data(), -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC,
                           (unsigned char *)hostBytes.data(), -1, -1, 0);
    X509_set_issuer_name(x509, name);
    
    X509_sign(x509, pkey, EVP_sha1());
    
    BIO *key_mem = BIO_new(BIO_s_mem());  
    BIO *cert_mem = BIO_new(BIO_s_mem());
    
    PEM_write_bio_PrivateKey(
        key_mem,            // write the key to the buffer
        pkey,               // our key from earlier
        NULL, // default cipher for encrypting the key in memory
        NULL,      // passphrase required for decrypting the key in memory
        0,                 // length of the passphrase string
        NULL,               // callback for requesting a password
        NULL                // data to pass to the callback
    );
    
    PEM_write_bio_X509(
        cert_mem,   /* write the certificate to the buffer */
        x509        /* our certificate */
    );
    
    EVP_PKEY_free(pkey);
    X509_free(x509);
    
    char *cert_ptr;
    long cert_len = BIO_get_mem_data(cert_mem, &cert_ptr);
    QByteArray cert_data = QByteArray::fromRawData(cert_ptr, cert_len);
    QSslCertificate ret_1 { cert_data };
    
    char *key_ptr;
    long key_len = BIO_get_mem_data(key_mem, &key_ptr);
    QByteArray key_data = QByteArray::fromRawData(key_ptr, key_len);
    QSslKey ret_2 { key_data, QSsl::Rsa };
    
    BIO_free_all(key_mem);
    BIO_free_all(cert_mem);
    
    return {ret_1, ret_2};
}
