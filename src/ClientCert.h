#pragma once

#include <QSslCertificate>
#include <QSslKey>

QPair<QSslCertificate, QSslKey> createTempClientCert(QString host);
