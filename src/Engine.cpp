#include "./Engine.h"
#include "./ClientCert.h"
#include "./Formatter.h"
#include <cctype>

static constexpr int DEFAULT_PORT = 1965;
static constexpr int HEADER_MAX_SIZE = 1030; // meta str of max 1024 + 2 status digits + 1 space + 3 bytes (cr, lf, nul)

Engine::Engine()
{
    
}

Engine::~Engine()
{
    m_sock = nullptr;
}

void Engine::retrieve(QString urlStr)
{
    QUrl url{ urlStr };
    qDebug() << "Requesting: " << url;
    
    if(!url.isValid())
    {
        returnClientError(tr("The URL '%1' is invalid.").arg(urlStr), urlStr);
        return;
    }
    
    if ( url.scheme() == QStringLiteral("gemini") )
    {
        if(m_sock)
        {
            
            qDebug() << "Canceling currently ongoing request.";
            // cancel current request
            m_sock->close();
            if(m_sock)
            {
                qDebug() << "Socket remained, freeing...";
                m_sock->deleteLater();
                m_sock = nullptr;
            }
            else
                qDebug() << "Socket is gone.";
        }
        
        m_state = ProtocolState::IDLE;
        m_payload.clear();
        m_meta.clear();
        m_status = 0;
        
        m_sock = new QSslSocket(nullptr);
        m_sock->setPeerVerifyMode( QSslSocket::QueryPeer );
        
        // when we have no certificate in store, don't generate one by default
        if(m_certs.contains(url.host()))
        {
            CertData *certData = &(m_certs[url.host()]);
            if(certData->isTemp && certData->cert.effectiveDate().addDays(1) < QDateTime::currentDateTimeUtc())
            {
                // a new cert needs to be generated
                loadTempCertificate(urlStr);
                certData = &(m_certs[url.host()]);
            }
            
            // now we should have a proper client certificate, let's use it
            qDebug() << "Using key and cert:";
            qDebug() << certData->key << ", isNull: " << certData->key.isNull();
            qDebug() << certData->cert << ", isNull: " << certData->cert.isNull();
            m_sock->setLocalCertificate(certData->cert);
            m_sock->setPrivateKey(certData->key);
        }
        
        QObject::connect(m_sock, &QSslSocket::encrypted, [this, url](){
            qDebug() << "Established SSL connection for: " << url;
            Q_EMIT encryptionEstablished();
            QByteArray request = (url.toString() + QStringLiteral("\r\n")).toUtf8();
            m_sock->write(request);
            m_state = ProtocolState::READING_HEADER;
        });
        QObject::connect(m_sock, &QSslSocket::connected, [this, url](){
            qDebug() << "Connected to: " << url;
            m_state = ProtocolState::CONNECTING_SSL;
        });
        QObject::connect(m_sock, &QIODevice::readyRead, [this, urlStr](){
            qDebug() << "On readyRead";
            if(m_state == ProtocolState::READING_HEADER)
            {
                if(!m_sock->canReadLine())
                    return; // wait for a line to be readable or an error
                    
                char headerLine[HEADER_MAX_SIZE + 1];
                qint64 readCount = m_sock->readLine(headerLine, sizeof(headerLine));
                
                if(readCount > HEADER_MAX_SIZE)
                {
                    m_state = ProtocolState::ERROR_WRONG_HEADER;
                    returnClientError(tr("Header is too long."), urlStr);
                    m_sock->close();
                    return;
                }
                else if (readCount < 5)
                {
                    m_state = ProtocolState::ERROR_WRONG_HEADER;
                    returnClientError(tr("Header is too short."), urlStr);
                    m_sock->close();
                    return;
                }
                
                if(isdigit(headerLine[0]) && isdigit(headerLine[1]) && headerLine[2] == ' ' && headerLine[readCount-1] == '\n' && headerLine[readCount-2] == '\r')
                {
                    qDebug() << "Header: " << headerLine;
                    m_status = 10*(headerLine[0]-'0') + headerLine[1] - '0';
                    if( readCount == 5 )
                        m_meta.clear();
                    else
                        m_meta = QString::fromUtf8(QByteArray::fromRawData(headerLine + 3, readCount - 5));
                }
                else
                {
                    m_state = ProtocolState::ERROR_WRONG_HEADER;
                    returnClientError(tr("Malformed header."), urlStr);
                    m_sock->close();
                    return;
                }
                
                // valid header, state and meta are filled in
                int decade_raw = m_status / 10;
                if(decade_raw >= static_cast<int>(GeminiStatusDecade::COUNT) || decade_raw <= 0)
                {
                    m_state = ProtocolState::ERROR_WRONG_STATUS;
                    returnClientError(tr("Unknown status code: %1.").arg(m_status), urlStr);
                    m_sock->close();
                    return;
                }
                
                GeminiStatusDecade decade = static_cast<GeminiStatusDecade>(decade_raw);
                qDebug() << "Got status" << m_status << "for : " << urlStr;
                qDebug() << "    meta is:" << m_meta<<", decade is:" << static_cast<int>(decade);
                
                switch(decade)
                {
                    case GeminiStatusDecade::STATUS_2x_SUCCESS:
                        m_state = ProtocolState::READING_BODY;
                        qDebug() << "Reading body: " << urlStr;
                        break;
                        
                    case GeminiStatusDecade::STATUS_6x_CLIENT_CERTIFICATE_REQUIRED:
                        qDebug() << "Reporting certificate was requested." << urlStr;
                        m_state = ProtocolState::DONE;
                        m_sock->close();
                        Q_EMIT certificateRequested(urlStr, m_meta);
                        break;
                        
                    default:
                        m_state = ProtocolState::ERROR_STATUS_CODE;
                        returnClientError(tr("Cannot handle status (yet?): %1.").arg(m_status), urlStr);
                        m_sock->close();
                }
            }
            
            if(m_state == ProtocolState::READING_BODY)
            {
                QByteArray data = m_sock->readAll();
                m_payload += data;
                qDebug() << "Received" << data.size() << "bytes.";
            }
        });
        QObject::connect(m_sock, &QSslSocket::disconnected, [this, urlStr](){
            qDebug() << "Disconnected: " << urlStr;
            if(m_state == ProtocolState::ERROR_STATUS_CODE)
            {
                returnClientError(tr("Ended with error code %1: %2.").arg(m_status).arg(Engine::statusToString(m_status)), urlStr);
            }
            else if(m_state == ProtocolState::READING_BODY)
            {
                // was reading body, read the rest and dispatch
                m_payload += m_sock->readAll();
                if(m_meta.startsWith(QStringLiteral("text/gemini")))
                    Q_EMIT retrievalDone(false, urlStr, gemtextToHTML(QString::fromUtf8(m_payload)));
                else if(m_meta.startsWith(QStringLiteral("text/plain")))
                    Q_EMIT retrievalDone(false, urlStr, QStringLiteral("<pre>%1</pre>").arg(QString::fromUtf8(m_payload)));
                else
                    qDebug() << "Something weird on the pipe...";
            }
            else if(m_state != ProtocolState::DONE)
            {
                returnClientError(tr("Socket disconnected before done. Status is %1.").arg(m_status), urlStr);
            }
            
            m_sock->deleteLater();
            m_sock = nullptr; // remove connection
        });
        
        Q_EMIT willConnect();
        m_state = ProtocolState::CONNECTING_TCP;
        m_sock->connectToHostEncrypted(url.host(), url.port(DEFAULT_PORT) );
        
    }
    else if( url.scheme() == QStringLiteral("about") )
    {
        if( url.host() == QStringLiteral("home") )
        {
            returnHome();
        }
        else
            returnClientError(tr("Unknown about identifier."), urlStr);
    }
    else
        returnClientError(tr("Unknown URL scheme: '%1'.").arg(url.scheme()), urlStr);
}


bool Engine::loadTempCertificate(QString hostUrlStr )
{
    QUrl hostUrl( hostUrlStr );
    
    if (!hostUrl.isValid())
    {
        qDebug() << "[Engine] loadTempCertificate: cannot parse url: " << hostUrlStr;
        return false;
    }
    
    QString host = hostUrl.host();
    qDebug() << "[Engine] loadTempCertificate: creating temp certificate for this session, for host: " << host;
    
    QByteArray key; 
    auto pair = createTempClientCert(host);
    qDebug() << "[Engine] loadTempCertificate: got pair: " << pair;
    m_certs[host] = CertData { true, pair.first, pair.second };
    
    return true;
}

void Engine::returnClientError(QString reason, QString url)
{
    Q_EMIT retrievalDone(false, url, tr("<h1>Client Error</h1><p>%1</p>").arg(reason));
}
    
void Engine::returnHome()
{
    Q_EMIT retrievalDone(true, QStringLiteral("about:home"), tr("<h1>Komparsa Home</h1><p>Under construction.</p>"));
}

QString Engine::statusToString(int status)
{
    return QStringLiteral("(TODO status string: %1)").arg(status);
}
