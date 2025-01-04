#pragma once

#include <QSharedPointer>
#include <QObject>
#include <QtNetwork>


enum class ProtocolState
{
    IDLE,
    CONNECTING_TCP,
    CONNECTING_SSL,
    READING_HEADER,
    READING_BODY,
    ERROR_STATUS_CODE,
    ERROR_WRONG_HEADER,
    ERROR_WRONG_STATUS,
    DONE
};

enum class GeminiStatusDecade
{
    STATUS_0x_UNKNOWN,
    STATUS_1x_INPUT,
    STATUS_2x_SUCCESS,
    STATUS_3x_REDIRECT,
    STATUS_4x_TEMPORARY_FAILURE,
    STATUS_5x_PERMANENT_FAILURE,
    STATUS_6x_CLIENT_CERTIFICATE_REQUIRED,
    COUNT
};

struct CertData
{
    bool isTemp{ true };
    QSslCertificate cert;
    QSslKey key;
};

class Engine : public QObject
{
Q_OBJECT
Q_PROPERTY(bool retrievalInProgrtess READ retrievalInProgress NOTIFY retrievalInProgressChanged)    

public:
    Engine();
    Engine(Engine &&e) = delete;
    Engine(Engine &e) = delete;
    virtual ~Engine();
    
    Q_INVOKABLE static QString statusToString(int status);
    
public Q_SLOTS:
    void retrieve(QString url);
    bool loadTempCertificate(QString hostUrl);
    
    bool retrievalInProgress() { return m_sock != nullptr; }
    
Q_SIGNALS:
    void retrievalInProgressChanged(bool value);
    void retrievalDone(bool ok, QString url, QString document);
    void inputRequested(QString forUrl);
    void certificateRequested(QString forUrl, QString details);
    
    void willConnect();
    void encryptionEstablished();
    
private:
    void returnClientError(QString reason, QString url);
    void returnHome();
    
    QSslSocket *m_sock{ nullptr };
    QMap<QString, CertData> m_certs;
    ProtocolState m_state{ ProtocolState::IDLE };
    QString m_meta;
    QByteArray m_payload;
    int m_status{ 0 };

};
